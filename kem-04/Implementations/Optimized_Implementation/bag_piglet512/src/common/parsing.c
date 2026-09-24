#include "parsing.h"

#include <stdio.h>
#include <string.h>

#include "parameters.h"

void bag_piglet_key_to_string(unsigned char *key, const ffi_vec *s,
                              const unsigned char *seed, int flag) {
  memcpy(key, seed, SEEDEXPANDER_SEED_BYTES);
  if(flag == 1) {
    ffi_vec_to_string_compact(key + SEEDEXPANDER_SEED_BYTES, &s[0],
                              PARAM_N);
  } else {
    ffi_vec packed = {0};
    ffi_vec_set_length(&packed, BLOCK * PARAM_N);
    for(int i = 0; i < BLOCK; ++i) {
      for(int j = 0; j < PARAM_N; ++j) {
        const ffi_elt value = ffi_vec_get_coeff(&s[i], (unsigned int) j);
        ffi_vec_set_coeff(&packed, &value,
                          (unsigned int) (i * PARAM_N + j));
      }
    }
    ffi_vec_to_string_compact(key + SEEDEXPANDER_SEED_BYTES, &packed,
                              BLOCK * PARAM_N);
    ffi_vec_clear(&packed);
  }
}

void bag_piglet_key_from_string(unsigned char *seed, ffi_vec *s,
                                const unsigned char *key, int flag) {
  memcpy(seed, key, SEEDEXPANDER_SEED_BYTES);
  if(flag == 1) {
    ffi_vec_from_string_compact(&s[0], PARAM_N,
                                key + SEEDEXPANDER_SEED_BYTES);
  } else {
    ffi_vec packed = {0};
    ffi_vec_from_string_compact(&packed, BLOCK * PARAM_N,
                                key + SEEDEXPANDER_SEED_BYTES);
    for(int i = 0; i < BLOCK; ++i) {
      ffi_vec_set_length(&s[i], PARAM_N);
      for(int j = 0; j < PARAM_N; ++j) {
        const ffi_elt value = ffi_vec_get_coeff(
          &packed, (unsigned int) (i * PARAM_N + j));
        ffi_vec_set_coeff(&s[i], &value, (unsigned int) j);
      }
    }
    ffi_vec_clear(&packed);
  }
}

void bag_piglet_cipher_to_string(unsigned char *ciphertext,
                                 const ffi_vec *u,
                                 const ffi_vec *v) {
  ffi_vec packed = {0};
  const int half_size = PARAM_N * PARAM_N_1;
  ffi_vec_set_length(&packed, 2 * half_size);

  for(int i = 0; i < half_size; ++i) {
    const ffi_elt value = ffi_vec_get_coeff(v, (unsigned int) i);
    ffi_vec_set_coeff(&packed, &value, (unsigned int) i);
  }
  for(int i = 0; i < PARAM_N_1; ++i) {
    for(int j = 0; j < PARAM_N; ++j) {
      const ffi_elt value = ffi_vec_get_coeff(&u[i], (unsigned int) j);
      ffi_vec_set_coeff(&packed, &value,
                        (unsigned int) (half_size + i * PARAM_N + j));
    }
  }
  ffi_vec_to_string_compact(ciphertext, &packed, 2 * half_size);
  ffi_vec_clear(&packed);
}

void bag_piglet_cipher_from_string(ffi_vec *u, ffi_vec *v,
                                   const unsigned char *ciphertext) {
  ffi_vec packed = {0};
  const int half_size = PARAM_N * PARAM_N_1;
  ffi_vec_from_string_compact(&packed, 2 * half_size, ciphertext);
  ffi_vec_set_length(v, half_size);
  for(int i = 0; i < half_size; ++i) {
    const ffi_elt value = ffi_vec_get_coeff(&packed, (unsigned int) i);
    ffi_vec_set_coeff(v, &value, (unsigned int) i);
  }
  for(int i = 0; i < PARAM_N_1; ++i) {
    ffi_vec_set_length(&u[i], PARAM_N);
    for(int j = 0; j < PARAM_N; ++j) {
      const ffi_elt value = ffi_vec_get_coeff(
        &packed, (unsigned int) (half_size + i * PARAM_N + j));
      ffi_vec_set_coeff(&u[i], &value, (unsigned int) j);
    }
  }
  ffi_vec_clear(&packed);
}

void unsigned_char_print(const unsigned char *value, int size) {
  for(int i = 0; i < size; ++i) {
    printf("%u ", (unsigned int) value[i]);
  }
  putchar('\n');
}
