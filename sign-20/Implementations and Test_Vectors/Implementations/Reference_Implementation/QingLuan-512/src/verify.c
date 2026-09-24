/*
 * QingLuan Digital Signature Scheme
 * verify.c - Signature verification (CROSS-RSDP, Algorithm 3)
 *
 * Recomputes chall1/chall2 from the sent digests, reconstructs y[i] and cmt0[i]
 * for every round (chall2[i]=1: from the revealed seed + Proof commitment;
 * chall2[i]=0: from the response (y,v) with the restricted check on v), then
 * accepts iff the recomputed digest_cmt and digest_chall2 match the signature.
 * Constant-time digest comparison.
 */

#include "api.h"
#include "sign.h"
#include "rsdp.h"
#include "restr.h"
#include "mpc.h"
#include "hash.h"
#include "utils.h"
#include "fq_arith.h"
#include <string.h>
#include <stdlib.h>

int crypto_sign_verify(const unsigned char *sig_in, size_t siglen,
                       const unsigned char *m, size_t mlen,
                       const unsigned char *pk)
{
    int ret = -1;

    /* All locals that the cleanup path touches are declared up front. */
    uint8_t eta_p[PARAM_N];
    fq_t    u_p[PARAM_N];
    uint8_t v_exp[PARAM_N];
    uint8_t pk_hash[PARAM_HASH_BYTES];
    uint8_t digest_msg[PARAM_HASH_BYTES];
    uint8_t digest_chall1[PARAM_HASH_BYTES];
    uint8_t digest_cmt_p[PARAM_HASH_BYTES];
    uint8_t digest_chall2_p[PARAM_HASH_BYTES];
    fq_t    chall1[PARAM_TAU];
    uint8_t chall2[PARAM_TAU];

    fq_t    *V        = NULL;
    fq_t    *s        = NULL;
    fq_t    *y_all    = NULL;
    uint8_t *cmt0_all = NULL;
    uint8_t *cmt1_all = NULL;

    if (siglen != (size_t)QINGLUAN_SIG_BYTES) return -1;

    V        = (fq_t *)malloc((size_t)PARAM_R * PARAM_K * sizeof(fq_t));
    s        = (fq_t *)malloc((size_t)PARAM_R * sizeof(fq_t));
    y_all    = (fq_t *)malloc((size_t)PARAM_TAU * PARAM_N * sizeof(fq_t));
    cmt0_all = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    cmt1_all = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    if (!V || !s || !y_all || !cmt0_all || !cmt1_all) goto cleanup;

    /* Recover public matrix H=[V|I] and syndrome s. */
    rsdp_expand_matrix(V, pk);                                  /* pk starts with Seed_pk */
    if (rsdp_unpack_fp(s, pk + PARAM_KEYSEED_BYTES, PARAM_R) != 0) goto cleanup;

    const uint8_t *salt          = sig_in + SIG_OFF_SALT;
    const uint8_t *digest_cmt     = sig_in + SIG_OFF_DIGEST_CMT;
    const uint8_t *digest_chall2  = sig_in + SIG_OFF_DIGEST_CHALL2;

    /* Recompute the Fiat-Shamir challenges. */
    hash_digest(pk_hash, pk, QINGLUAN_PK_BYTES);
    mpc_digest_msg(digest_msg, salt, pk_hash, m, mlen);
    mpc_digest_chall1(digest_chall1, digest_msg, digest_cmt, salt);
    mpc_gen_chall1(chall1, digest_chall1);
    mpc_gen_chall2(chall2, digest_chall2);

    /* Walk rounds, consuming Path / Proof / resp in increasing round order. */
    {
        size_t op = SIG_OFF_PATH, opr = SIG_OFF_PROOF, ore = SIG_OFF_RESP;
        for (int i = 0; i < PARAM_TAU; i++) {
            uint16_t dom = (uint16_t)(i + PARAM_C);
            uint8_t *cmt0_i = cmt0_all + (size_t)i * PARAM_HASH_BYTES;
            uint8_t *cmt1_i = cmt1_all + (size_t)i * PARAM_HASH_BYTES;
            fq_t    *y_i    = y_all + (size_t)i * PARAM_N;

            if (chall2[i] == 1) {
                const uint8_t *seed_i = sig_in + op;
                op += PARAM_SEED_BYTES;
                mpc_commit1(cmt1_i, seed_i, salt, dom);
                memcpy(cmt0_i, sig_in + opr, PARAM_HASH_BYTES);   /* from Proof */
                opr += PARAM_HASH_BYTES;
                mpc_expand_round(eta_p, u_p, seed_i, salt, dom);
                mpc_compute_y(y_i, u_p, eta_p, chall1[i]);
            } else {
                if (rsdp_unpack_fp(y_i, sig_in + ore, PARAM_N) != 0) goto cleanup;
                ore += QINGLUAN_Y_BYTES;
                /* restricted-membership check on v lives in rsdp_unpack_fz */
                if (rsdp_unpack_fz(v_exp, sig_in + ore, PARAM_N) != 0) goto cleanup;
                ore += QINGLUAN_V_BYTES;
                memcpy(cmt1_i, sig_in + ore, PARAM_HASH_BYTES);
                ore += PARAM_HASH_BYTES;
                mpc_recompute_cmt0(cmt0_i, y_i, v_exp, chall1[i], V, s, salt, dom);
            }
        }
    }

    /* Accept iff both recomputed digests match the sent ones. */
    mpc_digest_cmt(digest_cmt_p, cmt0_all, cmt1_all);
    mpc_digest_chall2(digest_chall2_p, y_all, digest_chall1);

    if (ct_memcmp(digest_cmt_p, digest_cmt, PARAM_HASH_BYTES) == 0 &&
        ct_memcmp(digest_chall2_p, digest_chall2, PARAM_HASH_BYTES) == 0)
        ret = 0;

cleanup:
    secure_zero(eta_p, sizeof(eta_p));
    secure_zero(u_p, sizeof(u_p));
    secure_zero(v_exp, sizeof(v_exp));
    if (y_all)    free(y_all);
    if (cmt0_all) free(cmt0_all);
    if (cmt1_all) free(cmt1_all);
    if (s) free(s);
    if (V) free(V);
    return ret;
}

int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk)
{
    if (smlen < (unsigned long long)QINGLUAN_SIG_BYTES) return -1;

    size_t msg_len = (size_t)smlen - (size_t)QINGLUAN_SIG_BYTES;
    const unsigned char *msg_ptr = sm + QINGLUAN_SIG_BYTES;

    if (crypto_sign_verify(sm, (size_t)QINGLUAN_SIG_BYTES, msg_ptr, msg_len, pk) != 0)
        return -1;

    memmove(m, msg_ptr, msg_len);
    *mlen = (unsigned long long)msg_len;
    return 0;
}
