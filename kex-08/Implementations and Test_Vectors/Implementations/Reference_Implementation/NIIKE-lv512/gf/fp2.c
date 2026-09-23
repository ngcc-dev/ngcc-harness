// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from fp2.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#include <fp2.h>
#include <stdio.h>
#include "encode_sizes.h"

// extern const digit_t R[NWORDS_FIELD];

/* Arithmetic modulo X^2 + 1 */
const fp2_t fp2_1 = {{0x523b4e8ed3b65f4, 0x718540700464bde, 0xed286419d863d4, 0x590cca8cf47692e, 0x5570cee74cb92b3, 0xba878120d20695, 0x778d60195c69179, 0x509fcf233555b37, 0x515b6cac7ea3640, 0x5701eadb36cb0a2, 0x7475d6d258cf2b7, 0x256145e99453e31, 0x7400114ba520d74, 0x505c1c604e512d6, 0x53cb85cb0d1d441, 0x6ce35e9801fa78a, 0x26e79e9c09ab4ee, 0x11ff6a377b63b78, 0x4e2c0d9dc96c70c, 0x48f58e1c99ac095, 0x1662e655118b14a, 0x35b62e845a99eeb, 0x24c8c09b6816b54, 0x546ad23c9a9fcd8, 0x66f26d07dc2cf4c, 0x6a44e24a2931a6a, 0x28a6d86319f8b4f, 0x2320f83afbe13b0, 0x494a4e1ef371650, 0x7f58f28ac48c01d, 0x7df612edae58de9, 0x68ee734fe59f2e8, 0x18030a1661fb768, 0x26fff8c0b4800ab, 0x46f1e14c2c12bdd, 0x50c37d57a4a0ee, 0x3eea0dfc5c72b53, 0x72781ae1fcab38d, 0x41c232edf95b872, 0xf102528475a283, 0x675e0610651be18, 0x1b0a77c8cbda8ef, 0x6cd2a739770e85, 0x24676cb36a42289, 0x5a5efc3488370bb, 0x8620e9db6ba0fd, 0x7fd9767fb1b89a, 0x2b58e9623fc9b5c, 0x7502d87295b78e7, 0x6582fdd4289c4c8, 0x44b4f31a9ae12ed, 0x59ac8962d9fa1ad, 0x77ae98b640c1988, 0x709366ab5d7c081, 0x76e2a0390b638d0, 0x4fc49551c2da254, 0x604f27ce82e4607, 0x19990123a702e57, 0x6d6c5bd059c2243, 0x585ec2e5d3c2a3d, 0x2670605e2439df9, 0x53719545dfe615b, 0x1c413793eeb3908, 0x33ec85a84f59ff7, 0x4ff4a50d3fffbde, 0x29ba315c20bd186, 0x1d188e0f24ea1d6, 0x1ee9c196b9471a0, 0x38bbdae5be9c50a, 0x5067efff3da4dbf, 0x2c30dbbb7dbd806, 0x4d2ca0ef03f603a, 0x39f9ffb10725a4c, 0x410d70cb6351d84, 0x48528b326bd0102, 0x1918ce411ac2e82, 0x60bc976d2afab24, 0x445184eab92958b, 0x143e73d8e7e4909, 0x36a84a384173c70, 0x7945818777b3c7a, 0xdcb9f6651402b0, 0x3edbb130cc09707, 0x71b84d4d9385690, 0x112850b72a54505, 0x91b8473128af5e, 0x46b6c4eb53d20d3, 0x3cc0eee1dae76f7, 0x593d2686c4ace2, 0x4b7ca5464601bbc, 0x1c31273d066be03, 0x451815e7ac6c4cd, 0x24dd346bf78f8ac, 0x76434fa0c9f9c6f, 0x688f337747de258, 0x73059177b554d24, 0x23945f1cc5c1abe, 0x54ae2ae8aaa59ab, 0x67563e80bc3c184, 0x3214d3d07e526e0, 0x45a25c01eb3a02d, 0x48c6a3698a064bb, 0x4bf6eac697fd20c, 0x38a48411a4e72f4, 0x4738a0be233e0a, 0x30a23aae815b0e0, 0x453ddcc9a8da60f, 0x289d257a6d4ee61, 0x4fb073fce21c3c3, 0x12d8ae9ce457f30, 0x3e6732bfee1fa06, 0x1c409a64882c809, 0x6fd3b5ba8b0fb2c, 0x53840a5a9252a6a, 0x5882cf04e044ddc, 0xd3a35abf496bde, 0x2e153ec7612f1ab, 0x647cb6734e3e15b, 0x1efcc2dd79c4bb3, 0x2adb09117a2c48f, 0x4e596beccbf562b, 0x186f52876b7319f, 0x7a9b5ce65b18586, 0x6a20f63395b505f, 0x128efda0d1007b6, 0x6f40cd313af143c, 0x5944575a16ef64f, 0x23427ad2174bde0, 0x42df15ffa05aeb2, 0x3848a0e8cb940b4, 0x69b0f0178ac5648, 0x54a0705ff4243c3, 0x4d0d2617db5b6c, 0x7d03e7de27ef0d8, 0x20cfb8a55303998, 0x6ab3cb1acf74702, 0x53727bea7152065, 0x984d7080056805, 0x4b6e6bb1c8a523a, 0x1470594a26d360d, 0x7c96538835f2dcf, 0x17d9283091f5aaa, 0x550a3e122b9ac11, 0xbcd73956f2f91c, 0x71050b3d7e536aa, 0x5809adbcb6428ee, 0x527adb89030cec8, 0x5f3e5fbbcf41ce7, 0x40e3344b7fb2aba, 0x46d276a572652fe, 0xcf34370d92651c, 0xad3e9fbc8e8ee3, 0x34ae4c227143beb, 0x48e1b80df29a66c, 0x22c95380f0b703a, 0xeb94708bc98975, 0x722623d882fcb4d, 0x22b9907a5a974cc, 0x46f83478f728398, 0x540b41a270a2a54, 0x6b2ed2a5336475b, 0x50a0be732233946, 0x62ed6a823bd058b, 0x82e22289cf9bef, 0x4d9e90530b88f84, 0x55e4bf1cda1c9ec, 0x165dd4b5ca5fa79, 0x60ccf43824c3493, 0xb7664247a3559a, 0x4247b98e3003ade, 0x2820cd1eb56d638, 0x14ff9191cc5ca89, 0x2cf0ad19878bfae, 0x7636c86409e5b9c, 0x3ac5217a599892c, 0x43de4446075816f, 0x68d45a276226c8a, 0x541a15d9d3aaedf, 0x418ce98bcedf07a, 0x1c29aaac3c4444d, 0x2e80ec68c1086c5, 0x6fe9b8b54b3e1, 0x213e95d127989af, 0x7a3247664fbbb7e, 0x81d8b226b2d917, 0x3a52c23e11e8a71, 0x5c3c1f2c4ec4718, 0x7723fc45a0d148a, 0x225372b025640d4, 0x29beae7637f1ebf, 0x7becc2f3ed30a00, 0x62342fb21217b04, 0x7e75658ec26559f, 0x198f1f3d9528ecd, 0x43837cc4bef83c4, 0x622acc060eb25ca, 0x53563f5fecbb8a7, 0x16cba2be0d959c8, 0x28b5f6945e9e079, 0x77095acbd2d787f, 0x57c6b841b7b9e9f, 0x4b23e36fe314eea, 0x5df4f5c7eb43855, 0x2d2345f0a51cd9a, 0xc2f08d28a85713, 0x262764e8084c5df, 0x41cfa20b5673ebb, 0x42bdaf51b80c28d, 0x720c4e5a0473108, 0x79442adccabc9f4, 0x36936caa49f6851, 0x17a3945a3a838fb, 0xc67a29d5a76ff, 0x72c5592e130ad17, 0x42611c42c642c3d, 0x14e170469bfe33e, 0x64b627930cdb0de, 0x7cd2225148c23d5, 0x1ab81e67d509118, 0x1401f7fb7483d69, 0x435045455f1810e, 0x64091d529dcf714, 0x35063fd4cfd1edc, 0xda7d04471d2bd3, 0x4d3bc11b0b4899f, 0x2f4895cbbec89f9, 0x1a5764ce299af2e, 0x1a29488d557079c, 0x74d6731a7e7e6be, 0x2482eebd3bb5a4a, 0x7eb70b1a8060e3d, 0x7b8e349637aaecd, 0x9325e5b1cd1122, 0x7d487729155bdfa, 0x459b9791b02fe6, 0x59075b5bc800a0c, 0x5f444467cdbb50b, 0x71add4235fbed9f, 0x4e0660ce2eda64a, 0x40d351957c7273e, 0xf56bb2e7252132, 0x1d6c7696dded85a, 0x72ceba46f178967, 0x73c6393344dc8fe, 0x7fd2e6b15d49d7a, 0x7e076d082d05423, 0x2526f108f173880, 0x1ecd48815f27ce6, 0xdd063d787c5fd5, 0x78451761c58e37, 0x507b298517b9d54, 0x7adcf59fe2f85ec, 0x41d2feee929ee5d, 0x538ddea720800c2, 0x7d6b019fb112929, 0x7702f6068b877b1, 0x4e37e77402c2fcf, 0x4ee208d91c31777, 0x4e529e2ad66d8ca, 0x3689f9803a1d70e, 0x645d95672eeb60, 0x5ae84a0e9c70cb0, 0x239416d6ea2d7f0, 0x14df0b66fde4bbd, 0x509b9d661ec5dd0, 0x480fb876689ff71, 0x6d5395133ff7dae, 0x64631c679d29fbc, 0x449349b949b2f7d, 0x7b436cc226d4c95, 0x5faeab9335e8630, 0x5cbc20f2ef62682, 0x25d95d13d9172af, 0x1b3a6686be8239d, 0x54953d93bc09303, 0x21b829a3bf1e836, 0x56d981627b56852, 0x5619a18d8764}, {0}};

