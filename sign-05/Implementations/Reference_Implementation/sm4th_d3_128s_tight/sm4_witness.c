/*
 * Extended-witness generation for sm4th_d3_128s_tight.
 *
 * This variant keeps the public SM4 OWF unchanged, but changes what the
 * private witness commits to.  Most non-anchor SM4 words are still stored as
 * full 32-bit words.  Selected "anchor" words are replaced by 4-bit norm
 * witnesses, one nibble per S-box byte.  The constraint system later uses the
 * norm witness to reconstruct the corresponding S-box output virtually.
 *
 * Non-EM witness layout:
 *
 *   Key schedule part, params->lke = 1024 bits = 128 bytes:
 *     bytes  0..15   : full K0..K3, after SM4 FK whitening
 *     bytes 16..31   : packed norm nibbles for K4,K8,...,K32
 *     bytes 32..127  : full non-anchor K words among K4..K35
 *
 *   Encryption part, params->lenc = 784 bits = 98 bytes:
 *     bytes  0..13   : packed norm nibbles for X5,X9,...,X29
 *     bytes 14..97   : full non-anchor X words among X4..X31
 *
 * A boxed word such as K16 or X17 is therefore not stored directly.  Instead
 * this file stores norm(affine(S-box input)) = x^-17 for each of the 4 bytes
 * feeding the SM4 S-box that produces the boxed word.
 */

#include "fields.h"
#include "sm4.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/* Functions used for the SM4 OWF in SM4th. */

/* ================================================================== *
 * SM4 round-key initialization                                       *
 * ================================================================== *
 *
 * expand_keys contains 36 big-endian 32-bit words:
 *
 *   K0..K3   = MK0..MK3 xor FK0..FK3
 *   K4..K35  = SM4 key-schedule output words
 *
 * The constraints work over these K words, not over the raw 128-bit master
 * key, so this routine materializes the exact word sequence used by the SM4
 * recurrence.
 */

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
 * SM4 encryption-state recording                                     *
 * ================================================================== *
 *
 * enc_states contains 36 big-endian 32-bit words:
 *
 *   X0..X3   = plaintext input words
 *   X4..X35  = internal SM4 round states after each of the 32 rounds
 *
 * SM4_encrypt_store_state records only the newly generated round words, so we
 * first write X0..X3 and then append the 32 recorded words at positions X4..X35.
 */

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

static const uint8_t SM4_INV_NORM_NIBBLE_LUT[256] = {
  0x9, 0xe, 0x3, 0xa, 0xa, 0x8, 0xa, 0xa, 0xc, 0x8, 0x6, 0x1, 0xf, 0x8, 0x2, 0x4,
  0x5, 0xd, 0xc, 0x7, 0x8, 0x9, 0x9, 0xa, 0x7, 0x4, 0x7, 0x3, 0x3, 0x6, 0x9, 0xf,
  0x7, 0x8, 0x6, 0x4, 0x2, 0x9, 0xd, 0xa, 0x2, 0xd, 0x2, 0x6, 0xc, 0xb, 0x7, 0xc,
  0x7, 0x1, 0xc, 0x3, 0xe, 0x7, 0xe, 0xb, 0xa, 0x8, 0x1, 0x1, 0xb, 0xd, 0xd, 0x7,
  0x9, 0x2, 0x7, 0x3, 0xe, 0xc, 0x5, 0x1, 0x3, 0x1, 0x5, 0x6, 0xe, 0x3, 0xc, 0xd,
  0xd, 0x1, 0x4, 0x5, 0x8, 0xf, 0xc, 0x3, 0x4, 0xa, 0xb, 0x6, 0xc, 0x2, 0x3, 0x3,
  0xc, 0x2, 0xe, 0xe, 0xc, 0x5, 0x8, 0x4, 0x1, 0x7, 0x2, 0x4, 0x4, 0xf, 0x2, 0x8,
  0xb, 0xf, 0x1, 0x8, 0x8, 0x0, 0x6, 0x5, 0x5, 0x9, 0x3, 0xd, 0x4, 0x7, 0xd, 0xa,
  0x1, 0x5, 0x5, 0x9, 0x1, 0x2, 0x6, 0xa, 0x2, 0x6, 0x9, 0x8, 0xb, 0x2, 0x9, 0x4,
  0xb, 0x1, 0x9, 0xb, 0x8, 0x2, 0xd, 0x1, 0xf, 0xf, 0x3, 0x4, 0x3, 0xa, 0xd, 0xb,
  0xd, 0x7, 0xe, 0x9, 0xb, 0xb, 0x3, 0x4, 0x5, 0x4, 0x4, 0xf, 0x7, 0x9, 0xa, 0x5,
  0x2, 0xb, 0xe, 0xa, 0x9, 0xa, 0xd, 0x2, 0xe, 0x5, 0xf, 0xe, 0xd, 0x3, 0xe, 0xc,
  0xc, 0xc, 0xf, 0x5, 0xb, 0x6, 0x5, 0xc, 0x7, 0x9, 0xb, 0x9, 0xb, 0xe, 0xc, 0x8,
  0x1, 0x8, 0x3, 0xe, 0xd, 0xf, 0x6, 0xf, 0xf, 0x9, 0x7, 0x8, 0x1, 0x2, 0xe, 0xf,
  0xe, 0x5, 0xa, 0x6, 0x4, 0x6, 0x3, 0x6, 0xf, 0xd, 0x7, 0x2, 0xc, 0xd, 0xf, 0xb,
  0x1, 0x6, 0xe, 0x7, 0xb, 0x1, 0x5, 0xa, 0x8, 0x4, 0xa, 0x4, 0x6, 0x6, 0xf, 0x5
};

