#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "api.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

void randombytes(uint8_t *out, size_t outlen)
{
    if (outlen) get_random_number(&drng_algorithm, out, outlen * 8);
}

static void fill(uint8_t *out, size_t len, uint32_t state)
{
    for (size_t i = 0; i < len; i++) {
        state = state * 1103515245u + 12345u;
        out[i] = (uint8_t)(state >> 16);
    }
}

int main(void)
{
    uint8_t seed[48], pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    uint8_t sig[CRYPTO_BYTES];
    uint8_t *a = malloc(20000), *b = malloc(20000), *c = malloc(20000);
    size_t siglen = 0;
    const size_t boundary = 8192 - CRYPTO_PUBLICKEYBYTES;
    const size_t longlen = boundary + 1;
    if (!a || !b || !c || boundary >= 20000) return 2;
    for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (uint8_t)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, sizeof(seed));
    fill(a, 20000, 1); fill(b, 20000, 2); fill(c, 20000, 3);
    if (crypto_sign_keypair(pk, sk) != 0) return 2;

    if (crypto_sign_signature(sig, &siglen, a, longlen, sk) != 0) return 2;
    if (crypto_sign_verify(sig, siglen, a, longlen, pk) != 0) return 1;
    if (crypto_sign_verify(sig, siglen, b, longlen, pk) != 0) return 1;
    if (crypto_sign_verify(sig, siglen, c, 20000, pk) != 0) return 1;
    if (crypto_sign_verify(sig, siglen, b, boundary, pk) == 0) return 1;

    if (crypto_sign_signature(sig, &siglen, a, boundary, sk) != 0) return 2;
    if (crypto_sign_verify(sig, siglen, a, boundary, pk) != 0) return 1;
    if (crypto_sign_verify(sig, siglen, b, boundary, pk) == 0) return 1;
    if (crypto_sign_verify(sig, siglen, b, longlen, pk) == 0) return 1;
    printf("CONFIRMED sign-22-6: pk=%d threshold=%zu long-message transfer accepted, boundary controls rejected\n",
           CRYPTO_PUBLICKEYBYTES, boundary);
    free(a); free(b); free(c);
    return 0;
}
