/**
 * \file rbc_mat_fq.c
 * \brief Implementation of rbc_mat_fq.h
 */

#include "rbc_mat_fq.h"




/**
 * \fn void rbc_mat_fq_init(rbc_mat_fq* m, uint32_t rows, uint32_t columns)
 * \brief This function allocates the memory for a rbc_mat_fq.
 *
 * \param[out] m Pointer to the allocated rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_init(rbc_mat_fq* m, uint32_t rows, uint32_t columns) {
  uint32_t words = (columns + 63) / 64;
  *m = calloc(rows, sizeof(uint64_t *));
  (*m)[0] = calloc(rows * words, sizeof(uint64_t *));
  for(size_t i = 1 ; i < rows ; ++i) {
    (*m)[i] = (*m)[0] + (i * words);
  }
if (m == NULL) exit(EXIT_FAILURE);
}




/**
 * \fn void rbc_mat_fq_clear(rbc_mat_fq m)
 * \brief This function clears a rbc_mat_fq element.
 *
 * \param[out] m rbc_mat_fq
 * \param[in] size Row size of the rbc_mat
 */
void rbc_mat_fq_clear(rbc_mat_fq m) {
    free(m[0]);
    free(m);
}



/**
 * \fn void rbc_mat_fq_set_zero(rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief This function sets a matrix over GF(q) to zero.
 *
 * \param[out] o rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_set_zero(rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  uint32_t words = (columns + 63) / 64;
  for(size_t i = 0 ; i < rows ; ++i) {
    memset(m[i], 0, words * sizeof(uint64_t));
  }
}




/**
 * \fn void rbc_mat_fq_set_identity(rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief This function sets a matrix over GF(q) to zero.
 *
 * \param[out] o rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_set_identity(rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  uint32_t words = (columns + 63) / 64;
  for(size_t i = 0 ; i < rows ; ++i) {
    memset(m[i], 0, words * sizeof(uint64_t));
    m[i][i / 64] = (uint64_t)1 << (i % 64);
  }
}




/**
 * \fn void rbc_mat_fq_set(rbc_mat_fq o, const rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief This function copies a matrix over GF(q).
 *
 * \param[out] o rbc_mat_fq
 * \param[in] m Pointer to the allocated rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_set(rbc_mat_fq o, const rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  uint32_t words = (columns + 63) / 64
;  for(size_t i = 0 ; i < rows ; ++i) {
    for(size_t j = 0 ; j < words ; ++j) {
      o[i][j] = m[i][j];
    }
  }
}




/**
 * \fn void rbc_mat_fq_set_random(random_source* ctx, rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief This function random samples a matrix over GF(q).
 *
 * \param[out] ctx NGCC seed expander
 * \param[out] m rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_fq_set_random(random_source* ctx, rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  uint32_t bytes = (columns + 7) / 8;
  uint32_t words = (columns + 63) / 64;
  uint8_t random[bytes];
  uint8_t mask = (uint8_t)(columns % 8);
  mask = ((uint8_t)(mask != 0) << mask) - 1;
  for(size_t i = 0 ; i < rows ; ++i) {
    memset(m[i], 0, words * sizeof(uint64_t));
    random_source_get_bytes(ctx, random, bytes);
    random[bytes - 1] &= mask;
    memcpy(m[i], random, bytes);
  }
}




/**
 * \fn void rbc_mat_fq_set_transpose(rbc_mat_fq o, const rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief This function transpose matrices over GF(q).
 *
 * \param[out] o rbc_mat_fq equal to \f$ m ^ T \f$
 * \param[in] m rbc_mat_fq 
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_set_transpose(rbc_mat_fq o, const rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  uint64_t m_ij;
  rbc_mat_fq_set_zero(o, columns, rows);
  for(size_t i = 0 ; i < rows ; ++i) {
    for(size_t j = 0 ; j < columns ; ++j) {
      m_ij = (m[i][j / 64] >> (j % 64)) & 1;
      o[j][i / 64] ^= (m_ij << (i % 64));
    }
  }
}




/**
 * \fn void rbc_mat_fq_set_inverse(rbc_mat_fq o, const rbc_mat_fq m, uint32_t size)
 * \brief This function inverses matrices over GF(q) via Gaussian elimination.
 *
 * \param[out] o rbc_mat_fq equal to \f$ m ^ {-1} \f$
 * \param[in] m rbc_mat_fq 
 * \param[in] size Column and row sizes of the rbc_mat_fq
 */
