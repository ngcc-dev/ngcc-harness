/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
/* Shim: route the canonical NTT entry points to the hand-written asm,
   so test_speed / test_correctness exercise the assembly implementation. */
#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "align.h"

/* Canonical NTT names are provided as zero-overhead aliases in alias.S.
   montgomery_lift kept as intrinsics (not part of the asm rewrite scope). */
#define MONT_R2 1759
#define INVERSE_Q -26879
#define V_Q _mm256_set1_epi16(BIT_Q)
#define V_R2 _mm256_set1_epi16(MONT_R2)
#define V_R2_QINV _mm256_set1_epi16((int16_t)(MONT_R2 * INVERSE_Q))
#define MONT_FIX(V) V = _mm256_add_epi16(V, _mm256_and_si256(_mm256_srai_epi16(V, 15), V_Q))
void ntt_montgomery_lift(int16_t *r) {
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i x = _mm256_load_si256((__m256i *)&r[i]);
        __m256i l = _mm256_mullo_epi16(x, V_R2_QINV);
        __m256i h = _mm256_mulhi_epi16(x, V_R2);
        __m256i t = _mm256_mulhi_epi16(l, V_Q);
        x = _mm256_sub_epi16(h, t);
        MONT_FIX(x);
        _mm256_store_si256((__m256i *)&r[i], x);
    }
}
