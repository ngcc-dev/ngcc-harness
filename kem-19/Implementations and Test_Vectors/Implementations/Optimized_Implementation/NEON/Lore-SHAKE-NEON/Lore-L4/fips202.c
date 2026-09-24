#include "fips202.h"

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

static uint64_t rotl64(uint64_t x, unsigned int n)
{
    return n == 0 ? x : ((x << n) | (x >> (64 - n)));
}

static void keccak_permute(uint64_t a[25])
{
    uint64_t c[5];
    uint64_t d[5];
    uint64_t b[25];

    for (unsigned int round = 0; round < 24; round++) {
        for (unsigned int x = 0; x < 5; x++) {
            c[x] = a[x] ^ a[x + 5] ^ a[x + 10] ^ a[x + 15] ^ a[x + 20];
        }

        for (unsigned int x = 0; x < 5; x++) {
            d[x] = c[(x + 4) % 5] ^ rotl64(c[(x + 1) % 5], 1);
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                a[x + 5 * y] ^= d[x];
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                unsigned int src = x + 5 * y;
                unsigned int dst = y + 5 * ((2 * x + 3 * y) % 5);
                b[dst] = rotl64(a[src], keccak_rho_offsets[src]);
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                a[x + 5 * y] = b[x + 5 * y] ^
                    ((~b[((x + 1) % 5) + 5 * y]) & b[((x + 2) % 5) + 5 * y]);
            }
        }

        a[0] ^= keccak_round_constants[round];
    }
}

static uint8_t state_get_byte(const keccak_state *state, unsigned int pos)
{
    return (uint8_t)(state->s[pos / 8] >> (8 * (pos % 8)));
}

static void state_xor_byte(keccak_state *state, unsigned int pos, uint8_t value)
{
    state->s[pos / 8] ^= (uint64_t)value << (8 * (pos % 8));
}

static void keccak_init(keccak_state *state, unsigned int rate)
{
    memset(state, 0, sizeof(*state));
    state->rate = rate;
}

static void keccak_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
    while (inlen > 0) {
        if (state->pos == state->rate) {
            keccak_permute(state->s);
            state->pos = 0;
        }

        state_xor_byte(state, state->pos, *in);
        state->pos++;
        in++;
        inlen--;
    }
}

static void keccak_finalize(keccak_state *state, uint8_t domain)
{
    if (state->finalized) {
        return;
    }

    state_xor_byte(state, state->pos, domain);
    state_xor_byte(state, state->rate - 1, 0x80);
    keccak_permute(state->s);
    state->pos = 0;
    state->finalized = 1;
}

static void keccak_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
    while (outlen > 0) {
        if (state->pos == state->rate) {
            keccak_permute(state->s);
            state->pos = 0;
        }

        *out = state_get_byte(state, state->pos);
        state->pos++;
        out++;
        outlen--;
    }
}

static void xof_once(uint8_t *out, size_t outlen,
                     unsigned int rate,
                     const uint8_t *in, size_t inlen)
{
    keccak_state state;

    keccak_init(&state, rate);
    keccak_absorb(&state, in, inlen);
    keccak_finalize(&state, 0x1f);
    keccak_squeeze(out, outlen, &state);
}

static void hash_once(uint8_t *out, size_t outlen,
                      unsigned int rate,
                      const uint8_t *in, size_t inlen)
{
    keccak_state state;

    keccak_init(&state, rate);
    keccak_absorb(&state, in, inlen);
    keccak_finalize(&state, 0x06);
    keccak_squeeze(out, outlen, &state);
}

void shake128_init(keccak_state *state)
{
    keccak_init(state, SHAKE128_RATE);
}

void shake128_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
    keccak_absorb(state, in, inlen);
}

void shake128_finalize(keccak_state *state)
{
    keccak_finalize(state, 0x1f);
}

void shake128_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
    shake128_finalize(state);
    keccak_squeeze(out, outlen, state);
}

void shake128_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
    shake128_init(state);
    shake128_absorb(state, in, inlen);
    shake128_finalize(state);
}

void shake128_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
    keccak_squeeze(out, nblocks * SHAKE128_RATE, state);
}

void shake256_init(keccak_state *state)
{
    keccak_init(state, SHAKE256_RATE);
}

void shake256_absorb(keccak_state *state, const uint8_t *in, size_t inlen)
{
    keccak_absorb(state, in, inlen);
}

void shake256_finalize(keccak_state *state)
{
    keccak_finalize(state, 0x1f);
}

void shake256_squeeze(uint8_t *out, size_t outlen, keccak_state *state)
{
    shake256_finalize(state);
    keccak_squeeze(out, outlen, state);
}

void shake256_absorb_once(keccak_state *state, const uint8_t *in, size_t inlen)
{
    shake256_init(state);
    shake256_absorb(state, in, inlen);
    shake256_finalize(state);
}

void shake256_squeezeblocks(uint8_t *out, size_t nblocks, keccak_state *state)
{
    keccak_squeeze(out, nblocks * SHAKE256_RATE, state);
}

void shake128(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    xof_once(out, outlen, SHAKE128_RATE, in, inlen);
}

void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    xof_once(out, outlen, SHAKE256_RATE, in, inlen);
}

void sha3_256(uint8_t h[32], const uint8_t *in, size_t inlen)
{
    hash_once(h, 32, SHA3_256_RATE, in, inlen);
}

void sha3_512(uint8_t h[64], const uint8_t *in, size_t inlen)
{
    hash_once(h, 64, SHA3_512_RATE, in, inlen);
}
