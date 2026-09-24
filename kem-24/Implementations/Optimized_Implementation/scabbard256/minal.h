#ifndef MINAL_H
#define MINAL_H

#include "params.h"
#include <stdint.h>
#include <stdlib.h>

void minal_b2_code_encode (int16_t codeword[], uint8_t msg_bits[]);
uint16_t minal_b2_code_decode (int16_t target[]);

#endif
