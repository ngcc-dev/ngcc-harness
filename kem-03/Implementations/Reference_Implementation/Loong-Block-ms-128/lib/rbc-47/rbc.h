#ifndef RBC_H
#define RBC_H

#include <stdint.h>

#define RBC_FIELD_M 47
#define RBC_ELT_WORDS 1
#define RBC_ELT_BYTES ((RBC_FIELD_M + 7) / 8)
#define RBC_FIELD_POLY_TERMS { 5U, 0U }
#define RBC_FIELD_POLY_TERMS_COUNT 2U
#define RBC_FIELD_POLY_ERRATUM_UNCONFIRMED 0

#endif
