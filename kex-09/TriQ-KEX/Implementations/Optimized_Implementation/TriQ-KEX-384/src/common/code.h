/**
 * @file code.h
 * @brief Header file of code.c
 */

#ifndef TRIQ_CODE_H
#define TRIQ_CODE_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

void code_encode(uint64_t *em, const uint64_t *m);
void code_decode(uint64_t *m, const uint64_t *em);

#endif  // TRIQ_CODE_H