void rbc_mat_fq_set_inverse(rbc_mat_fq o, const rbc_mat_fq m, uint32_t size) {
  uint32_t words = (size + 63) / 64;
  uint64_t mask_i, mask_j, tmp_i, tmp_j;
  rbc_mat_fq t, v;
  rbc_mat_fq_init(&t, size, size);
  rbc_mat_fq_init(&v, 1, size);
  rbc_mat_fq_set(t, m, size, size);
  rbc_mat_fq_set_identity(o, size, size);
  for(size_t i = 0 ; i < size; ++i) {
    mask_i = -(t[i][i / 64] & ((uint64_t)1 << (i % 64)));
    mask_i >>= 63;
    tmp_i = mask_i;
    for(size_t j = (i + 1); j < size ; ++j) {
      mask_j = -(t[j][i / 64] & ((uint64_t)1 << (i % 64)));
      mask_j >>= 63;
      tmp_j = mask_j;
      rbc_mat_fq_minmax(&t[j], &mask_j, &t[i], &mask_i, size);
      rbc_mat_fq_minmax(&o[j], &tmp_j, &o[i], &tmp_i, size);
    }
    mask_i = 0;
    for(size_t j = 0; j < i ; ++j) {
      for(size_t k = 0; k < words ; ++k) {
        v[0][k] = t[i][k] ^ t[j][k];
      }
      mask_j = -(t[j][i / 64] & ((uint64_t)1 << (i % 64)));
      mask_j >>= 63;
      tmp_i = mask_i;
      tmp_j = mask_j;
      rbc_mat_fq_minmax(&t[j], &tmp_j, &v[0], &tmp_i, size);
      for(size_t k = 0; k < words ; ++k) {
        v[0][k] = o[i][k] ^ o[j][k];
      }
      tmp_i = mask_i;
      tmp_j = mask_j;
      rbc_mat_fq_minmax(&o[j], &tmp_j, &v[0], &tmp_i, size);
    }
    for(size_t j = (i + 1); j < size ; ++j) {
      for(size_t k = 0; k < words ; ++k) {
        v[0][k] = t[i][k] ^ t[j][k];
      }
      mask_j = -(t[j][i / 64] & ((uint64_t)1 << (i % 64)));
      mask_j >>= 63;
      tmp_i = mask_i;
      tmp_j = mask_j;
      rbc_mat_fq_minmax(&t[j], &tmp_j, &v[0], &tmp_i, size);
      for(size_t k = 0; k < words ; ++k) {
        v[0][k] = o[i][k] ^ o[j][k];
      }
      tmp_i = mask_i;
      tmp_j = mask_j;
      rbc_mat_fq_minmax(&o[j], &tmp_j, &v[0], &tmp_i, size);
    }
  }
  mask_i = 0;
  mask_j = 0;
  tmp_i = 0;
  tmp_j = 0;
  rbc_mat_fq_clear(v);
  rbc_mat_fq_clear(t);
}

/**
 * \fn void rbc_mat_fq_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows1, uint32_t columns1_rows2, uint32_t columns2)
 * \brief This functions multiplies matrices over GF(q).
 *
 * \param[out] o rbc_mat_fq equal to \f$ m1 \times m2 \f$
 * \param[in] m1 rbc_mat_fq
 * \param[in] m2 rbc_mat_fq
 * \param[in] rows1 Row size of m1
 * \param[in] columns1_rows2 Column and row sizes of m1 and m2, respectively
 * \param[in] columns2 Column size of m2
 */
