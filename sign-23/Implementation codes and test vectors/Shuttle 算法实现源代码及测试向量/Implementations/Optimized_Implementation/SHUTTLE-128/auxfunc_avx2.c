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
 * 8-way AVX2 SM3 / KDF-SM3 (pseudoXOF).
 *
 * Layout: lane k of every 256-bit register holds the corresponding 32-bit
 * SM3 word of message k.  A 64-byte block of each of the 8 messages is
 * loaded as two 256-bit vectors and turned into the 16 message words
 * W[0..15] by an 8x8 dword transpose followed by a per-dword byte swap
 * (SM3 reads words big-endian).
 *
 * pseudoXOF detail (matches SHUTTLE/ref/auxfunc.c bit-for-bit):
 *   output block i (i = 0,1,2,...) is SM3(msg || ct) with ct = i+1
 * appended MSB-first as a 32-bit counter.  Because msg is identical across
 * the ct iterations, the *complete* 512-bit blocks that consist purely of
 * msg bits are compressed once into a cached state; only the trailing
 * block(s) that carry ct + padding are rebuilt per counter value.
 */

#include "auxfunc_avx2.h"

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
#define MEMORY_ALLOCATION_FAILED -2

/* ------------------------------------------------------------------ */
/* 256-bit lane-wise 32-bit ALU helpers                               */
/* ------------------------------------------------------------------ */
#define X8(a, b) _mm256_xor_si256((a), (b))
#define A8(a, b) _mm256_add_epi32((a), (b))
#define ND8(a, b) _mm256_and_si256((a), (b))
#define OR8(a, b) _mm256_or_si256((a), (b))
/* Single-uop EVEX rotate when the TU is compiled for AVX512VL (e.g. a
 * Rocket Lake variant with -mavx512vl); the shift|shift|or fallback keeps
 * the default -mavx2 build portable. Both forms are bit-identical rotates.
 */
#ifdef __AVX512VL__
#    define ROL8(x, n) _mm256_rol_epi32((x), (n))
#else
#    define ROL8(x, n)                               \
        _mm256_or_si256(_mm256_slli_epi32((x), (n)), \
                        _mm256_srli_epi32((x), 32 - (n)))
#endif

#define FF1_8(a, b, c) X8(X8((a), (b)), (c))
#define FF2_8(a, b, c) \
    OR8(OR8(ND8((a), (b)), ND8((a), (c))), ND8((b), (c)))
#define GG1_8(e, f, g) X8(X8((e), (f)), (g))
#define GG2_8(e, f, g) X8(ND8(X8((f), (g)), (e)), (g))
#define P0_8(x) X8(X8((x), ROL8((x), 9)), ROL8((x), 17))
#define P1_8(x) X8(X8((x), ROL8((x), 15)), ROL8((x), 23))

/* In-place 8x8 dword transpose (verified against intrinsic semantics). */
static inline void transpose8x8(__m256i v[8])
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

