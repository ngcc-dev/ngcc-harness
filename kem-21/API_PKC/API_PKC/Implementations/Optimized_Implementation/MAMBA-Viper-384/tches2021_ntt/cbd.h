#ifndef CBD_H
#define CBD_H

#include <stdint.h>
#include "params.h"

#define cbd POLYMUL_NAMESPACE(_cbd)
void cbd(int16_t r[KEM_N], const uint8_t buf[KEM_N]);

#endif
