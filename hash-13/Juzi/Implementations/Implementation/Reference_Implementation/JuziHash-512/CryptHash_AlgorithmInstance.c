/*
 * JuziHash reference implementation (single-file, ISO C style).
 *
 * This file is self-contained. It implements the 24-round 2048-bit Juzi
 * permutation and one JuziHash instance. The public hash function accepts the
 * message length in bits; valid inputs satisfy 0 <= msg_bitlen < 2^64.
 *
 * Padding: M || 0^z || Len_1024(len), where z=(-len) mod 1024 and
 * Len_1024(len)=0^960 || enc64_be(len).
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "CryptHash_AlgorithmInstance.h"

#define JUZI_STATE_BYTES  256u
#define JUZI_BLOCK_BYTES  128u
#define JUZI512_DIGEST_BYTES   64u
#define JUZI1024_DIGEST_BYTES 128u

static uint64_t juzi_rotl64(uint64_t x, unsigned int n)
{
    return (x << n) | (x >> (64u - n));
}

static void juzi_store64_be(uint8_t b[8], uint64_t x)
{
    b[0] = (uint8_t)(x >> 56);
    b[1] = (uint8_t)(x >> 48);
    b[2] = (uint8_t)(x >> 40);
    b[3] = (uint8_t)(x >> 32);
    b[4] = (uint8_t)(x >> 24);
    b[5] = (uint8_t)(x >> 16);
    b[6] = (uint8_t)(x >>  8);
    b[7] = (uint8_t)x;
}

static uint64_t juzi_data_block_count(uint64_t bitlen)
{
    return (bitlen / 1024u) + ((bitlen % 1024u) != 0u);
}

static void juzi_make_data_block(const uint8_t *msg, uint64_t bitlen,
                                 uint64_t block_index,
                                 uint8_t block[JUZI_BLOCK_BYTES])
{
    const uint64_t start_bit = block_index * 1024u;
    uint64_t rem_bits;

    memset(block, 0, JUZI_BLOCK_BYTES);
    if (start_bit >= bitlen) {
        return;
    }

    rem_bits = bitlen - start_bit;
    if (rem_bits >= 1024u) {
        memcpy(block, msg + block_index * JUZI_BLOCK_BYTES, JUZI_BLOCK_BYTES);
        return;
    }

    if (rem_bits >= 8u) {
        const size_t full_bytes = (size_t)(rem_bits / 8u);
        memcpy(block, msg + block_index * JUZI_BLOCK_BYTES, full_bytes);
    }
    if ((rem_bits % 8u) != 0u) {
        const size_t full_bytes = (size_t)(rem_bits / 8u);
        const unsigned int keep = (unsigned int)(rem_bits % 8u);
        const uint8_t mask = (uint8_t)(0xffu << (8u - keep));
        block[full_bytes] = (uint8_t)(msg[block_index * JUZI_BLOCK_BYTES + full_bytes] & mask);
    }
}

static void juzi_make_length_block(uint64_t bitlen,
                                   uint8_t block[JUZI_BLOCK_BYTES])
{
    memset(block, 0, JUZI_BLOCK_BYTES);
    juzi_store64_be(block + JUZI_BLOCK_BYTES - 8u, bitlen);
}

static void juzi_make_zero_block(uint8_t block[JUZI_BLOCK_BYTES])
{
    memset(block, 0, JUZI_BLOCK_BYTES);
}

static const uint8_t JUZI_S4[16] = {
    9, 0, 4, 11, 13, 12, 3, 15,
    1, 10, 2, 6, 7, 5, 8, 14
};

static uint8_t juzi_sbox8(uint8_t x)
{
    const uint8_t l0 = (uint8_t)(x >> 4);
    const uint8_t r0 = (uint8_t)(x & 0x0f);
    const uint8_t r1 = (uint8_t)(l0 ^ JUZI_S4[r0]);
    const uint8_t r2 = (uint8_t)(r0 ^ JUZI_S4[r1]);
    const uint8_t r3 = (uint8_t)(r1 ^ JUZI_S4[r2]);
    return (uint8_t)((r2 << 4) | r3);
}

static uint8_t juzi_xtime(uint8_t x)
{
    return (uint8_t)((uint8_t)(x << 1) ^ (uint8_t)((x >> 7) * 0x1bu));
}

static void juzi_ref_subbytes(uint8_t s[JUZI_STATE_BYTES])
{
    size_t i;
    for (i = 0; i < JUZI_STATE_BYTES; i++) {
        s[i] = juzi_sbox8(s[i]);
    }
}

static void juzi_ref_mds_group(uint8_t *s0, uint8_t *s1, uint8_t *s2, uint8_t *s3)
{
    size_t j;
    for (j = 0; j < 32u; j++) {
        const uint8_t x0 = s0[j];
        const uint8_t x1 = s1[j];
        const uint8_t x2 = s2[j];
        const uint8_t x3 = s3[j];
        const uint8_t x0_2 = juzi_xtime(x0);
        const uint8_t x1_2 = juzi_xtime(x1);
        const uint8_t x2_2 = juzi_xtime(x2);
        const uint8_t x3_2 = juzi_xtime(x3);
        const uint8_t x0_3 = (uint8_t)(x0_2 ^ x0);
        const uint8_t x1_3 = (uint8_t)(x1_2 ^ x1);
        const uint8_t x2_3 = (uint8_t)(x2_2 ^ x2);
        const uint8_t x3_3 = (uint8_t)(x3_2 ^ x3);
        s0[j] = (uint8_t)(x0_2 ^ x1_3 ^ x2 ^ x3);
        s1[j] = (uint8_t)(x0 ^ x1_2 ^ x2_3 ^ x3);
        s2[j] = (uint8_t)(x0 ^ x1 ^ x2_2 ^ x3_3);
        s3[j] = (uint8_t)(x0_3 ^ x1 ^ x2 ^ x3_2);
    }
}

static void juzi_ref_mds(uint8_t s[JUZI_STATE_BYTES])
{
    juzi_ref_mds_group(s + 0u * 32u, s + 1u * 32u, s + 2u * 32u, s + 3u * 32u);
    juzi_ref_mds_group(s + 4u * 32u, s + 5u * 32u, s + 6u * 32u, s + 7u * 32u);
}

static void juzi_rotate_left(uint8_t *x, size_t n, size_t sh)
{
    uint8_t tmp[64];
    size_t i;
    sh %= n;
    if (sh == 0u) {
        return;
    }
    for (i = 0; i < n; i++) {
        tmp[i] = x[(i + sh) % n];
    }
    memcpy(x, tmp, n);
}

static void juzi_ref_byteperm(uint8_t s[JUZI_STATE_BYTES], unsigned int round)
{
    const unsigned int mode = round % 3u;
    size_t w, g;
    if (mode == 0u) {
        for (w = 0; w < 8u; w++) {
            const size_t sh = w % 4u;
            for (g = 0; g < 8u; g++) {
                juzi_rotate_left(s + w * 32u + g * 4u, 4u, sh);
            }
        }
    } else if (mode == 1u) {
        for (w = 0; w < 8u; w++) {
            const size_t sh = 4u * (w % 4u);
            for (g = 0; g < 2u; g++) {
                juzi_rotate_left(s + w * 32u + g * 16u, 16u, sh);
            }
        }
    } else {
        uint8_t pair[64];
        for (w = 0; w < 4u; w++) {
            const size_t sh = 16u * w;
            memcpy(pair, s + w * 32u, 32u);
            memcpy(pair + 32u, s + (w + 4u) * 32u, 32u);
            juzi_rotate_left(pair, 64u, sh);
            memcpy(s + w * 32u, pair, 32u);
            memcpy(s + (w + 4u) * 32u, pair + 32u, 32u);
        }
    }
}

static void juzi_ref_add_rc(uint8_t s[JUZI_STATE_BYTES], const uint64_t rc[8])
{
    uint8_t b[8];
    size_t j, k;
    for (j = 0; j < 8u; j++) {
        juzi_store64_be(b, rc[j]);
        for (k = 0; k < 8u; k++) {
            s[j * 8u + k] ^= b[k];
        }
    }
}

static void juzi_ref_permute(uint8_t s[JUZI_STATE_BYTES])
{
    uint64_t rc[8] = {
        UINT64_C(0x243f6a8885a308d3), UINT64_C(0x13198a2e03707344),
        UINT64_C(0xa4093822299f31d0), UINT64_C(0x082efa98ec4e6c89),
        UINT64_C(0x452821e638d01377), UINT64_C(0xbe5466cf34e90c6c),
        UINT64_C(0xc0ac29b7c97c50dd), UINT64_C(0x3f84d5b5b5470917)
    };
    unsigned int group, k;
    size_t j;

    for (group = 0; group < 6u; group++) {
        juzi_ref_add_rc(s, rc);
        for (k = 0; k < 4u; k++) {
            const unsigned int round = 4u * group + k;
            juzi_ref_subbytes(s);
            juzi_ref_mds(s);
            juzi_ref_byteperm(s, round);
        }
        for (j = 0; j < 8u; j++) {
            rc[j] = juzi_rotl64(rc[j], 3u) ^ juzi_rotl64(rc[j], 7u) ^ juzi_rotl64(rc[j], 12u);
        }
    }
}

/* Shared sponge-style message injection: xor the 1024-bit block into the rate branch
 * words S0,S1,S4,S5 and apply the 24-round permutation. */
