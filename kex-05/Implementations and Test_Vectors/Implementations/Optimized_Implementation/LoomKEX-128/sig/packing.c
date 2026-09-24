/*
 * packing.c -- byte primitives + scheme (de)serialization for SHUTTLE.
 * Pure C99 scalar; the KAT oracle for the wire format.
 *
 * See packing.h for the full API + bit/byte-order contract.  Everything
 * here is constant-time in the sense that matters: the byte SCHEDULE
 * (which byte/shift is touched at each step) depends only on loop indices,
 * never on the coefficient values.  The range checks at decode time use
 * the branchless ct_range_reject accumulator and branch only ONCE at the
 * end of each fixed-length loop.  No hardware divide on any path: the
 * /alpha_b is a power-of-two shift, and the decode "mod" is a range
 * compare, never an idiv.
 */
#include "packing.h"

#include <stdint.h>
#include <string.h>

#include "params.h"
#include "poly.h"
#include "rans.h"

/* ceil(q / alpha_b): the legal value range [0, CEIL_Q_ALPHA_B) of a packed
 * pk field b1 = b/alpha_b.  Reproducible closed form of (q, alpha_b):
 * 7681 / 30721 / 14849.  NOTE 2^d_b can EXCEED this (8192>7681,
 * 32768>30721, 16384>14849), so a malformed pk can carry an over-range b1
 * -> pk_decode must range-check it. */
#define CEIL_Q_ALPHA_B (((Q) + (ALPHA_B)-1) / (ALPHA_B))

/* alpha_b is a power of two for all sets; the /alpha_b on encode and the
 * *alpha_b on decode are both shifts by LOG2_ALPHA_B. */
_Static_assert((ALPHA_B & (ALPHA_B - 1)) == 0,
               "alpha_b must be a power of two for the shift rescale");
#define LOG2_ALPHA_B ((ALPHA_B) == 2 ? 1 : 2)

/* ====================================================================== *
 *  Byte primitives                                                       *
 * ======================================================================
 */

int integer_to_bytes(uint8_t *out, uint64_t x, unsigned len)
{
    unsigned i;
    /* Bottom check: x must fit in `len` bytes.  For len >= 8 every uint64
     * fits, so only the small-len case can overflow (1<<(8*len) would
     * itself overflow for len==8). */
    if (len < 8 && x >= ((uint64_t)1 << (8 * len)))
        return -1;
    for (i = 0; i < len; ++i)
        out[i] = (uint8_t)((x >> (8 * i)) & 0xFF);
    return 0;
}

uint64_t bytes_to_integer(const uint8_t *in, unsigned len)
{
    uint64_t x = 0;
    unsigned i;
    for (i = 0; i < len; ++i)
        x |= (uint64_t)in[i] << (8 * i);
    return x;
}

/* poly_to_bytes / bytes_to_poly: the LSB-first d-bit packer with a
 * bit-accumulator whose byte schedule depends only on the loop index (the
 * standard bitpack/bitunpack idiom).  For every SHUTTLE field N*d is a
 * multiple of 8 (asserted in params.h), so the flush emits exactly N*d/8
 * full bytes with no padding bits -- but the generic zero-pad flush is
 * implemented so the code is correct for any future d. */
void poly_to_bytes(uint8_t *out, const poly *w, unsigned d)
{
    const uint32_t mask =
        (d >= 32) ? 0xFFFFFFFFu : (((uint32_t)1u << d) - 1u);
    uint32_t acc = 0;     /* LSB-first bit accumulator        */
    unsigned accbits = 0; /* valid bits currently in acc      */
    unsigned outpos = 0;
    unsigned i;
    for (i = 0; i < N; ++i) {
        acc |= ((uint32_t)w->coeffs[i] & mask) << accbits;
        accbits += d;
        /* Emit whole bytes while available.  Schedule is index-driven. */
        while (accbits >= 8) {
            out[outpos++] = (uint8_t)(acc & 0xFF);
            acc >>= 8;
            accbits -= 8;
        }
    }
    /* Flush a final partial byte zero-padded (no-op when N*d % 8 == 0). */
    if (accbits > 0)
        out[outpos++] = (uint8_t)(acc & 0xFF);
}

