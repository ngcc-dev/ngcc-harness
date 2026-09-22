/**
 * @file reed_muller.h
 * @brief Header file of reed_muller.c
 */

#ifndef MITO_REED_MULLER_H
#define MITO_REED_MULLER_H

#include <stdint.h>
#include <string.h>
#include "parameters.h"

void reed_muller_encode(uint64_t* cdw, const uint64_t* msg);
int reed_muller_decode(uint64_t* msg, uint64_t* pos, const uint64_t* cdw);

#endif  // MITO_REED_MULLER_H
