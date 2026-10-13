#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adkex_sig.h"
#include "dilithium/randombytes.h"

/* The adapter supplies the complete key-generation seed through this hook. */
void randombytes(uint8_t *out, size_t length) {
    if (adkex_sig_random_hook(out, (unsigned long long)length) != 0) {
        fputs("DKEX seed hook failed\n", stderr);
        abort();
    }
}

int main(void) {
    uint8_t seed[32], other_seed[32], transcript[64], changed[64];
    uint8_t pk[ADKEX_SIG_PKBITS/8], pk_again[ADKEX_SIG_PKBITS/8];
    uint8_t pk_other[ADKEX_SIG_PKBITS/8], sk[ADKEX_SIG_SKBITS/8];
    uint8_t sk_again[ADKEX_SIG_SKBITS/8], sk_other[ADKEX_SIG_SKBITS/8];
    uint8_t signature[ADKEX_SIG_SNBITS/8];
    for (unsigned i = 0; i < sizeof seed; i++) seed[i] = (uint8_t)i;
    memcpy(other_seed, seed, sizeof seed);
    other_seed[0] ^= 1;
    for (unsigned i = 0; i < sizeof transcript; i++) transcript[i] = (uint8_t)i;
    memcpy(changed, transcript, sizeof transcript);
    changed[0] ^= 1;

    adkex_sig_keygen(pk, sk, seed);
    adkex_sig_keygen(pk_again, sk_again, seed);
    adkex_sig_keygen(pk_other, sk_other, other_seed);
    assert(memcmp(pk, pk_again, sizeof pk) == 0);
    assert(memcmp(pk, pk_other, sizeof pk) != 0);
    adkex_sig_sign(signature, sk, transcript, 512);
    int accepted = adkex_sig_verify(pk, signature, transcript, 512);
    int rejected = adkex_sig_verify(pk, signature, changed, 512);
    if (accepted != 1 || rejected != 0) {
        fputs("DKEX signature controls failed\n", stderr);
        return 1;
    }
    puts("CONFIRMED kex-04-3: a 256-bit seed determines the DKEX-512 signing key and permits an accepted transcript signature");
    return 0;
}
