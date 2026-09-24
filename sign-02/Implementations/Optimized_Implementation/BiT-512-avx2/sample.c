#define BYTELEN (4 * BIT_N)
#define BYTELEN_16BIT (16 * BIT_N / 4)
/* 15-bit triangular reads 4 bytes/2-coeffs from BOTH a lo and a hi half →
 * needs 4*BIT_N bytes (hi = buf + BYTELEN_15BIT/2).  The old (2*BIT_N) value
 * made the hi-half read run off the end of the buffer past coeff N/2. */
#define BYTELEN_15BIT (4 * BIT_N)
/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#include "consts.h"
#include "sample.h"
#include "symmetric.h"
#include "endian.h"
#include "align.h"
#include "params.h"
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

void poly_sample_triangular_15bit(int32_t *out, const uint8_t *seed, uint16_t *nonce);

#if BIT_USE_SHAKE
#define REJECT_SAMPLE_SQUEEZE_BYTES BIT_XOF256_RATE
#else
#define REJECT_SAMPLE_SQUEEZE_BYTES 136
#endif

/* ── Core unpack — pure buffer→coefficients, no XOF, no nonce ─────── */

/* 14-bit triangular: buf = [lo_half(2048B) ‖ hi_half(2048B)].
   AVX2: broadcast 16 bytes, vpshufb → 8 overlapping 4-byte windows,
   vpsrlvd {0,5,0,5,...}, mask 13 bits; out = lo − hi (int32).
   val[7] window = [13,14,15,15]: 4th byte contributes only to bits 19+
   after >>5, which are annihilated by the &0x1FFF mask — correct.
   Stride = 16 B/iter; 128 iters × 16 B = 2048 B per half.  No overread. */static void poly_unpack_triangular_14bit(int32_t *RESTRICT out, const uint8_t *buf)
{
    const __m256i sh = _mm256_load_si256((const __m256i *)unpack14_shuf);
    const __m256i vs = _mm256_setr_epi32(0,5,0,5, 0,5,0,5);
    const __m256i vm = _mm256_set1_epi32(0x1FFF);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(buf + 16 * i));
        __m128i hi = _mm_loadu_si128((const __m128i *)(buf + BYTELEN / 2 + 16 * i));
        __m256i lv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(lo), sh), vs), vm);
        __m256i hv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(hi), sh), vs), vm);
        _mm256_storeu_si256((__m256i *)(out + 8 * i), _mm256_sub_epi32(lv, hv));
    }
}

/* 15-bit triangular: buf = [lo_half(2048B) ‖ hi_half(2048B)].
   Each coefficient = LE uint16 masked to 15 bits.
   AVX2: load 16 bytes (8 × uint16), mask 0x7FFF, sub int16, sign-extend int32.
   Stride = 16 B/iter; 128 iters.  No overread. */
static void poly_unpack_triangular_15bit(int32_t *RESTRICT out, const uint8_t *buf)
{
    const __m128i mask15 = _mm_set1_epi16((short)0x7FFF);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(buf + 16 * i));
        __m128i hi = _mm_loadu_si128((const __m128i *)(buf + BYTELEN_15BIT / 2 + 16 * i));
        __m128i diff = _mm_sub_epi16(_mm_and_si128(lo, mask15),
                                     _mm_and_si128(hi, mask15));
        _mm256_storeu_si256((__m256i *)(out + 8 * i), _mm256_cvtepi16_epi32(diff));
    }
}

/* 4-bit eta0: buf = [lo_half(512B) ‖ hi_half(512B)].
   2 bytes from lo, 2 from hi → 4 coefficients. */
static void poly_unpack_triangular_4bit(int32_t *RESTRICT out, const uint8_t *buf)
{
    for (int i = 0; i < BIT_N / 4; i++) {
        const uint8_t *lo = buf + 2 * i;
        const uint8_t *hi = buf + BIT_N / 2 + 2 * i;

        uint8_t l0 = lo[0], l1 = lo[1];
        uint8_t h0 = hi[0], h1 = hi[1];

        out[4*i + 0] = (int32_t)(l0 & 0x0F)        - (int32_t)(h0 & 0x0F);
        out[4*i + 1] = (int32_t)((l0 >> 4) & 0x0F) - (int32_t)((h0 >> 4) & 0x0F);
        out[4*i + 2] = (int32_t)(l1 & 0x0F)        - (int32_t)(h1 & 0x0F);
        out[4*i + 3] = (int32_t)((l1 >> 4) & 0x0F) - (int32_t)((h1 >> 4) & 0x0F);
    }
}

/* ── scalar drivers (one-shot XOF) ─────────────────────────────────── */

void poly_sample_triangular(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN + 16];   /* +16: conservative overread guard */
    bit_xof256_nonce(buf, BYTELEN, seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_14bit(out, buf);
}

