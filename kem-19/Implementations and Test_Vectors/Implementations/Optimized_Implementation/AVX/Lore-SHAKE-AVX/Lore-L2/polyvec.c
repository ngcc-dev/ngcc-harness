#include "polyvec.h"
#include "poly.h"
#include "reduce.h"
#include "symmetric.h"
#include <string.h> // for memset

#ifdef LORE_USE_AVX2_SHAKE
#include "simd/shake_avx2.h"
#include <immintrin.h>
#endif

/*************************************************
* Name:        polyvec_ntt
*
* Description: Apply NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *v: pointer to the polynomial vector
**************************************************/
void polyvec_ntt(polyvec *v) {
    for (int i = 0; i < LORE_K; i++) {
        poly_ntt(&v->vec[i]);
    }
}

/*************************************************
* Name:        polyvec_invntt_tomont
*
* Description: Apply inverse NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *v: pointer to the polynomial vector
**************************************************/
void polyvec_invntt_tomont(polyvec *v) {
    for (int i = 0; i < LORE_K; i++) {
        poly_invntt_tomont(&v->vec[i]);
    }
}

/*************************************************
* Name:        polyvec_pointwise_acc_montgomery
*
* Description: Pointwise multiply elements of a and b and accumulate into r
*
* Arguments:   - poly *r:          pointer to the output polynomial
* - const polyvec *a: pointer to the first polynomial vector
* - const polyvec *b: pointer to the second polynomial vector
**************************************************/
void polyvec_pointwise_acc_montgomery(poly *r, const polyvec *a, const polyvec *b) {
    poly t;
    poly_pointwise_montgomery(r, &a->vec[0], &b->vec[0]);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t, &a->vec[i], &b->vec[i]);
        poly_add(r, r, &t);
    }
}

/*************************************************
* Name:        polyvec_add
*
* Description: Add vectors of polynomials
*
* Arguments:   - polyvec *r:       pointer to the output polynomial vector
* - const polyvec *a: pointer to the first polynomial vector
* - const polyvec *b: pointer to the second polynomial vector
**************************************************/
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b) {
    for (int i = 0; i < LORE_K; i++) {
        poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

/*************************************************
* Name:        poly_crt_vec_add
*
* Description: Add two vectors of CRT polynomials
*
* Arguments:   - poly_crt_vec *r:       pointer to the output CRT polynomial vector
* - const poly_crt_vec *a: pointer to the first CRT polynomial vector
* - const poly_crt_vec *b: pointer to the second CRT polynomial vector
**************************************************/
void poly_crt_vec_add(poly_crt_vec *r, const poly_crt_vec *a, const poly_crt_vec *b) {
    for(int i=0; i<LORE_K; ++i)
        poly_crt_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/*************************************************
* Name:        poly_crt_vec_ntt
*
* Description: Apply NTT to the q-part of a vector of CRT polynomials
*
* Arguments:   - poly_crt_vec *v: pointer to the CRT polynomial vector
**************************************************/
void poly_crt_vec_ntt(poly_crt_vec *v) {
    for (int i = 0; i < LORE_K; ++i)
        poly_ntt(&v->vec[i].q_poly);
}

/*************************************************
* Name:        poly_crt_vec_invntt_tomont
*
* Description: Apply inverse NTT to the q-part of a vector of CRT polynomials
*
* Arguments:   - poly_crt_vec *v: pointer to the CRT polynomial vector
**************************************************/
void poly_crt_vec_invntt_tomont(poly_crt_vec *v) {
    for (int i = 0; i < LORE_K; ++i)
        poly_invntt_tomont(&v->vec[i].q_poly);
}

/*************************************************
* Name:        poly_crt_vec_pointwise_acc_montgomery
*
* Description: (Dense * Dense)
* Pointwise multiply a matrix row of CRT polynomials
* with a vector of CRT polynomials and accumulate.
* - q-part uses NTT.
* - t-part uses Karatsuba (poly_mul_modt).
*
* Arguments:   - poly_crt *r:                   pointer to the output CRT polynomial
* - const poly_crt_vec *a_row_dense: pointer to the dense matrix row
* - const poly_crt_vec *b_dense:     pointer to the dense vector
**************************************************/
void poly_crt_vec_pointwise_acc_montgomery(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_dense) {
    poly_crt t;

    /* q-part (NTT domain) */
    poly_pointwise_montgomery(&r->q_poly, &a_row_dense->vec[0].q_poly, &b_dense->vec[0].q_poly);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t.q_poly, &a_row_dense->vec[i].q_poly, &b_dense->vec[i].q_poly);
        poly_add(&r->q_poly, &r->q_poly, &t.q_poly);
    }

    /* t-part (normal domain): use Karatsuba convolution and mod t accumulation */
    poly t_prod;
    /* Calculate the first product and store directly in r->t_poly */
    poly_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_dense->vec[0].t_poly);

    for (int i = 1; i < LORE_K; i++) {
        poly_mul_modt(&t_prod, &a_row_dense->vec[i].t_poly, &b_dense->vec[i].t_poly);
        /* Calculate subsequent products and accumulate */
        for(int j=0; j<LORE_N; ++j) {
            int16_t sum = r->t_poly.coeffs[j] + t_prod.coeffs[j];
            
            /* Constant time reduction mapping to [-T/2, T/2) */
            sum = sum % LORE_T;
            sum += (sum >> 15) & LORE_T; 
            sum -= ((LORE_T / 2 - sum) >> 15) & LORE_T;

            r->t_poly.coeffs[j] = sum;
        }
    }
}


