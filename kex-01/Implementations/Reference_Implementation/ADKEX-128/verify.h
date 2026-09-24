#ifndef DKE_VERIFY_H
#define DKE_VERIFY_H

#include <stddef.h>
#include <stdint.h>

int DKE_verify(const uint8_t *a, const uint8_t *b, size_t len);
void DKE_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b);
void DKE_cmov_int16(int16_t *r, int16_t v, uint16_t b);

#endif