static uint8_t sm4_inv_norm_nibble(uint8_t sbox_input) {
  /*
   * Convert one SM4 S-box input byte into the 4-bit norm witness stored in w.
   *
   * This table is the SM4-specific analogue of the faest-avx invnorm witness
   * compression path.  For each possible S-box input byte it has already
   * applied the SM4 affine transform, computed h = x^-17 in the SM4 byte field,
   * and converted h to its 4-bit coordinate in the subfield basis:
   *
   *   1, 0x0c, 0x50, 0x2a
   *
   * The single x = 0 case maps to nibble 0; the norm constraint handles that
   * patched inverse case in the same way as the old scalar computation.
   */
  return SM4_INV_NORM_NIBBLE_LUT[sbox_input];
}

// generate 4 norm witness nibble 
static void sm4_pack_norm_nibble(uint8_t* out, unsigned int idx, uint8_t nibble) {
  /*
   * Store one 4-bit norm coordinate.
   *
   * Even indices occupy the low nibble of out[idx/2], odd indices occupy the
   * high nibble.  A key anchor has 4 nibbles, one for each byte of the SM4 word;
   * the same packing is used for encryption anchors.
   */
  assert((nibble & 0xf0u) == 0);
  if ((idx & 1u) == 0) {
    out[idx >> 1u] = (uint8_t)((out[idx >> 1u] & 0xf0u) | nibble);
  } else {
    out[idx >> 1u] = (uint8_t)((out[idx >> 1u] & 0x0fu) | (uint8_t)(nibble << 4));
  }
}

static bool sm4_is_norm_key_anchor(unsigned int word_idx) {
  /*
   * Key-schedule anchors: K4,K8,K12,K16,K20,K24,K28,K32.
   *
   * These are exactly the words whose full 32-bit commitment is removed from
   * the key witness and replaced by four packed norm nibbles.
   */
  return word_idx >= 4u && word_idx <= 32u && ((word_idx & 3u) == 0u);
}

static bool sm4_is_norm_enc_anchor(unsigned int word_idx) {
  /*
   * Encryption anchors: X5,X9,X13,X17,X21,X25,X29.
   *
   * X33 is public output-side material, so it is not part of the private
   * encryption witness and is intentionally not listed here.
   */
  return word_idx >= 5u && word_idx <= 29u && ((word_idx & 3u) == 1u);
}

