#ifndef M2E_H
#define M2E_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sizes.h"
#include "scheme_api.h"

typedef uint16_t index_t;

int fixed_weight_random(OUT int *error);
int m2error(IN unsigned char *m, OUT int *error);

#endif
