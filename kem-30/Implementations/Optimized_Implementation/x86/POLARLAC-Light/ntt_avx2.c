/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements AVX2 NTT arithmetic routines for the optimized POLARLAC-Light instance.
*/

#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "ntt_avx2.h"
#include "ntt_avx2_params.h"

int mul_mod(int16_t *a, int16_t *b, int16_t *c)
{
    int i;

    __m256i tmp_a, tmp_b, tmp_c1, tmp_c0, tmp_x;
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

    for (i = 0; i < RL_KEM_N; i += 16)
    {
        //load
        tmp_a = _mm256_load_si256((__m256i *)(a + i));
        tmp_b = _mm256_load_si256((__m256i *)(b + i));
        //c=a*b
        tmp_c0 = _mm256_mullo_epi16(tmp_a, tmp_b);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a, tmp_b);
        //montgomery mod RL_KEM_Q
        tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_c1 = _mm256_sub_epi16(tmp_c1, tmp_x);
        //if c1<0 then +RL_KEM_Q
        tmp_c1 = _mm256_add_epi16(tmp_c1, _mm256_and_si256(_mm256_srai_epi16(tmp_c1, 15), tmp_Q));
        //store
        _mm256_store_si256((__m256i *)(c + i), tmp_c1);
    }

    return 0;
}


static inline __m256i montgomery_mullo_epi16(__m256i a, __m256i b)
{
    const __m256i qinv = _mm256_set1_epi16(INVERSE_Q);
    const __m256i q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i lo = _mm256_mullo_epi16(a, b);
    __m256i hi = _mm256_mulhi_epi16(a, b);
    __m256i x = _mm256_mullo_epi16(lo, qinv);
    x = _mm256_mulhi_epi16(x, q);
    return _mm256_sub_epi16(hi, x);
}

static inline __m256i swap_adjacent_epi16(__m256i x)
{
    x = _mm256_shufflelo_epi16(x, 0xB1);
    x = _mm256_shufflehi_epi16(x, 0xB1);
    return x;
}

static inline __m256i canonicalize_nonnegative_epi16(__m256i x)
{
    const __m256i q = _mm256_set1_epi16(RL_KEM_Q);
    return _mm256_add_epi16(x, _mm256_and_si256(_mm256_srai_epi16(x, 15), q));
}

static inline __m256i reduce_upto_3q_epi16(__m256i x)
{
    const __m256i q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i y;

    y = _mm256_sub_epi16(x, q);
    x = _mm256_add_epi16(y, _mm256_and_si256(_mm256_srai_epi16(y, 15), q));
    y = _mm256_sub_epi16(x, q);
    x = _mm256_add_epi16(y, _mm256_and_si256(_mm256_srai_epi16(y, 15), q));
    return x;
}

static inline __m256i basemul_f2_product_epi16(__m256i va, __m256i vb, __m256i z16)
{
    __m256i prod_same = montgomery_mullo_epi16(va, vb);
    __m256i prod_same_swapped = swap_adjacent_epi16(prod_same);
    __m256i z_times_odd = montgomery_mullo_epi16(prod_same_swapped, z16);
    __m256i r0_candidate = _mm256_add_epi16(prod_same, z_times_odd);
    r0_candidate = canonicalize_nonnegative_epi16(r0_candidate);

    __m256i vb_swapped = swap_adjacent_epi16(vb);
    __m256i prod_cross = montgomery_mullo_epi16(va, vb_swapped);
    __m256i r1_candidate = _mm256_add_epi16(prod_cross, swap_adjacent_epi16(prod_cross));
    r1_candidate = canonicalize_nonnegative_epi16(r1_candidate);

    return _mm256_blend_epi16(r0_candidate, r1_candidate, 0xAA);
}

int incom_mul_acc_avx2(int16_t *acc, const int16_t *a, const int16_t *b)
{
    /*
     * Fused incomplete-base multiplication and accumulation for the imported
     * q=257,N=256 AVX2 NTT format.
     *
     * For each base block (a0,a1),(b0,b1):
     *   r0 = a0*b0 + zeta*a1*b1
     *   r1 = a1*b0 + a0*b1
     *   acc <- acc + r mod q
     *
     * The interleaved layout [a0,a1,a0,a1,...] is kept throughout the vector
     * computation.  Adjacent 16-bit lanes are swapped to obtain the odd/even
     * partners, so no temporary product polynomial is materialized.
     */
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_load_si256((const __m256i *)(b + i));
        __m256i vacc = _mm256_load_si256((const __m256i *)(acc + i));

        /* Build [z0,z0,z1,z1,...,z7,z7] for 8 base blocks in this vector. */
        __m256i z16 = _mm256_set_epi16(
            poly_basemul_f2[(i >> 1) + 7], poly_basemul_f2[(i >> 1) + 7],
            poly_basemul_f2[(i >> 1) + 6], poly_basemul_f2[(i >> 1) + 6],
            poly_basemul_f2[(i >> 1) + 5], poly_basemul_f2[(i >> 1) + 5],
            poly_basemul_f2[(i >> 1) + 4], poly_basemul_f2[(i >> 1) + 4],
            poly_basemul_f2[(i >> 1) + 3], poly_basemul_f2[(i >> 1) + 3],
            poly_basemul_f2[(i >> 1) + 2], poly_basemul_f2[(i >> 1) + 2],
            poly_basemul_f2[(i >> 1) + 1], poly_basemul_f2[(i >> 1) + 1],
            poly_basemul_f2[(i >> 1) + 0], poly_basemul_f2[(i >> 1) + 0]);

        __m256i r = basemul_f2_product_epi16(va, vb, z16);
        __m256i sum = _mm256_add_epi16(vacc, r);
        sum = reduce_upto_3q_epi16(sum);
        _mm256_store_si256((__m256i *)(acc + i), sum);
    }

    return 0;
}

