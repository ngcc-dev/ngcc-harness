#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "auxfunc.h"

/*
 * hash_h: 输出 SEEDBYTES 字节
 *   128/256: sm3hash  → 256 bit = 32 字节
 *   512:     pseudohash → 512 bit = 64 字节
 *
 * hash_g: 输出 2*SEEDBYTES 字节
 *   128/256: pseudohash → 512 bit = 64 字节
 *   512:     pseudohash → 1024 bit = 128 字节
 */
#if KEM_MODE == 128 || KEM_MODE == 256

#define hash_h(OUT, IN, INBYTES) \
    sm3hash(256, \
            (const unsigned char *)(IN), \
            (unsigned long long)(INBYTES) * 8, \
            (unsigned char *)(OUT))

#define hash_g(OUT, IN, INBYTES) \
    pseudohash(512, \
               (const unsigned char *)(IN), \
               (unsigned long long)(INBYTES) * 8, \
               (unsigned char *)(OUT))

#elif KEM_MODE == 512

#define hash_h(OUT, IN, INBYTES) \
    pseudohash(512, \
               (const unsigned char *)(IN), \
               (unsigned long long)(INBYTES) * 8, \
               (unsigned char *)(OUT))

#define hash_g(OUT, IN, INBYTES) \
    pseudohash(1024, \
               (const unsigned char *)(IN), \
               (unsigned long long)(INBYTES) * 8, \
               (unsigned char *)(OUT))
#endif

#define prf(OUT, OUTBYTES, KEY, NONCE) \
    do { \
        uint8_t _prf_buf_[SEEDBYTES + 1]; \
        memcpy(_prf_buf_, (KEY), SEEDBYTES); \
        _prf_buf_[SEEDBYTES] = (uint8_t)(NONCE); \
        pseudoXOF((unsigned long long)(OUTBYTES) * 8, \
                  (const unsigned char *)_prf_buf_, \
                  (unsigned long long)(SEEDBYTES + 1) * 8, \
                  (unsigned char *)(OUT)); \
    } while(0)

#define kdf(OUT, IN, INBYTES) \
    pseudoXOF((unsigned long long)(SEEDBYTES) * 8, \
              (const unsigned char *)(IN), \
              (unsigned long long)(INBYTES) * 8, \
              (unsigned char *)(OUT))

#endif