void bytes_to_poly(poly *w, const uint8_t *in, unsigned d)
{
    const uint32_t mask =
        (d >= 32) ? 0xFFFFFFFFu : (((uint32_t)1u << d) - 1u);
    uint32_t acc = 0;
    unsigned accbits = 0;
    unsigned inpos = 0;
    unsigned i;
    for (i = 0; i < N; ++i) {
        while (accbits < d) {
            acc |= (uint32_t)in[inpos++] << accbits;
            accbits += 8;
        }
        w->coeffs[i] = (int32_t)(acc & mask);
        acc >>= d;
        accbits -= d;
    }
}

/* ====================================================================== *
 *  ct_range_reject                                                       *
 * ======================================================================
 */

uint32_t ct_range_reject(int32_t v, int32_t lo, int32_t hi)
{
    /* (v < lo) | (v > hi), as a 0/1 accumulator.  Branchless: the two
     * comparisons lower to set-on-condition, not a control-flow branch. */
    uint32_t below = (uint32_t)(v < lo);
    uint32_t above = (uint32_t)(v > hi);
    return below | above;
}

/* ====================================================================== *
 *  Public key                                                            *
 * ======================================================================
 */

void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES],
             const uint8_t seedA[SEEDBYTES], const poly b[EM])
{
    unsigned i, k;
    poly b1;
    memcpy(pk, seedA, SEEDBYTES);
    for (i = 0; i < (unsigned)EM; ++i) {
        /* b1 = b / alpha_b (exact: every coeff is a multiple of alpha_b
         * after RoundB).  Public data -> a plain shift is fine. */
        for (k = 0; k < N; ++k)
            b1.coeffs[k] = b[i].coeffs[k] >> LOG2_ALPHA_B;
        poly_to_bytes(pk + SEEDBYTES + (size_t)i * POLYPK_PACKEDBYTES, &b1,
                      DB_BITS);
    }
}

int unpack_pk(uint8_t seedA[SEEDBYTES], poly b[EM],
              const uint8_t pk[CRYPTO_PUBLICKEYBYTES])
{
    unsigned i, k;
    uint32_t fail = 0;
    memcpy(seedA, pk, SEEDBYTES);
    for (i = 0; i < (unsigned)EM; ++i) {
        poly b1;
        bytes_to_poly(&b1, pk + SEEDBYTES + (size_t)i * POLYPK_PACKEDBYTES,
                      DB_BITS);
        for (k = 0; k < N; ++k) {
            /* RANGE-CHECK: b1 must be < ceil(q/alpha_b) (2^d_b over-covers
             * it).  Accumulate the reject decision; branch once at the
             * end.
             */
            fail |= ct_range_reject(b1.coeffs[k], 0, CEIL_Q_ALPHA_B - 1);
            /* Rescale *alpha_b; lands in [0,q) for a legal b1. */
            b[i].coeffs[k] = b1.coeffs[k] << LOG2_ALPHA_B;
        }
    }
    return fail ? -1 : 0;
}

/* ====================================================================== *
 *  Secret key                                                            *
 * ======================================================================
 */

