/*
 * QingLuan Digital Signature Scheme
 * sign.c - Signature generation (CROSS-RSDP, Algorithm 2) + serialization
 *
 * See docs/protocol_reference.md. Fast-style: round seeds are independent
 * leaves CSPRNG(Seed|Salt); Path/Proof are the w revealed seeds / cmt0 sent
 * directly. digest_Msg = Hash(DOMAIN_MSG | Salt | pk_hash | Msg): the Salt is the
 * design's per-signature randomizer r (eTCR), and pk_hash is BUFF binding -- both
 * strengthen EUF-CMA over the CROSS spec's bare Hash(Msg). Salting is REQUIRED
 * because H_w is multi-pipe SM3 (collision capped at ~2^128); see security-argument.md.
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

int crypto_sign_signature(unsigned char *sig_out, size_t *siglen,
                          const unsigned char *m, size_t mlen,
                          const unsigned char *sk)
{
    int ret = -1;

    uint8_t eta_sk[PARAM_N];
    uint8_t seed_e[PARAM_KEYSEED_BYTES];
    uint8_t seed_pk[PARAM_KEYSEED_BYTES];
    uint8_t master_seed[PARAM_SEED_BYTES] = {0};
    uint8_t salt[PARAM_SALT_BYTES] = {0};
    uint8_t pk_hash[PARAM_HASH_BYTES] = {0};
    uint8_t digest_cmt[PARAM_HASH_BYTES];
    uint8_t digest_msg[PARAM_HASH_BYTES];
    uint8_t digest_chall1[PARAM_HASH_BYTES];
    uint8_t digest_chall2[PARAM_HASH_BYTES];
    fq_t   chall1[PARAM_TAU];
    uint8_t chall2[PARAM_TAU];

    fq_t    *V = (fq_t *)malloc((size_t)PARAM_R * PARAM_K * sizeof(fq_t));
    fq_t    *e = (fq_t *)malloc((size_t)PARAM_N * sizeof(fq_t));
    fq_t    *s = (fq_t *)malloc((size_t)PARAM_R * sizeof(fq_t));
    uint8_t *seed_leaves = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_SEED_BYTES);
    uint8_t *cmt0_all    = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    uint8_t *cmt1_all    = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    uint8_t *v_exp_all   = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_N);
    fq_t    *y_all       = (fq_t *)malloc((size_t)PARAM_TAU * PARAM_N * sizeof(fq_t));
    if (!V || !e || !s || !seed_leaves || !cmt0_all || !cmt1_all || !v_exp_all || !y_all)
        goto cleanup;

    /* ExpandSK (keep seed_pk to bind pk into the message digest) */
    rsdp_keypair_seeds(seed_e, seed_pk, sk);
    rsdp_expand_matrix(V, seed_pk);
    rsdp_gen_secret_exp(eta_sk, seed_e);
    restr_vec_from_exp(e, eta_sk, PARAM_N);
    rsdp_compute_syndrome(s, V, e);

    {   /* pk_hash = Hash(Seed_pk | pack(s)) */
        uint8_t *pk_buf = (uint8_t *)malloc(QINGLUAN_PK_BYTES);
        if (!pk_buf) goto cleanup;
        memcpy(pk_buf, seed_pk, PARAM_KEYSEED_BYTES);
        rsdp_pack_fp(pk_buf + PARAM_KEYSEED_BYTES, s, PARAM_R);
        hash_digest(pk_hash, pk_buf, QINGLUAN_PK_BYTES);
        free(pk_buf);
    }

    if (randombytes(master_seed, PARAM_SEED_BYTES) != 0) goto cleanup;
    if (randombytes(salt, PARAM_SALT_BYTES) != 0) goto cleanup;

    {   /* SeedLeaves: t independent round seeds from (master_seed | Salt) */
        xof_ctx_t xof;
        xof_init(&xof);
        uint8_t d = DOMAIN_SEEDLEAVES;
        xof_absorb(&xof, &d, 1);
        xof_absorb(&xof, master_seed, PARAM_SEED_BYTES);
        xof_absorb(&xof, salt, PARAM_SALT_BYTES);
        xof_finalize(&xof);
        xof_squeeze(&xof, seed_leaves, (size_t)PARAM_TAU * PARAM_SEED_BYTES);
        secure_zero(&xof, sizeof(xof));
    }

    {   /* Phase A: commitments */
        uint8_t eta_p[PARAM_N];
        fq_t u_p[PARAM_N];
        for (int i = 0; i < PARAM_TAU; i++) {
            uint16_t dom = (uint16_t)(i + PARAM_C);
            const uint8_t *seed_i = seed_leaves + (size_t)i * PARAM_SEED_BYTES;
            mpc_expand_round(eta_p, u_p, seed_i, salt, dom);
            mpc_commit0(cmt0_all + (size_t)i * PARAM_HASH_BYTES,
                        v_exp_all + (size_t)i * PARAM_N,
                        eta_sk, eta_p, u_p, V, salt, dom);
            mpc_commit1(cmt1_all + (size_t)i * PARAM_HASH_BYTES, seed_i, salt, dom);
        }
        secure_zero(eta_p, sizeof(eta_p));
        secure_zero(u_p, sizeof(u_p));
    }

    mpc_digest_cmt(digest_cmt, cmt0_all, cmt1_all);
    mpc_digest_msg(digest_msg, salt, pk_hash, m, mlen);
    mpc_digest_chall1(digest_chall1, digest_msg, digest_cmt, salt);
    mpc_gen_chall1(chall1, digest_chall1);

    {   /* Phase B: first responses y[i] = u'[i] + chall1[i]*g^{eta'[i]} */
        uint8_t eta_p[PARAM_N];
        fq_t u_p[PARAM_N];
        for (int i = 0; i < PARAM_TAU; i++) {
            uint16_t dom = (uint16_t)(i + PARAM_C);
            const uint8_t *seed_i = seed_leaves + (size_t)i * PARAM_SEED_BYTES;
            mpc_expand_round(eta_p, u_p, seed_i, salt, dom);
            mpc_compute_y(y_all + (size_t)i * PARAM_N, u_p, eta_p, chall1[i]);
        }
        secure_zero(eta_p, sizeof(eta_p));
        secure_zero(u_p, sizeof(u_p));
    }

    mpc_digest_chall2(digest_chall2, y_all, digest_chall1);
    mpc_gen_chall2(chall2, digest_chall2);

    /* Serialize */
    memcpy(sig_out + SIG_OFF_SALT, salt, PARAM_SALT_BYTES);
    memcpy(sig_out + SIG_OFF_DIGEST_CMT, digest_cmt, PARAM_HASH_BYTES);
    memcpy(sig_out + SIG_OFF_DIGEST_CHALL2, digest_chall2, PARAM_HASH_BYTES);
    {
        size_t op = SIG_OFF_PATH, opr = SIG_OFF_PROOF, ore = SIG_OFF_RESP;
        for (int i = 0; i < PARAM_TAU; i++) {
            if (chall2[i] == 1) {
                memcpy(sig_out + op, seed_leaves + (size_t)i * PARAM_SEED_BYTES,
                       PARAM_SEED_BYTES);
                op += PARAM_SEED_BYTES;
                memcpy(sig_out + opr, cmt0_all + (size_t)i * PARAM_HASH_BYTES,
                       PARAM_HASH_BYTES);
                opr += PARAM_HASH_BYTES;
            } else {
                rsdp_pack_fp(sig_out + ore, y_all + (size_t)i * PARAM_N, PARAM_N);
                ore += QINGLUAN_Y_BYTES;
                rsdp_pack_fz(sig_out + ore, v_exp_all + (size_t)i * PARAM_N, PARAM_N);
                ore += QINGLUAN_V_BYTES;
                memcpy(sig_out + ore, cmt1_all + (size_t)i * PARAM_HASH_BYTES,
                       PARAM_HASH_BYTES);
                ore += PARAM_HASH_BYTES;
            }
        }
    }
    *siglen = QINGLUAN_SIG_BYTES;
    ret = 0;