void fp2_set(fp2_t* x, const digit_t val)
{
    fp_set(x->re, val);
    fp_set(x->im, 0);
}

void fp2_setone(fp2_t *x)
{
    fp_mont_setone(&(x->re));
    fp_set(&(x->im), 0);
}

bool fp2_is_zero(const fp2_t* a)
{ // Is a GF(p^2) element zero?
  // Returns 1 (true) if a=0, 0 (false) otherwise

    return fp_is_zero(a->re)/* & fp_is_zero(a->im)*/;
}

uint32_t fp2_is_zero_mask(const fp2_t *a)
{
    return fp_is_zero_mask(a->re)/* & fp_is_zero_mask(a->im)*/;
}

bool fp2_is_equal(const fp2_t* a, const fp2_t* b)
{ // Compare two GF(p^2) elements in constant time
  // Returns 1 (true) if a=b, 0 (false) otherwise

    return fp_is_equal(a->re, b->re)/* & fp_is_equal(a->im, b->im)*/;
}

void fp2_copy(fp2_t* x, const fp2_t* y)
{
    fp_copy(x->re, y->re);
    fp_copy(x->im, y->im);
}

fp2_t fp2_non_residue()
{ // 2 + i is a quadratic non-residue for p1913
    fp_t one = {0};
    fp2_t res;

    one[0] = 1;
    fp_tomont(one, one);
    fp_add(res.re, one, one);
    fp_copy(res.im, one);
    return res;
}