void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES],
             const uint8_t seedA[SEEDBYTES], const poly b[EM],
             const uint8_t masterSeed[CHALLENGESEEDBYTES],
             const uint8_t tr[CHALLENGESEEDBYTES], const poly s[ELL],
             const poly ep[EM])
{
    unsigned i, k;
    size_t cur = 0;
    poly tmp;

    /* (1) seedA */
    memcpy(sk + cur, seedA, SEEDBYTES);
    cur += SEEDBYTES;

    /* (2) PolyToBytes(b/alpha_b, d_b) over EM polys (== pk body). */
    for (i = 0; i < (unsigned)EM; ++i) {
        for (k = 0; k < N; ++k)
            tmp.coeffs[k] = b[i].coeffs[k] >> LOG2_ALPHA_B;
        poly_to_bytes(sk + cur, &tmp, DB_BITS);
        cur += POLYPK_PACKEDBYTES;
    }

    /* (3) masterSeed (K) */
    memcpy(sk + cur, masterSeed, CHALLENGESEEDBYTES);
    cur += CHALLENGESEEDBYTES;

    /* (4) tr */
    memcpy(sk + cur, tr, CHALLENGESEEDBYTES);
    cur += CHALLENGESEEDBYTES;

    /* (5) PolyToBytes(s + BS_ENC, d_s) over ELL polys (shift into
     * [0, 2 BS_ENC] subset [0, 2^d_s)). */
    for (i = 0; i < (unsigned)ELL; ++i) {
        for (k = 0; k < N; ++k)
            tmp.coeffs[k] = s[i].coeffs[k] + BS_ENC;
        poly_to_bytes(sk + cur, &tmp, DS_BITS);
        cur += POLYS_PACKEDBYTES;
    }

    /* (6) PolyToBytes(e' + BE_ENC, d_e) over EM polys. */
    for (i = 0; i < (unsigned)EM; ++i) {
        for (k = 0; k < N; ++k)
            tmp.coeffs[k] = ep[i].coeffs[k] + BE_ENC;
        poly_to_bytes(sk + cur, &tmp, DE_BITS);
        cur += POLYE_PACKEDBYTES;
    }

    /* The byte cursor is data-independent; it must land exactly at the
     * declared sk size (layout-drift guard). */
    _Static_assert(SEEDBYTES + (size_t)EM * POLYPK_PACKEDBYTES +
                           2 * CHALLENGESEEDBYTES +
                           (size_t)ELL * POLYS_PACKEDBYTES +
                           (size_t)EM * POLYE_PACKEDBYTES ==
                       CRYPTO_SECRETKEYBYTES,
                   "skEncode layout must sum to CRYPTO_SECRETKEYBYTES");
    (void)cur;
}

int unpack_sk(uint8_t seedA[SEEDBYTES], poly b[EM],
              uint8_t masterSeed[CHALLENGESEEDBYTES],
              uint8_t tr[CHALLENGESEEDBYTES], poly s[ELL], poly ep[EM],
              const uint8_t sk[CRYPTO_SECRETKEYBYTES])
{
    unsigned i, k;
    size_t cur = 0;
    uint32_t fail = 0;

    /* (1) seedA */
    memcpy(seedA, sk + cur, SEEDBYTES);
    cur += SEEDBYTES;

    /* (2) b/alpha_b -> rescale *alpha_b.  The sk is locally produced, so
     * this is the trusted path; we still rescale via the shift.  (A
     * malformed sk is a local-only fault; we do not range-check b here
     * since the same bytes also appear in the pk which pk_decode does
     * check.) */
    for (i = 0; i < (unsigned)EM; ++i) {
        poly b1;
        bytes_to_poly(&b1, sk + cur, DB_BITS);
        for (k = 0; k < N; ++k)
            b[i].coeffs[k] = b1.coeffs[k] << LOG2_ALPHA_B;
        cur += POLYPK_PACKEDBYTES;
    }

    /* (3) masterSeed (K) */
    memcpy(masterSeed, sk + cur, CHALLENGESEEDBYTES);
    cur += CHALLENGESEEDBYTES;

    /* (4) tr */
    memcpy(tr, sk + cur, CHALLENGESEEDBYTES);
    cur += CHALLENGESEEDBYTES;

    /* (5) s : unshift -BS_ENC, RANGE-CHECK into [-BS_ENC, BS_ENC]. */
    for (i = 0; i < (unsigned)ELL; ++i) {
        poly t;
        bytes_to_poly(&t, sk + cur, DS_BITS);
        for (k = 0; k < N; ++k) {
            int32_t v = t.coeffs[k] - BS_ENC;
            fail |= ct_range_reject(v, -(int32_t)BS_ENC, (int32_t)BS_ENC);
            s[i].coeffs[k] = v;
        }
        cur += POLYS_PACKEDBYTES;
    }

    /* (6) e' : unshift -BE_ENC, RANGE-CHECK into [-BE_ENC, BE_ENC]. */
    for (i = 0; i < (unsigned)EM; ++i) {
        poly t;
        bytes_to_poly(&t, sk + cur, DE_BITS);
        for (k = 0; k < N; ++k) {
            int32_t v = t.coeffs[k] - BE_ENC;
            fail |= ct_range_reject(v, -(int32_t)BE_ENC, (int32_t)BE_ENC);
            ep[i].coeffs[k] = v;
        }
        cur += POLYE_PACKEDBYTES;
    }

    (void)cur;
    return fail ? -1 : 0;
}

