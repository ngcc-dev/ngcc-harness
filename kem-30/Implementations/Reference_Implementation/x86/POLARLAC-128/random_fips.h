/*
File Description: Switchable random-number wrapper for the KEM implementation.
*/

#ifndef RANDOM_FIPS_H
#define RANDOM_FIPS_H

#include <stddef.h>

#include "drng.h"



#if BIT_USE_SHAKE
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#elif defined(__NetBSD__)
#include <sys/random.h>
#endif
#endif

#define POLARLAC_RANDOM_DIVISION_ROUND_UP(dividend, divisor, result) \
    do                                                              \
    {                                                               \
        (result) = (dividend) / (divisor);                          \
        if ((dividend) % (divisor))                                 \
            (result)++;                                             \
    } while (0)

static int get_random_number_fips(DRNG_ctx *drng, unsigned char *random_number,
    unsigned long long random_number_len_bits)
{
    unsigned long long random_number_len_bytes;
    size_t outlen;
    unsigned char *out;

    (void)drng;
    if (random_number == NULL) {
        return -1;
    }

    POLARLAC_RANDOM_DIVISION_ROUND_UP(random_number_len_bits, 8,
        random_number_len_bytes);
    if (random_number_len_bytes > (unsigned long long)((size_t)-1)) {
        return -1;
    }

    out = random_number;
    outlen = (size_t)random_number_len_bytes;

#ifdef _WIN32
    HCRYPTPROV ctx;
    size_t len;

    if (!CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT)) {
        return -1;
    }
    while (outlen > 0) {
        len = (outlen > 1048576U) ? 1048576U : outlen;
        if (!CryptGenRandom(ctx, (DWORD)len, (BYTE *)out)) {
            (void)CryptReleaseContext(ctx, 0);
            return -1;
        }
        out += len;
        outlen -= len;
    }
    if (!CryptReleaseContext(ctx, 0)) {
        return -1;
    }
#elif defined(__linux__) && defined(SYS_getrandom)
    ssize_t ret;

    while (outlen > 0) {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        if (ret <= 0) {
            return -1;
        }
        out += (size_t)ret;
        outlen -= (size_t)ret;
    }
#elif defined(__NetBSD__)
    ssize_t ret;

    while (outlen > 0) {
        ret = getrandom(out, outlen, 0);
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        if (ret <= 0) {
            return -1;
        }
        out += (size_t)ret;
        outlen -= (size_t)ret;
    }
#else
    static int fd = -1;
    ssize_t ret;

    while (fd == -1) {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR) {
            continue;
        }
        if (fd == -1) {
            return -1;
        }
    }

    while (outlen > 0) {
        ret = read(fd, out, outlen);
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        if (ret <= 0) {
            return -1;
        }
        out += (size_t)ret;
        outlen -= (size_t)ret;
    }
#endif

    if ((random_number_len_bits & 7ULL) != 0 && random_number_len_bytes != 0) {
        unsigned int valid_bits = (unsigned int)(random_number_len_bits & 7ULL);
        random_number[random_number_len_bytes - 1] &=
            (unsigned char)(0xFFU << (8U - valid_bits));
    }

    return 0;
}

#endif

static inline int polarlac_get_random_number(DRNG_ctx *drng,
    unsigned char *random_number, unsigned long long random_number_len_bits)
{
#if BIT_USE_SHAKE
    return get_random_number_fips(drng, random_number, random_number_len_bits);
#else
    return get_random_number(drng, random_number, random_number_len_bits);
#endif
}

#endif
