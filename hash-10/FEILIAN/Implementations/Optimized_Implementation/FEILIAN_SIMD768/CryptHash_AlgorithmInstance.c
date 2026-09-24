/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

/* ========================================================================== */
/*  Compiler adaptation                                                       */
/* ========================================================================== */

#if defined(_MSC_VER)
#  define ALIGN32       __declspec(align(32))
#  define ALIGN32_ATTR
#  define RESTRICT      __restrict
#  define MM_PREFETCH(p) _mm_prefetch((const char *)(p), _MM_HINT_NTA)
#else
#  define ALIGN32
#  define ALIGN32_ATTR  __attribute__((aligned(32)))
#  define RESTRICT      __restrict
#  define MM_PREFETCH(p) __builtin_prefetch((p), 0, 0)
#endif

/* ========================================================================== */
/*  Constants                                                                  */
/* ========================================================================== */

#define BLOCK_BITS      1024
#define BLOCK_BYTES     (BLOCK_BITS / 8)
#define MATRIX_ROWS     4
#define MATRIX_COLS     4
#define TOTAL_ROUNDS    20

#define FEILIAN_VERSION UINT64_C(0x300)
#define HASH_BYTES      96    /* 768 / 8 */
#define HASH_ROWS       3     /* output rows 0,1,2; row 3 truncated */

/* ========================================================================== */
/*  SIMD load/store aliases                                                    */
/* ========================================================================== */

#define LOADU(p)    _mm256_loadu_si256((const __m256i *)(p))
#define STOREU(p,r) _mm256_storeu_si256((__m256i *)(p), (r))

/* ========================================================================== */
/*  BLAKE2 shuffle masks                                                       */
/* ========================================================================== */

ALIGN32 static const uint8_t rot24_mask[32] ALIGN32_ATTR = {
    3,4,5,6,7,0,1,2, 11,12,13,14,15,8,9,10,
    3,4,5,6,7,0,1,2, 11,12,13,14,15,8,9,10
};
ALIGN32 static const uint8_t rot16_mask[32] ALIGN32_ATTR = {
    2,3,4,5,6,7,0,1, 10,11,12,13,14,15,8,9,
    2,3,4,5,6,7,0,1, 10,11,12,13,14,15,8,9
};
ALIGN32 static const uint8_t rot8_mask[32] ALIGN32_ATTR = {
    1,2,3,4,5,6,7,0, 9,10,11,12,13,14,15,8,
    1,2,3,4,5,6,7,0, 9,10,11,12,13,14,15,8
};
ALIGN32 static const uint8_t rot40_mask[32] ALIGN32_ATTR = {
    5,6,7,0,1,2,3,4, 13,14,15,8,9,10,11,12,
    5,6,7,0,1,2,3,4, 13,14,15,8,9,10,11,12
};
ALIGN32 static const uint8_t rot48_mask[32] ALIGN32_ATTR = {
    6,7,0,1,2,3,4,5, 14,15,8,9,10,11,12,13,
    6,7,0,1,2,3,4,5, 14,15,8,9,10,11,12,13
};

#define ROT24_MASK (*(const __m256i *)rot24_mask)
#define ROT16_MASK (*(const __m256i *)rot16_mask)
#define ROT8_MASK  (*(const __m256i *)rot8_mask)
#define ROT40_MASK (*(const __m256i *)rot40_mask)
#define ROT48_MASK (*(const __m256i *)rot48_mask)

/* ========================================================================== */
/*  BLAKE2 rotations — all using HW shortcuts                                 */
/* ========================================================================== */

#define ROT24(x) _mm256_shuffle_epi8((x), ROT24_MASK)
#define ROT16(x) _mm256_shuffle_epi8((x), ROT16_MASK)
#define ROT8(x)  _mm256_shuffle_epi8((x), ROT8_MASK)
#define ROT40(x) _mm256_shuffle_epi8((x), ROT40_MASK)
#define ROT48(x) _mm256_shuffle_epi8((x), ROT48_MASK)
#define ROT11(x) _mm256_or_si256(                          \
    _mm256_srli_epi64((x), 11),                             \
    _mm256_slli_epi64((x), 53))
#define ROT41(x) _mm256_or_si256(                          \
    _mm256_srli_epi64((x), 41),                             \
    _mm256_slli_epi64((x), 23))
#define ROT63(x) _mm256_xor_si256(                          \
    _mm256_srli_epi64((x), 63),                              \
    _mm256_add_epi64((x), (x)))