/* ====================================================================== *
 *  Commitment                                                            *
 * ======================================================================
 */

void pack_com(uint8_t out[ENCODECOM_BYTES], const poly *comY_h,
              const poly *comY_0)
{
    /* EncodeCom = PolyToBytes(comY_h, d_h) || PolyToBytes(comY_0, 1).
     * Preconditions (caller-guaranteed): comY_h[k] in [0,H_h),
     * comY_0[k] in {0,1}.  The 1-bit comY_0 packing is the degenerate
     * d=1 case of poly_to_bytes. */
    poly_to_bytes(out, comY_h, DH_BITS);
    poly_to_bytes(out + POLYWH_PACKEDBYTES, comY_0, 1);
}

int unpack_com(poly *comY_h, poly *comY_0,
               const uint8_t in[ENCODECOM_BYTES])
{
    unsigned k;
    uint32_t fail = 0;

    /* comY_h: decode d_h-bit fields, then RANGE-CHECK each into [0, H_h).
     * H_h = 30/120/58 is NOT a power of two and d_h=5/7/6 over-covers it,
     * so the gap [H_h, 2^d_h) is a non-empty per-coeff reject region.  We
     * range-CHECK then REJECT -- we do NOT reduce mod H_h (that would
     * break injectivity -> SUF-CMA). */
    bytes_to_poly(comY_h, in, DH_BITS);
    for (k = 0; k < N; ++k)
        fail |= ct_range_reject(comY_h->coeffs[k], 0, (int32_t)HH - 1);

    /* comY_0: a 1-bit field, so every decoded value is in {0,1} -- the
     * range check is structurally a no-op (kept for symmetry / clarity).
     */
    bytes_to_poly(comY_0, in + POLYWH_PACKEDBYTES, 1);
    for (k = 0; k < N; ++k)
        fail |= ct_range_reject(comY_0->coeffs[k], 0, 1);

    return fail ? -1 : 0;
}

/* ====================================================================== *
 *  Signature serialization (sigEncode / sigDecode)                       *
 * ====================================================================== *
 *
 *  Symbol counts (logical order Q0 ++ Qs ++ h, polynomial-major then
 *  coefficient-major): the z0 block is the FIRST poly of z1; the z_s block
 * is the next ELL polys; the hint is EM polys.  z1 has Z1LEN = 1 + ELL
 * polys.
 */
#define SIG_NQ0 ((size_t)N)       /* z0 quotients: 1 poly       */
#define SIG_NQS ((size_t)ELL * N) /* z_s quotients: ELL polys   */
#define SIG_NH ((size_t)EM * N)   /* hint: EM polys             */
#define SIG_NTOT (SIG_NQ0 + SIG_NQS + SIG_NH)

/* sig_split_z: the signed-arithmetic block-adaptive peel.
 *   head = z >> b  (arithmetic shift = floor(z / 2^b) for negative z too);
 *   low  = z & (2^b - 1)  (always in [0, 2^b));
 * reconstruction z = (head << b) | low = 2^b*head + low, NO modular
 * reduction (Description.tex:2364-2369; decode condition 2). */
static inline void sig_split_z(int32_t z, int b, int32_t *head,
                               int32_t *low)
{
    *head = z >> b; /* arithmetic shift */
    *low = z & ((1 << b) - 1);
}