void rbc_mat_fq_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows1, uint32_t columns1_rows2, uint32_t columns2) {
  uint64_t acc;
  rbc_mat_fq t2;
  rbc_mat_fq_init(&t2, columns2, columns1_rows2);
  rbc_mat_fq_set_transpose(t2, m2, columns1_rows2, columns2);
  rbc_mat_fq_set_zero(o, rows1, columns2);
  for(size_t i = 0 ; i < rows1 ; ++i) {
    for(size_t j = 0 ; j < columns2 ; ++j) {
      acc = 0;
      for(size_t k = 0 ; k < ((columns1_rows2 + 63) / 64) ; ++k) {
        acc ^= (m1[i][k] & t2[j][k]);
      }
      o[i][j / 64] ^= ((uint64_t)__builtin_popcountll(acc) & 0x1) << (j % 64);
    }
  }
  rbc_mat_fq_clear(t2);
}




/**
 * \fn void rbc_mat_fq_tr_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows1, uint32_t columns, uint32_t rows2)
 * \brief This functions multiplies matrices over GF(q).
 *
 * \param[out] o rbc_mat_fq equal to \f$ m1 \times m2^T \f$
 * \param[in] m1 rbc_mat_fq
 * \param[in] m2 rbc_mat_fq
 * \param[in] rows1 Row size of m1
 * \param[in] columns Column size for both of m1 and m2
 * \param[in] rows2 Row size of m2
 */
void rbc_mat_fq_tr_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows1, uint32_t columns, uint32_t rows2) {
  uint64_t acc;
  rbc_mat_fq_set_zero(o, rows1, rows2);
  for(size_t i = 0 ; i < rows1 ; ++i) {
    for(size_t j = 0 ; j < rows2 ; ++j) {
      acc = 0;
      for(size_t k = 0 ; k < ((columns + 63) / 64) ; ++k) {
        acc ^= (m1[i][k] & m2[j][k]);
      }
      o[i][j / 64] ^= ((uint64_t)__builtin_popcountll(acc) & 0x1) << (j % 64);
    }
  }
}




/**
 * \fn void rbc_tr_mat_fq_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows, uint32_t columns1, uint32_t columns2)
 * \brief This functions multiplies matrices over GF(q).
 *
 * \param[out] o rbc_mat_fq equal to \f$ m1^T \times m2 \f$
 * \param[in] m1 rbc_mat_fq
 * \param[in] m2 rbc_mat_fq
 * \param[in] rows Row size for both of m1 and m2
 * \param[in] columns1 Column size of m1
 * \param[in] columns2 Column size of m2
 */
void rbc_tr_mat_fq_mul(rbc_mat_fq o, const rbc_mat_fq m1, const rbc_mat_fq m2, uint32_t rows, uint32_t columns1, uint32_t columns2) {
  uint64_t acc;
  rbc_mat_fq tm1, tm2;
  rbc_mat_fq_init(&tm1, columns1, rows);
  rbc_mat_fq_init(&tm2, columns2, rows);
  rbc_mat_fq_set_transpose(tm1, m1, rows, columns1);
  rbc_mat_fq_set_transpose(tm2, m2, rows, columns2);
  rbc_mat_fq_set_zero(o, columns1, columns2);
  for(size_t i = 0 ; i < columns1 ; ++i) {
    for(size_t j = 0 ; j < columns2 ; ++j) {
      acc = 0;
      for(size_t k = 0 ; k < ((rows + 63) / 64) ; ++k) {
        acc ^= (tm1[i][k] & tm2[j][k]);
      }
      o[i][j / 64] ^= ((uint64_t)__builtin_popcountll(acc) & 0x1) << (j % 64);
    }
  }
  rbc_mat_fq_clear(tm1);
  rbc_mat_fq_clear(tm2);
}




/**
 * \fn void rbc_mat_fq_minmax(rbc_mat_fq c1, uint64_t* x, rbc_mat_fq c2, uint64_t* y, uint32_t size)
 * \brief MinMax functions to swap matrices over GF(q) in the djb-sort.
 *
 * \param[in] c1 rbc_mat_fq of dimension 1 x size
 * \param[in] x permutation entry
 * \param[in] c2 rbc_mat_fq of dimension 1 x size
 * \param[in] k permutation entry
 * \param[in] n Column size of the rbc_mat_fq elements c1 and c2
 */
