/**
 * \file rbc_elt.c
 * \brief Implementation of rbc_elt.h
 */

#include "rbc_163.h"
#include "rbc_elt.h"

static uint8_t rbc_init_field = 0;
uint64_t RBC_163_SQR_LOOKUP_TABLE[256];



/**
 * \fn void rbc_field_init(void)
 * \brief This function initializes various constants used to perform finite field arithmetic.
 *
 */
void rbc_field_init(void) {
  uint8_t bit = 0;
  uint64_t mask = 0;

  if(rbc_init_field == 0) {
    memset(RBC_163_SQR_LOOKUP_TABLE, 0, 8 * 256);
    for(size_t i = 0 ; i < 256 ; ++i) {
      for(size_t j = 0 ; j < 8 ; ++j) {
        mask = 1 << j;
        bit = (mask & i) >> j;
        RBC_163_SQR_LOOKUP_TABLE[i] ^= bit << (2 * j);
      }
    }

    rbc_init_field = 1;
  }
}



/**
 * \fn void rbc_elt_set_zero(rbc_elt o)
 * \brief This function sets a finite field element to zero.
 *
 * \param[out] o rbc_elt
 */
void rbc_elt_set_zero(rbc_elt o) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_SIZE ; i++) {
    o[i] = 0;
  }
}



/**
 * \fn void rbc_elt_set_one(rbc_elt o)
 * \brief This function sets a finite field element to one.
 *
 * \param[out] o rbc_elt
 */
void rbc_elt_set_one(rbc_elt o) {
  o[0] = 1;
  for(uint8_t i = 1 ; i < RBC_163_ELT_SIZE ; i++) {
    o[i] = 0;
  }
}



/**
 * \fn void rbc_elt_set(rbc_elt o, const rbc_elt e)
 * \brief This function copies a finite field element into another one.
 *
 * \param[out] o rbc_elt
 * \param[in] e rbc_elt
 */
void rbc_elt_set(rbc_elt o, const rbc_elt e) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_SIZE ; i++) {
    o[i] = e[i];
  }
}

/**
* \fn void rbc_elt_set_mask1(rbc_elt o, const rbc_elt e1, const rbc_elt e2, uint32_t mask)
* \brief This function copies either e1 or e2 into o depending on the mask value
*
* \param[out] o rbc_elt
* \param[in] e1 rbc_elt
* \param[in] e2 rbc_elt_n* \param[in] mask 1 to copy e1 and 0 to copy e2
*/
void rbc_elt_set_mask1(rbc_elt o, const rbc_elt e1, const rbc_elt e2, uint32_t mask) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_SIZE ; i++) {
    o[i] = mask * e1[i] + (1 - mask) * e2[i];
  }
}

/**
 * \fn void rbc_elt_set(rbc_elt o1, rbc_elt o2, const rbc_elt e, uint32_t mask)
 * \brief This function copies e either into o1 or o2 depending on the mask value
 *
 * \param[out] o1 rbc_elt
 * \param[out] o2 rbc_elt
 * \param[in] e rbc_elt
 * \param[in] mask 1 to copy into o1 and 0 to copy into o2
 */
void rbc_elt_set_mask2(rbc_elt o1, rbc_elt o2, const rbc_elt e, uint32_t mask) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_SIZE ; i++) {
    o1[i] = mask * e[i] + (1 - mask) * o1[i];
    o2[i] = mask * e[i] + (1 - mask) * o2[i];
  }
}

/**
 * \fn void rbc_elt_set_from_uint64(rbc_elt o, uint64_t* rbc_elt e)
 * \brief This function set a finite field element from a pointer to uint64_t.
 *
 * \param[out] o rbc_elt
 * \param[in] e Pointer to uint64_t
 */
void rbc_elt_set_from_uint64(rbc_elt o, const uint64_t* e) {
  rbc_elt_set(o, e);
}