static void juzi_update_sp(uint8_t state[JUZI_STATE_BYTES], const uint8_t block[JUZI_BLOCK_BYTES])
{
    size_t i;
    for (i = 0; i < 32u; i++) {
        state[0u * 32u + i] ^= block[0u * 32u + i];
        state[1u * 32u + i] ^= block[1u * 32u + i];
        state[4u * 32u + i] ^= block[2u * 32u + i];
        state[5u * 32u + i] ^= block[3u * 32u + i];
    }
    juzi_ref_permute(state);
}

/* Full-state DM-like update for JuziHash-1024: after the same message injection
 * and permutation, xor the old 1024-bit capacity branch into the new capacity branch. */
static void juzi_update_dm(uint8_t state[JUZI_STATE_BYTES], const uint8_t block[JUZI_BLOCK_BYTES])
{
    uint8_t old_cap[JUZI_BLOCK_BYTES];
    size_t i;
    memcpy(old_cap +  0u, state + 2u * 32u, 32u);
    memcpy(old_cap + 32u, state + 3u * 32u, 32u);
    memcpy(old_cap + 64u, state + 6u * 32u, 32u);
    memcpy(old_cap + 96u, state + 7u * 32u, 32u);

    juzi_update_sp(state, block);

    for (i = 0; i < 32u; i++) {
        state[2u * 32u + i] ^= old_cap[0u * 32u + i];
        state[3u * 32u + i] ^= old_cap[1u * 32u + i];
        state[6u * 32u + i] ^= old_cap[2u * 32u + i];
        state[7u * 32u + i] ^= old_cap[3u * 32u + i];
    }
}


