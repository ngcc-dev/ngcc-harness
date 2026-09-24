
/*
 * Single-file AVX2 implementation of one JuziHash instance.
 *
 * This file has no external header dependency.  It implements the 24-round
 * 2048-bit Juzi permutation, the required padding rule, and one one-shot hash
 * function.  The initial state H^(0) is stored directly as a 2048-bit constant,
 * so the implementation does not spend one permutation call deriving H^(0).
 *
 * Public input length: 0 <= msg_bitlen < 2^64.  Message bits are interpreted in
 * big-endian bit order inside each byte: bit 0 is the MSB of msg[0].  If
 * msg_bitlen is not byte-aligned, unused low bits of the last byte are ignored.
 *
 * Build examples:
 *   MSVC: cl /O2 /W3 /arch:AVX2 /DJUZIHASH_SELFTEST <this_file>.c
 *   GCC : gcc -O3 -mavx2 -Wall -Wextra -DJUZIHASH_SELFTEST <this_file>.c -o test
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <immintrin.h>
#include "CryptHash_AlgorithmInstance.h"

#define JUZI_STATE_BYTES       256u
#define JUZI_BLOCK_BYTES       128u
#define JUZI512_DIGEST_BYTES    64u
#define JUZI1024_DIGEST_BYTES  128u

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

static const uint8_t JUZI_SHUF4[4][32] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
    { 1, 2, 3, 0, 5, 6, 7, 4, 9,10,11, 8,13,14,15,12,17,18,19,16,21,22,23,20,25,26,27,24,29,30,31,28},
    { 2, 3, 0, 1, 6, 7, 4, 5,10,11, 8, 9,14,15,12,13,18,19,16,17,22,23,20,21,26,27,24,25,30,31,28,29},
    { 3, 0, 1, 2, 7, 4, 5, 6,11, 8, 9,10,15,12,13,14,19,16,17,18,23,20,21,22,27,24,25,26,31,28,29,30}
};

static const uint8_t JUZI_SHUF16[4][32] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
    { 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 1, 2, 3,20,21,22,23,24,25,26,27,28,29,30,31,16,17,18,19},
    { 8, 9,10,11,12,13,14,15, 0, 1, 2, 3, 4, 5, 6, 7,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23},
    {12,13,14,15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,28,29,30,31,16,17,18,19,20,21,22,23,24,25,26,27}
};

static __m256i juzi_loadu256(const void *p)
{
    return _mm256_loadu_si256((const __m256i *)p);
}

static void juzi_storeu256(void *p, __m256i x)
{
    _mm256_storeu_si256((__m256i *)p, x);
}

static __m256i juzi_avx2_xtime(__m256i x)
{
    const __m256i zero = _mm256_setzero_si256();
    const __m256i poly = _mm256_set1_epi8(0x1b);
    const __m256i high = _mm256_cmpgt_epi8(zero, x);
    const __m256i dbl = _mm256_add_epi8(x, x);
    return _mm256_xor_si256(dbl, _mm256_and_si256(high, poly));
}

static __m256i juzi_avx2_s4(__m256i x)
{
    const __m256i tbl = _mm256_setr_epi8(
        9,0,4,11,13,12,3,15,1,10,2,6,7,5,8,14,
        9,0,4,11,13,12,3,15,1,10,2,6,7,5,8,14);
    return _mm256_shuffle_epi8(tbl, x);
}

static __m256i juzi_avx2_sbox8(__m256i x)
{
    const __m256i mask0f = _mm256_set1_epi8(0x0f);
    const __m256i maskf0 = _mm256_set1_epi8((char)0xf0);
    const __m256i l0 = _mm256_and_si256(_mm256_srli_epi16(x, 4), mask0f);
    const __m256i r0 = _mm256_and_si256(x, mask0f);
    const __m256i r1 = _mm256_xor_si256(l0, juzi_avx2_s4(r0));
    const __m256i r2 = _mm256_xor_si256(r0, juzi_avx2_s4(r1));
    const __m256i r3 = _mm256_xor_si256(r1, juzi_avx2_s4(r2));
    const __m256i high = _mm256_and_si256(_mm256_slli_epi16(r2, 4), maskf0);
    return _mm256_or_si256(high, r3);
}

static void juzi_avx2_subbytes(__m256i s[8])
{
    unsigned int i;
    for (i = 0; i < 8u; i++) {
        s[i] = juzi_avx2_sbox8(s[i]);
    }
}

static void juzi_avx2_mds_group(__m256i *x0, __m256i *x1, __m256i *x2, __m256i *x3)
{
    const __m256i a0 = *x0;
    const __m256i a1 = *x1;
    const __m256i a2 = *x2;
    const __m256i a3 = *x3;
    const __m256i t = _mm256_xor_si256(_mm256_xor_si256(a0, a1), _mm256_xor_si256(a2, a3));
    *x0 = _mm256_xor_si256(_mm256_xor_si256(a0, t), juzi_avx2_xtime(_mm256_xor_si256(a0, a1)));
    *x1 = _mm256_xor_si256(_mm256_xor_si256(a1, t), juzi_avx2_xtime(_mm256_xor_si256(a1, a2)));
    *x2 = _mm256_xor_si256(_mm256_xor_si256(a2, t), juzi_avx2_xtime(_mm256_xor_si256(a2, a3)));
    *x3 = _mm256_xor_si256(_mm256_xor_si256(a3, t), juzi_avx2_xtime(_mm256_xor_si256(a3, a0)));
}

static void juzi_avx2_mds(__m256i s[8])
{
    juzi_avx2_mds_group(&s[0], &s[1], &s[2], &s[3]);
    juzi_avx2_mds_group(&s[4], &s[5], &s[6], &s[7]);
}

static void juzi_avx2_rotate_pair(__m256i *a, __m256i *b, unsigned int which)
{
    __m256i na;
    __m256i nb;
    switch (which & 3u) {
    case 0u:
        return;
    case 1u:
        na = _mm256_permute2x128_si256(*a, *b, 0x21);
        nb = _mm256_permute2x128_si256(*a, *b, 0x03);
        break;
    case 2u:
        na = *b;
        nb = *a;
        break;
    default:
        na = _mm256_permute2x128_si256(*a, *b, 0x03);
        nb = _mm256_permute2x128_si256(*a, *b, 0x21);
        break;
    }
    *a = na;
    *b = nb;
}

static void juzi_avx2_byteperm(__m256i s[8], unsigned int round)
{
    const unsigned int mode = round % 3u;
    unsigned int i;
    if (mode == 0u) {
        for (i = 0; i < 8u; i++) {
            s[i] = _mm256_shuffle_epi8(s[i], juzi_loadu256(JUZI_SHUF4[i & 3u]));
        }
    } else if (mode == 1u) {
        for (i = 0; i < 8u; i++) {
            s[i] = _mm256_shuffle_epi8(s[i], juzi_loadu256(JUZI_SHUF16[i & 3u]));
        }
    } else {
        for (i = 0; i < 4u; i++) {
            juzi_avx2_rotate_pair(&s[i], &s[i + 4u], i);
        }
    }
}

static void juzi_avx2_add_rc(__m256i s[8], const uint64_t rc[8])
{
    uint8_t b[64];
    size_t j;
    for (j = 0; j < 8u; j++) {
        juzi_store64_be(b + j * 8u, rc[j]);
    }
    s[0] = _mm256_xor_si256(s[0], juzi_loadu256(b));
    s[1] = _mm256_xor_si256(s[1], juzi_loadu256(b + 32u));
}

static void juzi_avx2_permute(__m256i s[8])
{
    uint64_t rc[8] = {
        UINT64_C(0x243f6a8885a308d3), UINT64_C(0x13198a2e03707344),
        UINT64_C(0xa4093822299f31d0), UINT64_C(0x082efa98ec4e6c89),
        UINT64_C(0x452821e638d01377), UINT64_C(0xbe5466cf34e90c6c),
        UINT64_C(0xc0ac29b7c97c50dd), UINT64_C(0x3f84d5b5b5470917)
    };
    unsigned int group;
    unsigned int k;
    size_t j;
    for (group = 0; group < 6u; group++) {
        juzi_avx2_add_rc(s, rc);
        for (k = 0; k < 4u; k++) {
            const unsigned int round = 4u * group + k;
            juzi_avx2_subbytes(s);
            juzi_avx2_mds(s);
            juzi_avx2_byteperm(s, round);
        }
        for (j = 0; j < 8u; j++) {
            rc[j] = juzi_rotl64(rc[j], 3u) ^ juzi_rotl64(rc[j], 7u) ^ juzi_rotl64(rc[j], 12u);
        }
    }
}

static void juzi_avx2_load_state(__m256i s[8], const uint8_t bytes[JUZI_STATE_BYTES])
{
    unsigned int i;
    for (i = 0; i < 8u; i++) {
        s[i] = juzi_loadu256(bytes + 32u * i);
    }
}

static void juzi_avx2_store_state(uint8_t bytes[JUZI_STATE_BYTES], const __m256i s[8])
{
    unsigned int i;
    for (i = 0; i < 8u; i++) {
        juzi_storeu256(bytes + 32u * i, s[i]);
    }
}

static void juzi_avx2_update_sp(__m256i s[8], const uint8_t block[JUZI_BLOCK_BYTES])
{
    s[0] = _mm256_xor_si256(s[0], juzi_loadu256(block + 0u * 32u));
    s[1] = _mm256_xor_si256(s[1], juzi_loadu256(block + 1u * 32u));
    s[4] = _mm256_xor_si256(s[4], juzi_loadu256(block + 2u * 32u));
    s[5] = _mm256_xor_si256(s[5], juzi_loadu256(block + 3u * 32u));
    juzi_avx2_permute(s);
}

static void juzi_avx2_update_dm(__m256i s[8], const uint8_t block[JUZI_BLOCK_BYTES])
{
    const __m256i old2 = s[2];
    const __m256i old3 = s[3];
    const __m256i old6 = s[6];
    const __m256i old7 = s[7];
    juzi_avx2_update_sp(s, block);
    s[2] = _mm256_xor_si256(s[2], old2);
    s[3] = _mm256_xor_si256(s[3], old3);
    s[6] = _mm256_xor_si256(s[6], old6);
    s[7] = _mm256_xor_si256(s[7], old7);
}

static const uint8_t JUZI1024_H0[JUZI_STATE_BYTES] = {
    0x66u, 0xfau, 0x32u, 0x43u, 0xc9u, 0xc1u, 0x44u, 0xbfu, 0xdcu, 0xeeu, 0xcfu, 0xd1u, 0xe7u, 0x89u, 0x43u, 0x2du,
    0x4au, 0x10u, 0x29u, 0x29u, 0xd5u, 0x67u, 0x0eu, 0x9fu, 0x1du, 0x13u, 0x0au, 0x72u, 0xe6u, 0x7du, 0xcdu, 0xadu,
    0x40u, 0xd7u, 0xb9u, 0x41u, 0xd1u, 0x92u, 0x7au, 0x1du, 0x28u, 0x16u, 0x54u, 0xe2u, 0xc4u, 0x41u, 0x8bu, 0x03u,
    0xf8u, 0xb7u, 0xd0u, 0x44u, 0xb8u, 0x96u, 0xe5u, 0x30u, 0x5au, 0x38u, 0x3eu, 0x09u, 0xe2u, 0x2au, 0x44u, 0x35u,
    0x04u, 0x9eu, 0x74u, 0x1cu, 0x2au, 0x29u, 0x38u, 0x39u, 0x74u, 0x64u, 0x5eu, 0x2du, 0x1cu, 0x1du, 0x18u, 0xbbu,
    0x7bu, 0x39u, 0x80u, 0xb4u, 0xefu, 0x51u, 0x1fu, 0xeeu, 0xc6u, 0x30u, 0xf5u, 0xa6u, 0x42u, 0xb3u, 0x80u, 0x45u,
    0xefu, 0xefu, 0xadu, 0xd7u, 0x7du, 0xc3u, 0x2eu, 0xd7u, 0xbdu, 0xd6u, 0xfeu, 0x8cu, 0x3cu, 0xfbu, 0xe6u, 0x5du,
    0x68u, 0x49u, 0x0du, 0x25u, 0xb6u, 0x1au, 0xffu, 0x36u, 0x8fu, 0xa4u, 0xa6u, 0x1du, 0xebu, 0x35u, 0xc1u, 0xa3u,
    0x9du, 0x1cu, 0xa8u, 0x13u, 0x52u, 0x0fu, 0xe0u, 0xf3u, 0x70u, 0x89u, 0x65u, 0xbdu, 0xd1u, 0xa0u, 0x68u, 0x06u,
    0x20u, 0xd1u, 0x7bu, 0xa3u, 0xc7u, 0x6du, 0x8bu, 0x98u, 0x17u, 0x50u, 0x82u, 0xf4u, 0xfeu, 0x9au, 0xb4u, 0x1fu,
    0xfbu, 0x85u, 0xbau, 0x3fu, 0x75u, 0x2cu, 0xccu, 0xdcu, 0x33u, 0x42u, 0xa7u, 0xa2u, 0xf7u, 0xfcu, 0x5fu, 0x79u,
    0xc9u, 0x70u, 0x53u, 0x13u, 0x29u, 0x40u, 0x4du, 0xd9u, 0xc7u, 0x91u, 0x37u, 0xd7u, 0xe3u, 0xbdu, 0xf0u, 0x5au,
    0x00u, 0xddu, 0x6au, 0x35u, 0x33u, 0xa8u, 0x5au, 0x30u, 0x71u, 0xd0u, 0x42u, 0xddu, 0x98u, 0x20u, 0x29u, 0x34u,
    0x34u, 0xf3u, 0x46u, 0x58u, 0xf4u, 0x0du, 0x96u, 0x76u, 0xbfu, 0x06u, 0x8du, 0x24u, 0x84u, 0x71u, 0x7eu, 0x22u,
    0xa5u, 0xa1u, 0xdfu, 0x28u, 0xb0u, 0xc2u, 0x8au, 0xfeu, 0xd0u, 0x4au, 0xdfu, 0xd0u, 0xdcu, 0xc4u, 0xe8u, 0xd7u,
    0x66u, 0x1au, 0xe0u, 0x40u, 0x92u, 0x05u, 0x85u, 0xb3u, 0xbau, 0x1fu, 0xe2u, 0x4du, 0x90u, 0x71u, 0x11u, 0xfcu
};

void juzihash1024_avx2(const uint8_t *msg, uint64_t msg_bitlen,
                      uint8_t digest[JUZI1024_DIGEST_BYTES])
{
    uint8_t state_bytes[JUZI_STATE_BYTES];
    uint8_t block[JUZI_BLOCK_BYTES];
    __m256i s[8];
    const uint64_t data_blocks = juzi_data_block_count(msg_bitlen);
    uint64_t i;

    memcpy(state_bytes, JUZI1024_H0, JUZI_STATE_BYTES);
    juzi_avx2_load_state(s, state_bytes);

    for (i = 0; i < data_blocks; i++) {
        juzi_make_data_block(msg, msg_bitlen, i, block);
        juzi_avx2_update_dm(s, block);
    }

    juzi_make_length_block(msg_bitlen, block);
    juzi_avx2_update_dm(s, block);

    juzi_avx2_store_state(state_bytes, s);
    memcpy(digest +  0u, state_bytes + 2u * 32u, 32u);
    memcpy(digest + 32u, state_bytes + 3u * 32u, 32u);
    memcpy(digest + 64u, state_bytes + 6u * 32u, 32u);
    memcpy(digest + 96u, state_bytes + 7u * 32u, 32u);
}

#ifdef JUZIHASH_SELFTEST
static void print_hex(const uint8_t *x, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        printf("%02x", x[i]);
    }
    printf("\n");
}

int main(void)
{
    const uint8_t msg[] = "abc";
    uint8_t out[JUZI1024_DIGEST_BYTES];
    juzihash1024_avx2(msg, 24u, out);
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
    juzihash1024_avx2((const uint8_t *)msg, (uint64_t)msg_len_bits, (uint8_t *)digest);
    return 0;
}
