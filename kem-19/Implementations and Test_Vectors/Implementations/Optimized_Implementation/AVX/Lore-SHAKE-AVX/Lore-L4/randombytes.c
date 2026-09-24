#define _DEFAULT_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include "randombytes.h"

#ifdef LORE_USE_API_PKC_DRNG
#include "drng.h"

/*
 * In API_PKC KAT builds, this global DRNG context is provided by the
 * KAT driver translation unit.
 */
extern DRNG_ctx drng_algorithm;

void randombytes(unsigned char *out, size_t outlen)
{
    (void)get_random_number(&drng_algorithm, out, (unsigned long long)outlen * 8ULL);
}

#else

#ifdef SYS_getrandom
static void randombytes_sys_getrandom(unsigned char *out, size_t outlen)
{
    ssize_t ret;

    while (outlen > 0) {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            return;
        }

        out += ret;
        outlen -= (size_t)ret;
    }
}
#endif

void randombytes(unsigned char *out, size_t outlen)
{
#ifdef SYS_getrandom
    randombytes_sys_getrandom(out, outlen);
#else
    static int fd = -1;
    ssize_t ret;

    while (fd == -1) {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR) {
            continue;
        } else if (fd == -1) {
            return;
        }
    }

    while (outlen > 0) {
        ret = read(fd, out, outlen);
        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            return;
        }

        out += ret;
        outlen -= (size_t)ret;
    }
#endif
}

#endif /* LORE_USE_API_PKC_DRNG */

uint32_t randombytes_uniform(uint32_t upper_bound)
{
    uint32_t r, min;

    if (upper_bound < 2) {
        return 0;
    }

    min = (uint32_t)-upper_bound % upper_bound;

    for (;;) {
        randombytes((unsigned char *)&r, sizeof(r));
        if (r >= min) {
            return r % upper_bound;
        }
    }
}