void juzihash512_ref(const uint8_t *msg, uint64_t msg_bitlen,
                     uint8_t digest[JUZI512_DIGEST_BYTES])
{
    uint8_t state[JUZI_STATE_BYTES];
    uint8_t block[JUZI_BLOCK_BYTES];
    const uint64_t data_blocks = juzi_data_block_count(msg_bitlen);
    uint64_t i;

    memset(state, 0, JUZI_STATE_BYTES);
    state[0] = 0x02u;
    state[1] = 0x00u;

    /* IV injection. */
    juzi_make_zero_block(block);
    juzi_update_sp(state, block);

    for (i = 0; i < data_blocks; i++) {
        juzi_make_data_block(msg, msg_bitlen, i, block);
        juzi_update_sp(state, block);
    }

    juzi_make_length_block(msg_bitlen, block);
    juzi_update_sp(state, block);

    memcpy(digest, state, JUZI512_DIGEST_BYTES);
}

#ifdef JUZIHASH_SELFTEST
static void print_hex(const uint8_t *x, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) printf("%02x", x[i]);
    printf("\n");
}

int main(void)
{
    const uint8_t msg[] = "abc";
    uint8_t out[JUZI512_DIGEST_BYTES];
    juzihash512_ref(msg, 24u, out);
    print_hex(out, sizeof(out));
    return 0;
}
#endif


/* ICCS NGCC API wrapper. */
int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return 1;
    }
    if (digest == 0) {
        return 2;
    }
    if (msg == 0 && msg_len_bits != 0ULL) {
        return 3;
    }
    juzihash512_ref((const uint8_t *)msg, (uint64_t)msg_len_bits, (uint8_t *)digest);
    return 0;
}
