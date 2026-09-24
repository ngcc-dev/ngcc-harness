/* Fallback implementations to avoid implicit-function-declaration errors
 * This header is force-included by the Makefile for the standalone bundle.
 */
#ifndef FALLBACKS_H
#define FALLBACKS_H

#include <string.h>
#include <stddef.h>

/* explicit_bzero: provide a simple inline fallback */
static inline void explicit_bzero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) *p++ = 0;
}

/* mempcpy: copy and return pointer to byte after last written */
static inline void *mempcpy(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
    return (void *)((char *)dst + n);
}

#endif /* FALLBACKS_H */
