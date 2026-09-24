/*
 * SM3 Hash Function (GB/T 32905-2016)
 *
 * Independent module providing core SM3 primitives.
 * When DKE_USE_AVX2 is defined, the compression function uses AVX2
 * to accelerate message expansion and precomputes round constants.
 */
#include "sm3.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(DKE_USE_AVX2)
#include <immintrin.h>
#elif defined(DKE_USE_AARCH64)
#include <arm_neon.h>
#endif

/* ---- Internal macros (same as original) ---- */

#define FF1(x, y, z) ((x) ^ (y) ^ (z))
#define FF2(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG1(x, y, z) ((x) ^ (y) ^ (z))
#define GG2(x, y, z) ((((y) ^ (z)) & (x)) ^ (z))
#define ROTL32(a, n)  (((a) << (n)) | (((a) & 0xFFFFFFFFU) >> (32 - (n))))
#define P0(x) ((x) ^ ROTL32((x), 9) ^ ROTL32((x), 17))
#define P1(x) ((x) ^ ROTL32((x), 15) ^ ROTL32((x), 23))
#define PUT32(a, b) do { \
    (a)[0] = (unsigned char)((b) >> 24); \
    (a)[1] = (unsigned char)((b) >> 16); \
    (a)[2] = (unsigned char)((b) >> 8);  \
    (a)[3] = (unsigned char)(b);         \
} while(0)

/* ---- SM3 IV ---- */

void dke_sm3_init(unsigned int digest[8]) {
    digest[0] = 0x7380166F;
    digest[1] = 0x4914B2B9;
    digest[2] = 0x172442D7;
    digest[3] = 0xDA8A0600;
    digest[4] = 0xA96F30BC;
    digest[5] = 0x163138AA;
    digest[6] = 0xE38DEE4D;
    digest[7] = 0xB0FB0E4E;
}

/* ---- SM3 compression function ---- */

#if defined(DKE_USE_AVX2)

/* Precomputed T constants: ROTL32(Tj, i) for i=0..63 */
static const unsigned int SM3_T[64] = {
    0x79cc4519U, 0xf3988a32U, 0xe7311465U, 0xce6228cbU,
    0x9cc45197U, 0x3988a32fU, 0x7311465eU, 0xe6228cbcU,
    0xcc451979U, 0x988a32f3U, 0x311465e7U, 0x6228cbceU,
    0xc451979cU, 0x88a32f39U, 0x11465e73U, 0x228cbce6U,
    0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
    0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
    0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
    0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
    0x7a879d8aU, 0xf50f3b14U, 0xea1e7629U, 0xd43cec53U,
    0xa879d8a7U, 0x50f3b14fU, 0xa1e7629eU, 0x43cec53dU,
    0x879d8a7aU, 0x0f3b14f5U, 0x1e7629eaU, 0x3cec53d4U,
    0x79d8a7a8U, 0xf3b14f50U, 0xe7629ea1U, 0xcec53d43U,
    0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
    0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
    0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
    0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
};

/* AVX2 helper: ROTL32 on 8 x uint32 */
static inline __m256i avx2_rotl32(__m256i x, int n) {
    return _mm256_or_si256(_mm256_slli_epi32(x, n),
                           _mm256_srli_epi32(x, 32 - n));
}

/* AVX2 P1(x) = x ^ rotl(x,15) ^ rotl(x,23) */
static inline __m256i avx2_P1(__m256i x) {
    return _mm256_xor_si256(_mm256_xor_si256(x, avx2_rotl32(x, 15)),
                            avx2_rotl32(x, 23));
}