void sm4_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key, const uint8_t* in) {
  const bool use_em             = is_em(params);

#if !defined(NDEBUG)
  uint8_t* const w_out = w;
#endif

/*
 * In the ordinary SM4th OWF, key is the SM4 key and in is the SM4 plaintext.
 * In EM mode, the caller uses the opposite naming convention: key is the SM4
 * input block and in is the SM4 key.  Swap them here so the rest of this
 * function can always read:
 *
 *   key = SM4 key
 *   in  = SM4 plaintext/input block
 */
if (use_em) {
    const uint8_t* tmp = key;
    key                = in;
    in                 = tmp;
  }

/*
 * Materialize K0..K35 once.  The norm witness needs true S-box inputs for both
 * key anchors and encryption anchors, and the full-word witness needs the
 * non-anchor K words.
 */
uint8_t expand_keys[144];
sm4_init_round_keys(expand_keys, key);

if (!use_em) {
    /*
     * Non-EM key witness:
     *
     *   w[0..15]   : K0..K3 as full words
     *   w[16..31]  : 8 anchors * 4 nibbles = 32 nibbles = 16 bytes
     *   w[32..127] : 24 non-anchor words * 4 bytes
     *
     * For an anchor word K_word, the stored norm corresponds to the byte input
     * of the T' S-box:
     *
     *   K[word-3] xor K[word-2] xor K[word-1] xor CK[word-4]
     *
     * The verifier/prover constraints reconstruct the S-box output from that
     * norm witness and then derive K_word virtually as (K[word-4] xor L'(S)).
     */

    // here, key norm is the address of w[16..31]
    // key full is the address of w[32..127]
    uint8_t* const key_norm = w + 16u;
    uint8_t* key_full = w + 32u;

    memset(key_norm, 0, 16u);
    memcpy(w, expand_keys, 16u);

    for (unsigned int word = 4u; word != 36u; ++word) {
      if (sm4_is_norm_key_anchor(word)) {
        const unsigned int round = word - 4u;
        const uint8_t* k1 = expand_keys + (word - 3u) * 4u;
        const uint8_t* k2 = expand_keys + (word - 2u) * 4u;
        const uint8_t* k3 = expand_keys + (word - 1u) * 4u;
        uint8_t ck[4];
        ck[0] = (uint8_t)(CK[round] >> 24);
        ck[1] = (uint8_t)(CK[round] >> 16);
        ck[2] = (uint8_t)(CK[round] >> 8);
        ck[3] = (uint8_t)CK[round];

        const unsigned int anchor = (word - 4u) >> 2u;
        for (unsigned int j = 0; j != 4u; ++j) {
          /*
           * Store the norm nibble for byte j of this anchor's S-box input.
           * The anchor index is word-based; multiplying by 4 selects the first
           * byte nibble belonging to the anchor.
           */

          // for example, put K16, 4 nibbles into 2 bytes, key_norm[6] and key_norm[7]
          const uint8_t sbox_input = (uint8_t)(k1[j] ^ k2[j] ^ k3[j] ^ ck[j]);
          sm4_pack_norm_nibble(key_norm, anchor * 4u + j, sm4_inv_norm_nibble(sbox_input));
        }
      } else {
        /*
         * Non-anchor K words are still committed as ordinary full words.  They
         * are checked by the usual S-box IO constraints against neighboring
         * virtual/full K words.
         */
        memcpy(key_full, expand_keys + word * 4u, sizeof(sm4_word_t));
        key_full += sizeof(sm4_word_t);
      }
    }
    w += params->lke / 8u;
}

/*
 * In non-EM this proves the key part wrote exactly lke bits.  In EM, lke is the
 * 128-bit input block witness already handled by the caller convention above.
 */
assert(w - w_out == params->lke / 8);

/*
 * Materialize X0..X35 once.  X0..X3 are public/plain input in non-EM and are
 * not stored here; X32..X35 are checked from the public ciphertext/output side.
 */
uint8_t enc_states[144];
sm4_get_enc_state(enc_states, in, expand_keys);

if (!use_em) {
    /*
     * Non-EM encryption witness:
     *
     *   w[0..13]  : 7 anchors * 4 nibbles = 28 nibbles = 14 bytes
     *   w[14..97] : 21 non-anchor words * 4 bytes
     *
     * For an anchor word X_word, the stored norm corresponds to the byte input
     * of the round S-box:
     *
     *   X[word-3] xor X[word-2] xor X[word-1] xor K[word]
     *
     * The constraints reconstruct X_word virtually as X[word-4] xor L(S).
     */
    uint8_t* const enc_norm = w;
    uint8_t* enc_full = w + 14u;

    memset(enc_norm, 0, 14u);
    for (unsigned int word = 4u; word != 32u; ++word) {
      if (sm4_is_norm_enc_anchor(word)) {
        const unsigned int round = word - 4u;
        const uint8_t* x1 = enc_states + (round + 1u) * 4u;
        const uint8_t* x2 = enc_states + (round + 2u) * 4u;
        const uint8_t* x3 = enc_states + (round + 3u) * 4u;
        const uint8_t* rk = expand_keys + (round + 4u) * 4u;

        const unsigned int anchor = (word - 5u) >> 2u;
        for (unsigned int j = 0; j != 4u; ++j) {
          /*
           * Store the norm nibble for byte j of this encryption anchor's
           * S-box input.  K[round+4] is the SM4 round key used in this round.
           */
          const uint8_t sbox_input = (uint8_t)(x1[j] ^ x2[j] ^ x3[j] ^ rk[j]);
          sm4_pack_norm_nibble(enc_norm, anchor * 4u + j, sm4_inv_norm_nibble(sbox_input));
        }
      } else {
        /*
         * Non-anchor X words are full 32-bit witness words.  The public output
         * side supplies X32..X35, so this loop stores only X4..X31.
         */
        memcpy(enc_full, enc_states + word * 4u, sizeof(sm4_word_t));
        enc_full += sizeof(sm4_word_t);
      }
    }
    w += params->lenc / 8u;
} else {
    /*
     * EM mode keeps the old full-state layout.  Store X0..X31; X32..X35 are
     * recoverable from the EM output relation and do not need private storage.
     * The norm layout above is only used by the non-EM sm4th_d3_128s_tight path.
     */
    for (unsigned int i = 0; i != SM4_ROUNDS; ++i) {
        memcpy(w, enc_states + i * 4, sizeof(sm4_word_t));
        w += sizeof(sm4_word_t);
    }
}

/* Final guard: the generated extended witness must match params->ell exactly. */
assert(w - w_out == params->ell / 8);
}