static inline int32_t sig_join_z(int32_t head, int32_t low, int b)
{
    return (head << b) | low; /* = 2^b*head + low, no mod */
}

/* pack_sig_zlow / unpack_sig_zlow: the raw low-bit body R, LSB-first per
 * the PolyToBytes convention.  Each poly's N coeffs contribute b low
 * bits. A 64-bit accumulator handles b up to 7 with margin (b0=2, b_s<=7).
 */
static void pack_sig_zlow(uint8_t *out, const poly *z1i, int b)
{
    const uint32_t mask = (1u << b) - 1u;
    uint64_t acc = 0;
    int accbits = 0;
    size_t outpos = 0;
    for (int k = 0; k < N; k++) {
        int32_t head, low;
        sig_split_z(z1i->coeffs[k], b, &head, &low);
        acc |= (uint64_t)((uint32_t)low & mask) << accbits;
        accbits += b;
        while (accbits >= 8) {
            out[outpos++] = (uint8_t)acc;
            acc >>= 8;
            accbits -= 8;
        }
    }
    if (accbits > 0)
        out[outpos] = (uint8_t)acc;
}

/* Reconstruct z1i->coeffs from the already-decoded heads (in z1i) + the b
 * low bits.  The head was placed in z1i by the rANS decode; this
 * multiplies it back in.  Low part is in [0,2^b) by construction (an
 * unsigned b-bit field), so NO modular reduction is needed (decode
 * condition 2). */
static void unpack_sig_zlow(poly *z1i, const uint8_t *in, int b)
{
    const uint32_t mask = (1u << b) - 1u;
    uint64_t acc = 0;
    int accbits = 0;
    size_t inpos = 0;
    for (int k = 0; k < N; k++) {
        while (accbits < b) {
            acc |= (uint64_t)in[inpos++] << accbits;
            accbits += 8;
        }
        int32_t low = (int32_t)((uint32_t)acc & mask);
        acc >>= b;
        accbits -= b;
        z1i->coeffs[k] = sig_join_z(z1i->coeffs[k], low, b);
    }
}

static void sig_clear_outputs(poly z1[Z1LEN], poly h[EM])
{
    memset(z1, 0, (size_t)Z1LEN * sizeof z1[0]);
    memset(h, 0, (size_t)EM * sizeof h[0]);
}

/* ---- RAW path (validation; un-rANS'd) ---- */
void pack_sig_raw(uint8_t *sig, const uint8_t seedC[CHALLENGESEEDBYTES],
                  const poly z1[Z1LEN], const poly h[EM])
{
    uint8_t *p = sig;
    int i, k;
    memcpy(p, seedC, CHALLENGESEEDBYTES);
    p += CHALLENGESEEDBYTES;
    /* z1: 2 bytes/coeff, little-endian signed int16 (centered coeffs fit).
     */
    for (i = 0; i < Z1LEN; i++) {
        for (k = 0; k < N; k++) {
            int16_t v = (int16_t)z1[i].coeffs[k];
            p[2 * k] = (uint8_t)((uint16_t)v & 0xff);
            p[2 * k + 1] = (uint8_t)(((uint16_t)v >> 8) & 0xff);
        }
        p += SIG_RAW_Z1_PACKEDBYTES;
    }
    /* hint: d_h-bit fields (in [0,H_h) by construction). */
    for (i = 0; i < EM; i++) {
        poly_to_bytes(p, &h[i], DH_BITS);
        p += SIG_RAW_H_PACKEDBYTES;
    }
}

