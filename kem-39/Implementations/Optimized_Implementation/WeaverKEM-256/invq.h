#ifndef INVQ_H
#define INVQ_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

typedef struct {
  uint16_t bucket_lo[512];
  uint8_t bucket_size[512];
} invq_table_t;

#define polyvec_invq WEAVER_NAMESPACE(_polyvec_invq)
void polyvec_invq(polyvec *v,
                  const uint8_t seed[WEAVER_SYMBYTES],
                  uint8_t nonce);

#endif /* INVQ_H */
