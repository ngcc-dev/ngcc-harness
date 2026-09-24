/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
File Description: MLWE PolarLAC scalar reference fallback and selected ARM arithmetic backend.
*/
#include <stdint.h>
#include "params.h"
#include "ntt.h"
#define POLARLAC_BASE_DEG 8
#include "arm_mlwe_arith.h"



#define CONJ_NTT_QINV (-767)

/**
 * @file ntt.c
 * @brief NTT-domain routines for the selected PolarLAC-MLWE ARM arithmetic backend.
 *
 * Functions include Montgomery reduction, forward/inverse NTT transforms,
 * pointwise multiplication, and small-block base multiplication.
 * Comments follow the @brief/@param style used elsewhere in the
 * project (see KEM_AlgorithmInstance.h).
 */

/**
 * @brief Montgomery reduction: reduce a 32-bit integer modulo RL_KEM_Q using
 *        Montgomery constant QINV. Result is a 16-bit representative.
 * @param[in] a Integer to be reduced (int32_t).
 * @return Reduced value (int16_t) congruent to a * R^{-1} mod RL_KEM_Q where R=2^16.
 */

#ifndef POLARLAC_BASE_DEG
#error "POLARLAC_BASE_DEG must be defined"
#endif

static inline int16_t polarlac_s16_from_u16(uint16_t b){return (int16_t)((int32_t)(b^UINT16_C(0x8000))-INT32_C(0x8000));}
static inline int16_t polarlac_add_s16(int16_t a,int16_t b){return polarlac_s16_from_u16((uint16_t)((uint16_t)a+(uint16_t)b));}
static inline int16_t polarlac_sub_s16(int16_t a,int16_t b){return polarlac_s16_from_u16((uint16_t)((uint16_t)a-(uint16_t)b));}
static inline int16_t fqmul(int16_t a,int16_t b){return rl_kem_montgomery_reduce((int32_t)a*(int32_t)b);}

