/* Write one public key from the frozen UVW reference key generation. */
#include <stdio.h>
#include <stdlib.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    unsigned char seed[48];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f || fread(seed, 1, sizeof seed, f) != sizeof seed) return 2;
    fclose(f);
    if (argc > 1) seed[0] ^= (unsigned char)atoi(argv[1]);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    unsigned long long pl = kem_get_pk_len_bytes(), sl = kem_get_sk_len_bytes();
    unsigned char *pk = malloc(pl), *sk = malloc(sl);
    if (!pk || !sk || kem_keygen(pk, &pl, sk, &sl) != 0) return 2;
    fwrite(pk, 1, kem_get_pk_len_bytes(), stdout);
    return 0;
}
