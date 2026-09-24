/*
 * System entropy source. Compiled with /GL- (no LTCG) and no AVX2.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>
#define RtlGenRandom SystemFunction036
BOOLEAN NTAPI RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);
#pragma comment(lib, "advapi32.lib")

void randombytes_sys_fill(uint8_t *out, size_t outlen) {
    if (!RtlGenRandom((PVOID)out, (ULONG)outlen))
        abort();
}

#elif defined(__linux__)

#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

void randombytes_sys_fill(uint8_t *out, size_t outlen) {
    ssize_t ret;
    while (outlen > 0) {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR) continue;
        else if (ret == -1) abort();
        out += ret;
        outlen -= (size_t)ret;
    }
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void randombytes_sys_fill(uint8_t *out, size_t outlen) {
    static int fd = -1;
    ssize_t ret;
    while (fd == -1) {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR) continue;
        else if (fd == -1) abort();
    }
    while (outlen > 0) {
        ret = read(fd, out, outlen);
        if (ret == -1 && errno == EINTR) continue;
        else if (ret == -1) abort();
        out += ret;
        outlen -= (size_t)ret;
    }
}

#endif
