/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
File Description: MLWE PolarLAC scalar reference fallback and selected ARM arithmetic backend.
*/
#include <stdint.h>
#include "params.h"
#include "ntt.h"
#define POLARLAC_BASE_DEG 4
#include "arm_mlwe_arith.h"



#if RL_KEM_N != 512
#error "The conjugate NTT tables in this file are for RL_KEM_N == 512."
#endif

static const int16_t con_ntt_f_769[RL_KEM_N] = {
171,605,688,361,186,766,519,649,461,129,753,546,407,626,131,432,
693,671,36,694,430,514,282,566,735,199,178,270,759,149,369,577,
147,655,497,54,767,645,689,423,86,718,364,267,161,754,288,169,
191,307,719,745,599,226,121,581,389,279,180,394,612,263,641,523,
746,112,618,635,717,621,227,232,698,212,236,21,341,379,567,549,
352,292,238,145,194,493,70,495,117,333,66,247,532,686,517,525,
331,528,167,357,414,291,411,105,654,560,14,99,509,29,366,391,
451,278,353,354,585,127,330,466,222,691,421,725,201,158,350,168
};

static const int16_t con_ntt_fn_769[RL_KEM_N] = {
601,419,611,568,44,348,78,547,303,439,642,184,415,416,491,318,
378,403,740,260,670,755,209,115,664,358,478,355,412,602,241,438,
244,252,83,237,522,703,436,652,274,699,276,575,624,531,477,417,
220,202,390,428,748,533,557,71,537,542,148,52,134,151,657,23,
246,128,506,157,375,589,490,380,188,648,543,170,24,50,462,578,
600,481,15,608,502,405,51,683,346,80,124,2,715,272,114,622,
192,400,620,10,499,591,570,34,203,487,255,339,75,733,98,76,
337,638,143,362,223,16,640,308,120,250,3,583,408,81,164,655
};

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
static void base_mul(int16_t *r,const int16_t *a,const int16_t *b,int16_t z){int16_t c0=fqmul(a[0],b[0]),c1=fqmul(a[1],b[1]),c2=fqmul(a[2],b[2]),c3=fqmul(a[3],b[3]);int16_t x;
x=fqmul(polarlac_add_s16(a[1],a[3]),polarlac_add_s16(b[1],b[3]));x=polarlac_add_s16(polarlac_sub_s16(polarlac_sub_s16(x,c1),c3),c2);r[0]=polarlac_add_s16(fqmul(x,z),c0);
x=fqmul(polarlac_add_s16(a[2],a[3]),polarlac_add_s16(b[2],b[3]));x=fqmul(polarlac_sub_s16(polarlac_sub_s16(x,c2),c3),z);x=polarlac_add_s16(x,fqmul(polarlac_add_s16(a[0],a[1]),polarlac_add_s16(b[0],b[1])));r[1]=polarlac_sub_s16(polarlac_sub_s16(x,c0),c1);
x=polarlac_add_s16(fqmul(c3,z),c1);x=polarlac_add_s16(x,fqmul(polarlac_add_s16(a[0],a[2]),polarlac_add_s16(b[0],b[2])));r[2]=polarlac_sub_s16(polarlac_sub_s16(x,c0),c2);
x=fqmul(polarlac_add_s16(a[0],a[3]),polarlac_add_s16(b[0],b[3]));x=polarlac_sub_s16(polarlac_sub_s16(x,c0),c3);x=polarlac_add_s16(x,fqmul(polarlac_add_s16(a[1],a[2]),polarlac_add_s16(b[1],b[2])));r[3]=polarlac_sub_s16(polarlac_sub_s16(x,c1),c2);}
#endif
#if POLARLAC_HAVE_ARM_MLWE_NEON
static inline int16x8x4_t base4x8(int16x8x4_t a,int16x8x4_t b,int16x8_t z){int16x8x4_t r;const int16x8_t c0=polarlac_neon_fqmul_s16x8(a.val[0],b.val[0]),c1=polarlac_neon_fqmul_s16x8(a.val[1],b.val[1]),c2=polarlac_neon_fqmul_s16x8(a.val[2],b.val[2]),c3=polarlac_neon_fqmul_s16x8(a.val[3],b.val[3]);int16x8_t x;
x=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[1],a.val[3]),polarlac_neon_wrap_add_s16x8(b.val[1],b.val[3]));x=polarlac_neon_wrap_add_s16x8(polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c1),c3),c2);r.val[0]=polarlac_neon_wrap_add_s16x8(polarlac_neon_fqmul_s16x8(x,z),c0);
x=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[2],a.val[3]),polarlac_neon_wrap_add_s16x8(b.val[2],b.val[3]));x=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c2),c3),z);x=polarlac_neon_wrap_add_s16x8(x,polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[0],a.val[1]),polarlac_neon_wrap_add_s16x8(b.val[0],b.val[1])));r.val[1]=polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c0),c1);
x=polarlac_neon_wrap_add_s16x8(polarlac_neon_fqmul_s16x8(c3,z),c1);x=polarlac_neon_wrap_add_s16x8(x,polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[0],a.val[2]),polarlac_neon_wrap_add_s16x8(b.val[0],b.val[2])));r.val[2]=polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c0),c2);
x=polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[0],a.val[3]),polarlac_neon_wrap_add_s16x8(b.val[0],b.val[3]));x=polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c0),c3);x=polarlac_neon_wrap_add_s16x8(x,polarlac_neon_fqmul_s16x8(polarlac_neon_wrap_add_s16x8(a.val[1],a.val[2]),polarlac_neon_wrap_add_s16x8(b.val[1],b.val[2])));r.val[3]=polarlac_neon_wrap_sub_s16x8(polarlac_neon_wrap_sub_s16x8(x,c1),c2);return r;}
static inline int16x8_t root_lanes(unsigned block){const int16x4_t p=vld1_s16(f+64U+block/2U);const int16x4x2_t z=vzip_s16(p,polarlac_neon_wrap_neg_s16x4(p));return vcombine_s16(z.val[0],z.val[1]);}
#endif
void mq_poly_pointwise_mul(int16_t *r,int16_t *a,int16_t *b){
#if POLARLAC_HAVE_ARM_MLWE_NEON
for(unsigned block=0;block<RL_KEM_N/4U;block+=8U){const int16x8x4_t v=base4x8(vld4q_s16(a+4U*block),vld4q_s16(b+4U*block),root_lanes(block));vst4q_s16(r+4U*block,v);}
#else
for(unsigned i=0;i<RL_KEM_N/8U;i++){base_mul(r+8U*i,a+8U*i,b+8U*i,f[64U+i]);base_mul(r+8U*i+4U,a+8U*i+4U,b+8U*i+4U,(int16_t)-f[64U+i]);}
#endif
}
void mq_poly_pointwise_mulacc2(int16_t *r,const int16_t *a0,const int16_t *b0,const int16_t *a1,const int16_t *b1){
#if POLARLAC_HAVE_ARM_MLWE_NEON
for(unsigned block=0;block<RL_KEM_N/4U;block+=8U){const int16x8_t z=root_lanes(block);int16x8x4_t x=base4x8(vld4q_s16(a0+4U*block),vld4q_s16(b0+4U*block),z);const int16x8x4_t y=base4x8(vld4q_s16(a1+4U*block),vld4q_s16(b1+4U*block),z);for(unsigned j=0;j<4U;j++)x.val[j]=polarlac_neon_accumulate_products_s16x8(x.val[j],y.val[j]);vst4q_s16(r+4U*block,x);}
#else
for(unsigned i=0;i<RL_KEM_N/8U;i++){int16_t x[4],y[4];base_mul(x,a0+8U*i,b0+8U*i,f[64U+i]);base_mul(y,a1+8U*i,b1+8U*i,f[64U+i]);for(unsigned j=0;j<4;j++)r[8U*i+j]=rl_kem_mod_q((int32_t)rl_kem_mod_q(x[j])+y[j]);base_mul(x,a0+8U*i+4U,b0+8U*i+4U,(int16_t)-f[64U+i]);base_mul(y,a1+8U*i+4U,b1+8U*i+4U,(int16_t)-f[64U+i]);for(unsigned j=0;j<4;j++)r[8U*i+4U+j]=rl_kem_mod_q((int32_t)rl_kem_mod_q(x[j])+y[j]);}
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
    for (len = RL_KEM_N >> 1; len >= 4; len >>= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = con_ntt_f_769[k++];
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
    for (len = 4; len <= RL_KEM_N >> 1; len <<= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = con_ntt_fn_769[k++];
            for (j = start; j < start + len; j++) {
                t = a[j];
                a[j] = polarlac_add_s16(t, a[j + len]);
                a[j + len] = polarlac_sub_s16(t, a[j + len]);
                a[j + len] = con_fqmul_769(zeta, a[j + len]);
            }
        }
    }

    for (j = 0; j < RL_KEM_N; j++) {
        a[j] = con_fqmul_769(a[j], con_ntt_fn_769[127]);
    }
}