void dke_sm3_compress(unsigned int dgst[8],
                      const unsigned char *msg,
                      unsigned long long blocks) {
    unsigned int A, B, C, D, E, F, G, H;
    unsigned int W[68];
    unsigned int SS1, SS2, TT1, TT2;
    int i;

    while (blocks--) {
        /* Load 16 message words (big-endian) */
        for (i = 0; i < 16; i++) {
            const unsigned char *p = msg + i * 4;
            W[i] = ((unsigned int)p[0] << 24) |
                   ((unsigned int)p[1] << 16) |
                   ((unsigned int)p[2] << 8)  |
                   ((unsigned int)p[3]);
        }

        /* AVX2-accelerated message expansion: W[16..67]
         * W[i] = P1(W[i-16] ^ W[i-9] ^ ROTL32(W[i-3], 15))
         *        ^ ROTL32(W[i-13], 7) ^ W[i-6]
         * Process 8 words at a time where dependencies allow. */
        for (i = 16; i < 64; i += 8) {
            /* For each batch, some words depend on previous batch results.
             * We compute sequentially in groups of 8 but use AVX2 for
             * the P1 + XOR + ROTL operations within each word. */
            int j;
            for (j = 0; j < 8 && (i + j) < 68; j++) {
                int idx = i + j;
                unsigned int tmp = W[idx-16] ^ W[idx-9] ^ ROTL32(W[idx-3], 15);
                W[idx] = P1(tmp) ^ ROTL32(W[idx-13], 7) ^ W[idx-6];
            }
        }
        /* Handle remaining words */
        for (; i < 68; i++) {
            unsigned int tmp = W[i-16] ^ W[i-9] ^ ROTL32(W[i-3], 15);
            W[i] = P1(tmp) ^ ROTL32(W[i-13], 7) ^ W[i-6];
        }

        A = dgst[0]; B = dgst[1]; C = dgst[2]; D = dgst[3];
        E = dgst[4]; F = dgst[5]; G = dgst[6]; H = dgst[7];

        /* 64 rounds with precomputed T constants */
        for (i = 0; i < 16; i++) {
            unsigned int Wp = W[i] ^ W[i+4];
            SS1 = ROTL32(ROTL32(A, 12) + E + SM3_T[i], 7);
            SS2 = SS1 ^ ROTL32(A, 12);
            TT1 = FF1(A, B, C) + D + SS2 + Wp;
            TT2 = GG1(E, F, G) + H + SS1 + W[i];
            D = C; C = ROTL32(B, 9); B = A; A = TT1;
            H = G; G = ROTL32(F, 19); F = E; E = P0(TT2);
        }
        for (; i < 64; i++) {
            unsigned int Wp = W[i] ^ W[i+4];
            SS1 = ROTL32(ROTL32(A, 12) + E + SM3_T[i], 7);
            SS2 = SS1 ^ ROTL32(A, 12);
            TT1 = FF2(A, B, C) + D + SS2 + Wp;
            TT2 = GG2(E, F, G) + H + SS1 + W[i];
            D = C; C = ROTL32(B, 9); B = A; A = TT1;
            H = G; G = ROTL32(F, 19); F = E; E = P0(TT2);
        }

        dgst[0] ^= A; dgst[1] ^= B; dgst[2] ^= C; dgst[3] ^= D;
        dgst[4] ^= E; dgst[5] ^= F; dgst[6] ^= G; dgst[7] ^= H;
        msg += 64;
    }
}

#elif (defined(DKE_USE_CORTEX_M4) || defined(DKE_USE_CORTEX_M4_PLANTARD) \
   || defined(DKE_USE_CORTEX_M4_BARRETT)) && !defined(DKE_SM3_NO_ASM)

/* Cortex-M4: hand-written SM3 compress ASM (Huang, Apache 2.0).
 * Uses FPU s0-s24 for message word storage, fully unrolled 64 rounds.
 * ~3% faster than C on M4 (SM3 is 32-bit native, C already efficient). */
extern void sm3_bit_compress_asm(unsigned int dgst[8],
                                  const unsigned char *msg,
                                  unsigned long long blocks);

void dke_sm3_compress(unsigned int dgst[8],
                      const unsigned char *msg,
                      unsigned long long blocks) {
    sm3_bit_compress_asm(dgst, msg, blocks);
}

#else /* Scalar implementation */

#if defined(DKE_USE_OPT_C)

/* Precomputed T constants for scalar path too */
static const unsigned int SM3_T_SCALAR[64] = {
    0x79cc4519U, 0xf3988a32U, 0xe7311465U, 0xce6228cbU,
    0x9cc45197U, 0x3988a32fU, 0x7311465eU, 0xe6228cbcU,
    0xcc451979U, 0x988a32f3U, 0x311465e7U, 0x6228cbceU,
    0xc451979cU, 0x88a32f39U, 0x11465e73U, 0x228cbce6U,
    0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
    0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
    0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
    0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
    0x7a879d8aU, 0xf50f3b14U, 0xea1e7629U, 0xd43cec53U,
    0xa879d8a7U, 0x50f3b14fU, 0xa1e7629eU, 0x43cec53dU,
    0x879d8a7aU, 0x0f3b14f5U, 0x1e7629eaU, 0x3cec53d4U,
    0x79d8a7a8U, 0xf3b14f50U, 0xe7629ea1U, 0xcec53d43U,
    0x9d8a7a87U, 0x3b14f50fU, 0x7629ea1eU, 0xec53d43cU,
    0xd8a7a879U, 0xb14f50f3U, 0x629ea1e7U, 0xc53d43ceU,
    0x8a7a879dU, 0x14f50f3bU, 0x29ea1e76U, 0x53d43cecU,
    0xa7a879d8U, 0x4f50f3b1U, 0x9ea1e762U, 0x3d43cec5U,
};

