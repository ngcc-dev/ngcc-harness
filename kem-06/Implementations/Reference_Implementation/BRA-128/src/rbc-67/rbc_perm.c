/**
 * \file rbc_perm.c
 * \brief Implementation of rbc_perm.h
 */

#include "rbc_perm.h"




/**
 * \fn void rbc_perm_init(rbc_perm* perm, uint32_t size)
 * \brief This function allocates the memory for a rbc_perm.
 *
 * \param[out] perm Pointer to the allocated rbc_perm
 * \param[in] size Size of the rbc_perm element perm
 */
void rbc_perm_init(rbc_perm* perm, uint32_t size) {
  *perm = calloc(size, sizeof(uint64_t));
  if (perm == NULL) exit(EXIT_FAILURE);
}




/**
 * \fn void rbc_perm_clear(rbc_perm perm)
 * \brief This function clears a rbc_perm element.
 *
 * \param[out] perm rbc_perm
 * \param[in] size Row size of the rbc_perm
 */
void rbc_perm_clear(rbc_perm perm) {
    free(perm);
}



/**
 * \fn void void rbc_perm_set_zero(rbc_perm o, uint32_t size)
 * \brief This function copies a matrix over GF(q).
 *
 * \param[out] o rbc_mat_fq
 * \param[in] size Size of the rbc_perm
 */
void rbc_perm_set_zero(rbc_perm o, uint32_t size) {
  for(size_t i = 0 ; i < size ; ++i) {
      o[i] = 0;
  }
}








/**
 * \fn void rbc_perm_set(rbc_perm o, const rbc_perm perm, uint32_t size)
 * \brief This function copies a matrix over GF(q).
 *
 * \param[out] o rbc_mat_fq
 * \param[in] m Pointer to the allocated rbc_mat_fq
 * \param[in] rows Row size of the rbc_mat_fq
 * \param[in] columns Column size of the rbc_mat_fq
 */
void rbc_perm_set(rbc_perm o, const rbc_perm perm, uint32_t size) {
  for(size_t i = 0 ; i < size ; ++i) {
      o[i] = perm[i];
  }
}








/**
 * \fn void rbc_perm_set_random(random_source* ctx, rbc_perm perm, uint32_t size)
 * \brief This function random samples a permutation matrix over GF(q) coded as a random array of integers.
 *
 * \param[out] ctx NGCC seed expander
 * \param[out] perm array of random integers determining a permutation matrix m
 * \param[in] size Column and row sizes of the rbc_mat element m
 */
void rbc_perm_set_random(random_source* ctx, rbc_perm perm, uint32_t size) {
  uint32_t bytes = size * sizeof(uint64_t);
  uint8_t random[bytes];
  rbc_perm_set_zero(perm, size);
  random_source_get_bytes(ctx, random, bytes);
  memcpy(perm, random, bytes);
}




/**
 * \fn void rbc_perm_set_random_from_xof(rbc_perm perm, uint32_t size, void (*xof)(uint8_t *, size_t, const uint8_t *, size_t), const uint8_t *xof_input, uint32_t xof_size)
 * \brief This function random samples a permutation matrix over GF(q) coded as a random array of integers.
 *
 * \param[out] perm array of random integers determining a permutation matrix m
 * \param[in] size Column and row sizes of the rbc_mat element m
 * \param[in] xof XOF procedure
 * \param[in] xof_input XOF input byte array
 * \param[in] xof_size XOF input byte array length of xof_input
 */
void rbc_perm_set_random_from_xof(rbc_perm perm, uint32_t size, void (*xof)(uint8_t *, size_t, const uint8_t *, size_t), const uint8_t *xof_input, uint32_t xof_size) {
  uint32_t bytes = size * sizeof(uint64_t);
  uint8_t random[bytes];
  rbc_perm_set_zero(perm, size);
  xof(random, bytes, xof_input, xof_size);
  memcpy(perm, random, bytes);
}

/**
 * \fn void rbc_perm_apply(rbc_mat_fq o, const rbc_mat_fq m, const rbc_perm perm, uint32_t size)
 * \brief Multiplication by permutation matrix over GF(q) via djb-sort.
 *
 * \param[in] m rbc_mat_fq of dimension size x size
 * \param[in] perm permutation coded as a random list of integers
 * \param[in] size Column and row sizes of the rbc_mat_fq element m
 */
void rbc_perm_apply(rbc_mat_fq o, const rbc_mat_fq m, const rbc_perm perm, uint32_t size) {
  rbc_perm tmp;
  rbc_perm_init(&tmp, size);
  rbc_perm_set(tmp, perm, size);
  rbc_mat_fq_set(o, m, size, size);
  for(int j = 0 ; j < (int)size; ++j) {
    for(int i =  (j - 1); i >= 0; --i) {
      rbc_mat_fq_minmax(&o[i], &tmp[i], &o[i + 1], &tmp[i + 1], size);
    }
  }
  rbc_perm_clear(tmp);
}

