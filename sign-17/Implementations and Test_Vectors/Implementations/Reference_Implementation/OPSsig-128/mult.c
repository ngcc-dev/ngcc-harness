#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "mult.h"

/*
 * Correctness argument for the j-i+N indexing:
 *
 * In the negacyclic ring Z[x]/(x^N+1), the product (c*p)[j] is:
 *   Σ_{i: j>=i} c[i]*p[j-i]  -  Σ_{i: j<i} c[i]*p[j-i+N]
 *
 * The 2*N table stores:
 *   table[k+N]  (k ∈ [0,N-1]):  positive form: shift + p[k]
 *   table[k]    (k ∈ [0,N-1]):  complement:    MASK - table[k+N]
 *                                             = MASK - shift - p[k]
 *
 * Accessing table[j-i+N]:
 *   j >= i  →  index ∈ [N, 2N-1]:  reads positive table at k=j-i
 *   j <  i  →  index ∈ [1,  N-1]:  reads complement table at k=j-i+N
 *
 * For c[i]=+1, j<i (negacyclic −p[j-i+N]):
 *   complement at k=j-i+N = MASK_slot - (shift + p[j-i+N])
 *                          = (shift − p[j-i+N])   when MASK_slot = 2*shift
 * The extra +shift cancels when we subtract TAU*shift during unpack.
 *
 * For c[i]=−1, inline: MASK - table[j-i+N]
 *   j >= i: MASK - (shift+p[j-i])    = shift − p[j-i]     ✓
 *   j <  i: MASK - complement[j-i+N] = shift + p[j-i+N]   ✓ (negacyclic +p)
 *
 * In all four cases the extra contribution per nonzero c[i] is +shift,
 * giving a total extra of TAU*shift, which is removed at unpack time.
 */

/* ------------------------------------------------------------------ */
/*  s1 table                                                           */
/* ------------------------------------------------------------------ */

void prepare_s1_table_ops128(uint32_t s1_table[2*N], polyvecl *s1)
{
    uint32_t k, j;
    uint32_t temp;

    for (k = 0; k < N; k++) {
        s1_table[k+N] = 0;
        for (j = 0; j < L; j++) {              /* L=3: 3 coefficients → 3 bytes */
            temp = (uint32_t)(ETA + s1->vec[j].coeffs[k]);
            s1_table[k+N] = (s1_table[k+N] << 8) | temp;
        }
        s1_table[k] = MASKS_S - s1_table[k+N]; /* complement for negacyclic wrap */
    }
}

/* ------------------------------------------------------------------ */
/*  s2 table                                                           */
/* ------------------------------------------------------------------ */

void prepare_s2_table_ops128(uint32_t s2_table[2*N], polyveck *s2)
{
    uint32_t k, j;
    uint32_t temp;

    for (k = 0; k < N; k++) {
        s2_table[k+N] = 0;
        for (j = 0; j < K; j++) {              /* K=3 */
            temp = (uint32_t)(ETA + s2->vec[j].coeffs[k]);
            s2_table[k+N] = (s2_table[k+N] << 8) | temp;
        }
        s2_table[k] = MASKS_S - s2_table[k+N];
    }
}

/* ------------------------------------------------------------------ */
/*  Fused cs1 + cs2 with early rejection                              */
/* ------------------------------------------------------------------ */