int incom_mul_add2_avx2(int16_t *out,
                        const int16_t *a0, const int16_t *b0,
                        const int16_t *a1, const int16_t *b1)
{
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i va0 = _mm256_load_si256((const __m256i *)(a0 + i));
        __m256i vb0 = _mm256_load_si256((const __m256i *)(b0 + i));
        __m256i va1 = _mm256_load_si256((const __m256i *)(a1 + i));
        __m256i vb1 = _mm256_load_si256((const __m256i *)(b1 + i));

        __m256i z16 = _mm256_set_epi16(
            poly_basemul_f2[(i >> 1) + 7], poly_basemul_f2[(i >> 1) + 7],
            poly_basemul_f2[(i >> 1) + 6], poly_basemul_f2[(i >> 1) + 6],
            poly_basemul_f2[(i >> 1) + 5], poly_basemul_f2[(i >> 1) + 5],
            poly_basemul_f2[(i >> 1) + 4], poly_basemul_f2[(i >> 1) + 4],
            poly_basemul_f2[(i >> 1) + 3], poly_basemul_f2[(i >> 1) + 3],
            poly_basemul_f2[(i >> 1) + 2], poly_basemul_f2[(i >> 1) + 2],
            poly_basemul_f2[(i >> 1) + 1], poly_basemul_f2[(i >> 1) + 1],
            poly_basemul_f2[(i >> 1) + 0], poly_basemul_f2[(i >> 1) + 0]);

        __m256i sum = reduce_upto_3q_epi16(basemul_f2_product_epi16(va0, vb0, z16));
        sum = reduce_upto_3q_epi16(_mm256_add_epi16(sum, basemul_f2_product_epi16(va1, vb1, z16)));
        _mm256_store_si256((__m256i *)(out + i), sum);
    }

    return 0;
}

int incom_mul_mod_avx2(int16_t *a, int16_t *b, int16_t *c)  //  c=a*b
{
    int i;

    int16_t __attribute__((aligned(64))) r0[RL_KEM_N/2],r1[RL_KEM_N/2],a0[RL_KEM_N/2],a1[RL_KEM_N/2],b0[RL_KEM_N/2],b1[RL_KEM_N/2];

    for(i=0;i<RL_KEM_N;i+=2)
    {
        a0[i/2]=a[i];
        a1[i/2]=a[i+1];
        b0[i/2]=b[i];
        b1[i/2]=b[i+1];
    }

    __m256i tmp_r0,tmp_r1,tmp_a0,tmp_a1,tmp_b0,tmp_b1,tmp_s,tmp_tmp;
    __m256i tmp_c0, tmp_c1, tmp_x;
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

    for (i = 0; i < RL_KEM_N_Half; i += 16)
    {
        tmp_a0 = _mm256_load_si256((__m256i *)(a0+i));
        tmp_a1 = _mm256_load_si256((__m256i *)(a1+i));
        tmp_b0 = _mm256_load_si256((__m256i *)(b0+i));
        tmp_b1 = _mm256_load_si256((__m256i *)(b1+i));
        tmp_s = _mm256_load_si256((__m256i *)(poly_basemul_f2+i));
        //r[0]=(a[1]*b[1])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r0     = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[0]=(r[0]*zeta)%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_r0, tmp_s);
        tmp_c1 = _mm256_mulhi_epi16(tmp_r0, tmp_s);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r0 = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[0]+=(a[0]*b[0])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
        tmp_r0 = _mm256_add_epi16(tmp_r0,tmp_tmp);
        tmp_r0=_mm256_add_epi16(tmp_r0, _mm256_and_si256(_mm256_srai_epi16(tmp_r0, 15), tmp_Q));



        //r[1]=(a[0]*b[1])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r1 = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[1]+=(a[1]*b[0])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
        tmp_r1 = _mm256_add_epi16(tmp_r1,tmp_tmp);
        tmp_r1=_mm256_add_epi16(tmp_r1, _mm256_and_si256(_mm256_srai_epi16(tmp_r1, 15), tmp_Q));



        //store
        _mm256_store_si256((__m256i *)(r0+i), tmp_r0);
        _mm256_store_si256((__m256i *)(r1+i), tmp_r1);
    }
    for(i=0;i<RL_KEM_N;i+=2)
    {
        c[i]=r0[i/2];
        c[i+1]=r1[i/2];
    }

    return 0;
}

