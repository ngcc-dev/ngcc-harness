/*
 * QingLuan Digital Signature Scheme
 * mpc.c - CROSS-ID per-round primitives + Fiat-Shamir challenge generation
 *
 * See docs/protocol_reference.md and mpc.h. All field vectors are bit-packed
 * (rsdp_pack_fp / rsdp_pack_fz) before hashing. Domain separation: a 1-byte
 * DOMAIN_* tag plus a 2-byte little-endian instance constant (i+c, t+c, ...).
 */

#include "mpc.h"
#include "rsdp.h"
#include "restr.h"
#include "fq_arith.h"
#include "hash.h"
#include "utils.h"
#include <string.h>

/* ---- little-endian u16 absorb helpers ---- */
static void xof_u16le(xof_ctx_t *x, uint16_t v)
{
    uint8_t b[2]; b[0] = (uint8_t)(v & 0xFF); b[1] = (uint8_t)(v >> 8);
    xof_absorb(x, b, 2);
}
static void hash_u16le(hash_ctx_t *h, uint16_t v)
{
    uint8_t b[2]; b[0] = (uint8_t)(v & 0xFF); b[1] = (uint8_t)(v >> 8);
    hash_update(h, b, 2);
}

/* cmt0 = Hash(DOMAIN_CMT0 | pack_fp(s') | pack_fz(v_exp) | Salt | LE16(dom)) */
static void hash_cmt0(uint8_t *cmt0, const fq_t *sprime, const uint8_t *v_exp,
                      const uint8_t *salt, uint16_t dom)
{
    uint8_t sp[QINGLUAN_SYNDROME_BYTES];
    uint8_t vp[QINGLUAN_V_BYTES];
    rsdp_pack_fp(sp, sprime, PARAM_R);
    rsdp_pack_fz(vp, v_exp, PARAM_N);

    hash_ctx_t h;
    hash_init(&h);
    uint8_t d = DOMAIN_CMT0;
    hash_update(&h, &d, 1);
    hash_update(&h, sp, QINGLUAN_SYNDROME_BYTES);
    hash_update(&h, vp, QINGLUAN_V_BYTES);
    hash_update(&h, salt, PARAM_SALT_BYTES);
    hash_u16le(&h, dom);
    hash_final(&h, cmt0);
}

void mpc_expand_round(uint8_t *eta_prime, fq_t *u_prime,
                      const uint8_t *seed_i, const uint8_t *salt, uint16_t dom)
{
    xof_ctx_t xof;
    xof_init(&xof);
    uint8_t d = DOMAIN_ROUND;
    xof_absorb(&xof, &d, 1);
    xof_absorb(&xof, seed_i, PARAM_SEED_BYTES);
    xof_absorb(&xof, salt, PARAM_SALT_BYTES);
    xof_u16le(&xof, dom);
    xof_finalize(&xof);

    for (int i = 0; i < PARAM_N; i++) eta_prime[i] = rsdp_csprng_fz(&xof);  /* F_z^n */
    for (int i = 0; i < PARAM_N; i++) u_prime[i]   = rsdp_csprng_fp(&xof);  /* F_p^n */

    secure_zero(&xof, sizeof(xof));
}

void mpc_commit0(uint8_t *cmt0, uint8_t *v_exp,
                 const uint8_t *eta_sk, const uint8_t *eta_prime,
                 const fq_t *u_prime, const fq_t *V,
                 const uint8_t *salt, uint16_t dom)
{
    fq_t u[PARAM_N];
    fq_t sprime[PARAM_R];

    for (int i = 0; i < PARAM_N; i++)
        v_exp[i] = restr_exp_sub(eta_sk[i], eta_prime[i]);   /* v = eta - eta' mod z */
    for (int i = 0; i < PARAM_N; i++)
        u[i] = fq_mul(restr_val(v_exp[i]), u_prime[i]);      /* u = g^v * u' */

    rsdp_apply_H(sprime, V, u);                              /* s' = u H^T */
    hash_cmt0(cmt0, sprime, v_exp, salt, dom);

    secure_zero(u, sizeof(u));
    secure_zero(sprime, sizeof(sprime));
}

