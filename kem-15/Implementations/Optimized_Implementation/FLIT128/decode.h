#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct {
    uint64_t w[F1_N_WORDS];
} poly_f1_u64;

typedef struct{
  uint8_t coeffs[F1_N];
} poly_f1;

#if KEM_MODE == 128

#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#elif KEM_MODE == 256 || KEM_MODE == 512

#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#endif

#endif