inline int Reduce(__m256i *r) //对系数按照2的幂次结构求模
{
    __m256i t,u,mid;
    t = _mm256_srai_epi16(*r, 13);  // t=r >>13
    mid =_mm256_set1_epi16(8191); // mid =normal  mod 2^{13}
    u =  _mm256_and_si256(mid, *r); // u=mid & r
    u =_mm256_sub_epi16(u, t);  // u=u-t
    *r =_mm256_add_epi16(u, _mm256_slli_epi16(t, 9)); //r = u + t<<9

    return 0;
}


// U 按照模数结构求模，
// V=V*S%RL_KEM_Q, A1=(U+V)%RL_KEM_Q, A2=(U-V)%RL_KEM_Q  S为本原单位根
// inline int ntt_core(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)  //对蝴蝶变换输入U求模
// {
//     __m256i tmp_c0, tmp_c1, tmp_x;

//     __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
//     __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    
//     //V*S  系数*本原单位根
//     tmp_c0 = _mm256_mullo_epi16(*V, *S);
//     tmp_c1 = _mm256_mulhi_epi16(*V, *S);
    
//     //对乘积利用Montgomery求模，并map到(0,q)
//     //V*S montgomery mod RL_KEM_Q
//     tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//     tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//     *V     = _mm256_sub_epi16(tmp_c1, tmp_x);
//     //if c1<0 then +RL_KEM_Q
//   //  *V     = _mm256_add_epi16(*V, _mm256_and_si256(_mm256_srai_epi16(*V, 15), tmp_Q));

//     //U mod RL_KEM_Q 【Lazy Reduction对加减结果进行系数求模】
//     Reduce(U);
    
//     //(U mod RL_KEM_Q) + V -RL_KEM_Q
//     *A1    = _mm256_add_epi16(*U, *V);
//   //  *A1    = _mm256_sub_epi16(*A1, tmp_Q);
    
//     //(U mod RL_KEM_Q) - V
//     *A2   = _mm256_sub_epi16(*U, *V);

//     return 0;
// }
inline int ntt_core(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)  //对蝴蝶变换输入U求模
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_B = _mm256_set1_epi16(B_Q); //RL_KEM_Q^{-1} mod 2^16
    
    //V*S  系数*本原单位根
    tmp_c0 = _mm256_mullo_epi16(*V, *S);
    tmp_c1 = _mm256_mulhi_epi16(*V, *S);
    
    //对乘积利用Montgomery求模，并map到(0,q)
    //V*S montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *V     = _mm256_sub_epi16(tmp_c1, tmp_x);

    //Mont(U) 
    tmp_c0 = _mm256_mullo_epi16(*U, tmp_B);
    tmp_c1 = _mm256_mulhi_epi16(*U, tmp_B);
        // Montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *U    = _mm256_sub_epi16(tmp_c1, tmp_x);
    
    //(U mod RL_KEM_Q) + V 
    *A1    = _mm256_add_epi16(*U, *V);
    
    //(U mod RL_KEM_Q) - V
    *A2   = _mm256_sub_epi16(*U, *V);

    return 0;
}
inline int ntt_core_lazymod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
     __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //V*S 系数*本原单位根
    tmp_c0 = _mm256_mullo_epi16(*V, *S);
    tmp_c1 = _mm256_mulhi_epi16(*V, *S);
    
    //对乘积利用Montgomery求模，并map到(0,RL_KEM_Q)
    //V*S montgomery map full mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *V     = _mm256_sub_epi16(tmp_c1, tmp_x);
    //if c1<0 then +RL_KEM_Q
  //  *V     = _mm256_add_epi16(*V, _mm256_and_si256(_mm256_srai_epi16(*V, 15), tmp_Q));

    //U + V - RL_KEM_Q
    *A1    = _mm256_add_epi16(*U, *V);
   // *A1    = _mm256_sub_epi16(*A1, tmp_Q); //去掉出错
    
    //U - V
    *A2   = _mm256_sub_epi16(*U, *V);

    return 0;

    
}

//  A1=(U+V)%RL_KEM_Q, A2=(U-V)*S%RL_KEM_Q
inline int intt_core(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=(U+V)%RL_KEM_Q
    *A1 = _mm256_add_epi16(*U, *V);
    *A1 = _mm256_sub_epi16(*A1, tmp_Q);
    *A1 = _mm256_add_epi16(*A1, _mm256_and_si256(_mm256_srai_epi16(*A1, 15), tmp_Q));
    //A2=V=(U-V)%RL_KEM_Q 
    *A2 = _mm256_sub_epi16(*U, *V);
  //  *A2    = _mm256_add_epi16(*A2, _mm256_and_si256(_mm256_srai_epi16(*A2, 15), tmp_Q));
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);
    //if c1<0 then +RL_KEM_Q
    *A2    = _mm256_add_epi16(*A2, _mm256_and_si256(_mm256_srai_epi16(*A2, 15), tmp_Q));

    return 0;
}

