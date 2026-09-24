#include "fixpoint.h"
#include "params.h"
#include <stdint.h>
#include <stdlib.h>

static void __cneg(fp96_76 *x, const uint8_t sign) {
    x->limb48[0] ^= (-(int64_t)sign) & ((1ULL << 48) - 1);
    x->limb48[1] ^= -(int64_t)sign;
    x->limb48[0] += sign;
    renormalize(x);
}

static void __copy_cneg(fp96_76 *y, const fp96_76 *x, const uint8_t sign) {
    y->limb48[0] = ((-(int64_t)sign) & ((1ULL << 48) - 1)) ^ x->limb48[0];
    ;
    y->limb48[1] = x->limb48[1] ^ (-(int64_t)sign);
    y->limb48[0] += sign;
    renormalize(y);
}

static void fixpoint_mul(fp96_76 *xy, const fp96_76 *x, const fp96_76 *y) {
    uint64_t tmp[2];
    mul48(&xy->limb48[0], x->limb48[0], y->limb48[0]);

    // shift right by 48, rounding
    xy->limb48[0] = xy->limb48[1] + (((xy->limb48[0] >> 47) + 1) >> 1);

    mul48(tmp, x->limb48[0], y->limb48[1]);
    xy->limb48[0] += tmp[0];
    xy->limb48[1] = tmp[1];
    mulacc48(&xy->limb48[0], x->limb48[1], y->limb48[0]);

    // shift right by 28, rounding
    xy->limb48[0] += 1UL << 27;
    xy->limb48[0] >>= 28;
    xy->limb48[0] += (xy->limb48[1] << 20) & ((1ULL << 48) - 1);
    xy->limb48[1] >>= 28;

    mul64(tmp, x->limb48[1], y->limb48[1]);
    xy->limb48[0] += (tmp[0] << 20) & ((1ULL << 48) - 1);
    xy->limb48[1] += (tmp[0] >> 28) + (tmp[1] << 36);

    renormalize(xy);
}

static void fixpoint_unsigned_signed_mul(fp96_76 *xy, const fp96_76 *y) {
    fp96_76 x, z;
    uint8_t sign = (y->limb48[1] >> 63) & 1;
    __copy_cneg(&x, y, sign);
    fixpoint_mul(&z, &x, xy);
    __copy_cneg(xy, &z, sign);
}

static void fixpoint_sub(fp96_76 *xminy, const fp96_76 *x, const fp96_76 *y) {
    fp96_76 yneg;
    __copy_cneg(&yneg, y, 1);
    fixpoint_add(xminy, x, &yneg);
}

static void fixpoint_sub_from_threehalves(fp96_76 *x) {
    __cneg(x, 1);
    x->limb48[1] += 3ULL << 27; // left shift by 28 would be "3"
    renormalize(x);
}

void fixpoint_square(fp96_76 *sqx, const fp96_76 *x) {
    uint64_t tmp[2];
    sq48(&sqx->limb48[0], x->limb48[0]);

    // shift right by 48, rounding
    //sqx->limb48[0] += 1ULL << 47;
    sqx->limb48[0] >>= 48;
    sqx->limb48[0] += sqx->limb48[1];

    // mul
    mul48(tmp, x->limb48[0], x->limb48[1]);
    sqx->limb48[0] += tmp[0] << 1;
    sqx->limb48[1] = tmp[1] << 1;

    // shift right by 28, rounding
    //sqx->limb48[0] += 1ULL << 27;
    sqx->limb48[0] >>= 28;
    sqx->limb48[0] += (sqx->limb48[1] << 20) & ((1ULL << 48) - 1);
    sqx->limb48[1] >>= 28;

    sq64(tmp, x->limb48[1]);
    sqx->limb48[0] += (tmp[0] << 20) & ((1ULL << 48) - 1);
    sqx->limb48[1] += (tmp[0] >> 28) + (tmp[1] << 36);

    renormalize(sqx);
}

// start_cube = hex(round(2^76/(sqrt((K + L)*N + 2)^3)))
// start_times_threehalves = hex(round(2^76 * 3/(2 * sqrt((K + L)*N + 2))))
#if SIGN_MODE == 128
const fp96_76 start_cube = {.limb48 = {0x770077e2e41aULL, 0x1162ULL}};
const fp96_76 start_times_threehalves = {.limb48 = {0x693861ad937bULL, 0x9caa56ULL}};
#elif SIGN_MODE == 256
const fp96_76 start_cube = {.limb48 = {0x107f5256727ULL, 0x816ULL}};
const fp96_76 start_times_threehalves = {.limb48 = {0x7a75107b7db5ULL, 0x796251ULL}};
#elif SIGN_MODE == 512
const fp96_76 start_cube = {.limb48 = {0x491fa9186aabULL, 0x2dcULL}};
const fp96_76 start_times_threehalves = {.limb48 = {0x912fd7c9421fULL, 0x55d926ULL}};
#endif
// implements Newton's method
void fixpoint_newton_invsqrt(fp96_76 *invsqrtx, const fp96_76 *xhalf) {
    fp96_76 tmp, tmp2;
    fixpoint_mul(&tmp, xhalf, &start_cube); // definitely two positive values
    fixpoint_sub(invsqrtx, &start_times_threehalves,
                 &tmp); // first Newton iteration done, might be negative (very
                        // improbable)

    for (int i = 0; i < 6; i++) // 6 more iterations
    {
        fixpoint_square(&tmp, invsqrtx);  // tmp = y^2, never negative
        fixpoint_mul(&tmp2, xhalf, &tmp); // tmp2 = x/2 * y^2, never negative
        fixpoint_sub_from_threehalves(&tmp2);          // tmp = 3/2 - x/2 * y^2
        fixpoint_unsigned_signed_mul(invsqrtx, &tmp2); // y * (3/2 - x/2 * y^2)
    }
}

int32_t fixpoint_mul_rnd(const uint64_t x, const fp96_76 *y,
                         const uint8_t sign) {
    int64_t res;
    fp96_76 tmp, xx;
    xx.limb48[1] = x >> 32;
    xx.limb48[0] = (x & ((1ULL << 32) - 1)) << 16;
    fixpoint_mul(&tmp, &xx, y);
    res = (tmp.limb48[1] + (1UL << (LNBITS - 1))) >> LNBITS; // rounding
    return (1 - 2 * (int32_t)sign) * res;
}

void fixpoint_add(fp96_76 *xy, const fp96_76 *x, const fp96_76 *y) {
    xy->limb48[0] = x->limb48[0] + y->limb48[0];
    xy->limb48[1] = x->limb48[1] + y->limb48[1];
}