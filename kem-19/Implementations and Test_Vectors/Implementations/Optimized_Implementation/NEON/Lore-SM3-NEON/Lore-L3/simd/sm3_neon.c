#include "simd/sm3_neon.h"

#ifdef LORE_USE_NEON_SM3

#include <arm_neon.h>
#include <stddef.h>
#include <string.h>

#define SM3_NEON_OK 0
#define SM3_NEON_UNSUPPORTED -1

#define ROL32_CASE(N) \
    case N: return vorrq_u32(vshlq_n_u32(x, N), vshrq_n_u32(x, 32 - (N)))

static inline uint32x4_t rol32(uint32x4_t x, int n)
{
    switch (n) {
    case 0:
        return x;
    ROL32_CASE(1);
    ROL32_CASE(2);
    ROL32_CASE(3);
    ROL32_CASE(4);
    ROL32_CASE(5);
    ROL32_CASE(6);
    ROL32_CASE(7);
    ROL32_CASE(8);
    ROL32_CASE(9);
    ROL32_CASE(10);
    ROL32_CASE(11);
    ROL32_CASE(12);
    ROL32_CASE(13);
    ROL32_CASE(14);
    ROL32_CASE(15);
    ROL32_CASE(16);
    ROL32_CASE(17);
    ROL32_CASE(18);
    ROL32_CASE(19);
    ROL32_CASE(20);
    ROL32_CASE(21);
    ROL32_CASE(22);
    ROL32_CASE(23);
    ROL32_CASE(24);
    ROL32_CASE(25);
    ROL32_CASE(26);
    ROL32_CASE(27);
    ROL32_CASE(28);
    ROL32_CASE(29);
    ROL32_CASE(30);
    ROL32_CASE(31);
    default:
        return vorrq_u32(vshlq_u32(x, vdupq_n_s32(n)),
                         vshlq_u32(x, vdupq_n_s32(n - 32)));
    }
}

#undef ROL32_CASE

static uint32x4_t p0(uint32x4_t x)
{
    return veorq_u32(veorq_u32(x, rol32(x, 9)), rol32(x, 17));
}