#define ROT5(x) _mm256_or_si256(                           \
    _mm256_srli_epi64((x), 5),                              \
    _mm256_slli_epi64((x), 59))

/* ========================================================================== */
/*  SBOX — 6 BLAKE2 rotations {5,8,11,63,48,41}                              */
/* ========================================================================== */

#define SBOX_SIMD(r0, r1, r2, r3)                                       \
do {                                                                     \
    __m256i _a_sbox = _mm256_add_epi64((r0), (r1));                      \
    __m256i _c_sbox = _mm256_add_epi64((r2), (r3));                      \
    __m256i _d_sbox = _mm256_xor_si256((r3), _a_sbox);                   \
    _d_sbox = ROT8(_d_sbox);                                              \
                                                                         \
    __m256i _s0_a = ROT5(_d_sbox);                                        \
    __m256i _s0_b = ROT48(_d_sbox);                                       \
    __m256i _sigma0 = _mm256_xor_si256(_d_sbox,                           \
                        _mm256_xor_si256(_s0_a, _s0_b));                  \
    _c_sbox = _mm256_add_epi64(_c_sbox, _sigma0);                         \
                                                                         \
    __m256i _b_sbox = _mm256_xor_si256((r1), _c_sbox);                   \
    _b_sbox = ROT63(_b_sbox);                                              \
                                                                         \
    __m256i _s1_a = ROT11(_b_sbox);                                        \
    __m256i _s1_b = ROT40(_b_sbox);                                        \
    __m256i _sigma1 = _mm256_xor_si256(_b_sbox,                           \
                        _mm256_xor_si256(_s1_a, _s1_b));                  \
                                                                         \
    (r0) = _mm256_add_epi64(_a_sbox, _sigma1);                            \
    (r1) = _b_sbox;                                                        \
    (r2) = _c_sbox;                                                        \
    (r3) = _d_sbox;                                                        \
} while (0)

/* ========================================================================== */
/*  SHIFT_ROWS — lane permutation                                              */
/* ========================================================================== */

#define SHIFT_ROWS_SIMD(r1, r2, r3)                                    \
do {                                                                     \
    (r1) = _mm256_permute4x64_epi64((r1), _MM_SHUFFLE(0, 3, 2, 1));     \
    (r2) = _mm256_permute4x64_epi64((r2), _MM_SHUFFLE(1, 0, 3, 2));     \
    (r3) = _mm256_permute4x64_epi64((r3), _MM_SHUFFLE(2, 1, 0, 3));     \
} while (0)

/* ========================================================================== */
/*  ROUND macros                                                              */
/* ========================================================================== */

#define ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc, fab)                    \
do {                                                                     \
    SBOX_SIMD(r0, r1, r2, r3);                                          \
    SHIFT_ROWS_SIMD(r1, r2, r3);                                        \
    SBOX_SIMD(r0, r1, r2, r3);                                          \
    SHIFT_ROWS_SIMD(r1, r2, r3);                                        \
    (r1) = _mm256_xor_si256((r1), (rc));                                 \
    (r3) = _mm256_xor_si256((r3), (fab));                                \
} while (0)

#define ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc, fab)       \
do {                                                                     \
    (r0) = _mm256_xor_si256((r0), (m0));                                 \
    (r2) = _mm256_xor_si256((r2), (m1));                                 \
    SBOX_SIMD(r0, r1, r2, r3);                                          \
    SHIFT_ROWS_SIMD(r1, r2, r3);                                        \
    (r0) = _mm256_xor_si256((r0), (m2));                                 \
    (r2) = _mm256_xor_si256((r2), (m3));                                 \
    SBOX_SIMD(r0, r1, r2, r3);                                          \
    SHIFT_ROWS_SIMD(r1, r2, r3);                                        \
    (r1) = _mm256_xor_si256((r1), (rc));                                 \
    (r3) = _mm256_xor_si256((r3), (fab));                                \
} while (0)

/* ========================================================================== */
/*  Message padding                                                           */
/* ========================================================================== */

