/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_MACROS_H
#define SYDO_MACROS_H

#include "sydo_defines.h"

#include <stdint.h>

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define ASSUME(p) ((void)(p))
#define SYDO_UNUSED(x) x

#define ATTR_NONNULL
#define ATTR_NONNULL_ARG(i)
#define ATTR_DTOR
#define ATTR_ASSUME_ALIGNED(i)
#define ATTR_ALIGNED(i)
#define ATTR_ALWAYS_INLINE
#define ATTR_PURE
#define ATTR_CONST
#define ATTR_ARTIFICIAL
#define ATTR_MALLOC(arg)
#define ATTR_ALLOC_ALIGN(arg)
#define ATTR_ALLOC_SIZE(arg)
#define ATTR_DEPRECATED

#define ALIGNT(s, t) (((s) + sizeof(t) - 1) & ~(sizeof(t) - 1))
#define ALIGNU64T(s) ALIGNT(s, uint64_t)
#define UNREACHABLE ((void)0)
#define ASSUME_ALIGNED(p, a) (p)

#define CONCAT2(a, b) a##_##b
#define CONCAT(a, b) CONCAT2(a, b)

#define SIZET_FMT "%zu"
#define sydo_declassify(x, len) ((void)(x), (void)(len))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#endif
