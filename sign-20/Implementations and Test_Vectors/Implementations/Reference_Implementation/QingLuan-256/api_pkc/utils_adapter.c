/*
 * QingLuan utils — API_PKC integration variant
 *
 * randombytes() is bridged to the framework-managed drng_algorithm.
 * drbg_init() becomes a no-op: DRNG is (re)seeded per test vector
 * by KAT_SIG.c before sig_keygen/sig_sign are called.
 *
 * Security-critical helpers (ct_memcmp, secure_zero, endianness helpers)
 * keep the Reference_Implementation behavior.
 */

#include "utils.h"
#include "drng.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* drng_algorithm lives in KAT_SIG.c translation unit */
extern DRNG_ctx drng_algorithm;

int drbg_init(void)
{
    /* DRNG lifetime is owned by KAT_SIG.c; nothing to do here. */
    return 0;
}

int randombytes(uint8_t *out, size_t len)
{
    /* API_PKC DRNG takes a *bit* length; QingLuan operates in bytes. */
    if (get_random_number(&drng_algorithm, out, (unsigned long long)len * 8) != 0)
        return -1;
    return 0;
}

int ct_memcmp(const void *a, const void *b, size_t len)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) diff |= pa[i] ^ pb[i];
    return (int)diff;
}

void ct_cswap(uint8_t *a, uint8_t *b, size_t len, int condition)
{
    uint8_t mask = (uint8_t)(-(condition & 1));
    for (size_t i = 0; i < len; i++) {
        uint8_t t = mask & (a[i] ^ b[i]);
        a[i] ^= t;
        b[i] ^= t;
    }
}

void u32_to_be(uint8_t *out, uint32_t v)
{
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v);
}

uint32_t be_to_u32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) |
           ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8)  |
           (uint32_t)in[3];
}

void secure_zero(void *ptr, size_t len)
{
    if (ptr == NULL || len == 0) return;
#ifdef _WIN32
    SecureZeroMemory(ptr, len);
#elif defined(__GNUC__) || defined(__clang__)
    memset(ptr, 0, len);
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#else
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    for (size_t i = 0; i < len; i++) p[i] = 0;
#endif
}