/**
 * \fn void rbc_elt_set_random
 * \brief This function sets a finite field element with random values using the NGCC seed expander.
 *
 * \param[out] ctx random_source
 * \param[out] o rbc_elt
 */
void rbc_elt_set_random(random_source* ctx, rbc_elt o) {
  rbc_elt_set_zero(o);

  uint8_t random[RBC_163_ELT_UINT8_SIZE];
  random_source_get_bytes(ctx, random, RBC_163_ELT_UINT8_SIZE);
  random[RBC_163_ELT_UINT8_SIZE - 1] &= RBC_163_ELT_MASK;

  memcpy((uint64_t*) o, random, RBC_163_ELT_UINT8_SIZE);
}

/**
 * \fn uint8_t rbc_elt_is_zero(const rbc_elt e)
 * \brief This function tests if a finite field element is equal to zero.
 * 
 * \param[in] e rbc_elt
 * \return 1 if <b>e</b> is equal to zero, 0 otherwise
 */
uint8_t rbc_elt_is_zero(const rbc_elt e) {
  int8_t result = 1;
  for(int i = RBC_163_ELT_DATA_SIZE - 1 ; i >= 0 ; i--) {
    result &= (e[i] == 0);
  }

  return result;
}

/**
 * \fn uint8_t rbc_elt_is_equal_to(const rbc_elt e1, const rbc_elt e2)
 * \brief This function tests if two finite field elements are equal.
 *
 * \param[in] e1 rbc_elt
 * \param[in] e2 rbc_elt
 * \return 1 if <b>e1</b> and <b>e2</b> are equal, 0 otherwise
 */
uint8_t rbc_elt_is_equal_to(const rbc_elt e1, const rbc_elt e2) {
  int8_t result = 1;
  for(uint8_t i = 0 ; i < RBC_163_ELT_DATA_SIZE ; i++) {
    result &= (e1[i] == e2[i]);
  }
 return result;
}

/**
 * \fn uint8_t rbc_elt_is_greater_than(const rbc_elt e1, const rbc_elt e2)
 * \brief This function compares two finite field elements.
 *
 * \param[in] e1 rbc_elt
 * \param[in] e2 rbc_elt
 * 
eturn 1 if <b>e1</b> > <b>e2</b>, 0 otherwise
 */
uint8_t rbc_elt_is_greater_than(const rbc_elt e1, const rbc_elt e2) {
  int8_t result = 0, flag = 0, tmp = 0;

  for(int8_t i = RBC_163_ELT_DATA_SIZE - 1 ; i >= 0 ; i--) {
    tmp = (e1[i] > e2[i]);
    result = (tmp && !flag) || (flag && result);

    if(i != 0) {
      flag = tmp || flag;
      tmp = (e1[i] < e2[i]);
      flag = tmp || flag;
    }
  }
  return result;
}

/**
 * \fn int32_t rbc_elt_get_degree(const rbc_elt e)
 * \brief This function returns the degree of a finite field element.
 *
 * \param[in] e rbc_elt
 * \return Degree of <b>e</b> 
 */
int32_t rbc_elt_get_degree(const rbc_elt e) {
  int64_t index = 0, result = -1;
  int8_t mask = 0;

  for(uint8_t i = 0 ; i < RBC_163_ELT_DATA_SIZE ; i++) {
    __asm__ volatile("bsr %1,%0;" : "=r"(index) : "r"(e[i]));
    mask = (e[i] != 0);
    result = mask * (index + 64 * i) + (1 - mask) * result;
  }

  return result;
}

/**
 * \fn uint8_t rbc_elt_get_coefficient(const rbc_elt e, uint32_t index)
 * \brief This function returns the coefficient of the polynomial <b>e</b> at a given index.
 *
 * \param[in] e rbc_elt
 * \param[in] index Index of the coefficient
 * \return Coefficient of <b>e</b> at the given index
 */
