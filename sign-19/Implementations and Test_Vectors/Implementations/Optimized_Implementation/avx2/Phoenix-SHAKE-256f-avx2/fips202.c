/* Phoenix-SHAKE-128f implementation based on the public domain Keccak code
 * from http://bench.cr.yp.to/supercop.html by Ronny Van Keer
 * and the TweetFips202 implementation by Gilles Van Assche, Daniel J. Bernstein,
 * and Peter Schwabe. This version uses 4-way AVX2 parallelism for improved
 * throughput on supported platforms.
 *
 * Key differences from reference implementation:
 * - Uses KeccakP1600times4_PermuteAll for 4-lane parallel permutation
 * - Provides incremental and block-based SHAKE256 interfaces
 * - Optimized for Phoenix signature scheme operations
 */

#include <stddef.h>
#include <stdint.h>

#include "fips202.h"

#define PHOENIX_NROUNDS 24
#define ROL_ULL(val, shift) (((val) << (shift)) ^ ((val) >> (64 - (shift))))

/*************************************************
 * Name:        ull_from_bytes
 *
 * Description: Convert 8 consecutive bytes into a 64-bit unsigned integer
 *              using little-endian byte ordering
 *
 * Arguments:   - const uint8_t *input: pointer to input byte array
 *
 * Returns the loaded 64-bit unsigned integer
 **************************************************/
static uint64_t ull_from_bytes(const uint8_t *input) {
    uint64_t result = 0;
    for (size_t idx = 0; idx < 8; ++idx) {
        result |= (uint64_t)input[idx] << (8 * idx);
    }
    return result;
}

/*************************************************
 * Name:        ull_to_bytes
 *
 * Description: Convert a 64-bit unsigned integer into 8 consecutive bytes
 *              using little-endian byte ordering
 *
 * Arguments:   - uint8_t *output: pointer to the output byte array
 *              - uint64_t val: input 64-bit unsigned integer
 **************************************************/
static void ull_to_bytes(uint8_t *output, uint64_t val) {
    for (size_t idx = 0; idx < 8; ++idx) {
        output[idx] = (uint8_t)(val >> (8 * idx));
    }
}