void poly_sample_triangular_eta0(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BIT_N];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_4bit(out, buf);
}

/* ── batched y sampling (x4 / x2 XOF, like avx2-128) ────────────────── */

#define Y_BUF_SZ BYTELEN   /* 4096 — max of eta0(1024) and triangular(4096) */

void polyvecy_sample_y0y1_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    /* +16: conservative overread guard consistent with avx2-256 style */
    uint8_t ALIGNED_32 buf0[Y_BUF_SZ + 16], buf1[Y_BUF_SZ + 16];
    uint8_t ALIGNED_32 buf2[Y_BUF_SZ + 16], buf3[Y_BUF_SZ + 16];

    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U), n3 = (uint16_t)(*nonce + 3U);
    *nonce = (uint16_t)(*nonce + 4U);

    bit_xof256x4(buf0, buf1, buf2, buf3, Y_BUF_SZ, seed_y, BIT_SEEDBYTES, n0, n1, n2, n3);

    poly_unpack_triangular_4bit( y->vec[0].coeffs, buf0);
    poly_unpack_triangular_14bit(y->vec[1].coeffs, buf1);
    poly_unpack_triangular_14bit(y->vec[2].coeffs, buf2);
    poly_unpack_triangular_14bit(y->vec[3].coeffs, buf3);
}

void polyvecy_sample_y2_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    /* K=3 y2 polys batched through one x4 XOF call (lane 3 = dummy nonce n2).
       Same (seed,nonce) → same XOF bytes as the scalar path; KAT preserved. */
    uint8_t ALIGNED_32 b0[BYTELEN_15BIT + 16], b1[BYTELEN_15BIT + 16];
    uint8_t ALIGNED_32 b2[BYTELEN_15BIT + 16], bd[BYTELEN_15BIT + 16];

    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U), n2 = (uint16_t)(*nonce + 2U);
    *nonce = (uint16_t)(*nonce + BIT_K);

    bit_xof256x4(b0, b1, b2, bd, BYTELEN_15BIT, seed_y, BIT_SEEDBYTES, n0, n1, n2, n2);

    poly_unpack_triangular_15bit(y->vec[4].coeffs, b0);
    poly_unpack_triangular_15bit(y->vec[5].coeffs, b1);
    poly_unpack_triangular_15bit(y->vec[6].coeffs, b2);
}

#define TWO_POW_24 16777216ULL
#define REJECT_POOL_BYTES 136

_Static_assert(BIT_GAMMA1 == 8192 && BIT_BETA == 115,
               "fast path in reject_sample uses constants derived for gamma1=8192,beta2=115");

/* ── 8-way AVX2 fast-accept + CTZ sparse fallback ────────────────────
   Each int32 lane maps to 4 bytes in movemask; mask 0x11111111
   keeps one bit per lane.  ctz>>2 gives 0-based lane index. ──────── */

