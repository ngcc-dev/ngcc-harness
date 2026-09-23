#include "bag_piglet.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "augabidulin.h"
#include "auxfunc.h"
#include "ffi_elt.h"
#include "ffi_field.h"
#include "ffi_vec.h"
#include "parameters.h"
#include "parsing.h"
#include "randombytes.h"

static void sm3_xof(unsigned char *output, const unsigned char *input,
                    size_t input_len, size_t output_len) {
  pseudoXOF(8ULL * output_len, input, 8ULL * input_len, output);
}

static void clear_vec_array(ffi_vec *array, size_t count) {
  for(size_t i = 0; i < count; ++i) {
    ffi_vec_clear(&array[i]);
  }
}

void bag_piglet_pke_keygen(unsigned char *pk_string,
                           unsigned char *sk_string,
                           const unsigned char *seed) {
  unsigned char coin[SEEDEXPANDER_SEED_BYTES];
  unsigned char sk_seed[SEEDEXPANDER_SEED_BYTES];
  unsigned char pk_seed[SEEDEXPANDER_SEED_BYTES];
  unsigned char *seed_output = NULL;
  unsigned char *output = NULL;
  ffi_vec g = {0};
  ffi_vec h[BLOCK] = {{0}};
  ffi_vec support_total_1 = {0};
  ffi_vec support_x[1] = {{0}};
  ffi_vec x[1] = {{0}};
  ffi_vec support_y[BLOCK] = {{0}};
  ffi_vec y[BLOCK] = {{0}};
  ffi_vec s[BLOCK] = {{0}};

  if(seed == NULL) {
    CryptoRandomBytes(coin, SEEDEXPANDER_SEED_BYTES);
  } else {
    memcpy(coin, seed, SEEDEXPANDER_SEED_BYTES);
  }

  seed_output = (unsigned char *) malloc(SEEDEXPANDER_MAX_LENGTH);
  output = (unsigned char *) malloc(SEEDEXPANDER_MAX_LENGTH);
  if(seed_output == NULL || output == NULL) {
    abort();
  }
  sm3_xof(seed_output, coin, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);
  memcpy(sk_seed, seed_output, SEEDEXPANDER_SEED_BYTES);
  memcpy(pk_seed, seed_output + SEEDEXPANDER_SEED_BYTES,
         SEEDEXPANDER_SEED_BYTES);

  ffi_vec_set_length(&g, PARAM_N * PARAM_N_1);
  sm3_xof(output, pk_seed, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);
  const int g_blocks = ffi_vec_set_g_using_shake(&g,
                                                  PARAM_N * PARAM_N_1,
                                                  PARAM_N_, output);
  const size_t g_stream_size = PARAM_N_ * PARAM_M / 8 + 1;
  const size_t g_stream_end = (size_t) g_blocks * g_stream_size;

  for(int i = 0; i < BLOCK; ++i) {
    ffi_vec_set_length(&h[i], PARAM_N);
    ffi_vec_set_random_using_shake(&h[i], PARAM_N,
                                   output + g_stream_end +
                                   i * PARAM_N * FFI_ELT_BYTES);
  }

  sm3_xof(output, sk_seed, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);
  const int support_blocks = ffi_vec_set_random_full_rank_using_shake(
    &support_total_1, WEIGHT_TOTAL_1, output);
  size_t random_offset = (size_t) support_blocks *
    (WEIGHT_TOTAL_1 * PARAM_M / 8 + 1);
  const size_t random_stride = PARAM_N * FFI_ELT_BYTES +
    2 * WEIGHT_TOTAL_1;

  for(int j = 0; j < WEIGHT_X_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(&support_total_1,
                                             (unsigned int) j);
    ffi_vec_set_coeff(&support_x[0], &value, (unsigned int) j);
  }
  ffi_vec_set_random_from_support_using_shake(&x[0], PARAM_N,
                                               &support_x[0],
                                               WEIGHT_X_VALUES,
                                               output + random_offset);
  random_offset += random_stride;

  for(int j = 0; j < WEIGHT_Y1_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(
      &support_total_1, (unsigned int) (j + WEIGHT_X_VALUES));
    ffi_vec_set_coeff(&support_y[0], &value, (unsigned int) j);
  }
  for(int j = 0; j < WEIGHT_Y2_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(
      &support_total_1,
      (unsigned int) (j + WEIGHT_X_VALUES + WEIGHT_Y1_VALUES));
    ffi_vec_set_coeff(&support_y[1], &value, (unsigned int) j);
  }
  ffi_vec_set_random_from_support_using_shake(&y[0], PARAM_N,
                                               &support_y[0],
                                               WEIGHT_Y1_VALUES,
                                               output + random_offset);
  random_offset += random_stride;
  ffi_vec_set_random_from_support_using_shake(&y[1], PARAM_N,
                                               &support_y[1],
                                               WEIGHT_Y2_VALUES,
                                               output + random_offset);

  for(int i = 0; i < BLOCK; ++i) {
    ffi_vec_mul(&s[i], &x[0], &h[i], PARAM_N);
    ffi_vec_add(&s[i], &s[i], &y[i], PARAM_N);
  }

  bag_piglet_key_to_string(pk_string, s, pk_seed, 2);
  bag_piglet_key_to_string(sk_string, x, pk_seed, 1);

  clear_vec_array(s, BLOCK);
  clear_vec_array(y, BLOCK);
  clear_vec_array(support_y, BLOCK);
  clear_vec_array(x, 1);
  clear_vec_array(support_x, 1);
  ffi_vec_clear(&support_total_1);
  clear_vec_array(h, BLOCK);
  ffi_vec_clear(&g);
  free(output);
  free(seed_output);
}

