#include "randombytes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <string.h>

#ifdef _WIN32
#include <wincrypt.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#ifdef __linux__
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif
#endif

#ifdef _WIN32
int randombytes(uint8_t *out, size_t outlen) {
    HCRYPTPROV ctx;
    size_t len;

    if (!CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
        abort();

    while (outlen > 0) {
        len = (outlen > 1048576) ? 1048576 : outlen;
        if (!CryptGenRandom(ctx, len, (BYTE *)out))
            abort();

        out += len;
        outlen -= len;
    }

    if (!CryptReleaseContext(ctx, 0))
        abort();
    return 0;
}
// #elif defined(__linux__) && defined(SYS_getrandom)
#else
AES256_CTR_DRBG_struct DRBG_ctx;

void AES256_ECB(unsigned char *key, unsigned char *ctr, unsigned char *buffer);

void handleErrors(void) {
    ERR_print_errors_fp(stderr);
    abort();
}

// Use whatever AES implementation you have. This uses AES from openSSL library
//    key - 256-bit AES key
//    ctr - a 128-bit plaintext value
//    buffer - a 128-bit ciphertext value
void AES256_ECB(unsigned char *key, unsigned char *ctr, unsigned char *buffer) {
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), NULL, key, NULL))
        handleErrors();

    if (1 != EVP_EncryptUpdate(ctx, buffer, &len, ctr, 16))
        handleErrors();
    ciphertext_len = len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
}

void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string,
                      int security_strength) {
    unsigned char seed_material[48];

    memcpy(seed_material, entropy_input, 48);
    if (personalization_string)
        for (int i = 0; i < 48; i++)
            seed_material[i] ^= personalization_string[i];
    memset(DRBG_ctx.Key, 0x00, 32);
    memset(DRBG_ctx.V, 0x00, 16);
    AES256_CTR_DRBG_Update(seed_material, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter = 1;
    DRBG_ctx.init = 1;
}

int randombytes(uint8_t *x, size_t xlen) {
    unsigned char block[16];
    int i = 0;

    if (!DRBG_ctx.init) {
        ssize_t ret;

        unsigned char seed[48] = {0};
        ret = syscall(SYS_getrandom, seed, 48, 0);
        if (ret != 48) {
            return RNG_FAIL_SYSCALL;
        }
        randombytes_init(seed, NULL, 256);
    }

    while (xlen > 0) {
        // increment V
        for (int j = 15; j >= 0; j--) {
            if (DRBG_ctx.V[j] == 0xff)
                DRBG_ctx.V[j] = 0x00;
            else {
                DRBG_ctx.V[j]++;
                break;
            }
        }
        AES256_ECB(DRBG_ctx.Key, DRBG_ctx.V, block);
        if (xlen > 15) {
            memcpy(x + i, block, 16);
            i += 16;
            xlen -= 16;
        } else {
            memcpy(x + i, block, xlen);
            xlen = 0;
        }
    }
    AES256_CTR_DRBG_Update(NULL, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter++;

    return RNG_SUCCESS;
}

void AES256_CTR_DRBG_Update(unsigned char *provided_data, unsigned char *Key,
                            unsigned char *V) {
    unsigned char temp[48];

    for (int i = 0; i < 3; i++) {
        // increment V
        for (int j = 15; j >= 0; j--) {
            if (V[j] == 0xff)
                V[j] = 0x00;
            else {
                V[j]++;
                break;
            }
        }

        AES256_ECB(Key, V, temp + 16 * i);
    }
    if (provided_data != NULL)
        for (int i = 0; i < 48; i++)
            temp[i] ^= provided_data[i];
    memcpy(Key, temp, 32);
    memcpy(V, temp + 32, 16);
}
#endif

/*
void randombytes(uint8_t *out, size_t outlen) {
    ssize_t ret;

    while (outlen > 0) {
        ret = syscall(SYS_getrandom, out, outlen, 0);
        if (ret == -1 && errno == EINTR)
            continue;
        else if (ret == -1)
            abort();

        out += ret;
        outlen -= ret;
    }
}
#else
void randombytes(uint8_t *out, size_t outlen) {
    static int fd = -1;
    ssize_t ret;

    while (fd == -1) {
        fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1 && errno == EINTR)
            continue;
        else if (fd == -1)
            abort();
    }

    while (outlen > 0) {
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
*/