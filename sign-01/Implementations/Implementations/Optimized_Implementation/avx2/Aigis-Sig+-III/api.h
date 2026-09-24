#ifndef API_H
#define API_H

#include "params.h"
#include "hashkdf.h"


#if SEEDBYTES == 16
#define Hash hash128
#define Hash2 hash256
#define KDF kdf128
#define KDF_ABSORB kdf128_absorb
#define KDF_SQUEEZEBLOCK kdf128_squeezeblocks
#define KDF_RATE KDF128RATE
#define KDFX4 kdf128x4
#elif SEEDBYTES == 20
#define Hash hash160
#define Hash2 hash320
#define KDF kdf256
#define KDF_ABSORB kdf256_absorb
#define KDF_SQUEEZEBLOCK kdf256_squeezeblocks
#define KDF_RATE KDF256RATE
#define KDFX4 kdf256x4
#elif SEEDBYTES == 32
#define Hash hash256
#define Hash2 hash512
#define KDF kdf256
#define KDF_ABSORB kdf256_absorb
#define KDF_SQUEEZEBLOCK kdf256_squeezeblocks
#define KDF_RATE KDF256RATE
#define KDFX4 kdf256x4
#elif SEEDBYTES == 64
#define Hash hash512
#define Hash2 hash1024
#define KDF kdf512
#define KDF_ABSORB kdf512_absorb
#define KDF_SQUEEZEBLOCK kdf512_squeezeblocks
#define KDF_RATE KDF512RATE
#define KDFX4 kdf512x4
#endif


#define SIG_SECRETKEYBYTES SK_SIZE_PACKED
#define SIG_PUBLICKEYBYTES PK_SIZE_PACKED
#define SIG_BYTES SIG_MAX_SIZE_PACKED

#define SIG_ALGNAME "Aigis-sig"

            
int32_t msig_keygen(uint8_t *pk, uint8_t *sk);

int32_t msig_sign(uint8_t *sk,
	uint8_t *m, int32_t mlen,
	uint8_t *sm, int32_t *smlen);

int32_t msig_verf(uint8_t *pk,
	uint8_t *sm, int32_t smlen,
	uint8_t *m, int32_t mlen);
#endif