void fp2_add(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    fp_add(x->re, y->re, z->re);
    // fp_add(x->im, y->im, z->im);
}

void fp2_sub(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    fp_sub(x->re, y->re, z->re);
    // fp_sub(x->im, y->im, z->im);
}

void fp2_neg(fp2_t* x, const fp2_t* y)
{
    fp_neg(x->re, y->re);
    // fp_neg(x->im, y->im);
}

void fp2_mul(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    // fp_t t0, t1;

    // fp_add(t0, y->re, y->im);
    // fp_add(t1, z->re, z->im);
    // fp_mul(t0, t0, t1);
    // fp_mul(t1, y->im, z->im);
    // fp_mul(x->re, y->re, z->re);
    // fp_sub(x->im, t0, t1);
    // fp_sub(x->im, x->im, x->re);
    // fp_sub(x->re, x->re, t1);
    fp_mul(x->re, y->re, z->re);
}

void fp2_sqr(fp2_t* x, const fp2_t* y)
{
    // fp_t sum, diff;

    // fp_add(sum, y->re, y->im);
    // fp_sub(diff, y->re, y->im);
    // fp_mul(x->im, y->re, y->im);
    // fp_add(x->im, x->im, x->im);
    // fp_mul(x->re, sum, diff);
    fp_sqr(x->re, y->re);
}

void fp2_inv(fp2_t* x)
{
    // fp_t t0, t1;

    // fp_sqr(t0, x->re);
    // fp_sqr(t1, x->im);
    // fp_add(t0, t0, t1);
    // fp_inv(t0);
    // fp_mul(x->re, x->re, t0);
    // fp_mul(x->im, x->im, t0);
    // fp_neg(x->im, x->im);
    fp_inv(x->re);
}

bool fp2_is_square(const fp2_t* x)
{
    fp_t t0, t1;

    fp_sqr(t0, x->re);
    fp_sqr(t1, x->im);
    fp_add(t0, t0, t1);

    return fp_is_square(t0);
}

