/**
 * \file augmented_gabidulin.h
 * \brief Functions to encode and decode messages using Augmented Gabidulin codes
 *
 * The decoding algorithm provided is based on q_polynomials reconstruction, see \cite gabidulin:welch and \cite gabidulin:generalized for details.
 *
 */

#ifndef RBC_83_AUGMENTED_GABIDULIN_H
#define RBC_83_AUGMENTED_GABIDULIN_H

#include "rbc_vec.h"


/**
  * \typedef rbc_augmented_gabidulin
  * \brief Structure of an augmented gabidulin code
  */
typedef struct rbc_augmented_gabidulin {
  rbc_vec g; /**< Generator vector defining the code */
  uint32_t k; /**< Size of vectors representing messages */
  uint32_t n; /**< Size of vectors representing codewords */
} rbc_augmented_gabidulin;


void rbc_augmented_gabidulin_init(rbc_augmented_gabidulin* code, const rbc_vec g, uint32_t k, uint32_t n);

void rbc_augmented_gabidulin_encode(rbc_vec c, const rbc_augmented_gabidulin gc, const rbc_vec m);
void rbc_augmented_gabidulin_decode(rbc_vec m, const rbc_augmented_gabidulin gc, const rbc_vec y);

#endif
