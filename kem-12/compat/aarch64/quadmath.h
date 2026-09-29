/* Harness compatibility header for AArch64, which has no libquadmath: there
 * long double is IEEE binary128, the format of __float128, so the type and the
 * functions ntru_solver.c uses map onto it (the Q literal suffix is accepted as
 * is). Used only on aarch64 (see ../../Makefile); the submitted KATs pass. */
#ifndef NGCC_COMPAT_QUADMATH_H
#define NGCC_COMPAT_QUADMATH_H
#include <complex.h>
#include <math.h>
typedef long double __float128;
typedef _Complex long double __complex128;
#define sinq sinl
#define cosq cosl
#define crealq creall
#define cimagq cimagl
#define llroundq llroundl
#endif
