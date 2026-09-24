/*
 * QingLuan Digital Signature Scheme
 * rsdp.c - RSDP operations (CROSS-RSDP conventions)
 *
 *   - H = [V | I_{n-k}], V uniform in F_p^{(n-k) x k} from Seed_pk.
 *   - x H^T = V * x[0:k] + x[k:n]  (rsdp_apply_H).
 *   - Secret e = g^eta, eta in F_z^n.
 *   - CSPRNG = SM3 XOF (hash.h) with 1-byte domain tag + 2-byte LE instance const.
 *   - Bit-packing little-endian; rsdp_unpack_fz enforces the restricted check.
 *
 * Rejection sampling has negligible timing variation (acceptance >= 7/8).
 */

#include "rsdp.h"
#include "restr.h"
#include "fq_arith.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * XOF (CSPRNG) setup with domain separation
 * ============================================================ */

static void xof_u16le(xof_ctx_t *xof, uint16_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)(v >> 8);
    xof_absorb(xof, b, 2);
}

/* Initialize an XOF over (domain || seed[seed_len] || LE16(inst)). */
static void csprng_init(xof_ctx_t *xof, uint8_t domain,
                        const uint8_t *seed, size_t seed_len, uint16_t inst)
{
    xof_init(xof);
    xof_absorb(xof, &domain, 1);
    xof_absorb(xof, seed, seed_len);
    xof_u16le(xof, inst);
    xof_finalize(xof);
}

/* ============================================================
 * Rejection sampling
 * ============================================================ */

fq_t rsdp_csprng_fp(xof_ctx_t *xof)
{
    uint8_t buf;
    for (;;) {
        xof_squeeze(xof, &buf, 1);
        buf &= 0x7F;                 /* p = 127 -> 7 bits */
        if (buf < PARAM_Q) return (fq_t)buf;
    }
}

fq_t rsdp_csprng_fp_star(xof_ctx_t *xof)
{
    uint8_t buf;
    for (;;) {
        xof_squeeze(xof, &buf, 1);
        buf &= 0x7F;
        if (buf < (uint8_t)(PARAM_Q - 1)) return (fq_t)(buf + 1); /* {1..p-1} */
    }
}

uint8_t rsdp_csprng_fz(xof_ctx_t *xof)
{
    uint8_t buf;
    for (;;) {
        xof_squeeze(xof, &buf, 1);
        buf &= 0x07;                 /* z = 7 -> 3 bits */
        if (buf < PARAM_Z) return buf;
    }
}

/* ============================================================
 * Key / matrix expansion
 * ============================================================ */

void rsdp_keypair_seeds(uint8_t *seed_e, uint8_t *seed_pk,
                        const uint8_t *seed_sk)
{
    xof_ctx_t xof;
    csprng_init(&xof, DOMAIN_EXPAND_SK, seed_sk, PARAM_KEYSEED_BYTES,
                (uint16_t)(3 * PARAM_TAU + 1));
    xof_squeeze(&xof, seed_e, PARAM_KEYSEED_BYTES);
    xof_squeeze(&xof, seed_pk, PARAM_KEYSEED_BYTES);
    secure_zero(&xof, sizeof(xof));
}

void rsdp_expand_matrix(fq_t *V, const uint8_t *seed_pk)
{
    xof_ctx_t xof;
    csprng_init(&xof, DOMAIN_MATRIX, seed_pk, PARAM_KEYSEED_BYTES,
                (uint16_t)(3 * PARAM_TAU + 2));
    for (size_t i = 0; i < (size_t)PARAM_R * (size_t)PARAM_K; i++)
        V[i] = rsdp_csprng_fp(&xof);
    secure_zero(&xof, sizeof(xof));
}

void rsdp_gen_secret_exp(uint8_t *eta, const uint8_t *seed_e)
{
    xof_ctx_t xof;
    csprng_init(&xof, DOMAIN_ERROR, seed_e, PARAM_KEYSEED_BYTES,
                (uint16_t)(3 * PARAM_TAU + 3));
    for (int i = 0; i < PARAM_N; i++)
        eta[i] = rsdp_csprng_fz(&xof);
    secure_zero(&xof, sizeof(xof));
}

void rsdp_expand_sk(uint8_t *eta, fq_t *V, const uint8_t *seed_sk)
{
    uint8_t seed_e[PARAM_KEYSEED_BYTES];
    uint8_t seed_pk[PARAM_KEYSEED_BYTES];
    rsdp_keypair_seeds(seed_e, seed_pk, seed_sk);
    rsdp_expand_matrix(V, seed_pk);
    rsdp_gen_secret_exp(eta, seed_e);
    secure_zero(seed_e, sizeof(seed_e));
    secure_zero(seed_pk, sizeof(seed_pk));
}

/* ============================================================
 * Linear algebra:  out = x H^T = V * x[0:k] + x[k:n]
 * ============================================================ */

