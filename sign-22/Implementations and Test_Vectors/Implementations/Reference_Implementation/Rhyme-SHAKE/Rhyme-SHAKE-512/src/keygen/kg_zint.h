#ifndef RHYME_KG_ZINT_H
#define RHYME_KG_ZINT_H
#include <stdint.h>
#include <stddef.h>

/* signed magnitude bignum, 32-bit limbs little-endian. keygen-only (heap ok). */
typedef struct {
    int sign;          /* -1, 0, +1 */
    uint32_t len;      /* used limbs (0 iff sign == 0) */
    uint32_t cap;
    uint32_t *d;
} zint;

void zi_init(zint *z);
void zi_free(zint *z);
void zi_set_i64(zint *z, int64_t v);
void zi_copy(zint *r, const zint *a);
unsigned zi_bitlen(const zint *z);
int  zi_is_zero(const zint *z);
int  zi_cmp_abs(const zint *a, const zint *b);
void zi_add(zint *r, const zint *a, const zint *b);   /* r = a + b (aliasing ok) */
void zi_sub(zint *r, const zint *a, const zint *b);
void zi_mul(zint *r, const zint *a, const zint *b);   /* r must not alias */
void zi_neg(zint *z);
/* r += m * (u32 t)   (m >= 0 assumed for CRT use; handles general) */
void zi_addmul_u32(zint *r, const zint *m, uint32_t t);
/* r *= u32 */
void zi_mul_u32(zint *r, uint32_t t);
/* residue of z mod p (0 <= res < p), sign respected */
uint32_t zi_mod_u32(const zint *z, uint32_t p);
/* r -= (v << shiftbits), v signed 128-bit value */
void zi_sub_shifted_i128(zint *r, __int128 v, unsigned shiftbits);
/* top window: floor(|z| / 2^shift) as double with sign (shift chosen so result < 2^53) */
double zi_top_double(const zint *z, unsigned shift);
/* truncate to int64 (caller ensures fits) */
int64_t zi_to_i64(const zint *z);
/* extended gcd: g = gcd(|a|,|b|) > 0 with u*a + v*b = g (a,b nonzero) */
void zi_xgcd(zint *g, zint *u, zint *v, const zint *a, const zint *b);
/* gcd only (no Bezout), Lehmer-accelerated; for coprimality testing. */
void zi_gcd(zint *g, const zint *a, const zint *b);

#endif
