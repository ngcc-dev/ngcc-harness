/* kem-13-5: DKEM-512 derives K with pseudohash through the void dke_kem_kdf
 * (dkecca.c:31-38), so a failed allocation leaves the output unwritten while
 * kem_enc / kem_dec return 0. Linked with -Wl,--wrap=malloc; the wrapper fails
 * the (skip+1)-th allocation of one size.
 *   enc : malloc |cpa_ss|+2, the first buffer of K = KDF(cpa_ss): K is never
 *         written, so the caller's buffer (here zero-initialised) is returned.
 *   dec : malloc 2|ss|+2, skipping the hmac buffer of the re-encryption KDF;
 *         the rejection key KDF(z || tag) is never written, so for an invalid
 *         ciphertext implicit rejection returns the caller's buffer.
 * Controls: unfaulted runs return a nonzero K for both. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include "drng.h"
#include "KEM_AlgorithmInstance.h"
DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fs;
static int failed, skip;
void *__wrap_malloc(size_t n)
{
    if (fs && n == fs) {
        if (skip-- > 0) return __real_malloc(n);
        fs = 0; failed++; return NULL;
    }
    return __real_malloc(n);
}
static uint8_t pk[DKE_PKBYTES], sk[DKE_SKBYTES], ct[DKE_CTBYTES], ss[DKE_SSBYTES], ssd[DKE_SSBYTES], z[DKE_SSBYTES];
int main(void)
{
    uint8_t seed[48] = {3};
    unsigned long long a, b;
    int rc;
    init_random_number(&drng_algorithm, seed, 48);
    kem_keygen(pk, &a, sk, &b);
    for (int fault = 0; fault <= 1; fault++) {
        memset(ss, 0, sizeof ss); failed = 0; skip = 0; fs = fault ? DKE_SSBYTES + 2 : 0;
        rc = kem_enc(pk, sizeof pk, ss, &a, ct, &b);
        printf("enc%s: faults=%d kem_enc=%d ss_all_zero=%d\n", fault ? " KDF alloc fails" : " control", failed, rc, !memcmp(ss, z, sizeof z));
    }
    kem_enc(pk, sizeof pk, ss, &a, ct, &b);
    ct[5] ^= 1;                                   /* invalid ciphertext */
    for (int fault = 0; fault <= 1; fault++) {
        memset(ssd, 0, sizeof ssd); failed = 0; skip = 1; fs = fault ? 2 * DKE_SSBYTES + 2 : 0;
        rc = kem_dec(sk, sizeof sk, ct, sizeof ct, ssd, &a);
        printf("dec invalid ct%s: faults=%d kem_dec=%d ss_all_zero=%d\n", fault ? " rejection-KDF alloc fails" : " control", failed, rc, !memcmp(ssd, z, sizeof z));
    }
    return 0;
}