void mpc_commit1(uint8_t *cmt1, const uint8_t *seed_i,
                 const uint8_t *salt, uint16_t dom)
{
    hash_ctx_t h;
    hash_init(&h);
    uint8_t d = DOMAIN_CMT1;
    hash_update(&h, &d, 1);
    hash_update(&h, seed_i, PARAM_SEED_BYTES);
    hash_update(&h, salt, PARAM_SALT_BYTES);
    hash_u16le(&h, dom);
    hash_final(&h, cmt1);
}

void mpc_compute_y(fq_t *y, const fq_t *u_prime,
                   const uint8_t *eta_prime, fq_t chall1)
{
    for (int i = 0; i < PARAM_N; i++) {
        fq_t e_prime = restr_val(eta_prime[i]);              /* g^{eta'} */
        y[i] = fq_add(u_prime[i], fq_mul(chall1, e_prime));  /* u' + chall1*e' */
    }
}

void mpc_recompute_cmt0(uint8_t *cmt0, const fq_t *y, const uint8_t *v_exp,
                        fq_t chall1, const fq_t *V, const fq_t *s,
                        const uint8_t *salt, uint16_t dom)
{
    fq_t yprime[PARAM_N];
    fq_t sprime[PARAM_R];

    for (int i = 0; i < PARAM_N; i++)
        yprime[i] = fq_mul(restr_val(v_exp[i]), y[i]);       /* y' = g^v * y */

    rsdp_apply_H(sprime, V, yprime);                         /* y' H^T */
    for (int i = 0; i < PARAM_R; i++)
        sprime[i] = fq_sub(sprime[i], fq_mul(chall1, s[i])); /* - chall1*s */

    hash_cmt0(cmt0, sprime, v_exp, salt, dom);

    secure_zero(yprime, sizeof(yprime));
    secure_zero(sprime, sizeof(sprime));
}

/* ---- Fiat-Shamir challenge generation ---- */

void mpc_gen_chall1(fq_t *chall1, const uint8_t *digest_chall1)
{
    xof_ctx_t xof;
    xof_init(&xof);
    uint8_t d = DOMAIN_CHALL1_GEN;
    xof_absorb(&xof, &d, 1);
    xof_absorb(&xof, digest_chall1, PARAM_HASH_BYTES);
    xof_u16le(&xof, (uint16_t)(PARAM_TAU + PARAM_C));
    xof_finalize(&xof);

    for (int i = 0; i < PARAM_TAU; i++)
        chall1[i] = rsdp_csprng_fp_star(&xof);

    secure_zero(&xof, sizeof(xof));
}

/* Uniform integer in [0, bound) by rejection sampling (bound >= 1). */
static uint32_t csprng_index(xof_ctx_t *xof, uint32_t bound)
{
    uint32_t m = 0, t = bound - 1;
    while (t) { m++; t >>= 1; }                  /* bit length of bound-1 */
    uint32_t mask = (m >= 32) ? 0xFFFFFFFFu : ((1u << m) - 1u);
    for (;;) {
        uint8_t b[2];
        xof_squeeze(xof, b, 2);
        uint32_t v = ((uint32_t)b[0] | ((uint32_t)b[1] << 8)) & mask;
        if (v < bound) return v;
    }
}

void mpc_gen_chall2(uint8_t *chall2, const uint8_t *digest_chall2)
{
    /* Start from 1^w 0^{t-w}, then Fisher-Yates shuffle -> uniform weight-w. */
    for (int i = 0; i < PARAM_TAU; i++)
        chall2[i] = (i < PARAM_W) ? 1 : 0;

    xof_ctx_t xof;
    xof_init(&xof);
    uint8_t d = DOMAIN_CHALL2_GEN;
    xof_absorb(&xof, &d, 1);
    xof_absorb(&xof, digest_chall2, PARAM_HASH_BYTES);
    xof_u16le(&xof, (uint16_t)(PARAM_TAU + PARAM_C + 1));
    xof_finalize(&xof);

    for (int i = PARAM_TAU - 1; i >= 1; i--) {
        uint32_t j = csprng_index(&xof, (uint32_t)(i + 1));  /* j in [0, i] */
        uint8_t tmp = chall2[i]; chall2[i] = chall2[j]; chall2[j] = tmp;
    }

    secure_zero(&xof, sizeof(xof));
}

