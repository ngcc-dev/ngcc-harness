/**
 * \file rbc_poly.c
 * \brief Implementation of rbc_poly.h
 */

#include "rbc_79.h"
#include "rbc_vec.h"
#include "rbc_poly.h"




/**
 * \fn void rbc_poly_init(rbc_poly* p, int32_t degree)
 * \brief This function allocates the memory for a rbc_poly element.
 *
 * \param[out] p Pointer to the allocated rbc_poly
 * \param[in] degree Degree of the polynomial
 */
void rbc_poly_init(rbc_poly* p, int32_t degree) {
  *p = malloc(sizeof(rbc_poly_struct));
  if(p == NULL) exit(EXIT_FAILURE);
  rbc_vec_init(&((*p)->v), degree + 1);
  (*p)->max_degree = degree;
  (*p)->degree = degree;
}




/**
 * \fn void rbc_poly_clear(rbc_poly p)
 * \brief This function clears the memory allocated for a rbc_poly element.
 *
 * \param p Polynomial
 */
void rbc_poly_clear(rbc_poly p) {
  rbc_vec_clear(p->v);
  free(p);
  p = NULL;
}





/**
 * \fn void rbc_poly_sparse_init(rbc_poly_sparse* p, uint32_t coeff_nb, uint32_t *coeffs)
 * \brief This function allocates the memory for a rbc_poly_sparse element.
 *
 * \param p Pointer to the allocated rbc_poly_sparse
 * \param[in] coeff_nb Number of coefficients
 * \param[in] coeffs Coefficients
 */
void rbc_poly_sparse_init(rbc_poly_sparse* p, uint32_t coeff_nb, uint32_t *coeffs) {
  *p = malloc(sizeof(rbc_poly_sparse_struct));
  if(p == NULL) exit(EXIT_FAILURE);
  (*p)->coeffs_nb = coeff_nb;
  (*p)->coeffs = malloc(coeff_nb * sizeof(uint32_t));
  if((*p)->coeffs == NULL) exit(EXIT_FAILURE);
  memcpy((*p)->coeffs, coeffs, coeff_nb * sizeof(uint32_t));
}




/**
 * \fn void rbc_poly_sparse_clear(rbc_poly_sparse p)
 * \brief This function clears the memory allocated for a rbc_poly_sparse element.
 *
 * \param p Polynomial
 */
void rbc_poly_sparse_clear(rbc_poly_sparse p) {
  free(p->coeffs);
  free(p);
}




/**
 * \fn void rbc_poly_set_zero(rbc_poly o, int32_t degree)
 * \brief This functions sets a polynomial to zero.
 *
 * \param[in] p Polynomial
 * \param[in] degree Degree of the polynomial
 */
void rbc_poly_set_zero(rbc_poly o, int32_t degree) {
  rbc_vec_set_zero(o->v, degree + 1);
}




/**
 * \fn void rbc_poly_set(rbc_poly o, const rbc_poly p)
 * \brief This function copies a polynomial into another one.
 *
 * \param[in] o Polynomial
 * \param[in] p Polynomial
 */
void rbc_poly_set(rbc_poly o, const rbc_poly p) {
  rbc_vec_set(o->v, p->v, o->max_degree + 1);
}




/**
 * \fn void rbc_poly_set_random(random_source* ctx, rbc_poly o, int32_t degree)
 * \brief This function sets a polynomial with random values using NGCC seed expander.
 *
 * \param[out] ctx random_source
 * \param[out] o Polynomial
 * \param[in] degree Degree of the polynomial
 */
void rbc_poly_set_random(random_source* ctx, rbc_poly o, int32_t degree) {
  rbc_vec_set_random(ctx, o->v, degree + 1);
}




/**
 * \fn void rbc_poly_set_random_from_xof(rbc_poly o, int32_t degree, void (*xof)(), const uint8_t *xof_input, uint32_t xof_size)
 * \brief This function sets a polynomial with random values using XOF.
 *
 * \param[out] o Polynomial
 * \param[in] degree Degree of the polynomial
 * \param[in] xof XOF procedure
 * \param[in] xof_input XOF input byte array
 * \param[in] xof_size XOF input byte array length of xof_input
 */
