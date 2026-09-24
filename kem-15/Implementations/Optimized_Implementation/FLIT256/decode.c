#include "decode.h"
#include "params.h"
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

/*************************************************
* Name:        f1u64_getbit
*
* Description: Extract the i-th coefficient of polynomial p (0 or 1).
*              i>>6 locates the 64-bit word, i&63 locates the bit within it.
*
* Arguments:   - const poly_f1_u64 *p: input polynomial
*              - int i:                coefficient index
*
* Returns:     0 or 1
**************************************************/
static inline int f1u64_getbit(const poly_f1_u64 *p, int i)
{
    int pos = i >> 6;  /* i / 64 */
    int bit = i & 63;  /* i % 64 */
    return (int)((p->w[pos] >> bit) & 1);
}

/*************************************************
* Name:        f1u64_eval_one
*
* Description: Evaluate f(1) mod 2, i.e. the XOR of all coefficients
*              (parity check). In F2, X^n+1 = (X+1)^n, so f(1)=0 iff
*              (X+1) divides f, i.e. f is not invertible.
*              The successive halving XOR compresses 64-bit parity to
*              the lowest bit.
*
* Arguments:   - const poly_f1_u64 *a: input polynomial
*
* Returns:     1 if invertible, 0 otherwise
**************************************************/
static int f1u64_eval_one(const poly_f1_u64 *a)
{
    uint64_t v = 0;
    unsigned int i;

    for (i = 0; i < F1_N_WORDS; i++)
    {
        v ^= a->w[i];
    }

    v ^= v >> 32;
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;

    return (int)(v & 1);
}

/*************************************************
* Name:        f1u64_translate
*
* Description: Apply f(X) -> f(X+1) mod (X^n+1) over F2.
*              This is an involution (applying it twice recovers
*              the original), so it also serves as the inverse.
*              Implemented as a constant-time butterfly network:
*              - s < 64:  bit-parallel shift + mask within each word
*              - s >= 64: word-block XOR
*
* Arguments:   - poly_f1_u64 *f: polynomial to transform (in-place)
**************************************************/
static void f1u64_translate(poly_f1_u64 *f)
{
    unsigned int s, w, ws, i, j;
    for (s = 1; s < F1_N; s <<= 1) {
        if (s < 64) {
            uint64_t mask = ~0ULL / ((1ULL << s) + 1);
            for (w = 0; w < F1_N_WORDS; w++)
                f->w[w] ^= (f->w[w] >> s) & mask;
        } else {
            ws = s >> 6;
            for (i = 0; i < F1_N_WORDS; i += 2 * ws)
                for (j = 0; j < ws; j++)
                    f->w[i + j] ^= f->w[i + ws + j];
        }
    }
}

/*************************************************
* Name:        sq2_64
*
* Description: Square a 32-bit polynomial in F2[X]: insert a zero
*              after each coefficient.  Single BMI2 pdep instruction.
*
* Arguments:   - uint32_t x: 32 input bits
*
* Returns:     64-bit result with a 0 after every input bit
**************************************************/
static inline uint64_t sq2_64(uint32_t x)
{
    return _pdep_u64(x, 0x5555555555555555ULL);
}

/*************************************************
* Name:        f1u64_square_trunc
*
* Description: Compute a^2 in F2[X], keeping only the first trunc terms.
*              Uses BMI2 pdep to insert a 0 after each coefficient.
*              Processes 64-bit input words; each word expands to two
*              output words (bits at even positions).
*
* Arguments:   - poly_f1_u64 *r:    output polynomial (mod X^trunc)
*              - const poly_f1_u64 *a: input polynomial
*              - int prec:           number of valid bits in a
*              - int trunc:          number of terms to keep
**************************************************/
static void f1u64_square_trunc(poly_f1_u64 *r, const poly_f1_u64 *a,
                                int prec, int trunc)
{
    int k, nwords, lim, rem;
    uint64_t bits;

    memset(r, 0, sizeof(*r));
    lim = prec < trunc / 2 ? prec : trunc / 2;
    nwords = (lim + 63) / 64;

    for (k = 0; k < nwords; k++) {
        bits = a->w[k];
        rem = lim - k * 64;
        if (rem < 64)
            bits &= (1ULL << rem) - 1;
        r->w[2*k]     = sq2_64((uint32_t)bits);
        r->w[2*k + 1] = sq2_64((uint32_t)(bits >> 32));
    }
}