static size_t pad_message(const uint8_t *msg, size_t msg_bits,
                          uint8_t **padded, size_t *padded_len)
{
    if (msg_bits == 0) {
        *padded_len = BLOCK_BYTES;
        *padded = (uint8_t *)calloc(BLOCK_BYTES, 1);
        if (*padded == NULL) return 0;
        return 1;
    }

    size_t msg_bytes = (msg_bits + 7) / 8;
    size_t rem = msg_bits % BLOCK_BITS;
    size_t pad_bits = (rem == 0) ? 0 : (BLOCK_BITS - rem);
    *padded_len = msg_bytes + (pad_bits + 7) / 8;
    *padded = (uint8_t *)calloc(*padded_len, 1);
    if (*padded == NULL) return 0;
    memcpy(*padded, msg, msg_bytes);
    return *padded_len / BLOCK_BYTES;
}

/* ========================================================================== */
/*  Matrix helpers (SIMD load/store)                                          */
/* ========================================================================== */

static void copy_state(uint64_t dst[MATRIX_ROWS][MATRIX_COLS],
                       const uint64_t src[MATRIX_ROWS][MATRIX_COLS])
{
    STOREU(&dst[0][0], LOADU(&src[0][0]));
    STOREU(&dst[1][0], LOADU(&src[1][0]));
    STOREU(&dst[2][0], LOADU(&src[2][0]));
    STOREU(&dst[3][0], LOADU(&src[3][0]));
}

/* store_state — output only first HASH_ROWS rows (high 768 bits) */
static void store_state(uint8_t out[HASH_BYTES],
                        const uint64_t state[MATRIX_ROWS][MATRIX_COLS])
{
#if HASH_ROWS >= 1
    STOREU(out +  0, LOADU(&state[0][0]));
#endif
#if HASH_ROWS >= 2
    STOREU(out + 32, LOADU(&state[1][0]));
#endif
#if HASH_ROWS >= 3
    STOREU(out + 64, LOADU(&state[2][0]));
#endif
#if HASH_ROWS >= 4
    /* HASH_ROWS=3: row 3 not output (truncated) */
#endif
}

/* ========================================================================== */
/*  Round constants                                                           */
/* ========================================================================== */

static const uint64_t RC1[MATRIX_COLS] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL
};

static const uint64_t RC2[MATRIX_COLS] = {
    0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL, 0x6a09e667f3bcc908ULL
};

static const uint64_t RC3[MATRIX_COLS] = {
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL
};

static const uint64_t RC4[MATRIX_COLS] = {
    0xa54ff53a5f1d36f1ULL, 0x6a09e667f3bcc908ULL,
    0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL
};

/* ========================================================================== */
/*  Initial value                                                             */
/* ========================================================================== */

static const uint64_t FEILIAN_IV[MATRIX_ROWS][MATRIX_COLS] = {
    { 0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL, 0xa4093822299f31d0ULL, 0x082efa98ec4e6c89ULL },
    { 0x452821e638d01377ULL, 0xbe5466cf34e90c6cULL, 0xc0ac29b7c97c50ddULL, 0x3f84d5b5b5470917ULL },
    { 0x9216d5d98979fb1bULL, 0xd1310ba698dfb5acULL, 0x2ffd72dbd01adfb7ULL, 0xb8e1afed6a267e96ULL },
    { 0xba7c9045f12c7f99ULL, 0x24a19947b3916cf7ULL, 0x0801f2e2858efc16ULL, 0x636920d871574e69ULL }
};

/* ========================================================================== */
/*  Compression function — fab vector: {bits_mid, bits_lo, flag64, version64} */
/* ========================================================================== */