/*************************************************
* Name:        poly_crt_vec_pointwise_acc_montgomery_sparse
*
* Description: (Dense * Sparse)
* Pointwise multiply a matrix row of CRT polynomials (dense)
* with a vector of CRT polynomials (sparse) and accumulate.
* - q-part uses NTT.
* - t-part uses sparse multiplication.
*
* Arguments:   - poly_crt *r:                         pointer to the output CRT polynomial
* - const poly_crt_vec *a_row_dense:       pointer to the dense matrix row
* - const poly_crt_vec *b_crt_with_ntt_q:  pointer to the sparse vector (with NTT'd q-part)
* - const poly_sparse *b_t_poly_sparse_vec: pointer to the sparse t-part of the vector
**************************************************/
void poly_crt_vec_pointwise_acc_montgomery_sparse(poly_crt *r, const poly_crt_vec *a_row_dense, const poly_crt_vec *b_crt_with_ntt_q, const poly_sparse *b_t_poly_sparse_vec) {
    poly_crt t_q_only;

    /* q-part (in NTT domain) */
    poly_pointwise_montgomery(&r->q_poly, &a_row_dense->vec[0].q_poly, &b_crt_with_ntt_q->vec[0].q_poly);
    for (int i = 1; i < LORE_K; i++) {
        poly_pointwise_montgomery(&t_q_only.q_poly, &a_row_dense->vec[i].q_poly, &b_crt_with_ntt_q->vec[i].q_poly);
        poly_add(&r->q_poly, &r->q_poly, &t_q_only.q_poly);
    }

    // Step 2: t-part (in standard domain) - use conditional compilation to select the optimal algorithm
#if LORE_LEVEL == 1
    // Level 1: Sparse polynomial multiplication.
    poly_sparse_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_t_poly_sparse_vec[0]);
    for (int i = 1; i < LORE_K; i++) {
        poly t_temp_prod;
        poly_sparse_mul_modt(&t_temp_prod, &a_row_dense->vec[i].t_poly, &b_t_poly_sparse_vec[i]);
        poly_add_modt(&r->t_poly, &r->t_poly, &t_temp_prod);
    }
#else
    // Levels 2, 3, 4: Dense polynomial multiplication via Toom-Cook.
    poly b_t_dense_temp; // For temporarily storing the converted dense polynomial

    // Calculate a_row[0] * s[0]
    poly_from_sparse(&b_t_dense_temp, &b_t_poly_sparse_vec[0]);
    poly_mul_modt(&r->t_poly, &a_row_dense->vec[0].t_poly, &b_t_dense_temp);

    // Accumulate subsequent products a_row[i] * s[i]
    for (int i = 1; i < LORE_K; i++) {
        poly t_temp_prod_local;
        poly_from_sparse(&b_t_dense_temp, &b_t_poly_sparse_vec[i]);
        poly_mul_modt(&t_temp_prod_local, &a_row_dense->vec[i].t_poly, &b_t_dense_temp);
        poly_add_modt(&r->t_poly, &r->t_poly, &t_temp_prod_local);
    }