inline int intt_core_final(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_b = _mm256_set1_epi16(INVERSE_N_BETA2);//N^-1*2^16*2*16 mod RL_KEM_Q
     //A1=(U+V)%RL_KEM_Q
    *A1 = _mm256_add_epi16(*U, *V);
    *A1 = _mm256_sub_epi16(*A1, tmp_Q);
   // *A1 = _mm256_add_epi16(*A1, _mm256_and_si256(_mm256_srai_epi16(*A1, 15), tmp_Q));
    //c=A1*b
    tmp_c0 = _mm256_mullo_epi16(*A1, tmp_b);
    tmp_c1 = _mm256_mulhi_epi16(*A1, tmp_b);
    //montgomery mod RL_KEM_Q
    tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A1   = _mm256_sub_epi16(tmp_c1, tmp_x);
    //if c1<0 then +RL_KEM_Q
    *A1 = _mm256_add_epi16(*A1, _mm256_and_si256(_mm256_srai_epi16(*A1, 15), tmp_Q));

    //A2=(U-V)%RL_KEM_Q 
    *A2 = _mm256_sub_epi16(*U, *V);
    //*A2    = _mm256_add_epi16(*A2, _mm256_and_si256(_mm256_srai_epi16(*A2, 15), tmp_Q));
     //c=A2*S
    tmp_c0 = _mm256_mullo_epi16(*A2,  *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2,  *S);
    //montgomery mod RL_KEM_Q
    tmp_x = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2   = _mm256_sub_epi16(tmp_c1, tmp_x);
    
    //if A2<0 then +RL_KEM_Q
    *A2    = _mm256_add_epi16(*A2, _mm256_and_si256(_mm256_srai_epi16(*A2, 15), tmp_Q));

    return 0;
}
int NTT_AVX2(int16_t *a)
{
    int i, j, s;

    __m256i tmp_U, tmp_V, tmp_A1, tmp_A2, tmp_S;

    // __m256i tmp_shuffle16_pre = _mm256_set_epi8(0x0f, 0x0e, 0x0b, 0x0a, 0x07, 0x06, 0x03, 0x02, 0x0d, 0x0c, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00,
    //                                     0x0f, 0x0e, 0x0b, 0x0a, 0x07, 0x06, 0x03, 0x02, 0x0d, 0x0c, 0x09, 0x08, 0x05, 0x04, 0x01, 0x00);

    // m：使用不同本原单位根的个数 （蝴蝶变换的组数）
    //level 1  step=N/2=128  m=1
    //load root 本原单位根存到 tmp_S
    tmp_S = _mm256_set1_epi16(f[1]);
    for(j=0;j<128;j+=16) //因为每个系数16bit存储，申请256bit的空间，所以一次可以计算16个系数乘法
    {
        //load  把系数保存到tmp_U tmp_V ，step=N/2
        tmp_U = _mm256_load_si256((__m256i *)(a +j));  //f[m+ (s++)]
        tmp_V = _mm256_load_si256((__m256i *)(a  + 128 +j)); //第一层 step=N/2
        //
        //执行CT蝴蝶变换
        // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store  把蝴蝶变换的结果保存到系数向量中
        _mm256_store_si256((__m256i *)(a +j), tmp_A1);
        _mm256_store_si256((__m256i *)(a  + 128 +j), tmp_A2);
    }
    
    //level 2   step=N/4=64  m=2
    s=0;
    for (i = 0; i < RL_KEM_N; i += 128) //i=0,256  i=start index  循环m=2次
    {
        //load root
        tmp_S = _mm256_set1_epi16(f[2 +(s++)]);  //f[m+ (s++)]： f[2], f[3]
        for(j=0;j<64;j+=16) // j=0~step
            //i+j:每个蝴蝶变换的第一个输入， i+j+step：第二个输入 两个输入之间间隔一个step的长度
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 64+j));
            // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 64+j), tmp_A2);
        }
    }

    //level 3  step=N/8=32  m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 64)  //  循环m=4次 i=start index
    {
        //load root
        tmp_S = _mm256_set1_epi16(f[4 +(s++)]);   //f[m+ (s++)]：f[4],f[5],f[6],f[7]
        for(j=0;j<32;j+=16) //j=0~step i+j: first input  i+j+step: second input
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 32+j));
            // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 32 +j), tmp_A2);
        }
    }

    //level 4 step=N/16=16 m=8
    s=0;
    for (i = 0; i < RL_KEM_N; i += 32)  //循环8次 i=start index
    {
         //load
        tmp_S = _mm256_set1_epi16(f[8 +(s++)]);
        //j=0
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        // V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
       
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    //level 5  step=N/32=8 m=16  step不够一次并行计算 permute处理  2个参与
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(f[16 + s + 1], f[16 + s + 1], f[16 + s + 1], f[16 + s + 1], f[16 + s + 1], f[16 + s + 1], f[16 + s + 1], f[16 + s + 1],
                                 f[16 + s], f[16 + s], f[16 + s], f[16 + s], f[16 + s], f[16 + s], f[16 + s], f[16 + s]);  //256长 16个item，每个长16bit
        s += 2;
        //load
        tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
        tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);  //A1低128bit + A2低128bit
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);  //A1高128bit + A2高128bit
        // _mm256_permute2x128_si256
       // 功能：由两个256bit的数，选择其中的一部分构成一个新的256bit数。

        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }


    //level 6  step=N/64=4 m=32  step不够一次并行计算 permute处理  4个参与
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(f[32 + s + 3], f[32 + s + 3], f[32 + s + 3], f[32 + s + 3], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1], f[32 + s + 1],
                                 f[32 + s + 2], f[32 + s + 2], f[32 + s + 2], f[32 + s + 2], f[32 + s], f[32 + s], f[32 + s], f[32 + s]);
        s += 4;
        //load
        tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
        tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
        //permute
        tmp_A1 = _mm256_permute4x64_epi64(tmp_A1, 0xd8);
        tmp_A2 = _mm256_permute4x64_epi64(tmp_A2, 0xd8);
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    //level 7  step=N/128=2  m=64  step不够一次并行计算，permute处理 8个参与
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(f[64 + s + 7], f[64 + s + 7], f[64 + s + 3], f[64 + s + 3], f[64 + s + 5], f[64 + s + 5], f[64 + s + 1], f[64 + s + 1],
                                 f[64 + s + 6], f[64 + s + 6], f[64 + s + 2], f[64 + s + 2], f[64 + s + 4], f[64 + s + 4], f[64 + s], f[64 + s]);
        s += 8;
        //load
        tmp_A1 = _mm256_load_si256((__m256i *)(a + i));
        tmp_A2 = _mm256_load_si256((__m256i *)(a + i + 16));
        //permute
        tmp_A1 = _mm256_shuffle_epi32(tmp_A1, 0xd8);
        tmp_A1 = _mm256_permute4x64_epi64(tmp_A1, 0xd8);

        tmp_A2 = _mm256_shuffle_epi32(tmp_A2, 0xd8);
        tmp_A2 = _mm256_permute4x64_epi64(tmp_A2, 0xd8);

        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //V*S
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        // ntt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        ntt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }


    return 0;
}

