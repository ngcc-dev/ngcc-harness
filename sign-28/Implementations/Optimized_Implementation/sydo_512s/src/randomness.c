/*
 *  SPDX-License-Identifier: MIT
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "randomness.h"
#include "drng.h"

#include <stddef.h>
#include <stdint.h>

#define DRNG_SEED_BYTES 64

#if defined(__GLIBC__)
#define GLIBC_CHECK(maj, min) __GLIBC_PREREQ(maj, min)
#else
#define GLIBC_CHECK(maj, min) 0
#endif

#if (defined(HAVE_SYS_RANDOM_H) && defined(HAVE_GETRANDOM)) ||                                    \
    (defined(__linux__) && GLIBC_CHECK(2, 25))
#include <sys/random.h>
#elif defined(HAVE_ARC4RANDOM_BUF)
#include <stdlib.h>
#elif defined(__APPLE__) && defined(HAVE_APPLE_FRAMEWORK)
#include <Security/Security.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NETBSD__) ||   \
    defined(__NetBSD__)
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#error "Unsupported OS! Please implement seed_drng_from_os."
#endif

static int seed_drng_from_os(uint8_t* dst, size_t len)
{
#if (defined(HAVE_SYS_RANDOM_H) && defined(HAVE_GETRANDOM)) ||                                    \
    (defined(__linux__) && GLIBC_CHECK(2, 25))
    while (len != 0)
    {
        const ssize_t ret = getrandom(dst, len, 0);
        if (ret <= 0)
            return -1;
        dst += ret;
        len -= (size_t)ret;
    }
    return 0;
#elif defined(HAVE_ARC4RANDOM_BUF)
    arc4random_buf(dst, len);
    return 0;
#elif defined(__APPLE__) && defined(HAVE_APPLE_FRAMEWORK)
    return SecRandomCopyBytes(kSecRandomDefault, len, dst) == errSecSuccess ? 0 : -1;
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NETBSD__) ||   \
    defined(__NetBSD__)
#if !defined(O_NOFOLLOW)
#define O_NOFOLLOW 0
#endif
#if !defined(O_CLOEXEC)
#define O_CLOEXEC 0
#endif

    int fd;
    while ((fd = open("/dev/urandom", O_RDONLY | O_NOFOLLOW | O_CLOEXEC, 0)) == -1)
    {
        if (errno != EINTR)
            return -1;
    }
#if O_CLOEXEC == 0
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
#endif

    while (len != 0)
    {
        const ssize_t ret = read(fd, dst, len);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            close(fd);
            return -1;
        }
        if (ret == 0)
        {
            close(fd);
            return -1;
        }

        dst += ret;
        len -= (size_t)ret;
    }

    close(fd);
    return 0;
#elif defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
    if (len > ULONG_MAX)
        return -1;
    return BCRYPT_SUCCESS(BCryptGenRandom(NULL, dst, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG))
               ? 0
               : -1;
#endif
}

int rand_bytes(uint8_t* dst, size_t len)
{
    static _Thread_local DRNG_ctx drng;
    static _Thread_local int drng_initialized = 0;

    if (!dst && len != 0)
        return -1;

    if (!drng_initialized)
    {
        uint8_t seed[DRNG_SEED_BYTES];
        if (seed_drng_from_os(seed, sizeof(seed)) != 0)
            return -1;
        if (init_random_number(&drng, seed, sizeof(seed)) != 0)
            return -1;
        drng_initialized = 1;
    }

    if (len > (UINT64_MAX / 8))
        return -1;
    return get_random_number(&drng, dst, (unsigned long long)len * 8);
}
