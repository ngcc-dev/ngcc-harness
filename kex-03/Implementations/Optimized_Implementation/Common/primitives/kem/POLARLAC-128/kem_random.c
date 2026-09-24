/*
Copyright (c) 2026 Ying Liu.
File Description: KEM randomness wrapper. The BIT_USE_SHAKE path adapts
                  Kyber's ref/randombytes.c implementation.
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "params.h"
#include "kem_random.h"

#if BIT_USE_SHAKE

#include <stdlib.h>
#include "fips202.h"
#include "fips202x4.h"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <errno.h>
#ifdef __linux__
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#elif __NetBSD__
#include <sys/random.h>
#else
#include <unistd.h>
#endif
#endif

#ifdef _WIN32
static void randombytes_fips(uint8_t *out, size_t outlen)
{
    HCRYPTPROV ctx;
    size_t len;

    if (!CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        abort();
    }

    while (outlen > 0) {
        len = (outlen > 1048576) ? 1048576 : outlen;
        if (!CryptGenRandom(ctx, len, (BYTE *)out)) {
            abort();
        }

        out += len;
        outlen -= len;
    }

    if (!CryptReleaseContext(ctx, 0)) {
        abort();
    }
}
#elif defined(__linux__) && defined(SYS_getrandom)
static void randombytes_fips(uint8_t *out, size_t outlen)
{
    ssize_t ret;

    while (outlen > 0) {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            abort();
        }

        out += ret;
        outlen -= (size_t)ret;
    }
}
#elif defined(__NetBSD__)
static void randombytes_fips(uint8_t *out, size_t outlen)
{
    ssize_t ret;

    while (outlen > 0) {
        ret = getrandom(out, outlen, 0);
        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            abort();
        }

        out += ret;
        outlen -= (size_t)ret;
    }
}
#else
static void randombytes_fips(uint8_t *out, size_t outlen)
{
    static int fd = -1;
    ssize_t ret;

    while (fd == -1) {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR) {
            continue;
        } else if (fd == -1) {
            abort();
        }
    }

    while (outlen > 0) {
        ret = read(fd, out, outlen);
        if (ret == -1 && errno == EINTR) {
            continue;
        } else if (ret == -1) {
            abort();
        }

        out += ret;
        outlen -= (size_t)ret;
    }
}
#endif

#define FIPS_RNG_SEED_BYTES 32
#define FIPS_RNG_INPUT_BYTES (FIPS_RNG_SEED_BYTES + 8 + 1)
#define FIPS_RNG_LANES 4
#define FIPS_RNG_BUFFER_BYTES (FIPS_RNG_LANES * SHAKE256_RATE)

static uint8_t fips_rng_seed[FIPS_RNG_SEED_BYTES];
static uint8_t fips_rng_buffer[FIPS_RNG_LANES][SHAKE256_RATE];
static uint64_t fips_rng_counter = 0;
static size_t fips_rng_pos = FIPS_RNG_BUFFER_BYTES;
static int fips_rng_ready = 0;

static void store64_le(uint8_t out[8], uint64_t x)
{
    for (size_t i = 0; i < 8; i++) {
        out[i] = (uint8_t)(x >> (8 * i));
    }
}

static void fips_rng_init(void)
{
    randombytes_fips(fips_rng_seed, sizeof(fips_rng_seed));
    fips_rng_counter = 0;
    fips_rng_pos = FIPS_RNG_BUFFER_BYTES;
    fips_rng_ready = 1;
}

static void fips_rng_refill_x4(void)
{
    uint8_t in[FIPS_RNG_LANES][FIPS_RNG_INPUT_BYTES];

    for (size_t lane = 0; lane < FIPS_RNG_LANES; lane++) {
        memcpy(in[lane], fips_rng_seed, FIPS_RNG_SEED_BYTES);
        store64_le(in[lane] + FIPS_RNG_SEED_BYTES, fips_rng_counter + lane);
        in[lane][FIPS_RNG_SEED_BYTES + 8] = (uint8_t)lane;
    }

    shake256x4(fips_rng_buffer[0], fips_rng_buffer[1],
               fips_rng_buffer[2], fips_rng_buffer[3],
               SHAKE256_RATE,
               in[0], in[1], in[2], in[3],
               FIPS_RNG_INPUT_BYTES);
    fips_rng_counter += FIPS_RNG_LANES;
    fips_rng_pos = 0;
}

int get_random_number_fips(unsigned char *random_number,
                           unsigned long long random_number_len_bits)
{
    size_t outlen;

    if (random_number == NULL || (random_number_len_bits & 7ULL) != 0) {
        return -1;
    }

    if (!fips_rng_ready) {
        fips_rng_init();
    }

    outlen = (size_t)(random_number_len_bits >> 3);
    while (outlen > 0) {
        size_t lane;
        size_t off;
        size_t take;

        if (fips_rng_pos == FIPS_RNG_BUFFER_BYTES) {
            fips_rng_refill_x4();
        }

        lane = fips_rng_pos / SHAKE256_RATE;
        off = fips_rng_pos - lane * SHAKE256_RATE;
        take = SHAKE256_RATE - off;
        if (take > outlen) {
            take = outlen;
        }

        memcpy(random_number, fips_rng_buffer[lane] + off, take);
        random_number += take;
        outlen -= take;
        fips_rng_pos += take;
    }

    return 0;
}

#endif

int kem_get_random_number(DRNG_ctx *drng,
                          unsigned char *random_number,
                          unsigned long long random_number_len_bits)
{
#if BIT_USE_SHAKE
    (void)drng;
    return get_random_number_fips(random_number, random_number_len_bits);
#else
    return get_random_number(drng, random_number, random_number_len_bits);
#endif
}
