#include "simd/shake_neon.h"

#ifdef LORE_USE_NEON_SHAKE

#include "fips202.h"

#include <arm_neon.h>
#include <string.h>

static const uint64_t keccak_round_constants[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

static const unsigned int keccak_rho_offsets[25] = {
     0,  1, 62, 28, 27,
    36, 44,  6, 55, 20,
     3, 10, 43, 25, 39,
    41, 45, 15, 21,  8,
    18,  2, 61, 56, 14
};

static uint64_t load64(const uint8_t in[8])
{
    uint64_t r;
    memcpy(&r, in, sizeof(r));
    return r;
}

static void store64(uint8_t out[8], uint64_t v)
{
    memcpy(out, &v, sizeof(v));
}

static inline uint64x2_t rol64(uint64x2_t x, unsigned int n)
{
    switch (n) {
    case 0:
        return x;
    case 1:
        return vorrq_u64(vshlq_n_u64(x, 1), vshrq_n_u64(x, 63));
    case 2:
        return vorrq_u64(vshlq_n_u64(x, 2), vshrq_n_u64(x, 62));
    case 3:
        return vorrq_u64(vshlq_n_u64(x, 3), vshrq_n_u64(x, 61));
    case 6:
        return vorrq_u64(vshlq_n_u64(x, 6), vshrq_n_u64(x, 58));
    case 8:
        return vorrq_u64(vshlq_n_u64(x, 8), vshrq_n_u64(x, 56));
    case 10:
        return vorrq_u64(vshlq_n_u64(x, 10), vshrq_n_u64(x, 54));
    case 14:
        return vorrq_u64(vshlq_n_u64(x, 14), vshrq_n_u64(x, 50));
    case 15:
        return vorrq_u64(vshlq_n_u64(x, 15), vshrq_n_u64(x, 49));
    case 18:
        return vorrq_u64(vshlq_n_u64(x, 18), vshrq_n_u64(x, 46));
    case 20:
        return vorrq_u64(vshlq_n_u64(x, 20), vshrq_n_u64(x, 44));
    case 21:
        return vorrq_u64(vshlq_n_u64(x, 21), vshrq_n_u64(x, 43));
    case 25:
        return vorrq_u64(vshlq_n_u64(x, 25), vshrq_n_u64(x, 39));
    case 27:
        return vorrq_u64(vshlq_n_u64(x, 27), vshrq_n_u64(x, 37));
    case 28:
        return vorrq_u64(vshlq_n_u64(x, 28), vshrq_n_u64(x, 36));
    case 36:
        return vorrq_u64(vshlq_n_u64(x, 36), vshrq_n_u64(x, 28));
    case 39:
        return vorrq_u64(vshlq_n_u64(x, 39), vshrq_n_u64(x, 25));
    case 41:
        return vorrq_u64(vshlq_n_u64(x, 41), vshrq_n_u64(x, 23));
    case 43:
        return vorrq_u64(vshlq_n_u64(x, 43), vshrq_n_u64(x, 21));
    case 44:
        return vorrq_u64(vshlq_n_u64(x, 44), vshrq_n_u64(x, 20));
    case 45:
        return vorrq_u64(vshlq_n_u64(x, 45), vshrq_n_u64(x, 19));
    case 55:
        return vorrq_u64(vshlq_n_u64(x, 55), vshrq_n_u64(x, 9));
    case 56:
        return vorrq_u64(vshlq_n_u64(x, 56), vshrq_n_u64(x, 8));
    case 61:
        return vorrq_u64(vshlq_n_u64(x, 61), vshrq_n_u64(x, 3));
    case 62:
        return vorrq_u64(vshlq_n_u64(x, 62), vshrq_n_u64(x, 2));
    default:
        return vorrq_u64(vshlq_u64(x, vdupq_n_s64((int64_t)n)),
                         vshlq_u64(x, vdupq_n_s64((int64_t)n - 64)));
    }
}

static void keccak_permute2x(uint64x2_t a[25])
{
    uint64x2_t c[5];
    uint64x2_t d[5];
    uint64x2_t b[25];

    for (unsigned int round = 0; round < 24; round++) {
        for (unsigned int x = 0; x < 5; x++) {
            c[x] = veorq_u64(veorq_u64(a[x], a[x + 5]),
                             veorq_u64(veorq_u64(a[x + 10], a[x + 15]), a[x + 20]));
        }

        for (unsigned int x = 0; x < 5; x++) {
            d[x] = veorq_u64(c[(x + 4) % 5], rol64(c[(x + 1) % 5], 1));
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                a[x + 5 * y] = veorq_u64(a[x + 5 * y], d[x]);
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                const unsigned int src = x + 5 * y;
                const unsigned int dst = y + 5 * ((2 * x + 3 * y) % 5);
                b[dst] = rol64(a[src], keccak_rho_offsets[src]);
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                const unsigned int idx = x + 5 * y;
                a[idx] = veorq_u64(b[idx],
                                   vbicq_u64(b[((x + 2) % 5) + 5 * y],
                                             b[((x + 1) % 5) + 5 * y]));
            }
        }

        a[0] = veorq_u64(a[0], vdupq_n_u64(keccak_round_constants[round]));
    }
}

static void absorb_full_block(uint64x2_t state[25],
                              const uint8_t *in0,
                              const uint8_t *in1,
                              unsigned int rate)
{
    const unsigned int words = rate / 8;

    for (unsigned int i = 0; i < words; i++) {
        const uint64_t lanes[2] = { load64(in0 + 8 * i), load64(in1 + 8 * i) };
        state[i] = veorq_u64(state[i], vld1q_u64(lanes));
    }
    keccak_permute2x(state);
}

static void absorb_final_block(uint64x2_t state[25],
                               const uint8_t *in0,
                               const uint8_t *in1,
                               size_t inlen,
                               unsigned int rate,
                               uint8_t domain)
{
    uint8_t block0[SHAKE128_RATE] = {0};
    uint8_t block1[SHAKE128_RATE] = {0};

    memcpy(block0, in0, inlen);
    memcpy(block1, in1, inlen);

    block0[inlen] ^= domain;
    block1[inlen] ^= domain;
    block0[rate - 1] ^= 0x80;
    block1[rate - 1] ^= 0x80;

    absorb_full_block(state, block0, block1, rate);
}

static void squeeze_block(uint8_t *out0,
                          uint8_t *out1,
                          const uint64x2_t state[25],
                          unsigned int rate)
{
    const unsigned int words = rate / 8;
    uint64_t lanes[2];

    for (unsigned int i = 0; i < words; i++) {
        vst1q_u64(lanes, state[i]);
        store64(out0 + 8 * i, lanes[0]);
        store64(out1 + 8 * i, lanes[1]);
    }
}

void lore_shake128x2_absorb_once_squeezeblocks(uint8_t *out0,
                                               uint8_t *out1,
                                               size_t outblocks,
                                               const uint8_t *in0,
                                               const uint8_t *in1,
                                               size_t inlen)
{
    uint64x2_t state[25];
    const unsigned int rate = SHAKE128_RATE;

    if (outblocks == 0) {
        return;
    }

    memset(state, 0, sizeof(state));

    while (inlen >= rate) {
        absorb_full_block(state, in0, in1, rate);
        in0 += rate;
        in1 += rate;
        inlen -= rate;
    }

    absorb_final_block(state, in0, in1, inlen, rate, 0x1f);

    for (size_t block = 0; block < outblocks; block++) {
        squeeze_block(out0 + block * rate, out1 + block * rate, state, rate);
        if (block + 1 < outblocks) {
            keccak_permute2x(state);
        }
    }
}

#endif