void rsdp_apply_H(fq_t *out, const fq_t *V, const fq_t *x)
{
    const fq_t *x_info = x;              /* first k coordinates */
    const fq_t *x_syst = x + PARAM_K;    /* last  r = n-k coordinates */
    fq_mat_vec_mul(out, V, x_info, PARAM_R, PARAM_K);
    for (size_t i = 0; i < (size_t)PARAM_R; i++)
        out[i] = fq_add(out[i], x_syst[i]);
}

void rsdp_compute_syndrome(fq_t *s, const fq_t *V, const fq_t *e)
{
    rsdp_apply_H(s, V, e);
}

/* ============================================================
 * Bit-packing (little-endian)
 * ============================================================ */

static void pack_bits(uint8_t *out, const uint8_t *vals_lo, const fq_t *vals_fq,
                      size_t cnt, unsigned bits)
{
    size_t out_bytes = (cnt * bits + 7) / 8;
    memset(out, 0, out_bytes);
    size_t bit_pos = 0;
    for (size_t i = 0; i < cnt; i++) {
        uint32_t val = vals_fq ? (uint32_t)vals_fq[i] : (uint32_t)vals_lo[i];
        size_t byte_pos = bit_pos >> 3;
        unsigned off = (unsigned)(bit_pos & 7);
        out[byte_pos] |= (uint8_t)(val << off);
        if (off + bits > 8 && byte_pos + 1 < out_bytes)
            out[byte_pos + 1] |= (uint8_t)(val >> (8 - off));
        if (off + bits > 16 && byte_pos + 2 < out_bytes)
            out[byte_pos + 2] |= (uint8_t)(val >> (16 - off));
        bit_pos += bits;
    }
}

/* Decode cnt values; return them in vals (uint8_t). No range/pad check here. */
static void unpack_bits(uint8_t *vals, const uint8_t *in, size_t cnt, unsigned bits)
{
    size_t out_bytes = (cnt * bits + 7) / 8;
    uint32_t mask = (1u << bits) - 1u;
    size_t bit_pos = 0;
    for (size_t i = 0; i < cnt; i++) {
        size_t byte_pos = bit_pos >> 3;
        unsigned off = (unsigned)(bit_pos & 7);
        uint32_t val = (uint32_t)in[byte_pos] >> off;
        if (off + bits > 8 && byte_pos + 1 < out_bytes)
            val |= (uint32_t)in[byte_pos + 1] << (8 - off);
        if (off + bits > 16 && byte_pos + 2 < out_bytes)
            val |= (uint32_t)in[byte_pos + 2] << (16 - off);
        vals[i] = (uint8_t)(val & mask);
        bit_pos += bits;
    }
}

/* Return 1 iff the high pad bits of the final packed byte are all zero. */
static int pad_bits_zero(const uint8_t *in, size_t cnt, unsigned bits)
{
    size_t total = cnt * bits;
    size_t out_bytes = (total + 7) / 8;
    unsigned used_in_last = (unsigned)(total - (out_bytes - 1) * 8); /* 1..8 */
    if (used_in_last == 8) return 1;
    return (in[out_bytes - 1] >> used_in_last) == 0;
}

void rsdp_pack_fp(uint8_t *out, const fq_t *v, size_t n_elems)
{
    pack_bits(out, NULL, v, n_elems, PARAM_Q_BITS);
}

int rsdp_unpack_fp(fq_t *v, const uint8_t *in, size_t n_elems)
{
    uint8_t *tmp = (uint8_t *)malloc(n_elems);
    if (!tmp) return -1;                         /* fail closed (was: silent zero) */
    unpack_bits(tmp, in, n_elems, PARAM_Q_BITS);
    int ok = pad_bits_zero(in, n_elems, PARAM_Q_BITS);
    /* Canonical-encoding check (mirrors rsdp_unpack_fz): every F_p element must
     * be < p and the trailing pad bits must be zero. Without this, the high pad
     * bits of each packed value are unauthenticated (the verifier re-packs the
     * decoded vector, zeroing pad) and value p (== 0 mod p) is a non-canonical
     * alias -- both let distinct byte strings pass as the same signature
     * (signature malleability / sEUF break). */
    for (size_t i = 0; i < n_elems; i++) {
        if (tmp[i] >= (uint8_t)PARAM_Q) ok = 0;
        v[i] = (fq_t)tmp[i];
    }
    secure_zero(tmp, n_elems);
    free(tmp);
    return ok ? 0 : -1;
}

void rsdp_pack_fz(uint8_t *out, const uint8_t *e, size_t n_elems)
{
    pack_bits(out, e, NULL, n_elems, PARAM_Z_BITS);
}

int rsdp_unpack_fz(uint8_t *e, const uint8_t *in, size_t n_elems)
{
    unpack_bits(e, in, n_elems, PARAM_Z_BITS);
    int ok = pad_bits_zero(in, n_elems, PARAM_Z_BITS);
    /* Restricted-membership check: every exponent must be < z. */
    for (size_t i = 0; i < n_elems; i++)
        if (e[i] >= (uint8_t)PARAM_Z) ok = 0;
    return ok ? 0 : -1;
}
