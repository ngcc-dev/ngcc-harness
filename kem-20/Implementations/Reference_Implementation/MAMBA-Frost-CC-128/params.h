/* Parameter macro bridge for the API_PKC MAMBA-Frost-CC-128 KEM instance.
 * Byte lengths are sourced from Frost-CC/src/api_frostcc128.h and checked
 * against the official API_PKC KEM_AlgorithmInstance.h macros.
 */
#ifndef PARAMS_H
#define PARAMS_H

#include "Frost-CC/src/api_frostcc128.h"
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
