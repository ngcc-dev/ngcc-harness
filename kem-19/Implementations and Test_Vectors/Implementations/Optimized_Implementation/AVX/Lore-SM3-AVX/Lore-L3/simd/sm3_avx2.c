#include "simd/sm3_avx2.h"

#ifdef LORE_USE_AVX2_SM3

#include <immintrin.h>
#include <stddef.h>
#include <string.h>

#define SM3_AVX2_OK 0
#define SM3_AVX2_UNSUPPORTED -1

static __m256i rol32(__m256i x, int n)
{
    return _mm256_or_si256(_mm256_slli_epi32(x, n),
                           _mm256_srli_epi32(x, 32 - n));
}

static __m256i p0(__m256i x)
{
    return _mm256_xor_si256(_mm256_xor_si256(x, rol32(x, 9)), rol32(x, 17));
}

static __m256i p1(__m256i x)
{
    return _mm256_xor_si256(_mm256_xor_si256(x, rol32(x, 15)), rol32(x, 23));
}

static uint8_t message_byte(const unsigned char *msg,
                            size_t msg_len,
                            int with_counter,
                            uint32_t counter,
                            size_t total_len,
                            size_t padded_len,
                            uint64_t bit_len,
                            size_t pos)
{
    if (pos < msg_len) {
        return msg[pos];
    }

    if (with_counter && pos < msg_len + 4) {
        const unsigned int shift = 24U - 8U * (unsigned int)(pos - msg_len);
        return (uint8_t)(counter >> shift);
    }

    if (pos == total_len) {
        return 0x80;
    }

    if (pos >= padded_len - 8) {
        const unsigned int shift = 56U - 8U * (unsigned int)(pos - (padded_len - 8));
        return (uint8_t)(bit_len >> shift);
    }

    return 0;
}

static __m256i load_word8(const unsigned char *msg,
                          size_t msg_len,
                          int with_counter,
                          uint32_t counter_start,
                          size_t total_len,
                          size_t padded_len,
                          uint64_t bit_len,
                          size_t block,
                          unsigned int word)
{
    uint32_t lane_words[8];
    const size_t base = block * 64 + (size_t)word * 4;

    for (unsigned int lane = 0; lane < 8; lane++) {
        const uint32_t counter = counter_start + lane;
        uint32_t w = 0;
        for (unsigned int b = 0; b < 4; b++) {
            w = (w << 8) |
                message_byte(msg, msg_len, with_counter, counter,
                             total_len, padded_len, bit_len, base + b);
        }
        lane_words[lane] = w;
    }

    return _mm256_setr_epi32((int)lane_words[0], (int)lane_words[1],
                             (int)lane_words[2], (int)lane_words[3],
                             (int)lane_words[4], (int)lane_words[5],
                             (int)lane_words[6], (int)lane_words[7]);
}

static void store_hash8(unsigned char out[8][32], const __m256i digest[8])
{
    uint32_t lanes[8];

    for (unsigned int word = 0; word < 8; word++) {
        _mm256_storeu_si256((__m256i *)lanes, digest[word]);
        for (unsigned int lane = 0; lane < 8; lane++) {
            out[lane][4 * word + 0] = (uint8_t)(lanes[lane] >> 24);
            out[lane][4 * word + 1] = (uint8_t)(lanes[lane] >> 16);
            out[lane][4 * word + 2] = (uint8_t)(lanes[lane] >> 8);
            out[lane][4 * word + 3] = (uint8_t)lanes[lane];
        }
    }
}

