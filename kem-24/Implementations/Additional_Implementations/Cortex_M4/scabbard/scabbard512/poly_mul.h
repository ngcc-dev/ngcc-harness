#ifndef POLY_MUL_H
#define POLY_MUL_H

#include <stdint.h>
#include "params.h"

#if defined(SCABBARD_USE_ASM_MUL) && SCABBARD_USE_ASM_MUL
void poly_mul_64_sch_acc(
    const uint16_t *a,
    const uint8_t *b_packed,
    uint16_t *acc,
    uint16_t mod_mask);
#endif

#endif