#endif
}


/*************************************************
* Name:        gen_matrix_std
*
* Description: Generates a matrix of CRT polynomials in standard domain.
*
* Arguments:   - poly_crt_vec a[LORE_K]: pointer to the output matrix
* - const unsigned char *seed: pointer to the seed
**************************************************/
void gen_matrix_std(poly_crt_vec a[LORE_K], const unsigned char *seed)
{
    gen_matrix_ntt(a, seed, 0); // Add 0 to indicate non-transposed
}

#ifdef LORE_USE_AVX2_SHAKE
/* 16-lane Barrett reduction mod LORE_Q=257.
 * Equivalent to two iterations of scalar barrett_reduce on each int16 lane. */
static inline __m256i reduce_q_from_u16(__m256i x)
{
    const __m256i mask = _mm256_set1_epi16(0x00FF);
    __m256i t = _mm256_sub_epi16(_mm256_and_si256(x, mask),
                                  _mm256_srai_epi16(x, 8));
    t = _mm256_sub_epi16(_mm256_and_si256(t, mask),
                          _mm256_srai_epi16(t, 8));
    return t;
}

/* Rejection sampling for q-coefficients, 16 uint16 at a time.
 * Matches scalar rej_uniform_q output order: when a 0xFFFF is found in a
 * 32-byte chunk (prob ≈ 1/65536 per value), falls back to scalar for that
 * chunk only. */
static unsigned int rej_uniform_q_avx2(int16_t *r,
                                        unsigned int len,
                                        const uint8_t *buf,
                                        unsigned int buflen)
{
    unsigned int ctr = 0, pos = 0;
    const __m256i reject_val = _mm256_set1_epi16((short)(int16_t)-1); /* 0xFFFF */

    while (ctr + 16 <= len && pos + 32 <= buflen) {
        __m256i vals = _mm256_loadu_si256((const __m256i *)(const void *)(buf + pos));
        __m256i cmp  = _mm256_cmpeq_epi16(vals, reject_val);
        int     mask = _mm256_movemask_epi8(cmp);

        if (mask == 0) {
            /* No 0xFFFF in this 16-value chunk */
            __m256i reduced = reduce_q_from_u16(vals);
            _mm256_storeu_si256((__m256i *)(void *)(r + ctr), reduced);
            ctr += 16;
        } else {
            /* Rare: scalar fallback to maintain exact output order */
            for (unsigned int k = 0; k < 16 && ctr < len; k++) {
                uint16_t val;
                __builtin_memcpy(&val, buf + pos + k * 2, sizeof(val));
                if (val < 0xFFFF) {
                    r[ctr++] = barrett_reduce((int16_t)val);
                }
            }
        }
        pos += 32;
    }
    /* Scalar tail */
    while (ctr < len && pos + 2 <= buflen) {
        uint16_t val;
        __builtin_memcpy(&val, buf + pos, sizeof(val));
        pos += 2;
        if (val < 0xFFFF) {
            r[ctr++] = barrett_reduce((int16_t)val);
        }
    }
    return ctr;
}

/* Rejection sampling for t-coefficients, 16 uint8 at a time.
 * For LORE_T==2 or LORE_T==4 the rejection probability is 0 (all bytes pass),
 * so this is a pure transform with no branch-on-rejection in the hot path. */
