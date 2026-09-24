/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#include "sample.h"
#include "symmetric.h"
#include "endian.h"
#include "align.h"
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#if BIT_USE_SHAKE
#define REJECT_SAMPLE_SQUEEZE_BYTES BIT_XOF256_RATE
#else
#define REJECT_SAMPLE_SQUEEZE_BYTES 136
#endif

#define BYTELEN (10 * 2 * BIT_N / 8)

// AVX2 10-bit unpack: processes 8 coefficients per iteration using vpshufb + blend.
// Each 5 bytes of lo (and 5 bytes of hi) produce 4 x 10-bit values.
// The 10 lo + 10 hi bytes are loaded simultaneously, shuffled, then shifted by
// {0,2,4,6,0,2,4,6} using blend to select the correct shift per word.
static inline void poly_unpack_triangular_10bit_avx(int16_t *out, const uint8_t *buf)
{
    const uint8_t *lo_ptr = buf;
    const uint8_t *hi_ptr = buf + BYTELEN / 2;

    // Shuffle mask: maps 5-byte groups into 8 x 16-bit words
    // {0,1, 1,2, 2,3, 3,4, 5,6, 6,7, 7,8, 8,9}
    __m128i mask = _mm_setr_epi8(
        0, 1,  1, 2,  2, 3,  3, 4,
        5, 6,  6, 7,  7, 8,  8, 9
    );
    __m256i mask256 = _mm256_broadcastsi128_si256(mask);
    __m256i mask3FF = _mm256_set1_epi16(0x03FF);

    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo_in = _mm_loadu_si128((const __m128i *)(lo_ptr + 10 * i));
        __m128i hi_in = _mm_loadu_si128((const __m128i *)(hi_ptr + 10 * i));

        // hi in high 128-bit lane, lo in low lane — dual-lane shuffle
        __m256i in256  = _mm256_set_m128i(hi_in, lo_in);
        __m256i val256 = _mm256_shuffle_epi8(in256, mask256);

        // Compute all four shift variants
        __m256i s0 = val256;
        __m256i s1 = _mm256_srli_epi16(val256, 2);
        __m256i s2 = _mm256_srli_epi16(val256, 4);
        __m256i s3 = _mm256_srli_epi16(val256, 6);

        // Blend to select shifts: [0, 2, 4, 6, 0, 2, 4, 6]
        __m256i s01 = _mm256_blend_epi16(s0, s1, 0x22);   // bits 1,5 → shift 2
        __m256i s23 = _mm256_blend_epi16(s2, s3, 0x88);   // bits 3,7 → shift 6
        __m256i final256 = _mm256_blend_epi16(s01, s23, 0xCC); // bits 2,3,6,7 → from s23

        final256 = _mm256_and_si256(final256, mask3FF);

        // lo (low lane) - hi (high lane)
        __m128i final_lo = _mm256_castsi256_si128(final256);
        __m128i final_hi = _mm256_extracti128_si256(final256, 1);
        __m128i out_vec  = _mm_sub_epi16(final_lo, final_hi);

        _mm_store_si128((__m128i *)(out + 8 * i), out_vec);
    }
}


#define BYTELEN_11 (11 * 2 * BIT_N / 8)

