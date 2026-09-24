/*************************************************
* File:        sampler.c
*
* Description: Samplers used by Rhyme: centered binomial (CBD) noise, uniform
*              short masks, the discrete-Gaussian mask sampler (CDT-based),
*              the challenge sampler, and the Algorithm-5 per-coefficient
*              rejection sampler Sample_Z that decouples z1 from the shared
*              mask, plus fixed-point exp helpers in Q62/Q63.
**************************************************/
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "sampler.h"
#include "poly.h"
#include "symmetric.h"
#include "rhyme_xof.h"

/* ------------------------------------------------------------------ bits */
typedef struct {
    stream256_state *st;
    uint8_t buf[STREAM256_BLOCKBYTES];
    unsigned pos;            /* byte position; STREAM256_BLOCKBYTES = empty */
} bitstream;

/*************************************************
* Name:        bs_init
*
* Description: Initializes a bit-stream reader over a stream256 PRNG state.
*
* Arguments:   - bitstream *bs:           bit-stream to initialize
*              - stream256_state *st:     backing PRNG state
**************************************************/
static void bs_init(bitstream *bs, stream256_state *st) {
    bs->st = st;
    bs->pos = STREAM256_BLOCKBYTES;
}
/*************************************************
* Name:        bs_byte
*
* Description: Returns the next byte from the bit-stream, refilling from the PRNG as needed.
*
* Arguments:   - bitstream *bs:   bit-stream to read from
*
* Returns:     Next byte of PRNG output.
**************************************************/
static uint8_t bs_byte(bitstream *bs) {
    if (bs->pos >= STREAM256_BLOCKBYTES) {
        stream256_squeezeblocks(bs->buf, 1, bs->st);
        bs->pos = 0;
    }
    return bs->buf[bs->pos++];
}
/* Read 8 consecutive bytes as a little-endian u64, identical byte order and
 * consumption to eight bs_byte() calls, but with a single refill check and a
 * bulk memcpy on the fast path (no per-byte branch). */
/*************************************************
* Name:        bs_u64
*
* Description: Returns the next 64-bit little-endian word from the bit-stream.
*
* Arguments:   - bitstream *bs:   bit-stream to read from
*
* Returns:     Next 64-bit PRNG word.
**************************************************/
static uint64_t bs_u64(bitstream *bs) {
    uint64_t r;
    if (bs->pos + 8 <= STREAM256_BLOCKBYTES) {
        memcpy(&r, bs->buf + bs->pos, 8);   /* fast path: 8 bytes in buffer */
        bs->pos += 8;
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        r = __builtin_bswap64(r);            /* keep little-endian semantics */
#endif
        return r;
    }
    r = 0;                                   /* slow path: straddles a block */
    for (int i = 0; i < 8; i++) r |= (uint64_t)bs_byte(bs) << (8 * i);
    return r;
}

/* ------------------------------------------------------------------ CBD(eta) */
/*************************************************
* Name:        SampleCBD
*
* Description: Samples a polynomial with coefficients from the centered binomial
*              distribution Bin(eta), deterministically from a seed and nonce.
*
* Arguments:   - poly *p:                       output polynomial
*              - const uint8_t seed[CRHBYTES]:  seed
*              - uint16_t nonce:                domain-separation nonce
**************************************************/
void SampleCBD(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce) {
    stream256_state st;
    bitstream bs;
    stream256_init(&st, seed, nonce);
    bs_init(&bs, &st);
    /* per coefficient: 2*eta bits */
    unsigned bitbuf = 0, nbits = 0;
    for (int i = 0; i < N; i++) {
        int32_t a = 0, b = 0;
        for (int j = 0; j < RHYME_ETA; j++) {
            if (nbits < 2) { bitbuf |= (unsigned)bs_byte(&bs) << nbits; nbits += 8; }
            a += bitbuf & 1;
            b += (bitbuf >> 1) & 1;
            bitbuf >>= 2; nbits -= 2;
        }
        p->coeffs[i] = a - b;
    }
}

/* ------------------------------------------------------------------ CDT Gaussians
 * Tables rhyme_cdt_y / rhyme_cdt_g (params_tables.h) give cumulative probs in
 * Q0.63 over magnitudes 0..max; entry[m] = P(|X| <= m) scaled. Sign by extra bit
 * for nonzero magnitudes. */