int INTT_AVX2(int16_t *a)
{
    int i, j, s;

    __m256i tmp_U, tmp_V, tmp_A1, tmp_A2, tmp_S;


    //level 7  step=N/128=2  m=64  8-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[64 + s + 7], fn[64 + s + 7], fn[64 + s + 3], fn[64 + s + 3], fn[64 + s + 5], fn[64 + s + 5], fn[64 + s + 1], fn[64 + s + 1],
                                 fn[64 + s + 6], fn[64 + s + 6], fn[64 + s + 2], fn[64 + s + 2], fn[64 + s + 4], fn[64 + s + 4], fn[64 + s], fn[64 + s]);
        s += 8;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);

        tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
        tmp_U = _mm256_shuffle_epi32(tmp_U, 0xd8);

        tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
        tmp_V = _mm256_shuffle_epi32(tmp_V, 0xd8);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }

    //level 6  step=N/64=4  m=32  4-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1],
                                 fn[32 + s + 2], fn[32 + s + 2], fn[32 + s + 2], fn[32 + s + 2], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s]);
        s += 4;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);

        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
        tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
        //store  
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }

    //level 5   step=N/32=8  m=16  2-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1],
                                 fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s]);
        s += 2;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }
    //level 4  step=N/16=16   m=8  
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set1_epi16(fn[8 + s++]);
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
       
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    //level 3  step=N/8=32   m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 64)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[4 +(s++)]);
        //load block 1
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 32));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 32), tmp_A2);

         //load block 2
        tmp_U = _mm256_load_si256((__m256i *)(a + i+16));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 48));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i+16), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 48), tmp_A2);
    }

    //level 2  step=N/4=64  m=2
    s=0;
    for (i = 0; i < RL_KEM_N; i += 128)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[2 +(s++)]);
        for(j=0;j<64;j+=16)
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 64+j));
            //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            intt_core(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 64+j), tmp_A2);
        }
    }

    //level 1 step=N/2=128  m=1
    tmp_S = _mm256_set1_epi16(Normal);
    //fn[1]*INVERSE_N_BETA%RL_KEM_Q, 
    //point-mul and inverse-n-mul use montgomery mod, so two 2^16 factor are embeded in INVERSE_N,
    //however, for the final level, the right half inverse-n-mul is combined with *fn[1]
    for(j=0;j<128;j+=16)
    {
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a +j));
        tmp_V = _mm256_load_si256((__m256i *)(a  + 128 +j));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_final(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a +j), tmp_A1);
        _mm256_store_si256((__m256i *)(a  + 128 +j), tmp_A2);
    }
    
    // *1/N %RL_KEM_Q
    //intt_post(a);

    return 0;
}

