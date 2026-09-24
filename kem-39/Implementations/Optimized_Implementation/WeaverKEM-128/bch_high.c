# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <stdint.h>
# include "params.h"  // <--- 引入我们自己架构的参数
# include "bch.h"

#if WEAVER_MODE == 1
    #include "bch127_112_2.h"
#elif WEAVER_MODE == 3
    #include "bch255_220_4.h"
#elif WEAVER_MODE == 5
    #include "bch511_448_7.h"
#else
    #error "Invalid WEAVER_MODE for BCH configuration"
#endif

// convert 32-bit ecc words to ecc bytes
static void store_ecc8(uint8_t *dst, const uint32_t *src)
{
    uint8_t pad[4];
    unsigned int i, nwords = BCH_ECC_WORDS-1;

    for (i = 0; i < nwords; i++) {
        *dst++ = (src[i] >> 24);
        *dst++ = (src[i] >> 16) & 0xff;
        *dst++ = (src[i] >>  8) & 0xff;
        *dst++ = (src[i] >>  0) & 0xff;
    }
    pad[0] = (src[nwords] >> 24);
    pad[1] = (src[nwords] >> 16) & 0xff;
    pad[2] = (src[nwords] >>  8) & 0xff;
    pad[3] = (src[nwords] >>  0) & 0xff;
    memcpy(dst, pad, BCH_ECC_BYTES-4*nwords);
}

// bch encode
void encode_bch_high(const unsigned char *data, unsigned int len, uint8_t *ecc)
{
    int i;
    const uint32_t *p;
    const int l = BCH_ECC_WORDS-1;
    uint32_t ecc_buf[BCH_ECC_WORDS];
    
    memset(ecc_buf,0,BCH_ECC_WORDS*sizeof(uint32_t));

    while (len--) 
    {
        p = mod8_tab_half + (l+1)*(((ecc_buf[0] >> 28)^((*data)>>4)) & 0x0f);
        for (i = 0; i < l; i++)
            ecc_buf[i] = ((ecc_buf[i] << 4)|(ecc_buf[i+1] >> 28))^(*p++);
        ecc_buf[l] = (ecc_buf[l] << 4)^(*p);
        
        p = mod8_tab_half + (l+1)*(((ecc_buf[0] >> 28)^(*data++)) & 0x0f);
        for (i = 0; i < l; i++)
            ecc_buf[i] = ((ecc_buf[i] << 4)|(ecc_buf[i+1] >> 28))^(*p++);
        ecc_buf[l] = (ecc_buf[l] << 4)^(*p);
    }
    
    store_ecc8(ecc,ecc_buf);    
}

//shorter and faster modulo function, only works when v < 2N.
static inline int mod_s(unsigned int v)
{
    unsigned int tmp;
    tmp=(v<bch.n)?0x0:0xffffffff;
    
    return v-(bch.n&tmp);
}

//Galois field basic operations: multiply, divide, inverse, etc.
static inline unsigned int gf_mul(unsigned int a, unsigned int b)
{
    unsigned int tmp,mask;
    
    tmp=a_pow_tab[mod_s(a_log_tab[a]+a_log_tab[b])];
    mask=(a && b)? 0xffffffff:0x0;
    
    return  tmp&mask;
}

static inline unsigned int gf_sqr(unsigned int a)
{
    unsigned int tmp,mask;
    
    tmp=a_pow_tab[mod_s( 2*a_log_tab[a])];
    mask=(a)? 0xffffffff:0x0;
    
    return tmp&mask;
}

static inline int a_log(unsigned int x)
{
    return a_log_tab[x];
}

