/*
 * C-Hash-1024 x86 AVX2 Optimized Implementation 
 *
 * Architecture: C-Engine-FF (P_FF(x)=P(x) XOR x) + unified VIL/FIL mode.
 * The VIL/FIL structure is identical to C-Hash-512;
 * only the C-Engine variant, lane ID, IV, and output length differ.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>
#include "c-hash.h"

/* ═══════════════════════════════════════════════════════════════════ */
/* C-Engine AVX2 Core                                                */
/* ═══════════════════════════════════════════════════════════════════ */
static const uint8_t SB[16]={0x1,0x4,0x0,0xC,0x3,0x2,0x5,0xB,0xA,0x8,0x6,0xF,0x7,0x9,0xD,0xE};
static const uint32_t RCS[28]={
    0x243F6A88U,0x85A308D3U,0x13198A2EU,0x03707344U,0xA4093822U,0x299F31D0U,
    0x082EFA98U,0xEC4E6C89U,0x452821E6U,0x38D01377U,0xBE5466CFU,0x34E90C6CU,
    0xC0AC29B7U,0xC97C50DDU,0x3F84D5B5U,0xB5470917U,0x9216D5D9U,0x8979FB1BU,
    0xD1310BA6U,0x98DFB5ACU,0x2FFD72DBU,0xD01ADFB7U,0xB8E1AFEDU,0x6A267E96U,
    0xBA7C9045U,0xF12C7F99U,0x24A19947U,0xB3916CF7U
};
static const uint8_t SC[6]={0xC,0x9,0x0,0x8,0xB,0x2};

static __m256i g_SBOX_AVX,g_CON0F,g_FUSED_SBOX[6],g_RC256[28];
static __m128i g_NIB_EVEN,g_NIB_ODD;
static int g_init=0;

