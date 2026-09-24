#ifndef CODING_H
#define CODING_H

#include <stdint.h>

uint32_t decode_bw32(int16_t t[32]);
uint64_t encode_bw32(uint32_t m);

#endif