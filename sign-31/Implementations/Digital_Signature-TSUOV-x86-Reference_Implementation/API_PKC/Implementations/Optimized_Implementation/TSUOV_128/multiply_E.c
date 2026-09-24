#include <stdint.h>
#include <stdlib.h>
#include "tsuov_params.h"
#include "Fql.h"
#include "fast_matrix_opt.h"
#include <string.h>
#include <immintrin.h>



typedef struct {
  __m256i T0;
  __m256i T1;
} F31_MulTable_avx2;

F31_MulTable_avx2 c0_table;
F31_MulTable_avx2 c1_table;
F31_MulTable_avx2 c2_table;

#if (TSUOV_security_strength_category == 1)

// c0 = 4
static const uint8_t c0_T0_data[32] = {
    0,4,8,12,16,20,24,28,1,5,9,13,17,21,25,29,
    0,4,8,12,16,20,24,28,1,5,9,13,17,21,25,29
};
static const uint8_t c0_T1_data[32] = {
    2,6,10,14,18,22,26,30,3,7,11,15,19,23,27,0,
    2,6,10,14,18,22,26,30,3,7,11,15,19,23,27,0
};

// c1 = 3
static const uint8_t c1_T0_data[32] = {
    0,3,6,9,12,15,18,21,24,27,30,2,5,8,11,14,
    0,3,6,9,12,15,18,21,24,27,30,2,5,8,11,14
};
static const uint8_t c1_T1_data[32] = {
    17,20,23,26,29,1,4,7,10,13,16,19,22,25,28,0,
    17,20,23,26,29,1,4,7,10,13,16,19,22,25,28,0
};

// c2 = 1
static const uint8_t c2_T0_data[32] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};
static const uint8_t c2_T1_data[32] = {
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0
};

#elif (TSUOV_security_strength_category == 3)

// c0 = 6
static const uint8_t c0_T0_data[32] = {
  0,6,12,18,24,30,5,11,17,23,29,4,10,16,22,28,
  0,6,12,18,24,30,5,11,17,23,29,4,10,16,22,28
};
static const uint8_t c0_T1_data[32] = {
  3,9,15,21,27,2,8,14,20,26,1,7,13,19,25,0,
  3,9,15,21,27,2,8,14,20,26,1,7,13,19,25,0
};

// c1 = 4
static const uint8_t c1_T0_data[32] = {
  0,4,8,12,16,20,24,28,1,5,9,13,17,21,25,29,
  0,4,8,12,16,20,24,28,1,5,9,13,17,21,25,29
};
static const uint8_t c1_T1_data[32] = {
  2,6,10,14,18,22,26,30,3,7,11,15,19,23,27,0,
  2,6,10,14,18,22,26,30,3,7,11,15,19,23,27,0
};

// c2 = 1
static const uint8_t c2_T0_data[32] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};
static const uint8_t c2_T1_data[32] = {
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0
};

#elif (TSUOV_security_strength_category == 5)

// c0 = 15

static const uint8_t c0_T0_data[32] = {
  0,15,30,14,29,13,28,12,27,11,26,10,25,9,24,8,
  0,15,30,14,29,13,28,12,27,11,26,10,25,9,24,8
};
static const uint8_t c0_T1_data[32] = {
  23,7,22,6,21,5,20,4,19,3,18,2,17,1,16,0,
  23,7,22,6,21,5,20,4,19,3,18,2,17,1,16,0
};


// c1 = 12
static const uint8_t c1_T0_data[32] = {
  0,12,24,5,17,29,10,22,3,15,27,8,20,1,13,25,
  0,12,24,5,17,29,10,22,3,15,27,8,20,1,13,25
};
static const uint8_t c1_T1_data[32] = {
  6,18,30,11,23,4,16,28,9,21,2,14,26,7,19,0,
  6,18,30,11,23,4,16,28,9,21,2,14,26,7,19,0
};

// c2 = 1
static const uint8_t c2_T0_data[32] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};
static const uint8_t c2_T1_data[32] = {
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0,
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,0
};

#endif