void rbc_mat_fq_minmax(rbc_mat_fq c1, uint64_t* x, rbc_mat_fq c2, uint64_t* y, uint32_t size) {
  int64_t a = *x;
  int64_t b = *y;
  int64_t ab = b ^ a;
  int64_t c = b - a;
  c ^= ab & (c ^ b);
  c >>= 63;
  uint64_t z;
  z = (uint64_t)(c & ab);
  *x = a ^ z;
  *y = b ^ z;
  uint64_t words = (size + 63)/64;
  for(size_t j = 0 ; j < words; ++j) {
    a = (*c1)[j];
    b = (*c2)[j];
    ab = a ^ b;
    z = (uint64_t)(c & ab);
    (*c1)[j] = a ^ z;
    (*c2)[j] = b ^ z;
  }
}




/**
 * \fn void rbc_mat_fq_gen(random_source* ctx, rbc_perm perm, rbc_mat_fq t, uint32_t size)
 * \brief Utility function that generates a random permutation matrix A and a random non-singular upper triangular matrix T.
 *
 * \param[out] ctx NGCC seed expander
 * \param[out] a random permutation rbc_mat_fq coded according djb-sort array
 * \param[out] t random non-singular lower triangular matrix rbc_mat_fq
 * \param[in] size Matrix dimension
 */
void rbc_mat_fq_gen(random_source* ctx, rbc_perm perm, rbc_mat_fq t, uint32_t size) {
  uint64_t mask;
  uint32_t bytes = 0, pos = 0, words, words_row = (size + 63) / 64;
  for(uint64_t row = (size - 1); row > 0; row--) {
    bytes += ((row + 7) / 8);  }
  uint8_t random[bytes], mask_bytes;  random_source_get_bytes(ctx, random, bytes);
  for(uint32_t row = (size - 1); row > 0; row--) {
    memset(t[row], 0, words_row * sizeof(uint64_t));
    words = (row + 7) / 8;    mask_bytes = (uint8_t)(row % 8);
    mask_bytes = ((uint8_t)(mask_bytes != 0) << mask_bytes) - 1;
    random[pos + words - 1] &= mask_bytes;
    memcpy(t[row], &random[pos], words);
    mask = (uint64_t)1 << (row % 64);
    t[row][row / 64] |= mask;
    pos += words;  }
  t[0][0] = 1;
  memset(random, 0, bytes);

  rbc_perm_set_random(ctx, perm, size);
  rbc_perm_apply(t, t, perm, size);
}




/**
 * \fn void rbc_mat_fq_gen_from_xof(rbc_perm perm, rbc_mat_fq t, uint32_t size, void (*xof)(uint8_t *, size_t, const uint8_t *, size_t), const uint8_t *xof_input, uint32_t xof_size)
 * \brief Utility function that generates a random permutation matrix A and a random non-singular upper triangular matrix T.
 *
 * \param[out] a random permutation rbc_mat_fq coded according djb-sort array
 * \param[out] t random non-singular lower triangular matrix rbc_mat_fq
 * \param[in] size Matrix dimension
 * \param[in] xof XOF procedure
 * \param[in] xof_input XOF input byte array
 * \param[in] xof_size XOF input byte array length of xof_input
 */
void rbc_mat_fq_gen_from_xof(rbc_perm perm, rbc_mat_fq t, uint32_t size, void (*xof)(uint8_t *, size_t, const uint8_t *, size_t), const uint8_t *xof_input, uint32_t xof_size) {
  uint64_t mask;
  uint32_t bytes = 0, pos = 0, words, words_row = (size + 63) / 64;
  for(uint32_t row = (size - 1); row > 0; row--) {
    bytes += ((row + 7) / 8);  }
  uint8_t random[bytes + xof_size], mask_bytes;
  xof(random, bytes + xof_size, xof_input, xof_size);
  for(uint32_t row = (size - 1); row > 0; row--) {
    memset(t[row], 0, words_row * sizeof(uint64_t));
    words = (row + 7) / 8;    mask_bytes = (uint8_t)(row % 8);
    mask_bytes = ((uint8_t)(mask_bytes != 0) << mask_bytes) - 1;
    random[pos + words - 1] &= mask_bytes;
    memcpy(t[row], &random[pos], words);
    mask = (uint64_t)1 << (row % 64);
    t[row][row / 64] |= mask;
    pos += words;  }
  t[0][0] = 1;

  rbc_perm_set_random_from_xof(perm, size, xof, &random[bytes], xof_size);
  rbc_perm_apply(t, t, perm, size);
  memset(random, 0, bytes + xof_size);
}