uint8_t rbc_elt_get_coefficient(const rbc_elt e, uint32_t index) {
  uint64_t w = 0;

  for(uint8_t i = 0 ; i < RBC_163_ELT_DATA_SIZE ; i++) {
    w |= -((i ^ (index >> 6)) == 0) & e[i];
  }

  return (w >> (index & 63)) & 1;
}

/**
 * \fn void rbc_elt_set_coefficient_vartime(rbc_elt o, uint32_t index, uint64_t bit)
 * \brief This function set a coefficient of the polynomial <b>e</b>.
 *
 * \param[in] e rbc_elt
 * \param[in] index Index of the coefficient
 * \param[in] bit Value of the coefficient
 */
void rbc_elt_set_coefficient_vartime(rbc_elt o, uint32_t index, uint8_t bit) {
  size_t position = index / 64;
  o[position] |= (uint64_t) bit << (index % 64);
}

/**
 * \fn rbc_elt_add(rbc_elt o, const rbc_elt e1, const rbc_elt e2)
 * \brief This function adds two finite field elements.
 *
 * \param[out] o Sum of <b>e1</b> and <b>e2</b>
 * \param[in] e1 rbc_elt
 * \param[in] e2 rbc_elt
 */
void rbc_elt_add(rbc_elt o, const rbc_elt e1, const rbc_elt e2) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_SIZE ; i++) {
    o[i] = e1[i] ^ e2[i];
  }
}


/**
 * \fn void rbc_elt_mul(rbc_elt o, const rbc_elt e1, const rbc_elt e2)
 * \brief This function multiplies two finite field elements.
 *
 * \param[out] o Product of <b>e1</b> and <b>e2</b>
 * \param[in] e1 rbc_elt
 * \param[in] e2 rbc_elt
 */
void rbc_elt_mul(rbc_elt o, const rbc_elt e1, const rbc_elt e2) {
  rbc_elt_ur tmp;
  rbc_elt_ur_mul(tmp, e1, e2);
  rbc_elt_reduce(o, tmp);
}



/**
 * \fn rbc_elt_inv(rbc_elt o, const rbc_elt e)
 * \brief This function computes the multiplicative inverse of a finite field element.
 *
 * \param[out] o rbc_elt
 * \param[in] e rbc_elt
 */
void rbc_elt_inv(rbc_elt o, const rbc_elt e) {
  rbc_elt tmp;

  rbc_elt_set(tmp, e);
  for(int i = 0 ; i < (RBC_163_FIELD_M - 2) ; i++) {
    rbc_elt_sqr(o, tmp);
    rbc_elt_mul(tmp, o, e);
  }
  rbc_elt_sqr(o, tmp);
}



/**
 * \fn void rbc_elt_sqr(rbc_elt o, const rbc_elt e)
 * \brief This function computes the square of a finite field element.
 *
 * \param[out] o rbc_elt equal to \f$ e^{2} \f$
 * \param[in] e rbc_elt
 */
void rbc_elt_sqr(rbc_elt o, const rbc_elt e) {
  /*
  if(rbc_init_field == 0) {
    printf("Call to rbc_elt_sqr with uninitialized field\n");
    exit(1);
  }
  */

  rbc_elt_ur tmp;
  rbc_elt_ur_sqr(tmp, e);
  rbc_elt_reduce(o, tmp);
}

/**
 * \fn void rbc_elt_nth_root(rbc_elt o, const rbc_elt e, uint32_t n)
 * \brief This function computes the nth root of a finite field element.
 *
 * \param[out] o Nth root of <b>e</b>
 * \param[in] e rbc_elt
 * \param[in] n Parameter defining the nth root
 */
void rbc_elt_nth_root(rbc_elt o, const rbc_elt e, uint32_t n) {
  uint32_t exp = n * (RBC_163_FIELD_M - 1) % RBC_163_FIELD_M;

  if (exp == 0) {
    rbc_elt_set(o, e);
    return;
  }

  rbc_elt_sqr(o, e);
  for(size_t i = 0 ; i < exp - 1 ; ++i) {
    rbc_elt_sqr(o, o);
  }
}