void init_mul_table_mod31() {
  c0_table.T0 = _mm256_load_si256((__m256i*)c0_T0_data);
  c0_table.T1 = _mm256_load_si256((__m256i*)c0_T1_data);
  c1_table.T0 = _mm256_load_si256((__m256i*)c1_T0_data);
  c1_table.T1 = _mm256_load_si256((__m256i*)c1_T1_data);
  c2_table.T0 = _mm256_load_si256((__m256i*)c2_T0_data);
  c2_table.T1 = _mm256_load_si256((__m256i*)c2_T1_data);
}


static inline __m256i _mm256_mod31_epi8(__m256i v) {
  __m256i q = _mm256_and_si256(_mm256_srli_epi16(v, 5), _mm256_set1_epi8(0x07));
  __m256i r = _mm256_and_si256(v, _mm256_set1_epi8(31));
  __m256i sum = _mm256_add_epi8(q, r);
  __m256i cmp = _mm256_cmpgt_epi8(sum, _mm256_set1_epi8(30)); 
  __m256i sub = _mm256_and_si256(cmp, _mm256_set1_epi8(31));
  return _mm256_sub_epi8(sum, sub);
}

static inline __m256i _mm256_mul_table_mod31_epi8(__m256i v, F31_MulTable_avx2 table_avx2) {
  __m256i res0 = _mm256_shuffle_epi8(table_avx2.T0, v); 
  __m256i res1 = _mm256_shuffle_epi8(table_avx2.T1, v); 
  __m256i mask = _mm256_cmpgt_epi8(v, _mm256_set1_epi8(15));
  return _mm256_blendv_epi8(res0, res1, mask);
}



/**
 * @param m length of u
 * @param u input vector
 * @param u_new output vector, u_new = E^ell * u and the address of u_new not equal to u
 * @param ell power of E
 */
