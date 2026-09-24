/* Parameter macro bridge for the API_PKC optimized MAMBA-Frost-384 KEM instance.
 * The byte lengths below are obtained from Frost/src/api_frost384.h through
 * the CRYPTO_* macros; KEM_AlgorithmInstance.h exposes the official API_PKC
 * PUBLICKEYBYTES/SECRETKEYBYTES/CIPHERTEXTBYTES/SHAREDSECRETBYTES names.
 */
#ifndef PARAMS_H
#define PARAMS_H

#include "Frost/src/api_frost384.h"
#include "KEM_AlgorithmInstance.h"

#if CRYPTO_PUBLICKEYBYTES != PUBLICKEYBYTES
#error "public-key length mismatch"
#endif
#if CRYPTO_SECRETKEYBYTES != SECRETKEYBYTES
#error "secret-key length mismatch"
#endif
#if CRYPTO_CIPHERTEXTBYTES != CIPHERTEXTBYTES
#error "ciphertext length mismatch"
#endif
#if CRYPTO_BYTES != SHAREDSECRETBYTES
#error "shared-secret length mismatch"
#endif

#endif