int poly_mul_ntt_avx2(const uint16_t *a, const uint16_t *s, uint16_t *b)
{
    int16_t __attribute__((aligned(64))) a_buf[RL_KEM_N], s_buf[RL_KEM_N], b_buf[RL_KEM_N];

    memcpy(a_buf, a, RL_KEM_N * sizeof(a_buf[0]));
    memcpy(s_buf, s, RL_KEM_N * sizeof(s_buf[0]));

    //NTT form
    NTT_AVX2(a_buf);
    NTT_AVX2(s_buf);

    //point mul
    incom_mul_mod_avx2(a_buf, s_buf, b_buf);

    //INTT form
    INTT_AVX2(b_buf);
    memcpy(b, b_buf, RL_KEM_N * (sizeof(b_buf[0])));

    return 0;
}

//********************************Lazy INTT************************************//

int lazyincom_mul_mod_avx2(int16_t *a, int16_t *b, int16_t *c)  //  c=a*b
{
    int i;

    int16_t __attribute__((aligned(64))) r0[RL_KEM_N_Half],r1[RL_KEM_N_Half],a0[RL_KEM_N_Half],a1[RL_KEM_N_Half],b0[RL_KEM_N_Half],b1[RL_KEM_N_Half];

    for(i=0;i<RL_KEM_N;i+=2)
    {
        a0[i>>1]=a[i];
        a1[i>>1]=a[i+1];
        b0[i>>1]=b[i];
        b1[i>>1]=b[i+1];
    }


    __m256i tmp_r0,tmp_r1,tmp_a0,tmp_a1,tmp_b0,tmp_b1,tmp_s,tmp_tmp;
    __m256i tmp_c0, tmp_c1, tmp_x;
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

    for (i = 0; i < RL_KEM_N_Half; i += 16)
    {
        tmp_a0 = _mm256_load_si256((__m256i *)(a0+i));
        tmp_a1 = _mm256_load_si256((__m256i *)(a1+i));
        tmp_b0 = _mm256_load_si256((__m256i *)(b0+i));
        tmp_b1 = _mm256_load_si256((__m256i *)(b1+i));
        tmp_s = _mm256_load_si256((__m256i *)(poly_basemul_f2+i));
        //r[0]=(a[1]*b[1])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r0     = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[0]=(r[0]*zeta)%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_r0, tmp_s);
        tmp_c1 = _mm256_mulhi_epi16(tmp_r0, tmp_s);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r0 = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[0]+=(a[0]*b[0])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
        tmp_r0 = _mm256_add_epi16(tmp_r0,tmp_tmp);
        

        //r[1]=(a[0]*b[1])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b1);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b1);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_r1 = _mm256_sub_epi16(tmp_c1, tmp_x);

        //r[1]+=(a[1]*b[0])%RL_KEM_Q;
        tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b0);
        tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b0);
        tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
        tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
        tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
        tmp_r1 = _mm256_add_epi16(tmp_r1,tmp_tmp);


        //store
        _mm256_store_si256((__m256i *)(r0+i), tmp_r0);
        _mm256_store_si256((__m256i *)(r1+i), tmp_r1);
    }
    for(i=0;i<RL_KEM_N;i+=2)
    {
        c[i]=r0[i>>1];
        c[i+1]=r1[i>>1];
    }

    return 0;
}


// int lazyincom_mul_mod_avx2(int16_t *a, int16_t *b, int16_t *c)  //  c=a*b
// {
//     int i;

//     int16_t __attribute__((aligned(64))) r0[RL_KEM_N_Half],r1[RL_KEM_N_Half],a0[RL_KEM_N_Half],a1[RL_KEM_N_Half],b0[RL_KEM_N_Half],b1[RL_KEM_N_Half];

//     for(i=0;i<RL_KEM_N;i+=2)
//     {
//         a0[i>>1]=a[i];
//         a1[i>>1]=a[i+1];
//         b0[i>>1]=b[i];
//         b1[i>>1]=b[i+1];
//     }


//     __m256i tmp_r0,tmp_r1,tmp_a0,tmp_a1,tmp_b0,tmp_b1,tmp_s,tmp_tmp;
//     __m256i tmp_c0, tmp_c1, tmp_x;
//     __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16, for montgomery
//     __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);

