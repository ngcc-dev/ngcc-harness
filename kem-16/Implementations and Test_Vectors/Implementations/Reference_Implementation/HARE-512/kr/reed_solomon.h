#ifndef HARE_REED_SOLOMON_H
#define HARE_REED_SOLOMON_H

#include <stdint.h>
#include "parameters.h"

#define PARAM_RS_PARITY (PARAM_N1 - PARAM_K)
#define PARAM_G (PARAM_RS_PARITY + 1)

void reed_solomon_encode(uint64_t *cdw, const uint8_t *msg);
void reed_solomon_decode(uint8_t *msg, uint64_t *cdw, const uint8_t *erasures);
void compute_generator_poly(uint16_t *poly);

#endif
