#ifndef NSS_HQC_CODE_LAYER_H
#define NSS_HQC_CODE_LAYER_H

#include <stdint.h>

#include "nss_hqc_core.h"

/* Concatenated shortened RS over GF(256) with duplicated RM(1,7). */
void nss_hqc_code_encode(uint8_t *cw, const uint8_t *m);
int nss_hqc_code_decode(uint8_t *m, const uint8_t *cw);
int nss_hqc_code_selftest(void);

#endif