void bag_piglet_pke_encrypt(const unsigned char *message,
                            unsigned char *ciphertext,
                            const unsigned char *pk_string,
                            const unsigned char *seed) {
  unsigned char pk_seed[SEEDEXPANDER_SEED_BYTES];
  unsigned char coin[SEEDEXPANDER_SEED_BYTES];
  unsigned char *output = NULL;
  ffi_vec s[BLOCK] = {{0}};
  ffi_vec u[PARAM_N_1] = {{0}};
  ffi_vec v[PARAM_N_1] = {{0}};
  ffi_vec m = {0};
  ffi_vec g = {0};
  ffi_vec h[BLOCK] = {{0}};
  ffi_vec support_total_2 = {0};
  ffi_vec support_r1[1] = {{0}};
  ffi_vec r1[PARAM_N_1] = {{0}};
  ffi_vec e[PARAM_N_1] = {{0}};
  ffi_vec support_e = {0};
  ffi_vec support_r2[BLOCK] = {{0}};
  ffi_vec r2[PARAM_N_1 * BLOCK] = {{0}};
  ffi_vec mg = {0};
  ffi_vec temp = {0};
  ffi_vec combined_v = {0};
  ffi_vec zero = {0};
  augabidulin_code code = {0};

  memset(ciphertext, 0, CPAPKE_CT_SIZE);
  bag_piglet_key_from_string(pk_seed, s, pk_string, 2);

  output = (unsigned char *) malloc(SEEDEXPANDER_MAX_LENGTH);
  if(output == NULL) {
    abort();
  }
  sm3_xof(output, pk_seed, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);
  ffi_vec_set_length(&g, PARAM_N * PARAM_N_1);
  const int g_blocks = ffi_vec_set_g_using_shake(&g,
                                                  PARAM_N * PARAM_N_1,
                                                  PARAM_N_, output);
  const size_t g_stream_size = PARAM_N_ * PARAM_M / 8 + 1;
  const size_t g_stream_end = (size_t) g_blocks * g_stream_size;

  for(int i = 0; i < BLOCK; ++i) {
    ffi_vec_set_random_using_shake(&h[i], PARAM_N,
                                   output + g_stream_end +
                                   i * PARAM_N * FFI_ELT_BYTES);
  }
  ffi_vec_from_string_compact(&m, PARAM_K, message);

  if(seed == NULL) {
    CryptoRandomBytes(coin, SEEDEXPANDER_SEED_BYTES);
  } else {
    memcpy(coin, seed, SEEDEXPANDER_SEED_BYTES);
  }

  sm3_xof(output, coin, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);

  const int support_blocks = ffi_vec_set_random_full_rank_using_shake(
    &support_total_2, WEIGHT_TOTAL_2, output);
  size_t random_offset = (size_t) support_blocks *
    (WEIGHT_TOTAL_2 * PARAM_M / 8 + 1);
  const size_t random_stride = PARAM_N * FFI_ELT_BYTES +
    2 * WEIGHT_TOTAL_2;
  for(int j = 0; j < WEIGHT_R1_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(&support_total_2,
                                             (unsigned int) j);
    ffi_vec_set_coeff(&support_r1[0], &value, (unsigned int) j);
  }
  for(int j = 0; j < PARAM_N_1; ++j) {
    ffi_vec_set_random_from_support_using_shake(&r1[j], PARAM_N,
                                                 &support_r1[0],
                                                 WEIGHT_R1_VALUES,
                                                 output + random_offset);
    random_offset += random_stride;
  }

  ffi_vec_set_length(&support_e, WEIGHT_E_VALUES);
  for(int j = 0; j < WEIGHT_E_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(
      &support_total_2,
      (unsigned int) (j + WEIGHT_R1_VALUES + WEIGHT_R21_VALUES +
                      WEIGHT_R22_VALUES));
    ffi_vec_set_coeff(&support_e, &value, (unsigned int) j);
  }
  for(int i = 0; i < PARAM_N_1; ++i) {
    ffi_vec_set_random_from_support_using_shake(&e[i], PARAM_N,
                                                 &support_e,
                                                 WEIGHT_E_VALUES,
                                                 output + random_offset);
    random_offset += random_stride;
  }

  for(int j = 0; j < WEIGHT_R21_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(
      &support_total_2, (unsigned int) (j + WEIGHT_R1_VALUES));
    ffi_vec_set_coeff(&support_r2[0], &value, (unsigned int) j);
  }
  for(int j = 0; j < WEIGHT_R22_VALUES; ++j) {
    const ffi_elt value = ffi_vec_get_coeff(
      &support_total_2,
      (unsigned int) (j + WEIGHT_R1_VALUES + WEIGHT_R21_VALUES));
    ffi_vec_set_coeff(&support_r2[1], &value, (unsigned int) j);
  }

  for(int j = 0; j < PARAM_N_1; ++j) {
    ffi_vec_set_random_from_support_using_shake(&r2[j * BLOCK], PARAM_N,
                                                 &support_r2[0],
                                                 WEIGHT_R21_VALUES,
                                                 output + random_offset);
    random_offset += random_stride;
  }
  for(int j = 0; j < PARAM_N_1; ++j) {
    ffi_vec_set_random_from_support_using_shake(&r2[j * BLOCK + 1], PARAM_N,
                                                 &support_r2[1],
                                                 WEIGHT_R22_VALUES,
                                                 output + random_offset);
    random_offset += random_stride;
  }

  ffi_vec_set_length(&mg, PARAM_N_1 * PARAM_N);
  ffi_vec_set_zero(&temp, PARAM_N);
  for(int k = 0; k < PARAM_N_1; ++k) {
    ffi_vec_set_length(&u[k], PARAM_N);
    for(int i = 0; i < BLOCK; ++i) {
      if(i == 0) {
        ffi_vec_mul(&u[k], &h[i], &r2[i + k * BLOCK], PARAM_N);
      } else {
        ffi_vec_mul(&temp, &h[i], &r2[i + k * BLOCK], PARAM_N);
        ffi_vec_add(&u[k], &u[k], &temp, PARAM_N);
      }
    }
    ffi_vec_add(&u[k], &u[k], &r1[k], PARAM_N);
  }

  for(int k = 0; k < PARAM_N_1; ++k) {
    ffi_vec_set_length(&v[k], PARAM_N);
    for(int i = 0; i < BLOCK; ++i) {
      if(i == 0) {
        ffi_vec_mul(&v[k], &s[i], &r2[i + k * BLOCK], PARAM_N);
      } else {
        ffi_vec_mul(&temp, &s[i], &r2[i + k * BLOCK], PARAM_N);
        ffi_vec_add(&v[k], &v[k], &temp, PARAM_N);
      }
    }
    ffi_vec_add(&v[k], &v[k], &e[k], PARAM_N);
  }

  ffi_vec_set_length(&combined_v, PARAM_N * PARAM_N_1);
  for(int k = 0; k < PARAM_N_1; ++k) {
    for(int i = 0; i < PARAM_N; ++i) {
      ffi_vec_set_coeff(&combined_v, &v[k].data[i],
                        (unsigned int) (k * PARAM_N + i));
    }
  }
  ffi_vec_set_zero(&zero, PARAM_N * PARAM_N_1);

  if(augabidulin_code_init(&code, &g, PARAM_K,
                           PARAM_N * PARAM_N_1, PARAM_N_) != 0) {
    abort();
  }
  augabidulin_code_encode(&mg, &code, &m);
  ffi_vec_set_length(&mg, PARAM_N * PARAM_N_1);
  ffi_vec_add(&combined_v, &combined_v, &mg, PARAM_N * PARAM_N_1);
  bag_piglet_cipher_to_string(ciphertext, u, &combined_v);

  augabidulin_code_clear(&code);
  ffi_vec_clear(&zero);
  ffi_vec_clear(&combined_v);
  ffi_vec_clear(&temp);
  ffi_vec_clear(&mg);
  clear_vec_array(r2, PARAM_N_1 * BLOCK);
  clear_vec_array(support_r2, BLOCK);
  ffi_vec_clear(&support_e);
  clear_vec_array(e, PARAM_N_1);
  clear_vec_array(r1, PARAM_N_1);
  clear_vec_array(support_r1, 1);
  ffi_vec_clear(&support_total_2);
  clear_vec_array(h, BLOCK);
  ffi_vec_clear(&g);
  ffi_vec_clear(&m);
  clear_vec_array(v, PARAM_N_1);
  clear_vec_array(u, PARAM_N_1);
  clear_vec_array(s, BLOCK);
  free(output);
}