/*************************************************
* Name:        f1u64_mul_trunc
*
* Description: Multiply a * b in F2[X], truncated to the first trunc
*              terms (i.e., mod X^trunc).  Used by the Newton iteration.
*              Uses PCLMULQDQ for 64×64-bit carry-less word products
*              with schoolbook accumulation, replacing the previous
*              bit-serial loop.
*
* Arguments:   - poly_f1_u64 *r:    output polynomial (mod X^trunc)
*              - const poly_f1_u64 *a: input polynomial a
*              - const poly_f1_u64 *b: input polynomial b
*              - int trunc:          number of terms to keep
**************************************************/
static void f1u64_mul_trunc(poly_f1_u64 *r, const poly_f1_u64 *a,
                             const poly_f1_u64 *b, int trunc)
{
    int tw = (trunc + 63) >> 6;
    uint64_t prod[2 * F1_N_WORDS];
    int i, j;
    uint64_t ai;

    memset(prod, 0, sizeof(prod));

    /* Schoolbook: a[i] * b[j] → prod[i+j] via PCLMULQDQ */
    for (i = 0; i < F1_N_WORDS; i++) {
        ai = a->w[i];
        if (!ai) continue;
        for (j = 0; j < F1_N_WORDS; j++) {
            if (i + j >= tw) break;
            uint64_t bj = b->w[j];
            if (!bj) continue;

            __m128i va = _mm_set1_epi64x((int64_t)ai);
            __m128i vb = _mm_set1_epi64x((int64_t)bj);
            __m128i p  = _mm_clmulepi64_si128(va, vb, 0x00);

            prod[i + j]     ^= _mm_cvtsi128_si64(p);
            prod[i + j + 1] ^= _mm_extract_epi64(p, 1);
        }
    }

    memcpy(r->w, prod, tw * sizeof(uint64_t));
    if (trunc & 63)
        r->w[tw - 1] &= (1ULL << (trunc & 63)) - 1;
}

/*************************************************
* Name:        f1u64_newton_inv
*
* Description: Compute the inverse of f_hat in F2[[X]] up to F1_N terms
*              using Newton iteration.  Requires f_hat(0) = 1 (guaranteed
*              by the translate step).  Each iteration doubles the
*              precision: g <- f_hat * g^2 mod X^{2l}.
*              In F2, g' = f_hat * g^2 mod X^{2l} gives the inverse
*              because g = f_hat^{-1} + eps*X^l => g' = f_hat^{-1} + f_hat*eps^2*X^{2l}.
*
* Arguments:   - poly_f1_u64 *g:       output inverse (truncated to F1_N)
*              - const poly_f1_u64 *fhat: input polynomial (after translate)
**************************************************/
static void f1u64_newton_inv(poly_f1_u64 *g, const poly_f1_u64 *fhat)
{
    poly_f1_u64 gsq;
    int prec, np;

    memset(g, 0, sizeof(*g));
    g->w[0] = 1;  /* Initial approximation: g = 1 (since f_hat(0)=1) */

    prec = 1;  /* Current precision */
    while (prec < F1_N) {
        np = prec << 1;          /* Double the target precision */
        if (np > F1_N) np = F1_N; /* Don't overshoot on the final round */

        /* g^2 mod X^{np}; input only needs prec bits (since squaring doubles precision) */
        f1u64_square_trunc(&gsq, g, prec, np);
        /* g <- f_hat * g^2 mod X^{np} */
        f1u64_mul_trunc(g, fhat, &gsq, np);

        prec = np;  /* Precision has been doubled */
    }
}

/*************************************************
* Name:        f1_pack
*
* Description: Pack f1 from a uint8_t array into a 64-bit word array.
*              Each coefficient contributes only its lowest bit,
*              achieving a 16x space reduction.
*
* Arguments:   - poly_f1_u64 *dst:  pointer to output bit-packed polynomial
*              - const poly_f1 *src: pointer to input uint8_t polynomial
**************************************************/
static void f1_pack(poly_f1_u64 *b, const poly_f1 *a)
{
    unsigned int i, j;
    const __m256i one = _mm256_set1_epi8(1);

    for (i = 0; i < F1_N_WORDS; i++) {
        uint64_t word = 0;
        for (j = 0; j < 64; j += 32) {
            __m256i v   = _mm256_loadu_si256((const __m256i *)(a->coeffs + (i << 6) + j));
            __m256i cmp = _mm256_cmpeq_epi8(v, one);
            uint32_t m  = _mm256_movemask_epi8(cmp);
            word |= (uint64_t)m << j;
        }
        b->w[i] = word;
    }
}