static uint32x4_t p1(uint32x4_t x)
{
    return veorq_u32(veorq_u32(x, rol32(x, 15)), rol32(x, 23));
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

static uint32x4_t load_word4(const unsigned char *msg,
                             size_t msg_len,
                             int with_counter,
                             uint32_t counter_start,
                             size_t total_len,
                             size_t padded_len,
                             uint64_t bit_len,
                             size_t block,
                             unsigned int word)
{
    uint32_t lane_words[4];
    const size_t base = block * 64 + (size_t)word * 4;

    for (unsigned int lane = 0; lane < 4; lane++) {
        const uint32_t counter = counter_start + lane;
        uint32_t w = 0;
        for (unsigned int b = 0; b < 4; b++) {
            w = (w << 8) |
                message_byte(msg, msg_len, with_counter, counter,
                             total_len, padded_len, bit_len, base + b);
        }
        lane_words[lane] = w;
    }

    return vld1q_u32(lane_words);
}

static void store_hash4(unsigned char out[4][32], const uint32x4_t digest[8])
{
    uint32_t lanes[4];

    for (unsigned int word = 0; word < 8; word++) {
        vst1q_u32(lanes, digest[word]);
        for (unsigned int lane = 0; lane < 4; lane++) {
            out[lane][4 * word + 0] = (uint8_t)(lanes[lane] >> 24);
            out[lane][4 * word + 1] = (uint8_t)(lanes[lane] >> 16);
            out[lane][4 * word + 2] = (uint8_t)(lanes[lane] >> 8);
            out[lane][4 * word + 3] = (uint8_t)lanes[lane];
        }
    }
}

static void sm3_hash4(unsigned char out[4][32],
                      const unsigned char *msg,
                      size_t msg_len,
                      uint32_t counter_start,
                      int with_counter)
{
    const size_t total_len = msg_len + (with_counter ? 4U : 0U);
    const size_t padded_len = ((total_len + 1U + 8U + 63U) / 64U) * 64U;
    const size_t blocks = padded_len / 64U;
    const uint64_t bit_len = (uint64_t)total_len * 8U;

    uint32x4_t digest[8] = {
        vdupq_n_u32(0x7380166fU),
        vdupq_n_u32(0x4914b2b9U),
        vdupq_n_u32(0x172442d7U),
        vdupq_n_u32(0xda8a0600U),
        vdupq_n_u32(0xa96f30bcU),
        vdupq_n_u32(0x163138aaU),
        vdupq_n_u32(0xe38dee4dU),
        vdupq_n_u32(0xb0fb0e4eU)
    };

    for (size_t block = 0; block < blocks; block++) {
        uint32x4_t w[68];
        uint32x4_t wp[64];

        for (unsigned int i = 0; i < 16; i++) {
            w[i] = load_word4(msg, msg_len, with_counter, counter_start,
                              total_len, padded_len, bit_len, block, i);
        }
        for (unsigned int i = 16; i < 68; i++) {
            w[i] = veorq_u32(
                p1(veorq_u32(veorq_u32(w[i - 16], w[i - 9]), rol32(w[i - 3], 15))),
                veorq_u32(rol32(w[i - 13], 7), w[i - 6]));
        }
        for (unsigned int i = 0; i < 64; i++) {
            wp[i] = veorq_u32(w[i], w[i + 4]);
        }

        uint32x4_t a = digest[0];
        uint32x4_t b = digest[1];
        uint32x4_t c = digest[2];
        uint32x4_t d = digest[3];
        uint32x4_t e = digest[4];
        uint32x4_t f = digest[5];
        uint32x4_t g = digest[6];
        uint32x4_t h = digest[7];

        for (unsigned int i = 0; i < 64; i++) {
            const uint32_t tj = (i < 16) ? 0x79cc4519U : 0x7a879d8aU;
            const uint32x4_t tjv = rol32(vdupq_n_u32(tj), (int)(i & 31U));
            const uint32x4_t a12 = rol32(a, 12);
            const uint32x4_t ss1 = rol32(vaddq_u32(vaddq_u32(a12, e), tjv), 7);
            const uint32x4_t ss2 = veorq_u32(ss1, a12);
            uint32x4_t ff;
            uint32x4_t gg;

            if (i < 16) {
                ff = veorq_u32(veorq_u32(a, b), c);
                gg = veorq_u32(veorq_u32(e, f), g);
            } else {
                ff = vorrq_u32(vorrq_u32(vandq_u32(a, b), vandq_u32(a, c)), vandq_u32(b, c));
                gg = veorq_u32(vandq_u32(e, veorq_u32(f, g)), g);
            }

            const uint32x4_t tt1 = vaddq_u32(vaddq_u32(vaddq_u32(ff, d), ss2), wp[i]);
            const uint32x4_t tt2 = vaddq_u32(vaddq_u32(vaddq_u32(gg, h), ss1), w[i]);

            d = c;
            c = rol32(b, 9);
            b = a;
            a = tt1;
            h = g;
            g = rol32(f, 19);
            f = e;
            e = p0(tt2);
        }

        digest[0] = veorq_u32(digest[0], a);
        digest[1] = veorq_u32(digest[1], b);
        digest[2] = veorq_u32(digest[2], c);
        digest[3] = veorq_u32(digest[3], d);
        digest[4] = veorq_u32(digest[4], e);
        digest[5] = veorq_u32(digest[5], f);
        digest[6] = veorq_u32(digest[6], g);
        digest[7] = veorq_u32(digest[7], h);
    }

    store_hash4(out, digest);
}

int lore_sm3_hash_neon(unsigned char *digest,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits)
{
    unsigned char lanes[4][32];

    if ((msg_len_bits & 7ULL) != 0) {
        return SM3_NEON_UNSUPPORTED;
    }

    sm3_hash4(lanes, msg, (size_t)(msg_len_bits / 8ULL), 0, 0);
    memcpy(digest, lanes[0], 32);
    return SM3_NEON_OK;
}

int lore_sm3_pseudoXOF_neon(unsigned long long output_len_bits,
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
        return SM3_NEON_UNSUPPORTED;
    }

    for (size_t block = 0; block < output_blocks; block += 4) {
        unsigned char lanes[4][32];
        sm3_hash4(lanes, msg, msg_len, counter, 1);
        counter += 4;

        for (unsigned int lane = 0; lane < 4 && produced < output_len; lane++) {
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

    return SM3_NEON_OK;
}

#endif
