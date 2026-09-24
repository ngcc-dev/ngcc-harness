#ifndef UBLOCK_CONSTRAINTS_H
#define UBLOCK_CONSTRAINTS_H

#include <stdint.h>

#include "fields.h"

/*
 * uBlockith.KeyExpCstrnts
 *
 * Witness layout (non-EM):
 *   w[0..255]     : k[0] = (k0^(0), k1^(0), k2^(0), k3^(0)), each 64 bits
 *   w[256..]      : per round i in [0, UBLOCK_ROUNDS), 64-bit witness chunk
 *                   for k2_wit^(i) (the next-round k0 candidate).
 *
 * Prover updates secret + tag and emits degree-3 constraints o:
 *   o^(i) = k2_wit^(i) + t2^(i),  i in [0, UBLOCK_ROUNDS).
 *
 * Verifier updates key and emits aligned constraint keys:
 *   o_key^(i) = Delta^2 * q(k2_wit^(i)) + t2_key^(i),
 * where t2_key is the verifier-side degree-3 aligned key from the same round.
 *
 * Buffer sizes:
 *   k_out / k_out_tag / k_out_key : (UBLOCK_ROUNDS + 1) * 256 bits
 *   o / o_tag_deg* / o_key        : UBLOCK_ROUNDS * 64 bits
 */
void ublock_SSS_expkey_constraints_prover(uint8_t* k_out,
                                          bf256_t* k_out_tag,
                                          uint8_t* o,
                                          bf256_t* o_tag_deg0,
                                          bf256_t* o_tag_deg1,
                                          bf256_t* o_tag_deg2,
                                          const uint8_t* w,
                                          const bf256_t* w_tag);

void ublock_SSS_expkey_constraints_verifier(bf256_t* k_out_key,
                                            bf256_t* o_key,
                                            const bf256_t* w_key,
                                            bf256_t delta);

/*
 * uBlockith.EncCstrnts
 *
 * Inputs are commitments to:
 *   - plaintext in  (256 bits)
 *   - ciphertext out (256 bits)
 *   - enc witness w  ((UBLOCK_ROUNDS/2 - 1) * 256 bits, i.e. 11 states for R=24)
 *   - expanded round keys k_bar ((UBLOCK_ROUNDS + 1) * 256 bits)
 *
 * Output constraints are degree-3 commitments:
 *   o of size (UBLOCK_ROUNDS/2) * 256 bits (12 blocks for R=24).
 */
void ublock_SSS_enc_constraints_prover(uint8_t* o,
                                       bf256_t* o_tag_deg0,
                                       bf256_t* o_tag_deg1,
                                       bf256_t* o_tag_deg2,
                                       const uint8_t* in,
                                       const bf256_t* in_tag,
                                       const uint8_t* out,
                                       const bf256_t* out_tag,
                                       const uint8_t* w,
                                       const bf256_t* w_tag,
                                       const uint8_t* k_bar,
                                       const bf256_t* k_bar_tag);

void ublock_SSS_enc_constraints_verifier(bf256_t* o_key,
                                         const bf256_t* in_key,
                                         const bf256_t* out_key,
                                         const bf256_t* w_key,
                                         const bf256_t* k_bar_key,
                                         bf256_t delta);

#endif /* UBLOCK_CONSTRAINTS_H */

