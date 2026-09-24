/**
 * @file random.c
 * @brief Runtime randombytes adapter used by correctness and benchmark builds.
 */

#include "random.h"
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <wincrypt.h>

#else
#if defined(__linux__)
#include <sys/random.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

/**
 * @brief Fill a caller-provided buffer with operating-system randomness.
 *
 * The function is intentionally small and dependency-free so every build entry
 * can link it. KAT generation does not use this path; it uses the
 * deterministic API_PKC DRNG to keep test vectors reproducible.
 */
int randombytes(unsigned char *buffer, unsigned int size) {
    if (size == 0) {
        return 0;
    }
    if (buffer == NULL) {
        return -1;  // invalid argument
    }

#if defined(_WIN32) || defined(_WIN64)
    /* Windows implementation compatible with MinGW/MSVC. */
    HCRYPTPROV hCryptProv = 0;

    // Get a cryptographic provider handle.
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return -2;  // failed to acquire context
    }

    // Generate random bytes.
    if (!CryptGenRandom(hCryptProv, size, buffer)) {
        CryptReleaseContext(hCryptProv, 0);
        return -3;  // random generation failed
    }

    // Release context.
    CryptReleaseContext(hCryptProv, 0);
    return 0;

#else
#if defined(__linux__)
    size_t offset = 0U;

    while (offset < (size_t)size) {
        const ssize_t result = getrandom(buffer + offset, (size_t)size - offset, 0);

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSYS) {
                break;
            }
            return -5;
        }
        if (result == 0) {
            return -5;
        }
        offset += (size_t)result;
    }
    if (offset == (size_t)size) {
        return 0;
    }
#endif

    /* UNIX-like fallback (macOS, older Linux, and other POSIX targets). */
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return -4;  // failed to open device
    }

    size_t bytes_read = 0U;
    while (bytes_read < (size_t)size) {
        ssize_t result = read(fd, buffer + bytes_read, (size_t)size - bytes_read);
        if (result < 0) {
            if (errno == EINTR) continue;  // retry if interrupted by signal
            close(fd);
            return -5;  // read failed
        }
        if (result == 0) {
            close(fd);
            return -5;  // unexpected EOF
        }
        bytes_read += result;
    }

    close(fd);
    return 0;
#endif
}
