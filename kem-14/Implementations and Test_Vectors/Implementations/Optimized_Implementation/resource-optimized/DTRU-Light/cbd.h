#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"
#include "poly.h"
#include <immintrin.h>

void cbd1(poly *r, const uint8_t buf[DTRU_CBD1_BYTES]);
void cbd2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]);
// void cbd3(poly *r, const uint8_t buf[DTRU_CBD3_BYTES]);

void cbd1_avx2(poly *r, const uint8_t buf[DTRU_CBD1_BYTES]);
void cbd2_avx2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]);

#endif
