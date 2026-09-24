#ifndef SIG_ADAPTER_H
#define SIG_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

typedef struct {
    size_t (*get_pk_len)(void);
    size_t (*get_sk_len)(void);
    size_t (*get_sig_len)(void);

    /* Reseed the algorithm DRNG for reproducible keygen/signing. */
    void (*drng_seed)(const uint8_t *seed, size_t seed_len);

    int (*keygen)(uint8_t *pk, size_t *pk_len, uint8_t *sk, size_t *sk_len);

    int (*sign)(uint8_t *sk, size_t sk_len,
                uint8_t *m, size_t m_len,
                uint8_t *sig, size_t *sig_len);

    int (*verify)(uint8_t *pk, size_t pk_len,
                  uint8_t *sig, size_t sig_len,
                  uint8_t *m, size_t m_len);
} SIG_METHOD;

void register_sig_algorithm(void);

#endif
