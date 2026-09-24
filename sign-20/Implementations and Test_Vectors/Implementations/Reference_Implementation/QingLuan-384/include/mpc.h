/*
 * QingLuan Digital Signature Scheme
 * mpc.h - CROSS-ID per-round primitives + Fiat-Shamir challenge generation
 *
 * One round of CROSS-ID (R-SDP), see docs/protocol_reference.md:
 *   (eta', u') <- CSPRNG(Seed[i]|Salt|i+c)
 *   v = eta_sk - eta' (mod z);  v_E = g^v;  u = v_E * u';  s' = u H^T
 *   cmt0 = Hash(s' | v | Salt | i+c);  cmt1 = Hash(Seed[i] | Salt | i+c)
 *   y = u' + chall1 * g^{eta'}
 * Verifier (chall2=0) recomputes:  s' = (v_E * y) H^T - chall1*s;  cmt0 = Hash(...)
 */

#ifndef QINGLUAN_MPC_H
#define QINGLUAN_MPC_H

#include "params.h"
#include <stdint.h>

/* (eta' in F_z^n, u' in F_p^n) <- CSPRNG(Seed[i] | Salt | LE16(dom)). dom = i+c. */
void mpc_expand_round(uint8_t *eta_prime, fq_t *u_prime,
                      const uint8_t *seed_i, const uint8_t *salt, uint16_t dom);

/*
 * Signer commitment cmt0[i] and the exponent transform v_exp[i] (n bytes):
 *   v_exp = eta_sk - eta' (mod z);  s' = (g^v_exp * u') H^T;
 *   cmt0  = Hash(DOMAIN_CMT0 | pack_fp(s') | pack_fz(v_exp) | Salt | LE16(dom)).
 */
void mpc_commit0(uint8_t *cmt0, uint8_t *v_exp,
                 const uint8_t *eta_sk, const uint8_t *eta_prime,
                 const fq_t *u_prime, const fq_t *V,
                 const uint8_t *salt, uint16_t dom);

/* cmt1[i] = Hash(DOMAIN_CMT1 | Seed[i] | Salt | LE16(dom)). */
void mpc_commit1(uint8_t *cmt1, const uint8_t *seed_i,
                 const uint8_t *salt, uint16_t dom);

/* First response y = u' + chall1 * g^{eta'} (in F_p^n). */
void mpc_compute_y(fq_t *y, const fq_t *u_prime,
                   const uint8_t *eta_prime, fq_t chall1);

/*
 * Verifier (chall2=0) recomputes cmt0 from the response (y, v_exp) and pk:
 *   s' = (g^v_exp * y) H^T - chall1*s;  cmt0 = Hash(... same layout as signer).
 * The caller must have validated v_exp in F_z^n (via rsdp_unpack_fz).
 */
void mpc_recompute_cmt0(uint8_t *cmt0, const fq_t *y, const uint8_t *v_exp,
                        fq_t chall1, const fq_t *V, const fq_t *s,
                        const uint8_t *salt, uint16_t dom);

/* chall1: t elements of F_p^* from digest_chall1 (PARAM_HASH_BYTES). */
void mpc_gen_chall1(fq_t *chall1, const uint8_t *digest_chall1);

/* chall2: length-t binary string of weight exactly w from digest_chall2. */
void mpc_gen_chall2(uint8_t *chall2, const uint8_t *digest_chall2);

/* ---- Shared Fiat-Shamir digests (identical on signer and verifier) ---- */

/*
 * digest_Msg = Hash(DOMAIN_MSG | Salt | pk_hash | Msg).
 * Salt is the per-signature randomizer (design §4.2): it makes message binding
 * rely on (multi-target) eTCR/2nd-preimage instead of plain collision resistance
 * -- mandatory because H_w is multi-pipe SM3 (collision capped at ~2^128, Joux).
 */
void mpc_digest_msg(uint8_t *out, const uint8_t *salt, const uint8_t *pk_hash,
                    const uint8_t *msg, size_t mlen);

/*
 * digest_cmt = Hash(DOMAIN_DIGEST_CMT
 *                   | Hash(DOMAIN_DIGEST_CMT0 | cmt0[0..t-1])
 *                   | Hash(DOMAIN_DIGEST_CMT1 | cmt1[0..t-1]) ).
 * cmt0_all, cmt1_all are flat arrays of PARAM_TAU * PARAM_HASH_BYTES bytes.
 */
void mpc_digest_cmt(uint8_t *out, const uint8_t *cmt0_all, const uint8_t *cmt1_all);

/* digest_chall1 = Hash(DOMAIN_CHALL1 | digest_Msg | digest_cmt | Salt). */
void mpc_digest_chall1(uint8_t *out, const uint8_t *digest_msg,
                       const uint8_t *digest_cmt, const uint8_t *salt);

/*
 * digest_chall2 = Hash(DOMAIN_CHALL2 | pack(y[0]) | ... | pack(y[t-1]) | digest_chall1).
 * y_all is a flat array of PARAM_TAU * PARAM_N field elements.
 */
void mpc_digest_chall2(uint8_t *out, const fq_t *y_all, const uint8_t *digest_chall1);

#endif /* QINGLUAN_MPC_H */