// AVX2 11-bit unpack: 11 bytes per half → 8 × 11-bit values, out = a - b.
//
// Bit layout (ref, little-endian LSB-first): value v occupies bits
// [11v .. 11v+10], i.e. uint32 window at byte floor(11v/8) shifted right by
// (11v mod 8):   windows {0,1,2,4,5,6,8,9}, shifts {0,3,6,1,4,7,2,5}.
// Values 2 and 5 straddle three bytes, so uint16 windows (as in the 10-bit
// path) lose the top bit(s); we use uint32 windows + vpsrlvd instead.
//
// vpsrlvd gives only 4 variable-shifted lanes per 128-bit half (8 per 256), so
// 8 a-values and 8 b-values cannot share one shuffle (unlike the 10-bit
// uint16-window path).  We therefore broadcast each 11-byte half across both
// lanes and run two independent extract passes (vals 0..3 in the low lane,
// 4..7 in the high lane), then subtract.  No memory round-trip; one store/iter.
static inline void poly_unpack_triangular_11bit_avx(int16_t *RESTRICT out,
                                                     const uint8_t *buf)
{
    const uint8_t *lo_ptr = buf;
    const uint8_t *hi_ptr = buf + BYTELEN_11 / 2;

    /* low lane:  vals 0..3 → uint32 windows @ bytes {0,1,2,4}
       high lane: vals 4..7 → uint32 windows @ bytes {5,6,8,9} */
    const __m256i shuf = _mm256_setr_epi8(
        0,1,2,3,   1,2,3,4,   2,3,4,5,   4,5,6,7,
        5,6,7,8,   6,7,8,9,   8,9,10,11, 9,10,11,12
    );
    const __m256i vshift = _mm256_setr_epi32(0, 3, 6, 1, 4, 7, 2, 5);
    const __m256i vmask  = _mm256_set1_epi32(0x07FF);

    for (int i = 0; i < BIT_N / 8; i++) {
        /* 16-byte loads read up to 5 bytes past the 11 used; the XOF buffer is
           padded (SAMPLING_BUF_LEN_PAD_11) so the over-read stays in bounds and
           the surplus bits are removed by vmask. */
        __m128i a_in = _mm_loadu_si128((const __m128i *)(lo_ptr + 11 * i));
        __m128i b_in = _mm_loadu_si128((const __m128i *)(hi_ptr + 11 * i));
        __m256i a256 = _mm256_broadcastsi128_si256(a_in);
        __m256i b256 = _mm256_broadcastsi128_si256(b_in);

        __m256i av = _mm256_and_si256(
            _mm256_srlv_epi32(_mm256_shuffle_epi8(a256, shuf), vshift), vmask);
        __m256i bv = _mm256_and_si256(
            _mm256_srlv_epi32(_mm256_shuffle_epi8(b256, shuf), vshift), vmask);

        /* diff lanes: [d0 d1 d2 d3 | d4 d5 d6 d7]  (a,b ∈ [0,2047] → no sat) */
        __m256i diff = _mm256_sub_epi32(av, bv);
        __m256i packed = _mm256_packs_epi32(diff, _mm256_setzero_si256());
        /* 64-bit chunks [d0..3 | 0 | d4..7 | 0] → [d0..3 | d4..7 | 0 | 0] */
        packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3,1,2,0));
        _mm_store_si128((__m128i *)(out + 8 * i),
                        _mm256_castsi256_si128(packed));
    }
}

/* 3-bit triangular: LCM(3,8)=24 bits=3 bytes → 8 values per half.
   buf = [lo_half(96B) ‖ hi_half(96B)], total BYTELEN_3BIT = 192. */
#define BYTELEN_3BIT (2 * 3 * BIT_N / 8)