static unsigned int rej_uniform_t_avx2(int16_t *r,
                                        unsigned int len,
                                        const uint8_t *buf,
                                        unsigned int buflen)
{
    unsigned int ctr = 0, pos = 0;
    const uint16_t t_limit = (uint16_t)(((unsigned)0x100 / LORE_T) * LORE_T);

#if LORE_T == 2
    const __m256i one_mask = _mm256_set1_epi16(1);
    while (ctr + 16 <= len && pos + 16 <= buflen) {
        __m128i b8  = _mm_loadu_si128((const __m128i *)(const void *)(buf + pos));
        __m256i b16 = _mm256_cvtepu8_epi16(b8);
        b16 = _mm256_and_si256(b16, one_mask);
        _mm256_storeu_si256((__m256i *)(void *)(r + ctr), b16);
        ctr += 16; pos += 16;
    }
#elif LORE_T == 4
    /* val = byte & 3; sign fix: if val==3 subtract 4 to get -1 */
    const __m256i mask3  = _mm256_set1_epi16(3);
    const __m256i one_v  = _mm256_set1_epi16(1);
    while (ctr + 16 <= len && pos + 16 <= buflen) {
        __m128i b8   = _mm_loadu_si128((const __m128i *)(const void *)(buf + pos));
        __m256i vals = _mm256_cvtepu8_epi16(b8);
        vals = _mm256_and_si256(vals, mask3);
        __m256i vsh1 = _mm256_srli_epi16(vals, 1);
        __m256i vb0  = _mm256_and_si256(vals, one_v);
        __m256i adj  = _mm256_slli_epi16(_mm256_and_si256(vsh1, vb0), 2);
        vals = _mm256_sub_epi16(vals, adj);
        _mm256_storeu_si256((__m256i *)(void *)(r + ctr), vals);
        ctr += 16; pos += 16;
    }
#endif
    /* Scalar tail */
    while (ctr < len && pos < buflen) {
        if (buf[pos] < t_limit) {
            int16_t tc = (int16_t)(buf[pos] & (LORE_T - 1));
            if (LORE_T == 4)
                tc -= (int16_t)(((tc >> 1) & (tc & 1)) << 2);
            r[ctr++] = tc;
        }
        pos++;
    }
    return ctr;
}
#endif /* LORE_USE_AVX2_SHAKE */

static void gen_matrix_entry_scalar(poly_crt_vec a[LORE_K],
                                    const unsigned char *seed,
                                    int transposed,
                                    unsigned int i,
                                    unsigned int j)
{
  const unsigned int NBLOCKS = 5;
  const size_t BUF_SIZE = NBLOCKS * SHAKE128_RATE;
  keccak_state state;
  uint8_t buf[5 * SHAKE128_RATE];

  if (transposed) {
      xof_absorb(&state, seed, (uint8_t)j, (uint8_t)i);
  } else {
      xof_absorb(&state, seed, (uint8_t)i, (uint8_t)j);
  }

  xof_squeezeblocks(buf, NBLOCKS, &state);

  unsigned int ctr_q = rej_uniform_q(a[i].vec[j].q_poly.coeffs, LORE_N, buf, BUF_SIZE);
  while(ctr_q < LORE_N) {
    xof_squeezeblocks(buf, 1, &state);
    ctr_q += rej_uniform_q(a[i].vec[j].q_poly.coeffs + ctr_q, LORE_N - ctr_q, buf, SHAKE128_RATE);
  }

  unsigned int ctr_t = rej_uniform_t(a[i].vec[j].t_poly.coeffs, LORE_N, buf, BUF_SIZE);
  while(ctr_t < LORE_N) {
    xof_squeezeblocks(buf, 1, &state);
    ctr_t += rej_uniform_t(a[i].vec[j].t_poly.coeffs + ctr_t, LORE_N - ctr_t, buf, SHAKE128_RATE);
  }
}


