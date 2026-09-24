#include "tsuov.h"
#include <x86intrin.h>

typedef __m256i Fq_vec ;



inline static Fq_vec Fq_vec_dot_accumulate(Fq_vec a, Fq_vec b, Fq_vec c){
  __m256i d = _mm256_maddubs_epi16(a,b) ; // Fq_vec_PRINT(d) ;
  __m256i e = _mm256_set1_epi16(1)      ; // Fq_vec_PRINT(e) ;
  __m256i f = _mm256_madd_epi16 (d,e)   ; // Fq_vec_PRINT(f) ;
  __m256i g = _mm256_add_epi32 (f,c)    ; // Fq_vec_PRINT(g) ;
  return g ;
}

inline static uint64_t Fq_vec_dot_accumulate_final(Fq_vec c){
  __m128i lo = _mm256_castsi256_si128(c) ;
  __m128i hi = _mm256_extracti128_si256(c, 1) ;
  __m128i s1 = _mm_hadd_epi32(lo, hi) ;
  __m128i s2 = _mm_hadd_epi32(s1, s1) ;
  __m128i s3 = _mm_hadd_epi32(s2, s2) ;
  return ((uint64_t)_mm_extract_epi32(s3, 0)) ;
}

// =============================================================================
//
// =============================================================================


inline static uint64_t vector_V_dot_vector_V(const vector_V A, const vector_V B){
  Fq_vec * a = (Fq_vec *) A ;
  Fq_vec * b = (Fq_vec *) B ;
  Fq_vec   c ; c = _mm256_set1_epi32(0) ;
  for(int j = 0 ; j < BYTES2BLOCKS(TSUOV_V) ; j++) c = Fq_vec_dot_accumulate(*a++, *b++, c) ;
  uint64_t C = Fq_vec_dot_accumulate_final(c) ;   // sun 8 32-bit elements
  return C ;
}




// =============================================================================
//
// =============================================================================


void vector_mul_scalar_avx2_fp31(const uint8_t *vec, uint8_t scalar, size_t len, uint8_t *res) {
    __m256i v_scalar = _mm256_set1_epi16(scalar);
    __m256i v_31 = _mm256_set1_epi16(31);
    __m256i v_barrett = _mm256_set1_epi16(2114);

    for (size_t i = 0; i < len; i += 32) { 
        
        __m256i raw = _mm256_loadu_si256((__m256i*)(vec + i));
        
        __m256i v_vec_l = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(raw, 0));
        __m256i v_vec_h = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(raw, 1));

       
        __m256i mul_l = _mm256_mullo_epi16(v_vec_l, v_scalar);
        __m256i mul_h = _mm256_mullo_epi16(v_vec_h, v_scalar);

        // Barrett 
        __m256i q_l = _mm256_mulhi_epu16(mul_l, v_barrett);
        __m256i q_h = _mm256_mulhi_epu16(mul_h, v_barrett);
        
        __m256i rem_l = _mm256_sub_epi16(mul_l, _mm256_mullo_epi16(q_l, v_31));
        __m256i rem_h = _mm256_sub_epi16(mul_h, _mm256_mullo_epi16(q_h, v_31));

        
        __m256i packed = _mm256_packus_epi16(rem_l, rem_h);
        
        packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
        
        _mm256_storeu_si256((__m256i*)(res + i), packed);
    }
}


//Mersenne
void vector_mod31_fast_avx2(const uint8_t *vec, size_t len, uint8_t *res) {
    __m256i v_31 = _mm256_set1_epi8(31);
    __m256i v_30 = _mm256_set1_epi8(30);
    __m256i v_h_mask = _mm256_set1_epi8(0x07);
    
    for (size_t i = 0; i < len; i += 32) {
        __m256i a = _mm256_loadu_si256((__m256i*)(vec + i));

        // 1. a = (a >> 5) + (a & 31)
        __m256i h = _mm256_srli_epi16(a, 5); 
        h = _mm256_and_si256(h, v_h_mask);
        __m256i l = _mm256_and_si256(a, v_31);
        
        __m256i sum = _mm256_add_epi8(h, l); 

        // 2. if sum > 30, then sub 31
        __m256i is_greater = _mm256_cmpgt_epi8(sum, v_30);
        __m256i fix = _mm256_and_si256(is_greater, v_31);
        __m256i r = _mm256_sub_epi8(sum, fix);

        _mm256_storeu_si256((__m256i*)(res + i), r);
    }
}

void vector_add_avx2_no_mod(const uint8_t *a, const uint8_t *b, size_t len, uint8_t *res) {
    for (size_t i = 0; i < len; i += 32) {
        __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
        __m256i vres = _mm256_add_epi8(va, vb); 
        _mm256_storeu_si256((__m256i*)(res + i), vres);
    }
}

uint64_t vector_v_dot_vector_v(const VECTOR_V A, const VECTOR_V B){
  uint64_t ci = 0;
  for(int l=0; l < TSUOV_L; l++) {
    ci += vector_V_dot_vector_V(A[l], B[l]);   
  }
  return ci;
}

/**
 * @brief res = (a + b) % 31
 * restriction: a[i], b[i] < 31
 */
