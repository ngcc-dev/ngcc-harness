#ifndef AVX2_H
#define AVX2_H

#include <stdint.h>
#include "params.h"

void poly_tobytes_avx2(uint8_t r[restrict NTRUOAEP_POLYBYTES],
                       const int16_t a[restrict NTRUOAEP_N]);
void poly_frombytes_avx2(int16_t r[restrict NTRUOAEP_N],
                         const uint8_t a[restrict NTRUOAEP_POLYBYTES]);
void poly_cbd1_avx2(int16_t r[NTRUOAEP_N], const uint8_t buf[NTRUOAEP_N / 4]);
int poly_cbd1_inv_avx2(uint8_t *w, const int16_t a[NTRUOAEP_N],
                       const uint8_t buf[NTRUOAEP_N / 8]);
int poly_sotp_inv_avx2(uint8_t *msg, const int16_t a[NTRUOAEP_N],
                       const uint8_t *buf);
void short_poly_to_bytes_avx2(uint8_t *buf, const int16_t a[NTRUOAEP_N]);
void poly_sub_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                   const int16_t b[NTRUOAEP_N]);
void poly_triple_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void poly_crepmod3_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void poly_tomontgomery_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void poly_frommontgomery_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
int poly_baseinv_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
int poly_baseinv_asm(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void poly_basemul_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                       const int16_t b[NTRUOAEP_N]);
void poly_basemul_asm(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                      const int16_t b[NTRUOAEP_N]);
void poly_baseadd_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                       const int16_t c[NTRUOAEP_N]);
void poly_baseadd_asm(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                      const int16_t c[NTRUOAEP_N]);

#endif
