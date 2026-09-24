/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#include "CryptHash_AlgorithmInstance.h"
#include <stdint.h>

typedef uint64_t word_t;

static const word_t C[26] = {
    UINT64_C(0x243f6a8885a308d3),
    UINT64_C(0x13198a2e03707344),
    UINT64_C(0xa4093822299f31d0),
    UINT64_C(0x082efa98ec4e6c89),
    UINT64_C(0x452821e638d01377),
    UINT64_C(0xbe5466cf34e90c6c),
    UINT64_C(0xc0ac29b7c97c50dd),
    UINT64_C(0x3f84d5b5b5470917),
    UINT64_C(0x9216d5d98979fb1b),
    UINT64_C(0xd1310ba698dfb5ac),
    UINT64_C(0x2ffd72dbd01adfb7),
    UINT64_C(0xb8e1afed6a267e96),
    UINT64_C(0xba7c9045f12c7f99),
    UINT64_C(0x24a19947b3916cf7),
    UINT64_C(0x0801f2e2858efc16),
    UINT64_C(0x636920d871574e69),
    UINT64_C(0xa458fea3f4933d7e),
    UINT64_C(0x0d95748f728eb658),
    UINT64_C(0x718bcd5882154aee),
    UINT64_C(0x7b54a41dc25a59b5),
    UINT64_C(0x9c30d5392af26013),
    UINT64_C(0xc5d1b023286085f0),
    UINT64_C(0xca417918b8db38ef),
    UINT64_C(0x8e79dcb0603a180e),
    UINT64_C(0x6c9e0e8bb01e8a3e),
    UINT64_C(0xd71577c1bd314b27)
};

static word_t rotl64(word_t x, unsigned n)
{
    return (x << n) | (x >> (64U - n));
}

static word_t S(const word_t *x, int k)
{
    k /= 2;
    return k ? ((k & 10) ? S(x, k) ^ S(x + k, k) : S(x, k) + S(x + k, k)) : *x;
}

void CryptHash_AlgorithmInstance_permutation(word_t x[32])
{
    word_t y[32];
    int r, i, j, k, l, g;

    for (r = 0; r < 5; r++) {
        for (i = 0; i < 5; i++) {
            g = 2 << i;

            for (j = 0; j < 32; j += g)
                x[j] ^= C[j / g] ^ C[16 + i] ^ C[21 + r];

            for (j = 0; j < 32; j += g) {
                word_t s = S(x + j, g);
                l = j;
                for (k = 32; k > (l | g); ) {
                    k >>= 1;
                    l ^= k;
                }
                for (k = 0; k < g; k++) {
                    if (k == 0)
                        y[l] = x[l] + 3U * s;
                    else
                        y[l + k] = x[l + k] ^ (s + (s << (k + 1)));
                }
            }

            for (j = 0; j < 32; j++)
                y[j] = rotl64(y[j], (unsigned)((7 * j % 32) + 16));

            for (j = 0; j < 32; j++)
                x[j / 2 + (j % 2) * 16] = y[j];
        }
    }
}

static int get_message_bit(const unsigned char *msg, unsigned long long pos)
{
    return (msg[pos / 8U] >> (7U - (unsigned)(pos % 8U))) & 1U;
}

static unsigned char reverse8(unsigned char x)
{
    x = (unsigned char)(((x & 0xf0U) >> 4) | ((x & 0x0fU) << 4));
    x = (unsigned char)(((x & 0xccU) >> 2) | ((x & 0x33U) << 2));
    return (unsigned char)(((x & 0xaaU) >> 1) | ((x & 0x55U) << 1));
}

static word_t load_message_word(const unsigned char *msg, unsigned long long base)
{
    word_t w = 0;
    unsigned i;

    for (i = 0; i < 8U; i++)
        w |= (word_t)reverse8(msg[base / 8U + i]) << (8U * i);
    return w;
}

static word_t load_final_word(const unsigned char *msg,
                              unsigned long long final_base,
                              unsigned long long remaining_bits,
                              unsigned final_bits, unsigned local_base)
{
    word_t w = 0;
    unsigned bit;

    for (bit = 0; bit < 64U; bit++) {
        unsigned local_pos = local_base + bit;
        if ((local_pos < remaining_bits &&
             get_message_bit(msg, final_base + local_pos)) ||
            local_pos == remaining_bits || local_pos + 1U == final_bits)
            w |= (word_t)1 << bit;
    }
    return w;
}

static void store64_le(unsigned char *out, word_t x)
{
    unsigned i;

    for (i = 0; i < 8U; i++)
        out[i] = (unsigned char)(x >> (8U * i));
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    word_t state[32] = {0};
    unsigned rate_bits, rate_words, capacity_words, out_words;
    unsigned long long full_blocks, final_base, block_number, block;
    unsigned remaining_bits, padding_blocks, final_bits, padding_block, i;

    if (digest == 0 || (msg == 0 && msg_len_bits != 0))
        return -1;
    if (digest_len_bits != DIGEST_BIT_LENGTH)
        return -1;

    rate_bits = 2048U - (unsigned)digest_len_bits - 64U;
    rate_words = rate_bits / 64U;
    capacity_words = 32U - rate_words;
    out_words = (unsigned)digest_len_bits / 64U;
    full_blocks = msg_len_bits / rate_bits;
    remaining_bits = (unsigned)(msg_len_bits % rate_bits);
    padding_blocks = remaining_bits > rate_bits - 2U ? 2U : 1U;
    final_bits = padding_blocks * rate_bits;

    for (block_number = 0; block_number < full_blocks; block_number++) {
        word_t feed_forward[17];
        block = block_number * rate_bits;

        for (i = 0; i < rate_words; i++)
            state[i] ^= load_message_word(msg, block + 64ULL * i);

        for (i = 0; i < capacity_words; i++)
            feed_forward[i] = state[rate_words + i];

        CryptHash_AlgorithmInstance_permutation(state);

        for (i = 0; i < capacity_words; i++)
            state[rate_words + i] ^= feed_forward[i];
    }

    final_base = full_blocks * rate_bits;
    for (padding_block = 0; padding_block < padding_blocks; padding_block++) {
        word_t feed_forward[17];

        for (i = 0; i < rate_words; i++)
            state[i] ^= load_final_word(msg, final_base, remaining_bits,
                                        final_bits,
                                        padding_block * rate_bits + 64U * i);

        for (i = 0; i < capacity_words; i++)
            feed_forward[i] = state[rate_words + i];

        CryptHash_AlgorithmInstance_permutation(state);

        for (i = 0; i < capacity_words; i++)
            state[rate_words + i] ^= feed_forward[i];
    }

    for (i = 0; i < out_words; i++)
        store64_le(digest + 8U * i, state[rate_words + i]);

    return 0;
}
