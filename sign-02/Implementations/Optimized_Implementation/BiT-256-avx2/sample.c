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

#if BIT_USE_SHAKE
#define REJECT_SAMPLE_SQUEEZE_BYTES BIT_XOF256_RATE
#else
#define REJECT_SAMPLE_SQUEEZE_BYTES 136
#endif

/* ── parameter-derived constants ───────────────────────────────────── */

/* Triangular: GAMMA1=4096 → 12-bit per coeff, 2 coeffs = 24b = 3 bytes.
   Each half: BIT_N × 12/8 = 512 × 1.5 = 768 bytes.  BYTELEN = 1536.
   BYTELEN is defined in sample.h; do not redefine it here.             */

/* eta0: GAMMA1_0=16 → 4-bit per coeff, lo+hi each 4 bits.
   Each half: BIT_N/2 bytes.  Total = BIT_N bytes. */
#define ETA0_BYTES (BIT_N)        /* = 512 (was 384 for 3-bit) */

/* Y buffer: max of 12-bit (1536) and 4-bit (512) */
#define Y_BUF_SZ BYTELEN           /* = 1536 >= 512 */

/* 13-bit triangular (y2, GAMMA1_2=8192): 2×13×N/8 bytes per poly */
#define BYTELEN_13BIT (2 * 13 * BIT_N / 8)   /* = 1664 */
static void poly_unpack_triangular_13bit(int32_t *out, const uint8_t *buf);

/* ── Core unpack — pure buffer→coefficients, no XOF, no nonce ─────── */

/* 12-bit triangular: buf = [lo_half(768B) ‖ hi_half(768B)].
   AVX2: 8 coeffs/iter.  Each 16-byte broadcast load feeds vpshufb to 8 overlapping
   uint32 windows (bytes {0,1,3,4,6,7,9,10}), vpsrlvd by {0,4,...}, mask 12 bits;
   out = lo − hi (int32).  Bit-identical to the scalar path.  The 16-byte loads
   over-read up to 4 bytes past the half — callers pad the XOF buffer (see
   SAMPLING_*_PAD) so the over-read stays in-bounds; surplus bits are masked off. */static void poly_unpack_triangular_12bit(int32_t *RESTRICT out, const uint8_t *buf)
{
    const __m256i sh = _mm256_load_si256((const __m256i *)unpack12_shuf);
    const __m256i vs = _mm256_setr_epi32(0,4,0,4,0,4,0,4);
    const __m256i vm = _mm256_set1_epi32(0xFFF);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(buf + 12 * i));
        __m128i hi = _mm_loadu_si128((const __m128i *)(buf + BYTELEN / 2 + 12 * i));
        __m256i lv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(lo), sh), vs), vm);
        __m256i hv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(hi), sh), vs), vm);
        _mm256_storeu_si256((__m256i *)(out + 8 * i), _mm256_sub_epi32(lv, hv));
    }
}

/* 4-bit triangular (gamma_{1,0}=16): 4 bits from lo, 4 from hi → 4 coeffs.
   buf = [lo_half(N/2 B) | hi_half(N/2 B)], total  N = 512 B. */
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

/* ── scalar drivers (one-shot XOF, kept for non-batch paths) ───────── */

void poly_sample_triangular(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN + 16];   /* +16 pad for AVX2 12-bit unpacker over-read */
    bit_xof256_nonce(buf, BYTELEN, seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_12bit(out, buf);
}

void poly_sample_triangular_eta0(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[ETA0_BYTES];      /* = BIT_N bytes for 4-bit */
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_4bit(out, buf);
}

/* ── batched y sampling (x4 / x2 XOF, like avx2-128) ────────────────── */

void polyvecy_sample_y0y1_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    /* +16 pad: the AVX2 12-bit unpacker uses 16-byte loads that over-read up to
       ~4 bytes past the hi half; the squeeze still fills only Y_BUF_SZ. */
    uint8_t ALIGNED_32 buf0[Y_BUF_SZ + 16], buf1[Y_BUF_SZ + 16];
    uint8_t ALIGNED_32 buf2[Y_BUF_SZ + 16], buf3[Y_BUF_SZ + 16];

    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U), n3 = (uint16_t)(*nonce + 3U);
    *nonce = (uint16_t)(*nonce + 4U);

    bit_xof256x4(buf0, buf1, buf2, buf3, Y_BUF_SZ, seed_y, BIT_SEEDBYTES, n0, n1, n2, n3);

    poly_unpack_triangular_4bit( y->vec[0].coeffs, buf0);
    poly_unpack_triangular_12bit(y->vec[1].coeffs, buf1);
    poly_unpack_triangular_12bit(y->vec[2].coeffs, buf2);
    poly_unpack_triangular_12bit(y->vec[3].coeffs, buf3);
}

void polyvecy_sample_y2_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    /* K=3 y2 polys batched through one x4 Keccak (lane 3 = dummy).
       Identical (seed,nonce) → identical XOF bytes as the scalar path,
       so KAT is preserved. */
    /* +16 pad for the AVX2 13-bit unpacker's 16-byte over-reading loads. */
    uint8_t ALIGNED_32 b0[BYTELEN_13BIT + 16], b1[BYTELEN_13BIT + 16];
    uint8_t ALIGNED_32 b2[BYTELEN_13BIT + 16], bd[BYTELEN_13BIT + 16];

    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U), n2 = (uint16_t)(*nonce + 2U);
    *nonce = (uint16_t)(*nonce + BIT_K);

    bit_xof256x4(b0, b1, b2, bd, BYTELEN_13BIT, seed_y, BIT_SEEDBYTES, n0, n1, n2, n2);

    poly_unpack_triangular_13bit(y->vec[4].coeffs, b0);
    poly_unpack_triangular_13bit(y->vec[5].coeffs, b1);
    poly_unpack_triangular_13bit(y->vec[6].coeffs, b2);
}

