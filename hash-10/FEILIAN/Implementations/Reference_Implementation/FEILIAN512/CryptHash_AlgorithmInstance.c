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

/* ========================================================================== */
/*  Constants                                                                  */
/* ========================================================================== */

#define BLOCK_BITS      1024
#define BLOCK_BYTES     (BLOCK_BITS / 8)
#define WORDS_PER_BLOCK (BLOCK_BITS / 64)
#define MATRIX_ROWS     4
#define MATRIX_COLS     4
#define TOTAL_ROUNDS    20

#define FEILIAN_VERSION UINT64_C(0x200)
#define HASH_BYTES      64    /* 512 / 8 */
#define HASH_ROWS       2     /* output rows 0,1; rows 2,3 truncated */

/* ========================================================================== */
/*  64-bit rotate right                                                       */
/* ========================================================================== */

static inline uint64_t rotr64(uint64_t x, unsigned int n)
{
    n &= 63;
    return (x >> n) | (x << ((64 - n) & 63));
}

/* ========================================================================== */
/*  S-box Sigma functions                                                     */
/* ========================================================================== */

static inline uint64_t Sigma0_sbox(uint64_t x)
{
    return x ^ rotr64(x, 5) ^ rotr64(x, 48);
}

static inline uint64_t Sigma1_sbox(uint64_t x)
{
    return x ^ rotr64(x, 11) ^ rotr64(x, 40);
}

/* ========================================================================== */
/*  Message block load (little-endian)                                        */
/* ========================================================================== */

static void load_block(const uint8_t block[BLOCK_BYTES], uint64_t w[WORDS_PER_BLOCK])
{
    for (int j = 0; j < WORDS_PER_BLOCK; j++) {
        w[j] = 0;
        for (int k = 0; k < 8; k++) {
            w[j] |= (uint64_t)block[j * 8 + k] << (8 * k);
        }
    }
}

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
/*  Matrix helpers                                                            */
/* ========================================================================== */

static void copy_state(uint64_t dst[MATRIX_ROWS][MATRIX_COLS],
                       const uint64_t src[MATRIX_ROWS][MATRIX_COLS])
{
    memcpy(dst, src, BLOCK_BYTES);
}

static void add_state(uint64_t a[MATRIX_ROWS][MATRIX_COLS],
                      const uint64_t b[MATRIX_ROWS][MATRIX_COLS])
{
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            a[r][c] ^= b[r][c];
        }
    }
}

/* store_state — output only the first HASH_ROWS rows (high 512 bits) */
static void store_state(uint8_t out[HASH_BYTES],
                        const uint64_t state[MATRIX_ROWS][MATRIX_COLS])
{
    for (int r = 0; r < HASH_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            int idx = (r * MATRIX_COLS + c) * 8;
            uint64_t v = state[r][c];
            for (int k = 0; k < 8; k++) {
                out[idx + k] = (uint8_t)(v >> (8 * k));
            }
        }
    }
}

/* ========================================================================== */
/*  128-bit hash_bits helper                                                  */
/* ========================================================================== */