#if !POLARLAC_HAVE_ARM_MLWE_NEON
static void polarlac_scalar_ntt_stage(int16_t *a,unsigned int len,unsigned int *k)
{
    for(unsigned int start=0;start<RL_KEM_N;start+=2U*len){const int16_t z=f[(*k)++];for(unsigned int j=start;j<start+len;j++){const int16_t lo=a[j],t=fqmul(z,a[j+len]);a[j]=polarlac_add_s16(lo,t);a[j+len]=polarlac_sub_s16(lo,t);}}
}
static void polarlac_scalar_intt_stage(int16_t *a,unsigned int len,unsigned int *k)
{
    for(unsigned int start=0;start<RL_KEM_N;start+=2U*len){const int16_t z=fn[(*k)++];for(unsigned int j=start;j<start+len;j++){const int16_t lo=a[j],hi=a[j+len];a[j]=polarlac_add_s16(lo,hi);a[j+len]=fqmul(z,polarlac_sub_s16(lo,hi));}}
}
#endif
static void polarlac_ntt_stage(int16_t *a,unsigned int len,unsigned int *k)
{
#if POLARLAC_HAVE_ARM_MLWE_NEON
#if POLARLAC_BASE_DEG == 2
    if(len==2U){for(unsigned int start=0;start<RL_KEM_N;start+=16U){const unsigned int root=*k;const int16_t z[4]={f[root],f[root+1U],f[root+2U],f[root+3U]};*k=root+4U;polarlac_neon_ntt_four_groups2(a+start,z);}return;}
#endif
    if(len==4U){for(unsigned int start=0;start<RL_KEM_N;start+=16U){const int16_t z0=f[(*k)++],z1=f[(*k)++];polarlac_neon_ntt_two_groups4(a+start,z0,a+start+8U,z1);}return;}
    for(unsigned int start=0;start<RL_KEM_N;start+=2U*len){const int16_t z=f[(*k)++];polarlac_neon_ntt_group(a,start,len,z);}return;
#else
    polarlac_scalar_ntt_stage(a,len,k);
#endif
}
static void polarlac_intt_stage(int16_t *a,unsigned int len,unsigned int *k)
{
#if POLARLAC_HAVE_ARM_MLWE_NEON
#if POLARLAC_BASE_DEG == 2
    if(len==2U){for(unsigned int start=0;start<RL_KEM_N;start+=16U){const unsigned int root=*k;const int16_t z[4]={fn[root],fn[root+1U],fn[root+2U],fn[root+3U]};*k=root+4U;polarlac_neon_intt_four_groups2(a+start,z);}return;}
#endif
    if(len==4U){for(unsigned int start=0;start<RL_KEM_N;start+=16U){const int16_t z0=fn[(*k)++],z1=fn[(*k)++];polarlac_neon_intt_two_groups4(a+start,z0,a+start+8U,z1);}return;}
    for(unsigned int start=0;start<RL_KEM_N;start+=2U*len){const int16_t z=fn[(*k)++];polarlac_neon_intt_group(a,start,len,z);}return;
#else
    polarlac_scalar_intt_stage(a,len,k);
#endif
}
static void polarlac_intt_final_scale(int16_t *a)
{
    unsigned int j=0;
#if POLARLAC_HAVE_ARM_MLWE_NEON
    for(;j+8U<=RL_KEM_N;j+=8U)vst1q_s16(a+j,polarlac_neon_fqmul_s16x8(vld1q_s16(a+j),vdupq_n_s16(fn[127])));
#endif
    for(;j<RL_KEM_N;j++)a[j]=fqmul(a[j],fn[127]);
}
#ifdef POLARLAC_ARITH_TEST_HOOKS
void polarlac_test_ntt_stage(int16_t *a,unsigned int len,unsigned int *k){polarlac_ntt_stage(a,len,k);}
void polarlac_test_intt_stage(int16_t *a,unsigned int len,unsigned int *k){polarlac_intt_stage(a,len,k);}
void polarlac_test_intt_final_scale(int16_t *a){polarlac_intt_final_scale(a);}
void polarlac_test_fqmul_vector(int16_t *o,const int16_t *x,const int16_t *y,unsigned int n){unsigned int i=0;
#if POLARLAC_HAVE_ARM_MLWE_NEON
for(;i+8U<=n;i+=8U)vst1q_s16(o+i,polarlac_neon_fqmul_s16x8(vld1q_s16(x+i),vld1q_s16(y+i)));
#endif
for(;i<n;i++)o[i]=fqmul(x[i],y[i]);}
int polarlac_test_backend_id(void){
#if POLARLAC_HAVE_ARM_MLWE_NEON
return 1;
#else
return 0;
#endif
}
#endif
void mq_poly_ntt(int16_t *a){unsigned int k=1U;for(unsigned int len=RL_KEM_N>>1;len>=POLARLAC_BASE_DEG;len>>=1)polarlac_ntt_stage(a,len,&k);}
void mq_poly_intt(int16_t *a){unsigned int k=0U;for(unsigned int len=POLARLAC_BASE_DEG;len<=RL_KEM_N>>1;len<<=1)polarlac_intt_stage(a,len,&k);polarlac_intt_final_scale(a);}