void multiply_E_vec_avx2(const int m, uint8_t *u, uint8_t *u_new, int ell) {

  memset(u_new, 0, ell);
  memcpy(u_new + ell, u, TSUOV_m2 - ell); 

  int i = 0;
  for (; i <= ell - 32; i += 32) {
      int idx = m-ell + i;
      __m256i U0 = _mm256_loadu_si256((__m256i*)&u[idx]);
      __m256i U1 = _mm256_loadu_si256((__m256i*)&u[idx- 1]);
      __m256i U2 = _mm256_loadu_si256((__m256i*)&u[idx- 2]);

      if (i == 0) {
          __m256i mask1 = _mm256_setr_epi8(
              0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
              -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
          __m256i mask2 = _mm256_setr_epi8(
              0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
              -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
          U1 = _mm256_and_si256(U1, mask1);
          U2 = _mm256_and_si256(U2, mask2);
      }

      __m256i R0 = _mm256_mul_table_mod31_epi8(U0, c0_table);
      __m256i R1 = _mm256_mul_table_mod31_epi8(U1, c1_table);
      __m256i R2 = _mm256_mul_table_mod31_epi8(U2, c2_table);

      __m256i unew = _mm256_loadu_si256((__m256i*)&u_new[i]);
      unew = _mm256_add_epi8(unew, R0);
      unew = _mm256_add_epi8(unew, R1);
      unew = _mm256_add_epi8(unew, R2);
      _mm256_storeu_si256((__m256i*)&u_new[i], unew);
  }

  // tail

  for (int j = i; j < ell + 2; j++) {
      int idx = m-ell + j;
      uint8_t u0 = (j < ell) ? u[idx] : 0;
      uint8_t u1 = (j >= 1 && j - 1 < ell) ? u[idx-1] : 0;
      uint8_t u2 = (j >= 2 && j - 2 < ell) ? u[idx-2] : 0;
      

      u_new[j] += ((u0 < 16) ? c0_T0_data[u0] : c0_T1_data[u0]);
      u_new[j] += ((u1 < 16) ? c1_T0_data[u1] : c1_T1_data[u1]);
      u_new[j] += ((u2 < 16) ? c2_T0_data[u2] : c2_T1_data[u2]);
  }


  // mod31
  int j = 0;
  for (; j <= TSUOV_m2 - 32; j += 32) {
      __m256i unew = _mm256_loadu_si256((__m256i*)&u_new[j]);
      unew = _mm256_mod31_epi8(unew);
      _mm256_storeu_si256((__m256i*)&u_new[j], unew);
  }
  for (; j < TSUOV_m2; j++) {
      u_new[j] = u_new[j] % 31;
  }
}

/**
* @param u len is m1
* @param u_acc len is m2
* @param ell power of E
* @return u_acc = u_acc + E^ell * u, len is m2
*/

void multiply_E_add_m1_m2_avx2(Fq *u, Fq * u_acc, int ell) {
  Fq u_m2 [TSUOV_m2];
  memcpy(u_m2, u, TSUOV_m1);
  memset(u_m2 + TSUOV_m1, 0, (TSUOV_m2 - TSUOV_m1)*sizeof(Fq));
  Fq u_new_temp[TSUOV_m2];
  multiply_E_vec_avx2(TSUOV_m2, u_m2, u_new_temp, ell);
  vector_add(u_acc, u_new_temp, TSUOV_m2);
}



/**
 * @param M len is m_col * TSUOV_m2, column major
 * @param M_new len is m_col * TSUOV_m2, column major
 * @param ctr power of E
 * @param m_col number of columns of M
 * @return M_new = E^ctr * M, len is m_col * TSUOV_m2, column major
 */

 void multiply_E_mat_m2_avx2(const uint8_t *M, uint8_t *M_new, int ctr, int m_col) {
  int ell = ctr;
  int shift = TSUOV_m2 - ell;

  __m256i mask1 = _mm256_setr_epi8(
      0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
  __m256i mask2 = _mm256_setr_epi8(
      0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

  for (int col = 0; col < m_col; col++) {
      const uint8_t *u = M + col * TSUOV_m2;
      uint8_t *u_new = M_new + col * TSUOV_m2;

      memset(u_new, 0, ell);
      memcpy(u_new + ell, u, TSUOV_m2 - ell); 

 
      int i = 0;
      for (; i <= ell - 32; i += 32) {
          __m256i U0 = _mm256_loadu_si256((__m256i*)(u + shift + i));
          __m256i U1 = _mm256_loadu_si256((__m256i*)(u + shift + i - 1));
          __m256i U2 = _mm256_loadu_si256((__m256i*)(u + shift + i - 2));

          if (i == 0) {
              U1 = _mm256_and_si256(U1, mask1);
              U2 = _mm256_and_si256(U2, mask2);
          }

          __m256i R0 = _mm256_mul_table_mod31_epi8(U0, c0_table);
          __m256i R1 = _mm256_mul_table_mod31_epi8(U1, c1_table);
          __m256i R2 = _mm256_mul_table_mod31_epi8(U2, c2_table);

          __m256i unew = _mm256_loadu_si256((__m256i*)(u_new + i));
          unew = _mm256_add_epi8(unew, R0);
          unew = _mm256_add_epi8(unew, R1);
          unew = _mm256_add_epi8(unew, R2);
          _mm256_storeu_si256((__m256i*)(u_new + i), unew);
      }

      // tail
      for (int j = i; j < ell + 2; j++) {
          uint8_t u0 = (j < ell) ? u[shift + j] : 0;
          uint8_t u1 = (j >= 1 && j - 1 < ell) ? u[shift + j - 1] : 0;
          uint8_t u2 = (j >= 2 && j - 2 < ell) ? u[shift + j - 2] : 0;
        
          u_new[j] += ((u0 < 16) ? c0_T0_data[u0] : c0_T1_data[u0]);
          u_new[j] += ((u1 < 16) ? c1_T0_data[u1] : c1_T1_data[u1]);
          u_new[j] += ((u2 < 16) ? c2_T0_data[u2] : c2_T1_data[u2]);
      }


      int j = 0;
      for (; j <= TSUOV_m2 - 32; j += 32) {
          __m256i unew = _mm256_loadu_si256((__m256i*)(u_new + j));
          unew = _mm256_mod31_epi8(unew);
          _mm256_storeu_si256((__m256i*)(u_new + j), unew);
      }
      for (; j < TSUOV_m2; j++) {
          u_new[j] = u_new[j] % 31;
      }
  }
}


void multiply_E_add_mat_m2_avx2(Fq *M, Fq *M_new, int ctr, int m_col) {
  Fq temp[TSUOV_o][TSUOV_m2];
  multiply_E_mat_m2_avx2(M, (Fq *)temp, ctr, m_col);
  vector_add((Fq *)M_new, (Fq *)temp, TSUOV_o*TSUOV_m2);
}