/*************************************************
* Name:        f1_unpack
*
* Description: Unpack a 64-bit word array back into a uint8_t array.
*              Each coefficient is 0 or 1.
*
* Arguments:   - poly_f1 *dst:        pointer to output uint8_t polynomial
*              - const poly_f1_u64 *src: pointer to input bit-packed polynomial
**************************************************/
static void f1_unpack(poly_f1 *b, const poly_f1_u64 *a)
{
    for (unsigned int i = 0; i < F1_N_WORDS; i++) {
        for (int j = 0; j < 64; j += 8) {
            uint64_t expanded = _pdep_u64((uint8_t)(a->w[i] >> j), 0x0101010101010101ULL);
            memcpy(&b->coeffs[i * 64 + j], &expanded, 8);
        }
    }
}

/*************************************************
* Name:        poly_inv_in_F2
*
* Description: Compute the inverse of f in F2[x]/(x^n+1).
*              Returns 0 if f is not invertible (f1 is unchanged),
*              returns 1 and writes the inverse into f1 if invertible.
*
* Algorithm (corresponds to paper Algorithm 2-4):
*   1. Coefficient folding: use repetition-code structure to fold
*      f mod 2 down to n dimensions
*      - 128-bit: sum every 2 coefficients -> 256 dimensions
*      - 256/512-bit: sum every 4 coefficients -> 256/512 dimensions
*   2. Invertibility check: if sum(f[i]) = 0 mod 2, then (X+1)|f, not invertible
*   3. Translate: f(X) -> f(X+1), ensures constant term is 1
*   4. Newton iteration: compute inverse in F2[[X]]
*   5. Inverse translate: transform back to inverse of f(X)
*
* Arguments:   - poly_f1 *f1: output, inverse of f (also used as scratch buffer)
*              - const poly *f: input, original polynomial
**************************************************/
int poly_inv_in_F2(poly_f1 *f1, const poly *f)
{
    poly_f1_u64 bp, g;

    /* f mod x^n + 1 — vectorized: 32 output bytes per iteration */
#if REPETITIONS == 2
    {
        const __m256i one = _mm256_set1_epi16(1);
        for (unsigned int i = 0; i < N / 2; i += 32) {
            __m256i a0 = _mm256_load_si256((const __m256i *)&f->coeffs[i]);
            __m256i a1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + 16]);
            __m256i b0 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/2]);
            __m256i b1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/2 + 16]);
            __m256i s0 = _mm256_and_si256(_mm256_add_epi16(a0, b0), one);
            __m256i s1 = _mm256_and_si256(_mm256_add_epi16(a1, b1), one);
            __m128i p0 = _mm_packus_epi16(_mm256_castsi256_si128(s0),
                                         _mm256_extracti128_si256(s0, 1));
            __m128i p1 = _mm_packus_epi16(_mm256_castsi256_si128(s1),
                                         _mm256_extracti128_si256(s1, 1));
            _mm256_store_si256((__m256i *)&f1->coeffs[i],
                _mm256_insertf128_si256(_mm256_castsi128_si256(p0), p1, 1));
        }
    }
#elif REPETITIONS == 4
    {
        const __m256i one = _mm256_set1_epi16(1);
        for (unsigned int i = 0; i < N / 4; i += 32) {
            __m256i a0 = _mm256_load_si256((const __m256i *)&f->coeffs[i]);
            __m256i a1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + 16]);
            __m256i b0 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/4]);
            __m256i b1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/4 + 16]);
            __m256i c0 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/2]);
            __m256i c1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + N/2 + 16]);
            __m256i d0 = _mm256_load_si256((const __m256i *)&f->coeffs[i + 3*N/4]);
            __m256i d1 = _mm256_load_si256((const __m256i *)&f->coeffs[i + 3*N/4 + 16]);
            __m256i s0 = _mm256_and_si256(
                _mm256_add_epi16(_mm256_add_epi16(a0, b0), _mm256_add_epi16(c0, d0)), one);
            __m256i s1 = _mm256_and_si256(
                _mm256_add_epi16(_mm256_add_epi16(a1, b1), _mm256_add_epi16(c1, d1)), one);
            __m128i p0 = _mm_packus_epi16(_mm256_castsi256_si128(s0),
                                         _mm256_extracti128_si256(s0, 1));
            __m128i p1 = _mm_packus_epi16(_mm256_castsi256_si128(s1),
                                         _mm256_extracti128_si256(s1, 1));
            _mm256_store_si256((__m256i *)&f1->coeffs[i],
                _mm256_insertf128_si256(_mm256_castsi128_si256(p0), p1, 1));
        }
    }
