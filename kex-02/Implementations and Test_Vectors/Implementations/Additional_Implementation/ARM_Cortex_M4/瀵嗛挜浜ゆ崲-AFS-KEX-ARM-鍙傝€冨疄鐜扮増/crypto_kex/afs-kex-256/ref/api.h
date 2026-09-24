#ifndef API_H
#define API_H

#include "params.h"
#include "KEX_AlgorithmInstance.h"

/*
 * Size macros for the AFS-KEX (4-pass authenticated KEX) interface, derived
 * from the underlying BW-KEM parameters. They mirror the runtime getters in
 * KEX_AlgorithmInstance.c (kex_get_*_len_bytes) and are used by the mupq
 * crypto_kex test harness to size its static buffers at compile time.
 */
#define CRYPTO_KEX_PUBLICKEYBYTES  (2 * KYBER_PUBLICKEYBYTES)
#define CRYPTO_KEX_SECRETKEYBYTES  (2 * KYBER_SECRETKEYBYTES)
#define CRYPTO_KEX_STATEBYTES      (4 * KYBER_SYMBYTES)
#define CRYPTO_KEX_SSBYTES         (KYBER_SYMBYTES)
/* Upper bound on the length of any single pass message (== total message
 * length); each per-pass buffer is sized to this for safety. */
#define CRYPTO_KEX_MSGBYTES        (2 * KYBER_CIPHERTEXTBYTES + 2 * KYBER_SYMBYTES)
#define CRYPTO_KEX_PASSES          4

#define CRYPTO_ALGNAME             ALGORITHM_INSTANCE

#endif