static int reject_sample_coeffs_pool(const poly *z,
                                     const int32_t *v,
                                     int32_t gamma1,
                                     int32_t beta2,
                                     const uint8_t *r_buf,
                                     const unsigned char *seed,
                                     uint16_t nonce) {
    const int32_t fast_lo = beta2;
    const int32_t fast_hi = gamma1 - beta2;
    const int32_t G_fast = 2 * (gamma1 - beta2);
    ALIGNED_32 uint8_t local_pool[REJECT_POOL_BYTES];
#if BIT_USE_SHAKE
    const uint8_t *cur = r_buf;
    size_t pos = 0;
#else
    /* SM3: the prefetched r_buf is bit_xof256_nonce(seed||nonce) (one-shot
       pseudoXOF), which does NOT equal the stateful stream's block 0 (input
       seed||nonce||block).  Drive the block-indexed stream from block 0,
       byte-for-byte like the reference. */
    const uint8_t *cur = local_pool;
    size_t pos = REJECT_POOL_BYTES;   /* force stateful refill before first byte */
    (void)r_buf;
#endif
    bit_xof256_state st;
    int st_inited = 0;
    uint32_t reject_flag = 0;

    const __m256i v_lo_m1 = _mm256_set1_epi32(fast_lo - 1);
    const __m256i v_hi    = _mm256_set1_epi32(fast_hi);

    for (int j = 0; j < BIT_N; j += 8) {
        __m256i zj = _mm256_load_si256((const __m256i *)&z->coeffs[j]);
        __m256i val_vec = _mm256_abs_epi32(zj);

        __m256i m_lo = _mm256_cmpgt_epi32(val_vec, v_lo_m1);      /* val > beta2-1 ≡ val >= beta2 */
        __m256i m_hi = _mm256_cmpgt_epi32(v_hi, val_vec);          /* val < gamma1-beta2 */
        __m256i fast_all = _mm256_and_si256(m_lo, m_hi);

        uint32_t mask = _mm256_movemask_epi8(fast_all);
        if (mask == 0xFFFFFFFFu) continue;

        uint32_t fail_bits = (~mask) & 0x11111111u;
        while (fail_bits) {
            int k = __builtin_ctz(fail_bits) >> 2;
            fail_bits &= fail_bits - 1;

            int idx = j + k;
            int32_t val = ct_abs(z->coeffs[idx]);
            int32_t beta1_i = ct_abs(v[idx]);

            if ((uint32_t)(val >= gamma1)) {
                reject_flag = 1;
                continue;
            }

            if (pos + 3 > REJECT_POOL_BYTES) {
                if (!st_inited) {
                    bit_xof256_init(&st, seed, BIT_SEEDBYTES, nonce);
#if BIT_USE_SHAKE
                    /* SHAKE block 0 duplicates the prefetched r_buf — skip it. */
                    bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);
#endif
                    st_inited = 1;
                }
                bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);
                cur = local_pool;
                pos = 0;
            }

            uint32_t R = load24_le(cur + pos);
            pos += 3;

            if (val < beta2) {
                int32_t F_int = 2 * gamma1 - 2 * ct_max(val, beta1_i);
                if ((uint64_t)R * F_int > (uint64_t)G_fast * TWO_POW_24)
                    reject_flag = 1;
            } else {
                int32_t f_minus = ct_max(gamma1 - ct_abs(val - beta1_i), 0);
                int32_t f_plus  = ct_max(gamma1 - (val + beta1_i), 0);
                int32_t F_int = f_minus + f_plus;
                int32_t G_int = 2 * ct_max(gamma1 - ct_max(val, beta2), 0);
                if ((uint64_t)R * F_int > (uint64_t)G_int * TWO_POW_24)
                    reject_flag = 1;
            }
        }
    }
    if (st_inited) bit_xof256_zeroize(&st);
    return (int)reject_flag;
}

static int reject_sample_coeffs_sparse_pool(const poly *z,
                                            const sparse_challenge *support,
                                            const int32_t *v,
                                            int32_t gamma1,
                                            int32_t beta2,
                                            const uint8_t *r_buf,
                                            const unsigned char *seed,
                                            uint16_t nonce) {
    const int32_t fast_lo = beta2;
    const int32_t fast_hi = gamma1 - beta2;
    const int32_t G_fast = 2 * (gamma1 - beta2);
    ALIGNED_32 uint8_t local_pool[REJECT_POOL_BYTES];
#if BIT_USE_SHAKE
    const uint8_t *cur = r_buf;
    size_t pos = 0;
#else
    /* SM3: the prefetched r_buf is bit_xof256_nonce(seed||nonce) (one-shot
       pseudoXOF), which does NOT equal the stateful stream's block 0 (input
       seed||nonce||block).  Drive the block-indexed stream from block 0,
       byte-for-byte like the reference. */
    const uint8_t *cur = local_pool;
    size_t pos = REJECT_POOL_BYTES;   /* force stateful refill before first byte */
    (void)r_buf;
#endif
    bit_xof256_state st;
    int st_inited = 0;
    uint32_t reject_flag = 0;

    for (int t = 0; t < BIT_TAU; t++) {
        int j = support->pos[t];
        int32_t val = ct_abs(z->coeffs[j]);
        int32_t beta1_i = ct_abs(v[j]);

        if (val >= fast_lo && val < fast_hi) continue;

        if ((uint32_t)(val >= gamma1)) {
            reject_flag = 1;
            continue;
        }

        if (pos + 3 > REJECT_POOL_BYTES) {
            if (!st_inited) {
                bit_xof256_init(&st, seed, BIT_SEEDBYTES, nonce);
#if BIT_USE_SHAKE
                bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);  /* discard duplicate */
#endif
                st_inited = 1;
            }
            bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);
            cur = local_pool;
            pos = 0;
        }

        uint32_t R = load24_le(cur + pos);
        pos += 3;

        if (val < beta2) {
            int32_t F_int = 2 * gamma1 - 2 * ct_max(val, beta1_i);
            if ((uint64_t)R * F_int > (uint64_t)G_fast * TWO_POW_24)
                reject_flag = 1;
        } else {
            int32_t f_minus = ct_max(gamma1 - ct_abs(val - beta1_i), 0);
            int32_t f_plus  = ct_max(gamma1 - (val + beta1_i), 0);
            int32_t F_int = f_minus + f_plus;
            int32_t G_int = 2 * ct_max(gamma1 - ct_max(val, beta2), 0);
            if ((uint64_t)R * F_int > (uint64_t)G_int * TWO_POW_24)
                reject_flag = 1;
        }
    }
    if (st_inited) bit_xof256_zeroize(&st);
    return (int)reject_flag;
}

