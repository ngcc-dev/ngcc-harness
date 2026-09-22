/*
The software is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#include "CryptHash_AlgorithmInstance.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define NL_P 4294967291ULL
#define NL_P32 4294967291U
#define NL_MAX_MU 5
#define NL_WORDS_PER_MODULE 16
#define NL_MAX_STATE_BYTES (NL_MAX_MU * NL_WORDS_PER_MODULE * 4)
#define NL_MAX_V_BYTES 136
#define NL_MAX_K_BYTES 184
#define NL_MAX_OUTPUT_BYTES 264
#define NL_LENGTH_FIELD_BITS 64
#define NL_BLANK_ROUNDS 32

typedef struct
{
    int n;
    int k;
    int v;
    int mu;
} NL_Params;

static const uint8_t NL_SBOX[256] = {
    0xA7, 0x12, 0x9B, 0x70, 0xB9, 0xEA, 0x4C, 0x73, 0x4E, 0x14, 0x01, 0x33, 0x52, 0x31, 0x2B, 0x9C,
    0xB5, 0xF5, 0x98, 0x21, 0xF4, 0xBE, 0x0B, 0x2A, 0xBB, 0xAC, 0xEC, 0x8D, 0xE1, 0x90, 0x3A, 0x38,
    0x48, 0xD2, 0x68, 0x0F, 0xDE, 0x56, 0xE4, 0x0E, 0xE8, 0xED, 0xCD, 0xE5, 0xF1, 0x05, 0x61, 0x86,
    0xA9, 0x13, 0x22, 0x2D, 0x02, 0x9E, 0xB2, 0x7E, 0x84, 0xDC, 0x3C, 0x1E, 0x69, 0xE7, 0x8E, 0xEB,
    0xB6, 0xD8, 0xFB, 0xBF, 0xA6, 0x4A, 0x15, 0x23, 0xFD, 0xA4, 0x5F, 0x71, 0xE0, 0x8B, 0x95, 0xB8,
    0xE6, 0x08, 0x82, 0x34, 0x92, 0x65, 0x60, 0xE9, 0x8C, 0xAB, 0x10, 0xC1, 0xC4, 0x0C, 0x37, 0x4F,
    0x46, 0xD6, 0x1B, 0x30, 0x83, 0x7B, 0xE2, 0xA5, 0x93, 0x3F, 0xDD, 0xCF, 0xCB, 0xF7, 0xAD, 0xB3,
    0xD0, 0x47, 0x1A, 0xC6, 0x6A, 0xB0, 0x9D, 0x7D, 0x26, 0x64, 0x87, 0x25, 0xD5, 0x99, 0x81, 0xFA,
    0x2F, 0x28, 0xFE, 0xEE, 0x89, 0x39, 0x4D, 0xF6, 0x27, 0xF2, 0x51, 0x19, 0x18, 0xD9, 0x03, 0x7F,
    0x8A, 0xD1, 0xC0, 0x43, 0x3D, 0xC3, 0xCC, 0xC5, 0x04, 0x55, 0xB1, 0x3E, 0x58, 0xBC, 0xCE, 0xAA,
    0x07, 0x42, 0x96, 0x77, 0xD3, 0x6E, 0x88, 0x45, 0xDB, 0x1D, 0x20, 0x6F, 0x44, 0xDF, 0x66, 0xA8,
    0x32, 0x54, 0xA1, 0x0A, 0x7C, 0x63, 0x94, 0x97, 0xF0, 0x76, 0x72, 0x6B, 0xEF, 0x3B, 0x35, 0x80,
    0x57, 0x74, 0x1F, 0x36, 0xF9, 0xAF, 0x6C, 0x06, 0x53, 0x24, 0xC9, 0xF3, 0xE3, 0xC7, 0x40, 0xFC,
    0x5B, 0x16, 0x0D, 0xDA, 0x9A, 0xC8, 0x75, 0xF8, 0x91, 0xBD, 0x8F, 0x59, 0xA2, 0xAE, 0x4B, 0xA3,
    0x1C, 0xFF, 0xD7, 0x50, 0x79, 0x2E, 0x17, 0x85, 0x41, 0x9F, 0x2C, 0x5C, 0xBA, 0x7A, 0xCA, 0x6D,
    0x67, 0x11, 0xA0, 0xC2, 0xB7, 0x29, 0x00, 0x49, 0x78, 0x62, 0x5E, 0x5A, 0xB4, 0xD4, 0x09, 0x5D};

static uint32_t nl_rotl32(uint32_t x, unsigned int r)
{
    r &= 31U;
    return r ? ((x << r) | (x >> (32U - r))) : x;
}

static uint32_t nl_redp32(uint32_t x)
{
    return x >= NL_P32 ? x - NL_P32 : x;
}

static uint32_t nl_redp64(uint64_t x)
{
    return (uint32_t)(x % NL_P);
}

static uint32_t nl_load32_be(const uint8_t x[4])
{
    return ((uint32_t)x[0] << 24) | ((uint32_t)x[1] << 16) | ((uint32_t)x[2] << 8) | (uint32_t)x[3];
}

static void nl_store32_be(uint8_t out[4], uint32_t x)
{
    out[0] = (uint8_t)(x >> 24);
    out[1] = (uint8_t)(x >> 16);
    out[2] = (uint8_t)(x >> 8);
    out[3] = (uint8_t)x;
}

static uint32_t nl_linear_layer(uint32_t x, int layer)
{
    if (layer == 0)
        return x ^ nl_rotl32(x, 2) ^ nl_rotl32(x, 10) ^ nl_rotl32(x, 18) ^ nl_rotl32(x, 24);
    return x ^ nl_rotl32(x, 8) ^ nl_rotl32(x, 14) ^ nl_rotl32(x, 22) ^ nl_rotl32(x, 30);
}

static uint32_t nl_sbox_word(uint32_t x)
{
    return ((uint32_t)NL_SBOX[(x >> 24) & 0xFFU] << 24) |
           ((uint32_t)NL_SBOX[(x >> 16) & 0xFFU] << 16) |
           ((uint32_t)NL_SBOX[(x >> 8) & 0xFFU] << 8) |
           (uint32_t)NL_SBOX[x & 0xFFU];
}

static uint32_t nl_F(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3)
{
    uint32_t a = r0 + r1;
    uint32_t b = r2 ^ r3;
    uint32_t u = (a << 16) | (b >> 16);
    uint32_t v = (b << 16) | (a >> 16);
    uint32_t pl = nl_sbox_word(nl_linear_layer(u, 0));
    uint32_t pr = nl_sbox_word(nl_linear_layer(v, 1));
    return (pl ^ nl_rotl32(r1, 8)) + pr;
}

static uint32_t nl_linear_core(const uint32_t L[NL_WORDS_PER_MODULE])
{
    uint64_t x = 0;
    x += 8193ULL * (uint64_t)L[0];
    x += 16777217ULL * (uint64_t)L[4];
    x += 4194304ULL * (uint64_t)L[9];
    x += 9ULL * (uint64_t)L[13];
    x += 524288ULL * (uint64_t)L[15];
    return nl_redp64(x);
}

static void nl_offsets(int mu, int which, int *a, int *b)
{
    static const int tab3[4][2] = {{0, 1}, {1, 2}, {2, 0}, {1, 0}};
    static const int tab4[4][2] = {{0, 1}, {2, 0}, {1, 3}, {3, 2}};
    static const int tab5[4][2] = {{0, 1}, {2, 4}, {3, 1}, {4, 3}};
    const int(*tab)[2] = (mu == 3) ? tab3 : ((mu == 4) ? tab4 : tab5);
    *a = tab[which][0];
    *b = tab[which][1];
}

static int nl_mod_index(int x, int mu)
{
    x %= mu;
    return x < 0 ? x + mu : x;
}

static void nl_update(uint32_t S[NL_MAX_MU][NL_WORDS_PER_MODULE], const NL_Params *p, uint32_t round)
{
    uint32_t phi[NL_MAX_MU];
    uint32_t A[NL_MAX_MU], B[NL_MAX_MU], C[NL_MAX_MU], D[NL_MAX_MU];
    uint32_t gamma[NL_MAX_MU];
    uint32_t T[NL_MAX_MU][NL_WORDS_PER_MODULE];
    int mu = p->mu;
    int rmu = (int)(round % (uint32_t)mu);

    for (int i = 0; i < mu; i++)
    {
        uint32_t lin = nl_linear_core(S[i]);
        uint32_t nonlin = nl_F(S[i][15], S[i][10], S[i][5], S[i][0]);
        phi[i] = nl_redp64((uint64_t)lin + (uint64_t)nonlin);
        A[i] = S[i][14];
        B[i] = S[i][9];
        C[i] = S[i][6];
        D[i] = S[i][1];
    }

    for (int i = 0; i < mu; i++)
    {
        int ax, bx, ay, by, az, bz, aw, bw;
        nl_offsets(mu, 0, &ax, &bx);
        nl_offsets(mu, 1, &ay, &by);
        nl_offsets(mu, 2, &az, &bz);
        nl_offsets(mu, 3, &aw, &bw);
        uint32_t X = A[nl_mod_index(i + rmu + ax, mu)] ^ nl_rotl32(B[nl_mod_index(i + rmu + bx, mu)], 7);
        uint32_t Y = B[nl_mod_index(i + rmu + ay, mu)] + C[nl_mod_index(i + rmu + by, mu)];
        uint32_t Z = C[nl_mod_index(i + rmu + az, mu)] ^ nl_rotl32(D[nl_mod_index(i + rmu + bz, mu)], 11);
        uint32_t W = D[nl_mod_index(i + rmu + aw, mu)] + A[nl_mod_index(i + rmu + bw, mu)];
        uint32_t kappa = nl_rotl32(0x9e3779b9U ^ round ^ (uint32_t)i, (unsigned int)((round + (uint32_t)i) & 31U));
        gamma[i] = nl_F(X, Y, Z, W) ^ kappa;
    }

    for (int i = 0; i < mu; i++)
    {
        for (int j = 0; j < NL_WORDS_PER_MODULE - 1; j++)
            T[i][j] = S[i][j + 1];
        T[i][3] = nl_redp32(S[i][4] ^ nl_rotl32(gamma[nl_mod_index(i + (mu == 3 ? 1 : 1), mu)], 5));
        T[i][7] = nl_redp32(S[i][8] ^ nl_rotl32(gamma[nl_mod_index(i + 2, mu)], 13));
        T[i][11] = nl_redp32(S[i][12] ^ nl_rotl32(gamma[nl_mod_index(i + (mu == 3 ? 1 : 3), mu)], 21));
        T[i][15] = nl_redp32(phi[i] ^ gamma[i]);
    }

    for (int i = 0; i < mu; i++)
        for (int j = 0; j < NL_WORDS_PER_MODULE; j++)
            S[i][j] = T[i][j];
}

static int nl_params_from_digest(int digest_len_bits, NL_Params *p)
{
    if (digest_len_bits == 512)
    {
        p->n = 512;
        p->k = 960;
        p->v = 576;
        p->mu = 3;
        return 0;
    }
    if (digest_len_bits == 768)
    {
        p->n = 768;
        p->k = 1216;
        p->v = 832;
        p->mu = 4;
        return 0;
    }
    if (digest_len_bits == 1024)
    {
        p->n = 1024;
        p->k = 1472;
        p->v = 1088;
        p->mu = 5;
        return 0;
    }
    return -1;
}

static void nl_make_iv(const NL_Params *p, uint8_t V[NL_MAX_V_BYTES])
{
    int words = p->v / 32;
    memset(V, 0, (size_t)p->v / 8U);
    for (int i = 0; i < words; i++)
    {
        uint32_t x = (uint32_t)(0x9e3779b9U * (uint32_t)(i + 1));
        x = nl_rotl32(x, (unsigned int)i) ^ nl_rotl32(0x6a09e667U ^ 1024U, (unsigned int)((7 * i) & 31));
        nl_store32_be(V + 4 * i, x);
    }
}

static void nl_xor_block_constant(uint8_t V[NL_MAX_V_BYTES], size_t v_bytes, unsigned long long index)
{
    size_t off = v_bytes - 8U;
    uint32_t hi = (uint32_t)(index >> 32);
    uint32_t lo = (uint32_t)index;
    V[off + 0] ^= (uint8_t)(hi >> 24);
    V[off + 1] ^= (uint8_t)(hi >> 16);
    V[off + 2] ^= (uint8_t)(hi >> 8);
    V[off + 3] ^= (uint8_t)hi;
    V[off + 4] ^= (uint8_t)(lo >> 24);
    V[off + 5] ^= (uint8_t)(lo >> 16);
    V[off + 6] ^= (uint8_t)(lo >> 8);
    V[off + 7] ^= (uint8_t)lo;
}

static void nl_init_state(uint32_t S[NL_MAX_MU][NL_WORDS_PER_MODULE],
                          const NL_Params *p,
                          const uint8_t V[NL_MAX_V_BYTES],
                          const uint8_t K[NL_MAX_K_BYTES])
{
    uint8_t material[NL_MAX_STATE_BYTES];
    size_t v_bytes = (size_t)p->v / 8U;
    size_t k_bytes = (size_t)p->k / 8U;
    int words = p->mu * NL_WORDS_PER_MODULE;
    memcpy(material, V, v_bytes);
    memcpy(material + v_bytes, K, k_bytes);
    for (int w = 0; w < words; w++)
        S[w / NL_WORDS_PER_MODULE][w % NL_WORDS_PER_MODULE] = nl_load32_be(material + 4 * w);
}

static void nl_prng(const NL_Params *p,
                    const uint8_t V[NL_MAX_V_BYTES],
                    const uint8_t K[NL_MAX_K_BYTES],
                    uint8_t *out,
                    size_t out_len)
{
    uint32_t S[NL_MAX_MU][NL_WORDS_PER_MODULE];
    size_t produced = 0;
    uint32_t tau = 0;

    nl_init_state(S, p, V, K);
    for (uint32_t r = 0; r < NL_BLANK_ROUNDS; r++)
        nl_update(S, p, r);

    while (produced < out_len)
    {
        nl_update(S, p, NL_BLANK_ROUNDS + tau);
        for (int i = 0; i < p->mu && produced < out_len; i++)
        {
            uint32_t left = S[i][13] ^ S[(i + 1) % p->mu][8];
            uint32_t right = S[(i + 2) % p->mu][4] ^ S[(i + 3) % p->mu][2];
            uint32_t word = left + right;
            uint8_t tmp[4];
            size_t take = out_len - produced;
            if (take > 4U)
                take = 4U;
            nl_store32_be(tmp, word);
            memcpy(out + produced, tmp, take);
            produced += take;
        }
        tau++;
    }
}

static void nl_set_bit(uint8_t *x, unsigned int bit_in_block)
{
    x[bit_in_block >> 3] |= (uint8_t)(0x80U >> (bit_in_block & 7U));
}

static unsigned long long nl_padding_zero_bits(unsigned long long msg_len_bits, int k)
{
    unsigned long long used = (msg_len_bits + 1ULL + NL_LENGTH_FIELD_BITS) % (unsigned long long)k;
    return used ? (unsigned long long)k - used : 0ULL;
}

static void nl_build_padded_block(const NL_Params *p,
                                  const unsigned char *msg,
                                  unsigned long long msg_len_bits,
                                  unsigned long long zero_bits,
                                  unsigned long long block_index,
                                  uint8_t block[NL_MAX_K_BYTES])
{
    unsigned long long block_start = block_index * (unsigned long long)p->k;
    unsigned long long block_end = block_start + (unsigned long long)p->k;
    unsigned long long length_start = msg_len_bits + 1ULL + zero_bits;
    size_t k_bytes = (size_t)p->k / 8U;

    memset(block, 0, k_bytes);

    if (block_start < msg_len_bits)
    {
        unsigned long long msg_bits_here = msg_len_bits - block_start;
        if (msg_bits_here > (unsigned long long)p->k)
            msg_bits_here = (unsigned long long)p->k;
        size_t whole = (size_t)(msg_bits_here / 8ULL);
        unsigned int rem = (unsigned int)(msg_bits_here & 7ULL);
        if (whole)
            memcpy(block, msg + block_start / 8ULL, whole);
        if (rem)
            block[whole] = (uint8_t)(msg[block_start / 8ULL + whole] & (uint8_t)(0xFFU << (8U - rem)));
    }

    if (msg_len_bits >= block_start && msg_len_bits < block_end)
        nl_set_bit(block, (unsigned int)(msg_len_bits - block_start));

    if (length_start < block_end && length_start + NL_LENGTH_FIELD_BITS > block_start)
    {
        for (unsigned int j = 0; j < NL_LENGTH_FIELD_BITS; j++)
        {
            unsigned long long pos = length_start + (unsigned long long)j;
            if (pos >= block_start && pos < block_end)
            {
                if ((msg_len_bits >> (63U - j)) & 1ULL)
                    nl_set_bit(block, (unsigned int)(pos - block_start));
            }
        }
    }
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    NL_Params p;
    uint8_t V[NL_MAX_V_BYTES];
    uint8_t block[NL_MAX_K_BYTES];
    uint8_t stream[NL_MAX_OUTPUT_BYTES];
    unsigned long long zero_bits;
    unsigned long long total_bits;
    unsigned long long block_count;
    size_t v_bytes;
    size_t n_bytes;

    if (digest == 0 || (msg == 0 && msg_len_bits != 0ULL))
        return -1;
    if (digest_len_bits != DIGEST_BIT_LENGTH)
        return -2;
    if (nl_params_from_digest(digest_len_bits, &p) != 0)
        return -3;

    v_bytes = (size_t)p.v / 8U;
    n_bytes = (size_t)p.n / 8U;
    zero_bits = nl_padding_zero_bits(msg_len_bits, p.k);
    total_bits = msg_len_bits + 1ULL + zero_bits + NL_LENGTH_FIELD_BITS;
    block_count = total_bits / (unsigned long long)p.k;

    nl_make_iv(&p, V);
    nl_xor_block_constant(V, v_bytes, 0ULL);

    for (unsigned long long i = 0; i < block_count; i++)
    {
        int last = (i + 1ULL == block_count);
        size_t out_len = last ? (v_bytes + n_bytes) : v_bytes;
        nl_build_padded_block(&p, msg, msg_len_bits, zero_bits, i, block);
        nl_prng(&p, V, block, stream, out_len);
        if (last)
        {
            memcpy(digest, stream + v_bytes, n_bytes);
        }
        else
        {
            memcpy(V, stream, v_bytes);
            nl_xor_block_constant(V, v_bytes, i + 1ULL);
        }
    }

    return 0;
}