#endif

    f1_pack(&bp, f1);  /* Pack into bit representation */

    /* Step 2: invertibility check - f(1) must be 1 mod 2 */
    if (!f1u64_eval_one(&bp))
        return 0;

    /* Step 3: X -> X+1 substitution, ensures constant term is 1 for Newton convergence */
    f1u64_translate(&bp);

    /* Step 4: Newton iteration for inversion */
    f1u64_newton_inv(&g, &bp);

    /* Step 5: X+1 -> X inverse substitution */
    f1u64_translate(&g);

    f1_unpack(f1, &g);  /* Unpack back to uint8_t array */

    return 1;
}

/*************************************************
* Name:        f1u64_mul_ring
*
* Description: Multiply in F2[X]/(X^n+1).  Uses PCLMULQDQ for
*              64x64-bit carry-less word products with schoolbook
*              accumulation, then folds the upper half modulo X^n+1.
*
* Arguments:   - poly_f1_u64 *r:    output polynomial
*              - const poly_f1_u64 *a: input polynomial a
*              - const poly_f1_u64 *b: input polynomial b
**************************************************/
static void f1u64_mul_ring(poly_f1_u64 *r,
                           const poly_f1_u64 *a,
                           const poly_f1_u64 *b)
{
    uint64_t prod[2 * F1_N_WORDS] = {0};
    int i, j;

    /* Step 1: schoolbook multiplication in F2[X] using PCLMULQDQ */
    for (i = 0; i < F1_N_WORDS; i++) {
        uint64_t ai = a->w[i];
        for (j = 0; j < F1_N_WORDS; j++) {
            uint64_t bj = b->w[j];
            __m128i va = _mm_set1_epi64x((int64_t)ai);
            __m128i vb = _mm_set1_epi64x((int64_t)bj);
            __m128i p  = _mm_clmulepi64_si128(va, vb, 0x00);
            prod[i + j]     ^= _mm_cvtsi128_si64(p);
            prod[i + j + 1] ^= (uint64_t)_mm_extract_epi64(p, 1);
        }
    }

    /* Step 2: reduction modulo (X^n+1) — fold upper half onto lower (XOR) */
    for (i = 0; i < F1_N_WORDS; i++)
        r->w[i] = prod[i] ^ prod[i + F1_N_WORDS];
}

