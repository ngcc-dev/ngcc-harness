/**
 * @file code.h
 * @brief QUBE concatenated code interface.
 */

#ifndef QUBE_CODE_H
#define QUBE_CODE_H

#include <stdint.h>

void code_encode(uint64_t *em, const uint8_t *m);
void code_decode(uint8_t *m, const uint64_t *em);

#endif  // QUBE_CODE_H
