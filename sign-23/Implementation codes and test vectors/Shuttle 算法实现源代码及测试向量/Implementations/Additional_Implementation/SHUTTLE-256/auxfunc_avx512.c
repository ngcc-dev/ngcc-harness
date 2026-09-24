/*
The software is provided by the Institute of Commercial Cryptography
Standards (ICCS), and is used for algorithm submissions in the
Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will
be uninterrupted or error-free in all cases. ICCS will take no
responsibility for the use of the software or the results thereof, if the
software is used for any other purposes.
*/

/*
 * 16-way AVX512 SM3 / KDF-SM3 (pseudoXOF).
 *
 * Lane k of every 512-bit register holds the corresponding 32-bit SM3 word
 * of message k.  A 64-byte block of each of the 16 messages is one ZMM
 * load; the 16 loads are turned into the message words W[0..15] by a 16x16
 * dword transpose (verified against intrinsic semantics) and a per-dword
 * byte swap.
 *
 * Boolean round functions use _mm512_ternarylogic_epi32:
 *   FF1/GG1/P0/P1 fold XOR-of-three      -> imm 0x96
 *   FF2           majority(a,b,c)         -> imm 0xE8
 *   GG2           ((f^g)&e)^g             -> imm 0xCA
 *
 * The pseudoXOF padding / counter logic is byte-for-byte identical to the
 * scalar reference (SHUTTLE/ref/auxfunc.c); see auxfunc_avx2.c for the
 * matching notes.
 */

#include "auxfunc_avx512.h"

#include <assert.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sm3_const.h"

/* off/n below are 64-bit but funnel through size_t; pin the x86-64
 * assumption. */
_Static_assert(sizeof(size_t) >= 8, "64-bit size_t required");

#define XOF_SUCCESS 0

/* ------------------------------------------------------------------ */
/* 512-bit lane-wise 32-bit helpers                                   */
/* ------------------------------------------------------------------ */
#define X16(a, b) _mm512_xor_si512((a), (b))
#define A16(a, b) _mm512_add_epi32((a), (b))
#define ROL16(x, n) _mm512_rol_epi32((x), (n))

#define XOR3_16(a, b, c) _mm512_ternarylogic_epi32((a), (b), (c), 0x96)
#define FF1_16(a, b, c) XOR3_16((a), (b), (c))
#define FF2_16(a, b, c) \
    _mm512_ternarylogic_epi32((a), (b), (c), 0xE8) /* majority */
#define GG1_16(e, f, g) XOR3_16((e), (f), (g))
#define GG2_16(e, f, g) \
    _mm512_ternarylogic_epi32((e), (f), (g), 0xCA) /* ((f^g)&e)^g */
#define P0_16(x) XOR3_16((x), ROL16((x), 9), ROL16((x), 17))
#define P1_16(x) XOR3_16((x), ROL16((x), 15), ROL16((x), 23))

