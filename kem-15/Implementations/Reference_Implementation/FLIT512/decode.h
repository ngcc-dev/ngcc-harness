#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#if KEM_MODE == 128
typedef struct{
  int16_t coeffs[N/2];
} poly_f1;

#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#elif KEM_MODE == 256 || KEM_MODE == 512
typedef struct{
  int16_t coeffs[N/4];
} poly_f1;

#define poly_inv_in_F2 KEM_NAMESPACE(poly_inv_in_F2)
int poly_inv_in_F2(poly_f1 *f1, const poly *f);
#define poly_decode_to_msg KEM_NAMESPACE(poly_decode)
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1);

#endif

#endif