void rbc_poly_set_random_from_xof(rbc_poly o,
                                  int32_t degree,
                                  void (*xof)(uint8_t *, size_t, const uint8_t *, size_t),
                                  const uint8_t *xof_input,
                                  uint32_t xof_size) {
  rbc_vec_set_random_from_xof(o->v, degree + 1, xof, xof_input, xof_size);
}




/**
 * \fn void rbc_poly_set_random_full_rank(random_source* ctx, rbc_poly o, int32_t degree)
 * \brief This function sets a polynomial with random values using the NGCC seed expander. The polynomial returned by this function has full rank.
 *
 * \param[out] o Polynomial
 * \param[in] degree Degree of the polynomial
 * \param[in] ctx random_source
 */
void rbc_poly_set_random_full_rank(random_source* ctx, rbc_poly o, int32_t degree) {
  rbc_vec_set_random_full_rank(ctx, o->v, degree + 1);
}




/**
 * \fn void rbc_poly_set_random_full_rank_with_one(random_source* ctx, rbc_poly o, int32_t degree) {
 * \brief This function sets a polynomial with random values using the NGCC seed expander. The polynomial returned by this function has full rank and contains one.
 *
 * \param[out] ctx random source
 * \param[out] o Polynomial
 * \param[in] degree Degree of the polynomial
 */
void rbc_poly_set_random_full_rank_with_one(random_source* ctx, rbc_poly o, int32_t degree) {
  rbc_vec_set_random_full_rank_with_one(ctx, o->v, degree + 1);
}




/**
 * \fn void rbc_poly_set_random_from_support(random_source* ctx, rbc_poly o1, rbc_poly o2, int32_t degree, const rbc_vspace support, uint32_t support_size)
 * \brief This function sets a pair of polynomials with random values using NGCC seedexpander. The support (o1 || o2) is the one given in input.
 *
 * \param[out] o1 Polynomial
 * \param[out] o2 Polynomial
 * \param[in] degree Degree of <b>o1</b> and <b>o2</b>
 * \param[in] support Support
 * \param[in] support_size Size of the support
 */
void rbc_poly_set_random_from_support(random_source* ctx, rbc_poly o, int32_t degree, const rbc_vspace support, uint32_t support_size, uint8_t copy_flag) {
  rbc_vec_set_random_from_support(ctx, o->v, degree + 1, support, support_size, copy_flag);
}




/**
 * \fn void rbc_poly_set_random_pair_from_support(random_source* ctx, rbc_poly o1, rbc_poly o2, int32_t degree, const rbc_vspace support, uint32_t support_size)
 * \brief This function sets a pair of polynomials with random values using NGCC seedexpander. The support (o1 || o2) is the one given in input.
 *
 * \param[out] o1 Polynomial
 * \param[out] o2 Polynomial
 * \param[in] degree Degree of <b>o1</b> and <b>o2</b>
 * \param[in] support Support
 * \param[in] support_size Size of the support
 * \param[in] copy_flag If not 0, the support is copied into random coordinates of the resulting vector
 */
void rbc_poly_set_random_pair_from_support(random_source* ctx, rbc_poly o1, rbc_poly o2, int32_t degree, const rbc_vspace support, uint32_t support_size, uint8_t copy_flag) {
  rbc_vec_set_random_pair_from_support(ctx, o1->v, o2->v, degree + 1, support, support_size, copy_flag);
}




/**
 * \fn uint8_t rbc_poly_is_equal_to(const rbc_poly p1, const rbc_poly p2)
 * \brief This function test if two polynomials are equal.
 *
 * \param[in] p1 Polynomial
 * \param[in] p2 Polynomial
 * \return 1 if the polynomials are equal, 0 otherwise
 */