int unpack_sig_raw(uint8_t seedC[CHALLENGESEEDBYTES], poly z1[Z1LEN],
                   poly h[EM], const uint8_t *sig)
{
    const uint8_t *p = sig;
    int i, k;
    uint32_t fail = 0;
    memcpy(seedC, p, CHALLENGESEEDBYTES);
    p += CHALLENGESEEDBYTES;
    for (i = 0; i < Z1LEN; i++) {
        for (k = 0; k < N; k++) {
            uint16_t u =
                (uint16_t)p[2 * k] | ((uint16_t)p[2 * k + 1] << 8);
            z1[i].coeffs[k] = (int32_t)(int16_t)u;
        }
        p += SIG_RAW_Z1_PACKEDBYTES;
    }
    /* hint: decode d_h-bit fields, RANGE-CHECK each into [0,H_h)
     * (never reduce mod H_h -- range-CHECK then reject). */
    for (i = 0; i < EM; i++) {
        bytes_to_poly(&h[i], p, DH_BITS);
        for (k = 0; k < N; k++)
            fail |= ct_range_reject(h[i].coeffs[k], 0, (int32_t)HH - 1);
        p += SIG_RAW_H_PACKEDBYTES;
    }
    if (fail) {
        sig_clear_outputs(z1, h);
        return -1;
    }
    return 0;
}

/* ---- Production path (rANS com + raw z-low) ---- */
/* Gather the merged symbol arrays from (z1, h): Q0 from z1[0], Qs from
 * z1[1..ELL], hint from h[0..EM-1]. */
static void sig_gather_symbols(const poly z1[Z1LEN], const poly h[EM],
                               int32_t *q0, int32_t *qs, int32_t *hh)
{
    int i, k;
    for (k = 0; k < N; k++) {
        int32_t head, low;
        sig_split_z(z1[0].coeffs[k], RANS_B0, &head, &low);
        q0[k] = head;
    }
    for (i = 0; i < ELL; i++)
        for (k = 0; k < N; k++) {
            int32_t head, low;
            sig_split_z(z1[i + 1].coeffs[k], RANS_BS, &head, &low);
            qs[(size_t)i * N + k] = head;
        }
    for (i = 0; i < EM; i++)
        for (k = 0; k < N; k++)
            hh[(size_t)i * N + k] = h[i].coeffs[k];
}

int pack_sig(uint8_t *sig, const uint8_t seedC[CHALLENGESEEDBYTES],
             const poly z1[Z1LEN], const poly h[EM])
{
    uint8_t *p = sig;
    int i;
    int32_t q0[SIG_NQ0], qs[SIG_NQS], hh[SIG_NH];

    /* (1) seedC verbatim prefix. */
    memcpy(p, seedC, CHALLENGESEEDBYTES);
    p += CHALLENGESEEDBYTES;

    /* (2) rlen field + reserved com region. */
    uint8_t *lenp = p;
    p += 2;
    uint8_t *rp = p;

    sig_gather_symbols(z1, h, q0, qs, hh);
    size_t rlen;
    if (shuttle_rans_encode(rp, &rlen, RANS_RESERVED_BYTES, q0, qs, hh,
                            SIG_NQ0, SIG_NQS, SIG_NH) != 0)
        return -2; /* out-of-support / overflow: signer retries. */
    /* Zero the padding tail [rlen, RESERVED) so every signature byte is
     * authenticated (the verifier requires it zero -> no malleability). */
    memset(rp + rlen, 0, RANS_RESERVED_BYTES - rlen);
    lenp[0] = (uint8_t)(rlen & 0xff);
    lenp[1] = (uint8_t)((rlen >> 8) & 0xff);
    p += RANS_RESERVED_BYTES;

    /* (3) raw low-bit body R: z0 block (b0), then z_s block (b_s). */
    pack_sig_zlow(p, &z1[0], RANS_B0);
    p += RANS_Z0_LO_PACKEDBYTES;
    for (i = 0; i < ELL; i++) {
        pack_sig_zlow(p, &z1[i + 1], RANS_BS);
        p += (RANS_ZS_LO_PACKEDBYTES / ELL);
    }

    _Static_assert(SIG_PACKED_BYTES == CRYPTO_BYTES,
                   "compact signature length must equal CRYPTO_BYTES "
                   "(CRYPTO_BYTES is the exact realized size, no padding)");
    _Static_assert(RANS_ZS_LO_PACKEDBYTES % ELL == 0,
                   "z_s low-bit body must split evenly across ELL polys "
                   "(N*b_s multiple of 8)");
    return 0;
}