/**
 * \fn void rbc_mat_fq_vec_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t rows, uint32_t columns)
 * \brief This functions multiplies a matrix over GF(q) by a vector over GF(q^m).
 *
 * \param[out] o rbc_vec equal to \f$ m \times v \f$
 * \param[in] m rbc_mat_fq
 * \param[in] v rbc_vec
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq, and Row size of the rbd_vec
 */
void rbc_mat_fq_vec_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t rows, uint32_t columns) {
  rbc_elt tmp, acc;
  uint64_t mask;
  for(size_t i = 0 ; i < rows ; ++i) {
    rbc_elt_set_zero(acc);
    for(size_t j = 0 ; j < columns ; ++j) {
      mask = (uint64_t)(m[i][j / 64] >> (j % 64));
      mask = -(mask & 1);
      for(size_t l = 0 ; l < RBC_79_ELT_SIZE; ++l) {
        tmp[l] = v[j][l] & mask;
      }
      rbc_elt_add(acc, acc, tmp);
    }
  rbc_elt_set(o[i], acc);
  }
}




/**
 * \fn void rbc_vec_mat_fq_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t rows, uint32_t columns)
 * \brief This functions multiplies a matrix over GF(q) by a vector over GF(q^m).
 *
 * \param[out] o rbc_vec equal to \f$ v \times m \f$
 * \param[in] m rbc_mat_fq
 * \param[in] v rbc_vec
 * \param[in] rows Row size of the rbc_mat_fq, and Row size of the rbd_vec
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_vec_mat_fq_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t rows, uint32_t columns) {
  rbc_elt tmp, acc;
  uint64_t mask;
  for(size_t j = 0 ; j < columns ; ++j) {
    rbc_elt_set_zero(acc);
    for(size_t k = 0 ; k < rows ; ++k) {
      mask = (uint64_t)(m[k][j / 64] >> (j % 64));
      mask = -(mask & 1);
      for(size_t l = 0 ; l < RBC_79_ELT_SIZE; ++l) {
        tmp[l] = v[k][l] & mask;
      }
      rbc_elt_add(acc, acc, tmp);
    }
  rbc_elt_set(o[j], acc);
  }
}




/**
 * \fn void rbc_tr_mat_fq_vec_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t size)
 * \brief This functions multiplies matrices over GF(q). Particular case of rbc_tr_mat_fq_mul()
 *
 * \param[out] o rbc_vec equal to \f$ m^T \times v \f$
 * \param[in] m rbc_mat_fq square matrix of dimension size
 * \param[in] v rbc_vec
 * \param[in] size Row size for both of m and v
 */
void rbc_tr_mat_fq_vec_mul(rbc_vec o, const rbc_mat_fq m, const rbc_vec v, uint32_t size) {
  uint64_t acc;
  rbc_mat_fq tm, mv, tv;
  rbc_mat_fq_init(&tm, size, size);
  rbc_mat_fq_init(&mv, size, RBC_79_FIELD_M);
  rbc_mat_fq_init(&tv, RBC_79_FIELD_M, size);
  rbc_mat_fq_set_transpose(tm, m, size, size);
  for(size_t i = 0 ; i < size ; ++i) { memcpy(mv[i], v[i], (RBC_79_FIELD_M + 7) / 8); }
  rbc_mat_fq_set_transpose(tv, mv, size, RBC_79_FIELD_M);
  rbc_vec_set_zero(o, size);
  for(size_t i = 0 ; i < size ; ++i) {
    for(size_t j = 0 ; j < RBC_79_FIELD_M ; ++j) {
      acc = 0;
      for(size_t k = 0 ; k < ((size + 63) / 64) ; ++k) {
        acc ^= (tm[i][k] & tv[j][k]);
      }
      o[i][j / 64] ^= ((uint64_t)__builtin_popcountll(acc) & 0x1) << (j % 64);
    }
  }
  rbc_mat_fq_clear(tm);
  rbc_mat_fq_clear(tv);
  rbc_mat_fq_clear(mv);
}