void init_cengine(void)
{
    if(g_init)return;
    g_CON0F=_mm256_set1_epi8(0x0F);
    g_SBOX_AVX=_mm256_setr_epi8(
        (char)SB[0],(char)SB[1],(char)SB[2],(char)SB[3],(char)SB[4],(char)SB[5],(char)SB[6],(char)SB[7],
        (char)SB[8],(char)SB[9],(char)SB[10],(char)SB[11],(char)SB[12],(char)SB[13],(char)SB[14],(char)SB[15],
        (char)SB[0],(char)SB[1],(char)SB[2],(char)SB[3],(char)SB[4],(char)SB[5],(char)SB[6],(char)SB[7],
        (char)SB[8],(char)SB[9],(char)SB[10],(char)SB[11],(char)SB[12],(char)SB[13],(char)SB[14],(char)SB[15]);
    g_NIB_EVEN=_mm_setr_epi8(0,2,4,6,8,10,12,14,
        (char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80);
    g_NIB_ODD=_mm_setr_epi8(1,3,5,7,9,11,13,15,
        (char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80,(char)0x80);
    for(int r=0;r<6;r++){
        uint8_t tbl[32];
        for(int x=0;x<16;x++){tbl[x]=SB[x^SC[r]];tbl[16+x]=SB[x^SC[r]];}
        g_FUSED_SBOX[r]=_mm256_loadu_si256((const __m256i*)tbl);
    }
    for(int r=0;r<28;r++){
        g_RC256[r]=_mm256_setr_epi8(
            (char)((RCS[r]>>28)&0xF),(char)((RCS[r]>>24)&0xF),(char)((RCS[r]>>20)&0xF),(char)((RCS[r]>>16)&0xF),
            (char)((RCS[r]>>12)&0xF),(char)((RCS[r]>>8)&0xF),(char)((RCS[r]>>4)&0xF),(char)(RCS[r]&0xF),
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    }
    g_init=1;
}

static inline void expand192(const uint8_t *src,__m256i st[12]){
    for(int i=0;i<6;i++){
        __m256i x=_mm256_loadu_si256((const __m256i*)(src+i*32));
        __m256i lo=_mm256_and_si256(x,g_CON0F);
        __m256i hi=_mm256_and_si256(_mm256_srli_epi16(x,4),g_CON0F);
        __m256i t0=_mm256_unpacklo_epi8(hi,lo);
        __m256i t1=_mm256_unpackhi_epi8(hi,lo);
        st[i*2]=_mm256_permute2x128_si256(t0,t1,0x20);
        st[i*2+1]=_mm256_permute2x128_si256(t0,t1,0x31);
    }
}
static inline void pack192(const __m256i st[12],uint8_t *dst){
    for(int i=0;i<6;i++){
        __m128i a=_mm256_castsi256_si128(st[i*2]),b=_mm256_extracti128_si256(st[i*2],1);
        __m128i ae=_mm_shuffle_epi8(a,g_NIB_EVEN),ao=_mm_shuffle_epi8(a,g_NIB_ODD);
        __m128i be=_mm_shuffle_epi8(b,g_NIB_EVEN),bo=_mm_shuffle_epi8(b,g_NIB_ODD);
        ae=_mm_slli_epi16(ae,4);__m128i ap=_mm_or_si128(ae,ao);
        be=_mm_slli_epi16(be,4);__m128i bp=_mm_or_si128(be,bo);
        _mm_storel_epi64((__m128i*)(dst+i*32+0),ap);
        _mm_storel_epi64((__m128i*)(dst+i*32+8),bp);
        a=_mm256_castsi256_si128(st[i*2+1]);b=_mm256_extracti128_si256(st[i*2+1],1);
        ae=_mm_shuffle_epi8(a,g_NIB_EVEN);ao=_mm_shuffle_epi8(a,g_NIB_ODD);
        be=_mm_shuffle_epi8(b,g_NIB_EVEN);bo=_mm_shuffle_epi8(b,g_NIB_ODD);
        ae=_mm_slli_epi16(ae,4);ap=_mm_or_si128(ae,ao);
        be=_mm_slli_epi16(be,4);bp=_mm_or_si128(be,bo);
        _mm_storel_epi64((__m128i*)(dst+i*32+16),ap);
        _mm_storel_epi64((__m128i*)(dst+i*32+24),bp);
    }
}
static inline void sl_avx2(__m256i st[12]){
    for(int i=0;i<12;i++)st[i]=_mm256_shuffle_epi8(g_SBOX_AVX,st[i]);
    {__m256i r0=st[0],r1=st[2],r2=st[4],r3=st[6],r4=st[8],r5=st[10];
     __m256i r01=_mm256_xor_si256(r0,r1),r012=_mm256_xor_si256(r01,r2);
     __m256i r23=_mm256_xor_si256(r2,r3),r12=_mm256_xor_si256(r1,r2);
     st[0]=_mm256_shuffle_epi8(g_FUSED_SBOX[0],_mm256_xor_si256(r0,r23));
     st[2]=_mm256_shuffle_epi8(g_FUSED_SBOX[1],_mm256_xor_si256(r01,r4));
     st[4]=_mm256_shuffle_epi8(g_FUSED_SBOX[2],_mm256_xor_si256(r12,r5));
     st[6]=_mm256_shuffle_epi8(g_FUSED_SBOX[3],_mm256_xor_si256(_mm256_xor_si256(r012,r3),r4));
     st[8]=_mm256_shuffle_epi8(g_FUSED_SBOX[4],_mm256_xor_si256(_mm256_xor_si256(r012,r4),r5));
     st[10]=_mm256_shuffle_epi8(g_FUSED_SBOX[5],_mm256_xor_si256(_mm256_xor_si256(r012,r3),r5));}
    {__m256i r0=st[1],r1=st[3],r2=st[5],r3=st[7],r4=st[9],r5=st[11];
     __m256i r01=_mm256_xor_si256(r0,r1),r012=_mm256_xor_si256(r01,r2);
     __m256i r23=_mm256_xor_si256(r2,r3),r12=_mm256_xor_si256(r1,r2);
     st[1]=_mm256_shuffle_epi8(g_FUSED_SBOX[0],_mm256_xor_si256(r0,r23));
     st[3]=_mm256_shuffle_epi8(g_FUSED_SBOX[1],_mm256_xor_si256(r01,r4));
     st[5]=_mm256_shuffle_epi8(g_FUSED_SBOX[2],_mm256_xor_si256(r12,r5));
     st[7]=_mm256_shuffle_epi8(g_FUSED_SBOX[3],_mm256_xor_si256(_mm256_xor_si256(r012,r3),r4));
     st[9]=_mm256_shuffle_epi8(g_FUSED_SBOX[4],_mm256_xor_si256(_mm256_xor_si256(r012,r4),r5));
     st[11]=_mm256_shuffle_epi8(g_FUSED_SBOX[5],_mm256_xor_si256(_mm256_xor_si256(r012,r3),r5));}
}
static inline void ml_avx2(__m256i st[12]){
    for(int row=0;row<6;row++){
        __m256i L=st[row*2],R=st[row*2+1];
        {__m256i T=_mm256_xor_si256(L,R),sw=_mm256_permute2x128_si256(T,T,0x01);
         T=_mm256_alignr_epi8(sw,T,15);L=_mm256_xor_si256(L,T);
         __m256i Tr=_mm256_xor_si256(R,T);sw=_mm256_permute2x128_si256(Tr,Tr,0x01);
         R=_mm256_alignr_epi8(Tr,sw,15);}
        {__m256i T=_mm256_xor_si256(L,R),sw=_mm256_permute2x128_si256(T,T,0x01);
         T=_mm256_alignr_epi8(sw,T,7);L=_mm256_xor_si256(L,T);
         __m256i Tr=_mm256_xor_si256(R,T);sw=_mm256_permute2x128_si256(Tr,Tr,0x01);
         R=_mm256_alignr_epi8(Tr,sw,15);}
        {__m256i T=_mm256_xor_si256(L,R),sw=_mm256_permute2x128_si256(T,T,0x01);
         T=_mm256_alignr_epi8(T,sw,7);L=_mm256_xor_si256(L,T);
         __m256i Tr=_mm256_xor_si256(R,T);sw=_mm256_permute2x128_si256(Tr,Tr,0x01);
         R=_mm256_alignr_epi8(Tr,sw,15);}
        {__m256i T=_mm256_xor_si256(L,R);
         T=_mm256_permute2x128_si256(T,T,0x01);L=_mm256_xor_si256(L,T);
         __m256i Tr=_mm256_xor_si256(R,T);__m256i sw=_mm256_permute2x128_si256(Tr,Tr,0x01);
         R=_mm256_alignr_epi8(Tr,sw,15);}
        st[row*2]=L;st[row*2+1]=R;
    }
}
static inline void kl_avx2(__m256i st[12],int r){st[0]=_mm256_xor_si256(st[0],g_RC256[r]);}

/* C-Engine pure permutation P(x) - static inline for zero call overhead */
static inline void c_engine_permute(const uint8_t *in,uint8_t *out)
{
    __m256i st[12];expand192(in,st);
    for(int r=0;r<28;r++){sl_avx2(st);ml_avx2(st);kl_avx2(st,r);}
    pack192(st,out);
}

/* C-Engine with FeedForward: P_FF(x) = P(x) XOR x (built-in FF) - static inline */
static inline void c_engine_permute_ff(const uint8_t *in,uint8_t *out)
{
    uint8_t permuted[192]__attribute__((aligned(32)));
    __m256i st[12];expand192(in,st);
    for(int r=0;r<28;r++){sl_avx2(st);ml_avx2(st);kl_avx2(st,r);}
    pack192(st,permuted);
    for(int i=0;i<5;i++){
        __m256i p=_mm256_loadu_si256((const __m256i*)(permuted+i*32));
        __m256i v=_mm256_loadu_si256((const __m256i*)(in+i*32));
        _mm256_storeu_si256((__m256i*)(out+i*32),_mm256_xor_si256(p,v));
    }
    __m128i p5=_mm_loadu_si128((const __m128i*)(permuted+160));
    __m128i v5=_mm_loadu_si128((const __m128i*)(in+160));
    _mm_storeu_si128((__m128i*)(out+160),_mm_xor_si128(p5,v5));
    *(uint64_t*)(out+176)=*(uint64_t*)(permuted+176)^*(uint64_t*)(in+176);
}

/* ═══════════════════════════════════════════════════════════════════ */
/* C-Hash-1024: Unified VIL/FIL mode with C-Engine-FF (P_FF)         */
/* ═══════════════════════════════════════════════════════════════════ */
#define NB      184
#define BB      92
#define N_BITS  1472

static inline void lmb(uint32_t b,const uint8_t *x,uint8_t *o,uint32_t ob){
    uint32_t n=(b+7)>>3;if(n>ob)n=ob;memcpy(o,x,n);
    if((b&7)&&n)o[n-1]&=(uint8_t)(0xFF<<(8-(b&7)));
    if(n<ob)memset(o+n,0,ob-n);
}
static inline void set_counter(uint8_t buf[192],uint64_t ctr){
    buf[184]=(uint8_t)(ctr>>56);buf[185]=(uint8_t)(ctr>>48);
    buf[186]=(uint8_t)(ctr>>40);buf[187]=(uint8_t)(ctr>>32);
    buf[188]=(uint8_t)(ctr>>24);buf[189]=(uint8_t)(ctr>>16);
    buf[190]=(uint8_t)(ctr>>8); buf[191]=(uint8_t)ctr;
}
static uint64_t build_counter(uint8_t lid,uint64_t p){return ((uint64_t)lid<<56)|(p&0x00FFFFFFFFFFFFFFULL);}

#define HASHLEN     1024
#define LANE_VIL    0x81
#define LANE_FIL    0x82
#define LANE_LDRH   0x83

static inline void build_iv(uint8_t iv[BB]){
    memset(iv,0,BB);
    iv[91]=(uint8_t)((HASHLEN>>0)&0xFF);iv[90]=(uint8_t)((HASHLEN>>8)&0xFF);
    iv[89]=(uint8_t)((HASHLEN>>16)&0xFF);iv[88]=(uint8_t)((HASHLEN>>24)&0xFF);
}
static inline size_t pad_msg(const uint8_t *msg,size_t msg_len,uint8_t **out_padded){
    size_t msg_bits=msg_len<<3;
    size_t rem=(msg_bits+1)%736;
    size_t pad_bits=rem?(736-rem):0;
    size_t total_bits=msg_bits+1+pad_bits;
    size_t num_blocks=total_bits/736;
    size_t total_bytes=(total_bits+7)>>3;
    *out_padded=(uint8_t*)malloc(total_bytes);
    if(!*out_padded)return 0;
    memcpy(*out_padded,msg,msg_len);
    (*out_padded)[msg_len]=(uint8_t)(1U<<(7-(msg_bits&7)));
    if(msg_len+1<total_bytes)memset(*out_padded+msg_len+1,0,total_bytes-msg_len-1);
    return num_blocks;
}
static inline size_t pad_bits(const uint8_t *msg,unsigned long long mlen_bits,uint8_t **out_padded){
    size_t rem=(size_t)((mlen_bits+1)%736);
    size_t pb=rem?(736-rem):0;
    size_t tb=(size_t)(mlen_bits+1+pb);
    size_t nb=tb/736;
    size_t tbc=(tb+7)>>3;
    *out_padded=(uint8_t*)malloc(tbc);
    if(!*out_padded){*out_padded=NULL;return 0;}
    size_t fb=(size_t)(mlen_bits>>3);
    size_t rb=(size_t)(mlen_bits&7);
    memcpy(*out_padded,msg,fb);
    if(rb){(*out_padded)[fb]=msg[fb];(*out_padded)[fb]&=(uint8_t)(0xFF<<(8-(int)rb));}
    else(*out_padded)[fb]=0;
    (*out_padded)[fb]|=(uint8_t)(1U<<(7-(int)rb));
    if(fb+1<tbc)memset(*out_padded+fb+1,0,tbc-fb-1);
    return nb;
}

static inline void ldrh_1024(const uint8_t *msg,size_t msg_len,uint8_t digest[128]){
    uint8_t x[NB];memset(x,0,NB);memcpy(x,msg,msg_len);
    x[msg_len]|=(uint8_t)(1U<<(7-((msg_len<<3)&7)));
    uint8_t in[192],y1[192],y2[192];
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,1));
    c_engine_permute_ff(in,y1);
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,2));
    c_engine_permute_ff(in,y2);
    for(int i=0;i<NB;i++)y1[i]^=y2[i];
    lmb(1024,y1,digest,128);
}
static inline int vil_1024(const uint8_t *msg,size_t msg_len,uint8_t digest[128]){
    uint8_t iv[BB];build_iv(iv);
    uint8_t h1[BB],h2[BB];
    {uint8_t in[192],o[192];memset(in,0,192);memcpy(in,iv,BB);
     set_counter(in,build_counter(LANE_VIL,0));
     c_engine_permute_ff(in,o);memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
     for(int i=0;i<BB;i++)h2[i]^=iv[i];}
    uint8_t *pd=NULL;
    size_t nb=pad_msg(msg,msg_len,&pd);
    if(!pd)return-1;
    for(size_t bi=0;bi<nb;bi++){
        const uint8_t *mi=pd+bi*BB;
        uint8_t in[192],o[192];
        for(int i=0;i<BB;i++)in[i]=h1[i]^mi[i];
        memcpy(in+BB,h2,BB);
        set_counter(in,build_counter(LANE_VIL,(uint64_t)(bi+1)));
        c_engine_permute_ff(in,o);
        memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
        for(int i=0;i<BB;i++)h2[i]^=mi[i];
    }
    free(pd);
    {uint8_t in[192],o[192];memcpy(in,h1,BB);memcpy(in+BB,h2,BB);
     set_counter(in,build_counter(LANE_FIL,1));
     c_engine_permute_ff(in,o);lmb(1024,o,digest,128);}
    return 0;
}
int c_hash_1024(const uint8_t *msg,size_t len,uint8_t digest[128]){
    init_cengine();
    if(len<(size_t)NB){ldrh_1024(msg,len,digest);return 0;}
    return vil_1024(msg,len,digest);
}

