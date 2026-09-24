#include "augabidulin.h"

#include <stdlib.h>

#include "ffi_elt.h"
#include "gabidulin.h"
#include "parameters.h"
#include "q_polynomial.h"

int augabidulin_code_init(augabidulin_code *code, const ffi_vec *g,
                          unsigned int k, unsigned int n,
                          unsigned int n_) {
  ffi_vec_init(&code->g);
  if(ffi_vec_copy(&code->g, g) != 0) {
    return -1;
  }
  code->k = k;
  code->n = n;
  code->n_ = n_;
  return 0;
}

void augabidulin_code_clear(augabidulin_code *code) {
  ffi_vec_clear(&code->g);
  code->k = 0;
  code->n = 0;
  code->n_ = 0;
}

void augabidulin_code_encode(ffi_vec *c, const augabidulin_code *code,
                             const ffi_vec *message) {
  ffi_elt matrix[PARAM_K][PARAM_N_] = {0};
  ffi_elt tmp = {0};

  for(unsigned int j = 0; j < code->n_; ++j) {
    ffi_elt_set(&matrix[0][j], &code->g.data[j]);
    for(unsigned int i = 1; i < code->k; ++i) {
      ffi_elt_sqr(&matrix[i][j], &matrix[i - 1][j]);
    }
  }

  ffi_elt_set_zero(&tmp);
  ffi_vec_set_zero(c, code->n);
  for(unsigned int i = 0; i < code->k; ++i) {
    for(unsigned int j = 0; j < code->n_; ++j) {
      ffi_elt_mul(&tmp, &message->data[i], &matrix[i][j]);
      ffi_elt_add(&tmp, &c->data[j], &tmp);
      ffi_vec_set_coeff(c, &tmp, j);
    }
  }

#ifdef VERBOSE
  printf("\n\n\n# AuGabidulin Encoding - Begin #");
  printf("\n\ng: ");
  ffi_vec_print(&code->g, PARAM_N_);
  printf("\nmatrix:[ ");
  for(unsigned int i = 0; i < code->k; ++i) {
    printf("[ ");
    for(unsigned int j = 0; j < code->n_; ++j) {
      ffi_elt_print(&matrix[i][j]);
    }
    printf("] ");
  }
  printf("]\n\ncodeword: ");
  ffi_vec_print(c, PARAM_N_);
  printf("\n# AuGabidulin Encoding - End #\n");
#endif
}

void augabidulin_code_decode(ffi_vec *message,
                             const augabidulin_code *code,
                             const ffi_vec *word,
                             unsigned int epsilon) {
  ffi_vec word_prefix = {0};
  ffi_vec error_suffix = {0};
  ffi_vec error_support = {0};
  ffi_vec result = {0};
  ffi_vec v2e = {0};
  ffi_vec z = {0};
  ffi_vec generator_prefix = {0};
  q_polynomial v2 = {0};
  gabidulin_code inner_code = {0};

  ffi_vec_set_length(&word_prefix, PARAM_N_);
  for(int i = 0; i < PARAM_N_; ++i) {
    const ffi_elt value = ffi_vec_get_coeff(word, (unsigned int) i);
    ffi_vec_set_coeff(&word_prefix, &value, (unsigned int) i);
  }

  ffi_vec_set_length(&error_suffix,
                     PARAM_N * PARAM_N_1 - PARAM_N_);
  for(int i = 0; i < PARAM_N * PARAM_N_1 - PARAM_N_; ++i) {
    const ffi_elt value = ffi_vec_get_coeff(word,
                                             (unsigned int) (i + PARAM_N_));
    ffi_vec_set_coeff(&error_suffix, &value, (unsigned int) i);
  }

  ffi_vec_set_length(&error_support, epsilon);
  ffi_vec_recover_part_error_support(&error_support, epsilon,
                                     PARAM_N * PARAM_N_1 - PARAM_N_,
                                     &error_suffix);

  if(q_polynomial_init(&v2, epsilon) != 0) {
    abort();
  }
  q_polynomial_set_interpolate(&v2, &error_support, (int) epsilon);

  ffi_vec_set_length(&result, PARAM_N * PARAM_N_1 - PARAM_N_);
  for(int i = 0; i < PARAM_N * PARAM_N_1 - PARAM_N_; ++i) {
    q_polynomial_evaluate(&result.data[i], &v2, &error_suffix.data[i]);
  }

  ffi_vec_set_length(&v2e, PARAM_N * PARAM_N_1 - PARAM_N_);
  for(unsigned int i = 0; i < epsilon; ++i) {
    q_polynomial_evaluate(&v2e.data[i], &v2, &error_support.data[i]);
  }

  ffi_vec_set_length(&z, PARAM_N_);
  for(int i = 0; i < PARAM_N_; ++i) {
    q_polynomial_evaluate(&z.data[i], &v2, &word_prefix.data[i]);
  }

  ffi_vec_set_length(&generator_prefix, PARAM_N_);
  for(int i = 0; i < PARAM_N_; ++i) {
    const ffi_elt value = ffi_vec_get_coeff(&code->g, (unsigned int) i);
    ffi_vec_set_coeff(&generator_prefix, &value, (unsigned int) i);
  }

  if(gabidulin_code_init(&inner_code, &generator_prefix,
                         PARAM_K_Gabidulin, PARAM_N_) != 0) {
    abort();
  }
  gabidulin_code_decode_3(message, &inner_code, &z, &v2);

  gabidulin_code_clear(&inner_code);
  q_polynomial_clear(&v2);
  ffi_vec_clear(&generator_prefix);
  ffi_vec_clear(&z);
  ffi_vec_clear(&v2e);
  ffi_vec_clear(&result);
  ffi_vec_clear(&error_support);
  ffi_vec_clear(&error_suffix);
  ffi_vec_clear(&word_prefix);
}