//     for (i = 0; i < RL_KEM_N_Half; i += 16)
//     {
//         tmp_a0 = _mm256_load_si256((__m256i *)(a0+i));
//         tmp_a1 = _mm256_load_si256((__m256i *)(a1+i));
//         tmp_b0 = _mm256_load_si256((__m256i *)(b0+i));
//         tmp_b1 = _mm256_load_si256((__m256i *)(b1+i));
//         tmp_s = _mm256_load_si256((__m256i *)(poly_basemul_f2+i));
//         //r[0]=(a[1]*b[1])%RL_KEM_Q;
//         tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b1);
//         tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b1);
//         tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//         tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//         tmp_r0     = _mm256_sub_epi16(tmp_c1, tmp_x);

//         //r[0]=(r[0]*zeta)%RL_KEM_Q;
//         tmp_c0 = _mm256_mullo_epi16(tmp_r0, tmp_s);
//         tmp_c1 = _mm256_mulhi_epi16(tmp_r0, tmp_s);
//         tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//         tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//         tmp_r0 = _mm256_sub_epi16(tmp_c1, tmp_x);

//         //r[0]+=(a[0]*b[0])%RL_KEM_Q;
//         tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b0);
//         tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b0);
//         tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//         tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//         tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
//         tmp_r0 = _mm256_add_epi16(tmp_r0,tmp_tmp);
        

//         //r[1]=(a[0]*b[1])%RL_KEM_Q;
//         tmp_c0 = _mm256_mullo_epi16(tmp_a0, tmp_b1);
//         tmp_c1 = _mm256_mulhi_epi16(tmp_a0, tmp_b1);
//         tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//         tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//         tmp_r1 = _mm256_sub_epi16(tmp_c1, tmp_x);

//         //r[1]+=(a[1]*b[0])%RL_KEM_Q;
//         tmp_c0 = _mm256_mullo_epi16(tmp_a1, tmp_b0);
//         tmp_c1 = _mm256_mulhi_epi16(tmp_a1, tmp_b0);
//         tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
//         tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
//         tmp_tmp = _mm256_sub_epi16(tmp_c1, tmp_x);
//         tmp_r1 = _mm256_add_epi16(tmp_r1,tmp_tmp);

//         tmp_c0 =  _mm256_unpacklo_epi16 (tmp_r0, tmp_r1);
//         tmp_c1 =  _mm256_unpackhi_epi16 (tmp_r0, tmp_r1);
//         tmp_r0 = _mm256_permute2x128_si256(tmp_c0, tmp_c1, 0x20);
//         tmp_r1 = _mm256_permute2x128_si256(tmp_c0, tmp_c1, 0x31);
//         //store
//         _mm256_store_si256((__m256i *)(c+i), tmp_r0);
//         _mm256_store_si256((__m256i *)(c+32+i), tmp_r1);
//     }


//     return 0;
// }
inline int intt_first_level(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=(U+V)-RL_KEM_Q
    *A1 = _mm256_add_epi16(*U, *V);
    *A1 = _mm256_sub_epi16(*A1, tmp_Q);
    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

    return 0;
}

inline int intt_core_lazymod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=U+V
    *A1 = _mm256_add_epi16(*U, *V);

    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

    return 0;
}

