#include <stdint.h>
#include <string.h>
#include "params.h"
#include "mult.h"
#include "polyvec.h"
#include "poly.h"

/* ------------------------------------------------------------------ */
/* s1 table (L=4 polys, 16-bit slots, N=1024)                         */
/* ------------------------------------------------------------------ */

void prepare_s1_table_ops512(uint64_t s1_table[2*N], polyvecl *s1)
{
    int k, j;
    for (k = 0; k < N; k++) {
        s1_table[k+N] = 0;
        for (j = 0; j < L; j++) {
            uint64_t temp = (uint64_t)(ETA + s1->vec[j].coeffs[k]);
            s1_table[k+N] = (s1_table[k+N] << 16) | temp;
        }
        s1_table[k] = MASKS_S_OPS512 - s1_table[k+N];
    }
}

/* ------------------------------------------------------------------ */
/* s2 table (K=4 polys, 16-bit slots)                                 */
/* ------------------------------------------------------------------ */

void prepare_s2_table_ops512(uint64_t s2_table[2*N], polyveck *s2)
{
    int k, j;
    for (k = 0; k < N; k++) {
        s2_table[k+N] = 0;
        for (j = 0; j < K; j++) {
            uint64_t temp = (uint64_t)(ETA + s2->vec[j].coeffs[k]);
            s2_table[k+N] = (s2_table[k+N] << 16) | temp;
        }
        s2_table[k] = MASKS_S_OPS512 - s2_table[k+N];
    }
}

/* ------------------------------------------------------------------ */
/* Fused cs1+cs2 with early rejection (N=1024, 16-bit slots)          */
/* ------------------------------------------------------------------ */

int evaluate_cs1_cs2_early_check_ops512(
        polyvecl *z, polyveck *w0prime, const poly *c,
        const uint64_t s1_table[2*N], const uint64_t s2_table[2*N],
        polyvecl *y, polyveck *w0,
        int32_t A, int32_t B)
{
    int i, j;
    uint64_t answer[N];
    uint64_t answer2[N];
    memset(answer,  0, sizeof(answer));
    memset(answer2, 0, sizeof(answer2));

    for (i = 0; i < N; i++) {
        if (c->coeffs[i] == 1) {
            for (j = 0; j < N; j++) {
                answer[j]  += s1_table[j - i + N];
                answer2[j] += s2_table[j - i + N];
            }
        } else if (c->coeffs[i] == -1) {
            for (j = 0; j < N; j++) {
                answer[j]  += MASKS_S_OPS512 - s1_table[j - i + N];
                answer2[j] += MASKS_S_OPS512 - s2_table[j - i + N];
            }
        }
    }

    for (i = 0; i < N; i++) {
        uint64_t ts  = answer[i];
        uint64_t ts2 = answer2[i];
        for (j = 0; j < L; j++) {
            z->vec[L-1-j].coeffs[i]  = (int32_t)(ts  & 0xFFFF) - S_UNPACK_SUB_OPS512;
            z->vec[L-1-j].coeffs[i] += y->vec[L-1-j].coeffs[i];
            ts >>= 16;

            w0prime->vec[K-1-j].coeffs[i]  = (int32_t)(ts2 & 0xFFFF) - S_UNPACK_SUB_OPS512;
            w0prime->vec[K-1-j].coeffs[i]  = w0->vec[K-1-j].coeffs[i]
                                             - w0prime->vec[K-1-j].coeffs[i];
            ts2 >>= 16;

            if (w0prime->vec[K-1-j].coeffs[i] >=  B) return 1;
            if (w0prime->vec[K-1-j].coeffs[i] <= -B) return 1;
            if (z->vec[L-1-j].coeffs[i] >=  A) return 1;
            if (z->vec[L-1-j].coeffs[i] <= -A) return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* t0 tables (K=4 polys, 20-bit slots, two tables)                    */
/* ------------------------------------------------------------------ */

void prepare_t0_table_ops512(uint64_t t0_table_lo[2*N], uint64_t t0_table_hi[2*N],
                              polyveck *t0)
{
    int k;
    for (k = 0; k < N; k++) {
        /* lo: vec[0] at bits 20-39, vec[1] at bits 0-19 */
        uint64_t p0 = (uint64_t)(4096 + t0->vec[0].coeffs[k]);
        uint64_t p1 = (uint64_t)(4096 + t0->vec[1].coeffs[k]);
        t0_table_lo[k+N] = (p0 << 20) | p1;
        t0_table_lo[k]   = MASKT_T0_OPS512 - t0_table_lo[k+N];

        /* hi: vec[2] at bits 20-39, vec[3] at bits 0-19 */
        uint64_t p2 = (uint64_t)(4096 + t0->vec[2].coeffs[k]);
        uint64_t p3 = (uint64_t)(4096 + t0->vec[3].coeffs[k]);
        t0_table_hi[k+N] = (p2 << 20) | p3;
        t0_table_hi[k]   = MASKT_T0_OPS512 - t0_table_hi[k+N];
    }
}

/* ------------------------------------------------------------------ */
/* ct0 evaluation: 20-bit unpack of both tables                       */
/* ------------------------------------------------------------------ */

void evaluate_ct0_ops512(polyveck *z, const poly *c,
                         const uint64_t t0_table_lo[2*N],
                         const uint64_t t0_table_hi[2*N])
{
    int i, j;
    uint64_t ans_lo[N];
    uint64_t ans_hi[N];
    memset(ans_lo, 0, sizeof(ans_lo));
    memset(ans_hi, 0, sizeof(ans_hi));

    for (i = 0; i < N; i++) {
        if (c->coeffs[i] == 1) {
            for (j = 0; j < N; j++) {
                ans_lo[j] += t0_table_lo[j - i + N];
                ans_hi[j] += t0_table_hi[j - i + N];
            }
        } else if (c->coeffs[i] == -1) {
            for (j = 0; j < N; j++) {
                ans_lo[j] += MASKT_T0_OPS512 - t0_table_lo[j - i + N];
                ans_hi[j] += MASKT_T0_OPS512 - t0_table_hi[j - i + N];
            }
        }
    }

    /* lo bits 20-39 → vec[0], lo bits 0-19 → vec[1]
     * hi bits 20-39 → vec[2], hi bits 0-19 → vec[3] */
    for (i = 0; i < N; i++) {
        z->vec[1].coeffs[i] = (int32_t)( ans_lo[i]        & 0xFFFFF) - T0_UNPACK_SUB_OPS512;
        z->vec[0].coeffs[i] = (int32_t)((ans_lo[i] >> 20) & 0xFFFFF) - T0_UNPACK_SUB_OPS512;
        z->vec[3].coeffs[i] = (int32_t)( ans_hi[i]        & 0xFFFFF) - T0_UNPACK_SUB_OPS512;
        z->vec[2].coeffs[i] = (int32_t)((ans_hi[i] >> 20) & 0xFFFFF) - T0_UNPACK_SUB_OPS512;
    }
}
