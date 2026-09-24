#ifndef REDUCE_H
#define REDUCE_H

#include "params.h"
#include <stdint.h>

//static const uint32_t QINV = 199935; // -inverse_mod(q,2^18)
static const uint32_t QINV = 7679; // -inverse_mod(q,2^18)
//static const uint32_t QINV = 462079; // -inverse_mod(q,2^20)

uint16_t montgomery_reduce (uint32_t a);
uint16_t barrett_reduce (uint16_t a);
uint16_t modular_reduction (uint32_t c);
uint16_t freeze (uint16_t a);

#endif