/* AVX2 batched triangular: replaces polyvecy_sample_triangular in sign.c */
void polyvecy_sample_triangular_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    polyvecy_sample_y0y1_avx(y, seed_y, nonce);
    polyvecy_sample_y2_avx(y, seed_y, nonce);
}

#define TWO_POW_24 16777216ULL
/* 13-bit triangular sampling (for GAMMA1_2=8192) */
void poly_sample_triangular_13bit(int32_t *out, const uint8_t *seed, uint16_t *nonce);

#define REJECT_POOL_BYTES REJECT_SAMPLE_SQUEEZE_BYTES

_Static_assert(BIT_GAMMA1 == 4096 && BIT_BETA == 58,
               "fast path uses constants derived for gamma1=4096,beta=58");

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
       pseudoXOF), which does NOT equal the reference stateful stream's block 0
       (input seed||nonce||block).  Ignore the prefetch and drive the stateful
       block-indexed stream from block 0, byte-for-byte like the reference. */
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
    /* SM3: see reject_sample_coeffs_pool — drive the stateful block-indexed
       stream from block 0 instead of the one-shot prefetch. */
    const uint8_t *cur = local_pool;
    size_t pos = REJECT_POOL_BYTES;
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

/* ── x4 pre-fetch z0+z1 (replaces scalar check_reject_sample_z0z1) ─── */

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

/* ── x2 pre-fetch z2 (replaces scalar check_reject_sample_z2) ──────── */

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

#ifdef BIT_HINT_BITPACK
    /* Bit-pack: h is bounded by BIT_H_INF directly (no overflow counting). */
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(BIT_H_INF - abs_h) >> 31;
        }
    }
#else
    uint32_t h_overflow_count = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t h_val = (int32_t)h->vec[i].coeffs[j];
            int32_t abs_h = ct_abs(h_val);
            int32_t gamma2_h = abs_h * BIT_GAMMA2;
            uint32_t over_bound = (uint32_t)(BIT_B_INF - gamma2_h) >> 31;
            reject_flag |= over_bound;
            h_overflow_count += (uint32_t)(abs_h >> 1);
        }
    }
    reject_flag |= (uint32_t)(BIT_H_OVERFLOW_MAX - h_overflow_count) >> 31;
#endif

    return reject_flag != 0 ? 1 : 0;
}

int check_reject_hint_range(const polyveck *h) {
    uint32_t reject_flag = 0;
#ifdef BIT_HINT_BITPACK
    /* Fixed-width bit packing: |h| <= BIT_H_INF, no count limit. */
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(BIT_H_INF - abs_h) >> 31;
        }
    }
#else
    uint32_t overflow_count = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(2 - abs_h) >> 31;
            overflow_count += (uint32_t)(abs_h >> 1);
        }
    }
    reject_flag |= (uint32_t)(BIT_H_OVERFLOW_MAX - overflow_count) >> 31;
#endif
    return (int)reject_flag;
}

/* 13-bit triangular (y2): the scalar layout's per-coeff uint32 windows sit at
   bytes {0,2,4,5,7,8,9,11} with right-shifts {0,5,2,7,4,1,6,3} (NOT a plain
   contiguous 13-bit pack — these mirror the scalar a0..a7 exactly).  AVX2 does
   8 coeffs/iter via vpshufb→vpsrlvd→mask, out = lo − hi (int32).  Bit-identical
   to the scalar.  16-byte loads over-read up to 3 bytes; callers pad the buffer. */static void poly_unpack_triangular_13bit(int32_t *out, const uint8_t *buf)
{
    const __m256i sh = _mm256_load_si256((const __m256i *)unpack13_shuf);
    const __m256i vs = _mm256_setr_epi32(0,5,2,7,4,1,6,3);
    /* lane 4 keeps only 12 bits: the scalar a4 = (lo[7]>>4)|(lo[8]<<4) spans just
       two bytes, so its bit 12 is always 0 — the 4-byte window must not leak the
       next byte's bit there.  All other lanes are full 13-bit. */
    const __m256i vm = _mm256_setr_epi32(0x1FFF,0x1FFF,0x1FFF,0x1FFF,
                                         0x0FFF,0x1FFF,0x1FFF,0x1FFF);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(buf + 13 * i));
        __m128i hi = _mm_loadu_si128((const __m128i *)(buf + BYTELEN_13BIT / 2 + 13 * i));
        __m256i lv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(lo), sh), vs), vm);
        __m256i hv = _mm256_and_si256(_mm256_srlv_epi32(
            _mm256_shuffle_epi8(_mm256_broadcastsi128_si256(hi), sh), vs), vm);
        _mm256_storeu_si256((__m256i *)(out + 8 * i), _mm256_sub_epi32(lv, hv));
    }
}

void poly_sample_triangular_13bit(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN_13BIT + 16];   /* +16 pad for AVX2 13-bit unpacker over-read */
    bit_xof256_nonce(buf, BYTELEN_13BIT, seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);
    poly_unpack_triangular_13bit(out, buf);
}
