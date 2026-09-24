/*
 * test_forgery.c - Public-key forgery regression gate (Task 6.1).
 *
 * The ORIGINAL QingLuan flaw (see memory: qingluan-universal-forgery): the
 * MPC-in-the-Head protocol never proved knowledge of the secret witness, so a
 * valid signature could be produced from the public key alone. This test
 * reconstructs the strongest form of that attack against the CROSS-RSDP
 * redesign and asserts it now FAILS.
 *
 * The forger `forge_from_pk` runs the ENTIRE honest signing flow of
 * crypto_sign_signature (same SeedLeaves, commitments, Fiat-Shamir digests,
 * challenge derivation, response, serialization) but it only has the public key
 * pk = (Seed_pk | pack(s)); it substitutes a FABRICATED exponent witness
 * eta_forge for the secret eta it does not know.
 *
 * Why this must be rejected: for the t-w rounds with chall2[i]=0 the verifier
 * recomputes the commitment input as
 *     s'_v = (g^v * y) H^T - chall1*s
 *          = s'_signer + chall1*(e_forge H^T - s),   e_forge = g^{eta_forge}.
 * This equals the committed s'_signer (so cmt0, hence digest_cmt, matches) iff
 * chall1*(e_forge H^T - s) = 0. As chall1 in F_p^* is nonzero, that needs
 * e_forge H^T = s, i.e. eta_forge to be a genuine R-SDP witness. A fabricated
 * witness is not, and chall2 has fixed weight w < t, so at least t-w >= 1 such
 * round always exists -> the forgery is deterministically rejected. The binding
 * the original protocol lacked is now enforced.
 *
 * A real-sk signature still verifies (completeness sanity) so the rejection is
 * meaningful and not a degenerate "rejects everything".
 */

#include "api.h"
#include "params.h"
#include "sign.h"
#include "rsdp.h"
#include "restr.h"
#include "mpc.h"
#include "hash.h"
#include "utils.h"
#include "fq_arith.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Does g^{eta} H^T equal the syndrome carried in pk? If so, eta IS a valid
 * secret witness and using it would be key recovery, not a protocol forgery.
 * Every forgery attempt below asserts this is false, documenting that each
 * attempt is a genuine "forge without the witness" attack.
 */
static int eta_syndrome_matches(const unsigned char *pk, const uint8_t *eta)
{
    fq_t *V = (fq_t *)malloc((size_t)PARAM_R * PARAM_K * sizeof(fq_t));
    fq_t  e[PARAM_N], s_forge[PARAM_R], s_real[PARAM_R];
    assert(V);
    rsdp_expand_matrix(V, pk);
    rsdp_unpack_fp(s_real, pk + PARAM_KEYSEED_BYTES, PARAM_R);
    restr_vec_from_exp(e, eta, PARAM_N);
    rsdp_compute_syndrome(s_forge, V, e);
    int match = (memcmp(s_forge, s_real, (size_t)PARAM_R * sizeof(fq_t)) == 0);
    free(V);
    return match;
}

/*
 * Produce a best-effort forgery from pk alone, using the fabricated witness
 * eta_forge. Salt and master seed are filled deterministically (salt_fill /
 * seed_fill) so the whole test is reproducible. Mirrors crypto_sign_signature
 * exactly except: (1) inputs are pk + eta_forge, not sk; (2) V and pk_hash come
 * from pk. The syndrome s is public too but the commitment never needs it (only
 * the verifier does), so the forger does not load it.
 */
static void forge_from_pk(unsigned char *sig_out,
                          const unsigned char *m, size_t mlen,
                          const unsigned char *pk, const uint8_t *eta_forge,
                          uint8_t salt_fill, uint8_t seed_fill)
{
    fq_t    *V          = (fq_t *)malloc((size_t)PARAM_R * PARAM_K * sizeof(fq_t));
    uint8_t *seed_leaves = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_SEED_BYTES);
    uint8_t *cmt0_all   = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    uint8_t *cmt1_all   = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_HASH_BYTES);
    uint8_t *v_exp_all  = (uint8_t *)malloc((size_t)PARAM_TAU * PARAM_N);
    fq_t    *y_all      = (fq_t *)malloc((size_t)PARAM_TAU * PARAM_N * sizeof(fq_t));
    assert(V && seed_leaves && cmt0_all && cmt1_all && v_exp_all && y_all);

    rsdp_expand_matrix(V, pk);                       /* real public matrix H=[V|I] */

    uint8_t pk_hash[PARAM_HASH_BYTES];
    hash_digest(pk_hash, pk, QINGLUAN_PK_BYTES);     /* real pk binding */

    uint8_t salt[PARAM_SALT_BYTES];
    uint8_t master_seed[PARAM_SEED_BYTES];
    memset(salt, salt_fill, sizeof(salt));
    memset(master_seed, seed_fill, sizeof(master_seed));

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

    {   /* Phase A: commitments using the FABRICATED witness eta_forge */
        uint8_t eta_p[PARAM_N];
        fq_t u_p[PARAM_N];
        for (int i = 0; i < PARAM_TAU; i++) {
            uint16_t dom = (uint16_t)(i + PARAM_C);
            const uint8_t *seed_i = seed_leaves + (size_t)i * PARAM_SEED_BYTES;
            mpc_expand_round(eta_p, u_p, seed_i, salt, dom);
            mpc_commit0(cmt0_all + (size_t)i * PARAM_HASH_BYTES,
                        v_exp_all + (size_t)i * PARAM_N,
                        eta_forge, eta_p, u_p, V, salt, dom);
            mpc_commit1(cmt1_all + (size_t)i * PARAM_HASH_BYTES, seed_i, salt, dom);
        }
    }

    uint8_t digest_cmt[PARAM_HASH_BYTES];
    uint8_t digest_msg[PARAM_HASH_BYTES];
    uint8_t digest_chall1[PARAM_HASH_BYTES];
    uint8_t digest_chall2[PARAM_HASH_BYTES];
    fq_t   chall1[PARAM_TAU];
    uint8_t chall2[PARAM_TAU];

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
    }

    mpc_digest_chall2(digest_chall2, y_all, digest_chall1);
    mpc_gen_chall2(chall2, digest_chall2);

    /* Serialize exactly as crypto_sign_signature. */
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

    free(V); free(seed_leaves); free(cmt0_all); free(cmt1_all);
    free(v_exp_all); free(y_all);
}

