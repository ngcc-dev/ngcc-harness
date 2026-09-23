/**
 * @file code.c
 * @brief QUBE concatenated code C = RM o RS.
 */

#include "code.h"

#include <stdint.h>

#include "crypto_memset.h"
#include "parameters.h"
#include "reed_muller.h"
#include "reed_solomon.h"

void code_encode(uint64_t *em, const uint8_t *m) {
    uint64_t tmp[VEC_N1_SIZE_64] = {0};

    reed_solomon_encode((uint8_t *)tmp, m);
    reed_muller_encode(em, tmp);

    memset_zero(tmp, sizeof tmp);
}

void code_decode(uint8_t *m, const uint64_t *em) {
    uint64_t tmp[VEC_N1_SIZE_64] = {0};

    reed_muller_decode(tmp, em);
    reed_solomon_decode(m, (const uint8_t *)tmp);

    memset_zero(tmp, sizeof tmp);
}