static void con_basemul_769(int16_t *r, const int16_t *a, const int16_t *b, int16_t zeta)
{
    int c0, c1, c2, c3;

    c0 = con_fqmul_769(a[0], b[0]);
    c1 = con_fqmul_769(a[1], b[1]);
    c2 = con_fqmul_769(a[2], b[2]);
    c3 = con_fqmul_769(a[3], b[3]);

    r[0] = con_fqmul_769((a[1] + a[3]), (b[1] + b[3]));
    r[0] -= c1;
    r[0] -= c3;
    r[0] += c2;
    r[0] = con_fqmul_769(r[0], zeta);
    r[0] += c0;

    r[1] = con_fqmul_769((a[2] + a[3]), (b[2] + b[3]));
    r[1] -= c2;
    r[1] -= c3;
    r[1] = con_fqmul_769(r[1], zeta);
    r[1] += con_fqmul_769((a[0] + a[1]), (b[0] + b[1]));
    r[1] -= c0;
    r[1] -= c1;

    r[2] = con_fqmul_769(c3, zeta);
    r[2] += c1;
    r[2] += con_fqmul_769((a[0] + a[2]), (b[0] + b[2]));
    r[2] -= c0;
    r[2] -= c2;

    r[3] = con_fqmul_769((a[0] + a[3]), (b[0] + b[3]));
    r[3] -= c0;
    r[3] -= c3;
    r[3] += con_fqmul_769((a[1] + a[2]), (b[1] + b[2]));
    r[3] -= c1;
    r[3] -= c2;
}

void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b)
{
    for (int i = 0; i < RL_KEM_N / 8; i++) {
        con_basemul_769(r + 8 * i, a + 8 * i, b + 8 * i, con_ntt_f_769[64 + i]);
        con_basemul_769(r + 8 * i + 4, a + 8 * i + 4, b + 8 * i + 4,
            (int16_t)-con_ntt_f_769[64 + i]);
    }
}

static void con_base_adjoint_769(int16_t *r, const int16_t *a, int16_t zeta)
{
    r[0] = a[0];
    r[1] = con_fqmul_769(a[3], zeta);
    r[2] = con_fqmul_769(a[2], zeta);
    r[3] = con_fqmul_769(a[1], zeta);
}

void con_poly_adjoint_ntt(int16_t *r, const int16_t *a)
{
    for (int i = 0; i < RL_KEM_N / 8; i++) {
        con_base_adjoint_769(r + RL_KEM_N - 8 * (i + 1) + 4,
            a + 8 * i, con_ntt_f_769[64 + i]);
        con_base_adjoint_769(r + RL_KEM_N - 8 * (i + 1),
            a + 8 * i + 4, (int16_t)-con_ntt_f_769[64 + i]);
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
    con_poly_mul_ntt(mul_b, con_a, a_ntt);
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