uint8_t rbc_poly_is_equal_to(const rbc_poly p1, const rbc_poly p2) {
  for(int32_t i = 0 ; i < p1->max_degree ; ++i) {
    if(rbc_elt_is_equal_to(p1->v[i], p2->v[i]) == 0) {
      return 0;
    }
  }

  return 1;
}




/**
 * \fn void rbc_poly_add(rbc_poly o, const rbc_poly p1, const rbc_poly p2)
 * \brief This function adds two polynomials.
 *
 * \param[out] o Sum of <b>p1</b> and <b>p2</b>
 * \param[in] p1 Polynomial
 * \param[in] p2 Polynomial
 */
void rbc_poly_add(rbc_poly o, const rbc_poly p1, const rbc_poly p2) {
  rbc_vec_add(o->v, p1->v, p2->v, p1->max_degree + 1);
}




/**
 * \fn void rbc_plain_mul(rbc_poly o, const rbc_poly p1, const rbc_poly p2)
 * \brief This function multiplies two polynomials.
 *
 * This function is based on NTL multiplication of GF2EX elements
 *
 * \param[out] o Polynomial
 * \param[in] a Polynomial
 * \param[in] b Polynomial
 */
void rbc_plain_mul(rbc_poly o, const rbc_poly a, const rbc_poly b) {
  int32_t d = a->max_degree + b->max_degree;
  rbc_elt acc, tmp;

  for(int32_t i = 0 ; i <= d ; ++i) {
    int32_t jmin, jmax;
    jmin = max(0, i - b->max_degree);
    jmax = min(a->max_degree, i);

    rbc_elt_set_zero(acc);

    for(int32_t j = jmin ; j <= jmax ; ++j) {
      rbc_elt_mul(tmp, a->v[j], b->v[i - j]);
      rbc_elt_add(acc, acc, tmp);
    }

    rbc_elt_set(o->v[i], acc);
  }
}



/**
 * Karatsuba multiplication of a and b using notations from "A course in computational algebraic number theory" (H. Cohen), 3.1.2. Implementation inspired from the NTL library.
 *
 * \param[out] o Polynomial
 * \param[in] a Polynomial
 * \param[in] b Polynomial
 */
void rbc_kar_fold(rbc_vec res, rbc_vec src, int32_t max, int32_t half_size) {
  for(int32_t i = 0 ; i < max / 2 ; ++i) {
    rbc_elt_add(res[i], src[i], src[i + half_size]);
  }

  if(max % 2 == 1) {
    rbc_elt_set(res[half_size - 1], src[half_size - 1]);
  }
}