/**
 * \fn void rbc_elt_reduce(rbc_elt o, const rbc_elt_ur e)
 * \brief This function reduces a finite field element.
 *
 * \param[out] o rbc_elt equal to $ e \pmod f $
 * \param[in] e rbc_elt
 */
void rbc_elt_reduce(rbc_elt o, const rbc_elt_ur e) {
  rbc_elt_ur e2;
  rbc_elt_ur_set(e2, e);

  e2[3] ^= (e2[5] >> 28) ^ (e2[5] >> 29) ^ (e2[5] >> 32) ^ (e2[5] >> 35);

  e2[2] ^= (e2[5] << 36) ^ (e2[5] << 35) ^ (e2[5] << 32) ^ (e2[5] << 29) ^ (e2[4] >> 28) ^ (e2[4] >> 29) ^ (e2[4] >> 32) ^ (e2[4] >> 35);

  e2[1] ^= (e2[4] << 36) ^ (e2[4] << 35) ^ (e2[4] << 32) ^ (e2[4] << 29) ^ (e2[3] >> 28) ^ (e2[3] >> 29) ^ (e2[3] >> 32) ^ (e2[3] >> 35);

  e2[0] ^= (e2[3] << 36) ^ (e2[3] << 35) ^ (e2[3] << 32) ^ (e2[3] << 29);

  uint64_t tmp = (e2[2] >> 35);
  e2[0] ^= (tmp << 7) ^ (tmp << 6) ^ (tmp << 3) ^ (tmp << 0);

  rbc_elt_set(o, e2);

  o[2] &= 0x7FFFFFFFF;
}

/**
 * \fn void rbc_elt_print(const rbc_elt e)
 * \brief This function displays a finite field element.
 *
 * \param[in] e rbc_elt
 */
void rbc_elt_print(const rbc_elt e) {
  printf("[");
  printf(" %16" PRIx64 " %16" PRIx64 " %16" PRIx64 , e[0], e[1], e[2]);
  printf(" ]");
}

/**
 * \fn void rbc_elt_ur_set(rbc_elt_ur o, const rbc_elt_ur e)
 * \brief This function copies an unreduced finite field element into another one.
 *
 * \param[out] o rbc_elt
 * \param[in] e rbc_elt
 */
void rbc_elt_ur_set(rbc_elt_ur o, const rbc_elt_ur e) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_UR_SIZE ; i++) {
    o[i] = e[i];
  }
}

/**
 * \fn void rbc_elt_ur_set_zero(rbc_elt_ur o)
 * \brief This function sets an unreduced finite field element to zero.
 *
 * \param[out] o rbc_elt_ur
 */
void rbc_elt_ur_set_zero(rbc_elt_ur o) {
  for(uint8_t i = 0 ; i < RBC_163_ELT_UR_SIZE ; i++) {
    o[i] = 0;
  }
}

/**
 * \fn void rbc_elt_ur_set_from_uint64(rbc_elt_ur o, uint64_t* e)
 * \brief This function set an unreduced finite field element from a pointer to uint64_t.
 *
 * \param[out] o rbc_elt_ur
 * \param[in] e Pointer to uint64_t
 */
void rbc_elt_ur_set_from_uint64(rbc_elt_ur o, const uint64_t* e) {
  rbc_elt_ur_set(o, e);
}

/**
 * \fn void rbc_elt_ur_mul(rbc_elt_ur o, const rbc_elt e1, const rbc_elt e2)
 * \brief This function computes the unreduced multiplication of two finite field elements.
 *
 * \param[out] o rbc_elt equal to \f$ e_1 \times e_2 $
 * \param[in] e1 rbc_elt
 * \param[in] e2 rbc_elt
 */
