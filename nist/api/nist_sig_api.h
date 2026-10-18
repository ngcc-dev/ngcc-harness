#ifndef NIST_SIG_API_H
#define NIST_SIG_API_H

#include <stdint.h>

/* Harness ABI, not an alteration of the submission's NIST API. */
#define NIST_SIG_ABI_VERSION 1u
typedef struct {
    uint32_t abi_version;
    const char *scheme;
    const char *parameter;
    const char *implementation;
    uint64_t public_key_bytes;
    uint64_t secret_key_bytes;
    uint64_t signature_bytes; /* maximum attached-signature overhead */
} nist_sig_api;

const nist_sig_api *nist_sig_get_api(void);
int nist_sig_seed(const uint8_t seed[48]);
int nist_sig_keypair(uint8_t *pk, uint8_t *sk);
int nist_sig_sign(uint8_t *sm, uint64_t *smlen, const uint8_t *m,
                  uint64_t mlen, const uint8_t *sk);
int nist_sig_open(uint8_t *m, uint64_t *mlen, const uint8_t *sm,
                  uint64_t smlen, const uint8_t *pk);

#endif
