/**
 * @file reed_muller.h
 * @brief Header file of reed_muller.c
 */

#ifndef HARE_COMMON_REED_MULLER_H
#define HARE_COMMON_REED_MULLER_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

void reed_muller_encode(uint64_t* cdw, const uint64_t* msg);
void reed_muller_decode(uint64_t *msg, uint8_t *erasures, const uint64_t *cdw);

#endif  // HARE_COMMON_REED_MULLER_H