/*************************************************
* Name:        poly_decode_to_msg
*
* Description: Decode message from ciphertext polynomial c, using the
*              secret polynomial f1 (the inverse of f in F2).
*
* Decoding procedure (corresponds to paper Algorithm 5: Decode):
*   1. Center ciphertext: shift reference point from (q+1)/2 to 0
*   2. Block threshold decision: use repetition-code redundancy to make
*      D2/D4 lattice closest-vector decisions per block of N/n coefficients,
*      recovering (f*m) in F2[X]/(X^n+1)
*   3. Multiply by f^{-1}: compute m = (f*m) * f^{-1} in F2[X]/(X^n+1)
*   4. Extract bytes: pull message bytes from the packed bit representation
*
* Arguments:   - uint8_t msg[]: output message byte array
*              - poly *c: input ciphertext polynomial (in Z_q[X]/(X^N+1))
*              - poly_f1 *f1: inverse of f in F2[X]/(X^n+1) (precomputed)
**************************************************/
void poly_decode_to_msg(uint8_t msg[KEM_MSGBYTES], poly *c, poly_f1 *f1) {
#if REPETITIONS == 2
    /*
     * FLIT128: N=512, n=256, q=769
     * Each 2 coefficients encode 1 message bit
     * Threshold: (q-1)/2 = 384 (the D2 lattice decision boundary)
     */
    unsigned int i;
    poly_f1 mf1;
    poly_f1_u64 mf1_packed, f1_packed, m_packed;

    /*
     * Step 1: center the ciphertext
     * Original ciphertext: c = h*r + e + (q+1)/2 * Encode(m)
     * After subtracting (q+1)/2: coefficients encoding 1 are near 0,
     * coefficients encoding 0 are near +/- (q+1)/2
     * poly_sub_halfq centers each coefficient to roughly [-q/2, q/2)
     */
    poly_sub_halfq(c);

    /*
     * Step 2: D2 lattice threshold decision
     * For each i = 0..255, take c[i] and c[i+256]:
     *   if |c[i]| + |c[i+256]| < (q-1)/2 = 384 -> decode as 1 (near 0)
     *   otherwise -> decode as 0 (near +/- q/2)
     *
     * Absolute value trick: (x ^ -(x>>15)) + (x>>15)
     *   if x >= 0: (x ^ 0) + 0 = x
     *   if x < 0:  (x ^ -1) + 1 = ~x + 1 = -x (two's complement abs)
     * This is faster and constant-time compared to branching.
     *
     * Comparison trick: (sum - threshold) >> 15
     *   if sum < threshold -> underflow -> sign bit = 1 -> result = 1
     *   if sum >= threshold -> non-negative -> sign bit = 0 -> result = 0
     * Uses unsigned underflow for comparison, avoiding branches.
     */
    {
        const __m256i threshold = _mm256_set1_epi16((int16_t)((Q - 1) / 2));
        const __m256i one = _mm256_set1_epi16(1);

        /* Process 32 pairs at a time to use _mm256_packus_epi16 efficiently */
        for (i = 0; i < N / 2; i += 32) {
            __m256i u0_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i));
            __m256i u0_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + 16));
            __m256i u1_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 2));
            __m256i u1_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 2 + 16));

            /* abs(x) = (x ^ (x>>15)) + (x>>15) */
            __m256i s0_lo = _mm256_srai_epi16(u0_lo, 15);
            __m256i s0_hi = _mm256_srai_epi16(u0_hi, 15);
            __m256i s1_lo = _mm256_srai_epi16(u1_lo, 15);
            __m256i s1_hi = _mm256_srai_epi16(u1_hi, 15);
            __m256i a0_lo = _mm256_sub_epi16(_mm256_xor_si256(u0_lo, s0_lo), s0_lo);
            __m256i a0_hi = _mm256_sub_epi16(_mm256_xor_si256(u0_hi, s0_hi), s0_hi);
            __m256i a1_lo = _mm256_sub_epi16(_mm256_xor_si256(u1_lo, s1_lo), s1_lo);
            __m256i a1_hi = _mm256_sub_epi16(_mm256_xor_si256(u1_hi, s1_hi), s1_hi);

            __m256i sum_lo = _mm256_add_epi16(a0_lo, a1_lo);
            __m256i sum_hi = _mm256_add_epi16(a0_hi, a1_hi);
            __m256i res_lo = _mm256_and_si256(_mm256_cmpgt_epi16(threshold, sum_lo), one);
            __m256i res_hi = _mm256_and_si256(_mm256_cmpgt_epi16(threshold, sum_hi), one);

            /* pack 2 x 16 int16_t → 32 x uint8_t */
            __m256i packed = _mm256_packus_epi16(res_lo, res_hi);
            /* In-lane pack gives [lo0..lo7, hi0..hi7, lo8..lo15, hi8..hi15]; reorder */
            __m256i perm = _mm256_permute4x64_epi64(packed, 0xD8); /* 0,2,1,3 → correct order */
            _mm256_storeu_si256((__m256i *)(mf1.coeffs + i), perm);
        }
    }
    /* At this point mf1 holds (f*m) in F2[X]/(X^n+1) */

    f1_pack(&mf1_packed, &mf1);
    f1_pack(&f1_packed, f1);
    f1u64_mul_ring(&m_packed, &mf1_packed, &f1_packed);

    for (i = 0; i < KEM_MSGBYTES; i++)
        msg[i] = (uint8_t)(m_packed.w[i >> 3] >> ((i & 7) << 3));

