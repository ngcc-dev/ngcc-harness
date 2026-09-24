/*
 * Generic signature adapter.
 *
 * Wraps API_PKC SIG_AlgorithmInstance behind SIG_METHOD for the self-assessment
 * benchmark. Parameter set is selected at compile time via ALG_FOLDER / ALG_LEVEL
 * (derived from the implementation folder name, e.g. TSUOV_128 -> TSUOV, 128).
 */
#include <stdio.h>
#include <string.h>

#include "sig_adapter.h"
#include "registry.h"
#include "SIG_AlgorithmInstance.h"
#include "drng.h"

#ifndef ALG_FAMILY
#define ALG_FAMILY "ALGORITHM"
#endif
#ifndef ALG_LEVEL
#define ALG_LEVEL 0
#endif
#ifndef ALG_FOLDER
#define ALG_FOLDER "ALGORITHM_0"
#endif
#ifndef ALG_ID
#define ALG_ID 0
#endif
#ifndef SIG_LIB_REL_PATH
#define SIG_LIB_REL_PATH "build/libsigalg.a"
#endif

DRNG_ctx drng_algorithm;

static size_t adapter_pk_len(void) { return (size_t)sig_get_pk_len_bytes(); }
static size_t adapter_sk_len(void) { return (size_t)sig_get_sk_len_bytes(); }
static size_t adapter_sig_len(void) { return (size_t)sig_get_sn_len_bytes(); }

static void adapter_drng_seed(const uint8_t *seed, size_t seed_len)
{
    init_random_number(&drng_algorithm, seed, (unsigned long long)seed_len);
}

static int adapter_keygen(uint8_t *pk, size_t *pk_len, uint8_t *sk, size_t *sk_len)
{
    unsigned long long pk_out = *pk_len;
    unsigned long long sk_out = *sk_len;
    int ret = sig_keygen(pk, &pk_out, sk, &sk_out);
    *pk_len = (size_t)pk_out;
    *sk_len = (size_t)sk_out;
    return ret == 0 ? CRYPTO_SUCCESS : CRYPTO_FAILED;
}

static int adapter_sign(uint8_t *sk, size_t sk_len,
                        uint8_t *m, size_t m_len,
                        uint8_t *sig, size_t *sig_len)
{
    unsigned long long sig_out = *sig_len;
    int ret = sig_sign(sk, (unsigned long long)sk_len, m, (unsigned long long)m_len,
                       sig, &sig_out);
    *sig_len = (size_t)sig_out;
    return ret == 0 ? CRYPTO_SUCCESS : CRYPTO_FAILED;
}

static int adapter_verify(uint8_t *pk, size_t pk_len,
                          uint8_t *sig, size_t sig_len,
                          uint8_t *m, size_t m_len)
{
    return sig_verify(pk, (unsigned long long)pk_len, sig, (unsigned long long)sig_len,
                      m, (unsigned long long)m_len);
}

static SIG_METHOD sig_method = {
    .get_pk_len  = adapter_pk_len,
    .get_sk_len  = adapter_sk_len,
    .get_sig_len = adapter_sig_len,
    .drng_seed   = adapter_drng_seed,
    .keygen      = adapter_keygen,
    .sign        = adapter_sign,
    .verify      = adapter_verify,
};

static ALGORITHM sig_alg = {
    .type = ALG_SIG,
    .method = &sig_method,
    .alg_name = ALGORITHM_INSTANCE,
    .author_name = ALG_FAMILY " team",
    .security_level = ALG_LEVEL,
    .alg_id = ALG_ID,
    .lib_name = SIG_LIB_REL_PATH,
};

void register_sig_algorithm(void)
{
    if (registry_algorithm(&sig_alg)) {
        fprintf(stderr, "Failed to register %s.\n", sig_alg.alg_name);
    }
}
