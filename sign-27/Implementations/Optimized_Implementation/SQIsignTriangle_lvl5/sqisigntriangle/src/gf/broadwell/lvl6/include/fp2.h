#ifndef FP2_H
#define FP2_H

#define NO_FP2X_MUL
#define NO_FP2X_SQR

#include <fp2x.h>

extern void fp2_sq_c0(fp2_t *out, const fp2_t *in);
extern void fp2_sq_c1(fp_t *out, const fp2_t *in);
extern void fp2_mul_c0(fp_t *out, const fp2_t *in0, const fp2_t *in1);
extern void fp2_mul_c1(fp_t *out, const fp2_t *in0, const fp2_t *in1);

static inline void
fp2_mul(fp2_t *x, const fp2_t *y, const fp2_t *z)
{
    fp_t t;
    fp2_mul_c0(&t, y, z);
    fp2_mul_c1(&x->im, y, z);
    x->re.arr[0] = t.arr[0];
    x->re.arr[1] = t.arr[1];
    x->re.arr[2] = t.arr[2];
    x->re.arr[3] = t.arr[3];
    x->re.arr[4] = t.arr[4];
    x->re.arr[5] = t.arr[5];
    x->re.arr[6] = t.arr[6];
    x->re.arr[7] = t.arr[7];
    x->re.arr[8] = t.arr[8];
    x->re.arr[9] = t.arr[9];
    x->re.arr[10] = t.arr[10];
    x->re.arr[11] = t.arr[11];
    x->re.arr[12] = t.arr[12];
    x->re.arr[13] = t.arr[13];
    x->re.arr[14] = t.arr[14];
    x->re.arr[15] = t.arr[15];
}

static inline void
fp2_sqr(fp2_t *x, const fp2_t *y)
{
    fp2_t t;
    fp2_sq_c0(&t, y);
    fp2_sq_c1(&x->im, y);
    x->re.arr[0] = t.re.arr[0];
    x->re.arr[1] = t.re.arr[1];
    x->re.arr[2] = t.re.arr[2];
    x->re.arr[3] = t.re.arr[3];
    x->re.arr[4] = t.re.arr[4];
    x->re.arr[5] = t.re.arr[5];
    x->re.arr[6] = t.re.arr[6];
    x->re.arr[7] = t.re.arr[7];
    x->re.arr[8] = t.re.arr[8];
    x->re.arr[9] = t.re.arr[9];
    x->re.arr[10] = t.re.arr[10];
    x->re.arr[11] = t.re.arr[11];
    x->re.arr[12] = t.re.arr[12];
    x->re.arr[13] = t.re.arr[13];
    x->re.arr[14] = t.re.arr[14];
    x->re.arr[15] = t.re.arr[15];
}

#endif