int unpack_sig(uint8_t seedC[CHALLENGESEEDBYTES], poly z1[Z1LEN],
               poly h[EM], const uint8_t *sig)
{
    const uint8_t *p = sig;
    int i;
    int32_t q0[SIG_NQ0], qs[SIG_NQS], hh[SIG_NH];
    uint8_t recom[RANS_RESERVED_BYTES];
    uint32_t fail = 0;

    /* (1) seedC verbatim. */
    memcpy(seedC, p, CHALLENGESEEDBYTES);
    p += CHALLENGESEEDBYTES;

    /* (2) rlen + reserved com region.  Outer-container checks: rlen <=
     * RESERVED, and every padding byte zero. */
    size_t rlen = (size_t)p[0] | ((size_t)p[1] << 8);
    p += 2;
    const uint8_t *rp = p;
    if (rlen > RANS_RESERVED_BYTES) {
        sig_clear_outputs(z1, h);
        return -1;
    }
    for (size_t pad = rlen; pad < (size_t)RANS_RESERVED_BYTES; pad++)
        if (rp[pad] != 0) {
            sig_clear_outputs(z1, h);
            return -1;
        }

    /* (2b) canonical rANS decode (initial-state range / full consume /
     * terminal state == L). */
    if (shuttle_rans_decode(q0, qs, hh, SIG_NQ0, SIG_NQS, SIG_NH, rp,
                            rlen) != 0) {
        sig_clear_outputs(z1, h);
        return -1;
    }
    p += RANS_RESERVED_BYTES;

    /* (3) place decoded heads into z1, hint into h.  Per-block SUPPORT is
     * already guaranteed by the SLOT lookup (decoded quotient lands in
     * [LO, LO+N)); we additionally RANGE-CHECK the hint into [0,H_h)
     * (range-CHECK then reject, NEVER mod H_h). */
    for (int k = 0; k < N; k++)
        z1[0].coeffs[k] = q0[k];
    for (i = 0; i < ELL; i++)
        for (int k = 0; k < N; k++)
            z1[i + 1].coeffs[k] = qs[(size_t)i * N + k];
    for (i = 0; i < EM; i++)
        for (int k = 0; k < N; k++) {
            int32_t v = hh[(size_t)i * N + k];
            fail |= ct_range_reject(v, 0, (int32_t)HH - 1);
            h[i].coeffs[k] = v;
        }
    if (fail) {
        sig_clear_outputs(z1, h);
        return -1;
    }

    /* (4) raw low-bit body R: reconstruct z = 2^b*head + low. */
    unpack_sig_zlow(&z1[0], p, RANS_B0);
    p += RANS_Z0_LO_PACKEDBYTES;
    for (i = 0; i < ELL; i++) {
        unpack_sig_zlow(&z1[i + 1], p, RANS_BS);
        p += (RANS_ZS_LO_PACKEDBYTES / ELL);
    }

    /* (5) byte-for-byte RE-ENCODE check (the SHUTTLE injectivity
     * addition): re-encode the recovered (z1,hint) and require the com
     * bytes to match the input exactly.  A decoded triple that re-encodes
     * to a different byte string would be a malleable alias.  Together
     * with the padding-zero + terminal-state checks this makes sigDecode
     * injective.
     */
    {
        int32_t r0[SIG_NQ0], rs[SIG_NQS], rh[SIG_NH];
        size_t relen;
        sig_gather_symbols(z1, h, r0, rs, rh);
        if (shuttle_rans_encode(recom, &relen, RANS_RESERVED_BYTES, r0, rs,
                                rh, SIG_NQ0, SIG_NQS, SIG_NH) != 0 ||
            relen != rlen || memcmp(recom, rp, rlen) != 0) {
            sig_clear_outputs(z1, h);
            return -1;
        }
    }
    return 0;
}
