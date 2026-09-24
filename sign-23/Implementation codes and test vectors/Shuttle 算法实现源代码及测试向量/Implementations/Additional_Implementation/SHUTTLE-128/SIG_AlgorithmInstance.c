/*
The software is provided by the Institute of Commercial Cryptography
Standards (ICCS), and is used for algorithm submissions in the
Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will
be uninterrupted or error-free in all cases. ICCS will take no
responsibility for the use of the software or the results thereof, if the
software is used for any other purposes.
*/

/*
 * SIG_AlgorithmInstance.c -- the NGCC adapter for SHUTTLE.
 *
 * Thin glue mapping the fixed NGCC sig_* contract
 * (SIG_AlgorithmInstance.h) onto the SHUTTLE top-level KeyGen/Sign/Verify
 * (sign.c).  Two responsibilities only:
 *
 *   (1) the three byte-length getters return the params.h size macros
 *       (CRYPTO_PUBLICKEYBYTES / CRYPTO_SECRETKEYBYTES / CRYPTO_BYTES);
 *       CRYPTO_BYTES is the EXACT rANS signature length
 *       (sig_get_sn_len_bytes() == SIG_PACKED_BYTES) -- KAT_SIG.c calloc's
 *       the sn buffer with it once.  (The RAW milestone path is larger and
 *       reports SIG_RAW_PACKED_BYTES instead; see SHUTTLE_SIG_LEN below.)
 *
 *   (2) the three entry points draw the scheme-internal randomness from
 * the global SM3 Hash-DRBG drng_algorithm (NGCC_MODE) and forward to the
 *       sign.c primitives:
 *
 *         sig_keygen : xi  <- get_random_number(drng_algorithm, lambda
 * bits) crypto_sign_keypair_xi(pk, sk, xi) sig_sign   : rnd <-
 * get_random_number(drng_algorithm, lambda/8 bytes -> RNDBYTES) for hedged
 * signing crypto_sign_signature_rnd(sn, &snlen, m, mlen, sk, rnd)
 *         sig_verify : crypto_sign_verify(sn, snlen, m, mlen, pk)
 *                      maps 0->0, -1->-1 (invalid sig), other<0 -> as-is
 *                      (a -2 length/format error, etc.)
 *
 * The DRNG byte schedule (draw xi for KeyGen, then rnd for Sign, both from
 * the same drng_algorithm the harness seeds per Count) is the KAT-defining
 * order; it must be identical across ref/avx2/avx512 within a MODE.  For a
 * deterministic KAT one may instead zero rnd -- this adapter uses the
 * hedged draw (rnd from the DRBG) to match the spec's Sign(sk, M, rnd)
 * signature; the rnd-source choice is pinned per MODE.
 */

#include "SIG_AlgorithmInstance.h"

#include <string.h>

#include "drng.h"
#include "params.h"  /* CRYPTO_*BYTES, SEEDBYTES, LAMBDA */
#include "polyvec.h" /* RNDBYTES */
#include "rans.h"    /* SIG_RAW_PACKED_BYTES / SIG_PACKED_BYTES */

/* The realized signature length depends on the compiled packing path:
 *  - RAW milestone (-DSIG_RAW): fixed SIG_RAW_PACKED_BYTES (larger; the
 *    un-compressed seedC||z1||hint layout).  CRYPTO_BYTES (the rANS exact
 *    length) is SMALLER than this, so sig_get_sn_len_bytes() MUST report
 * the RAW length or the KAT harness under-allocates the sn buffer.
 *  - rANS production path: fixed SIG_PACKED_BYTES == CRYPTO_BYTES. */
#if defined(SIG_RAW)
#    define SHUTTLE_SIG_LEN ((unsigned long long)SIG_RAW_PACKED_BYTES)
#else
#    define SHUTTLE_SIG_LEN ((unsigned long long)CRYPTO_BYTES)
#endif

/* sign.c primitives.  The xi/rnd-driven variants let this adapter inject
 * the DRBG-drawn randomness (the plain crypto_sign_keypair / _signature
 * use a zeroed seed and are only for the standalone NIST path). */
int crypto_sign_keypair_xi(unsigned char *pk, unsigned char *sk,
                           const unsigned char xi[SEEDBYTES]);
int crypto_sign_signature_rnd(unsigned char *sig, size_t *siglen,
                              const unsigned char *m, size_t mlen,
                              const unsigned char *sk,
                              const unsigned char rnd[RNDBYTES]);
int crypto_sign_verify(const unsigned char *sig, size_t siglen,
                       const unsigned char *m, size_t mlen,
                       const unsigned char *pk);

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number,
// random_number_len_bits);

unsigned long long sig_get_pk_len_bytes()
{
    return (unsigned long long)CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
    return (unsigned long long)CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
    /* The fixed signature length for the compiled packing path (RAW vs
     * rANS).  KAT_SIG.c calloc's the sn buffer with exactly this, so it
     * MUST be >= the realized signature length (RAW is larger than the
     * exact rANS CRYPTO_BYTES -- see SHUTTLE_SIG_LEN above). */
    return SHUTTLE_SIG_LEN;
}

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    unsigned char xi[SEEDBYTES];
    int rc;
    /* Draw xi (lambda bits = SEEDBYTES bytes) from the seeded DRBG. */
    if (get_random_number(&drng_algorithm, xi,
                          (unsigned long long)SEEDBYTES * 8) != 0)
        return -10;
    rc = crypto_sign_keypair_xi(pk, sk, xi);
    if (rc != 0)
        return rc; /* propagate the (negative) keygen error code */
    *pk_len_bytes = (unsigned long long)CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = (unsigned long long)CRYPTO_SECRETKEYBYTES;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes)
{
    unsigned char rnd[RNDBYTES];
    size_t siglen = 0;
    int rc;
    (void)
        sk_len_bytes; /* fixed by CRYPTO_SECRETKEYBYTES; not re-checked */
    /* Hedged signing: draw rnd (RNDBYTES) from the DRBG.  For a purely
     * deterministic KAT one would zero rnd instead; the byte schedule
     * (xi for KeyGen, then rnd for each Sign) is pinned per MODE.
     */
    if (get_random_number(&drng_algorithm, rnd,
                          (unsigned long long)RNDBYTES * 8) != 0)
        return -10;
    rc = crypto_sign_signature_rnd(sn, &siglen, m, (size_t)m_len_bytes, sk,
                                   rnd);
    if (rc != 0)
        return rc;
    *sn_len_bytes = (unsigned long long)siglen;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes)
{
    int rc;
    (void)
        pk_len_bytes; /* fixed by CRYPTO_PUBLICKEYBYTES; not re-checked */
    rc = crypto_sign_verify(sn, (size_t)sn_len_bytes, m,
                            (size_t)m_len_bytes, pk);
    /* Contract: 0 valid, -1 invalid, -2..-99 other error.
     * crypto_sign_verify already returns exactly 0 / -1 / -2 / -4, so
     * forward verbatim. */
    return rc;
}