// compute 2t syndromes of ecc polynomial, i.e. ecc(a^j) for j=1..2t
static void compute_syndromes(uint8_t *ecc, unsigned int *syn)
{
    int i, j, s;
    unsigned int m;
    uint32_t poly,mask_syn,syn_tmp;
    const int t = bch.t;
    unsigned int w,w2;

    s = bch.ecc_bits;
    //make sure extra bits in last ecc byte are cleared 
    m = s & 7;
    if (m)
        ecc[s/8] &= ~((1u << (8-m))-1);
        
    memset(syn, 0, 2*t*sizeof(*syn));

    //compute v(a^j) for j=1 .. 2t-1 
    do {
        poly = *ecc++;
        s -= 8;
        
        for(i=7;i>=0 && i+s>=0;i--)
        {
            mask_syn=((poly>>i)&0x1)?0xffffffff:0x0;
            w=i+s;
            w2=w*2;
            for (j = 0; j < 2*t; j += 2)
            {
                syn_tmp=a_pow_tab[w];
                syn[j] ^= (syn_tmp&mask_syn);
                w=mod_s(w+w2);
            }
        }
    } while (s > 0);

    // v(a^(2j)) = v(a^j)^2 
    for (j = 0; j < t; j++)
        syn[2*j+1] = gf_sqr(syn[j]);
}

static void gf_poly_copy(struct gf_poly *dst, struct gf_poly *src, int t)
{
    dst->deg=src->deg;
    memcpy(dst->c, src->c, (t+1)*sizeof(unsigned int));
}

static int compute_error_locator_polynomial(const unsigned int *syn, struct gf_poly *elp)
{
    const unsigned int t = bch.t;
    const unsigned int n = bch.n;
    unsigned int i, j, tmp, l, pd = 1, d = syn[0];
    
    struct gf_poly pelp ;
    struct gf_poly elp_copy ;
    int k, pp = -1;
    uint16_t mask_d,mask_pelp;
    unsigned int mask_tmp,mask_deg,tmp_c,mask_tmp2;
    
    memset(pelp.c, 0, (2*t+1)*sizeof(unsigned int));
    memset(elp->c, 0,(2*t+1)*sizeof(unsigned int));

    pelp.deg = 0;
    pelp.c[0] = 1;
    elp->deg = 0;
    elp->c[0] = 1;

    // use simplified binary Berlekamp-Massey algorithm 
    for (i = 0; i < t ; i++) 
    {
        mask_d=(d?0xffff:0x0);
        k = 2*i-pp;
            
        gf_poly_copy(&elp_copy, elp,i);
            // e[i+1](X) = e[i](X)+di*dp^-1*X^2(i-p)*e[p](X) 
        tmp = mod_s(a_log(d)+n-a_log(pd));

        for (j = 0; j <=i ; j++) 
        {
            mask_deg=((j <= pelp.deg)?0xffffffff:0x0);
            mask_pelp=(pelp.c[j]? 0xffff:0x0);
            l = mod_s(tmp+a_log( pelp.c[j]));
            tmp_c=a_pow_tab[l];
            elp->c[j+k] ^= (tmp_c&mask_d&mask_pelp&mask_deg);
        }
        // compute l[i+1] = max(l[i]->c[l[p]+2*(i-p]) 
        tmp = pelp.deg+(k&mask_d);
        mask_tmp =((tmp > elp->deg)?0xffffffff:0x0);
        mask_tmp2=((tmp > elp->deg)?0x0:0xffffffff);
        //memcpy cause 70 cycles gap
        if (tmp > elp->deg) 
        {
            gf_poly_copy(&pelp, &elp_copy,i);
            elp->deg = tmp;
        
        }
        else //just for constant time
        {
            gf_poly_copy(&elp_copy, elp,i);
            tmp =  elp->deg;
        }
        //update according to mask_tmp  
        pd = (d & mask_tmp)^(pd &mask_tmp2);
        pp = ((2*i) & mask_tmp)^(pp &mask_tmp2);
            
        // di+1 = S(2i+3)+elp[i+1].1*S(2i+2)+...+elp[i+1].lS(2i+3-l) 
        unsigned int mask_j,tmp_d;
        if (i < t-1) 
        {
            d = syn[2*i+2];
            
            for (j = 1; j <=i+1 ; j++)
            {
                mask_j=(j <= elp->deg)?0xffffffff:0x00;
                tmp_d=gf_mul( elp->c[j], syn[2*i+2-j]);
                d ^= (tmp_d&mask_j);
            }
        }
    }

    return (elp->deg > t) ? -1 : (int)elp->deg;
}