/* In-place 16x16 dword transpose (4 stages, verified). */
static inline void transpose16x16(__m512i v[16])
{
    __m512i t[16], s[16], u[16];
    /* stage 1: unpack epi32 on adjacent pairs */
    for (int p = 0; p < 8; p++) {
        t[2 * p] = _mm512_unpacklo_epi32(v[2 * p], v[2 * p + 1]);
        t[2 * p + 1] = _mm512_unpackhi_epi32(v[2 * p], v[2 * p + 1]);
    }
    /* stage 2: unpack epi64 */
    s[0] = _mm512_unpacklo_epi64(t[0], t[2]);
    s[1] = _mm512_unpackhi_epi64(t[0], t[2]);
    s[2] = _mm512_unpacklo_epi64(t[1], t[3]);
    s[3] = _mm512_unpackhi_epi64(t[1], t[3]);
    s[4] = _mm512_unpacklo_epi64(t[4], t[6]);
    s[5] = _mm512_unpackhi_epi64(t[4], t[6]);
    s[6] = _mm512_unpacklo_epi64(t[5], t[7]);
    s[7] = _mm512_unpackhi_epi64(t[5], t[7]);
    s[8] = _mm512_unpacklo_epi64(t[8], t[10]);
    s[9] = _mm512_unpackhi_epi64(t[8], t[10]);
    s[10] = _mm512_unpacklo_epi64(t[9], t[11]);
    s[11] = _mm512_unpackhi_epi64(t[9], t[11]);
    s[12] = _mm512_unpacklo_epi64(t[12], t[14]);
    s[13] = _mm512_unpackhi_epi64(t[12], t[14]);
    s[14] = _mm512_unpacklo_epi64(t[13], t[15]);
    s[15] = _mm512_unpackhi_epi64(t[13], t[15]);
    /* stage 3: shuffle 128-bit lanes across the 256-bit boundary */
    u[0] = _mm512_shuffle_i32x4(s[0], s[4], 0x88);
    u[1] = _mm512_shuffle_i32x4(s[1], s[5], 0x88);
    u[2] = _mm512_shuffle_i32x4(s[2], s[6], 0x88);
    u[3] = _mm512_shuffle_i32x4(s[3], s[7], 0x88);
    u[4] = _mm512_shuffle_i32x4(s[8], s[12], 0x88);
    u[5] = _mm512_shuffle_i32x4(s[9], s[13], 0x88);
    u[6] = _mm512_shuffle_i32x4(s[10], s[14], 0x88);
    u[7] = _mm512_shuffle_i32x4(s[11], s[15], 0x88);
    u[8] = _mm512_shuffle_i32x4(s[0], s[4], 0xdd);
    u[9] = _mm512_shuffle_i32x4(s[1], s[5], 0xdd);
    u[10] = _mm512_shuffle_i32x4(s[2], s[6], 0xdd);
    u[11] = _mm512_shuffle_i32x4(s[3], s[7], 0xdd);
    u[12] = _mm512_shuffle_i32x4(s[8], s[12], 0xdd);
    u[13] = _mm512_shuffle_i32x4(s[9], s[13], 0xdd);
    u[14] = _mm512_shuffle_i32x4(s[10], s[14], 0xdd);
    u[15] = _mm512_shuffle_i32x4(s[11], s[15], 0xdd);
    /* stage 4: shuffle 128-bit lanes across the 512-bit boundary */
    v[0] = _mm512_shuffle_i32x4(u[0], u[4], 0x88);
    v[1] = _mm512_shuffle_i32x4(u[1], u[5], 0x88);
    v[2] = _mm512_shuffle_i32x4(u[2], u[6], 0x88);
    v[3] = _mm512_shuffle_i32x4(u[3], u[7], 0x88);
    v[4] = _mm512_shuffle_i32x4(u[8], u[12], 0x88);
    v[5] = _mm512_shuffle_i32x4(u[9], u[13], 0x88);
    v[6] = _mm512_shuffle_i32x4(u[10], u[14], 0x88);
    v[7] = _mm512_shuffle_i32x4(u[11], u[15], 0x88);
    v[8] = _mm512_shuffle_i32x4(u[0], u[4], 0xdd);
    v[9] = _mm512_shuffle_i32x4(u[1], u[5], 0xdd);
    v[10] = _mm512_shuffle_i32x4(u[2], u[6], 0xdd);
    v[11] = _mm512_shuffle_i32x4(u[3], u[7], 0xdd);
    v[12] = _mm512_shuffle_i32x4(u[8], u[12], 0xdd);
    v[13] = _mm512_shuffle_i32x4(u[9], u[13], 0xdd);
    v[14] = _mm512_shuffle_i32x4(u[10], u[14], 0xdd);
    v[15] = _mm512_shuffle_i32x4(u[11], u[15], 0xdd);
}