int evaluate_cs1_cs2_early_check_ops128(
        polyvecl *z,       polyveck *w0prime,
        const poly *c,
        const uint32_t s1_table[2*N],
        const uint32_t s2_table[2*N],
        polyvecl *y,       polyveck *w0,
        int32_t A,         int32_t B)
{
    uint32_t i, j;
    uint32_t answer[N]  = {0};
    uint32_t answer2[N] = {0};
    uint32_t temp, temp2;

    /* Accumulate over the TAU nonzero positions of c.
     * table[j-i+N] automatically selects positive or complement entry
     * depending on whether the cyclic index wraps (j < i). */
    for (i = 0; i < N; i++) {
        if (c->coeffs[i] == 1) {
            for (j = 0; j < N; j++) {
                answer[j]  += s1_table[j - i + N];
                answer2[j] += s2_table[j - i + N];
            }
        } else if (c->coeffs[i] == -1) {
            for (j = 0; j < N; j++) {
                answer[j]  += MASKS_S - s1_table[j - i + N];
                answer2[j] += MASKS_S - s2_table[j - i + N];
            }
        }
    }

    /* Unpack 3 coefficients per word (one byte per slot, lowest byte first).
     * Subtract TAU*ETA to remove the accumulated shift, then apply y / w0.
     * Early-check each coefficient immediately after recovery. */
    for (i = 0; i < N; i++) {
        temp  = answer[i];
        temp2 = answer2[i];

        for (j = 0; j < L; j++) {    /* L=K=3: indices 2,1,0 in vec order */
            /* z = y + cs1 */
            z->vec[L-1-j].coeffs[i] = (int32_t)(temp  & 0xFF) - TAU*ETA;
            z->vec[L-1-j].coeffs[i] += y->vec[L-1-j].coeffs[i];
            temp  >>= 8;

            /* w0prime = w0 - cs2 */
            w0prime->vec[K-1-j].coeffs[i] = (int32_t)(temp2 & 0xFF) - TAU*ETA;
            w0prime->vec[K-1-j].coeffs[i] =
                w0->vec[K-1-j].coeffs[i] - w0prime->vec[K-1-j].coeffs[i];
            temp2 >>= 8;

            /* Early rejection: w0prime bound is tighter, check first */
            if (w0prime->vec[K-1-j].coeffs[i] >=  B ||
                w0prime->vec[K-1-j].coeffs[i] <= -B)
                return 1;
            if (z->vec[L-1-j].coeffs[i] >=  A ||
                z->vec[L-1-j].coeffs[i] <= -A)
                return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  t0 table                                                           */
/* ------------------------------------------------------------------ */

void prepare_t0_table_ops128(uint64_t t0_table[2*N], polyveck *t0)
{
    uint32_t k, j;
    uint64_t temp;

    for (k = 0; k < N; k++) {
        t0_table[k+N] = 0;
        for (j = 0; j < K; j++) {              /* K=3: 3 × 19 bits = 57 bits */
            temp = (uint64_t)(0x1000 + t0->vec[j].coeffs[k]);
            t0_table[k+N] = (t0_table[k+N] << 19) | temp;
        }
        /* Bit layout after loop (MSB→LSB):
         *   bits 38-56: t0[0][k]+4096   bits 19-37: t0[1][k]+4096
         *   bits  0-18: t0[2][k]+4096                               */
        t0_table[k] = MASKT_T0 - t0_table[k+N];
    }
}

/* ------------------------------------------------------------------ */
/*  ct0 evaluation                                                     */
/* ------------------------------------------------------------------ */

void evaluate_ct0_ops128(polyveck *z, const poly *c,
                         const uint64_t t0_table[2*N])
{
    uint32_t i, j;
    uint64_t answer[N] = {0};
    uint64_t temp;

    for (i = 0; i < N; i++) {
        if (c->coeffs[i] == 1) {
            for (j = 0; j < N; j++)
                answer[j] += t0_table[j - i + N];
        } else if (c->coeffs[i] == -1) {
            for (j = 0; j < N; j++)
                answer[j] += MASKT_T0 - t0_table[j - i + N];
        }
    }

    /* Unpack 3 coefficients per word (19 bits each, lowest slot first).
     * Subtract T0_UNPACK_SUB = TAU*4096 = 159744 to remove shift. */
    for (i = 0; i < N; i++) {
        temp = answer[i];
        for (j = 0; j < K; j++) {    /* K=3: indices 2,1,0 in vec order */
            z->vec[K-1-j].coeffs[i] = (int32_t)(temp & 0x7FFFF) - T0_UNPACK_SUB;
            temp >>= 19;
        }
    }
}