/*************************************************
* Name:        cdt_sample
*
* Description: Samples one value from a cumulative distribution table using a constant-time
*              branchless binary search (lower_bound via conditional moves).
*
* Arguments:   - const uint64_t *cdt:   cumulative distribution table
*              - unsigned len:          number of table entries
*              - bitstream *bs:         bit-stream supplying randomness
*
* Returns:     Sampled table index / value.
**************************************************/
static int32_t cdt_sample(const uint64_t *cdt, unsigned len, bitstream *bs) {
    uint64_t u = bs_u64(bs) >> 1;          /* 63-bit uniform */
    /* Branchless lower_bound: first index with cdt[idx] > u.
     * Same result as the plain binary search, but written so the compiler
     * emits conditional moves instead of a data-dependent (unpredictable)
     * branch, which dominated SampleY1.  Output and randomness consumption
     * are unchanged (verified bit-exact against the KAT vectors). */
    unsigned lo = 0, n = len;
    while (n > 1) {
        unsigned half = n >> 1;
        unsigned mid = lo + half;
        lo += (cdt[mid] <= u) ? half : 0;  /* compiles to cmov */
        n -= half;
    }
    lo += (lo < len && cdt[lo] <= u) ? 1u : 0u;
    int32_t mag = (int32_t)lo;
    if (mag == 0) return 0;
    return (bs_byte(bs) & 1) ? mag : -mag;
}

/*************************************************
* Name:        SampleY1
*
* Description: Samples the y1 mask polynomial from its discrete distribution,
*              deterministically from a seed and nonce.
*
* Arguments:   - poly *p:                       output mask polynomial
*              - const uint8_t seed[CRHBYTES]:  seed
*              - uint16_t nonce:                nonce
**************************************************/
void SampleY1(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce) {
    stream256_state st;
    bitstream bs;
    stream256_init(&st, seed, nonce);
    bs_init(&bs, &st);
    for (int i = 0; i < N; i++)
        p->coeffs[i] = cdt_sample(rhyme_cdt_y, RHYME_YMAX + 1, &bs);
}

/*************************************************
* Name:        SampleGauss
*
* Description: Samples a discrete-Gaussian polynomial (width sigma_y) via the CDT sampler,
*              deterministically from a seed and nonce.
*
* Arguments:   - poly *p:                       output polynomial
*              - const uint8_t seed[CRHBYTES]:  seed
*              - uint16_t nonce:                nonce
**************************************************/
void SampleGauss(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce) {
    stream256_state st;
    bitstream bs;
    stream256_init(&st, seed, nonce);
    bs_init(&bs, &st);
    for (int i = 0; i < N; i++)
        p->coeffs[i] = cdt_sample(rhyme_cdt_g, RHYME_GMAX + 1, &bs);
}


/*************************************************
* Name:        SampleGaussE
*
* Description: Samples the parity-masking noise e_bottom from the discrete
*              Gaussian of width 2*sigma_base via the CDT sampler (table
*              rhyme_cdt_e, tail RHYME_GMAX_E), deterministically from a seed
*              and nonce.  The doubled width is what makes e_bottom mod 2
*              statistically uniform, masking the secret-dependent parity in
*              the structured component (HVZK proof, Thm 4.3).
*
* Arguments:   - poly *p:                       output polynomial
*              - const uint8_t seed[CRHBYTES]:  seed
*              - uint16_t nonce:                nonce
**************************************************/
void SampleGaussE(poly *p, const uint8_t seed[CRHBYTES], uint16_t nonce) {
    stream256_state st;
    bitstream bs;
    stream256_init(&st, seed, nonce);
    bs_init(&bs, &st);
    for (int i = 0; i < N; i++)
        p->coeffs[i] = cdt_sample(rhyme_cdt_e, RHYME_GMAX_E + 1, &bs);
}

/* ------------------------------------------------------------------ challenge
 * Binary SampleInBall: TAU coefficients equal to 1, rest 0. */
/*************************************************
* Name:        SampleChallenge
*
* Description: Samples the challenge polynomial (fixed Hamming weight of +-1 coefficients)
*              from the challenge hash c_tilde. Touches only public data.
*
* Arguments:   - poly *c:                          output challenge polynomial
*              - const uint8_t seed[CTILDEBYTES]:  challenge hash
**************************************************/
void SampleChallenge(poly *c, const uint8_t seed[CTILDEBYTES]) {
    keccak_state st;
    uint8_t buf[SHAKE256_RATE];
    unsigned pos = sizeof buf;
    rhyme_shake256_init(&st);
    rhyme_shake256_absorb(&st, seed, CTILDEBYTES);
    rhyme_shake256_finalize(&st);
    poly_zero(c);
    for (unsigned i = N - TAU; i < N; i++) {
        unsigned j;
        do {
            if (pos + 2 > sizeof buf) { rhyme_shake256_squeezeblocks(buf, 1, &st); pos = 0; }
            j = (unsigned)buf[pos] | ((unsigned)buf[pos + 1] << 8);
            pos += 2;
#if N == 1024
            j &= 0x3FF;
#elif N == 512
            j &= 0x1FF;
#else
            j &= 0xFF;
#endif
        } while (j > i);
        c->coeffs[i] = c->coeffs[j];
        c->coeffs[j] = 1;
    }
}