void fp2_frob(fp2_t* x, const fp2_t* y)
{
    memcpy((digit_t*)x->re, (digit_t*)y->re, NWORDS_FIELD*RADIX/8);
    fp_neg(x->im, y->im);
}

void fp2_tomont(fp2_t* x, const fp2_t* y)
{ 
    fp_tomont(x->re, y->re);
    fp_tomont(x->im, y->im);
}

void fp2_frommont(fp2_t* x, const fp2_t* y)
{
    fp_frommont(x->re, y->re);
    fp_frommont(x->im, y->im);
}

// NOTE: old, non-constant-time implementation. Could be optimized
// void fp2_sqrt(fp2_t* x)
// {
//     fp_t sdelta, re, tmp1, tmp2, inv2, im;

//     if (fp_is_zero(x->im)) {
//         if (fp_is_square(x->re)) {
//             fp_sqrt(x->re);
//             return;
//         } else {
//             fp_neg(x->im, x->re);
//             fp_sqrt(x->im);
//             fp_set(x->re, 0);
//             return;
//         }
//     }

//     // sdelta = sqrt(re^2 + im^2)
//     fp_sqr(sdelta, x->re);
//     fp_sqr(tmp1, x->im);
//     fp_add(sdelta, sdelta, tmp1);
//     fp_sqrt(sdelta);

//     fp_set(inv2, 2);
//     fp_tomont(inv2, inv2);     // inv2 <- 2
//     fp_inv(inv2);
//     fp_add(re, x->re, sdelta);
//     fp_mul(re, re, inv2);
//     memcpy((digit_t*)tmp2, (digit_t*)re, NWORDS_FIELD*RADIX/8);

//     if (!fp_is_square(tmp2)) {
//         fp_sub(re, x->re, sdelta);
//         fp_mul(re, re, inv2);
//     }

//     fp_sqrt(re);
//     memcpy((digit_t*)im, (digit_t*)re, NWORDS_FIELD*RADIX/8);

//     fp_inv(im);
//     fp_mul(im, im, inv2);
//     fp_mul(x->im, im, x->im);    
//     memcpy((digit_t*)x->re, (digit_t*)re, NWORDS_FIELD*RADIX/8);
// }

void
fp2_sqrt(fp2_t *a)
{
    fp_set(&(a->im), 0);
    fp_t x0, x1, t0, t1;

    /* From "Optimized One-Dimensional SQIsign Verification on Intel and
     * Cortex-M4" by Aardal et al: https://eprint.iacr.org/2024/1563 */

    // x0 = \delta = sqrt(a0^2 + a1^2).
    fp_sqr(&x0, &(a->re));
    fp_sqr(&x1, &(a->im));
    fp_add(&x0, &x0, &x1);
    fp_sqrt(&x0);
    // If a1 = 0, there is a risk of \delta = -a0, which makes x0 = 0 below.
    // In that case, we restore the value \delta = a0.
    fp_select(&x0, &x0, &(a->re), fp_is_zero_mask(&(a->im)));
    // x0 = \delta + a0, t0 = 2 * x0.
    fp_add(&x0, &x0, &(a->re));
    fp_add(&t0, &x0, &x0);

    // x1 = t0^(p-3)/4
    fp_exp3div4(&x1, &t0);

    // x0 = x0 * x1, x1 = x1 * a1, t1 = (2x0)^2.
    fp_mul(&x0, &x0, &x1);
    fp_mul(&x1, &x1, &(a->im));
    fp_add(&t1, &x0, &x0);
    fp_sqr(&t1, &t1);
    // If t1 = t0, return x0 + x1*i, otherwise x1 - x0*i.
    fp_sub(&t0, &t0, &t1);
    uint32_t f = fp_is_zero_mask(&t0);
    fp_neg(&t1, &x0);
    fp_copy(&t0, &x1);
    fp_select(&t0, &t0, &x0, f);
    fp_select(&t1, &t1, &x1, f);

    // Check if t0 is zero
    uint32_t t0_is_zero = fp_is_zero_mask(&t0);

    // Check whether t0, t1 are odd
    // Note: we encode to ensure canonical representation
    uint8_t tmp_bytes[FP_ENCODED_BYTES];
    fp_encode(tmp_bytes, &t0);
    uint32_t t0_is_odd = -((uint32_t)tmp_bytes[0] & 1);
    fp_encode(tmp_bytes, &t1);
    uint32_t t1_is_odd = -((uint32_t)tmp_bytes[0] & 1);

    // We negate the output if:
    // t0 is odd, or
    // t0 is zero and t1 is odd
    uint32_t negate_output = t0_is_odd | (t0_is_zero & t1_is_odd);
    fp_neg(&x0, &t0);
    fp_select(&(a->re), &t0, &x0, negate_output);
    fp_neg(&x0, &t1);
    fp_select(&(a->im), &t1, &x0, negate_output);
}