/* ------------------------------------------------------------------ */
/* 8-way compression: absorb `nblk` 64-byte blocks of 8 messages.     */
/* lane[k] points at message k; block b of lane k is lane[k] + b*64.  */
/* ------------------------------------------------------------------ */
static void sm3_avx2_compress8(__m256i state[8],
                               const unsigned char *const lane[8],
                               size_t nblk)
{
    const __m256i bswap = _mm256_setr_epi8(
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12, 3, 2, 1, 0,
        7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);

    for (size_t b = 0; b < nblk; b++) {
        __m256i W[68];
        __m256i lo[8], hi[8];
        for (int k = 0; k < 8; k++) {
            const unsigned char *p = lane[k] + (size_t)b * 64;
            lo[k] = _mm256_loadu_si256((const __m256i *)(p));
            hi[k] = _mm256_loadu_si256((const __m256i *)(p + 32));
        }
        transpose8x8(
            lo); /* lo[j] = word j (little-endian bytes) across lanes */
        transpose8x8(hi); /* hi[j] = word 8+j */
        for (int j = 0; j < 8; j++) {
            W[j] = _mm256_shuffle_epi8(lo[j], bswap);
            W[8 + j] = _mm256_shuffle_epi8(hi[j], bswap);
        }
        /* message expansion W[16..67] */
        for (int i = 16; i < 68; i++) {
            __m256i tmp = X8(X8(W[i - 16], W[i - 9]), ROL8(W[i - 3], 15));
            W[i] = X8(X8(P1_8(tmp), ROL8(W[i - 13], 7)), W[i - 6]);
        }

        __m256i A = state[0], B = state[1], C = state[2], D = state[3];
        __m256i E = state[4], F = state[5], G = state[6], H = state[7];

        /* rounds 0..15 use FF1/GG1 */
        for (int i = 0; i < 16; i++) {
            __m256i kt = _mm256_set1_epi32((int)SM3_T[i]);
            __m256i rotA12 = ROL8(A, 12);
            __m256i SS1 = ROL8(A8(A8(rotA12, E), kt), 7);
            __m256i SS2 = X8(SS1, rotA12);
            __m256i Wp = X8(W[i], W[i + 4]);
            __m256i TT1 = A8(A8(A8(FF1_8(A, B, C), D), SS2), Wp);
            __m256i TT2 = A8(A8(A8(GG1_8(E, F, G), H), SS1), W[i]);
            D = C;
            C = ROL8(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = ROL8(F, 19);
            F = E;
            E = P0_8(TT2);
        }
        /* rounds 16..63 use FF2/GG2 */
        for (int i = 16; i < 64; i++) {
            __m256i kt = _mm256_set1_epi32((int)SM3_T[i]);
            __m256i rotA12 = ROL8(A, 12);
            __m256i SS1 = ROL8(A8(A8(rotA12, E), kt), 7);
            __m256i SS2 = X8(SS1, rotA12);
            __m256i Wp = X8(W[i], W[i + 4]);
            __m256i TT1 = A8(A8(A8(FF2_8(A, B, C), D), SS2), Wp);
            __m256i TT2 = A8(A8(A8(GG2_8(E, F, G), H), SS1), W[i]);
            D = C;
            C = ROL8(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = ROL8(F, 19);
            F = E;
            E = P0_8(TT2);
        }

        state[0] = X8(state[0], A);
        state[1] = X8(state[1], B);
        state[2] = X8(state[2], C);
        state[3] = X8(state[3], D);
        state[4] = X8(state[4], E);
        state[5] = X8(state[5], F);
        state[6] = X8(state[6], G);
        state[7] = X8(state[7], H);
    }
}

/* Extract the 8 digests (32 bytes each, big-endian) into dg[k]. */
static void sm3_avx2_store_digests(const __m256i state[8],
                                   unsigned char dg[8][32])
{
    const __m256i bswap = _mm256_setr_epi8(
        3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12, 3, 2, 1, 0,
        7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
    __m256i v[8];
    for (int j = 0; j < 8; j++)
        v[j] = _mm256_shuffle_epi8(state[j],
                                   bswap); /* word j, big-endian bytes */
    transpose8x8(v);                       /* v[k] = lane k's 8 words */
    for (int k = 0; k < 8; k++)
        _mm256_storeu_si256((__m256i *)dg[k], v[k]);
}

/* ------------------------------------------------------------------ */
/* Scalar helper: build the trailing block(s) of msg||ct for one lane */
/* exactly as the reference does, but only for the region that starts */
/* at the first non-constant block (byte offset tail_off).            */
/* ------------------------------------------------------------------ */
static void build_final_region(unsigned char *fb, size_t final_bytes,
                               const unsigned char *msg,
                               unsigned long long msg_len_bits,
                               size_t tail_off, unsigned int ct,
                               unsigned long long Lb)
{
    memset(fb, 0, final_bytes);
    size_t mbytes = (size_t)((msg_len_bits + 7) / 8);
    size_t tail_bytes =
        mbytes - tail_off; /* msg tail bytes living in this region */
    if (tail_bytes)
        memcpy(fb, msg + tail_off, tail_bytes);

    unsigned r =
        (unsigned)(msg_len_bits & 7); /* valid bits in last msg byte */
    size_t e = tail_bytes;            /* fb index just past the msg tail */
    if (r == 0) {
        fb[e + 0] = (unsigned char)(ct >> 24);
        fb[e + 1] = (unsigned char)((ct & 0xffffff) >> 16);
        fb[e + 2] = (unsigned char)((ct & 0xffff) >> 8);
        fb[e + 3] = (unsigned char)(ct & 0xff);
    } else {
        /* normalize() the last msg byte, then OR in ct MSB-first. */
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

    /* SM3 padding: a single '1' bit at position Lb (relative to whole
     * msg). */
    unsigned long long pad_bit = Lb - (unsigned long long)tail_off * 8;
    fb[pad_bit >> 3] |= (unsigned char)(1u << (7 - (pad_bit & 7)));

    /* 64-bit length field, encoded exactly as the reference sm3_bit()
     * does. */
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

/* zero the unused low bits of the final output byte (reference normalize)
 */
static void normalize_out(unsigned char *out,
                          unsigned long long total_bits)
{
    unsigned r = (unsigned)(total_bits & 7);
    if (r)
        out[total_bits / 8] &=
            (unsigned char)(~((1u << (8 - r)) - 1u) & 0xff);
}

/* ------------------------------------------------------------------ */
/* Public: 8-way SM3 hash (256-bit digests).                          */
/* ------------------------------------------------------------------ */
int sm3hash_avx2(const unsigned char *const msg[8],
                 unsigned long long msg_len_bits,
                 unsigned char *const digest[8])
{
    unsigned long long block_num = msg_len_bits / 512;
    unsigned long long remain = msg_len_bits & 0x1FF;
    size_t full_blocks = (size_t)block_num;
    size_t tail_off = full_blocks * 64;

    __m256i state[8];
    for (int j = 0; j < 8; j++)
        state[j] = _mm256_set1_epi32((int)SM3_IV[j]);

    if (full_blocks)
        sm3_avx2_compress8(state, msg, full_blocks);

    /* trailing block(s): tail bits + 0x80 pad + 64-bit length */
    size_t final_blocks = (remain <= (512 - 65)) ? 1 : 2;
    size_t final_bytes = final_blocks * 64;
    unsigned char fb[8][128];
    assert(final_bytes <= sizeof fb[0]); /* fb[][128] is the only sink */
    const unsigned char *lane[8];
    for (int k = 0; k < 8; k++) {
        memset(fb[k], 0, final_bytes);
        size_t mbytes = (size_t)((msg_len_bits + 7) / 8);
        size_t tail_bytes = mbytes - tail_off;
        if (tail_bytes)
            memcpy(fb[k], msg[k] + tail_off, tail_bytes);
        unsigned rr = (unsigned)(msg_len_bits & 7);
        size_t e = tail_bytes;
        /* append the single padding bit right after the message */
        if (rr)
            fb[k][e - 1] =
                (unsigned char)(fb[k][e - 1] &
                                (~((1u << (8 - rr)) - 1u) & 0xff));
        unsigned long long pad_bit =
            msg_len_bits - (unsigned long long)tail_off * 8;
        fb[k][pad_bit >> 3] |= (unsigned char)(1u << (7 - (pad_bit & 7)));
        /* length */
        uint32_t hi = (uint32_t)((block_num >> 32) << 9);
        uint32_t lo = (uint32_t)((block_num << 9) + remain);
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
    sm3_avx2_compress8(state, lane, final_blocks);

    unsigned char dg[8][32];
    sm3_avx2_store_digests(state, dg);
    for (int k = 0; k < 8; k++)
        memcpy(digest[k], dg[k], 32);
    return XOF_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* Public: 8-way KDF-SM3 XOF.                                         */
/* ------------------------------------------------------------------ */
int pseudoXOF_avx2(unsigned long long output_len_bits,
                   const unsigned char *const msg[8],
                   unsigned long long msg_len_bits,
                   unsigned char *const output[8])
{
    const unsigned long long Lb =
        msg_len_bits + 32; /* SM3 input length per ct */
    const unsigned long long block_num = Lb / 512;
    const unsigned long long remain = Lb & 0x1FF;
    const unsigned long long nblk =
        block_num + ((remain <= (512 - 65)) ? 1 : 2);

    const size_t msg_full_blocks = (size_t)(msg_len_bits / 512);
    const size_t tail_off = msg_full_blocks * 64;
    const size_t final_blocks =
        (size_t)nblk - msg_full_blocks; /* 1 or 2 */
    const size_t final_bytes = final_blocks * 64;

    const unsigned long long obytes = (output_len_bits + 7) / 8;
    const unsigned long long nblocks_out = (output_len_bits + 255) / 256;

    /* Precompute the constant pure-msg blocks once into cached state. */
    __m256i cached[8];
    for (int j = 0; j < 8; j++)
        cached[j] = _mm256_set1_epi32((int)SM3_IV[j]);
    if (msg_full_blocks)
        sm3_avx2_compress8(cached, msg, msg_full_blocks);

    unsigned char fb[8][128];
    assert(final_blocks <= 2 && final_bytes <= sizeof fb[0]);
    const unsigned char *lane[8];
    for (int k = 0; k < 8; k++)
        lane[k] = fb[k];

    for (unsigned long long i = 0; i < nblocks_out; i++) {
        unsigned int ct = (unsigned int)(i + 1);
        for (int k = 0; k < 8; k++)
            build_final_region(fb[k], final_bytes, msg[k], msg_len_bits,
                               tail_off, ct, Lb);

        __m256i state[8];
        memcpy(state, cached, sizeof(state));
        sm3_avx2_compress8(state, lane, final_blocks);

        unsigned char dg[8][32];
        sm3_avx2_store_digests(state, dg);

        size_t off = (size_t)(i * 32);
        size_t n = (size_t)(obytes - off);
        if (n > 32)
            n = 32;
        for (int k = 0; k < 8; k++)
            memcpy(output[k] + off, dg[k], n);
    }

    if (output_len_bits & 7)
        for (int k = 0; k < 8; k++)
            normalize_out(output[k], output_len_bits);

    return XOF_SUCCESS;
}
