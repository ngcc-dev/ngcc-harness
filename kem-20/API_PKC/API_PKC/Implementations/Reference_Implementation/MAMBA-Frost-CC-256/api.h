/* API macro aliases for the API_PKC Reference_Implementation MAMBA-Frost-CC-256 KEM instance. */
#ifndef API_H
#define API_H

#include "KEM_AlgorithmInstance.h"

#define CRYPTO_PUBLICKEYBYTES  PUBLICKEYBYTES
#define CRYPTO_SECRETKEYBYTES  SECRETKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES CIPHERTEXTBYTES
#define CRYPTO_BYTES           SHAREDSECRETBYTES
#define CRYPTO_ALGNAME         ALGORITHM_INSTANCE

#endif
