#include "symmetric.h"

#include <string.h>

void xof_absorb(xof_state *s, const uint8_t seed[LORE_SYMBYTES], uint8_t x, uint8_t y)
{
    uint8_t extseed[LORE_SYMBYTES + 2];

    memcpy(extseed, seed, LORE_SYMBYTES);
    extseed[LORE_SYMBYTES] = x;
    extseed[LORE_SYMBYTES + 1] = y;

    sm3_xof128_absorb_once(s, extseed, sizeof(extseed));
}

void xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *s)
{
    sm3_xof128_squeezeblocks(out, outblocks, s);
}

void prf(uint8_t *out, size_t outlen, const uint8_t key[LORE_SYMBYTES], uint8_t nonce)
{
    uint8_t extkey[LORE_SYMBYTES + 1];

    memcpy(extkey, key, LORE_SYMBYTES);
    extkey[LORE_SYMBYTES] = nonce;

    sm3_xof256(out, outlen, extkey, sizeof(extkey));
}

void lore_sm3_rkprf(unsigned char out[LORE_SYMBYTES],
                    const unsigned char key[LORE_SYMBYTES],
                    const unsigned char input[LORE_CIPHERTEXTBYTES])
{
    unsigned char buf[LORE_SYMBYTES + LORE_CIPHERTEXTBYTES];

    memcpy(buf, key, LORE_SYMBYTES);
    memcpy(buf + LORE_SYMBYTES, input, LORE_CIPHERTEXTBYTES);

    sm3_xof256(out, LORE_SYMBYTES, buf, sizeof(buf));
}