void rbc_kar_mul(rbc_vec o, rbc_vec a, rbc_vec b, int32_t a_size, int32_t b_size, rbc_vec stack) {
  int32_t ha_size;
  rbc_vec a2, b2;

  // In every call a_size must be >= b_size
  if(a_size == 1) {
    rbc_elt_mul(o[0], a[0], b[0]);
    return;
  }

  if(a_size == 2) {
    if(b_size == 2) {
      // Hardcoded mul
      rbc_elt ea2, eb2, ed;
      rbc_elt_set_zero(ed);
      rbc_elt_mul(o[0], a[0], b[0]);
      rbc_elt_mul(o[2], a[1], b[1]);
      rbc_elt_add(ea2, a[0], a[1]);
      rbc_elt_add(eb2, b[0], b[1]);
      rbc_elt_mul(ed, ea2, eb2);
      rbc_elt_add(o[1], o[0], o[2]);
      rbc_elt_add(o[1], o[1], ed);
    } else { // b_size = 1
      rbc_elt_mul(o[0], a[0], b[0]);
      rbc_elt_mul(o[1], a[1], b[0]);
    }
    return;
  }

  ha_size = (a_size + 1) / 2;

  // Compute a2 = a0 + a1 and b2 = b0 + b1
  a2 = stack;
  b2 = stack + ha_size;

  rbc_kar_fold(a2, a, a_size, ha_size);
  rbc_kar_fold(b2, b, b_size, ha_size);

  // Computation of d = a2*b2
  
  // Reset the stack
  for(int32_t i = 2 * ha_size ; i < 4 * ha_size ; ++i) {
    rbc_elt_set_zero(stack[i]);
  }

  rbc_kar_mul(stack + 2 * ha_size, a2, b2, ha_size, ha_size, stack + 4 * ha_size);

  for(int32_t i = 0 ; i < 2 * (a_size - ha_size - 1) + 1 + 2 * ha_size ; ++i) {
    rbc_elt_set_zero(o[i]);
  }

  // Computation of c0 = a0*b0 in the low part of o
  rbc_kar_mul(o, a, b, ha_size, ha_size, stack + 4 * ha_size);

  // Computation of c2 = a1*b1 in the high part of o
  rbc_kar_mul(o + 2 * ha_size, a + ha_size, b + ha_size, a_size - ha_size, b_size - ha_size, stack + 4 * ha_size);

  // Computation of c1 = d + c2 + c0
  for(int32_t i = 0 ; i < 2 * (a_size - ha_size - 1) + 1 ; ++i) {
    rbc_elt_add(stack[i + 2 * ha_size], stack[i + 2 * ha_size], (o + 2 * ha_size)[i]);
  }

  for(int32_t i = 0 ; i < 2 * (ha_size - 1) + 1 ; ++i) {
    rbc_elt_add(stack[i + 2 * ha_size], stack[i + 2 * ha_size], o[i]);
  }

  // Add c1 to o
  for(int32_t i = 0 ; i <= 2 * (ha_size - 1) + 1 ; i++) {
    rbc_elt_add(o[i + ha_size], o[i + ha_size], stack[i + 2 * ha_size]);
  }
}



/**
 * \fn void rbc_poly_mul(rbc_poly o, const rbc_poly p1, const rbc_poly p2)
 * \brief This function multiplies two polynomials.
 *
 * \param[in] o Product of <b>p1</b> and <b>p2</b>
 * \param[in] p1 Polynomial
 * \param[in] p2 Polynomial
 */
void rbc_poly_mul(rbc_poly o, const rbc_poly a, const rbc_poly b) {
  if(a->max_degree <= 1 || b->max_degree <= 1) {
    rbc_plain_mul(o, a, b);
    return;
  }

  rbc_poly_mul2(o, a, b, a->max_degree, b->max_degree);
}



/**
 * \fn void rbc_poly_mul2(rbc_poly o, const rbc_poly a, const rbc_poly b, int32_t a_degree, int32_t b_degree)
 * \brief This function multiplies two polynomials using a_degree and b_degree as polynomials degrees.
 *
 * \param[out] o Product of <b>a</b> and <b>b</b>
 * \param[in] a Polynomial
 * \param[in] b Polynomial
 * \param[in] a_degree Degree to consider for a
 * \param[in] b_degree Degree to consider for b
 */
void rbc_poly_mul2(rbc_poly o, const rbc_poly a, const rbc_poly b, int32_t a_degree, int32_t b_degree) {
  int32_t max_degree;
  int32_t stack_size = 0;
  int32_t n;

  // Prepare polynomials such that the allocated space is big enough in every polynomial
  max_degree = max(a_degree, b_degree);

  n = max_degree;
  do {
    stack_size += 4 * ((n + 2) >> 1);
    n >>= 1;
  } while(n > 1);

  rbc_vec stack;
  rbc_vec_init(&stack, stack_size);

  rbc_kar_mul(o->v, a->v, b->v, max_degree + 1, max_degree + 1, stack);

  rbc_vec_clear(stack);
}




/**
 * \fn void rbc_poly_mulmod_sparse(rbc_poly o, const rbc_poly p1, const rbc_poly p2, const rbc_poly_sparse modulus)
 * \brief This function computes the product of two polynomials modulo a sparse one.
 *
 * \param[out] o Product of <b>p1</b> and <b>p2</b> modulo <b>modulus</b>
 * \param[in] p1 Polynomial
 * \param[in] p2 Polynomial
 * \param[in] modulus Sparse polynomial
 */
