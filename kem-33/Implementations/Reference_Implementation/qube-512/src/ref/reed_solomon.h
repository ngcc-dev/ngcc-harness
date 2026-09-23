/**
 * @file reed_solomon.h
 * @brief Reed-Solomon interface for QUBE.
 */

#ifndef QUBE_REED_SOLOMON_H
#define QUBE_REED_SOLOMON_H

#include <stdint.h>

void reed_solomon_encode(uint8_t *cdw, const uint8_t *msg);
void reed_solomon_decode(uint8_t *msg, const uint8_t *cdw);
void compute_generator_poly(uint16_t *poly);

#endif  // QUBE_REED_SOLOMON_H
