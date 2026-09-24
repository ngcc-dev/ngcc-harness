/**
 * @file code.h
 * @brief Header file of code.c
 */

#ifndef HARE_COMMON_CODE_H
#define HARE_COMMON_CODE_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

void code_encode(uint64_t *em, const uint8_t *m);
void code_decode(uint8_t *m, const uint64_t *em);

#endif  // HARE_COMMON_CODE_H