/**
 * \fn void rbc_vec_mat_fq_tr_mul(rbc_vec o, const rbc_vec v, const rbc_mat_fq m, uint32_t size)
 * \brief This functions multiplies matrices over GF(q). Particular case of rbc_mat_fq_tr_mul()
 *
 * \param[out] o rbc_vec equal to \f$ v \times m^T \f$
 * \param[in] v rbc_vec
 * \param[in] m rbc_mat_fq square matrix of dimension RBC_79_FIELD_M
 * \param[in] size Row size of v
 */
void rbc_vec_mat_fq_tr_mul(rbc_vec o, const rbc_vec v, const rbc_mat_fq m, uint32_t size) {
  uint64_t acc;
  rbc_vec_set_zero(o, size);
  for(size_t i = 0 ; i < size ; ++i) {
    for(size_t j = 0 ; j < RBC_79_FIELD_M ; ++j) {
      acc = 0;
      for(size_t k = 0 ; k < ((RBC_79_FIELD_M + 63) / 64) ; ++k) {
        acc ^= (v[i][k] & m[j][k]);
      }
      o[i][j / 64] ^= ((uint64_t)__builtin_popcountll(acc) & 0x1) << (j % 64);
    }
  }
}




/**
 * \fn void rbc_mat_fq_to_string(uint8_t* str, const rbc_mat_fq m, uint32_t size)
 * \brief This function parses a matrix of finite field elements into a string.
 *
 * \param[out] str Output string
 * \param[in] m rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_fq_to_string(uint8_t* str, const rbc_mat_fq m, uint32_t rows, uint32_t columns) {
    uint32_t bytes = (columns + 7) / 8;
    memset(str, 0, rows * bytes);
    for(size_t i = 0 ; i < rows ; i++) {
        memcpy(str + i * bytes, (uint8_t *)(m[i]), bytes);
    }
}



/**
 * \fn void rbc_mat_fq_from_string(rbc_mat_fq m, uint32_t size, const uint8_t* str)
 * \brief This function parses a string into a matrix of finite field elements.
 *
 * \param[out] m rbc_mat_fq
 * \param[in] size Size of the matrix
 * \param[in] str String to parse
 * \param[in] rows Row size of the rbc_mat
 * \param[in] columns Column size of the rbc_mat
 */
void rbc_mat_fq_from_string(rbc_mat_fq m, uint32_t rows, uint32_t columns, const uint8_t* str) {
    uint32_t bytes = (columns + 7) / 8;
    rbc_mat_fq_set_zero(m, rows, columns);
    for(size_t i = 0 ; i < rows ; i++) {
        memcpy((uint8_t *)(m[i]), str + i * bytes, bytes);
    }
}



/**
 * \void rbc_mat_fq_print(rbc_mat_fq m, uint32_t rows, uint32_t columns)
 * \brief Display a rbc_mat_fq element.
 *
 * \param[in] m rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_mat_fq_print(rbc_mat_fq m, uint32_t rows, uint32_t columns) {
  printf("[\n");
  for(size_t i = 0 ; i < rows ; ++i) {
    printf("[");
    for(size_t j = 0 ; j < columns ; ++j) {
      printf(" %X", (uint8_t)((m[i][j / 64] >> (j % 64)) & 1));
    }
    printf("]\n");  }
  printf("]\n");}




/**
 * \fn void rbc_vec_mat_fq_compress(rbc_vec basis, rbc_vec lcomb, const rbc_vec v, uint32_t size, uint32_t w)
 * \brief This functions multiplies matrices over GF(q).
 *
 * \param[out] basis rbc_vec basis of rank w
 * \param[out] lcomb rbc_vec basis of rank w
 * \param[in] v rbc_vec
 * \param[in] size Size of v
 * \param[in] w Rank of v
 */