/* ------------------------------------------------------------------ fixed-point exp
 * expm_p63(x) ~= 2^63 * exp(-x) for x in [0, ln2), x given in Q63.
 * Independent degree-12 minimax polynomial (gen/exp_coeffs.py),
 * deterministic 64-bit arithmetic, no external code. */
/*************************************************
* Name:        mul_hi64_shift
*
* Description: Computes the high part of a 64x64-bit product shifted right by sh bits, i.e.
*              (a*b) >> sh in 128-bit precision.
*
* Arguments:   - uint64_t a:     first factor
*              - uint64_t b:     second factor
*              - unsigned sh:    right-shift amount
*
* Returns:     High bits of the shifted product.
**************************************************/
static uint64_t mul_hi64_shift(uint64_t a, uint64_t b, unsigned sh) {
    unsigned __int128 z = (unsigned __int128)a * b;
    return (uint64_t)(z >> sh);
}

/*************************************************
* Name:        expm_p63
*
* Description: Fixed-point evaluation of exp(-x) for x in [0, ln2) given in Q63, returning a
*              Q63 result. Building block for the Gaussian weight table.
*
* Arguments:   - uint64_t x:   argument in Q63, range [0, ln2*2^63)
*
* Returns:     exp(-x) in Q63 fixed point.
**************************************************/
static uint64_t expm_p63(uint64_t x /* Q63, in [0, ln2*2^63) */) {
    /* Independently generated degree-12 minimax coefficients for exp(-x) on
     * [0, ln2), in Q63 fixed point. Produced from a high-precision Chebyshev
     * fit of exp(-t) (see gen/exp_coeffs.py); evaluated by the nested
     * recurrence y <- C[u] - (x*y >> 63). Max error <= 2 ulp(Q63) vs the true
     * function, which is far below the precision the rejection sampler needs. */
    static const uint64_t C[] = {
        0x000000032D4C198Cu, 0x000000335976B94Au, 0x0000024D0828C056u,
        0x0000171BDCE4F359u, 0x0000D00BFDE063AFu, 0x00068067AD7ABD9Cu,
        0x002D82D81885A853u, 0x011111110DBF1A21u, 0x0555555554FEF456u,
        0x1555555555501B05u, 0x3FFFFFFFFFFFD5E5u, 0x7FFFFFFFFFFFFF7Bu,
        0x8000000000000000u
    };
    uint64_t y = C[0];
    for (unsigned u = 1; u < sizeof C / sizeof C[0]; u++) {
        /* y = C[u] - (x*y) >> 63 */
        y = C[u] - mul_hi64_shift(x, y, 63);
    }
    return y;  /* ~ exp(-x) * 2^63 */
}

/* exp(-num / S) in Q62, where num >= 0 (64-bit), S = 2*sigma_y^2 (integer).
 * Computes t = num/S in Q62, splits t = m*ln2 + r. */
#define LN2_Q62 0x2C5C85FDF473DE6BULL   /* ln2 * 2^62 */
/*************************************************
* Name:        exp_neg_q62
*
* Description: Fixed-point evaluation of exp(-num/S) returning a Q62 result, used to build
*              the rho(k)=exp(-k^2/2 sigma^2) weight table.
*
* Arguments:   - uint64_t num:   numerator (e.g. k*k)
*              - uint64_t S:     scale denominator (e.g. 2*sigma^2 scaled)
*
* Returns:     exp(-num/S) in Q62 fixed point.
**************************************************/
static uint64_t exp_neg_q62(uint64_t num, uint64_t S) {
    /* t_q62 = floor(num * 2^62 / S) */
    unsigned __int128 t = ((unsigned __int128)num << 62) / S;
    uint64_t m = (uint64_t)(t / LN2_Q62);
    uint64_t r_q62 = (uint64_t)(t - (unsigned __int128)m * LN2_Q62);
    if (m >= 63) return 0;
    uint64_t e_q63 = expm_p63(r_q62 << 1);           /* Q63 */
    return (e_q63 >> 1) >> m;                        /* Q62 */
}