void rbc_poly_mulmod_sparse(rbc_poly o, const rbc_poly p1, const rbc_poly p2, const rbc_poly_sparse modulus) {
  int32_t modulus_degree = modulus->coeffs[modulus->coeffs_nb - 1];
  rbc_poly unreduced;

  rbc_poly_init(&unreduced, 2 * modulus_degree - 1);
  rbc_poly_set_zero(unreduced, 2 * modulus_degree - 1);

  rbc_poly_mul2(unreduced, p1, p2, modulus_degree - 1, modulus_degree - 1);

  // Modular reduction
  for(int32_t i = unreduced->max_degree - modulus_degree ; i >= 0 ; i--) {
    for(size_t j = 0 ; j < modulus->coeffs_nb - 1 ; j++) {
      rbc_elt_add(unreduced->v[i + modulus->coeffs[j]],
      unreduced->v[i + modulus->coeffs[j]], unreduced->v[i + modulus_degree]);
    }
    rbc_elt_set_zero(unreduced->v[i + modulus_degree]);
  }

  rbc_poly_set(o, unreduced);
  rbc_poly_clear(unreduced);
}

/**
 * \fn void rbc_poly_inv(rbc_poly o, const rbc_poly a, const rbc_poly modulus)
 * \brief This function uses the Bernstein Yang algorithm and its implementation is inspired from the paper "A Constant-time AVX2 Implementation of a Variant of Code-Based Cryptography"
 *
 * \param[out] o Inverse of <b>a</b> modulo <b>modulus</b>
 * \param[in] a Polynomial
 * \param[in] modulus Polynomial
 */
 void rbc_poly_inv(rbc_poly o, const rbc_poly a, const rbc_poly modulus) {
  int size = modulus->max_degree;

  rbc_poly f_prime, g_prime;
  rbc_poly_init(&f_prime, size);
  rbc_poly_init(&g_prime, size);

  rbc_poly tmp;
  rbc_poly_init(&tmp, size);

  rbc_poly v, r;
  rbc_poly_init(&v, size);
  rbc_poly_init(&r, size);
  rbc_poly_set_zero(v, size);
  rbc_poly_set_zero(r, size);

  rbc_elt_set_one(r->v[0]);

  //Reverse f and g
  for(int i=0 ; i<size + 1 ; i++) rbc_elt_set(f_prime->v[i], modulus->v[size - i]);
  for(int i=0 ; i<size ; i++) rbc_elt_set(g_prime->v[i], a->v[size - 1 - i]);

  rbc_elt f0, g0, tmp_elt;
  rbc_elt_set_zero(tmp_elt);

  rbc_elt_int delta = 1;
  rbc_elt_uint mask;

  int n = 2*size - 1;
  int s = 1;

  int n1;
  int s1 = s;

  while(n > 0) {
    n1 = (n > size + 1 ? size + 1: n);
    s1 = (s > size ? size : s);

    //Multiply v by x
    for(int i=s1 ; i>0 ; i--) rbc_elt_set(v->v[i], v->v[i-1]);
    rbc_elt_set_zero(v->v[0]);

    //mask = 1 if delta > 0 && !rbc_elt_is_zero(g(0))
    mask = ((-delta) >> (RBC_79_INTEGER_LENGTH - 1) & (1 - rbc_elt_is_zero(g_prime->v[0])));

    //Conditional swaps
    rbc_vec_add(tmp->v, f_prime->v, g_prime->v, n1);
    for(int i=0 ; i<n1 ; i++) {
      for(int j=0 ; j<RBC_79_ELT_SIZE ; j++) tmp->v[i][j] &= -mask;
    }
    rbc_vec_add(f_prime->v, f_prime->v, tmp->v, n1);
    rbc_vec_add(g_prime->v, g_prime->v, tmp->v, n1);

    rbc_vec_add(tmp->v, v->v, r->v, s1);
    for(int i=0 ; i<s1 ; i++) {
      for(int j=0 ; j<RBC_79_ELT_SIZE ; j++) tmp->v[i][j] &= -mask;
    }
    rbc_vec_add(v->v, v->v, tmp->v, s1);
    rbc_vec_add(r->v, r->v, tmp->v, s1);

    delta += (((-delta) & (-mask)) + ((-delta) & (-mask)) + 1);

    rbc_elt_set(f0, f_prime->v[0]);
    rbc_elt_set(g0, g_prime->v[0]);

    //g = f0 g - g0 f
    for(int i=0 ; i<n1 ; i++) {
     
      rbc_elt_mul(g_prime->v[i], g_prime->v[i], f0);
      rbc_elt_mul(tmp_elt, f_prime->v[i], g0);
      rbc_elt_add(g_prime->v[i], g_prime->v[i], tmp_elt);
    }

    //r = f0 r - g0 v
    for(int i=0 ; i<s1 ; i++) {
      rbc_elt_mul(r->v[i], r->v[i], f0);
      rbc_elt_mul(tmp_elt, v->v[i], g0);
      rbc_elt_add(r->v[i], r->v[i], tmp_elt);
    }

    //Divide g by x
    for(int i=0 ; i<n1-1 ; i++) {
      rbc_elt_set(g_prime->v[i], g_prime->v[i+1]);
    }

    rbc_elt_set_zero(g_prime->v[n1-1]);

    n--;
    s++;
  }

  rbc_elt_inv(tmp_elt, f_prime->v[0]);

  for(int i=0 ; i<size ; i++) {
    rbc_elt_mul(v->v[i], v->v[i], tmp_elt);
    rbc_elt_set(o->v[size - 1 - i], v->v[i]);
  }

  rbc_poly_clear(f_prime);
  rbc_poly_clear(g_prime);

  rbc_poly_clear(tmp);

  rbc_poly_clear(v);
  rbc_poly_clear(r);
 }




