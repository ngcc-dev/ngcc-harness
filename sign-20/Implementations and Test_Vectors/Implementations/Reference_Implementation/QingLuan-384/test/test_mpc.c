/*
 * test_mpc.c - CROSS-ID round core.
 *
 * The central identity: given an honest response (y, v) and the public (V, s),
 * the verifier's recomputed cmt0 equals the signer's cmt0. This is what makes
 * honest signatures verify AND binds the response to the secret witness
 * (s' = (v*y)H^T - chall1*s = (v*u')H^T). Tampering chall1 or s breaks it.
 */
#include "mpc.h"
#include "rsdp.h"
#include "restr.h"
#include "fq_arith.h"
#include "params.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
    restr_init();

    /* Key */
    uint8_t seed_sk[PARAM_KEYSEED_BYTES];
    for (int i = 0; i < PARAM_KEYSEED_BYTES; i++) seed_sk[i] = (uint8_t)(i + 1);
    uint8_t eta_sk[PARAM_N];
    fq_t V[PARAM_R * PARAM_K];
    rsdp_expand_sk(eta_sk, V, seed_sk);
    fq_t e[PARAM_N];
    restr_vec_from_exp(e, eta_sk, PARAM_N);
    fq_t s[PARAM_R];
    rsdp_compute_syndrome(s, V, e);

    /* One round */
    uint8_t seed_i[PARAM_SEED_BYTES];
    for (int i = 0; i < PARAM_SEED_BYTES; i++) seed_i[i] = (uint8_t)(2 * i + 3);
    uint8_t salt[PARAM_SALT_BYTES];
    for (int i = 0; i < PARAM_SALT_BYTES; i++) salt[i] = (uint8_t)(5 * i);
    uint16_t dom = (uint16_t)(0 + PARAM_C);

    uint8_t eta_p[PARAM_N];
    fq_t u_p[PARAM_N];
    mpc_expand_round(eta_p, u_p, seed_i, salt, dom);
    for (int i = 0; i < PARAM_N; i++) assert(eta_p[i] < PARAM_Z);

    uint8_t cmt0[PARAM_HASH_BYTES], v_exp[PARAM_N];
    mpc_commit0(cmt0, v_exp, eta_sk, eta_p, u_p, V, salt, dom);
    for (int i = 0; i < PARAM_N; i++) assert(v_exp[i] < PARAM_Z); /* verifier unpack_fz ok */

    fq_t c1 = 5;
    fq_t y[PARAM_N];
    mpc_compute_y(y, u_p, eta_p, c1);

    /* CORE identity: honest (y, v) reproduce cmt0 */
    uint8_t cmt0v[PARAM_HASH_BYTES];
    mpc_recompute_cmt0(cmt0v, y, v_exp, c1, V, s, salt, dom);
    assert(memcmp(cmt0, cmt0v, PARAM_HASH_BYTES) == 0);

    /* Wrong chall1 -> mismatch */
    uint8_t bad[PARAM_HASH_BYTES];
    mpc_recompute_cmt0(bad, y, v_exp, (fq_t)6, V, s, salt, dom);
    assert(memcmp(cmt0, bad, PARAM_HASH_BYTES) != 0);

    /* Tampered syndrome -> mismatch (binds the public key) */
    fq_t s_bad[PARAM_R];
    memcpy(s_bad, s, sizeof(s));
    s_bad[0] = fq_add(s_bad[0], 1);
    uint8_t bad2[PARAM_HASH_BYTES];
    mpc_recompute_cmt0(bad2, y, v_exp, c1, V, s_bad, salt, dom);
    assert(memcmp(cmt0, bad2, PARAM_HASH_BYTES) != 0);

    /* chall1 in (F_p^*)^t */
    uint8_t dig[PARAM_HASH_BYTES];
    for (int i = 0; i < PARAM_HASH_BYTES; i++) dig[i] = (uint8_t)(3 * i + 1);
    fq_t ch1[PARAM_TAU];
    mpc_gen_chall1(ch1, dig);
    for (int i = 0; i < PARAM_TAU; i++) { assert(ch1[i] >= 1); assert(ch1[i] < PARAM_Q); }

    /* chall2 weight exactly w, binary, deterministic */
    uint8_t dig2[PARAM_HASH_BYTES];
    for (int i = 0; i < PARAM_HASH_BYTES; i++) dig2[i] = (uint8_t)(7 * i + 2);
    uint8_t ch2[PARAM_TAU], ch2b[PARAM_TAU];
    mpc_gen_chall2(ch2, dig2);
    int wt = 0;
    for (int i = 0; i < PARAM_TAU; i++) { assert(ch2[i] <= 1); wt += ch2[i]; }
    assert(wt == PARAM_W);
    mpc_gen_chall2(ch2b, dig2);
    assert(memcmp(ch2, ch2b, PARAM_TAU) == 0);

    printf("test_mpc OK\n");
    return 0;
}
