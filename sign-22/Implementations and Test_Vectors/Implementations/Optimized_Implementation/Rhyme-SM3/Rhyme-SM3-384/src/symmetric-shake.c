#include "rhyme_xof.h"
#include "params.h"
#include "symmetric.h"
#include <stdint.h>

#ifndef RHYME_USE_AES
void rhyme_shake128_stream_init(keccak_state *state,
                                 const uint8_t seed[SEEDBYTES],
                                 uint16_t nonce) {
    uint8_t t[2];
    t[0] = nonce;
    t[1] = nonce >> 8;

    rhyme_shake128_init(state);
    rhyme_shake128_absorb(state, seed, SEEDBYTES);
    rhyme_shake128_absorb(state, t, 2);
    rhyme_shake128_finalize(state);
}

void rhyme_shake256_stream_init(keccak_state *state,
                                 const uint8_t seed[CRHBYTES], uint16_t nonce) {
    uint8_t t[2];
    t[0] = nonce;
    t[1] = nonce >> 8;

    rhyme_shake256_init(state);
    rhyme_shake256_absorb(state, seed, CRHBYTES);
    rhyme_shake256_absorb(state, t, 2);
    rhyme_shake256_finalize(state);
}

void rhyme_shake256_absorb_twice(keccak_state *state, const uint8_t *in1,
                                  size_t in1len, const uint8_t *in2,
                                  size_t in2len) {
    rhyme_shake256_init(state);
    rhyme_shake256_absorb(state, in1, in1len);
    rhyme_shake256_absorb(state, in2, in2len);
    rhyme_shake256_finalize(state);
}
#endif /* !RHYME_USE_AES */
