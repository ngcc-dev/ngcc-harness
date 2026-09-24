#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "reduce.h"
#include "ntt.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*xoeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct{
  int16_t coeffs[N];
} poly;

/*---------------------多项式的约化---------------------*/
#define poly_freeze KEM_NAMESPACE(poly_freeze)
void poly_freeze(poly *r);
#define poly_freeze_centered KEM_NAMESPACE(poly_freeze_centered)
void poly_freeze_centered(poly *r);

/*---------------------多项式的运算---------------------*/
#define poly_ntt KEM_NAMESPACE(poly_ntt)
void poly_ntt(poly *a);
#define poly_invntt_tomont KEM_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_basemul_montgomery KEM_NAMESPACE(poly_basemul_montgomery)
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);
#define poly_tomont KEM_NAMESPACE(poly_tomont)
void poly_tomont(poly *r);
#define poly_baseinv KEM_NAMESPACE(poly_baseinv)
int poly_baseinv(poly *r, const poly *a);

#define poly_add KEM_NAMESPACE(_poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub KEM_NAMESPACE(_poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);
#define poly_sub_halfq KEM_NAMESPACE(_poly_sub_halfq)
void poly_sub_halfq(poly *r);

/*---------------------多项式的压缩---------------------*/
#define poly_compress_and_pack KEM_NAMESPACE(_poly_compress)
void poly_compress_and_pack(uint8_t r[KEM_POLYCOMPRESSEDBYTES], poly *a);
#define poly_unpack_and_decompress KEM_NAMESPACE(_poly_decompress)
void poly_unpack_and_decompress(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES]);

/*---------------------多项式的封装---------------------*/
#define poly_to_bytes KEM_NAMESPACE(poly_tobytes)
void poly_to_bytes(uint8_t r[KEM_POLYBYTES], const poly *a);
#define poly_from_bytes KEM_NAMESPACE(poly_frombytes)
void poly_from_bytes(poly *r, const uint8_t a[KEM_POLYBYTES]);
#define poly_from_msg KEM_NAMESPACE(poly_frommsg)
void poly_from_msg(poly *r, const uint8_t msg[KEM_MSGBYTES]);

/*---------------------多项式的采样---------------------*/
#define poly_f_ternary_p KEM_NAMESPACE(poly_f_ternary_p)
void poly_f_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce);
#define poly_g_ternary_p KEM_NAMESPACE(poly_g_ternary_p)
void poly_g_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce);
#define poly_r_ternary_p KEM_NAMESPACE(poly_r_ternary_p)
void poly_r_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce);
#define poly_e_ternary_p KEM_NAMESPACE(poly_e_ternary_p)
void poly_e_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce);

#endif