// Lexicographic comparison of two field elements. Returns +1 if x > y, -1 if x < y, 0 if x = y
int fp2_cmp(fp2_t* x, fp2_t* y){
    fp2_t a, b;
    fp2_frommont(&a, x);
    fp2_frommont(&b, y);
    for(int i = NWORDS_FIELD-1; i >= 0; i--){
        if(a.re[i] > b.re[i])
            return 1;
        if(a.re[i] < b.re[i])
            return -1;
    }
    for(int i = NWORDS_FIELD-1; i >= 0; i--){
        if(a.im[i] > b.im[i])
            return 1;
        if(a.im[i] < b.im[i])
            return -1;
    }
    return 0;
}

void fp2_swap(fp2_t* P, fp2_t* Q, const digit_t option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    fp_swap(P->re, Q->re, option);
    fp_swap(P->im, Q->im, option);
}

/*
 * If ctl == 0x00000000, then *d is set to a0
 * If ctl == 0xFFFFFFFF, then *d is set to a1
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void fp2_select(fp2_t *d, const fp2_t *a0, const fp2_t *a1, uint32_t ctl)
{
    fp_select(&(d->re), &(a0->re), &(a1->re), ctl);
    fp_select(&(d->im), &(a0->im), &(a1->im), ctl);
}

// #  define __PRI64_PREFIX	"l"
// # define PRIx64		__PRI64_PREFIX "x"
// void fp2_print(fp2_t const a){
//     fp2_t b;
//     fp2_frommont(&b, &a);
//     printf("0x");
//     for(int i = NWORDS_FIELD - 1; i >=0; i--)
//         printf("%013" PRIx64, b.re[i]);
//     printf(" + a*0x");
//     for(int i = NWORDS_FIELD - 1; i >=0; i--)
//         printf("%013" PRIx64, b.im[i]);
// }

void
fp2_print(const fp2_t a)
{
    printf("0x");

    uint8_t buf[FP_ENCODED_BYTES];
    fp_encode(&buf, &(a.re)); // Encoding ensures canonical rep
    for (int i = 0; i < FP_ENCODED_BYTES; i++) {
        printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
    }

    printf(" + a*0x");

    fp_encode(&buf, &(a.im));
    for (int i = 0; i < FP_ENCODED_BYTES; i++) {
        printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
    }
    printf("\n");
}

void
fp2_encode(void *dst, const fp2_t *a)
{
    uint8_t *buf = dst;
    fp_encode(buf, &(a->re));
    fp_encode(buf + FP_ENCODED_BYTES, &(a->im));
}

uint32_t
fp2_decode(fp2_t *d, const void *src)
{
    const uint8_t *buf = src;
    uint32_t re, im;

    re = fp_decode(&(d->re), buf);
    im = fp_decode(&(d->im), buf + FP_ENCODED_BYTES);
    return re & im;
}

void
fp2_batched_inv(fp2_t *x, int len)
{
    fp2_t t1[len], t2[len];
    fp2_t inverse;

    // x = x0,...,xn
    // t1 = x0, x0*x1, ... ,x0 * x1 * ... * xn
    fp2_copy(&t1[0], &x[0]);
    for (int i = 1; i < len; i++) {
        fp2_mul(&t1[i], &t1[i - 1], &x[i]);
    }

    // inverse = 1/ (x0 * x1 * ... * xn)
    fp2_copy(&inverse, &t1[len - 1]);
    fp2_inv(&inverse);

    fp2_copy(&t2[0], &inverse);
    // t2 = 1/ (x0 * x1 * ... * xn), 1/ (x0 * x1 * ... * x(n-1)) , ... , 1/xO
    for (int i = 1; i < len; i++) {
        fp2_mul(&t2[i], &t2[i - 1], &x[len - i]);
    }

    fp2_copy(&x[0], &t2[len - 1]);

    for (int i = 1; i < len; i++) {
        fp2_mul(&x[i], &t1[i - 1], &t2[len - i - 1]);
    }
}