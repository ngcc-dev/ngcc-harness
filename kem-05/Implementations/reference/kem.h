/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef __KEM_H_INCLUDED__
#define __KEM_H_INCLUDED__

#include "api.h"
#include "stdlib.h"
#include "string.h"
#include "utilities.h"
#include "FromNIST/rng.h"

enum _seeds_purpose
{
    KEYGEN_SEEDS = 0,
    ENCAPS_SEEDS = 1,
    DECAPS_SEEDS = 2
};

typedef enum _seeds_purpose seeds_purpose_t;

_INLINE_ void get_seeds(OUT double_seed_t* seeds, seeds_purpose_t seeds_type)
{
#ifdef NIST_RAND
    randombytes(seeds->raw, sizeof(double_seed_t));
#else
    for(uint32_t i = 0; i < sizeof(seed_t); ++i)
    {
        seeds->s1.raw[i] = rand(); // not cryptographically secure !
        seeds->s2.raw[i] = rand(); // not cryptographically secure !
    }
#endif
    EDMSG("s1: "); print(seeds->s1.qwords, sizeof(seed_t)*8);
    EDMSG("s2: "); print(seeds->s2.qwords, sizeof(seed_t)*8);
}

////////////////////////////////////////////////////////////////
//Below three APIs (keygen, encaps, decaps) are defined by NIST:
////////////////////////////////////////////////////////////////
//Keygenerate - pk is the public key,
//              sk is the private key,
int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk);

//Encapsulate - pk is the public key,
//              ct is a key encapsulation message (ciphertext),
//              ss is the shared secret.
int crypto_kem_enc(OUT unsigned char *ct,
        OUT unsigned char *ss,
        IN const unsigned char *pk);

//Decapsulate - ct is a key encapsulation message (ciphertext),
//              sk is the private key,
//              ss is the shared secret
int crypto_kem_dec(OUT unsigned char *ss,
        IN const unsigned char *ct,
        IN const unsigned char *sk);


#endif //__KEM_H_INCLUDED__