void rbc_elt_ur_mul(rbc_elt_ur o, const rbc_elt e1, const rbc_elt e2) {
  uint64_t shifts[64][RBC_163_ELT_SIZE + 1];
  rbc_elt_set(shifts[0], e2);
  shifts[0][RBC_163_ELT_SIZE] = 0;
  
  for(uint8_t shift=1 ; shift<64 ; shift++) {
    shifts[shift][0] = shifts[shift-1][0] << 1;
    for(uint8_t i=1 ; i<RBC_163_ELT_SIZE + 1 ; i++) {
      shifts[shift][i] = (shifts[shift-1][i] << 1) | (shifts[shift-1][i-1] >> 63);
    }
  }
  
  rbc_elt_ur_set_zero(o);
  for(uint8_t i=0 ; i<RBC_163_FIELD_M ; i++) {
    uint8_t shift = i % 64;
    uint8_t offset = i / 64;
    uint64_t multiplier = (e1[offset] >> shift) & 0x1;
    for(uint8_t j=0 ; j<RBC_163_ELT_SIZE + 1 ; j++) {
      o[j+offset] ^= multiplier * shifts[shift][j];
    }
  }
}

/**
 * \fn void rbc_elt_ur_sqr(rbc_elt o, const rbc_elt e)
 * \brief This function computes the unreduced square of a finite field element.
 *
 * \param[out] o rbc_elt_ur equal to $ e^{2} $
 * \param[in]  e rbc_elt
*/
void rbc_elt_ur_sqr(rbc_elt_ur o, const rbc_elt e) {
  rbc_elt_ur_mul(o, e, e);
}




/**
 * \fn void rbc_elt_to_string(uint8_t* str, const rbc_elt e)
 * \brief This function parses a finite field element into a string.
 *
 * \param[out] str Output string
 * \param[in] e rbc_elt
 */
void rbc_elt_to_string(uint8_t* str, const rbc_elt e) {
  uint32_t bytes1 = RBC_163_FIELD_M / 8;
  uint32_t bytes2 = RBC_163_FIELD_M % 8;

  memset(str, 0, RBC_163_ELT_UINT8_SIZE);
  memcpy(str, e, bytes1);

  uint8_t k = 0;
  for(size_t j = 1 ; j <= bytes2 ; j++) {
    uint8_t bit = rbc_elt_get_coefficient(e, RBC_163_FIELD_M - j);
    *(str + bytes1) |= (bit << k % 8);
    k++;
    if(k % 8 == 0) bytes1++;
  }
}




/**
 * \fn void rbc_elt_from_string(rbc_elt e, const uint8_t* str)
 * \brief This function parses a string into a finite field element.
 *
 * \param[out] e rbc_elt
 * \param[in] str String to parse
 */
void rbc_elt_from_string(rbc_elt e, const uint8_t* str) {
  uint32_t bytes1 = RBC_163_FIELD_M / 8;
  uint32_t bytes2 = RBC_163_FIELD_M % 8;

  rbc_elt_set_zero(e);
  memcpy(e, str, bytes1);

  uint8_t k = 0;
  for(size_t j = 1 ; j <= bytes2 ; j++) {
    uint8_t bit = (str[bytes1] >> k % 8) & 0x01;
    rbc_elt_set_coefficient_vartime(e, RBC_163_FIELD_M - j, bit);
    k++;
    if(k % 8 == 0) bytes1++;
  }
}

/**
 * \fn void rbc_elt_ur_print(const rbc_elt_ur e)
 * \brief This function displays an unreduced finite field element.
 *
 * \param[in] e rbc_elt_ur
 */
void rbc_elt_ur_print(const rbc_elt_ur e) {
  printf("[");
  printf(" %16" PRIx64 " %16" PRIx64 " %16" PRIx64 , e[0], e[1], e[2]);
  printf(" ]");
}