/* ---- Shared Fiat-Shamir digests ---- */

void mpc_digest_msg(uint8_t *out, const uint8_t *salt, const uint8_t *pk_hash,
                    const uint8_t *msg, size_t mlen)
{
    /*
     * digest_Msg = Hash(DOMAIN_MSG | Salt | pk_hash | Msg).  The per-signature
     * Salt is the design's randomizer r (design-doc §4.2: mu = H_w(.. r .. pk .. M)):
     * it converts the message-binding hash from plain collision resistance to
     * (multi-target) eTCR / 2nd-preimage.  This is REQUIRED here because H_w is
     * multi-pipe SM3, whose collision resistance is capped at ~2^128 (Joux) and
     * does NOT grow with width; an unsalted message hash would cap EUF-CMA at
     * ~2^128 and break the 256/384/512 levels.  See docs/security-argument.md.
     * (Strengthens over the CROSS spec's unsalted Hash(Msg), which is safe there
     * only because CROSS uses SHAKE, a sponge with full collision resistance.)
     */
    hash_ctx_t h;
    hash_init(&h);
    uint8_t d = DOMAIN_MSG;
    hash_update(&h, &d, 1);
    hash_update(&h, salt, PARAM_SALT_BYTES);
    hash_update(&h, pk_hash, PARAM_HASH_BYTES);
    hash_update(&h, msg, mlen);
    hash_final(&h, out);
}

void mpc_digest_cmt(uint8_t *out, const uint8_t *cmt0_all, const uint8_t *cmt1_all)
{
    uint8_t d0[PARAM_HASH_BYTES], d1[PARAM_HASH_BYTES];
    hash_ctx_t h;
    uint8_t d;

    hash_init(&h); d = DOMAIN_DIGEST_CMT0; hash_update(&h, &d, 1);
    hash_update(&h, cmt0_all, (size_t)PARAM_TAU * PARAM_HASH_BYTES);
    hash_final(&h, d0);

    hash_init(&h); d = DOMAIN_DIGEST_CMT1; hash_update(&h, &d, 1);
    hash_update(&h, cmt1_all, (size_t)PARAM_TAU * PARAM_HASH_BYTES);
    hash_final(&h, d1);

    hash_init(&h); d = DOMAIN_DIGEST_CMT; hash_update(&h, &d, 1);
    hash_update(&h, d0, PARAM_HASH_BYTES);
    hash_update(&h, d1, PARAM_HASH_BYTES);
    hash_final(&h, out);
}

void mpc_digest_chall1(uint8_t *out, const uint8_t *digest_msg,
                       const uint8_t *digest_cmt, const uint8_t *salt)
{
    hash_ctx_t h;
    hash_init(&h);
    uint8_t d = DOMAIN_CHALL1;
    hash_update(&h, &d, 1);
    hash_update(&h, digest_msg, PARAM_HASH_BYTES);
    hash_update(&h, digest_cmt, PARAM_HASH_BYTES);
    hash_update(&h, salt, PARAM_SALT_BYTES);
    hash_final(&h, out);
}

void mpc_digest_chall2(uint8_t *out, const fq_t *y_all, const uint8_t *digest_chall1)
{
    hash_ctx_t h;
    hash_init(&h);
    uint8_t d = DOMAIN_CHALL2;
    hash_update(&h, &d, 1);
    uint8_t yb[QINGLUAN_Y_BYTES];
    for (int i = 0; i < PARAM_TAU; i++) {
        rsdp_pack_fp(yb, y_all + (size_t)i * PARAM_N, PARAM_N);
        hash_update(&h, yb, QINGLUAN_Y_BYTES);
    }
    hash_update(&h, digest_chall1, PARAM_HASH_BYTES);
    hash_final(&h, out);
}