#if !POLARLAC_HAVE_ARM_MLWE_NEON
static void base_mul(int16_t *r,const int16_t *a,const int16_t *b,int16_t z){int16_t c[8],s[8][8]={{0}};for(int i=0;i<8;i++)c[i]=fqmul(a[i],b[i]);for(int i=0;i<8;i++)for(int j=i+1;j<8;j++)s[i][j]=polarlac_sub_s16(polarlac_sub_s16(fqmul(polarlac_add_s16(a[i],a[j]),polarlac_add_s16(b[i],b[j])),c[i]),c[j]);int16_t k;
k=polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(s[1][7],s[2][6]),s[3][5]),c[4]);r[0]=polarlac_add_s16(c[0],fqmul(k,z));k=polarlac_add_s16(polarlac_add_s16(s[2][7],s[3][6]),s[4][5]);r[1]=polarlac_add_s16(s[0][1],fqmul(k,z));k=polarlac_add_s16(polarlac_add_s16(s[3][7],s[4][6]),c[5]);r[2]=polarlac_add_s16(polarlac_add_s16(s[0][2],c[1]),fqmul(k,z));k=polarlac_add_s16(s[4][7],s[5][6]);r[3]=polarlac_add_s16(polarlac_add_s16(s[0][3],s[1][2]),fqmul(k,z));k=polarlac_add_s16(s[5][7],c[6]);r[4]=polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(s[0][4],s[1][3]),c[2]),fqmul(k,z));r[5]=polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(s[0][5],s[1][4]),s[2][3]),fqmul(s[6][7],z));r[6]=polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(s[0][6],s[1][5]),s[2][4]),c[3]),fqmul(c[7],z));r[7]=polarlac_add_s16(polarlac_add_s16(polarlac_add_s16(s[0][7],s[1][6]),s[2][5]),s[3][4]);}
#endif
#if POLARLAC_HAVE_ARM_MLWE_NEON
static inline void load8x8(const int16_t *s,int16x8_t v[8]){const int16x8x4_t l=vld4q_s16(s),h=vld4q_s16(s+32);v[0]=vuzp1q_s16(l.val[0],h.val[0]);v[4]=vuzp2q_s16(l.val[0],h.val[0]);v[1]=vuzp1q_s16(l.val[1],h.val[1]);v[5]=vuzp2q_s16(l.val[1],h.val[1]);v[2]=vuzp1q_s16(l.val[2],h.val[2]);v[6]=vuzp2q_s16(l.val[2],h.val[2]);v[3]=vuzp1q_s16(l.val[3],h.val[3]);v[7]=vuzp2q_s16(l.val[3],h.val[3]);}
static inline void store8x8(int16_t *d,const int16x8_t v[8]){int16x8x4_t l,h;l.val[0]=vzip1q_s16(v[0],v[4]);h.val[0]=vzip2q_s16(v[0],v[4]);l.val[1]=vzip1q_s16(v[1],v[5]);h.val[1]=vzip2q_s16(v[1],v[5]);l.val[2]=vzip1q_s16(v[2],v[6]);h.val[2]=vzip2q_s16(v[2],v[6]);l.val[3]=vzip1q_s16(v[3],v[7]);h.val[3]=vzip2q_s16(v[3],v[7]);vst4q_s16(d,l);vst4q_s16(d+32,h);}
static inline int16x8_t pair8(int16x8_t ai,int16x8_t aj,int16x8_t bi,int16x8_t bj,int16x8_t ci,int16x8_t cj){return polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(ai,aj),polarlac_neon_wrap_add_s16x8(bi,bj)),ci),cj);}
static inline void base8x8_compute(const int16_t *a,const int16_t *b,int16x8_t z,int16x8_t r[8]){int16x8_t av[8],bv[8],c[8],x;load8x8(a,av);load8x8(b,bv);for(int i=0;i<8;i++)c[i]=polarlac_neon_fqmul_s16x8(av[i],bv[i]);
#define P(I,J) pair8(av[I],av[J],bv[I],bv[J],c[I],c[J])
x=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(1,7),P(2,6)),P(3,5)),c[4]);r[0]=polarlac_neon_wrap_add_s16x8(c[0],polarlac_neon_fqmul_s16x8(x,z));x=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(2,7),P(3,6)),P(4,5));r[1]=polarlac_neon_wrap_add_s16x8(P(0,1),polarlac_neon_fqmul_s16x8(x,z));x=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(3,7),P(4,6)),c[5]);r[2]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,2),c[1]),polarlac_neon_fqmul_s16x8(x,z));x=polarlac_neon_wrap_add_s16x8(P(4,7),P(5,6));r[3]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,3),P(1,2)),polarlac_neon_fqmul_s16x8(x,z));x=polarlac_neon_wrap_add_s16x8(P(5,7),c[6]);r[4]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,4),P(1,3)),c[2]),polarlac_neon_fqmul_s16x8(x,z));r[5]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,5),P(1,4)),P(2,3)),polarlac_neon_fqmul_s16x8(P(6,7),z));r[6]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,6),P(1,5)),P(2,4)),c[3]),polarlac_neon_fqmul_s16x8(c[7],z));r[7]=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_add_s16x8(P(0,7),P(1,6)),P(2,5)),P(3,4));
#undef P
}
static inline int16x8_t root_lanes(unsigned block){const int16x4_t p=vld1_s16(f+64U+block/2U);const int16x4x2_t z=vzip_s16(p,polarlac_neon_wrap_neg_s16x4(p));return vcombine_s16(z.val[0],z.val[1]);}
#endif
void mq_poly_pointwise_mul(int16_t *r,int16_t *a,int16_t *b){
#if POLARLAC_HAVE_ARM_MLWE_NEON
for(unsigned block=0;block<RL_KEM_N/8U;block+=8U){int16x8_t v[8];base8x8_compute(a+8U*block,b+8U*block,root_lanes(block),v);store8x8(r+8U*block,v);}
#else
for(unsigned i=0;i<RL_KEM_N/16U;i++){base_mul(r+16U*i,a+16U*i,b+16U*i,f[64U+i]);base_mul(r+16U*i+8U,a+16U*i+8U,b+16U*i+8U,(int16_t)-f[64U+i]);}
#endif
}
void mq_poly_pointwise_mulacc2(int16_t *r,const int16_t *a0,const int16_t *b0,const int16_t *a1,const int16_t *b1){
#if POLARLAC_HAVE_ARM_MLWE_NEON
for(unsigned block=0;block<RL_KEM_N/8U;block+=8U){int16x8_t x[8],y[8];const int16x8_t z=root_lanes(block);base8x8_compute(a0+8U*block,b0+8U*block,z,x);base8x8_compute(a1+8U*block,b1+8U*block,z,y);for(unsigned j=0;j<8U;j++)x[j]=polarlac_neon_accumulate_products_s16x8(x[j],y[j]);store8x8(r+8U*block,x);}
#else
for(unsigned i=0;i<RL_KEM_N/16U;i++){int16_t x[8],y[8];base_mul(x,a0+16U*i,b0+16U*i,f[64U+i]);base_mul(y,a1+16U*i,b1+16U*i,f[64U+i]);for(unsigned j=0;j<8;j++)r[16U*i+j]=rl_kem_mod_q((int32_t)rl_kem_mod_q(x[j])+y[j]);base_mul(x,a0+16U*i+8U,b0+16U*i+8U,(int16_t)-f[64U+i]);base_mul(y,a1+16U*i+8U,b1+16U*i+8U,(int16_t)-f[64U+i]);for(unsigned j=0;j<8;j++)r[16U*i+8U+j]=rl_kem_mod_q((int32_t)rl_kem_mod_q(x[j])+y[j]);}
#endif
}

