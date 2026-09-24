/**
 * @file reed_muller.h
 * @brief Header file of reed_muller.c
 */

#ifndef TRIQ_REED_MULLER_H
#define TRIQ_REED_MULLER_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

void reed_muller_encode(uint64_t* cdw, const uint64_t* msg);
void reed_muller_decode(uint64_t* msg, const uint64_t* cdw);

#endif  // TRIQ_REED_MULLER_H