static void sm3_hash8(unsigned char out[8][32],
                      const unsigned char *msg,
                      size_t msg_len,
                      uint32_t counter_start,
                      int with_counter)
{
    const size_t total_len = msg_len + (with_counter ? 4U : 0U);
    const size_t padded_len = ((total_len + 1U + 8U + 63U) / 64U) * 64U;
    const size_t blocks = padded_len / 64U;
    const uint64_t bit_len = (uint64_t)total_len * 8U;

    __m256i digest[8] = {
        _mm256_set1_epi32(0x7380166fU),
        _mm256_set1_epi32(0x4914b2b9U),
        _mm256_set1_epi32(0x172442d7U),
        _mm256_set1_epi32(0xda8a0600U),
        _mm256_set1_epi32(0xa96f30bcU),
        _mm256_set1_epi32(0x163138aaU),
        _mm256_set1_epi32(0xe38dee4dU),
        _mm256_set1_epi32(0xb0fb0e4eU)
    };

    for (size_t block = 0; block < blocks; block++) {
        __m256i w[68];
        __m256i wp[64];

        for (unsigned int i = 0; i < 16; i++) {
            w[i] = load_word8(msg, msg_len, with_counter, counter_start,
                              total_len, padded_len, bit_len, block, i);
        }
        for (unsigned int i = 16; i < 68; i++) {
            w[i] = _mm256_xor_si256(
                p1(_mm256_xor_si256(
                    _mm256_xor_si256(w[i - 16], w[i - 9]),
                    rol32(w[i - 3], 15))),
                _mm256_xor_si256(rol32(w[i - 13], 7), w[i - 6]));
        }
        for (unsigned int i = 0; i < 64; i++) {
            wp[i] = _mm256_xor_si256(w[i], w[i + 4]);
        }

        __m256i a = digest[0];
        __m256i b = digest[1];
        __m256i c = digest[2];
        __m256i d = digest[3];
        __m256i e = digest[4];
        __m256i f = digest[5];
        __m256i g = digest[6];
        __m256i h = digest[7];

        for (unsigned int i = 0; i < 64; i++) {
            const uint32_t tj = (i < 16) ? 0x79cc4519U : 0x7a879d8aU;
            const __m256i tjv = rol32(_mm256_set1_epi32((int)tj), (int)(i & 31U));
            const __m256i a12 = rol32(a, 12);
            const __m256i ss1 = rol32(_mm256_add_epi32(_mm256_add_epi32(a12, e), tjv), 7);
            const __m256i ss2 = _mm256_xor_si256(ss1, a12);
            __m256i ff;
            __m256i gg;

            if (i < 16) {
                ff = _mm256_xor_si256(_mm256_xor_si256(a, b), c);
                gg = _mm256_xor_si256(_mm256_xor_si256(e, f), g);
            } else {
                ff = _mm256_or_si256(_mm256_or_si256(_mm256_and_si256(a, b),
                                                     _mm256_and_si256(a, c)),
                                     _mm256_and_si256(b, c));
                gg = _mm256_xor_si256(_mm256_and_si256(e, _mm256_xor_si256(f, g)), g);
            }

            const __m256i tt1 = _mm256_add_epi32(
                _mm256_add_epi32(_mm256_add_epi32(ff, d), ss2), wp[i]);
            const __m256i tt2 = _mm256_add_epi32(
                _mm256_add_epi32(_mm256_add_epi32(gg, h), ss1), w[i]);

            d = c;
            c = rol32(b, 9);
            b = a;
            a = tt1;
            h = g;
            g = rol32(f, 19);
            f = e;
            e = p0(tt2);
        }

        digest[0] = _mm256_xor_si256(digest[0], a);
        digest[1] = _mm256_xor_si256(digest[1], b);
        digest[2] = _mm256_xor_si256(digest[2], c);
        digest[3] = _mm256_xor_si256(digest[3], d);
        digest[4] = _mm256_xor_si256(digest[4], e);
        digest[5] = _mm256_xor_si256(digest[5], f);
        digest[6] = _mm256_xor_si256(digest[6], g);
        digest[7] = _mm256_xor_si256(digest[7], h);
    }

    store_hash8(out, digest);
}

int lore_sm3_hash_avx2(unsigned char *digest,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits)
{
    unsigned char lanes[8][32];

    if ((msg_len_bits & 7ULL) != 0) {
        return SM3_AVX2_UNSUPPORTED;
    }

    sm3_hash8(lanes, msg, (size_t)(msg_len_bits / 8ULL), 0, 0);
    memcpy(digest, lanes[0], 32);
    return SM3_AVX2_OK;
}

int lore_sm3_pseudoXOF_avx2(unsigned long long output_len_bits,
                            const unsigned char *msg,
                            unsigned long long msg_len_bits,
                            unsigned char *output)
{
    const size_t msg_len = (size_t)(msg_len_bits / 8ULL);
    const size_t output_len = (size_t)((output_len_bits + 7ULL) / 8ULL);
    const size_t output_blocks = (size_t)((output_len_bits + 255ULL) / 256ULL);
    size_t produced = 0;
    unsigned int counter = 1;

    if ((msg_len_bits & 7ULL) != 0) {
        return SM3_AVX2_UNSUPPORTED;
    }

    for (size_t block = 0; block < output_blocks; block += 8) {
        unsigned char lanes[8][32];
        sm3_hash8(lanes, msg, msg_len, counter, 1);
        counter += 8;

        for (unsigned int lane = 0; lane < 8 && produced < output_len; lane++) {
            const size_t remaining = output_len - produced;
            const size_t take = remaining < 32U ? remaining : 32U;
            memcpy(output + produced, lanes[lane], take);
            produced += take;
        }
    }

    if ((output_len_bits & 7ULL) != 0 && output_len > 0) {
        const unsigned int keep = (unsigned int)(output_len_bits & 7ULL);
        output[output_len - 1] &= (uint8_t)(0xffU << (8U - keep));
    }

    return SM3_AVX2_OK;
}

#endif