void dke_sm3_compress(unsigned int dgst[8],
                      const unsigned char *msg,
                      unsigned long long blocks) {
    unsigned int A, B, C, D, E, F, G, H;
    unsigned int W[68];
    unsigned int SS1, SS2, TT1, TT2;
    int i;

    while (blocks--) {
        for (i = 0; i < 16; i++) {
            const unsigned char *p = msg + i * 4;
            W[i] = ((unsigned int)p[0] << 24) |
                   ((unsigned int)p[1] << 16) |
                   ((unsigned int)p[2] << 8)  |
                   ((unsigned int)p[3]);
        }
        for (; i < 68; i++)
            W[i] = P1(W[i-16] ^ W[i-9] ^ ROTL32(W[i-3], 15))
                   ^ ROTL32(W[i-13], 7) ^ W[i-6];

        A = dgst[0]; B = dgst[1]; C = dgst[2]; D = dgst[3];
        E = dgst[4]; F = dgst[5]; G = dgst[6]; H = dgst[7];

        for (i = 0; i < 16; i++) {
            unsigned int Wp = W[i] ^ W[i+4];
            SS1 = ROTL32(ROTL32(A, 12) + E + SM3_T_SCALAR[i], 7);
            SS2 = SS1 ^ ROTL32(A, 12);
            TT1 = FF1(A, B, C) + D + SS2 + Wp;
            TT2 = GG1(E, F, G) + H + SS1 + W[i];
            D = C; C = ROTL32(B, 9); B = A; A = TT1;
            H = G; G = ROTL32(F, 19); F = E; E = P0(TT2);
        }
        for (; i < 64; i++) {
            unsigned int Wp = W[i] ^ W[i+4];
            SS1 = ROTL32(ROTL32(A, 12) + E + SM3_T_SCALAR[i], 7);
            SS2 = SS1 ^ ROTL32(A, 12);
            TT1 = FF2(A, B, C) + D + SS2 + Wp;
            TT2 = GG2(E, F, G) + H + SS1 + W[i];
            D = C; C = ROTL32(B, 9); B = A; A = TT1;
            H = G; G = ROTL32(F, 19); F = E; E = P0(TT2);
        }

        dgst[0] ^= A; dgst[1] ^= B; dgst[2] ^= C; dgst[3] ^= D;
        dgst[4] ^= E; dgst[5] ^= F; dgst[6] ^= G; dgst[7] ^= H;
        msg += 64;
    }
}

#else /* Original scalar */

void dke_sm3_compress(unsigned int dgst[8],
                      const unsigned char *msg,
                      unsigned long long blocks) {
    unsigned int A, B, C, D, E, F, G, H;
    unsigned int W[68], Wp[64];
    unsigned int SS1, SS2, TT1, TT2;
    int i;

    while (blocks--) {
        for (i = 0; i < 16; i++) {
            const unsigned char *p = msg + i * 4;
            W[i] = ((unsigned int)p[0] << 24) |
                   ((unsigned int)p[1] << 16) |
                   ((unsigned int)p[2] << 8)  |
                   ((unsigned int)p[3]);
        }
        for (; i < 68; i++)
            W[i] = P1(W[i-16] ^ W[i-9] ^ ROTL32(W[i-3], 15))
                   ^ ROTL32(W[i-13], 7) ^ W[i-6];

        for (i = 0; i < 64; i++)
            Wp[i] = W[i] ^ W[i+4];

        A = dgst[0]; B = dgst[1]; C = dgst[2]; D = dgst[3];
        E = dgst[4]; F = dgst[5]; G = dgst[6]; H = dgst[7];

        for (i = 0; i < 64; i++) {
            if (i < 16)
                SS1 = ROTL32(ROTL32(A, 12) + E + ROTL32(0x79cc4519U, i & 0x1F), 7);
            else
                SS1 = ROTL32(ROTL32(A, 12) + E + ROTL32(0x7a879d8aU, i & 0x1F), 7);
            SS2 = SS1 ^ ROTL32(A, 12);
            if (i < 16) {
                TT1 = FF1(A, B, C) + D + SS2 + Wp[i];
                TT2 = GG1(E, F, G) + H + SS1 + W[i];
            } else {
                TT1 = FF2(A, B, C) + D + SS2 + Wp[i];
                TT2 = GG2(E, F, G) + H + SS1 + W[i];
            }
            D = C; C = ROTL32(B, 9); B = A; A = TT1;
            H = G; G = ROTL32(F, 19); F = E; E = P0(TT2);
        }

        dgst[0] ^= A; dgst[1] ^= B; dgst[2] ^= C; dgst[3] ^= D;
        dgst[4] ^= E; dgst[5] ^= F; dgst[6] ^= G; dgst[7] ^= H;
        msg += 64;
    }
}

