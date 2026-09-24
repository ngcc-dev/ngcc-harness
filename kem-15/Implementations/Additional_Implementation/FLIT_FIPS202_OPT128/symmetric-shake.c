#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "fips202.h"
#include "symmetric.h"

/*************************************************
* Name:        kem_shake256_prf
*
* Description: Usage of SHAKE256 as a PRF, concatenates secret and public input
*              and then generates outlen bytes of SHAKE256 output
*
* Arguments:   - uint8_t *out:       pointer to output
*              - size_t outlen:      number of requested output bytes
*              - const uint8_t *key: pointer to the key
*                                    (of length SEEDBYTES)
*              - uint8_t nonce:      single-byte nonce (public PRF input)
**************************************************/
void kem_shake256_prf(uint8_t *out,
                        size_t outlen,
                        const uint8_t key[SEEDBYTES],
                        uint8_t nonce)
{
    uint8_t extkey[SEEDBYTES+1];

    memcpy(extkey, key, SEEDBYTES);
    extkey[SEEDBYTES] = nonce;

    shake256(out, outlen, extkey, sizeof(extkey));
}