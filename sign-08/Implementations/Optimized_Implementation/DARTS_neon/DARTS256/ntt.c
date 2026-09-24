#include "ntt.h"
#include "params.h"
#include "reduce_neon.h"
#include "consts.h"
#include <stdint.h>

/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int32_t a: first factor
*              - int32_t b: second factor
*
* Returns 32-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static inline __attribute__((always_inline))
int32_t fqmul(int32_t a, int32_t b) {
    return darts_scalar_fqmul(a, b);
}

/*************************************************
* Name:        fqinv
*
* Description: Inversion
*
* Arguments:   - int32_t a: first factor a = x * R mod q
*
* Returns 32-bit integer congruent to x^{-1} * R mod q
**************************************************/
static inline __attribute__((always_inline))
int32_t fqinv(int32_t a)
{
    return darts_scalar_fqinv(a);
}

/*************************************************
* Name:        ntt
*
* Description: Inplace number-theoretic transform (NTT) in Rq
*              input is in standard order, output is in bitreversed order
*
* Arguments:   - int32_t r[N]: pointer to input/output vector of elements
*                                of Zq
**************************************************/
void ntt(int32_t r[N]) {
    unsigned int len, start, j, k;
    int32_t zeta;

    k = 1;
    for (len = N/2; len >= 4; len >>= 1) {
        for (start = 0; start < N; start = j +len) {
            zeta = zetas[k++];
            int32x4_t vzeta = vdupq_n_s32(zeta);
            for (j = start; j < start + len; j += 4) {
                int32x4_t vrj = vld1q_s32(&r[j]);
                int32x4_t vrj_len = vld1q_s32(&r[j + len]);
                int32x4_t vt = darts_neon_fqmul(vzeta, vrj_len);
                vst1q_s32(&r[j + len], vsubq_s32(vrj, vt));
                vst1q_s32(&r[j], vaddq_s32(vrj, vt));
            }
        }
    }
}

/*************************************************
* Name:        invntt_tomont
*
* Description: Inplace inverse number-theoretic transform (NTT) in Rq
*              input is bitreversed order, output is in standard order
*              
* Arguments:   - int32_t r[N]: pointer to input/output vector of elements
*                                of Zq
 **************************************************/
void invntt_tomont(int32_t r[N]) {
    unsigned int start, len, j, k;
    int32_t zeta;

    k = 0;
    for (len = 4; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j +len) {
            zeta = zetas_inv[k++];
            int32x4_t vzeta = vdupq_n_s32(zeta);
            for (j = start; j < start + len; j += 4) {
                int32x4_t vrj = vld1q_s32(&r[j]);
                int32x4_t vrj_len = vld1q_s32(&r[j + len]);
                int32x4_t sum = vaddq_s32(vrj, vrj_len);
                int32x4_t diff = vsubq_s32(vrj, vrj_len);
                vst1q_s32(&r[j], sum);
                vst1q_s32(&r[j + len], darts_neon_fqmul(vzeta, diff));
            }
        }
    }

    int32x4_t vfinal_zeta = vdupq_n_s32(zetas_inv[N/4 - 1]);
    for (j = 0; j < N; j += 4) {
        int32x4_t vr = vld1q_s32(&r[j]);
        vst1q_s32(&r[j], darts_neon_fqmul(vr, vfinal_zeta));
    }
}

/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^4-zeta)
*              used for multiplication of elements in Rq in NTT domain
*              (Karatsuba multiplication)
*
* Arguments:   - int32_t r[4]:       pointer to the output polynomial
*              - const int32_t a[4]: pointer to the first factor
*              - const int32_t b[4]: pointer to the second factor
*              - int16_t zeta:       integer defining the reduction polynomial
**************************************************/
void basemul(int32_t r[4],
             const int32_t a[4],
             const int32_t b[4],
             int32_t zeta)
{
    int32_t lo[3], hi[3], mid[3];
    int32_t sa0 = a[0] + a[2], sa1 = a[1] + a[3];
    int32_t sb0 = b[0] + b[2], sb1 = b[1] + b[3];

    lo[0]  = fqmul(a[0], b[0]);
    lo[2]  = fqmul(a[1], b[1]);
    lo[1]  = fqmul(a[0] + a[1], b[0] + b[1]) - lo[0] - lo[2];

    hi[0]  = fqmul(a[2], b[2]);
    hi[2]  = fqmul(a[3], b[3]);
    hi[1]  = fqmul(a[2] + a[3], b[2] + b[3]) - hi[0] - hi[2];

    mid[0] = fqmul(sa0, sb0);
    mid[2] = fqmul(sa1, sb1);
    mid[1] = fqmul(sa0 + sa1, sb0 + sb1) - mid[0] - mid[2];

    r[0] = darts_scalar_freeze(lo[0] + fqmul(zeta, hi[0] + mid[2] - lo[2] - hi[2]));
    r[1] = darts_scalar_freeze(lo[1] + fqmul(zeta, hi[1]));
    r[2] = darts_scalar_freeze(lo[2] + fqmul(zeta, hi[2]) + mid[0] - lo[0] - hi[0]);
    r[3] = darts_scalar_freeze(mid[1] - lo[1] - hi[1]);
}

/*************************************************
* Name:        baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^4-zeta)
*              used for inversion of element in Rq in NTT domain
*
* Arguments:   - int32_t b[4]: pointer to the output polynomial
*              - const int32_t a[4]: pointer to the input polynomial
*              - int32_t zeta: integer defining the reduction polynomial
**************************************************/
int baseinv(int32_t b[4], const int32_t a[4], int32_t zeta) {
	int r1;
	int32_t t0, t1, t2;
    uint32_t x;

    t0 = fqmul(a[2], a[2]) - fqmul(2 * a[1], a[3]);
    t0 = fqmul(a[0], a[0]) + fqmul(t0, zeta);
    t1 = fqmul(a[3], a[3]);
    t1 = fqmul(2 * a[0], a[2]) - fqmul(a[1], a[1]) - fqmul(t1, zeta);
    // R ^ {-1}
    
	t2 = fqmul(t1,t1);        
	t2 = fqmul(t0,t0) - fqmul(t2,zeta);           
    // R ^ {-3}

	t2 = fqinv(t2);           
    // R ^ {5}

    x  = (uint32_t)t2;
    r1 = (-(uint64_t)x) >> 63;

	t0 = montgomery_reduce(fqmul(t0,t2));   
	t1 = montgomery_reduce(fqmul(t1,t2));  
	
    t0 = montgomery_reduce(t0);             
    t1 = montgomery_reduce(t1);    

    t2 = fqmul(t1,zeta); 
    
	b[0] =  fqmul(a[0],t0) - fqmul(a[2],t2);
	b[1] = -fqmul(a[1],t0) + fqmul(a[3],t2);
	b[2] =  fqmul(a[2],t0) - fqmul(a[0],t1);
	b[3] = -fqmul(a[3],t0) + fqmul(a[1],t1);

	return r1 - 1;
}
