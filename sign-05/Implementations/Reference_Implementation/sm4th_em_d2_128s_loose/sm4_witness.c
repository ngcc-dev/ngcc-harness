/*
* The sm4_extend_witness function for SM4th-d2, generating extend witness
*/

#include "sm4.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/* Functions used for SM4 OWF in SM4th */

 /* ================================================================== *
 * SM4 round key initialization
 * Output: expand_key[36] where each element is a sm4_word_t (4 bytes)
 * Stores k0-k35: k0-k3 are XORed with FK, k4-k35 are derived from key schedule                                              *
 * ================================================================== */

void sm4_init_round_keys(uint8_t* expand_keys, const uint8_t* key)
{
    uint32_t k0 = sm4_load_be_u32(key + 0) ^ FK[0];
    uint32_t k1 = sm4_load_be_u32(key + 4) ^ FK[1];
    uint32_t k2 = sm4_load_be_u32(key + 8) ^ FK[2];
    uint32_t k3 = sm4_load_be_u32(key + 12) ^ FK[3];

    sm4_store_be_u32(k0, expand_keys + 0);
    sm4_store_be_u32(k1, expand_keys + 4);
    sm4_store_be_u32(k2, expand_keys + 8);
    sm4_store_be_u32(k3, expand_keys + 12);

    SM4_KEY ks;
    SM4_set_key(key, &ks);
    for (unsigned int index = 0; index < SM4_ROUNDS; ++index) {
        sm4_store_be_u32(ks.rk[index], expand_keys + (4 + index) * 4);
    }
}

/* ================================================================== *
 *  SM4 encryption state recording                                    *
 * ================================================================== */

void sm4_get_enc_state(uint8_t* enc_states, const uint8_t* in, const uint8_t* expand_keys)
{
    SM4_KEY ks;
    for (unsigned int index = 0; index < SM4_ROUNDS; ++index) {
        ks.rk[index] = sm4_load_be_u32(expand_keys + (4 + index) * 4);
    }

    sm4_store_be_u32(sm4_load_be_u32(in + 0), enc_states + 0);
    sm4_store_be_u32(sm4_load_be_u32(in + 4), enc_states + 4);
    sm4_store_be_u32(sm4_load_be_u32(in + 8), enc_states + 8);
    sm4_store_be_u32(sm4_load_be_u32(in + 12), enc_states + 12);

    uint8_t out[16];
    uint32_t states[SM4_ROUNDS];
    SM4_encrypt_store_state(in, out, &ks, states);
    (void)out;

    for (unsigned int index = 0; index < SM4_ROUNDS; ++index) {
        sm4_store_be_u32(states[index], enc_states + (4 + index) * 4);
    }
}

void sm4_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key, const uint8_t* in) {
  const bool use_em             = is_em(params);

#if !defined(NDEBUG)
  uint8_t* const w_out = w;
#endif

// For em, key is owf_key, is SM4 input
// in is owf_input, is SM4 key
// Here, we switch, so in is SM4 input, key is SM4 key below
if (use_em) {
    // switch input and key for EM
    const uint8_t* tmp = key;
    key                = in;
    in                 = tmp;
  }

// Step 3-6
// expanded key, 36 * 4 = 144 bytes
uint8_t expand_keys[144];
sm4_init_round_keys(expand_keys, key);

// Step 7 - 10
if (!use_em) {
    // store the 36 * 4 = 144 bytes of the expanded key
    memcpy(w, expand_keys, 144);
    w += 144;
} 
// for em, don't need to store the owf input

assert(w - w_out == params->lke / 8);

// Step 14 - 25
uint8_t enc_states[144];
sm4_get_enc_state(enc_states, in, expand_keys);

if (!use_em) {
    // store the X[4-31]
    for (unsigned int i = 4; i != 32; ++i) {
        // position, copied element, copied size
        memcpy(w, enc_states + i * 4, sizeof(sm4_word_t));
        // position goes forward by 4 bytes (size of sm4_word_t)
        w += sizeof(sm4_word_t);
    }
} else {
    // store the X[0-31], X[32-35] can be computed from the xor of output and SM4 input, so not storeds
    for (unsigned int i = 0; i != SM4_ROUNDS; ++i) {
        // position, copied element, copied size
        memcpy(w, enc_states + i * 4, sizeof(sm4_word_t));
        // position goes forward by 4 bytes (size of sm4_word_t)
        w += sizeof(sm4_word_t);
    }
}

assert(w - w_out == params->ell / 8);
}