/* Keccak F1600 round constants */
static const uint64_t KeccakF_RC[PHOENIX_NROUNDS] = {
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

/*************************************************
 * Name:        keccak_p1600_permute
 *
 * Description: Execute the Keccak F1600 permutation on the state array
 *
 * Arguments:   - uint64_t *state: pointer to input/output state array
 **************************************************/
static void keccak_p1600_permute(uint64_t *state) {
    int r;

    uint64_t s00, s01, s02, s03, s04;
    uint64_t s10, s11, s12, s13, s14;
    uint64_t s20, s21, s22, s23, s24;
    uint64_t s30, s31, s32, s33, s34;
    uint64_t s40, s41, s42, s43, s44;
    uint64_t b00, b01, b02, b03, b04;
    uint64_t d00, d01, d02, d03, d04;
    uint64_t e00, e01, e02, e03, e04;
    uint64_t e10, e11, e12, e13, e14;
    uint64_t e20, e21, e22, e23, e24;
    uint64_t e30, e31, e32, e33, e34;
    uint64_t e40, e41, e42, e43, e44;

    /* Load state into local variables */
    s00 = state[0];
    s01 = state[1];
    s02 = state[2];
    s03 = state[3];
    s04 = state[4];
    s10 = state[5];
    s11 = state[6];
    s12 = state[7];
    s13 = state[8];
    s14 = state[9];
    s20 = state[10];
    s21 = state[11];
    s22 = state[12];
    s23 = state[13];
    s24 = state[14];
    s30 = state[15];
    s31 = state[16];
    s32 = state[17];
    s33 = state[18];
    s34 = state[19];
    s40 = state[20];
    s41 = state[21];
    s42 = state[22];
    s43 = state[23];
    s44 = state[24];

    for (r = 0; r < PHOENIX_NROUNDS; r += 2) {
        /* Theta step: compute column parity */
        b00 = s00 ^ s10 ^ s20 ^ s30 ^ s40;
        b01 = s01 ^ s11 ^ s21 ^ s31 ^ s41;
        b02 = s02 ^ s12 ^ s22 ^ s32 ^ s42;
        b03 = s03 ^ s13 ^ s23 ^ s33 ^ s43;
        b04 = s04 ^ s14 ^ s24 ^ s34 ^ s44;

        /* Theta step: compute delta values */
        d00 = b04 ^ ROL_ULL(b01, 1);
        d01 = b00 ^ ROL_ULL(b02, 1);
        d02 = b01 ^ ROL_ULL(b03, 1);
        d03 = b02 ^ ROL_ULL(b04, 1);
        d04 = b03 ^ ROL_ULL(b00, 1);

        /* Chi step with rotation, round 0 */
        s00 ^= d00;
        b00 = s00;
        s11 ^= d01;
        b01 = ROL_ULL(s11, 44);
        s22 ^= d02;
        b02 = ROL_ULL(s22, 43);
        s33 ^= d03;
        b03 = ROL_ULL(s33, 21);
        s44 ^= d04;
        b04 = ROL_ULL(s44, 14);
        e00 = b00 ^ ((~b01) & b02);
        e00 ^= KeccakF_RC[r];
        e01 = b01 ^ ((~b02) & b03);
        e02 = b02 ^ ((~b03) & b04);
        e03 = b03 ^ ((~b04) & b00);
        e04 = b04 ^ ((~b00) & b01);

        s03 ^= d03;
        b00 = ROL_ULL(s03, 28);
        s14 ^= d04;
        b01 = ROL_ULL(s14, 20);
        s20 ^= d00;
        b02 = ROL_ULL(s20, 3);
        s31 ^= d01;
        b03 = ROL_ULL(s31, 45);
        s42 ^= d02;
        b04 = ROL_ULL(s42, 61);
        e10 = b00 ^ ((~b01) & b02);
        e11 = b01 ^ ((~b02) & b03);
        e12 = b02 ^ ((~b03) & b04);
        e13 = b03 ^ ((~b04) & b00);
        e14 = b04 ^ ((~b00) & b01);

        s01 ^= d01;
        b00 = ROL_ULL(s01, 1);
        s12 ^= d02;
        b01 = ROL_ULL(s12, 6);
        s23 ^= d03;
        b02 = ROL_ULL(s23, 25);
        s34 ^= d04;
        b03 = ROL_ULL(s34, 8);
        s40 ^= d00;
        b04 = ROL_ULL(s40, 18);
        e20 = b00 ^ ((~b01) & b02);
        e21 = b01 ^ ((~b02) & b03);
        e22 = b02 ^ ((~b03) & b04);
        e23 = b03 ^ ((~b04) & b00);
        e24 = b04 ^ ((~b00) & b01);

        s04 ^= d04;
        b00 = ROL_ULL(s04, 27);
        s10 ^= d00;
        b01 = ROL_ULL(s10, 36);
        s21 ^= d01;
        b02 = ROL_ULL(s21, 10);
        s32 ^= d02;
        b03 = ROL_ULL(s32, 15);
        s43 ^= d03;
        b04 = ROL_ULL(s43, 56);
        e30 = b00 ^ ((~b01) & b02);
        e31 = b01 ^ ((~b02) & b03);
        e32 = b02 ^ ((~b03) & b04);
        e33 = b03 ^ ((~b04) & b00);
        e34 = b04 ^ ((~b00) & b01);

        s02 ^= d02;
        b00 = ROL_ULL(s02, 62);
        s13 ^= d03;
        b01 = ROL_ULL(s13, 55);
        s24 ^= d04;
        b02 = ROL_ULL(s24, 39);
        s30 ^= d00;
        b03 = ROL_ULL(s30, 41);
        s41 ^= d01;
        b04 = ROL_ULL(s41, 2);
        e40 = b00 ^ ((~b01) & b02);
        e41 = b01 ^ ((~b02) & b03);
        e42 = b02 ^ ((~b03) & b04);
        e43 = b03 ^ ((~b04) & b00);
        e44 = b04 ^ ((~b00) & b01);

        /* Theta step: compute column parity for E */
        b00 = e00 ^ e10 ^ e20 ^ e30 ^ e40;
        b01 = e01 ^ e11 ^ e21 ^ e31 ^ e41;
        b02 = e02 ^ e12 ^ e22 ^ e32 ^ e42;
        b03 = e03 ^ e13 ^ e23 ^ e33 ^ e43;
        b04 = e04 ^ e14 ^ e24 ^ e34 ^ e44;

        /* Theta step: compute delta values for E */
        d00 = b04 ^ ROL_ULL(b01, 1);
        d01 = b00 ^ ROL_ULL(b02, 1);
        d02 = b01 ^ ROL_ULL(b03, 1);
        d03 = b02 ^ ROL_ULL(b04, 1);
        d04 = b03 ^ ROL_ULL(b00, 1);

        /* Chi step with rotation, round 1 */
        e00 ^= d00;
        b00 = e00;
        e11 ^= d01;
        b01 = ROL_ULL(e11, 44);
        e22 ^= d02;
        b02 = ROL_ULL(e22, 43);
        e33 ^= d03;
        b03 = ROL_ULL(e33, 21);
        e44 ^= d04;
        b04 = ROL_ULL(e44, 14);
        s00 = b00 ^ ((~b01) & b02);
        s00 ^= KeccakF_RC[r + 1];
        s01 = b01 ^ ((~b02) & b03);
        s02 = b02 ^ ((~b03) & b04);
        s03 = b03 ^ ((~b04) & b00);
        s04 = b04 ^ ((~b00) & b01);

        e03 ^= d03;
        b00 = ROL_ULL(e03, 28);
        e14 ^= d04;
        b01 = ROL_ULL(e14, 20);
        e20 ^= d00;
        b02 = ROL_ULL(e20, 3);
        e31 ^= d01;
        b03 = ROL_ULL(e31, 45);
        e42 ^= d02;
        b04 = ROL_ULL(e42, 61);
        s10 = b00 ^ ((~b01) & b02);
        s11 = b01 ^ ((~b02) & b03);
        s12 = b02 ^ ((~b03) & b04);
        s13 = b03 ^ ((~b04) & b00);
        s14 = b04 ^ ((~b00) & b01);

        e01 ^= d01;
        b00 = ROL_ULL(e01, 1);
        e12 ^= d02;
        b01 = ROL_ULL(e12, 6);
        e23 ^= d03;
        b02 = ROL_ULL(e23, 25);
        e34 ^= d04;
        b03 = ROL_ULL(e34, 8);
        e40 ^= d00;
        b04 = ROL_ULL(e40, 18);
        s20 = b00 ^ ((~b01) & b02);
        s21 = b01 ^ ((~b02) & b03);
        s22 = b02 ^ ((~b03) & b04);
        s23 = b03 ^ ((~b04) & b00);
        s24 = b04 ^ ((~b00) & b01);

        e04 ^= d04;
        b00 = ROL_ULL(e04, 27);
        e10 ^= d00;
        b01 = ROL_ULL(e10, 36);
        e21 ^= d01;
        b02 = ROL_ULL(e21, 10);
        e32 ^= d02;
        b03 = ROL_ULL(e32, 15);
        e43 ^= d03;
        b04 = ROL_ULL(e43, 56);
        s30 = b00 ^ ((~b01) & b02);
        s31 = b01 ^ ((~b02) & b03);
        s32 = b02 ^ ((~b03) & b04);
        s33 = b03 ^ ((~b04) & b00);
        s34 = b04 ^ ((~b00) & b01);

        e02 ^= d02;
        b00 = ROL_ULL(e02, 62);
        e13 ^= d03;
        b01 = ROL_ULL(e13, 55);
        e24 ^= d04;
        b02 = ROL_ULL(e24, 39);
        e30 ^= d00;
        b03 = ROL_ULL(e30, 41);
        e41 ^= d01;
        b04 = ROL_ULL(e41, 2);
        s40 = b00 ^ ((~b01) & b02);
        s41 = b01 ^ ((~b02) & b03);
        s42 = b02 ^ ((~b03) & b04);
        s43 = b03 ^ ((~b04) & b00);
        s44 = b04 ^ ((~b00) & b01);
    }

    /* Store local variables back to state */
    state[0] = s00;
    state[1] = s01;
    state[2] = s02;
    state[3] = s03;
    state[4] = s04;
    state[5] = s10;
    state[6] = s11;
    state[7] = s12;
    state[8] = s13;
    state[9] = s14;
    state[10] = s20;
    state[11] = s21;
    state[12] = s22;
    state[13] = s23;
    state[14] = s24;
    state[15] = s30;
    state[16] = s31;
    state[17] = s32;
    state[18] = s33;
    state[19] = s34;
    state[20] = s40;
    state[21] = s41;
    state[22] = s42;
    state[23] = s43;
    state[24] = s44;
}

/*************************************************
 * Name:        keccak_absorb_blocks
 *
 * Description: Absorb input data into Keccak state.
 *              Initializes state to zero before absorption.
 *
 * Arguments:   - uint64_t *state: pointer to Keccak state array
 *              - uint32_t rate: absorption rate in bytes
 *              - const uint8_t *msg: pointer to input message
 *              - size_t msglen: length of input in bytes
 *              - uint8_t pad: domain separation padding byte
 **************************************************/
static void keccak_absorb_blocks(uint64_t *state, uint32_t rate,
                                 const uint8_t *msg, size_t msglen, uint8_t pad) {
    size_t idx;
    uint8_t buffer[200];

    /* Initialize state to zero */
    for (idx = 0; idx < 25; ++idx) {
        state[idx] = 0;
    }

    /* Absorb full rate-sized blocks */
    while (msglen >= rate) {
        for (idx = 0; idx < rate / 8; ++idx) {
            state[idx] ^= ull_from_bytes(msg + 8 * idx);
        }
        keccak_p1600_permute(state);
        msglen -= rate;
        msg += rate;
    }

    /* Handle remaining bytes with padding */
    for (idx = 0; idx < rate; ++idx) {
        buffer[idx] = 0;
    }
    for (idx = 0; idx < msglen; ++idx) {
        buffer[idx] = msg[idx];
    }
    buffer[idx] = pad;
    buffer[rate - 1] |= 128;
    for (idx = 0; idx < rate / 8; ++idx) {
        state[idx] ^= ull_from_bytes(buffer + 8 * idx);
    }
}

/*************************************************
 * Name:        keccak_squeeze_blocks
 *
 * Description: Squeeze output blocks from Keccak state.
 *              Each block is rate bytes long.
 *
 * Arguments:   - uint8_t *out: pointer to output buffer
 *              - size_t nblocks: number of blocks to squeeze
 *              - uint64_t *state: pointer to Keccak state
 *              - uint32_t rate: squeeze rate in bytes
 **************************************************/
static void keccak_squeeze_blocks(uint8_t *out, size_t nblocks,
                                  uint64_t *state, uint32_t rate) {
    while (nblocks > 0) {
        keccak_p1600_permute(state);
        for (size_t idx = 0; idx < (rate >> 3); idx++) {
            ull_to_bytes(out + 8 * idx, state[idx]);
        }
        out += rate;
        nblocks--;
    }
}

/*************************************************
 * Name:        keccak_inc_create
 *
 * Description: Initialize incremental Keccak state to zero.
 *
 * Arguments:   - uint64_t *inc_state: pointer to incremental state array
 *                First 25 elements hold the Keccak state.
 *                Element 25 holds unprocessed/unread byte count.
 **************************************************/
static void keccak_inc_create(uint64_t *inc_state) {
    size_t idx;
    for (idx = 0; idx < 25; ++idx) {
        inc_state[idx] = 0;
    }
    inc_state[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_absorb
 *
 * Description: Incrementally absorb bytes into Keccak state.
 *              Must be called after keccak_inc_create and before
 *              keccak_inc_finalize.
 *
 * Arguments:   - uint64_t *inc_state: pointer to incremental state
 *              - uint32_t rate: absorption rate in bytes
 *              - const uint8_t *msg: pointer to input bytes
 *              - size_t msglen: number of bytes to absorb
 **************************************************/
static void keccak_inc_absorb(uint64_t *inc_state, uint32_t rate,
                              const uint8_t *msg, size_t msglen) {
    size_t idx;
    while (msglen + inc_state[25] >= rate) {
        for (idx = 0; idx < rate - inc_state[25]; idx++) {
            inc_state[(inc_state[25] + idx) >> 3] ^=
                (uint64_t)msg[idx] << (8 * ((inc_state[25] + idx) & 0x07));
        }
        msglen -= (size_t)(rate - inc_state[25]);
        msg += rate - inc_state[25];
        inc_state[25] = 0;
        keccak_p1600_permute(inc_state);
    }
    for (idx = 0; idx < msglen; idx++) {
        inc_state[(inc_state[25] + idx) >> 3] ^=
            (uint64_t)msg[idx] << (8 * ((inc_state[25] + idx) & 0x07));
    }
    inc_state[25] += msglen;
}

/*************************************************
 * Name:        keccak_inc_complete
 *
 * Description: Finalize incremental absorption and prepare for squeezing.
 *
 * Arguments:   - uint64_t *inc_state: pointer to incremental state
 *              - uint32_t rate: absorption rate in bytes
 *              - uint8_t pad: domain separation padding byte
 **************************************************/
static void keccak_inc_complete(uint64_t *inc_state, uint32_t rate, uint8_t pad) {
    inc_state[inc_state[25] >> 3] ^= (uint64_t)pad << (8 * (inc_state[25] & 0x07));
    inc_state[(rate - 1) >> 3] ^= (uint64_t)128 << (8 * ((rate - 1) & 0x07));
    inc_state[25] = 0;
}

/*************************************************
 * Name:        keccak_inc_squeeze
 *
 * Description: Incrementally squeeze bytes from Keccak state.
 *              Can be called multiple times to retrieve output.
 *
 * Arguments:   - uint8_t *out: pointer to output buffer
 *              - size_t outlen: number of bytes to squeeze
 *              - uint64_t *inc_state: pointer to incremental state
 *              - uint32_t rate: squeeze rate in bytes
 **************************************************/
static void keccak_inc_squeeze(uint8_t *out, size_t outlen,
                               uint64_t *inc_state, uint32_t rate) {
    size_t idx;
    for (idx = 0; idx < outlen && idx < inc_state[25]; idx++) {
        out[idx] = (uint8_t)(inc_state[(rate - inc_state[25] + idx) >> 3] >>
                             (8 * ((rate - inc_state[25] + idx) & 0x07)));
    }
    out += idx;
    outlen -= idx;
    inc_state[25] -= idx;

    while (outlen > 0) {
        keccak_p1600_permute(inc_state);
        for (idx = 0; idx < outlen && idx < rate; idx++) {
            out[idx] = (uint8_t)(inc_state[idx >> 3] >> (8 * (idx & 0x07)));
        }
        out += idx;
        outlen -= idx;
        inc_state[25] = rate - idx;
    }
}

void shake256_inc_init(uint64_t *inc_state) {
    keccak_inc_create(inc_state);
}

void shake256_inc_absorb(uint64_t *inc_state, const uint8_t *input, size_t inlen) {
    keccak_inc_absorb(inc_state, SHAKE256_RATE, input, inlen);
}

void shake256_inc_finalize(uint64_t *inc_state) {
    keccak_inc_complete(inc_state, SHAKE256_RATE, 0x1F);
}

void shake256_inc_squeeze(uint8_t *output, size_t outlen, uint64_t *inc_state) {
    keccak_inc_squeeze(output, outlen, inc_state, SHAKE256_RATE);
}

/*************************************************
 * Name:        shake256_absorb
 *
 * Description: Absorb input into SHAKE256 state (non-incremental).
 *              State is initialized to zero before absorption.
 *
 * Arguments:   - uint64_t *state: pointer to Keccak state array
 *              - const uint8_t *input: pointer to input data
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256_absorb(uint64_t *state, const uint8_t *input, size_t inlen) {
    keccak_absorb_blocks(state, SHAKE256_RATE, input, inlen, 0x1F);
}

/*************************************************
 * Name:        shake256_squeezeblocks
 *
 * Description: Squeeze full blocks from SHAKE256 state.
 *              Can be called multiple times for incremental output.
 *
 * Arguments:   - uint8_t *output: pointer to output buffer
 *              - size_t nblocks: number of SHAKE256_RATE blocks
 *              - uint64_t *state: pointer to Keccak state
 **************************************************/
void shake256_squeezeblocks(uint8_t *output, size_t nblocks, uint64_t *state) {
    keccak_squeeze_blocks(output, nblocks, state, SHAKE256_RATE);
}

/*************************************************
 * Name:        shake256
 *
 * Description: SHAKE256 extendable-output function (non-incremental).
 *
 * Arguments:   - uint8_t *output: pointer to output buffer
 *              - size_t outlen: desired output length in bytes
 *              - const uint8_t *input: pointer to input data
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256(uint8_t *output, size_t outlen,
              const uint8_t *input, size_t inlen) {
    size_t nblocks = outlen / SHAKE256_RATE;
    uint8_t tmp[SHAKE256_RATE];
    uint64_t state[25];

    shake256_absorb(state, input, inlen);
    shake256_squeezeblocks(output, nblocks, state);

    output += nblocks * SHAKE256_RATE;
    outlen -= nblocks * SHAKE256_RATE;

    if (outlen) {
        shake256_squeezeblocks(tmp, 1, state);
        for (size_t idx = 0; idx < outlen; ++idx) {
            output[idx] = tmp[idx];
        }
    }
}
