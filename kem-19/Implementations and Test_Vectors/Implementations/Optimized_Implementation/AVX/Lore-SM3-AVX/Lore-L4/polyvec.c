#include "polyvec.h"
#include "poly.h"
#include "symmetric.h"
#include <string.h> // for memset

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
  
  // NBLOCKS *must* be large enough to generate q_poly and t_poly in one go
  // with overwhelming probability.
  // q needs ~514 bytes, t needs ~266. Total ~780 bytes.
  // XOF_BLOCKBYTES = 168. 780/168 = 4.6. We'll use 5 blocks as a safe bet.
  const unsigned int NBLOCKS = 5;
  uint8_t buf[NBLOCKS * XOF_BLOCKBYTES];
  
  for (i = 0; i < LORE_K; i++) {
    for (j = 0; j < LORE_K; j++) {
      xof_state state;
      // 1. Set XOF state independently for each polynomial
      if (transposed) {
          xof_absorb(&state, seed, (uint8_t)j, (uint8_t)i);
      } else {
          xof_absorb(&state, seed, (uint8_t)i, (uint8_t)j);
      }
      // 2. Squeeze enough bytes at once
      xof_squeezeblocks(buf, NBLOCKS, &state);

      // 3. Generate q-part and t-part from the same buffer stream
      
      // -- Generate q-part --
      unsigned int ctr_q = 0;
      // Note: We use the rej_uniform_q function here.
      ctr_q = rej_uniform_q(a[i].vec[j].q_poly.coeffs, LORE_N, buf, NBLOCKS * XOF_BLOCKBYTES);
      // In the rare case the buffer is not enough, squeeze another block
      while(ctr_q < LORE_N) {
        xof_squeezeblocks(buf, 1, &state);
        ctr_q += rej_uniform_q(a[i].vec[j].q_poly.coeffs + ctr_q, LORE_N - ctr_q, buf, XOF_BLOCKBYTES);
      }

      // -- Generate t-part --
      unsigned int ctr_t = 0;
      // Requires the rej_uniform_t function
      ctr_t = rej_uniform_t(a[i].vec[j].t_poly.coeffs, LORE_N, buf, NBLOCKS * XOF_BLOCKBYTES);
      // In the rare case the buffer is not enough, squeeze another block
      while(ctr_t < LORE_N) {
        xof_squeezeblocks(buf, 1, &state);
        ctr_t += rej_uniform_t(a[i].vec[j].t_poly.coeffs + ctr_t, LORE_N - ctr_t, buf, XOF_BLOCKBYTES);
      }
    }
  }
}
