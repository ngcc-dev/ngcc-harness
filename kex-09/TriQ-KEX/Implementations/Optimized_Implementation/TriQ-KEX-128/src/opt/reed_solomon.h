/**
 * @file reed_solomon.h
 * @brief Header file of reed_solomon.c
 */

#ifndef TRIQ_OPT_REED_SOLOMON_H
#define TRIQ_OPT_REED_SOLOMON_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

void reed_solomon_encode(uint64_t *cdw, const uint64_t *msg);
void reed_solomon_decode(uint64_t *msg, uint64_t *cdw);

void compute_generator_poly(uint16_t *poly);

#endif  // TRIQ_OPT_REED_SOLOMON_H

