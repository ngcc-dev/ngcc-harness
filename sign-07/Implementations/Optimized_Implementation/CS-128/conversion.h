#ifndef CONVERSION_H
#define CONVERSION_H

#include "ntt.h"

#define N ((tau + taup - 1) / taup)

void SimpleBitPack(uint8_t* z, const poly* w, int b);
void SimpleBitUnpack(poly* w, const uint8_t* z, int b);

void BitPack(uint8_t* z, const poly* w, int b);
void BitUnpack(poly* w, const uint8_t* z, int b);

#endif
