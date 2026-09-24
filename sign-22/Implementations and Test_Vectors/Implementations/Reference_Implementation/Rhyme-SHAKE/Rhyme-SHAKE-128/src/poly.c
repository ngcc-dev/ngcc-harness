/*************************************************
* File:        poly.c
*
* Description: Polynomial arithmetic over R_q = Z_q[X]/(X^N + 1):
*              addition, subtraction, NTT-domain multiplication, modular
*              reductions, norm checks, exact small-coefficient negacyclic
*              multiplication, and rejection sampling of uniform polynomials.
**************************************************/
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "symmetric.h"

/*************************************************
* Name:        poly_zero
*
* Description: Sets all coefficients of a polynomial to zero.
*
* Arguments:   - poly *p: pointer to the polynomial to clear
**************************************************/
void poly_zero(poly *p) { memset(p->coeffs, 0, sizeof p->coeffs); }

/*************************************************
* Name:        poly_add
*
* Description: Adds two polynomials coefficient-wise. No modular reduction
*              is performed; callers must ensure coefficients stay in range.
*
* Arguments:   - poly *c:       pointer to output polynomial (c = a + b)
*              - const poly *a: pointer to first summand
*              - const poly *b: pointer to second summand
**************************************************/
void poly_add(poly *c, const poly *a, const poly *b) {
    for (int i = 0; i < N; i++) c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtracts two polynomials coefficient-wise (c = a - b). No
*              modular reduction is performed.
*
* Arguments:   - poly *c:       pointer to output polynomial
*              - const poly *a: pointer to minuend
*              - const poly *b: pointer to subtrahend
**************************************************/
void poly_sub(poly *c, const poly *a, const poly *b) {
    for (int i = 0; i < N; i++) c->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}

/*************************************************
* Name:        poly_ntt
*
* Description: In-place forward NTT of a polynomial. Output is in bit-reversed
*              order and in Montgomery domain as produced by ntt().
*
* Arguments:   - poly *p: pointer to the polynomial to transform in place
**************************************************/
void poly_ntt(poly *p) { ntt(p->coeffs); }

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: In-place inverse NTT of a polynomial, leaving the result in
*              Montgomery domain.
*
* Arguments:   - poly *p: pointer to the polynomial to transform in place
**************************************************/
void poly_invntt_tomont(poly *p) { invntt_tomont(p->coeffs); }

/*************************************************
* Name:        poly_basemul
*
* Description: Pointwise (base) multiplication of two polynomials in NTT
*              domain (c = a o b).
*
* Arguments:   - poly *c:       pointer to output polynomial
*              - const poly *a: pointer to first operand (NTT domain)
*              - const poly *b: pointer to second operand (NTT domain)
**************************************************/
void poly_basemul(poly *c, const poly *a, const poly *b) { basemul(c->coeffs, a->coeffs, b->coeffs); }

/*************************************************
* Name:        poly_basemul_acc
*
* Description: Pointwise multiplication with accumulation in NTT domain
*              (c += a o b), used for matrix-vector products.
*
* Arguments:   - poly *c:       pointer to accumulator polynomial (NTT domain)
*              - const poly *a: pointer to first operand (NTT domain)
*              - const poly *b: pointer to second operand (NTT domain)
**************************************************/
void poly_basemul_acc(poly *c, const poly *a, const poly *b) { ntt_pointwise_acc(c->coeffs, a->coeffs, b->coeffs); }

/*************************************************
* Name:        poly_freeze
*
* Description: Reduces every coefficient to the centered representative
*              modulo Q.
*
* Arguments:   - poly *p: pointer to the polynomial to reduce in place
**************************************************/
void poly_freeze(poly *p)   { for (int i = 0; i < N; i++) p->coeffs[i] = freeze(p->coeffs[i]); }

/*************************************************
* Name:        poly_freeze2q
*
* Description: Reduces every coefficient to the centered representative
*              modulo 2Q.
*
* Arguments:   - poly *p: pointer to the polynomial to reduce in place
**************************************************/
void poly_freeze2q(poly *p) { for (int i = 0; i < N; i++) p->coeffs[i] = freeze2q(p->coeffs[i]); }

/*************************************************
* Name:        poly_creduce
*
* Description: Applies a centered Barrett-style reduction to every
*              coefficient modulo Q.
*
* Arguments:   - poly *p: pointer to the polynomial to reduce in place
**************************************************/
void poly_creduce(poly *p)  { for (int i = 0; i < N; i++) p->coeffs[i] = creduce(p->coeffs[i]); }

/*************************************************
* Name:        poly_chknorm
*
* Description: Checks whether the infinity norm of a polynomial is within a
*              given bound. Runs in constant time: all N coefficients are
*              always scanned with no data-dependent early exit, so the running
*              time does not reveal the position or count of any violating
*              coefficient.
*
* Arguments:   - const poly *p:   pointer to the polynomial to check
*              - int32_t bound:   inclusive bound on |coefficient|
*
* Returns:     1 if some |coefficient| > bound, 0 otherwise.
**************************************************/
int poly_chknorm(const poly *p, int32_t bound) {
    int32_t bad = 0;
    for (int i = 0; i < N; i++) {
        int32_t t = p->coeffs[i];
        /* branchless abs */
        int32_t m = t >> 31;          /* all-ones if t<0 else 0 */
        t = (t ^ m) - m;
        /* branchless (t > bound): top bit of (bound - t) is 1 iff t > bound */
        bad |= (int32_t)(((uint32_t)(bound - t)) >> 31);
    }
    return (int)(bad & 1);
}

/*************************************************
* Name:        poly_sqnorm_acc
*
* Description: Accumulates the squared Euclidean (L2) norm of a polynomial
*              into a running 64-bit accumulator.
*
* Arguments:   - uint64_t *acc:   pointer to the accumulator (acc += ||p||^2)
*              - const poly *p:   pointer to the polynomial
**************************************************/
void poly_sqnorm_acc(uint64_t *acc, const poly *p) {
    uint64_t s = 0;
    for (int i = 0; i < N; i++) {
        int64_t t = p->coeffs[i];
        s += (uint64_t)(t * t);
    }
    *acc += s;
}

/*************************************************
* Name:        poly_compare
*
* Description: Byte-wise comparison of the coefficient arrays of two
*              polynomials.
*
* Arguments:   - const poly *a: pointer to first polynomial
*              - const poly *b: pointer to second polynomial
*
* Returns:     0 if the coefficient arrays are identical, nonzero otherwise.
**************************************************/
int poly_compare(const poly *a, const poly *b) {
    return memcmp(a->coeffs, b->coeffs, sizeof a->coeffs);
}

/*************************************************
* Name:        poly_mul_small_acc
*
* Description: Exact negacyclic schoolbook multiplication with accumulation
*              over the integers (c += a*b mod X^N + 1), without modular
*              reduction of the coefficients. Intended for small-coefficient
*              operands where the integer products do not overflow int64.
*
* Arguments:   - poly *c:       pointer to accumulator polynomial
*              - const poly *a: pointer to first operand
*              - const poly *b: pointer to second operand
**************************************************/
void poly_mul_small_acc(poly *c, const poly *a, const poly *b) {
    int64_t acc[2 * N];
    memset(acc, 0, sizeof acc);
    for (int i = 0; i < N; i++) {
        int64_t ai = a->coeffs[i];
        if (ai == 0) continue;
        for (int j = 0; j < N; j++)
            acc[i + j] += ai * b->coeffs[j];
    }
    for (int i = 0; i < N; i++)
        c->coeffs[i] += (int32_t)(acc[i] - acc[i + N]);
}

/*************************************************
* Name:        rej_uniform
*
* Description: Rejection-samples coefficients uniform in {0, ..., Q-1} from a
*              byte buffer, masking each 16-bit sample to POLYQ_BITS bits and
*              accepting values below Q.
*
* Arguments:   - int32_t *a:           output coefficient array
*              - unsigned int len:     number of coefficients still required
*              - const uint8_t *buf:   pointer to random byte buffer
*              - unsigned int buflen:  length of the buffer in bytes
*
* Returns:     Number of coefficients sampled (<= len).
**************************************************/
static unsigned int rej_uniform(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
    unsigned int ctr = 0, pos = 0;
    while (ctr < len && pos + 2 <= buflen) {
        uint32_t t = (uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8);
        pos += 2;
#if POLYQ_BITS == 12
        t &= 0x0FFF;
#elif POLYQ_BITS == 14
        t &= 0x3FFF;
#else
        t &= 0x7FFF;
#endif
        if (t < Q) a[ctr++] = (int32_t)t;
    }
    return ctr;
}

/*************************************************
* Name:        poly_uniform_ntt
*
* Description: Samples a polynomial with coefficients uniform in {0,...,Q-1}
*              (interpreted as an NTT-domain element) deterministically from a
*              seed and nonce, using a stream128 XOF and rejection sampling.
*
* Arguments:   - poly *p:                 pointer to output polynomial
*              - const uint8_t seed[SEEDBYTES]: public seed
*              - uint16_t nonce:          domain-separation nonce
**************************************************/
void poly_uniform_ntt(poly *p, const uint8_t seed[SEEDBYTES], uint16_t nonce) {
    stream128_state state;
    uint8_t buf[STREAM128_BLOCKBYTES * 4 + 2];
    unsigned int ctr, off = 0;
    stream128_init(&state, seed, nonce);
    stream128_squeezeblocks(buf, 4, &state);
    ctr = rej_uniform(p->coeffs, N, buf, STREAM128_BLOCKBYTES * 4);
    while (ctr < N) {
        off = 0;
        stream128_squeezeblocks(buf, 1, &state);
        ctr += rej_uniform(p->coeffs + ctr, N - ctr, buf, STREAM128_BLOCKBYTES);
        (void)off;
    }
}