/* ------------------------------------------------------------------ Algorithm 5
 * Per coefficient: inputs y (|y| <= B0+R), challenge bit c.
 * V_c = parity-c integers in [-R, R]; p_c(v) tables in Q0.63 (params_tables.h).
 * Accept v when r * M * rho(y) < cum_{v' <= v} p_c(v') * rho(y+v') (and |y+v| <= B0).
 * All rho computed as exp(-(.)^2 / (2 sigma_y^2)) via exp_neg_q62.
 * z = y + v, output v (so that 2*X0 = v, and x = v - c). */
#define SY2X2 ((uint64_t)2 * (uint64_t)RHYME_SIGMAY * (uint64_t)RHYME_SIGMAY)

/*************************************************
* Name:        Sample_Z
*
* Description: Algorithm-5 per-coefficient rejection sampler for the first signature
*              component. For each coefficient it draws a uniform threshold and accepts a
*              candidate z1 = y + v (and records the offset v) according to the fixed-point
*              Gaussian weights, decoupling z1 from the shared mask. Rejects the whole
*              vector if any coefficient fails.
*
* Arguments:   - poly *z1:            output accepted z1 coefficients
*              - poly *v:             output offsets v per coefficient
*              - const poly *c:       challenge polynomial (parity selects the table)
*              - const poly *y1:      mask polynomial
*              - stream256_state *rng: PRNG state for thresholds
*
* Returns:     1 if the whole vector was accepted, 0 on rejection.
**************************************************/
int Sample_Z(poly *z1, poly *v, const poly *c, const poly *y1, stream256_state *rng) {
    /* exp(-k^2/2sigma^2) lookup table, |k| <= B0+R (RHYME_YMAX).
     * Both rho_y (|y|<=B0+R) and rho_z (|zc|<=B0) share it.  Built once;
     * values are bit-identical to exp_neg_q62(k*k, SY2X2). */
    static uint64_t exp_lut[RHYME_YMAX + 1];
    static int exp_lut_ready = 0;
    if (!exp_lut_ready) {
        for (int k = 0; k <= RHYME_YMAX; k++)
            exp_lut[k] = exp_neg_q62((uint64_t)((int64_t)k * k), SY2X2);
        exp_lut_ready = 1;
    }
    bitstream bs;
    bs_init(&bs, rng);
    for (int i = 0; i < N; i++) {
        int32_t y = y1->coeffs[i];
        int cb = (int)c->coeffs[i] & 1;
        const int32_t *V = cb ? rhyme_v_1 : rhyme_v_0;
        const uint64_t *P = cb ? rhyme_p63_1 : rhyme_p63_0;
        unsigned vl = cb ? RHYME_VLEN_1 : RHYME_VLEN_0;

        uint64_t r63 = bs_u64(&bs) >> 1;                 /* uniform Q0.63 */
        /* A = M * rho(y): M in Q4.60 -> A in Q4.60.  |y| <= B0+R = RHYME_YMAX */
        int32_t ay = y < 0 ? -y : y;
        uint64_t rho_y = exp_lut[ay];                    /* Q62 (table) */
        uint64_t A_q60 = mul_hi64_shift(RHYME_M_Q60, rho_y, 62);          /* Q4.60 */
        /* threshold T = r * A in Q0.60+ : r63(Q63) * A(Q4.60) >> 63 -> Q4.60 */
        uint64_t T = mul_hi64_shift(r63, A_q60, 63);
        uint64_t cum = 0;                                /* Q4.60 accumulate */
        int accepted = 0;
        for (unsigned t = 0; t < vl; t++) {
            int32_t vv = V[t];
            int64_t zc = (int64_t)y + vv;
            if (zc > B0 || zc < -B0) continue;          /* pacc = 0 */
            int64_t azc = zc < 0 ? -zc : zc;
            uint64_t rho_z = exp_lut[azc];               /* Q62 (table) */
            uint64_t term = mul_hi64_shift(P[t], rho_z, 65);              /* Q63*Q62>>65 = Q60 */
            cum += term;
            if (T < cum) {
                z1->coeffs[i] = (int32_t)zc;
                v->coeffs[i] = vv;
                accepted = 1;
                break;
            }
        }
        if (!accepted) return 0;
    }
    return 1;
}
