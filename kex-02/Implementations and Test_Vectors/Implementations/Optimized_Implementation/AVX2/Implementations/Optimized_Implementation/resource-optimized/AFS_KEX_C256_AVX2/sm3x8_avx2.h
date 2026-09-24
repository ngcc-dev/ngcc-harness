#ifndef SM3X8_AVX2_H
#define SM3X8_AVX2_H

#include <stddef.h>
#include <stdint.h>

void sm3_x8_digest(const uint8_t *data, size_t datalen, uint8_t dgst[8][32]);
void sm3_x4_digest(const uint8_t *data, size_t datalen, uint8_t dgst[4][32]);

#endif