// AVX2 3-bit unpack: structurally identical to the 11-bit path (shuffle to
// overlapping 2-byte windows → vpsrlv per-lane shifts {0,3,6,1,4,7,2,5} → mask).
// 3 input bytes → 8 x 3-bit values; loads use an 8-byte read (over-read stays
// inside the padded XOF buffer and is masked off in the high bits).
static inline void poly_unpack_triangular_3bit_avx(int16_t *out, const uint8_t *buf)
{
    const uint8_t *lo_ptr = buf;
    const uint8_t *hi_ptr = buf + BYTELEN_3BIT / 2;

    // 2-byte windows at byte offsets {0,0,0,1,1,1,2,2} for the 8 values
    __m128i shuf = _mm_setr_epi8(
        0,1, 0,1, 0,1, 1,2,
        1,2, 1,2, 2,3, 2,3
    );
    __m256i shuf256 = _mm256_broadcastsi128_si256(shuf);
    const __m256i vshift = _mm256_setr_epi32(0, 3, 6, 1, 4, 7, 2, 5);
    const __m256i vmask  = _mm256_set1_epi32(0x07);

    for (int i = 0; i < BIT_N / 8; i++) {
        __m128i lo128 = _mm_loadl_epi64((const __m128i *)(lo_ptr + 3*i));
        __m128i hi128 = _mm_loadl_epi64((const __m128i *)(hi_ptr + 3*i));
        __m256i in256  = _mm256_set_m128i(hi128, lo128);

        __m256i win = _mm256_shuffle_epi8(in256, shuf256);
        __m128i lo_w = _mm256_castsi256_si128(win);
        __m128i hi_w = _mm256_extracti128_si256(win, 1);

        __m256i lo32 = _mm256_cvtepu16_epi32(lo_w);
        __m256i hi32 = _mm256_cvtepu16_epi32(hi_w);

        lo32 = _mm256_and_si256(_mm256_srlv_epi32(lo32, vshift), vmask);
        hi32 = _mm256_and_si256(_mm256_srlv_epi32(hi32, vshift), vmask);

        __m256i out32 = _mm256_packus_epi32(lo32, hi32);
        out32 = _mm256_permute4x64_epi64(out32, _MM_SHUFFLE(3,1,2,0));

        __m128i result_lo = _mm256_castsi256_si128(out32);
        __m128i result_hi = _mm256_extracti128_si256(out32, 1);

        _mm_storeu_si128((__m128i *)(out + 8*i),
                         _mm_sub_epi16(result_lo, result_hi));
    }
}

static inline void poly_unpack_triangular_4bit_avx(int16_t *out, const uint8_t *buf)
{
    const uint8_t *lo = buf;
    const uint8_t *hi = buf + BIT_N;  // BYTELEN_4BIT = N, hi starts at N bytes
    const __m256i vmask0F = _mm256_set1_epi16(0x000F);

    // 8 bytes of lo + 8 bytes of hi → 16 nibbles each → 16 coeffs per iter
    for (int i = 0; i < BIT_N / 16; i++) {
        __m128i lo8 = _mm_loadl_epi64((const __m128i *)(lo + 8*i));
        __m128i hi8 = _mm_loadl_epi64((const __m128i *)(hi + 8*i));
        __m128i comb = _mm_unpacklo_epi64(lo8, hi8);  // [lo0..7, hi0..7]

        // Zero-extend 16 bytes → 16 words: [lo_w0..7, hi_w0..7]
        __m256i words = _mm256_cvtepu8_epi16(comb);

        // Low nibbles and high nibbles of each word
        __m256i lo_nib = _mm256_and_si256(words, vmask0F);
        __m256i hi_nib = _mm256_and_si256(_mm256_srli_epi16(words, 4), vmask0F);

        // Split lo/hi parts and subtract: sub_lo = lo_lo - hi_lo, sub_hi = lo_hi - hi_hi
        __m128i lo_lo = _mm256_castsi256_si128(lo_nib);
        __m128i hi_lo = _mm256_extracti128_si256(lo_nib, 1);
        __m128i lo_hi = _mm256_castsi256_si128(hi_nib);
        __m128i hi_hi = _mm256_extracti128_si256(hi_nib, 1);

        __m128i sub_lo = _mm_sub_epi16(lo_lo, hi_lo);
        __m128i sub_hi = _mm_sub_epi16(lo_hi, hi_hi);

        // Interleave: coeff[2*j]=sub_lo[j], coeff[2*j+1]=sub_hi[j]
        _mm_storeu_si128((__m128i *)(out + 16*i),
                         _mm_unpacklo_epi16(sub_lo, sub_hi));
        _mm_storeu_si128((__m128i *)(out + 16*i + 8),
                         _mm_unpackhi_epi16(sub_lo, sub_hi));
    }
}

// ==========================================================================
// Parallel triangular sampling for the full y vector (Y = 6 polynomials).
//
// vec[0] uses 3-bit (or 4-bit) sampling; vec[1..3] (BIT_L) use 10-bit;
// vec[4..5] (BIT_K) use 10-bit (or 11-bit).
//
// All XOF requests are made at the maximum buffer length needed so they can
// share the same x4 / x2 call.  The XOF prefix property guarantees that the
// first N bytes of an M-byte request (M > N) are identical to an N-byte
// request, so smaller unpackers simply ignore the tail bytes they don't need.
// ==========================================================================