inline int intt_core_extramod(__m256i *U, __m256i *V, __m256i *S,__m256i *A1,__m256i *A2)
{
    __m256i tmp_c0, tmp_c1, tmp_x;

    __m256i tmp_Q = _mm256_set1_epi16(RL_KEM_Q);
    __m256i tmp_QINV = _mm256_set1_epi16(INVERSE_Q); //RL_KEM_Q^{-1} mod 2^16
    __m256i tmp_B = _mm256_set1_epi16(B_Q); //RL_KEM_Q^{-1} mod 2^16
    //A1=Mont((U+V)* (beta mod q))
        //A1=Mont(U+V)
    *A1 = _mm256_add_epi16(*U, *V);
        //(U+V)*beta
    tmp_c0 = _mm256_mullo_epi16(*A1, tmp_B);
    tmp_c1 = _mm256_mulhi_epi16(*A1, tmp_B);
        // Montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A1    = _mm256_sub_epi16(tmp_c1, tmp_x);

    //V=(U-V)
    *A2 = _mm256_sub_epi16(*U, *V);
    //V*S
    tmp_c0 = _mm256_mullo_epi16(*A2, *S);
    tmp_c1 = _mm256_mulhi_epi16(*A2, *S);
    //mod RL_KEM_Q
    //montgomery mod RL_KEM_Q
    tmp_x  = _mm256_mullo_epi16(tmp_c0, tmp_QINV);
    tmp_x  = _mm256_mulhi_epi16(tmp_x, tmp_Q);
    *A2    = _mm256_sub_epi16(tmp_c1, tmp_x);

    return 0;
}
int INTT_AVX2_Lazy(int16_t *a)
{
    int i, j, s;

    __m256i tmp_U, tmp_V, tmp_A1, tmp_A2, tmp_S;


    //level 7  step=N/128=2  m=64  8-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[64 + s + 7], fn[64 + s + 7], fn[64 + s + 3], fn[64 + s + 3], fn[64 + s + 5], fn[64 + s + 5], fn[64 + s + 1], fn[64 + s + 1],
                                 fn[64 + s + 6], fn[64 + s + 6], fn[64 + s + 2], fn[64 + s + 2], fn[64 + s + 4], fn[64 + s + 4], fn[64 + s], fn[64 + s]);
        s += 8;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);

        tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
        tmp_U = _mm256_shuffle_epi32(tmp_U, 0xd8);

        tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
        tmp_V = _mm256_shuffle_epi32(tmp_V, 0xd8);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }

    //level 6  step=N/64=4  m=32  4-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 3], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1], fn[32 + s + 1],
                                 fn[32 + s + 2], fn[32 + s + 2], fn[32 + s + 2], fn[32 + s + 2], fn[32 + s], fn[32 + s], fn[32 + s], fn[32 + s]);
        s += 4;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);

        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        tmp_U = _mm256_permute4x64_epi64(tmp_U, 0xd8);
        tmp_V = _mm256_permute4x64_epi64(tmp_V, 0xd8);
        //store  
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }

    //level 5   step=N/32=8  m=16  2-permute
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set_epi16(fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1], fn[16 + s + 1],
                                 fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s], fn[16 + s]);
        s += 2;
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //permute
        tmp_U = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x20);
        tmp_V = _mm256_permute2x128_si256(tmp_A1, tmp_A2, 0x31);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_U);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_V);
    }
    //level 4  step=N/16=16   m=8  
    s = 0;
    for (i = 0; i < RL_KEM_N; i += 32)
    {
        //S=f[m+i];
        tmp_S = _mm256_set1_epi16(fn[8 + s++]);
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 16));
       
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
       
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 16), tmp_A2);
    }

    //level 3  step=N/8=32   m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 64)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[4 +(s++)]);
        //load block 1
        tmp_U = _mm256_load_si256((__m256i *)(a + i));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 32));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 32), tmp_A2);

         //load block 2
        tmp_U = _mm256_load_si256((__m256i *)(a + i+16));
        tmp_V = _mm256_load_si256((__m256i *)(a + i + 48));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a + i+16), tmp_A1);
        _mm256_store_si256((__m256i *)(a + i + 48), tmp_A2);
    }

    //level 2  step=N/4=64  m=4
    s=0;
    for (i = 0; i < RL_KEM_N; i += 128)
    {
        //load root
        tmp_S = _mm256_set1_epi16(fn[4 +(s++)]);
        for(j=0;j<64;j+=16)
        {
            //load
            tmp_U = _mm256_load_si256((__m256i *)(a + i+j));
            tmp_V = _mm256_load_si256((__m256i *)(a + i + 64+j));
            //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
            intt_core_lazymod(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
            //store
            _mm256_store_si256((__m256i *)(a + i+j), tmp_A1);
            _mm256_store_si256((__m256i *)(a + i + 64+j), tmp_A2);
        }
    }

    
    //level 1 step=N/2=128  m=1
    tmp_S = _mm256_set1_epi16(Normal);
    //fn[1]*INVERSE_N_BETA%RL_KEM_Q, 
    //point-mul and inverse-n-mul use montgomery mod, so two 2^16 factor are embeded in INVERSE_N,
    //however, for the final level, the right half inverse-n-mul is combined with *fn[1]
    for(j=0;j<128;j+=16)
    {
        //load
        tmp_U = _mm256_load_si256((__m256i *)(a +j));
        tmp_V = _mm256_load_si256((__m256i *)(a  + 128 +j));
        //V*S%RL_KEM_Q, (U+V)%RL_KEM_Q, (U-V)%RL_KEM_Q
        intt_core_final(&tmp_U,&tmp_V,&tmp_S,&tmp_A1,&tmp_A2);
        //store
        _mm256_store_si256((__m256i *)(a +j), tmp_A1);
        _mm256_store_si256((__m256i *)(a  + 128 +j), tmp_A2);
    }
    
    // *1/N %RL_KEM_Q
    //intt_post(a);

    return 0;
}

int poly_mul_ntt_avx2_lazy(const uint16_t *a, const uint16_t *s, uint16_t *b)
{
    int16_t __attribute__((aligned(64))) a_buf[RL_KEM_N], s_buf[RL_KEM_N], b_buf[RL_KEM_N];

    memcpy(a_buf, a, RL_KEM_N * sizeof(a_buf[0]));
    memcpy(s_buf, s, RL_KEM_N * sizeof(s_buf[0]));

    //NTT form
    NTT_AVX2(a_buf);
    NTT_AVX2(s_buf);

    //point mul
    lazyincom_mul_mod_avx2(a_buf, s_buf, b_buf);

    //INTT form
    INTT_AVX2_Lazy(b_buf);
    memcpy(b, b_buf, RL_KEM_N * (sizeof(b_buf[0])));

    return 0;
}
