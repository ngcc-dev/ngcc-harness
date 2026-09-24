#include "verify.h"
#include <stddef.h>
#include <stdint.h>

#include "parameters.h"

/* When AVX2 or AArch64 is enabled, all functions are in platform-specific files */
#if defined(DKE_USE_AVX2)
  /* verify, cmov, cmov_int16 supplied by avx2/verify_avx2.c */
#elif defined(DKE_USE_AARCH64)
  /* verify, cmov, cmov_int16 supplied by aarch64/verify_neon.c */
#else

#define D_PORTING

int DKE_verify(const uint8_t *a, const uint8_t *b, size_t len) {
    size_t i;
    uint8_t r = 0;
    for (i = 0; i < len; i++)
        r |= a[i] ^ b[i];
#ifdef D_PORTING
    return (~(uint64_t)r + 1) >> 63;
#else
    return (-(uint64_t)r) >> 63;
#endif
}

void DKE_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    size_t i;
    b = -b;
    for (i = 0; i < len; i++)
        r[i] ^= b & (r[i] ^ x[i]);
}

void DKE_cmov_int16(int16_t *r, int16_t v, uint16_t b) {
    b = -b;
    *r ^= b & ((*r) ^ v);
}

#endif /* !DKE_USE_AVX2 && !DKE_USE_AARCH64 */
