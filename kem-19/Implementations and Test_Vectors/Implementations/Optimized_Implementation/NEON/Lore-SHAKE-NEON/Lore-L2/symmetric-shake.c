#include "symmetric.h"
#include "fips202.h"

#include <string.h>

/*************************************************
* Name:        xof_absorb
*
* Description: Compatibility absorb step for Lore's XOF interface.
*              The domain-separation bytes x and y are concatenated to seed.
*
* Arguments:   - xof_state *s: pointer to compatibility state
*              - const uint8_t seed[LORE_SYMBYTES]: pointer to seed
*              - uint8_t x: first domain-separation byte
*              - uint8_t y: second domain-separation byte
**************************************************/
void xof_absorb(xof_state *s, const uint8_t seed[LORE_SYMBYTES], uint8_t x, uint8_t y)
{
    uint8_t extseed[LORE_SYMBYTES + 2];

    memcpy(extseed, seed, LORE_SYMBYTES);
    extseed[LORE_SYMBYTES] = x;
    extseed[LORE_SYMBYTES + 1] = y;

    shake128_absorb_once(s, extseed, sizeof(extseed));
}

/*************************************************
* Name:        xof_squeezeblocks
*
* Description: Compatibility squeeze step for Lore's XOF interface.
*              Squeezes full blocks of SHAKE128_RATE bytes each.
*
* Arguments:   - uint8_t *out: pointer to output buffer
*              - size_t outblocks: number of full blocks to be squeezed
*              - xof_state *s: pointer to compatibility state
**************************************************/
void xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *s)
{
    shake128_squeezeblocks(out, outblocks, s);
}

/*************************************************
* Name:        prf
*
* Description: Compatibility PRF interface.
*              Concatenates key and nonce, then expands with shake256 wrapper.
*
* Arguments:   - uint8_t *out: pointer to output buffer
*              - size_t outlen: requested output length in bytes
*              - const uint8_t key[LORE_SYMBYTES]: pointer to key
*              - uint8_t nonce: single-byte nonce
**************************************************/
void prf(uint8_t *out, size_t outlen, const uint8_t key[LORE_SYMBYTES], uint8_t nonce)
{
    uint8_t extkey[LORE_SYMBYTES + 1];

    memcpy(extkey, key, LORE_SYMBYTES);
    extkey[LORE_SYMBYTES] = nonce;

    shake256(out, outlen, extkey, sizeof(extkey));
}

/**
 * @brief Compatibility PRF used for rejection sampling key derivation.
 *        Concatenates secret key material and ciphertext, then expands.
 *
 * @param[out] out      Pointer to the output buffer (LORE_SYMBYTES bytes).
 * @param[in]  key      Pointer to the secret key (LORE_SYMBYTES bytes).
 * @param[in]  input    Pointer to the ciphertext (LORE_CIPHERTEXTBYTES bytes).
 */
void lore_shake256_rkprf(unsigned char out[LORE_SYMBYTES],
                         const unsigned char key[LORE_SYMBYTES],
                         const unsigned char input[LORE_CIPHERTEXTBYTES])
{
    unsigned char buf[LORE_SYMBYTES + LORE_CIPHERTEXTBYTES];

    memcpy(buf, key, LORE_SYMBYTES);
    memcpy(buf + LORE_SYMBYTES, input, LORE_CIPHERTEXTBYTES);

    shake256(out, LORE_SYMBYTES, buf, sizeof(buf));
}