void bag_piglet_pke_decrypt(unsigned char *message,
                            const unsigned char *ciphertext,
                            const unsigned char *sk_string) {
  unsigned char pk_seed[SEEDEXPANDER_SEED_BYTES];
  unsigned char *output = NULL;
  ffi_vec x[BLOCK] = {{0}};
  ffi_vec u[PARAM_N_1] = {{0}};
  ffi_vec v[PARAM_N_1] = {{0}};
  ffi_vec decoded = {0};
  ffi_vec combined_v = {0};
  ffi_vec g = {0};
  ffi_vec product[PARAM_N_1] = {{0}};
  augabidulin_code code = {0};

  bag_piglet_key_from_string(pk_seed, x, sk_string, 1);
  ffi_vec_set_length(&combined_v, PARAM_N * PARAM_N_1);
  bag_piglet_cipher_from_string(u, &combined_v, ciphertext);

  for(int k = 0; k < PARAM_N_1; ++k) {
    ffi_vec_set_length(&v[k], PARAM_N);
    for(int i = 0; i < PARAM_N; ++i) {
      ffi_vec_set_coeff(&v[k], &combined_v.data[i + k * PARAM_N],
                        (unsigned int) i);
    }
  }

  output = (unsigned char *) malloc(SEEDEXPANDER_MAX_LENGTH);
  if(output == NULL) {
    abort();
  }
  sm3_xof(output, pk_seed, SEEDEXPANDER_SEED_BYTES,
          SEEDEXPANDER_MAX_LENGTH);
  ffi_vec_set_length(&g, PARAM_N * PARAM_N_1);
  ffi_vec_set_g_using_shake(&g, PARAM_N * PARAM_N_1, PARAM_N_, output);

  for(int k = 0; k < PARAM_N_1; ++k) {
    ffi_vec_mul(&product[k], &x[0], &u[k], PARAM_N);
    ffi_vec_add(&product[k], &product[k], &v[k], PARAM_N);
  }
  for(int k = 0; k < PARAM_N_1; ++k) {
    for(int i = 0; i < PARAM_N; ++i) {
      ffi_vec_set_coeff(&combined_v, &product[k].data[i],
                        (unsigned int) (i + k * PARAM_N));
    }
  }

  if(augabidulin_code_init(&code, &g, PARAM_K,
                           PARAM_N * PARAM_N_1, PARAM_N_) != 0) {
    abort();
  }
  augabidulin_code_decode(&decoded, &code, &combined_v, PARAM_epsilon);
  ffi_vec_to_string_compact(message, &decoded, PARAM_K);

  augabidulin_code_clear(&code);
  clear_vec_array(product, PARAM_N_1);
  ffi_vec_clear(&g);
  ffi_vec_clear(&combined_v);
  ffi_vec_clear(&decoded);
  clear_vec_array(v, PARAM_N_1);
  clear_vec_array(u, PARAM_N_1);
  clear_vec_array(x, BLOCK);
  free(output);
}