int main(void)
{
    uint8_t seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(i + 1);
    drbg_seed(seed, sizeof(seed));            /* deterministic, reproducible run */

    unsigned char pk[QINGLUAN_PK_BYTES], sk[QINGLUAN_SK_BYTES];
    assert(crypto_sign_keypair(pk, sk) == 0);

    const char *msg = "forge me without the secret key";
    size_t mlen = strlen(msg);

    /* Completeness sanity: a real-sk signature MUST verify. */
    unsigned char good[QINGLUAN_SIG_BYTES];
    size_t glen = 0;
    assert(crypto_sign_signature(good, &glen, (const unsigned char *)msg, mlen, sk) == 0);
    assert(crypto_sign_verify(good, glen, (const unsigned char *)msg, mlen, pk) == 0);

    unsigned char forged[QINGLUAN_SIG_BYTES];
    uint8_t eta_forge[PARAM_N];
    int attempts = 0, rejected = 0;

    /* Family 1: structured fabricated witnesses, with salt/seed variations. */
    const uint8_t fills[] = { 0, 1, (uint8_t)(PARAM_Z - 1) };
    for (size_t f = 0; f < sizeof(fills) / sizeof(fills[0]); f++) {
        for (int i = 0; i < PARAM_N; i++) eta_forge[i] = fills[f];
        assert(!eta_syndrome_matches(pk, eta_forge));   /* genuine forgery, not key recovery */
        for (int v = 0; v < 3; v++) {
            forge_from_pk(forged, (const unsigned char *)msg, mlen, pk,
                          eta_forge, (uint8_t)(0x10 + v), (uint8_t)(0x20 + f));
            assert(crypto_sign_verify(forged, QINGLUAN_SIG_BYTES,
                                      (const unsigned char *)msg, mlen, pk) != 0);
            attempts++; rejected++;
        }
    }

    /* Family 2: pseudo-random fabricated witnesses (deterministic -> not flaky). */
    for (int k = 0; k < 16; k++) {
        for (int i = 0; i < PARAM_N; i++)
            eta_forge[i] = (uint8_t)((7 * k + 13 * i + 3) % PARAM_Z);
        assert(!eta_syndrome_matches(pk, eta_forge));
        forge_from_pk(forged, (const unsigned char *)msg, mlen, pk,
                      eta_forge, (uint8_t)(0x40 + k), (uint8_t)(0x80 + k));
        assert(crypto_sign_verify(forged, QINGLUAN_SIG_BYTES,
                                  (const unsigned char *)msg, mlen, pk) != 0);
        attempts++; rejected++;
    }

    /* Family 3: a genuine witness for a DIFFERENT key (attacker's own secret)
     * used against the victim pk must also fail. */
    {
        unsigned char pk2[QINGLUAN_PK_BYTES], sk2[QINGLUAN_SK_BYTES];
        uint8_t seed_e2[PARAM_KEYSEED_BYTES], seed_pk2[PARAM_KEYSEED_BYTES];
        assert(crypto_sign_keypair(pk2, sk2) == 0);
        rsdp_keypair_seeds(seed_e2, seed_pk2, sk2);
        rsdp_gen_secret_exp(eta_forge, seed_e2);        /* valid for pk2, not pk */
        assert(!eta_syndrome_matches(pk, eta_forge));
        forge_from_pk(forged, (const unsigned char *)msg, mlen, pk,
                      eta_forge, 0x55, 0xAA);
        assert(crypto_sign_verify(forged, QINGLUAN_SIG_BYTES,
                                  (const unsigned char *)msg, mlen, pk) != 0);
        attempts++; rejected++;
    }

    printf("test_forgery OK: honest sig accepted; %d/%d public-key forgeries REJECTED\n",
           rejected, attempts);
    return 0;
}
