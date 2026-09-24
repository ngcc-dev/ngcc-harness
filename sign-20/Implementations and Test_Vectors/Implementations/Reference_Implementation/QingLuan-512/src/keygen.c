/*
 * QingLuan Digital Signature Scheme
 * keygen.c - Key generation (CROSS-RSDP, Algorithm 1)
 *
 *   Seed_sk        <-$ {0,1}^{2lambda}
 *   (Seed_e,Seed_pk) <- CSPRNG(Seed_sk)
 *   V <- CSPRNG(Seed_pk);  H = [V | I]
 *   eta <- CSPRNG(Seed_e);  e = g^eta in E^n
 *   s = e H^T
 *   sk = Seed_sk ;  pk = (Seed_pk | pack(s))
 */

#include "api.h"
#include "rsdp.h"
#include "restr.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

int crypto_sign_keypair(unsigned char *pk, unsigned char *sk)
{
    uint8_t seed_sk[PARAM_KEYSEED_BYTES];
    uint8_t seed_e[PARAM_KEYSEED_BYTES];
    uint8_t seed_pk[PARAM_KEYSEED_BYTES];
    uint8_t eta[PARAM_N];
    int ret = -1;

    fq_t *V = (fq_t *)malloc((size_t)PARAM_R * (size_t)PARAM_K * sizeof(fq_t));
    fq_t *e = (fq_t *)malloc((size_t)PARAM_N * sizeof(fq_t));
    fq_t *s = (fq_t *)malloc((size_t)PARAM_R * sizeof(fq_t));
    if (!V || !e || !s) goto done;

    if (randombytes(seed_sk, PARAM_KEYSEED_BYTES) != 0) goto done;

    rsdp_keypair_seeds(seed_e, seed_pk, seed_sk);
    rsdp_expand_matrix(V, seed_pk);
    rsdp_gen_secret_exp(eta, seed_e);
    restr_vec_from_exp(e, eta, PARAM_N);          /* e = g^eta in E^n */
    rsdp_compute_syndrome(s, V, e);               /* s = e H^T */

    /* pk = Seed_pk || pack(s) ; sk = Seed_sk */
    memcpy(pk, seed_pk, PARAM_KEYSEED_BYTES);
    rsdp_pack_fp(pk + PARAM_KEYSEED_BYTES, s, PARAM_R);
    memcpy(sk, seed_sk, PARAM_KEYSEED_BYTES);

    ret = 0;

done:
    secure_zero(seed_sk, sizeof(seed_sk));
    secure_zero(seed_e, sizeof(seed_e));
    secure_zero(eta, sizeof(eta));
    if (V) { secure_zero(V, (size_t)PARAM_R * (size_t)PARAM_K * sizeof(fq_t)); free(V); }
    if (e) { secure_zero(e, (size_t)PARAM_N * sizeof(fq_t)); free(e); }
    if (s) { free(s); }   /* s is public (goes into pk) */
    return ret;
}
