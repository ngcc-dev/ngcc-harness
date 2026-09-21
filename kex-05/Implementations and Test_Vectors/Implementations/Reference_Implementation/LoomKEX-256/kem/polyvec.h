#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[WEAVER_K];
} polyvec;

#define polyvec_compress WEAVER_NAMESPACE(_polyvec_compress)
void polyvec_compress(uint8_t r[WEAVER_POLYVECCOMPRESSEDBYTES], const polyvec *a);
#define polyvec_decompress WEAVER_NAMESPACE(_polyvec_decompress)
void polyvec_decompress(polyvec *r, const uint8_t a[WEAVER_POLYVECCOMPRESSEDBYTES]);

#ifdef PK_COMPRESS
#define polyvec_compress_pk WEAVER_NAMESPACE(_polyvec_compress_pk)
void polyvec_compress_pk(uint8_t r[WEAVER_PK_POLYVECBYTES], const polyvec *a);
#define polyvec_decompress_pk WEAVER_NAMESPACE(_polyvec_decompress_pk)
void polyvec_decompress_pk(polyvec *r, const uint8_t a[WEAVER_PK_POLYVECBYTES]);
#define polyvec_fromcompressed_pk WEAVER_NAMESPACE(_polyvec_fromcompressed_pk)
void polyvec_fromcompressed_pk(polyvec *r, const uint8_t a[WEAVER_PK_POLYVECBYTES]);
#endif

#define polyvec_tobytes WEAVER_NAMESPACE(_polyvec_tobytes)
void polyvec_tobytes(uint8_t r[WEAVER_POLYVECBYTES], const polyvec *a);
#define polyvec_frombytes WEAVER_NAMESPACE(_polyvec_frombytes)
void polyvec_frombytes(polyvec *r, const uint8_t a[WEAVER_POLYVECBYTES]);

#define polyvec_ntt WEAVER_NAMESPACE(_polyvec_ntt)
void polyvec_ntt(polyvec *r);
#define polyvec_invntt_tomont WEAVER_NAMESPACE(_polyvec_invntt_tomont)
void polyvec_invntt_tomont(polyvec *r);

#define polyvec_basemul_acc_montgomery WEAVER_NAMESPACE(_polyvec_basemul_acc_montgomery)
void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b);

#define polyvec_reduce WEAVER_NAMESPACE(_polyvec_reduce)
void polyvec_reduce(polyvec *r);

#define polyvec_add WEAVER_NAMESPACE(_polyvec_add)
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);

#endif
