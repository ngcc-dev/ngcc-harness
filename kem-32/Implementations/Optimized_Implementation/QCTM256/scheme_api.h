#ifndef SCHEME_API_H
#define SCHEME_API_H

#define GF_EXT_DEGREE 18
#define LENGTH 19000
#define ORDER 19
#define GOPPA_DEGREE 513
#define DEFAULT_TWIST_ROW (GOPPA_DEGREE - 1)
#define ERROR_WEIGHT 256
#define NB_ERRORS ERROR_WEIGHT

#include "sizes.h"

#define IN const
#define OUT

#define CRYPTO_SECRETKEYBYTES SECRETKEY_BYTES
#define CRYPTO_PUBLICKEYBYTES PUBLICKEY_BYTES
#define CRYPTO_CIPHERTEXTBYTES CIPHERTEXT_BYTES
#define CRYPTO_BYTES HASH_SIZE

#define CRYPTO_ALGNAME "QCTM256"

#define CHECK_STATUS(stat) {if(stat != SUCCESS) {goto EXIT;}}
enum _status
{
    SUCCESS  = 0,
    FAIL = -1,
};

typedef enum _status status_t;

int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk);

int crypto_kem_enc(OUT unsigned char *ct,
                   OUT unsigned char *ss,
                   IN unsigned char *pk);

int crypto_kem_dec(OUT unsigned char *ss,
                   IN unsigned char *ct,
                   IN unsigned char *sk);

int qctm_precompute_public_key(IN unsigned char *pk);

int qctm_precompute_secret_key(IN unsigned char *sk);

int qctm_precompute_keypair(IN unsigned char *pk,
                            IN unsigned char *sk);

#endif
