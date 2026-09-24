#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define poly_cbd_eta KEM_NAMESPACE(poly_cbd_eta)
void poly_cbd_eta(poly *r, const uint8_t buf[KEM_ETA*KEM_N/4]);

#endif