static inline int16_t con_montgomery_reduce_769(int32_t a)
{
    const uint32_t low = (uint32_t)a * (uint32_t)(uint16_t)CONJ_NTT_QINV;
    const uint32_t u_bits = low & UINT32_C(0xffff);
    const int32_t u = (int32_t)(u_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    const int64_t t = (int64_t)a - (int64_t)u * INT64_C(769);
    const uint32_t q_bits = (uint32_t)(((uint64_t)t >> 16) & UINT64_C(0xffff));
    const int32_t quotient = (int32_t)(q_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    return (int16_t)quotient;
}

static inline int16_t con_fqmul_769(int16_t a, int16_t b)
{
    return con_montgomery_reduce_769((int32_t)a * b);
}

void con_poly_ntt(int16_t *a)
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for (len = RL_KEM_N >> 1; len >= 8; len >>= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = f[k++];
            for (j = start; j < start + len; j++) {
                t = con_fqmul_769(zeta, a[j + len]);
                a[j + len] = polarlac_sub_s16(a[j], t);
                a[j] = polarlac_add_s16(a[j], t);
            }
        }
    }
}

void con_poly_intt(int16_t *a)
{
    unsigned int start, len, j, k;
    int16_t t, zeta;

    k = 0;
    for (len = 8; len <= RL_KEM_N >> 1; len <<= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = fn[k++];
            for (j = start; j < start + len; j++) {
                t = a[j];
                a[j] = polarlac_add_s16(t, a[j + len]);
                a[j + len] = polarlac_sub_s16(t, a[j + len]);
                a[j + len] = con_fqmul_769(zeta, a[j + len]);
            }
        }
    }

    for (j = 0; j < RL_KEM_N; j++) {
        a[j] = con_fqmul_769(a[j], fn[127]);
    }
}


