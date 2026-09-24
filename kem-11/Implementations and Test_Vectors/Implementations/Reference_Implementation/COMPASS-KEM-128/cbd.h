#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define poly_cbd_eta1 COMPASS_KEM_NAMESPACE(poly_cbd_eta1)
void poly_cbd_eta1(poly *r, const uint8_t buf[COMPASS_KEM_ETA1*COMPASS_KEM_N/4]);

#define poly_cbd_eta2 COMPASS_KEM_NAMESPACE(poly_cbd_eta2)
void poly_cbd_eta2(poly *r, const uint8_t buf[COMPASS_KEM_ETA2*COMPASS_KEM_N/4]);

#endif
