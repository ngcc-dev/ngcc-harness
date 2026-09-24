#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "symmetric.h"

/*************************************************
* Name:        COMPASS_KEM_xof_absorb
*
* Description: Absorb step of the XOF specialized for the COMPASS_KEM context.
*              Uses the ICCS-provided pseudoXOF auxiliary function to pre-compute
*              a buffer of pseudo-random bytes.
*
* Arguments:   - xof_state *state: pointer to (uninitialized) output XOF state
*              - const uint8_t *seed: pointer to COMPASS_KEM_SYMBYTES input seed
*              - uint8_t x: additional byte of input (matrix row index)
*              - uint8_t y: additional byte of input (matrix column index)
**************************************************/
void COMPASS_KEM_xof_absorb(xof_state *state,
                            const uint8_t seed[COMPASS_KEM_SYMBYTES],
                            uint8_t x,
                            uint8_t y)
{
  uint8_t extseed[COMPASS_KEM_SYMBYTES + 2];

  memcpy(extseed, seed, COMPASS_KEM_SYMBYTES);
  extseed[COMPASS_KEM_SYMBYTES] = x;
  extseed[COMPASS_KEM_SYMBYTES + 1] = y;

  pseudoXOF(XOF_BUFFER_SIZE * 8,
            extseed,
            (COMPASS_KEM_SYMBYTES + 2) * 8,
            state->buf);
  state->pos = 0;
}

/*************************************************
* Name:        COMPASS_KEM_xof_squeezeblocks
*
* Description: Squeeze nblocks * XOF_BLOCKBYTES bytes from the XOF state
*              into the output buffer. The XOF state must have been
*              initialized by a prior call to COMPASS_KEM_xof_absorb.
*
* Arguments:   - uint8_t *out: pointer to output buffer
*              - size_t nblocks: number of XOF blocks to squeeze
*              - xof_state *state: pointer to initialized XOF state
**************************************************/
void COMPASS_KEM_xof_squeezeblocks(uint8_t *out,
                                   size_t nblocks,
                                   xof_state *state)
{
  size_t bytes = nblocks * XOF_BLOCKBYTES;
  memcpy(out, state->buf + state->pos, bytes);
  state->pos += bytes;
}

/*************************************************
* Name:        COMPASS_KEM_prf
*
* Description: Pseudo-Random Function (PRF) using the ICCS-provided
*              pseudoXOF auxiliary function. Concatenates the key
*              with a nonce byte and generates outlen bytes of
*              pseudo-random output.
*
* Arguments:   - uint8_t *out: pointer to output buffer
*              - size_t outlen: number of requested output bytes
*              - const uint8_t *key: pointer to the key
*                                    (of length COMPASS_KEM_SYMBYTES)
*              - uint8_t nonce: single-byte nonce (public PRF input)
**************************************************/
void COMPASS_KEM_prf(uint8_t *out,
                     size_t outlen,
                     const uint8_t key[COMPASS_KEM_SYMBYTES],
                     uint8_t nonce)
{
  uint8_t extkey[COMPASS_KEM_SYMBYTES + 1];

  memcpy(extkey, key, COMPASS_KEM_SYMBYTES);
  extkey[COMPASS_KEM_SYMBYTES] = nonce;

  pseudoXOF(outlen * 8,
            extkey,
            (COMPASS_KEM_SYMBYTES + 1) * 8,
            out);
}

/*************************************************
* Name:        COMPASS_KEM_rkprf
*
* Description: Rejection-key Pseudo-Random Function (RkPRF) using the
*              ICCS-provided pseudoXOF auxiliary function. Used in the
*              decapsulation routine to compute a pseudo-random shared
*              secret when ciphertext verification fails.
*
* Arguments:   - uint8_t out[COMPASS_KEM_SSBYTES]: output shared secret
*              - const uint8_t *key: pointer to the rejection key
*                                    (of length COMPASS_KEM_SYMBYTES)
*              - const uint8_t *input: pointer to the ciphertext
*                                      (of length COMPASS_KEM_CIPHERTEXTBYTES)
**************************************************/
void COMPASS_KEM_rkprf(uint8_t out[COMPASS_KEM_SSBYTES],
                       const uint8_t key[COMPASS_KEM_SYMBYTES],
                       const uint8_t input[COMPASS_KEM_CIPHERTEXTBYTES])
{
  unsigned long long ct_bytes = COMPASS_KEM_CIPHERTEXTBYTES;
  unsigned long long combined_len = COMPASS_KEM_SYMBYTES + ct_bytes;
  uint8_t combined[COMPASS_KEM_SYMBYTES + COMPASS_KEM_CIPHERTEXTBYTES];

  memcpy(combined, key, COMPASS_KEM_SYMBYTES);
  memcpy(combined + COMPASS_KEM_SYMBYTES, input, ct_bytes);

  pseudoXOF(COMPASS_KEM_SSBYTES * 8,
            combined,
            combined_len * 8,
            out);
}
