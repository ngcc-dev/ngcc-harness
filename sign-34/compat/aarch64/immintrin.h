/* Harness compatibility header for non-x86 hosts: keygen/ntrugen/src/bigint.c
 * includes <immintrin.h> unconditionally, but its AVX2/AVX-512 code is guarded
 * by __AVX2__/__AVX512IFMA__ and the portable path uses unsigned __int128, so
 * nothing from the header is needed. Used only on aarch64 (see ../../Makefile). */
#ifndef NGCC_COMPAT_IMMINTRIN_H
#define NGCC_COMPAT_IMMINTRIN_H
#include <stddef.h>
#include <stdlib.h>
#endif
