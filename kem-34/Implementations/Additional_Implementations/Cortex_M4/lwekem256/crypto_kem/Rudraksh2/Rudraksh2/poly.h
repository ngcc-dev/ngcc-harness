#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

/*
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*coeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct{
  uint16_t coeffs[KEM_N];
} poly;

#define poly_zeroize KEM_NAMESPACE(poly_zeroize)
void poly_zeroize(poly *r);
#define poly_compress KEM_NAMESPACE(poly_compress)
void poly_compress(uint8_t r[KEM_POLYCOMPRESSEDBYTES], const poly *a);
#define cmp_poly_compress KEM_NAMESPACE(cmp_poly_compress)
int cmp_poly_compress (const unsigned char *r, const poly *a);
#define poly_packcompress_forvec KEM_NAMESPACE(poly_packcompress_forvec)
void poly_packcompress_forvec (uint8_t *r, poly *a, int i);
#define cmp_poly_packcompress_forvec KEM_NAMESPACE(cmp_poly_packcompress_forvec)
int cmp_poly_packcompress_forvec(const unsigned char *r, poly *a, int i);
#define poly_decompress KEM_NAMESPACE(poly_decompress)
void poly_decompress(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES]);
#define poly_unpackdecompress_forvec KEM_NAMESPACE(poly_unpackdecompress_forvec)
void poly_unpackdecompress_forvec (poly *r, const uint8_t *a, int i);


#define poly_tobytes KEM_NAMESPACE(poly_tobytes)
void poly_tobytes(uint8_t r[KEM_POLYBYTES], const poly *a);
#define poly_frombytes KEM_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const uint8_t a[KEM_POLYBYTES]);

#define poly_frommsg KEM_NAMESPACE(poly_frommsg)
void poly_frommsg(poly *r, const uint8_t msg[KEM_INDCPA_MSGBYTES]);
#define poly_tomsg KEM_NAMESPACE(poly_tomsg)
void poly_tomsg(uint8_t msg[KEM_INDCPA_MSGBYTES], const poly *r);

#define poly_getnoise_eta KEM_NAMESPACE(poly_getnoise_eta)
void poly_getnoise_eta(poly *r, const uint8_t seed[KEM_SYMBYTES], uint8_t nonce);

#define poly_ntt KEM_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt_tomont KEM_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_basemul_montgomery KEM_NAMESPACE(poly_basemul_montgomery)
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b);
#define poly_basemul_montgomery_acc KEM_NAMESPACE(poly_basemul_montgomery_acc)
void poly_basemul_montgomery_acc (poly *r, poly *a, poly *b);

#define poly_tomont KEM_NAMESPACE(poly_tomont)
void poly_tomont(poly *r);

#define poly_reduce KEM_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#define poly_freeze KEM_NAMESPACE(poly_freeze)
void poly_freeze(poly *r);

#define poly_add KEM_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_sub KEM_NAMESPACE(poly_sub)
void poly_sub(poly *r, const poly *a, const poly *b);

#define poly_addnoise_ntt KEM_NAMESPACE(poly_addnoise_ntt)
void poly_addnoise_ntt (poly *r, poly *e, int buf1len, uint8_t *noiseseed1, uint8_t *buf1);

#define poly_addnoise_nontt KEM_NAMESPACE(poly_addnoise_nontt)
void poly_addnoise_nontt (poly *r, poly *e, int buf1len, uint8_t *noiseseed1, uint8_t *buf1);


// void poly_freeze(poly *r);

void poly_decompress_file(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES]);

#endif
