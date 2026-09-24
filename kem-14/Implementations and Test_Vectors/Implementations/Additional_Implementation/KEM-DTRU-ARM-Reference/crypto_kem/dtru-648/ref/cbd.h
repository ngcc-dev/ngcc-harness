#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

void cbd2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]);
void cbd9(poly *r, const uint8_t buf[DTRU_CBD9_BYTES]);

#endif
