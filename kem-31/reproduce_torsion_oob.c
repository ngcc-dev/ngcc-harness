/* kem-31-6 driver: key generation writes degree[3] of a three-element array.
 *
 * Linked with -Wl,--wrap=id2iso_ideal_to_isogeny_odd. The wrapper fills the
 * caller's ec_isog_odd_t (the stack object of keygen) with 0xA5, calls the
 * real function and prints the table and array dimensions and the bytes that
 * follow the degree array inside the structure. main then runs one
 * deterministic key generation, encapsulation and decapsulation and prints
 * FNV-1a digests of the public key, secret key, ciphertext and shared secret.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include <id2iso.h>
#include <torsion_constants.h>

DRNG_ctx drng_algorithm;

void __real_id2iso_ideal_to_isogeny_odd(ec_isog_odd_t *isog, const ec_curve_t *domain,
                                        const ec_basis_t *basis_plus, const ec_basis_t *basis_minus,
                                        const quat_left_ideal_t *lideal_input);

void __wrap_id2iso_ideal_to_isogeny_odd(ec_isog_odd_t *isog, const ec_curve_t *domain,
                                        const ec_basis_t *basis_plus, const ec_basis_t *basis_minus,
                                        const quat_left_ideal_t *lideal_input) {
    const unsigned char *p = (const unsigned char *)isog;
    size_t end = offsetof(ec_isog_odd_t, degree) + sizeof(isog->degree), i;

    memset(isog, 0xA5, sizeof(*isog));
    __real_id2iso_ideal_to_isogeny_odd(isog, domain, basis_plus, basis_minus, lideal_input);
    printf("table=%zu degree=%zu isog=%zu degree_end=%zu after=",
           sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS),
           sizeof(isog->degree) / sizeof(*isog->degree), sizeof(*isog), end);
    for (i = end; i < sizeof(*isog); i++) printf("%02x", p[i]);
    printf("\n");
    fflush(stdout);
}

static unsigned long long fnv(const unsigned char *p, unsigned long long n) {
    uint64_t h = 0xcbf29ce484222325ULL;
    while (n--) { h ^= *p++; h *= 0x100000001b3ULL; }
    return h;
}

int main(void) {
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes(), ssl2 = ssl;
    unsigned char *pk = calloc(pkl, 1), *sk = calloc(skl, 1), *ct = calloc(ctl, 1);
    unsigned char *ss = calloc(ssl, 1), *ss2 = calloc(ssl, 1);
    unsigned char seed[64];
    int i, kg, enc, dec;

    if (!pk || !sk || !ct || !ss || !ss2) return 2;
    for (i = 0; i < 64; i++) seed[i] = (unsigned char)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    kg = kem_keygen(pk, &pkl, sk, &skl);
    enc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    dec = kem_dec(sk, skl, ct, ctl, ss2, &ssl2);
    printf("keygen=%d enc=%d dec=%d agree=%d pk=%016llx sk=%016llx ct=%016llx ss=%016llx\n",
           kg, enc, dec, ssl == ssl2 && memcmp(ss, ss2, ssl) == 0,
           fnv(pk, pkl), fnv(sk, skl), fnv(ct, ctl), fnv(ss, ssl));
    return 0;
}
