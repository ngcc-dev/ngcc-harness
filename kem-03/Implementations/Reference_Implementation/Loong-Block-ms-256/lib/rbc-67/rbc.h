#ifndef RBC_H
#define RBC_H

#include <stdint.h>

#define RBC_FIELD_M 67
#define RBC_ELT_WORDS 2
#define RBC_ELT_BYTES ((RBC_FIELD_M + 7) / 8)
#define RBC_FIELD_POLY_TERMS { 5U, 2U, 1U, 0U }
#define RBC_FIELD_POLY_TERMS_COUNT 4U
#define RBC_FIELD_POLY_ERRATUM_UNCONFIRMED 0

#endif
