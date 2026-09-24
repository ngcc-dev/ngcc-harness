/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <errno.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#elif __NetBSD__
#include <sys/random.h>
#else
#include <unistd.h>
#endif
#endif

#define DRNG_SUCCESS 0

#ifdef _WIN32
static void randombytes(unsigned char *out, size_t outlen)
{
    HCRYPTPROV ctx;
    size_t len;

    if (!CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        abort();

    while (outlen > 0)
    {
        len = (outlen > 1048576) ? 1048576 : outlen;
        if (!CryptGenRandom(ctx, len, (BYTE *)out))
            abort();

        out += len;
        outlen -= len;
    }

    if (!CryptReleaseContext(ctx, 0))
        abort();
}
#elif defined(__linux__) && defined(SYS_getrandom)
static void randombytes(unsigned char *out, size_t outlen)
{
    ssize_t ret;

    while (outlen > 0)
    {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR)
            continue;
        else if (ret == -1)
            abort();

        out += ret;
        outlen -= ret;
    }
}
#elif defined(__NetBSD__)
static void randombytes(unsigned char *out, size_t outlen)
{
    ssize_t ret;

    while (outlen > 0)
    {
        ret = getrandom(out, outlen, 0);
        if (ret == -1 && errno == EINTR)
            continue;
        else if (ret == -1)
            abort();

        out += ret;
        outlen -= ret;
    }
}
#else
static void randombytes(unsigned char *out, size_t outlen)
{
    static int fd = -1;
    ssize_t ret;

    while (fd == -1)
    {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR)
            continue;
        else if (fd == -1)
            abort();
    }

    while (outlen > 0)
    {
        ret = read(fd, out, outlen);
        if (ret == -1 && errno == EINTR)
            continue;
        else if (ret == -1)
            abort();

        out += ret;
        outlen -= ret;
    }
}
#endif

int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes)
{
    (void)seed;
    (void)seed_len_bytes;

    if (drng != NULL)
        memset(drng, 0, sizeof(*drng));

    return DRNG_SUCCESS;
}

int get_random_number(DRNG_ctx *drng, unsigned char *random_number, unsigned long long random_number_len_bits)
{
    size_t random_number_len_bytes;
    unsigned int remaining_bits;

    (void)drng;

    random_number_len_bytes = (size_t)(random_number_len_bits / 8);
    remaining_bits = (unsigned int)(random_number_len_bits & 7);
    if (remaining_bits != 0)
        random_number_len_bytes++;

    randombytes(random_number, random_number_len_bytes);

    if (remaining_bits != 0 && random_number_len_bytes != 0)
        random_number[random_number_len_bytes - 1] &= (unsigned char)(0xFFU << (8 - remaining_bits));

    return DRNG_SUCCESS;
}
