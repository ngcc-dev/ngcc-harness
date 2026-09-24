/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#include "CryptHash_AlgorithmInstance.h"
#include <stdint.h>
#include <string.h>

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

static const unsigned char GM_DST[5][16] = {
    {16, 18, 20, 22, 24, 26, 28, 30, 8, 10, 12, 14, 4, 6, 2, 0},
    {16, 20, 24, 28, 8, 12, 4, 0},
    {16, 24, 8, 0},
    {16, 0},
    {0}
};

static const unsigned char REV8[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static inline word_t rotl64(word_t x, unsigned n)
{
    return (x << n) | (x >> (64U - n));
}

static inline word_t sum2(const word_t *x)
{
    return x[0] + x[1];
}

static inline word_t red4(const word_t *x)
{
    return sum2(x) ^ sum2(x + 2);
}

static inline word_t red8(const word_t *x)
{
    return red4(x) + red4(x + 4);
}

static inline word_t red16(const word_t *x)
{
    return red8(x) ^ red8(x + 8);
}

static inline word_t red32(const word_t *x)
{
    return red16(x) + red16(x + 16);
}

static inline void wr_wp(word_t x[32], word_t y[32])
{
    x[0] = rotl64(y[0], 16);
    x[16] = rotl64(y[1], 23);
    x[1] = rotl64(y[2], 30);
    x[17] = rotl64(y[3], 37);
    x[2] = rotl64(y[4], 44);
    x[18] = rotl64(y[5], 19);
    x[3] = rotl64(y[6], 26);
    x[19] = rotl64(y[7], 33);
    x[4] = rotl64(y[8], 40);
    x[20] = rotl64(y[9], 47);
    x[5] = rotl64(y[10], 22);
    x[21] = rotl64(y[11], 29);
    x[6] = rotl64(y[12], 36);
    x[22] = rotl64(y[13], 43);
    x[7] = rotl64(y[14], 18);
    x[23] = rotl64(y[15], 25);
    x[8] = rotl64(y[16], 32);
    x[24] = rotl64(y[17], 39);
    x[9] = rotl64(y[18], 46);
    x[25] = rotl64(y[19], 21);
    x[10] = rotl64(y[20], 28);
    x[26] = rotl64(y[21], 35);
    x[11] = rotl64(y[22], 42);
    x[27] = rotl64(y[23], 17);
    x[12] = rotl64(y[24], 24);
    x[28] = rotl64(y[25], 31);
    x[13] = rotl64(y[26], 38);
    x[29] = rotl64(y[27], 45);
    x[14] = rotl64(y[28], 20);
    x[30] = rotl64(y[29], 27);
    x[15] = rotl64(y[30], 34);
    x[31] = rotl64(y[31], 41);
}

static inline void round_g2(word_t x[32], word_t ci, word_t cr)
{
    word_t y[32], rc = ci ^ cr;
    unsigned q, j, l;

    for (q = 0, j = 0; q < 16U; q++, j += 2U)
        x[j] ^= C[q] ^ rc;
    for (q = 0, j = 0; q < 16U; q++, j += 2U) {
        word_t s = sum2(x + j);
        l = GM_DST[0][q];
        y[l] = x[l] + 3U * s;
        y[l + 1U] = x[l + 1U] ^ (s + (s << 2));
    }
    wr_wp(x, y);
}

static inline void round_g4(word_t x[32], word_t ci, word_t cr)
{
    word_t y[32], rc = ci ^ cr;
    unsigned q, j, l;

    for (q = 0, j = 0; q < 8U; q++, j += 4U)
        x[j] ^= C[q] ^ rc;
    for (q = 0, j = 0; q < 8U; q++, j += 4U) {
        word_t s = red4(x + j);
        l = GM_DST[1][q];
        y[l] = x[l] + 3U * s;
        y[l + 1U] = x[l + 1U] ^ (s + (s << 2));
        y[l + 2U] = x[l + 2U] ^ (s + (s << 3));
        y[l + 3U] = x[l + 3U] ^ (s + (s << 4));
    }
    wr_wp(x, y);
}

static inline void round_g8(word_t x[32], word_t ci, word_t cr)
{
    word_t y[32], rc = ci ^ cr;
    unsigned q, j, l, k;

    for (q = 0, j = 0; q < 4U; q++, j += 8U)
        x[j] ^= C[q] ^ rc;
    for (q = 0, j = 0; q < 4U; q++, j += 8U) {
        word_t s = red8(x + j);
        l = GM_DST[2][q];
        y[l] = x[l] + 3U * s;
        for (k = 1U; k < 8U; k++)
            y[l + k] = x[l + k] ^ (s + (s << (k + 1U)));
    }
    wr_wp(x, y);
}

static inline void round_g16(word_t x[32], word_t ci, word_t cr)
{
    word_t y[32], rc = ci ^ cr;
    unsigned q, j, l, k;

    for (q = 0, j = 0; q < 2U; q++, j += 16U)
        x[j] ^= C[q] ^ rc;
    for (q = 0, j = 0; q < 2U; q++, j += 16U) {
        word_t s = red16(x + j);
        l = GM_DST[3][q];
        y[l] = x[l] + 3U * s;
        for (k = 1U; k < 16U; k++)
            y[l + k] = x[l + k] ^ (s + (s << (k + 1U)));
    }
    wr_wp(x, y);
}

static inline void round_g32(word_t x[32], word_t ci, word_t cr)
{
    word_t y[32], s;
    unsigned k;

    x[0] ^= C[0] ^ ci ^ cr;
    s = red32(x);
    y[0] = x[0] + 3U * s;
    for (k = 1U; k < 32U; k++)
        y[k] = x[k] ^ (s + (s << (k + 1U)));
    wr_wp(x, y);
}

void CryptHash_AlgorithmInstance_permutation(word_t x[32])
{
    int r;

    for (r = 0; r < 5; r++) {
        word_t cr = C[21 + r];
        round_g2(x, C[16], cr);
        round_g4(x, C[17], cr);
        round_g8(x, C[18], cr);
        round_g16(x, C[19], cr);
        round_g32(x, C[20], cr);
    }
}

static int get_message_bit(const unsigned char *msg, unsigned long long pos)
{
    return (msg[pos / 8U] >> (7U - (unsigned)(pos % 8U))) & 1U;
}

static inline unsigned char reverse8(unsigned char x)
{
    return REV8[x];
}

static inline word_t reverse_bits_in_each_byte64(word_t x)
{
    x = ((x & UINT64_C(0x5555555555555555)) << 1) |
        ((x >> 1) & UINT64_C(0x5555555555555555));
    x = ((x & UINT64_C(0x3333333333333333)) << 2) |
        ((x >> 2) & UINT64_C(0x3333333333333333));
    x = ((x & UINT64_C(0x0f0f0f0f0f0f0f0f)) << 4) |
        ((x >> 4) & UINT64_C(0x0f0f0f0f0f0f0f0f));
    return x;
}

static inline word_t load_message_word(const unsigned char *msg, unsigned long long base)
{
    word_t x;
    const unsigned char *p = msg + (base >> 3);

    memcpy(&x, p, sizeof(x));
    return reverse_bits_in_each_byte64(x);
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
    unsigned long long full_blocks, final_base, block_number, block;
    unsigned rate_bits, rate_words, capacity_words, out_words;
    unsigned remaining_bits, padding_blocks, final_bits, padding_block, i;

    if (digest == 0 || (msg == 0 && msg_len_bits != 0))
        return -1;
    if (digest_len_bits != 1024)
        return -1;

    rate_bits = 2048U - ((unsigned)digest_len_bits + 64U);
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
