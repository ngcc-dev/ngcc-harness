/* Harness compatibility header for non-x86 hosts: the reference build includes
 * <immintrin.h> (common/parsing.h, common/vector.h) but uses no intrinsics; it
 * relies only on the declarations the x86 header pulls in (size_t, malloc).
 * Used only on aarch64 (see ../../Makefile); the submitted KATs pass. */
#ifndef NGCC_COMPAT_IMMINTRIN_H
#define NGCC_COMPAT_IMMINTRIN_H
#include <stddef.h>
#include <stdlib.h>
#endif
