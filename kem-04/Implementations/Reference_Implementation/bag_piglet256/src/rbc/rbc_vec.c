#include "rbc_vec.h"

void rbc_vec_init(rbc_vec* v, uint32_t size) {
  *v = (rbc_vec) calloc(size, sizeof(rbc_elt));
}

void rbc_vec_clear(rbc_vec v) {
  free(v);
}

void rbc_vec_set_zero(rbc_vec v, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_set_zero(v[i]);
  }
}

void rbc_vec_set(rbc_vec o, const rbc_vec v, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_set(o[i], v[i]);
  }
}

void rbc_vec_set_random(rbc_vec o, uint32_t size, DRNG_ctx* ctx) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_set_random(o[i], ctx);
  }
}

void rbc_vec_set_random2(rbc_vec o, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_set_random2(o[i]);
  }
}

void rbc_vec_set_random_full_rank(rbc_vec o, uint32_t size, DRNG_ctx* ctx) {
  const uint32_t rank_max = RBC_FIELD_M < size ? RBC_FIELD_M : size;
  do {
    rbc_vec_set_random(o, size, ctx);
  } while(rbc_vec_get_rank(o, size) != rank_max);
}

void rbc_vec_set_random_full_rank2(rbc_vec o, uint32_t size) {
  const uint32_t rank_max = RBC_FIELD_M < size ? RBC_FIELD_M : size;
  do {
    rbc_vec_set_random2(o, size);
  } while(rbc_vec_get_rank(o, size) != rank_max);
}

uint32_t rbc_vec_get_rank(const rbc_vec v, uint32_t size) {
  const uint32_t words = RBC_ELT_UINT64;
  uint64_t* rows = (uint64_t*) calloc((size_t) size * words, sizeof(uint64_t));
  if(rows == NULL) {
    return 0;
  }

  for(uint32_t i = 0; i < size; ++i) {
    for(uint32_t j = 0; j < words; ++j) {
      rows[(size_t) i * words + j] = v[i][j];
    }
  }

  uint32_t rank = 0;
  for(int32_t col = RBC_FIELD_M - 1; col >= 0 && rank < size; --col) {
    uint32_t pivot = rank;
    while(pivot < size && (((rows[(size_t) pivot * words + col / 64] >> (col % 64)) & 1) == 0)) {
      ++pivot;
    }

    if(pivot == size) {
      continue;
    }

    if(pivot != rank) {
      for(uint32_t w = 0; w < words; ++w) {
        uint64_t tmp = rows[(size_t) pivot * words + w];
        rows[(size_t) pivot * words + w] = rows[(size_t) rank * words + w];
        rows[(size_t) rank * words + w] = tmp;
      }
    }

    for(uint32_t r = 0; r < size; ++r) {
      if(r != rank && ((rows[(size_t) r * words + col / 64] >> (col % 64)) & 1)) {
        for(uint32_t w = 0; w < words; ++w) {
          rows[(size_t) r * words + w] ^= rows[(size_t) rank * words + w];
        }
      }
    }

    ++rank;
  }

  free(rows);
  return rank;
}

uint8_t rbc_vec_is_equal_to(const rbc_vec v1, const rbc_vec v2, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    if(!rbc_elt_is_equal_to(v1[i], v2[i])) {
      return 0;
    }
  }

  return 1;
}

void rbc_vec_add(rbc_vec o, const rbc_vec v1, const rbc_vec v2, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_add(o[i], v1[i], v2[i]);
  }
}

void rbc_vec_scalar_mul(rbc_vec o, const rbc_vec v, const rbc_elt e, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_mul(o[i], v[i], e);
  }
}

void rbc_vec_from_string(rbc_vec o, uint32_t size, const uint8_t* str) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_from_string(o[i], str + i * RBC_FIELD_BYTES);
  }
}

void rbc_vec_to_string(uint8_t* str, const rbc_vec v, uint32_t size) {
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_to_string(str + i * RBC_FIELD_BYTES, v[i]);
  }
}

void rbc_vec_print(const rbc_vec v, uint32_t size) {
  printf("[ ");
  for(uint32_t i = 0; i < size; ++i) {
    rbc_elt_print(v[i]);
    printf(" ");
  }
  printf("]\n");
}
