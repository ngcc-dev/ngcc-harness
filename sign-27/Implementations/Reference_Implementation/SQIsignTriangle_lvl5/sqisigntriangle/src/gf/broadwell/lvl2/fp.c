#include <assert.h>
#include "fp.h"

const digit_t p[NWORDS_FIELD] = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x011FFFFFFFFFFFFF };
const digit_t p2[NWORDS_FIELD] = { 0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x023FFFFFFFFFFFFF };

void fp_sqrt(fp_t *x) { fp_t tmp = *x; (void)gf9309_sqrt(x, &tmp); }

uint32_t fp_is_square(const fp_t *a) {
    int32_t ls = gf9309_legendre(a);
    return ~(uint32_t)(ls >> 1);
}

void fp_inv(fp_t *x) { fp_t tmp = *x; (void)gf9309_invert(x, &tmp); }

// a <- a^((p-3)/4)  (fixed public exponent square-and-multiply)
static const uint64_t EXP_E34[5] = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x0047FFFFFFFFFFFF };
void fp_exp3div4(fp_t *a) {
    fp_t tmp = *a;
    gf9309_pow(a, &tmp, EXP_E34, 311);
}
