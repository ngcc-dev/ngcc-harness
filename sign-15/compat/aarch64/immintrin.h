/* Harness compatibility header for non-x86 hosts: lwrdsa256/rounding.c and
 * lwrdsa512/poly.c include <immintrin.h> but use no intrinsics; they rely only
 * on the declarations the x86 header pulls in (size_t, malloc). Used only on
 * aarch64 (see ../../Makefile). */
#ifndef NGCC_COMPAT_IMMINTRIN_H
#define NGCC_COMPAT_IMMINTRIN_H
#include <stddef.h>
#include <stdlib.h>
#endif