cleanup:
    secure_zero(eta_sk, sizeof(eta_sk));
    secure_zero(seed_e, sizeof(seed_e));
    secure_zero(seed_pk, sizeof(seed_pk));
    secure_zero(master_seed, sizeof(master_seed));
    secure_zero(chall1, sizeof(chall1));
    if (seed_leaves) { secure_zero(seed_leaves, (size_t)PARAM_TAU * PARAM_SEED_BYTES); free(seed_leaves); }
    if (v_exp_all)   { secure_zero(v_exp_all, (size_t)PARAM_TAU * PARAM_N); free(v_exp_all); }
    if (y_all)       { secure_zero(y_all, (size_t)PARAM_TAU * PARAM_N * sizeof(fq_t)); free(y_all); }
    if (cmt0_all) free(cmt0_all);
    if (cmt1_all) free(cmt1_all);
    if (e) { secure_zero(e, (size_t)PARAM_N * sizeof(fq_t)); free(e); }
    if (s) free(s);
    if (V) free(V);
    return ret;
}

int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen,
                const unsigned char *sk)
{
    size_t siglen;
    int ret = crypto_sign_signature(sm, &siglen, m, (size_t)mlen, sk);
    if (ret != 0) return ret;

    memmove(sm + siglen, m, (size_t)mlen);
    *smlen = (unsigned long long)(siglen + (size_t)mlen);
    return 0;
}