void rbc_vec_mat_fq_compress(rbc_vec basis, rbc_vec lcomb, const rbc_vec v, uint32_t size, uint32_t w) {
  rbc_vec lc, bs;
  rbc_vec_set_zero(lcomb, w);
  rbc_vec_set_zero(basis, w);
  rbc_vec_init(&lc, RBC_79_FIELD_M);
  rbc_vec_set_zero(lc, RBC_79_FIELD_M);
  rbc_vec_init(&bs, RBC_79_FIELD_M);
  rbc_vec_set_zero(bs, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < RBC_79_FIELD_M ; ++i) {
    lc[i][i / 64] = (uint64_t)1 << (i % 64);
  }
  rbc_mat_fq v_mat, lc_mat, bs_mat, v_out;
  rbc_mat_fq_init(&v_mat, size, RBC_79_FIELD_M);
  rbc_mat_fq_set_zero(v_mat, size, RBC_79_FIELD_M);
  rbc_mat_fq_init(&lc_mat, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_set_zero(lc_mat, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_init(&bs_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_set_zero(bs_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_init(&v_out, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_set_zero(v_out, RBC_79_FIELD_M, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < size ; ++i) {
    for(size_t j = 0 ; j < (RBC_79_FIELD_M+63)/64; ++j) {
      v_mat[i][j] = v[i][j];
    }
  }
  rbc_mat_fq_set_transpose(bs_mat, v_mat, size, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < RBC_79_FIELD_M ; ++i) {
    for(size_t j = 0 ; j < (size+63)/64; ++j) {
      bs[i][j] = bs_mat[i][j];
    }
  }
  rbc_vec_gauss(bs, RBC_79_FIELD_M, 0, &lc, 1);
  for(size_t i = 0 ; i < RBC_79_FIELD_M ; ++i) {
    for(size_t j = 0 ; j < (size+63)/64; ++j) {
      lc_mat[i][j] = lc[i][j];
    }
  }
  rbc_mat_fq_set_inverse(lc_mat, lc_mat, RBC_79_FIELD_M);
  rbc_mat_fq_set_transpose(v_out, lc_mat, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_vec_set_zero(lc, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < w ; ++i) {
    for(size_t j = 0 ; j < (RBC_79_FIELD_M+63)/64; ++j) {
      lcomb[i][j] = v_out[i][j];
      basis[i][j] = bs[i][j];
    }
  }
  rbc_mat_fq_clear(v_out);
  rbc_mat_fq_clear(bs_mat);
  rbc_mat_fq_clear(lc_mat);
  rbc_mat_fq_clear(v_mat);
  rbc_vec_clear(bs);
  rbc_vec_clear(lc);
}




/**
 * \fn rbc_vec_mat_fq_decompress(rbc_vec v, const rbc_vec basis, const rbc_vec lcomb, uint32_t size, uint32_t w)
 * \brief This functions multiplies matrices over GF(q).
 *
 * \param[out] basis rbc_vec basis of rank w
 * \param[out] lcomb rbc_vec basis of rank w
 * \param[in] v rbc_vec
 * \param[in] size Size of v
 * \param[in] w Rank of v
 */
void rbc_vec_mat_fq_decompress(rbc_vec v, const rbc_vec basis, const rbc_vec lcomb, uint32_t size, uint32_t w) {
  rbc_vec_set_zero(v, size);
  rbc_mat_fq v_mat, lc_mat, bs_mat, v_out;
  rbc_mat_fq_init(&v_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_set_zero(v_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_init(&lc_mat, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_set_zero(lc_mat, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_init(&bs_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_set_zero(bs_mat, RBC_79_FIELD_M, size);
  rbc_mat_fq_init(&v_out, RBC_79_FIELD_M, RBC_79_FIELD_M);
  rbc_mat_fq_set_zero(v_out, RBC_79_FIELD_M, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < w ; ++i) {
    for(size_t j = 0 ; j < (RBC_79_FIELD_M+63)/64; ++j) {
      bs_mat[i][j] = basis[i][j];
      lc_mat[i][j] = lcomb[i][j];
    }
  }
  rbc_mat_fq_set_transpose(v_mat, bs_mat, size, RBC_79_FIELD_M);
  rbc_mat_fq_mul(v_out, v_mat, lc_mat, size, RBC_79_FIELD_M, RBC_79_FIELD_M);
  for(size_t i = 0 ; i < size ; ++i) {
    for(size_t j = 0 ; j < (RBC_79_FIELD_M+63)/64; ++j) {
      v[i][j] = v_out[i][j];
    }
  }
  rbc_mat_fq_clear(v_out);
  rbc_mat_fq_clear(bs_mat);
  rbc_mat_fq_clear(lc_mat);
  rbc_mat_fq_clear(v_mat);
}