/**
 * \fn void rbc_poly_to_string(uint8_t* str, const rbc_poly p)
 * \brief This function parses a polynomial into a string.
 *
 * \param[out] str String
 * \param[in] p rbc_poly
 */
void rbc_poly_to_string(uint8_t* str, const rbc_poly p) {
  rbc_vec_to_string(str, p->v, p->max_degree + 1);
}




/**
 * \fn void rbc_poly_from_string(rbc_poly p, const uint8_t* str)
 * \brief This function parses a string into a polynomial.
 *
 * \param[out] p rbc_poly
 * \param[in] str String to parse
 */
void rbc_poly_from_string(rbc_poly p, const uint8_t* str) {
  rbc_vec_from_string(p->v, p->max_degree + 1, str);
}




/**
 * \fn void rbc_poly_print(const rbc_poly p)
 * \brief This function displays a polynomial.
 *
 * \param[in] p rbc_poly
 */
void rbc_poly_print(const rbc_poly p) {
  for(int i = 0 ; i < p->max_degree + 1 ; ++i) {
    if (i < 10) printf("\n%i   - ", i);
    else if (i < 100) printf("\n%i  - ", i);
    else printf("\n%i - ", i);
    rbc_elt_print(p->v[i]);
  }
}




/**
 * \fn void rbc_poly_sparse_print(const rbc_poly_sparse p)
 * \brief This function displays a sparse polynomial.
 *
 * \param[in] p rbc_poly_sparse
 */
void rbc_poly_sparse_print(const rbc_poly_sparse p) {
  for(size_t i = 0 ; i < p->coeffs_nb - 1 ; i++) {
    printf("X^%" PRIu32 " + ", p->coeffs[i]);
  }
  printf("X^%" PRIu32 "\n", p->coeffs[p->coeffs_nb - 1]);
}




/**
 * \fn void rbc_poly_set_monomial(rbc_poly o, int32_t degree)
 * \brief This functions sets a polynomial to x^degree.
 *
 * \param[in] p Polynomial
 * \param[in] degree Degree of the monomial
 */
void rbc_poly_set_monomial(rbc_poly o, uint32_t degree) {
  rbc_poly_set_zero(o, o->degree);
  rbc_elt_set_one(o->v[degree]);
}