// static void con_basemul_769(int16_t *r, const int16_t *a, const int16_t *b, int16_t zeta)
// {
//     int16_t c0, c1, c2, c3, c4, c5, c6, c7;
//     int16_t s01, s02, s03, s04, s05, s06, s07;
//     int16_t s12, s13, s14, s15, s16, s17;
//     int16_t s23, s24, s25, s26, s27;
//     int16_t s34, s35, s36, s37;
//     int16_t s45, s46, s47;
//     int16_t s56, s57;
//     int16_t s67;
//     int16_t k;

//     c0 = con_fqmul_769(a[0], b[0]);
//     c1 = con_fqmul_769(a[1], b[1]);
//     c2 = con_fqmul_769(a[2], b[2]);
//     c3 = con_fqmul_769(a[3], b[3]);
//     c4 = con_fqmul_769(a[4], b[4]);
//     c5 = con_fqmul_769(a[5], b[5]);
//     c6 = con_fqmul_769(a[6], b[6]);
//     c7 = con_fqmul_769(a[7], b[7]);

//     s01 = con_fqmul_769(a[0] + a[1], b[0] + b[1]) - c0 - c1;
//     s02 = con_fqmul_769(a[0] + a[2], b[0] + b[2]) - c0 - c2;
//     s03 = con_fqmul_769(a[0] + a[3], b[0] + b[3]) - c0 - c3;
//     s04 = con_fqmul_769(a[0] + a[4], b[0] + b[4]) - c0 - c4;
//     s05 = con_fqmul_769(a[0] + a[5], b[0] + b[5]) - c0 - c5;
//     s06 = con_fqmul_769(a[0] + a[6], b[0] + b[6]) - c0 - c6;
//     s07 = con_fqmul_769(a[0] + a[7], b[0] + b[7]) - c0 - c7;

//     s12 = con_fqmul_769(a[1] + a[2], b[1] + b[2]) - c1 - c2;
//     s13 = con_fqmul_769(a[1] + a[3], b[1] + b[3]) - c1 - c3;
//     s14 = con_fqmul_769(a[1] + a[4], b[1] + b[4]) - c1 - c4;
//     s15 = con_fqmul_769(a[1] + a[5], b[1] + b[5]) - c1 - c5;
//     s16 = con_fqmul_769(a[1] + a[6], b[1] + b[6]) - c1 - c6;
//     s17 = con_fqmul_769(a[1] + a[7], b[1] + b[7]) - c1 - c7;

//     s23 = con_fqmul_769(a[2] + a[3], b[2] + b[3]) - c2 - c3;
//     s24 = con_fqmul_769(a[2] + a[4], b[2] + b[4]) - c2 - c4;
//     s25 = con_fqmul_769(a[2] + a[5], b[2] + b[5]) - c2 - c5;
//     s26 = con_fqmul_769(a[2] + a[6], b[2] + b[6]) - c2 - c6;
//     s27 = con_fqmul_769(a[2] + a[7], b[2] + b[7]) - c2 - c7;

//     s34 = con_fqmul_769(a[3] + a[4], b[3] + b[4]) - c3 - c4;
//     s35 = con_fqmul_769(a[3] + a[5], b[3] + b[5]) - c3 - c5;
//     s36 = con_fqmul_769(a[3] + a[6], b[3] + b[6]) - c3 - c6;
//     s37 = con_fqmul_769(a[3] + a[7], b[3] + b[7]) - c3 - c7;