// build log-based representation of a polynomial
static void init_rep(const struct gf_poly *a, uint16_t *rep, uint16_t *syn_mask, unsigned int pow_start)
{
    int i,w,t=bch.t;
    w=pow_start;
    for (i = 1; i <= t; i++)
    {
        rep[i] =  mod_s(a_log(a->c[i])+ w);
        w=mod_s(w+pow_start);
        syn_mask[i]=(a->c[i] ? 0xFFFF : 0x0);
    }
}

//exhaustive root search (Chien) implementation
static int chien_search(unsigned int len, struct gf_poly *p, unsigned int *roots)
{
    unsigned int i, j, syn, count = 0,n=bch.n,t=bch.t;
    unsigned int k = 8*len+bch.ecc_bits;
    unsigned int bound=n-bch.ecc_bits;
    uint16_t     syn_mask[BCH_T+1],syn_rep[BCH_T+1];
    unsigned int syn_tmp;
    unsigned int syn0=p->c[0];
    
    //use a log-based representation of polynomial 
    init_rep(p,syn_rep,syn_mask,n-k);
    
    for (i = n-k+1; i <= bound; i++) 
    {
        // compute elp(a^i) 
        syn = syn0;
        for (j = 1 ; j <= t; j++) 
        {
            syn_rep[j]=mod_s(syn_rep[j]+j);
            syn_tmp=a_pow_tab[syn_rep[j]];
            syn ^= (syn_tmp&syn_mask[j]);
        }
        roots[count] = n-i;
        count+=(syn==0);
    }
    
    return count;
}

/**
 * decode_bch - decode received codeword and find bit error locations
 * @bch:      BCH control structure
 * @data:     received data, ignored if @calc_ecc is provided
 * @len:      data length in bytes, must always be provided
 * @recv_ecc: received ecc, if NULL then assume it was XORed in @calc_ecc
 *
 * Returns:
 * The number of errors found, or -22 if decoding failed, or -1 if
 * invalid parameters were provided
 */
int decode_bch_high(uint8_t *data, unsigned int len, const uint8_t *recv_ecc)
{
    unsigned int nbits;
    int i, err;
    uint8_t ecc_buf[BCH_ECC_BYTES];
    struct gf_poly elp;
    unsigned int syn[2*BCH_T+1];
    unsigned int errloc[BCH_T];

    // sanity check: make sure data length can be handled 
    if (8*len > (bch.n-bch.ecc_bits))
        return -1;
        
    //check data and ecc pointer
    if (!data || !recv_ecc)
        return -1;
        
    // compute received data ecc into an internal buffer 
    encode_bch_high(data, len, ecc_buf);
    // XOR received and calculated ecc 
    for (i = 0; i < bch.ecc_bytes; i++) 
    {
        ecc_buf[i] ^= recv_ecc[i];
    }
    
    //compute syndromes
    compute_syndromes(ecc_buf, syn);
    //compute error locator polynomial
    compute_error_locator_polynomial(syn,&elp);
    //find roots
    err=chien_search(len, &elp, errloc);

    //post process error location
    nbits = (len*8)+bch.ecc_bits;
    
    unsigned char mask_err;
    
    for (i = 0; i < bch.t; i++) 
    {
        mask_err  = (unsigned char)(((int)(i - err) >> 31));
        errloc[i] = (nbits-1-errloc[i]);
        errloc[i] &= (unsigned int)mask_err * 0x01010101u;
        data[errloc[i]/8] ^= ((1u << (7 - (errloc[i] % 8)))&mask_err);
    }
    
    return err;
}