/* Squeeze whole SHAKE256 blocks (5 * 136 = 680 >= BYTELEN 640) so shake256x4
   takes the fast nblocks path and skips the partial-block temp copy.  The XOF
   prefix property makes the first BYTELEN bytes identical to a 640-byte squeeze,
   so unpackers (which read only BYTELEN) produce bit-identical output. */
#define SAMPLING_BUF_LEN       BYTELEN
#define SAMPLING_SQUEEZE_LEN   (((BYTELEN + BIT_XOF256_RATE - 1) / BIT_XOF256_RATE) * BIT_XOF256_RATE)
#define SAMPLING_BUF_LEN_PAD   (SAMPLING_SQUEEZE_LEN + 32)

/* gamma_{1,2} = 2^11 needs the 11-bit triangular unpacker (BYTELEN_11 = 704). */
#define SAMPLING_SQUEEZE_LEN_11 (((BYTELEN_11 + BIT_XOF256_RATE - 1) / BIT_XOF256_RATE) * BIT_XOF256_RATE)
#define SAMPLING_BUF_LEN_PAD_11 (SAMPLING_SQUEEZE_LEN_11 + 32)

void polyvecy_sample_y0y1_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    uint8_t ALIGNED_32 buf0[SAMPLING_BUF_LEN_PAD];
    uint8_t ALIGNED_32 buf1[SAMPLING_BUF_LEN_PAD];
    uint8_t ALIGNED_32 buf2[SAMPLING_BUF_LEN_PAD];
    uint8_t ALIGNED_32 buf3[SAMPLING_BUF_LEN_PAD];

    uint16_t n0 = *nonce;
    uint16_t n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U);
    uint16_t n3 = (uint16_t)(*nonce + 3U);

    bit_xof256x4(buf0, buf1, buf2, buf3, SAMPLING_SQUEEZE_LEN,
                    seed_y, BIT_SEEDBYTES, n0, n1, n2, n3);

    poly_unpack_triangular_3bit_avx(y->vec[0].coeffs, buf0);
    poly_unpack_triangular_10bit_avx(y->vec[1].coeffs, buf1);
    poly_unpack_triangular_10bit_avx(y->vec[2].coeffs, buf2);
    poly_unpack_triangular_10bit_avx(y->vec[3].coeffs, buf3);

    *nonce = (uint16_t)(*nonce + 4U);
}

void polyvecy_sample_y2_avx(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce)
{
    /* gamma_{1,2} = 2^11: y2[0..2] use the 11-bit triangular unpacker. */
    uint8_t ALIGNED_32 buf4[SAMPLING_BUF_LEN_PAD_11];
    uint8_t ALIGNED_32 buf5[SAMPLING_BUF_LEN_PAD_11];
    uint8_t ALIGNED_32 buf6[SAMPLING_BUF_LEN_PAD_11];

    uint16_t n4 = *nonce;
    uint16_t n5 = (uint16_t)(*nonce + 1U);
    uint16_t n6 = (uint16_t)(*nonce + 2U);

    // 3 lanes: x4 with one dummy lane
    uint8_t ALIGNED_32 dummy[SAMPLING_BUF_LEN_PAD_11];
    bit_xof256x4(buf4, buf5, buf6, dummy, SAMPLING_SQUEEZE_LEN_11,
                 seed_y, BIT_SEEDBYTES, n4, n5, n6, 0xFFFEU);

    poly_unpack_triangular_11bit_avx(y->vec[4].coeffs, buf4);
    poly_unpack_triangular_11bit_avx(y->vec[5].coeffs, buf5);
    poly_unpack_triangular_11bit_avx(y->vec[6].coeffs, buf6);

    *nonce = (uint16_t)(*nonce + 3U);
}