static void set_hash_bits(uint64_t hash_bits[2],
                          uint64_t bits_lo, uint64_t bits_mid)
{
    hash_bits[0] = bits_mid;
    hash_bits[1] = bits_lo;
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
/*  S-box column operation                                                    */
/* ========================================================================== */

static inline void sbox_column(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d)
{
    *a += *b;
    *c += *d;
    *d ^= *a;   *d = rotr64(*d, 8);
    *c += Sigma0_sbox(*d);
    *b ^= *c;   *b = rotr64(*b, 63);
    *a += Sigma1_sbox(*b);
}

static void sbox_matrix(uint64_t state[MATRIX_ROWS][MATRIX_COLS])
{
    for (int col = 0; col < MATRIX_COLS; col++)
        sbox_column(&state[0][col], &state[1][col], &state[2][col], &state[3][col]);
}

/* ========================================================================== */
/*  Shift rows                                                                */
/* ========================================================================== */

static void shift_rows(uint64_t state[MATRIX_ROWS][MATRIX_COLS])
{
    uint64_t t;

    t = state[1][0]; state[1][0] = state[1][1]; state[1][1] = state[1][2];
    state[1][2] = state[1][3]; state[1][3] = t;

    uint64_t t0 = state[2][0], t1 = state[2][1];
    state[2][0] = state[2][2]; state[2][1] = state[2][3];
    state[2][2] = t0;          state[2][3] = t1;

    t = state[3][3]; state[3][3] = state[3][2]; state[3][2] = state[3][1];
    state[3][1] = state[3][0]; state[3][0] = t;
}

/* ========================================================================== */
/*  Round functions                                                           */
/* ========================================================================== */

static void round_without_msg(uint64_t state[MATRIX_ROWS][MATRIX_COLS],
                              const uint64_t rc[MATRIX_COLS],
                              const uint64_t flag_and_bits[MATRIX_COLS])
{
    sbox_matrix(state);
    shift_rows(state);
    sbox_matrix(state);
    shift_rows(state);
    for (int c = 0; c < MATRIX_COLS; c++) {
        state[1][c] ^= rc[c];
        state[3][c] ^= flag_and_bits[c];
    }
}

static void round_with_msg(uint64_t state[MATRIX_ROWS][MATRIX_COLS],
                           const uint64_t m[16],
                           const uint64_t rc[MATRIX_COLS],
                           const uint64_t flag_and_bits[MATRIX_COLS])
{
    for (int c = 0; c < MATRIX_COLS; c++) {
        state[0][c] ^= m[c];
        state[2][c] ^= m[4 + c];
    }
    sbox_matrix(state);
    shift_rows(state);
    for (int c = 0; c < MATRIX_COLS; c++) {
        state[0][c] ^= m[8 + c];
        state[2][c] ^= m[12 + c];
    }
    sbox_matrix(state);
    shift_rows(state);
    for (int c = 0; c < MATRIX_COLS; c++) {
        state[1][c] ^= rc[c];
        state[3][c] ^= flag_and_bits[c];
    }
}

/* ========================================================================== */
/*  Compression function                                                      */
/*  Internal computation is identical to 1024; differs only by version value  */
/* ========================================================================== */

static void feilian_compress(uint64_t h_out[MATRIX_ROWS][MATRIX_COLS],
                             const uint64_t h_in[MATRIX_ROWS][MATRIX_COLS],
                             const uint64_t w_in[WORDS_PER_BLOCK],
                             int is_last_block,
                             const uint64_t hash_bits[2])
{
    uint64_t flag_and_bits[MATRIX_COLS];
    uint64_t version64 = FEILIAN_VERSION;
    uint64_t flag64 = is_last_block ? UINT64_C(0xFFFFFFFFFFFFFFFF)
                                    : UINT64_C(0x0000000000000000);
    flag_and_bits[0] = version64;
    flag_and_bits[1] = flag64;
    flag_and_bits[2] = hash_bits[0];
    flag_and_bits[3] = hash_bits[1];

    uint64_t h_initial[MATRIX_ROWS][MATRIX_COLS];
    copy_state(h_initial, h_in);
    copy_state(h_out, h_in);

    const uint64_t *rc_groups[5] = { RC1, RC2, RC3, RC4, RC1 };

    for (int rnd = 0; rnd < TOTAL_ROUNDS; rnd++) {
        const uint64_t *rc = rc_groups[rnd / 4];
        if (rnd % 4 == 0) {
            round_with_msg(h_out, w_in, rc, flag_and_bits);
        } else {
            round_without_msg(h_out, rc, flag_and_bits);
        }
    }

    add_state(h_out, h_initial);
}

/* ========================================================================== */
/*  Internal hash function                                                    */
/*  Counter is 128-bit (max message length = 2^128 bits)                      */
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

    uint64_t h_curr[MATRIX_ROWS][MATRIX_COLS];
    uint64_t h_next[MATRIX_ROWS][MATRIX_COLS];
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

        uint64_t w_in[WORDS_PER_BLOCK];
        load_block(&padded[i * BLOCK_BYTES], w_in);

        int is_last = (i == num_blocks - 1) ? 1 : 0;
        uint64_t hash_bits_so_far[2];
        set_hash_bits(hash_bits_so_far, bits_acc_lo, bits_acc_mid);

        feilian_compress(h_next, h_curr, w_in, is_last, hash_bits_so_far);
        copy_state(h_curr, h_next);
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