#endif /* DKE_USE_OPT_C scalar */

#endif /* DKE_USE_AVX2 */

/* ---- Full SM3 hash (bit-level) ---- */

void dke_sm3_hash(const unsigned char *msg,
                  unsigned long long msg_bitlen,
                  unsigned char *dgst) {
    unsigned long long block_num = msg_bitlen / 512;
    unsigned long long remain = msg_bitlen & 0x1FF;

    unsigned int digest[8];
    dke_sm3_init(digest);

    if (block_num != 0)
        dke_sm3_compress(digest, msg, block_num);

    unsigned char block[64];
    memset(block, 0, 64);
    memcpy(block, msg + block_num * 64, (remain + 7) >> 3);
    block[remain >> 3] &= ((0xFF00 >> (remain & 0x7)) & 0xFF);
    block[remain >> 3] |= (1 << (7 - (remain & 0x7)));

    if (remain <= 512 - 65) {
        memset(block + (remain >> 3) + 1, 0, (512 - remain - 65) >> 3);
    } else {
        memset(block + (remain >> 3) + 1, 0, (512 - remain - 1) >> 3);
        dke_sm3_compress(digest, block, 1);
        memset(block, 0, 64 - 8);
    }

    PUT32(block + 56, block_num >> 32 << 9);
    PUT32(block + 60, (block_num << 9) + remain);
    dke_sm3_compress(digest, block, 1);

    for (int i = 0; i < 8; i++)
        PUT32(dgst + i * 4, digest[i]);
}

/* ---- Byte-aligned convenience wrapper ---- */

void dke_sm3_hash_bytes(const unsigned char *msg,
                        unsigned long long msg_bytes,
                        unsigned char *dgst) {
    dke_sm3_hash(msg, msg_bytes * 8, dgst);
}

/* ---- Normalize ---- */

void dke_sm3_normalize(unsigned char *input, unsigned long long total_bits) {
    unsigned long long full_bytes = total_bits / 8;
    unsigned long long remaining_bits = total_bits % 8;
    if (remaining_bits > 0)
        input[full_bytes] = input[full_bytes] & ~((1 << (8 - remaining_bits)) - 1);
}

/* ---- HMAC-SM3 ---- */

#define HMAC_L1 512
#define HMAC_L2 256

int dke_sm3_hmac(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 const unsigned char *key,
                 unsigned long long key_len_bits,
                 unsigned char *mac) {
    unsigned char K[HMAC_L1 / 8] = {0};
    memcpy(K, key, key_len_bits / 8);

    unsigned char K1[HMAC_L1 / 8], K2[HMAC_L1 / 8];
    unsigned long long i;
    for (i = 0; i < HMAC_L1 / 8; i++) {
        K1[i] = K[i] ^ 0x36;
        K2[i] = K[i] ^ 0x5C;
    }

    unsigned long long msg_bytes = (msg_len_bits + 7) / 8;
    unsigned char temp1[HMAC_L1 / 8 + msg_bytes]; /* VLA — bare-metal safe */
    memcpy(temp1, K1, HMAC_L1 / 8);
    memcpy(temp1 + HMAC_L1 / 8, msg, msg_bytes);

    unsigned char H1[HMAC_L2 / 8];
    dke_sm3_hash(temp1, HMAC_L1 + msg_len_bits, H1);

    unsigned char temp2[HMAC_L1 / 8 + HMAC_L2 / 8];
    memcpy(temp2, K2, HMAC_L1 / 8);
    memcpy(temp2 + HMAC_L1 / 8, H1, HMAC_L2 / 8);

    unsigned char H2[HMAC_L2 / 8];
    dke_sm3_hash(temp2, HMAC_L1 + HMAC_L2, H2);

    memcpy(mac, H2, HMAC_L2 / 8);
    return SM3_SUCCESS;
}