#define TWO_POW_24 16777216ULL



#define REJECT_POOL_BYTES 136

// Pure memory-driven reject with AVX2 sparse dispatcher (CTZ trick).
// 16-way batch fast-accept, then only failed coefficients visited via __builtin_ctz.
//
// The fallback refill stream is initialised LAZILY: the REJECT_POOL_BYTES (136)
// pre-fetched bytes cover the worst realistic case (boundary coeffs need ~45 B),
// so for the overwhelming majority of calls the stream is never touched and we
// avoid an init + zeroize of a 200-byte keccak_state per polynomial.
static int reject_sample_coeffs_pool(const poly *z,
                                     const int16_t *v,
                                     int32_t gamma1,
                                     int32_t beta2,
                                     const uint8_t *r_buf,
                                     const unsigned char *seed,
                                     uint16_t nonce) {
    const int32_t fast_accept_lo = beta2;
    const int32_t fast_accept_hi = gamma1 - beta2;
    const int32_t G_fast = 2 * (gamma1 - beta2);
    ALIGNED_32 uint8_t local_pool[REJECT_POOL_BYTES];
#if BIT_USE_SHAKE
    const uint8_t *cur = r_buf;
    size_t pos = 0;
#else
    /* SM3: prefetched r_buf is bit_xof256_nonce(seed||nonce) (one-shot), which
       does NOT equal the stateful stream's block 0 (seed||nonce||block).  Drive
       the block-indexed stream from block 0, byte-for-byte like the reference. */
    const uint8_t *cur = local_pool;
    size_t pos = REJECT_POOL_BYTES;   /* force stateful refill before first byte */
    (void)r_buf;
#endif
    bit_xof256_state st;
    int st_inited = 0;  // fallback stream initialised on first exhaustion only
    uint32_t reject_flag = 0;

    const __m256i v_lo_m1 = _mm256_set1_epi16((int16_t)(fast_accept_lo - 1));
    const __m256i v_hi    = _mm256_set1_epi16((int16_t)fast_accept_hi);

    for (int j = 0; j < BIT_N; j += 16) {
        __m256i zj      = _mm256_load_si256((const __m256i *)&z->coeffs[j]);
        __m256i val_vec = _mm256_abs_epi16(zj);

        __m256i cmp_lo   = _mm256_cmpgt_epi16(val_vec, v_lo_m1);
        __m256i cmp_hi   = _mm256_cmpgt_epi16(v_hi, val_vec);
        __m256i fast_all = _mm256_and_si256(cmp_lo, cmp_hi);

        uint32_t mask = _mm256_movemask_epi8(fast_all);

        if (mask == 0xFFFFFFFFu) continue;

        uint32_t fail_bits = (~mask) & 0x55555555u;

        while (fail_bits) {
            int k = __builtin_ctz(fail_bits) >> 1;
            fail_bits &= fail_bits - 1;

            int idx = j + k;
            int32_t val = ct_abs(z->coeffs[idx]);
            int32_t beta1_i = ct_abs(v[idx]);

            if ((uint32_t)(val >= gamma1)) {
                reject_flag = 1;
                continue;
            }

            // Refill from stream if pre-fetched pool exhausted (rare path)
            if (pos + 3 > REJECT_POOL_BYTES) {
                if (!st_inited) {
                    bit_xof256_init(&st, seed, BIT_SEEDBYTES, nonce);
#if BIT_USE_SHAKE
                    bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);  // discard duplicate of r_buf
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
                                            const int16_t *v,
                                            int32_t gamma1,
                                            int32_t beta2,
                                            const uint8_t *r_buf,
                                            const unsigned char *seed,
                                            uint16_t nonce) {
    const int32_t fast_accept_lo = beta2;
    const int32_t fast_accept_hi = gamma1 - beta2;
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

        if (val >= fast_accept_lo && val < fast_accept_hi) continue;

        if ((uint32_t)(val >= gamma1)) {
            reject_flag = 1;
            continue;
        }

        if (pos + 3 > REJECT_POOL_BYTES) {
            if (!st_inited) {
                bit_xof256_init(&st, seed, BIT_SEEDBYTES, nonce);
#if BIT_USE_SHAKE
                bit_xof256_squeeze(&st, local_pool, REJECT_POOL_BYTES);  // discard duplicate of r_buf
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

// x4 pre-fetch + on-demand stream refill for worst-case pool exhaustion
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s,
                                  const poly *c_poly, const sparse_challenge *c_sparse,
                                  const unsigned char *seed, uint16_t *nonce) {
    ALIGNED_32 uint8_t r0[REJECT_POOL_BYTES], r1[REJECT_POOL_BYTES];
    ALIGNED_32 uint8_t r2[REJECT_POOL_BYTES], r3[REJECT_POOL_BYTES];

    uint16_t n0 = *nonce;
    uint16_t n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U);
    uint16_t n3 = (uint16_t)(*nonce + 3U);

#if BIT_USE_SHAKE
    bit_xof256x4(r0, r1, r2, r3, REJECT_POOL_BYTES,
                    seed, BIT_SEEDBYTES, n0, n1, n2, n3);
#endif
    /* SM3 ignores the prefetch pools and drives the stateful stream directly. */
    *nonce = (uint16_t)(*nonce + 4U);

    // Fallback refill streams are initialised lazily inside the pool functions,
    // keyed by (seed, nonce); they are only touched on pool exhaustion.
    int rej = 0;
    if (reject_sample_coeffs_pool(&z->vec[1], c_s->vec[0].coeffs, BIT_GAMMA1, BIT_BETA, r1, seed, n1)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[2], c_s->vec[1].coeffs, BIT_GAMMA1, BIT_BETA, r2, seed, n2)) rej = 1;
    if (!rej && reject_sample_coeffs_pool(&z->vec[3], c_s->vec[2].coeffs, BIT_GAMMA1, BIT_BETA, r3, seed, n3)) rej = 1;
    if (!rej && reject_sample_coeffs_sparse_pool(&z->vec[0], c_sparse, c_poly->coeffs, BIT_GAMMA1_0, 1, r0, seed, n0)) rej = 1;

    return rej;
}

// x2 pre-fetch + on-demand stream refill for worst-case pool exhaustion
int check_reject_sample_z2(const polyvecy *z, const polyveck *c_e,
                                const unsigned char *seed, uint16_t *nonce) {
    ALIGNED_32 uint8_t r4[REJECT_POOL_BYTES], r5[REJECT_POOL_BYTES], r6[REJECT_POOL_BYTES];
    ALIGNED_32 uint8_t dummy[REJECT_POOL_BYTES];

    uint16_t n4 = *nonce;
    uint16_t n5 = (uint16_t)(*nonce + 1U);
    uint16_t n6 = (uint16_t)(*nonce + 2U);

#if BIT_USE_SHAKE
    bit_xof256x4(r4, r5, r6, dummy, REJECT_POOL_BYTES,
                 seed, BIT_SEEDBYTES, n4, n5, n6, (uint16_t)0xFFFEU);
#else
    (void)dummy;  /* SM3 drives the stateful stream directly; no prefetch. */
#endif
    *nonce = (uint16_t)(*nonce + 3U);

    // Fallback refill streams initialised lazily on pool exhaustion only.
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
    uint32_t reject_flag = 0;
#ifdef BIT_HINT_BITPACK
    /* Fixed-width bit packing represents |h| <= BIT_H_INF, no count limit. */
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(BIT_H_INF - abs_h) >> 31;   /* reject |h| > 3 */
        }
    }
#else
    /* base-3 + overflow: |h| <= 2 and at most BIT_H_OVERFLOW_MAX of the +-2. */
    uint32_t count = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(2 - abs_h) >> 31;
            count += (uint32_t)(abs_h >> 1);
        }
    }
    reject_flag |= (uint32_t)(BIT_H_OVERFLOW_MAX - count) >> 31;
#endif
    return (int)reject_flag;
}