//     s45 = con_fqmul_769(a[4] + a[5], b[4] + b[5]) - c4 - c5;
//     s46 = con_fqmul_769(a[4] + a[6], b[4] + b[6]) - c4 - c6;
//     s47 = con_fqmul_769(a[4] + a[7], b[4] + b[7]) - c4 - c7;

//     s56 = con_fqmul_769(a[5] + a[6], b[5] + b[6]) - c5 - c6;
//     s57 = con_fqmul_769(a[5] + a[7], b[5] + b[7]) - c5 - c7;
//     s67 = con_fqmul_769(a[6] + a[7], b[6] + b[7]) - c6 - c7;

//     k = s17;
//     k += s26;
//     k += s35;
//     k += c4;
//     r[0] = c0 + con_fqmul_769(k, zeta);

//     k = s27;
//     k += s36;
//     k += s45;
//     r[1] = s01 + con_fqmul_769(k, zeta);

//     k = s37;
//     k += s46;
//     k += c5;
//     r[2] = s02 + c1 + con_fqmul_769(k, zeta);

//     k = s47;
//     k += s56;
//     r[3] = s03 + s12 + con_fqmul_769(k, zeta);

//     k = s57;
//     k += c6;
//     r[4] = s04 + s13 + c2 + con_fqmul_769(k, zeta);

//     r[5] = s05 + s14 + s23 + con_fqmul_769(s67, zeta);
//     r[6] = s06 + s15 + s24 + c3 + con_fqmul_769(c7, zeta);
//     r[7] = s07 + s16 + s25 + s34;
// }

// void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b)
// {
//     for (int i = 0; i < RL_KEM_N / 16; i++) {
//         con_basemul_769(r + 16 * i, a + 16 * i, b + 16 * i, f[64 + i]);
//         con_basemul_769(r + 16 * i + 8, a + 16 * i + 8, b + 16 * i + 8,
//             (int16_t)-f[64 + i]);
//     }
// }

static void con_base_adjoint_769(int16_t *r, const int16_t *a, int16_t zeta)
{
    r[0] = a[0];
    r[1] = con_fqmul_769(a[7], zeta);
    r[2] = con_fqmul_769(a[6], zeta);
    r[3] = con_fqmul_769(a[5], zeta);
    r[4] = con_fqmul_769(a[4], zeta);
    r[5] = con_fqmul_769(a[3], zeta);
    r[6] = con_fqmul_769(a[2], zeta);
    r[7] = con_fqmul_769(a[1], zeta);
}

void con_poly_adjoint_ntt(int16_t *r, const int16_t *a)
{
    for (int i = 0; i < RL_KEM_N / 16; i++) {
        con_base_adjoint_769(r + RL_KEM_N - 16 * (i + 1) + 8,
            a + 16 * i, f[64 + i]);
        con_base_adjoint_769(r + RL_KEM_N - 16 * (i + 1),
            a + 16 * i + 8, (int16_t)- f[64 + i]);
    }
}

static inline int32_t con_abs_i16(int16_t x)
{
    return x < 0 ? -(int32_t)x : (int32_t)x;
}

int32_t con_poly_rejection_score(const int16_t *a)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t con_a[RL_KEM_N];
    int16_t mul_b[RL_KEM_N];
    int32_t acc;

    for (int i = 0; i < RL_KEM_N; i++) {
        a_ntt[i] = a[i];
    }

    con_poly_ntt(a_ntt);
    con_poly_adjoint_ntt(con_a, a_ntt);
    // con_poly_mul_ntt(mul_b, con_a, a_ntt);
    mq_poly_pointwise_mul(mul_b, con_a, a_ntt);
    con_poly_intt(mul_b);

    acc = con_abs_i16(mul_b[0]) + con_abs_i16(mul_b[RL_KEM_N / 2]);
    for (int i = 1; i < RL_KEM_N / 2; i++) {
        acc += 2 * con_abs_i16(mul_b[i]);
    }

    return acc;
}

int con_poly_within_bound(const int16_t *a, int32_t bound)
{
    return con_poly_rejection_score(a) <= bound;
}
