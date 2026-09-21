#ifndef LOOM_PRF_MAC_H
#define LOOM_PRF_MAC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

#define loom_prf_derive LOOM_NAMESPACE(prf_derive)
int loom_prf_derive(uint8_t ss[LOOM_KEM_SSBYTES],
                     uint8_t mki[LOOM_MACBYTES],
                     uint8_t mkr[LOOM_MACBYTES],
                     const uint8_t k[LOOM_KEM_SSBYTES],
                     const uint8_t ni[LOOM_NONCEBYTES],
                     const uint8_t nr[LOOM_NONCEBYTES]);

#define loom_mac_compute LOOM_NAMESPACE(mac_compute)
int loom_mac_compute(uint8_t tau[LOOM_MACBYTES],
                      const uint8_t kmac[LOOM_MACBYTES],
                      const uint8_t *input,
                      size_t inlen);

#define loom_mac_verify LOOM_NAMESPACE(mac_verify)
int loom_mac_verify(const uint8_t tau[LOOM_MACBYTES],
                    const uint8_t kmac[LOOM_MACBYTES],
                    const uint8_t *input,
                    size_t inlen);

#endif