/* ------------------------------------------------------------------ */
/* 16-way compression: absorb `nblk` 64-byte blocks of 16 messages.   */
/* ------------------------------------------------------------------ */
static void sm3_avx512_compress16(__m512i state[8],
                                  const unsigned char *const lane[16],
                                  size_t nblk)
{
    const __m512i bswap = _mm512_broadcast_i32x4(_mm_setr_epi8(
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));

    for (size_t b = 0; b < nblk; b++) {
        __m512i W[68];
        for (int k = 0; k < 16; k++)
            W[k] = _mm512_loadu_si512(
                (const void *)(lane[k] + (size_t)b * 64));
        transpose16x16(
            W); /* W[j] = word j (little-endian bytes) across lanes */
        for (int j = 0; j < 16; j++)
            W[j] = _mm512_shuffle_epi8(W[j], bswap);

        for (int i = 16; i < 68; i++) {
            __m512i tmp =
                XOR3_16(W[i - 16], W[i - 9], ROL16(W[i - 3], 15));
            W[i] = XOR3_16(P1_16(tmp), ROL16(W[i - 13], 7), W[i - 6]);
        }

        __m512i A = state[0], B = state[1], C = state[2], D = state[3];
        __m512i E = state[4], F = state[5], G = state[6], H = state[7];

        for (int i = 0; i < 16; i++) {
            __m512i kt = _mm512_set1_epi32((int)SM3_T[i]);
            __m512i rotA12 = ROL16(A, 12);
            __m512i SS1 = ROL16(A16(A16(rotA12, E), kt), 7);
            __m512i SS2 = X16(SS1, rotA12);
            __m512i Wp = X16(W[i], W[i + 4]);
            __m512i TT1 = A16(A16(A16(FF1_16(A, B, C), D), SS2), Wp);
            __m512i TT2 = A16(A16(A16(GG1_16(E, F, G), H), SS1), W[i]);
            D = C;
            C = ROL16(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = ROL16(F, 19);
            F = E;
            E = P0_16(TT2);
        }
        for (int i = 16; i < 64; i++) {
            __m512i kt = _mm512_set1_epi32((int)SM3_T[i]);
            __m512i rotA12 = ROL16(A, 12);
            __m512i SS1 = ROL16(A16(A16(rotA12, E), kt), 7);
            __m512i SS2 = X16(SS1, rotA12);
            __m512i Wp = X16(W[i], W[i + 4]);
            __m512i TT1 = A16(A16(A16(FF2_16(A, B, C), D), SS2), Wp);
            __m512i TT2 = A16(A16(A16(GG2_16(E, F, G), H), SS1), W[i]);
            D = C;
            C = ROL16(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = ROL16(F, 19);
            F = E;
            E = P0_16(TT2);
        }

        state[0] = X16(state[0], A);
        state[1] = X16(state[1], B);
        state[2] = X16(state[2], C);
        state[3] = X16(state[3], D);
        state[4] = X16(state[4], E);
        state[5] = X16(state[5], F);
        state[6] = X16(state[6], G);
        state[7] = X16(state[7], H);
    }
}

/* 256-bit in-place 8x8 dword transpose (identical to the AVX2 build). */
static inline void transpose8x8_256(__m256i v[8])
{
    __m256i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i s0, s1, s2, s3, s4, s5, s6, s7;
    t0 = _mm256_unpacklo_epi32(v[0], v[1]);
    t1 = _mm256_unpackhi_epi32(v[0], v[1]);
    t2 = _mm256_unpacklo_epi32(v[2], v[3]);
    t3 = _mm256_unpackhi_epi32(v[2], v[3]);
    t4 = _mm256_unpacklo_epi32(v[4], v[5]);
    t5 = _mm256_unpackhi_epi32(v[4], v[5]);
    t6 = _mm256_unpacklo_epi32(v[6], v[7]);
    t7 = _mm256_unpackhi_epi32(v[6], v[7]);
    s0 = _mm256_unpacklo_epi64(t0, t2);
    s1 = _mm256_unpackhi_epi64(t0, t2);
    s2 = _mm256_unpacklo_epi64(t1, t3);
    s3 = _mm256_unpackhi_epi64(t1, t3);
    s4 = _mm256_unpacklo_epi64(t4, t6);
    s5 = _mm256_unpackhi_epi64(t4, t6);
    s6 = _mm256_unpacklo_epi64(t5, t7);
    s7 = _mm256_unpackhi_epi64(t5, t7);
    v[0] = _mm256_permute2x128_si256(s0, s4, 0x20);
    v[1] = _mm256_permute2x128_si256(s1, s5, 0x20);
    v[2] = _mm256_permute2x128_si256(s2, s6, 0x20);
    v[3] = _mm256_permute2x128_si256(s3, s7, 0x20);
    v[4] = _mm256_permute2x128_si256(s0, s4, 0x31);
    v[5] = _mm256_permute2x128_si256(s1, s5, 0x31);
    v[6] = _mm256_permute2x128_si256(s2, s6, 0x31);
    v[7] = _mm256_permute2x128_si256(s3, s7, 0x31);
}

/*
 * Extract 16 digests (32 bytes each, big-endian) into dg[k].
 *
 * The state is 8 words x 16 lanes. Rather than zero-pad to a 16x16
 * transpose and throw away the upper 256 bits of every output, byte-swap
 * each of the 8 state words and transpose the low (lanes 0..7) and high
 * (lanes 8..15) 256-bit halves separately with the 8x8 routine -- the
 * digest size matches the 256-bit half exactly, so nothing is computed and
 * discarded.
 */
static void sm3_avx512_store_digests(const __m512i state[8],
                                     unsigned char dg[16][32])
{
    const __m512i bswap = _mm512_broadcast_i32x4(_mm_setr_epi8(
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));
    __m256i lo[8], hi[8];
    for (int j = 0; j < 8; j++) {
        __m512i bw = _mm512_shuffle_epi8(state[j], bswap);
        lo[j] = _mm512_castsi512_si256(bw);       /* lanes 0..7,  word j */
        hi[j] = _mm512_extracti64x4_epi64(bw, 1); /* lanes 8..15, word j */
    }
    transpose8x8_256(lo); /* lo[k] = lane k's 8 words     */
    transpose8x8_256(hi); /* hi[k] = lane (8+k)'s 8 words */
    for (int k = 0; k < 8; k++) {
        _mm256_storeu_si256((__m256i *)dg[k], lo[k]);
        _mm256_storeu_si256((__m256i *)dg[8 + k], hi[k]);
    }
}

/* ------------------------------------------------------------------ */
/* Scalar helpers (identical to the AVX2 build).                       */
/* ------------------------------------------------------------------ */
static void build_final_region(unsigned char *fb, size_t final_bytes,
                               const unsigned char *msg,
                               unsigned long long msg_len_bits,
                               size_t tail_off, unsigned int ct,
                               unsigned long long Lb)
{
    memset(fb, 0, final_bytes);
    size_t mbytes = (size_t)((msg_len_bits + 7) / 8);
    size_t tail_bytes = mbytes - tail_off;
    if (tail_bytes)
        memcpy(fb, msg + tail_off, tail_bytes);

    unsigned r = (unsigned)(msg_len_bits & 7);
    size_t e = tail_bytes;
    if (r == 0) {
        fb[e + 0] = (unsigned char)(ct >> 24);
        fb[e + 1] = (unsigned char)((ct & 0xffffff) >> 16);
        fb[e + 2] = (unsigned char)((ct & 0xffff) >> 8);
        fb[e + 3] = (unsigned char)(ct & 0xff);
    } else {
        fb[e - 1] =
            (unsigned char)(fb[e - 1] & (~((1u << (8 - r)) - 1u) & 0xff));
        fb[e - 1] = (unsigned char)(fb[e - 1] ^ (ct >> (32 - (8 - r))));
        fb[e + 0] = (unsigned char)(((ct << (8 - r)) & 0xffffffffu) >> 24);
        fb[e + 1] =
            (unsigned char)(((ct << (16 - r)) & 0xffffffffu) >> 24);
        fb[e + 2] =
            (unsigned char)(((ct << (24 - r)) & 0xffffffffu) >> 24);
        fb[e + 3] =
            (unsigned char)(((ct << (32 - r)) & 0xffffffffu) >> 24);
    }

    unsigned long long pad_bit = Lb - (unsigned long long)tail_off * 8;
    fb[pad_bit >> 3] |= (unsigned char)(1u << (7 - (pad_bit & 7)));

    unsigned long long block_num = Lb / 512;
    unsigned long long remain = Lb & 0x1FF;
    uint32_t hi = (uint32_t)((block_num >> 32) << 9);
    uint32_t lo = (uint32_t)((block_num << 9) + remain);
    unsigned char *L = fb + final_bytes - 8;
    L[0] = (unsigned char)(hi >> 24);
    L[1] = (unsigned char)(hi >> 16);
    L[2] = (unsigned char)(hi >> 8);
    L[3] = (unsigned char)(hi);
    L[4] = (unsigned char)(lo >> 24);
    L[5] = (unsigned char)(lo >> 16);
    L[6] = (unsigned char)(lo >> 8);
    L[7] = (unsigned char)(lo);
}

static void normalize_out(unsigned char *out,
                          unsigned long long total_bits)
{
    unsigned r = (unsigned)(total_bits & 7);
    if (r)
        out[total_bits / 8] &=
            (unsigned char)(~((1u << (8 - r)) - 1u) & 0xff);
}

/* ------------------------------------------------------------------ */
/* Public: 16-way SM3 hash.                                           */
/* ------------------------------------------------------------------ */
int sm3hash_avx512(const unsigned char *const msg[16],
                   unsigned long long msg_len_bits,
                   unsigned char *const digest[16])
{
    unsigned long long block_num = msg_len_bits / 512;
    unsigned long long remain = msg_len_bits & 0x1FF;
    size_t full_blocks = (size_t)block_num;
    size_t tail_off = full_blocks * 64;

    __m512i state[8];
    for (int j = 0; j < 8; j++)
        state[j] = _mm512_set1_epi32((int)SM3_IV[j]);
    if (full_blocks)
        sm3_avx512_compress16(state, msg, full_blocks);

    size_t final_blocks = (remain <= (512 - 65)) ? 1 : 2;
    size_t final_bytes = final_blocks * 64;
    unsigned char fb[16][128];
    assert(final_bytes <= sizeof fb[0]); /* fb[][128] is the only sink */
    const unsigned char *lane[16];
    size_t mbytes = (size_t)((msg_len_bits + 7) / 8);
    size_t tail_bytes = mbytes - tail_off;
    unsigned rr = (unsigned)(msg_len_bits & 7);
    unsigned long long pad_bit =
        msg_len_bits - (unsigned long long)tail_off * 8;
    uint32_t hi = (uint32_t)((block_num >> 32) << 9);
    uint32_t lo = (uint32_t)((block_num << 9) + remain);
    for (int k = 0; k < 16; k++) {
        memset(fb[k], 0, final_bytes);
        if (tail_bytes)
            memcpy(fb[k], msg[k] + tail_off, tail_bytes);
        size_t e = tail_bytes;
        if (rr)
            fb[k][e - 1] =
                (unsigned char)(fb[k][e - 1] &
                                (~((1u << (8 - rr)) - 1u) & 0xff));
        fb[k][pad_bit >> 3] |= (unsigned char)(1u << (7 - (pad_bit & 7)));
        unsigned char *L = fb[k] + final_bytes - 8;
        L[0] = (unsigned char)(hi >> 24);
        L[1] = (unsigned char)(hi >> 16);
        L[2] = (unsigned char)(hi >> 8);
        L[3] = (unsigned char)(hi);
        L[4] = (unsigned char)(lo >> 24);
        L[5] = (unsigned char)(lo >> 16);
        L[6] = (unsigned char)(lo >> 8);
        L[7] = (unsigned char)(lo);
        lane[k] = fb[k];
    }
    sm3_avx512_compress16(state, lane, final_blocks);

    unsigned char dg[16][32];
    sm3_avx512_store_digests(state, dg);
    for (int k = 0; k < 16; k++)
        memcpy(digest[k], dg[k], 32);
    return XOF_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* Public: 16-way KDF-SM3 XOF.                                        */
/* ------------------------------------------------------------------ */
int pseudoXOF_avx512(unsigned long long output_len_bits,
                     const unsigned char *const msg[16],
                     unsigned long long msg_len_bits,
                     unsigned char *const output[16])
{
    const unsigned long long Lb = msg_len_bits + 32;
    const unsigned long long block_num = Lb / 512;
    const unsigned long long remain = Lb & 0x1FF;
    const unsigned long long nblk =
        block_num + ((remain <= (512 - 65)) ? 1 : 2);

    const size_t msg_full_blocks = (size_t)(msg_len_bits / 512);
    const size_t tail_off = msg_full_blocks * 64;
    const size_t final_blocks = (size_t)nblk - msg_full_blocks;
    const size_t final_bytes = final_blocks * 64;

    const unsigned long long obytes = (output_len_bits + 7) / 8;
    const unsigned long long nblocks_out = (output_len_bits + 255) / 256;

    __m512i cached[8];
    for (int j = 0; j < 8; j++)
        cached[j] = _mm512_set1_epi32((int)SM3_IV[j]);
    if (msg_full_blocks)
        sm3_avx512_compress16(cached, msg, msg_full_blocks);

    unsigned char fb[16][128];
    assert(final_blocks <= 2 && final_bytes <= sizeof fb[0]);
    const unsigned char *lane[16];
    for (int k = 0; k < 16; k++)
        lane[k] = fb[k];

    for (unsigned long long i = 0; i < nblocks_out; i++) {
        unsigned int ct = (unsigned int)(i + 1);
        for (int k = 0; k < 16; k++)
            build_final_region(fb[k], final_bytes, msg[k], msg_len_bits,
                               tail_off, ct, Lb);

        __m512i state[8];
        memcpy(state, cached, sizeof(state));
        sm3_avx512_compress16(state, lane, final_blocks);

        unsigned char dg[16][32];
        sm3_avx512_store_digests(state, dg);

        size_t off = (size_t)(i * 32);
        size_t n = (size_t)(obytes - off);
        if (n > 32)
            n = 32;
        for (int k = 0; k < 16; k++)
            memcpy(output[k] + off, dg[k], n);
    }

    if (output_len_bits & 7)
        for (int k = 0; k < 16; k++)
            normalize_out(output[k], output_len_bits);

    return XOF_SUCCESS;
}
