/* kem-30-4 driver: reference POLARLAC-Light reads past the zero selector.
 *
 * usage: drv POLY Z   honest key pair; the first Z low bytes of polynomial
 * POLY of the packed b are set to zero, and the key is passed to kem_enc in a
 * heap buffer of exactly its declared length. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    int poly = atoi(argv[1]), z = atoi(argv[2]);
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char seed[48], *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl), *ss = malloc(ssl);
    if (!pk || !sk || !ct || !ss || poly < 0 || poly >= RL_KEM_K || z < 0 || z > RL_KEM_N) return 2;
    memset(seed, 5, sizeof seed);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    unsigned char *buf = malloc((size_t)pkl);           /* exactly the declared length */
    if (!buf) return 2;
    memcpy(buf, pk, (size_t)pkl);
    for (int i = 0; i < z; i++) buf[PK_SEED_LEN_BYTES + poly * PK_POLY_BYTES + i] = 0;
    printf("pk_len=%llu poly=%d zeros=%d\n", pkl, poly, z);
    fflush(stdout);
    int rc = kem_enc(buf, pkl, ss, &ssl, ct, &ctl);
    printf("enc=%d\n", rc);
    return 0;
}
