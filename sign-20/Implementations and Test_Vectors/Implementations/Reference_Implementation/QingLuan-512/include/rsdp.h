/*
 * QingLuan Digital Signature Scheme
 * rsdp.h - Restricted Syndrome Decoding Problem operations (CROSS-RSDP)
 *
 * Conventions (see docs/protocol_reference.md):
 *   - H = [V | I_{n-k}], V in F_p^{(n-k) x k} expanded from Seed_pk.
 *   - Syndrome / "apply H": for x in F_p^n, x H^T = V * x[0:k] + x[k:n].
 *   - Secret error e = g^eta, eta in F_z^n.
 *   - Bit-packing: F_p with PARAM_Q_BITS, F_z with PARAM_Z_BITS, little-endian.
 */

#ifndef QINGLUAN_RSDP_H
#define QINGLUAN_RSDP_H

#include "params.h"
#include "hash.h"
#include <stdint.h>

/* ---- CSPRNG element sampling (rejection) over an initialized XOF ---- */

/* One uniform element of F_p (= {0..p-1}). */
fq_t    rsdp_csprng_fp(xof_ctx_t *xof);
/* One uniform element of F_p^* (= {1..p-1}): sample {0..p-2}, return +1. */
fq_t    rsdp_csprng_fp_star(xof_ctx_t *xof);
/* One uniform exponent in F_z (= {0..z-1}). */
uint8_t rsdp_csprng_fz(xof_ctx_t *xof);

/* ---- Key / matrix expansion ---- */

/*
 * Derive (Seed_e, Seed_pk) from Seed_sk (ExpandSK step 1 / KeyGen step 2).
 * Each output is PARAM_KEYSEED_BYTES long.
 */
void rsdp_keypair_seeds(uint8_t *seed_e, uint8_t *seed_pk,
                        const uint8_t *seed_sk);

/* Expand V in F_p^{(n-k) x k} (row-major) from Seed_pk. H = [V | I]. */
void rsdp_expand_matrix(fq_t *V, const uint8_t *seed_pk);

/* Sample the secret exponent vector eta in F_z^n from Seed_e. */
void rsdp_gen_secret_exp(uint8_t *eta, const uint8_t *seed_e);

/*
 * ExpandSK(Seed_sk) -> (eta, V): regenerate the secret exponents and public
 * matrix exactly as KeyGen, minus the syndrome. (Algorithm 4.)
 */
void rsdp_expand_sk(uint8_t *eta, fq_t *V, const uint8_t *seed_sk);

/* ---- Linear algebra ---- */

/*
 * Apply the parity-check map: out = x H^T = V * x[0:k] + x[k:n].
 *   out : r = n-k field elements
 *   V   : r x k matrix (row-major)
 *   x   : n field elements
 */
void rsdp_apply_H(fq_t *out, const fq_t *V, const fq_t *x);

/* Syndrome s = e H^T (e in E^n given as field elements). Alias of apply_H. */
void rsdp_compute_syndrome(fq_t *s, const fq_t *V, const fq_t *e);

/* ---- Bit-packing ---- */

/* Pack/unpack n_elems F_p elements at PARAM_Q_BITS bits each (little-endian). */
void rsdp_pack_fp(uint8_t *out, const fq_t *v, size_t n_elems);
/*
 * rsdp_unpack_fp returns 0 on success, -1 if ANY decoded element is >= p or the
 * trailing pad bits are nonzero (canonical-encoding check, mirroring
 * rsdp_unpack_fz). The verifier rejects on -1 so every F_p vector has a unique
 * encoding -- otherwise the unauthenticated pad bits make signatures malleable.
 */
int  rsdp_unpack_fp(fq_t *v, const uint8_t *in, size_t n_elems);

/*
 * Pack/unpack n_elems F_z exponents at PARAM_Z_BITS bits each.
 * rsdp_unpack_fz returns 0 on success, -1 if ANY decoded exponent is >= z
 * (this is the verifier's "v in F_z^n" restricted-membership check) or if the
 * trailing pad bits are nonzero.
 */
void rsdp_pack_fz(uint8_t *out, const uint8_t *e, size_t n_elems);
int  rsdp_unpack_fz(uint8_t *e, const uint8_t *in, size_t n_elems);

#endif /* QINGLUAN_RSDP_H */