static inline void ldrh_bits_1024(const uint8_t *msg,unsigned long long mlen_bits,uint8_t digest[128]){
    uint8_t x[NB];memset(x,0,NB);
    size_t fb=(size_t)(mlen_bits>>3);
    size_t rb=(size_t)(mlen_bits&7);
    memcpy(x,msg,fb);
    if(rb)x[fb]=(msg[fb]&(uint8_t)(0xFF<<(8-(int)rb)));
    x[fb]|=(uint8_t)(1U<<(7-(int)rb));
    uint8_t in[192],y1[192],y2[192];
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,1));
    c_engine_permute_ff(in,y1);
    memset(in,0,192);memcpy(in,x,NB);set_counter(in,build_counter(LANE_LDRH,2));
    c_engine_permute_ff(in,y2);
    for(int i=0;i<NB;i++)y1[i]^=y2[i];
    lmb(1024,y1,digest,128);
}
static inline int vil_fil_bits_1024(const uint8_t *msg,unsigned long long mlen_bits,uint8_t digest[128]){
    uint8_t iv[BB];build_iv(iv);
    uint8_t h1[BB],h2[BB];
    {uint8_t in[192],o[192];memset(in,0,192);memcpy(in,iv,BB);
     set_counter(in,build_counter(LANE_VIL,0));
     c_engine_permute_ff(in,o);memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
     for(int i=0;i<BB;i++)h2[i]^=iv[i];}
    uint8_t *pd=NULL;
    /* Fast path: byte-aligned messages use pad_msg (avoids bit-level overhead) */
    size_t nb;
    if((mlen_bits&7ULL)==0){
        nb=pad_msg(msg,(size_t)(mlen_bits>>3),&pd);
    }else{
        nb=pad_bits(msg,mlen_bits,&pd);
    }
    if(!pd)return-1;
    for(size_t bi=0;bi<nb;bi++){
        const uint8_t *mi=pd+bi*BB;
        uint8_t in[192],o[192];
        for(int i=0;i<BB;i++)in[i]=h1[i]^mi[i];
        memcpy(in+BB,h2,BB);
        set_counter(in,build_counter(LANE_VIL,(uint64_t)(bi+1)));
        c_engine_permute_ff(in,o);
        memcpy(h1,o,BB);memcpy(h2,o+BB,BB);
        for(int i=0;i<BB;i++)h2[i]^=mi[i];
    }
    free(pd);
    {uint8_t in[192],o[192];memcpy(in,h1,BB);memcpy(in+BB,h2,BB);
     set_counter(in,build_counter(LANE_FIL,1));
     c_engine_permute_ff(in,o);lmb(1024,o,digest,128);}
    return 0;
}
int c_hash_1024_bits(const uint8_t *msg,unsigned long long mlen_bits,uint8_t digest[128]){
    init_cengine();
    if(mlen_bits<(unsigned long long)N_BITS){ldrh_bits_1024(msg,mlen_bits,digest);return 0;}
    return vil_fil_bits_1024(msg,mlen_bits,digest);
}