/* ── x4 pre-fetch z0+z1 ────────────────────────────────────────────── */

int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s,
                              const poly *c_poly, const sparse_challenge *c_sparse,
                              const unsigned char *seed, uint16_t *nonce) {
    ALIGNED_32 uint8_t r0[REJECT_POOL_BYTES], r1[REJECT_POOL_BYTES];
    ALIGNED_32 uint8_t r2[REJECT_POOL_BYTES], r3[REJECT_POOL_BYTES];

    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U), n3 = (uint16_t)(*nonce + 3U);
    *nonce = (uint16_t)(*nonce + 4U);

#if BIT_USE_SHAKE
    bit_xof256x4(r0, r1, r2, r3, REJECT_POOL_BYTES, seed, BIT_SEEDBYTES, n0, n1, n2, n3);
#endif
    /* SM3 ignores the prefetch pools and drives the stateful stream directly. */

    /* Fallback refill streams initialised lazily on pool exhaustion only. */
    int rej = 0;
    if (reject_sample_coeffs_pool(&z->vec[1], c_s->vec[0].coeffs, BIT_GAMMA1, BIT_BETA, r1, seed, n1)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[2], c_s->vec[1].coeffs, BIT_GAMMA1, BIT_BETA, r2, seed, n2)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[3], c_s->vec[2].coeffs, BIT_GAMMA1, BIT_BETA, r3, seed, n3)) rej = 1;
    if (!rej && reject_sample_coeffs_sparse_pool(&z->vec[0], c_sparse, c_poly->coeffs, BIT_GAMMA1_0, 1, r0, seed, n0)) rej = 1;
    return rej;
}

/* ── x2 pre-fetch z2 ───────────────────────────────────────────────── */

int check_reject_sample_z2(const polyvecy *z, const polyveck *c_e,
                            const unsigned char *seed, uint16_t *nonce) {
    ALIGNED_32 uint8_t r4[REJECT_POOL_BYTES], r5[REJECT_POOL_BYTES], r6[REJECT_POOL_BYTES], dummy[REJECT_POOL_BYTES];

    uint16_t n4 = *nonce, n5 = (uint16_t)(*nonce + 1U), n6 = (uint16_t)(*nonce + 2U);
    *nonce = (uint16_t)(*nonce + 3U);

#if BIT_USE_SHAKE
    bit_xof256x4(r4, r5, r6, dummy, REJECT_POOL_BYTES,
                 seed, BIT_SEEDBYTES, n4, n5, n6, (uint16_t)0xFFFEU);
#else
    (void)dummy;  /* SM3 drives the stateful stream directly; no prefetch. */
#endif

    /* Fallback refill streams initialised lazily on pool exhaustion only. */
    int rej = 0;
    if (reject_sample_coeffs_pool(&z->vec[4], c_e->vec[0].coeffs, BIT_GAMMA1_2, BIT_BETA, r4, seed, n4)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[5], c_e->vec[1].coeffs, BIT_GAMMA1_2, BIT_BETA, r5, seed, n5)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[6], c_e->vec[2].coeffs, BIT_GAMMA1_2, BIT_BETA, r6, seed, n6)) rej = 1;
    return rej;
}


int check_reject_norm(const polyvecm1 *z1, const polyveck *h) {
    uint32_t reject_flag = 0;

    for (int i = 0; i < BIT_L + 1; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t val = (int32_t)z1->vec[i].coeffs[j];

            int32_t abs_val = ct_abs(val);

            uint32_t over_bound = (uint32_t)(BIT_B_INF - abs_val) >> 31;
            reject_flag |= over_bound;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t h_val = (int32_t)h->vec[i].coeffs[j];
            int32_t abs_h = ct_abs(h_val);

            int32_t gamma2_h = abs_h * BIT_GAMMA2;
            uint32_t over_bound = (uint32_t)(BIT_B_INF - gamma2_h) >> 31;
            reject_flag |= over_bound;
        }
    }

    return reject_flag != 0 ? 1 : 0;
}

int check_reject_hint_range(const polyveck *h) {
    /* base3 only: reject if |h| > 1 */
    uint32_t reject_flag = 0;
    for (int i = 0; i < BIT_K; i++)
        for (int j = 0; j < BIT_N; j++)
            reject_flag |= (uint32_t)(1 - ct_abs(h->vec[i].coeffs[j])) >> 31;
    return (int)reject_flag;
}

void poly_sample_triangular_15bit(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN_15BIT];   /* exact fit; no overread for 15-bit AVX2 */
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_15bit(out, buf);
}
