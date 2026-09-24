#ifndef ADKEX_SIG_H
#define ADKEX_SIG_H

/*
ADKEX (KEX + SIG) — backend-agnostic SIG adapter.

The KEX layer (KEX_AlgorithmInstance.c, adkex_derand.c) only ever sees
this header and the `adkex_sig_*` functions; it never knows which
underlying SIG primitive is in use. To swap SIG primitives, change the
CMake backend flag and link a different `adkex_sig_<backend>.c`.

Currently supported backends:
    ADKEX_SIG_BACKEND_MLDSA   pq-crystals ML-DSA (FIPS 204).
                              ADKEX_SIG_MLDSA_LEVEL selects 2 (44),
                              3 (65), or 5 (87).

The backend macro and level macro are passed by CMake via -D.
To add a new backend (e.g. Falcon, SLH-DSA, a custom scheme):
    1. Add a parallel #if block here with PKBITS/SKBITS/SNBITS/COINBITS.
    2. Implement adkex_sig_keygen/sign/verify in adkex_sig_<backend>.c.
    3. Update CMake to set -DADKEX_SIG_BACKEND_<X> and compile the
       corresponding adapter source instead.
*/

#include <stdint.h>

#if !defined(ADKEX_SIG_BACKEND_MLDSA)
#  error "No ADKEX_SIG_BACKEND_* selected. CMake must define one of: ADKEX_SIG_BACKEND_MLDSA."
#endif

#if defined(ADKEX_SIG_BACKEND_MLDSA)
#  if !defined(ADKEX_SIG_MLDSA_LEVEL)
#    error "ADKEX_SIG_BACKEND_MLDSA selected but ADKEX_SIG_MLDSA_LEVEL is not set (use 2, 3, or 5)."
#  endif
#  if   ADKEX_SIG_MLDSA_LEVEL == 2          /* ML-DSA-44 */
#    define ADKEX_SIG_PKBITS    (8 * 1312)
#    define ADKEX_SIG_SKBITS    (8 * 2560)
#    define ADKEX_SIG_SNBITS    (8 * 2420)
#  elif ADKEX_SIG_MLDSA_LEVEL == 3          /* ML-DSA-65 */
#    define ADKEX_SIG_PKBITS    (8 * 1952)
#    define ADKEX_SIG_SKBITS    (8 * 4032)
#    define ADKEX_SIG_SNBITS    (8 * 3309)
#  elif ADKEX_SIG_MLDSA_LEVEL == 5          /* ML-DSA-87 */
#    define ADKEX_SIG_PKBITS    (8 * 2592)
#    define ADKEX_SIG_SKBITS    (8 * 4896)
#    define ADKEX_SIG_SNBITS    (8 * 4627)
#  else
#    error "ADKEX_SIG_MLDSA_LEVEL must be 2, 3, or 5."
#  endif
/* ML-DSA crypto_sign_keypair pulls one SEEDBYTES (32 B = 256 b) block
   from randombytes(). Our adapter routes that block from caller-supplied
   coins via a randombytes() hook (see randombytes.c). */
#  define ADKEX_SIG_COINBITS    (8 * 32)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /* Generate a SIG key pair from caller-supplied coins.
       For ML-DSA the coins are the keygen seed (32 bytes), routed into
       crypto_sign_keypair via the local randombytes() hook.            */
    void adkex_sig_keygen(
        uint8_t       pk   [ADKEX_SIG_PKBITS   / 8],
        uint8_t       sk   [ADKEX_SIG_SKBITS   / 8],
        const uint8_t coins[ADKEX_SIG_COINBITS / 8]);

    /* Sign a message with sk. Deterministic (the underlying SIG is
       called via its internal API with zeroed per-sig randomness). */
    void adkex_sig_sign(
        uint8_t       sigma[ADKEX_SIG_SNBITS / 8],
        const uint8_t sk   [ADKEX_SIG_SKBITS / 8],
        const uint8_t *msg, unsigned long long msg_bits);

    /* Verify a signature. Returns 1 if valid, 0 otherwise. */
    int adkex_sig_verify(
        const uint8_t pk   [ADKEX_SIG_PKBITS / 8],
        const uint8_t sigma[ADKEX_SIG_SNBITS / 8],
        const uint8_t *msg, unsigned long long msg_bits);

    /* Coin-injection hook used by our randombytes.c. The backend
       adapter (adkex_sig_mldsa.c) owns the injection state; randombytes.c
       calls this on every request and falls back to drng_algorithm if
       no coins are pending. Returns 0 on hit, -1 to fall back. */
    int adkex_sig_random_hook(uint8_t *out, unsigned long long outlen);

#ifdef __cplusplus
}
#endif
#endif /* ADKEX_SIG_H */
