#ifndef TOOMCOOK_H
#define TOOMCOOK_H

#include "params.h"
#include <stdint.h>

#define poly_mul_acc LORE_NAMESPACE(poly_mul_acc)
void poly_mul_acc(const int16_t a[LORE_N], const int16_t b[LORE_N], int16_t res[LORE_N]);

#endif // TOOMCOOK_H