#if WEAVER_MODE == 3
void encode_bch_high_nibbles(const unsigned char *data, unsigned int nibbles, uint8_t *ecc)
{
    int i;
    const uint32_t *p;
    const int l = BCH_ECC_WORDS-1;
    uint32_t ecc_buf[BCH_ECC_WORDS];
    
    memset(ecc_buf,0,BCH_ECC_WORDS*sizeof(uint32_t));

    // 每次处理 2 个半字节 (恰好等于 1 个完整字节)
    while (nibbles >= 2) 
    {
        // 处理该字节的高 4 位
        p = mod8_tab_half + (l+1)*(((ecc_buf[0] >> 28)^((*data)>>4)) & 0x0f);
        for (i = 0; i < l; i++)
            ecc_buf[i] = ((ecc_buf[i] << 4)|(ecc_buf[i+1] >> 28))^(*p++);
        ecc_buf[l] = (ecc_buf[l] << 4)^(*p);
        
        // 处理该字节的低 4 位
        p = mod8_tab_half + (l+1)*(((ecc_buf[0] >> 28)^(*data++)) & 0x0f);
        for (i = 0; i < l; i++)
            ecc_buf[i] = ((ecc_buf[i] << 4)|(ecc_buf[i+1] >> 28))^(*p++);
        ecc_buf[l] = (ecc_buf[l] << 4)^(*p);
        
        nibbles -= 2;
    }
    
    // 如果半字节总数是奇数，最后会剩余 1 个半字节，精准只处理高 4 位
    if (nibbles == 1) 
    {
        p = mod8_tab_half + (l+1)*(((ecc_buf[0] >> 28)^((*data)>>4)) & 0x0f);
        for (i = 0; i < l; i++)
            ecc_buf[i] = ((ecc_buf[i] << 4)|(ecc_buf[i+1] >> 28))^(*p++);
        ecc_buf[l] = (ecc_buf[l] << 4)^(*p);
    }
    
    store_ecc8(ecc,ecc_buf);    
}

static int chien_search_nibbles(unsigned int nibbles, struct gf_poly *p, unsigned int *roots)
{
    unsigned int i, j, syn, count = 0, n=bch.n, t=bch.t;
    
    // 核心修正：信息总位数 = 4 * nibbles + 校验位数
    unsigned int k = 4*nibbles + bch.ecc_bits; 
    unsigned int bound = n - bch.ecc_bits;
    uint16_t     syn_mask[BCH_T+1], syn_rep[BCH_T+1];
    unsigned int syn_tmp;
    unsigned int syn0=p->c[0];
    
    init_rep(p,syn_rep,syn_mask,n-k);
    
    for (i = n-k+1; i <= bound; i++) 
    {
        syn = syn0;
        for (j = 1 ; j <= t; j++) 
        {
            syn_rep[j] = mod_s(syn_rep[j]+j);
            syn_tmp = a_pow_tab[syn_rep[j]];
            syn ^= (syn_tmp&syn_mask[j]);
        }
        roots[count] = n-i;
        count += (syn==0);
    }
    
    return count;
}

// 【关键改造 3】 decode 升级
// 参数由 len (字节) 改为 nibbles (半字节)
int decode_bch_high_nibbles(uint8_t *data, unsigned int nibbles, const uint8_t *recv_ecc)
{
    unsigned int nbits;
    int i, err;
    uint8_t ecc_buf[BCH_ECC_BYTES];
    struct gf_poly elp;
    unsigned int syn[2*BCH_T+1];
    unsigned int errloc[BCH_T];

    // 核心修正：容量检查公式变为 4 * nibbles
    if (4*nibbles > (bch.n-bch.ecc_bits))
        return -1;
        
    if (!data || !recv_ecc)
        return -1;
        
    // 1. 调用新的半字节编码器
    encode_bch_high_nibbles(data, nibbles, ecc_buf);
    
    for (i = 0; i < bch.ecc_bytes; i++) 
    {
        ecc_buf[i] ^= recv_ecc[i];
    }
    
    compute_syndromes(ecc_buf, syn);
    compute_error_locator_polynomial(syn,&elp);
    
    // 2. 调用新的半字节钱氏搜索
    err = chien_search_nibbles(nibbles, &elp, errloc);

    // 3. 核心修正：总位数变为 4 * nibbles
    nbits = (nibbles*4) + bch.ecc_bits;
    
    unsigned char mask_err;
    
    // 定位与翻转错误的逻辑天然是 bit 级的，完美向下兼容
    for (i = 0; i < bch.t; i++) 
    {
        mask_err  = (unsigned char)(((int)(i - err) >> 31));
        errloc[i] = (nbits-1-errloc[i]);
        errloc[i] = ((errloc[i] & ~7)|(7-(errloc[i] & 7)));
        errloc[i] &= (unsigned int)mask_err * 0x01010101u;
        data[errloc[i]/8] ^= ((1 << (errloc[i] % 8))&mask_err);
    }
    
    return err;
}
#endif