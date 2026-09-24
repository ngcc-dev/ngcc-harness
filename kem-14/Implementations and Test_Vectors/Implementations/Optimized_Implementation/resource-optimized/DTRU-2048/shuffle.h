#ifndef SHUFFLE_H
#define SHUFFLE_H

#include <immintrin.h>

void shuffle_to_basemul16x16(__m256i input[16]);
void shuffle_from_basemul16x16(__m256i input[16]);
#endif