/*************************************************
* Name:        gen_matrix_ntt
*
* Description: Generates a matrix of CRT polynomials with q-parts in NTT domain.
*
* Arguments:   - poly_crt_vec a[LORE_K]: pointer to the output matrix
* - const unsigned char *seed: pointer to the seed
* - int transposed: flag to indicate if the matrix is transposed
**************************************************/
void gen_matrix_ntt(poly_crt_vec a[LORE_K], const unsigned char *seed, int transposed)
{
  unsigned int i, j;

#ifdef LORE_USE_AVX2_SHAKE
  /* Compute how many SHAKE128 blocks are needed to fill q with overwhelming
   * probability.  For N=512: ceil(1024/168)=7 blocks.  For N=768: 10 blocks.
   * INITIAL_BLOCKS is the count passed to the initial x4 squeeze; if q is not
   * complete after INITIAL_BLOCKS blocks (extremely rare), we fall through to
   * the per-block loop using the pre-squeezed remaining blocks. */
  const unsigned int INITIAL_BLOCKS = 5;
  const unsigned int Q_BLOCKS =
      (2u * LORE_N + SHAKE128_RATE - 1u) / SHAKE128_RATE;
  const unsigned int total = LORE_K * LORE_K;

  /* Compile-time buffer size: enough for Q_BLOCKS blocks per lane. */
#define LORE_GENMTX_BUF ((((2 * 768 + 167) / 168) * 168))

  unsigned int base;
  for (base = 0; base + 4 <= total; base += 4) {
    uint8_t extseed[4][LORE_SYMBYTES + 2];
    uint8_t buf4[4][LORE_GENMTX_BUF];

    for (unsigned int lane = 0; lane < 4; lane++) {
      unsigned int idx = base + lane;
      unsigned int row = idx / LORE_K;
      unsigned int col = idx % LORE_K;
      memcpy(extseed[lane], seed, LORE_SYMBYTES);
      extseed[lane][LORE_SYMBYTES]     = transposed ? (uint8_t)col : (uint8_t)row;
      extseed[lane][LORE_SYMBYTES + 1] = transposed ? (uint8_t)row : (uint8_t)col;
    }

    lore_shake128x4_absorb_once_squeezeblocks(
        buf4[0], buf4[1], buf4[2], buf4[3],
        Q_BLOCKS,
        extseed[0], extseed[1], extseed[2], extseed[3],
        sizeof(extseed[0]));

    for (unsigned int lane = 0; lane < 4; lane++) {
      unsigned int idx = base + lane;
      unsigned int row = idx / LORE_K;
      unsigned int col = idx % LORE_K;

      /* t_head tracks the SHAKE buffer position used for t-sampling.
       * Semantics mirror the scalar loop: t reads from the last block
       * that was consumed for q, then wraps to initial blocks 1..4. */
      const uint8_t *t_head = buf4[lane];

      unsigned int ctr_q = rej_uniform_q_avx2(
          a[row].vec[col].q_poly.coeffs, LORE_N,
          buf4[lane], INITIAL_BLOCKS * SHAKE128_RATE);

      for (unsigned int block = INITIAL_BLOCKS;
           ctr_q < LORE_N && block < Q_BLOCKS; block++) {
        const uint8_t *qbuf = buf4[lane] + block * SHAKE128_RATE;
        t_head = qbuf;
        ctr_q += rej_uniform_q_avx2(
            a[row].vec[col].q_poly.coeffs + ctr_q,
            LORE_N - ctr_q, qbuf, SHAKE128_RATE);
      }

      /* Sample t-coefficients (0 % rejection for T=2/4). */
      unsigned int ctr_t = rej_uniform_t_avx2(
          a[row].vec[col].t_poly.coeffs, LORE_N,
          t_head, SHAKE128_RATE);

      if (ctr_t < LORE_N) {
        ctr_t += rej_uniform_t_avx2(
            a[row].vec[col].t_poly.coeffs + ctr_t,
            LORE_N - ctr_t,
            buf4[lane] + SHAKE128_RATE,
            (INITIAL_BLOCKS - 1) * SHAKE128_RATE);
      }
    }
  }
#undef LORE_GENMTX_BUF

  /* Scalar tail for remaining entries (at most 3). */
  for (unsigned int idx = base; idx < total; idx++) {
    gen_matrix_entry_scalar(a, seed, transposed, idx / LORE_K, idx % LORE_K);
  }

  _mm256_zeroupper();
  return;
#endif

  for (i = 0; i < LORE_K; i++) {
    for (j = 0; j < LORE_K; j++) {
      gen_matrix_entry_scalar(a, seed, transposed, i, j);
    }
  }
}