static void feilian_compress(
    uint64_t h_out[RESTRICT MATRIX_ROWS][MATRIX_COLS],
    const uint64_t h_in[RESTRICT MATRIX_ROWS][MATRIX_COLS],
    const uint8_t block[RESTRICT BLOCK_BYTES],
    int is_last_block,
    uint64_t bits_lo, uint64_t bits_mid)
{
    uint64_t version64 = FEILIAN_VERSION;
    uint64_t flag64 = is_last_block ? UINT64_C(0xFFFFFFFFFFFFFFFF)
                                    : UINT64_C(0x0000000000000000);

    const __m256i fab = _mm256_set_epi64x(
        (long long)bits_lo, (long long)bits_mid,
        (long long)flag64,   (long long)version64);

    const __m256i rc1 = LOADU(RC1);
    const __m256i rc2 = LOADU(RC2);
    const __m256i rc3 = LOADU(RC3);
    const __m256i rc4 = LOADU(RC4);

    __m256i r0 = LOADU(&h_in[0][0]);
    __m256i r1 = LOADU(&h_in[1][0]);
    __m256i r2 = LOADU(&h_in[2][0]);
    __m256i r3 = LOADU(&h_in[3][0]);

    const __m256i h0_init = r0;
    const __m256i h1_init = r1;
    const __m256i h2_init = r2;
    const __m256i h3_init = r3;

    const __m256i m0 = LOADU(block +  0);
    const __m256i m1 = LOADU(block + 32);
    const __m256i m2 = LOADU(block + 64);
    const __m256i m3 = LOADU(block + 96);

    /* Group 0 */
    ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);

    /* Group 1 */
    ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc2, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc2, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc2, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc2, fab);

    /* Group 2 */
    ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc3, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc3, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc3, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc3, fab);

    /* Group 3 */
    ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc4, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc4, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc4, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc4, fab);

    /* Group 4 */
    ROUND_WITH_MSG(r0, r1, r2, r3, m0, m1, m2, m3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);
    ROUND_WITHOUT_MSG(r0, r1, r2, r3, rc1, fab);

    /* FeedForward (XOR) */
    STOREU(&h_out[0][0], _mm256_xor_si256(r0, h0_init));
    STOREU(&h_out[1][0], _mm256_xor_si256(r1, h1_init));
    STOREU(&h_out[2][0], _mm256_xor_si256(r2, h2_init));
    STOREU(&h_out[3][0], _mm256_xor_si256(r3, h3_init));
}

/* ========================================================================== */
/*  Internal hash function — double-buffered + prefetch                       */
/* ========================================================================== */

static void feilian_hash_internal(const uint8_t *msg, size_t msg_bits,
                                  uint8_t hash_out[HASH_BYTES])
{
    uint8_t *padded = NULL;
    size_t padded_len = 0;
    size_t num_blocks = pad_message(msg, msg_bits, &padded, &padded_len);
    if (padded == NULL) {
        memset(hash_out, 0, HASH_BYTES);
        return;
    }

    ALIGN32 uint64_t h_buf1[MATRIX_ROWS][MATRIX_COLS] ALIGN32_ATTR;
    ALIGN32 uint64_t h_buf2[MATRIX_ROWS][MATRIX_COLS] ALIGN32_ATTR;
    uint64_t (*RESTRICT h_curr)[MATRIX_COLS] = h_buf1;
    uint64_t (*RESTRICT h_next)[MATRIX_COLS] = h_buf2;

    copy_state(h_curr, FEILIAN_IV);

    uint64_t bits_acc_lo  = 0;
    uint64_t bits_acc_mid = 0;

    for (size_t i = 0; i < num_blocks; i++) {
    	uint64_t block_bits = BLOCK_BITS;
        if (i == num_blocks - 1) {
            block_bits = msg_bits - i * BLOCK_BITS;
        }
        bits_acc_lo += block_bits;
        if (bits_acc_lo < block_bits) {
            bits_acc_mid++;
        }
        if (i + 2 < num_blocks) {
            MM_PREFETCH(&padded[(i + 2) * BLOCK_BYTES]);
        } else if (i + 1 < num_blocks) {
            MM_PREFETCH(&padded[(i + 1) * BLOCK_BYTES]);
        }

        int is_last = (i == num_blocks - 1);

        feilian_compress(h_next, h_curr,
                         &padded[i * BLOCK_BYTES], is_last,
                         bits_acc_lo, bits_acc_mid);

        {
            uint64_t (*RESTRICT tmp)[MATRIX_COLS] = h_curr;
            h_curr = h_next;
            h_next = tmp;
        }
    }

    free(padded);
    store_state(hash_out, h_curr);
}

/* ========================================================================== */
/*  Public API: CryptHash                                                     */
/* ========================================================================== */

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest_len_bits <= 0 || digest_len_bits > DIGEST_BIT_LENGTH)
        return -1;
    if (msg == NULL && msg_len_bits > 0)
        return -1;
    if (digest == NULL)
        return -1;

    uint8_t full_digest[HASH_BYTES];
    feilian_hash_internal(msg, (size_t)msg_len_bits, full_digest);

    size_t out_bytes = (size_t)((digest_len_bits + 7) / 8);
    memcpy(digest, full_digest, out_bytes);

    if (digest_len_bits % 8) {
        unsigned int shift = 8 - (digest_len_bits % 8);
        digest[out_bytes - 1] &= (unsigned char)(~0U << shift);
    }

    return 0;
}