void vector_add_mod31_avx2( const uint8_t *a, const uint8_t *b, size_t len, uint8_t *res) {
    __m256i v_31 = _mm256_set1_epi8(31);
    __m256i v_h_mask = _mm256_set1_epi8(0x07); // (60 >> 5 = 1)

    for (size_t i = 0; i < len; i += 32) {
        
        __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
        __m256i sum_raw = _mm256_add_epi8(va, vb);

        // (sum >> 5) + (sum & 31)
        __m256i h = _mm256_srli_epi16(sum_raw, 5); 
        h = _mm256_and_si256(h, v_h_mask);
        __m256i l = _mm256_and_si256(sum_raw, v_31);
        __m256i sum_f = _mm256_add_epi8(h, l); //  [0, 31]

        
        // if (sum_f > 30) sum_f -= 31
        __m256i mask = _mm256_cmpgt_epi8(sum_f, _mm256_set1_epi8(30));
        __m256i fix = _mm256_and_si256(mask, v_31);
        __m256i final_res = _mm256_sub_epi8(sum_f, fix);

        _mm256_storeu_si256((__m256i*)(res + i), final_res);
    }
}


void MATRIX_TRANSPOSE_m2xLO(const Fq A[TSUOV_m1][TSUOV_k][TSUOV_o], 
  Fq  C[TSUOV_k*TSUOV_o][TSUOV_m2]) 
{
    for (int i = 0; i < TSUOV_m1; i++) {
      for (int k = 0; k < TSUOV_k; k++) {
        for (int lo = 0; lo < (TSUOV_o); lo++) {
        C[k*TSUOV_o + lo][i] = A[i][k][lo];
        }
      }
    }
    for (int k = 0; k < TSUOV_k; k++) {
      for (int lo = 0; lo < (TSUOV_o); lo++) {
        memset(C[k*TSUOV_o + lo] + TSUOV_m1, 0, (TSUOV_m2 - TSUOV_m1) * sizeof(Fq));
      }
    }
}



static void whipk_accumulate_o_terms(
  vector_V acc,
  const MATRIX_OxV A,
  int l1,
  const MATRIX_kxO a,
  int i,
  int l2
){
  vector_V tmp ;
#if (TSUOV_O <= 8)
  for(int j = 0; j < TSUOV_O; j++) {
    vector_mul_scalar_avx2_fp31(A[j][l1], a[i][l2][j], BLOCK_ALIGNED_BYTES(TSUOV_V), tmp);
    vector_add_avx2_no_mod(acc, tmp, BLOCK_ALIGNED_BYTES(TSUOV_V), acc);
  }
  vector_mod31_fast_avx2(acc, BLOCK_ALIGNED_BYTES(TSUOV_V), acc);
#else
  for(int j = 0; j < TSUOV_O; j++) {
    vector_mul_scalar_avx2_fp31(A[j][l1], a[i][l2][j], BLOCK_ALIGNED_BYTES(TSUOV_V), tmp);
    vector_add_mod31_avx2(acc, tmp, BLOCK_ALIGNED_BYTES(TSUOV_V), acc);
  }
#endif
}

void MATRIX_VxO_dot_VECTOR_O_whipk(const MATRIX_OxV A, const MATRIX_kxO a, MATRIX_kxV res){
  vector_V T[2*TSUOV_L-1];
  for(int i = 0; i < TSUOV_k; i++){
    for(int l = 0; l < 2*TSUOV_L-1; l++){
        memset(T[l], 0, sizeof(T[l]));
      }
    for(int l1 = 0 ; l1 < TSUOV_L ; l1++)
      for(int l2 = 0 ; l2 < TSUOV_L ; l2++)
        whipk_accumulate_o_terms(T[l1+l2], A, l1, a, i, l2);
#if (TSUOV_L == 2)
    vector_mul_scalar_avx2_fp31(T[2], 3, BLOCK_ALIGNED_BYTES(TSUOV_V), T[2]);
    vector_add_mod31_avx2(T[0], T[2], BLOCK_ALIGNED_BYTES(TSUOV_V), T[0]);
#else
#  error "MATRIX_VxO_dot_VECTOR_O_whipk: unsupported TSUOV_L (whipk reduction)."
#endif
    for(int l = 0; l < TSUOV_L; l++){
      memcpy(res[i][l], T[l], sizeof(T[l]));
    }
  }
}


/**
 * @brief (res = (a - b) % 31)
 * res = (a + 31 - b) % 31
 */
inline static void vector_sub_mod31_avx2(const uint8_t *a, const uint8_t *b, size_t len, uint8_t *res) {
    __m256i v_31 = _mm256_set1_epi8(31);
    __m256i v_30 = _mm256_set1_epi8(30);
    __m256i v_h_mask = _mm256_set1_epi8(0x07);

    for (size_t i = 0; i < len; i += 32) {
        __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));

        // tmp = a + 31 - b
        // 30 + 31 - 0 = 61
        __m256i v_sub = _mm256_sub_epi8(v_31, vb); // (31 - b)
        __m256i sum_raw = _mm256_add_epi8(va, v_sub); // a + (31 - b)

        // (sum >> 5) + (sum & 31)
        __m256i h = _mm256_srli_epi16(sum_raw, 5); 
        h = _mm256_and_si256(h, v_h_mask);
        __m256i l = _mm256_and_si256(sum_raw, v_31);
        __m256i sum_f = _mm256_add_epi8(h, l); //  [0, 31]

        // 
        __m256i mask = _mm256_cmpgt_epi8(sum_f, v_30);
        __m256i fix = _mm256_and_si256(mask, v_31);
        __m256i final_res = _mm256_sub_epi8(sum_f, fix);

        _mm256_storeu_si256((__m256i*)(res + i), final_res);
    }
}

void VECTOR_V_sub_VECTOR_V_whipk_save_in_sign(const MATRIX_kxV a, const MATRIX_kxV b, MATRIX_kxN sign){
  for(int i = 0; i < TSUOV_k; i++) {
    for(int l = 0; l < TSUOV_L; l++) {
      vector_sub_mod31_avx2(a[i][l], b[i][l], BLOCK_ALIGNED_BYTES(TSUOV_V), sign[i][l]);
    } 
  }
}


