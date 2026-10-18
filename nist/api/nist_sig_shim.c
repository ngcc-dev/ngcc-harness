/* Compile alongside an unmodified NIST crypto_sign API implementation.
 * Define NIST_SIG_API_HEADER to a quoted path when api.h is not on -I.
 * A submission-specific shim may instead implement nist_sig_api.h directly.
 */
#include "nist_sig_api.h"
#ifndef NIST_SIG_API_HEADER
#define NIST_SIG_API_HEADER "api.h"
#endif
#include NIST_SIG_API_HEADER

#ifndef NIST_SIG_SCHEME
#error "Define NIST_SIG_SCHEME to the candidate name string"
#endif
#ifndef NIST_SIG_PARAMETER
#error "Define NIST_SIG_PARAMETER to the parameter name string"
#endif
#ifndef NIST_SIG_IMPLEMENTATION
#define NIST_SIG_IMPLEMENTATION "reference"
#endif
#ifndef NIST_SIG_PK_BYTES
#define NIST_SIG_PK_BYTES CRYPTO_PUBLICKEYBYTES
#endif
#ifndef NIST_SIG_SK_BYTES
#define NIST_SIG_SK_BYTES CRYPTO_SECRETKEYBYTES
#endif
#ifndef NIST_SIG_SIG_BYTES
#define NIST_SIG_SIG_BYTES CRYPTO_BYTES
#endif
#ifndef NIST_SIG_KEYPAIR
#define NIST_SIG_KEYPAIR crypto_sign_keypair
#endif
#ifndef NIST_SIG_SIGN
#define NIST_SIG_SIGN crypto_sign
#endif
#ifndef NIST_SIG_OPEN
#define NIST_SIG_OPEN crypto_sign_open
#endif
#ifndef NIST_SIG_LENGTH_TYPE
#define NIST_SIG_LENGTH_TYPE unsigned long long
#endif

static const nist_sig_api metadata = {
    NIST_SIG_ABI_VERSION, NIST_SIG_SCHEME, NIST_SIG_PARAMETER,
    NIST_SIG_IMPLEMENTATION, NIST_SIG_PK_BYTES, NIST_SIG_SK_BYTES,
    NIST_SIG_SIG_BYTES
};

const nist_sig_api *nist_sig_get_api(void) { return &metadata; }

#ifndef NIST_SIG_CUSTOM_SEED
extern void randombytes_init(unsigned char *, unsigned char *, int);
int nist_sig_seed(const uint8_t seed[48]) {
    unsigned char entropy[48];
    for (unsigned i = 0; i < sizeof entropy; ++i) entropy[i] = seed[i];
    randombytes_init(entropy, 0, 256);
    return 0;
}
#endif

int nist_sig_keypair(uint8_t *pk, uint8_t *sk) {
    return NIST_SIG_KEYPAIR(pk, sk);
}
int nist_sig_sign(uint8_t *sm, uint64_t *smlen, const uint8_t *m,
                  uint64_t mlen, const uint8_t *sk) {
    NIST_SIG_LENGTH_TYPE length = 0;
    int rc = NIST_SIG_SIGN(sm, &length, m, (NIST_SIG_LENGTH_TYPE)mlen, sk);
    *smlen = length;
    return rc;
}
int nist_sig_open(uint8_t *m, uint64_t *mlen, const uint8_t *sm,
                  uint64_t smlen, const uint8_t *pk) {
    NIST_SIG_LENGTH_TYPE length = 0;
    int rc = NIST_SIG_OPEN(m, &length, sm, (NIST_SIG_LENGTH_TYPE)smlen, pk);
    *mlen = length;
    return rc;
}
