/*
This folder contains a custom "scaled-up Xoodoo-like" permutation with:
  - 3 rows (planes)
  - 8 columns
  - 64-bit lanes

It follows 5-step round structure as Xoodoo:
  iota -> chi -> rho -> theta -> rho

*/

#ifndef _ZuD1536_h_
#define _ZuD1536_h_

#include <stdint.h>
#include <stdlib.h>

#define ZUD1536_MAXROUNDS   12
#define ZUD1536_NROWS       3
#define ZUD1536_NCOLUMNS    8
#define ZUD1536_NLANES      (ZUD1536_NROWS*ZUD1536_NCOLUMNS)
#define ZUD1536_STATE_BYTES (ZUD1536_NLANES*8)

/* Round constants (derived from Xoodoo 32-bit constants, zero-extended). */
#define ZUD1536_rc12   0x0000000000000058ULL
#define ZUD1536_rc11   0x0000000000000038ULL
#define ZUD1536_rc10   0x00000000000003C0ULL
#define ZUD1536_rc9    0x00000000000000D0ULL
#define ZUD1536_rc8    0x0000000000000120ULL
#define ZUD1536_rc7    0x0000000000000014ULL
#define ZUD1536_rc6    0x0000000000000060ULL
#define ZUD1536_rc5    0x000000000000002CULL
#define ZUD1536_rc4    0x0000000000000380ULL
#define ZUD1536_rc3    0x00000000000000F0ULL
#define ZUD1536_rc2    0x00000000000001A0ULL
#define ZUD1536_rc1    0x0000000000000012ULL

#if !defined(ROTL64)
    #if defined(_MSC_VER)
        #define ROTL64(a, offset) _rotl64((uint64_t)(a), (int)((offset) & 63))
    #else
        #define ROTL64(a, offset) ( (((offset) & 63) != 0) ? \
            ((((uint64_t)(a)) << ((offset) & 63)) ^ (((uint64_t)(a)) >> (64 - ((offset) & 63)))) : \
            ((uint64_t)(a)) )
    #endif
#endif

#if !defined(READ64_UNALIGNED)
    /* Portable unaligned load using memcpy (avoids alignment/aliasing issues). */
    #include <string.h>
    static inline uint64_t READ64_UNALIGNED(const void *p)
    {
        uint64_t v;
        memcpy(&v, p, sizeof(v));
        return v;
    }
#endif

#if !defined(WRITE64_UNALIGNED)
    #include <string.h>
    static inline void WRITE64_UNALIGNED(void *p, uint64_t v)
    {
        memcpy(p, &v, sizeof(v));
    }
#endif

#endif