#elif REPETITIONS == 4
    /*
     * FLIT256: N=1024, n=256, q=769
     * FLIT512: N=2048, n=512, q=3329
     * Each 4 coefficients encode 1 message bit (D4 lattice decoding)
     * Threshold: q-1 = 768 (FLIT256) or 3328 (FLIT512)
     *
     * Same principle as 128-bit, but block size is 4 instead of 2,
     * Decision: |c[i]|+|c[i+n]|+|c[i+2n]|+|c[i+3n]| < 2*(q-1)/2 = q-1
     */
    unsigned int i;
    poly_f1 mf1;
    poly_f1_u64 mf1_packed, f1_packed, m_packed;

    poly_sub_halfq(c);

    {
        const __m256i threshold = _mm256_set1_epi16((int16_t)(Q - 1));
        const __m256i one = _mm256_set1_epi16(1);

        for (i = 0; i < N / 4; i += 32) {
            __m256i u0_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i));
            __m256i u0_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + 16));
            __m256i u1_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 4));
            __m256i u1_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 4 + 16));
            __m256i u2_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 2));
            __m256i u2_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + N / 2 + 16));
            __m256i u3_lo = _mm256_load_si256((const __m256i *)(c->coeffs + i + 3 * N / 4));
            __m256i u3_hi = _mm256_load_si256((const __m256i *)(c->coeffs + i + 3 * N / 4 + 16));

            /* compute abs_lo */
            __m256i s0 = _mm256_srai_epi16(u0_lo, 15);
            __m256i s1 = _mm256_srai_epi16(u1_lo, 15);
            __m256i s2 = _mm256_srai_epi16(u2_lo, 15);
            __m256i s3 = _mm256_srai_epi16(u3_lo, 15);
            __m256i sum_lo = _mm256_add_epi16(u0_lo, u0_lo); /* dummy, will be overwritten */
            sum_lo = _mm256_sub_epi16(_mm256_xor_si256(u0_lo, s0), s0);
            __m256i tmp = _mm256_sub_epi16(_mm256_xor_si256(u1_lo, s1), s1);
            sum_lo = _mm256_add_epi16(sum_lo, tmp);
            tmp = _mm256_sub_epi16(_mm256_xor_si256(u2_lo, s2), s2);
            sum_lo = _mm256_add_epi16(sum_lo, tmp);
            tmp = _mm256_sub_epi16(_mm256_xor_si256(u3_lo, s3), s3);
            sum_lo = _mm256_add_epi16(sum_lo, tmp);

            /* compute abs_hi */
            s0 = _mm256_srai_epi16(u0_hi, 15);
            s1 = _mm256_srai_epi16(u1_hi, 15);
            s2 = _mm256_srai_epi16(u2_hi, 15);
            s3 = _mm256_srai_epi16(u3_hi, 15);
            __m256i sum_hi = _mm256_sub_epi16(_mm256_xor_si256(u0_hi, s0), s0);
            tmp = _mm256_sub_epi16(_mm256_xor_si256(u1_hi, s1), s1);
            sum_hi = _mm256_add_epi16(sum_hi, tmp);
            tmp = _mm256_sub_epi16(_mm256_xor_si256(u2_hi, s2), s2);
            sum_hi = _mm256_add_epi16(sum_hi, tmp);
            tmp = _mm256_sub_epi16(_mm256_xor_si256(u3_hi, s3), s3);
            sum_hi = _mm256_add_epi16(sum_hi, tmp);

            __m256i res_lo = _mm256_and_si256(_mm256_cmpgt_epi16(threshold, sum_lo), one);
            __m256i res_hi = _mm256_and_si256(_mm256_cmpgt_epi16(threshold, sum_hi), one);
            __m256i packed = _mm256_packus_epi16(res_lo, res_hi);
            __m256i perm   = _mm256_permute4x64_epi64(packed, 0xD8);
            _mm256_storeu_si256((__m256i *)(mf1.coeffs + i), perm);
        }
    }

    f1_pack(&mf1_packed, &mf1);
    f1_pack(&f1_packed, f1);
    f1u64_mul_ring(&m_packed, &mf1_packed, &f1_packed);

    for (i = 0; i < KEM_MSGBYTES; i++)
        msg[i] = (uint8_t)(m_packed.w[i >> 3] >> ((i & 7) << 3));
#endif
}
