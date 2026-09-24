#include "inverse_avx2.h"

static inline void shuffle8_avx2(__m256i *a, __m256i *b) {
    __m256i c = _mm256_permute2x128_si256(*a, *b, 0x20);

    *b = _mm256_permute2x128_si256(*a, *b, 0x31); // hi hi  8-15 56-63
    *a = c;                                       // lo lo  0- 7 48-55
}

static inline void shuffle4_avx2(__m256i *a, __m256i *b) {
    __m256i c = _mm256_unpacklo_epi64(*a, *b);
    *b = _mm256_unpackhi_epi64(*a, *b);
    *a = c;
}

static inline void shuffle2_avx2(__m256i *a, __m256i *b)
{
    __m256i b_shift = _mm256_slli_epi64(*b, 32);         // vpsllq $32,rh1 -> rh2
    __m256i a_shift = _mm256_srli_epi64(*a, 32); // vpsrlq $32,rh0 -> rh0
    *a = _mm256_blend_epi32(*a, b_shift, 0xAA);          // vpblendd $0xAA,rh2,rh0 -> rh2
    *b = _mm256_blend_epi32(a_shift, *b, 0xAA);  // vpblendd $0xAA,rh1,rh0 -> rh3
}

static inline void shuffle1_avx2(__m256i *a, __m256i *b)
{
    __m256i b_shift = _mm256_slli_epi32(*b, 16);         // vpslld $16,rh1 -> rh2
    __m256i a_shift = _mm256_srli_epi32(*a, 16); // vpsrld $16,rh0 -> rh0
    *a = _mm256_blend_epi16(*a, b_shift, 0xAA);          // vpblendw $0xAA,rh2,rh0 -> rh2
    *b = _mm256_blend_epi16(a_shift, *b, 0xAA);  // vpblendw $0xAA,rh1,rh0 -> rh3
}

static inline __m256i fqmul_avx2(__m256i l, __m256i h) 
{
    __m256i temp1 = _mm256_mulhi_epi16(l, h);          // vpmulhw
    __m256i temp2 = _mm256_mullo_epi16(l, h);          // vpmullw
    __m256i temp3 = _mm256_mullo_epi16(qinv_vec, temp2); // vpmullw with qinv
    temp3 = _mm256_mulhi_epi16(q_vec, temp3);          // vpmulhw with q
    temp3 = _mm256_sub_epi16(temp1, temp3);             // vpsubw
    return temp3;
}

static inline void barrett_reduce_avx2(__m256i *input){
    __m256i tmp0;
    tmp0 = _mm256_mulhi_epi16(v_vec, *input);
    tmp0 = _mm256_srai_epi16(tmp0, 10);
    tmp0 = _mm256_mullo_epi16(q_vec, tmp0);
    *input = _mm256_sub_epi16(*input, tmp0);
}

static inline __m256i fqinv_avx2(__m256i a)
{
    __m256i constR2 = _mm256_set1_epi16(R2);  // R^2 mod q
    __m256i t = _mm256_set1_epi16(1);            // 初始化 t = 1
    int16_t exp;

    // a = fqmul(a, 867)
    a = fqmul_avx2(a, constR2);

    // 费马小定理: a^(q-1) ≡ 1 (mod q), 所以 a^(-1) ≡ a^(q-2) (mod q)
    for (exp = DTRU_Q - 2; exp > 0; exp >>= 1)
    {
        if (exp & 1) {
            t = fqmul_avx2(t, a);
        }
        a = fqmul_avx2(a, a);
    }

    return t;
}

#define CAL_S_avx2(a, i, j) fqmul_avx2(a[i], a[j])

// AVX2版本的 rq_inverse_recursive
int rq_inverse_recursive_avx2(__m256i b_vec[4], const __m256i a_vec[4], __m256i zeta_vec)           
{
    __m256i a0[2], a1[2];
    __m256i d[4];
    __m256i det;
    __m256i temp, temp2;
    __m256i const_2 = _mm256_set1_epi16(2);
    __m256i const_neg1 = _mm256_set1_epi16(-1);
    
    a0[0] = _mm256_sub_epi16(CAL_S_avx2(a_vec, 2, 2), _mm256_mullo_epi16(const_2, CAL_S_avx2(a_vec, 1, 3)));
    a0[0] = fqmul_avx2(a0[0], zeta_vec);
    a0[0] = _mm256_add_epi16(a0[0], CAL_S_avx2(a_vec, 0, 0));
    
    a0[1] = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 1, 1), const_neg1);
    a0[1] = _mm256_sub_epi16(a0[1], fqmul_avx2(CAL_S_avx2(a_vec, 3, 3), zeta_vec));
    a0[1] = _mm256_add_epi16(a0[1], _mm256_mullo_epi16(const_2, CAL_S_avx2(a_vec, 0, 2)));

    det = CAL_S_avx2(a0, 1, 1);                            

    temp = _mm256_mullo_epi16(zeta_vec, const_neg1);          // -zeta
    det = fqmul_avx2(det, temp);
    
    // det += CAL_S(a0, 0, 0) = a0[0] * a0[0]
    temp = CAL_S_avx2(a0, 0, 0);                           
    det = _mm256_add_epi16(det, temp);
    
    // det = barrett_reduce(det)
     barrett_reduce_avx2(&det);
    
    // det = fqinv(det)
    det = fqinv_avx2(det);
    
    // 计算 a1[0] = fqmul(a0[0], det)
    a1[0] = fqmul_avx2(a0[0], det);
    
    // 计算 a1[1] = fqmul(-a0[1], det)
    temp = _mm256_mullo_epi16(a0[1], const_neg1);
    a1[1] = fqmul_avx2(temp, det);
    
    // 计算 d[i] = fqmul(a1[i>>1], a[i])
    d[0] = fqmul_avx2(a1[0], a_vec[0]);
    d[1] = fqmul_avx2(a1[0], a_vec[1]);
    d[2] = fqmul_avx2(a1[1], a_vec[2]);
    d[3] = fqmul_avx2(a1[1], a_vec[3]);
    
    // 计算 b[0] = d[0] + fqmul(d[2], zeta)
    temp = fqmul_avx2(d[2], zeta_vec);
    b_vec[0] = _mm256_add_epi16(d[0], temp);
    
    // 计算 b[1] = -d[1] + fqmul(-d[3], zeta)
    temp = _mm256_mullo_epi16(d[3], const_neg1);
    temp = fqmul_avx2(temp, zeta_vec);
    temp2 = _mm256_mullo_epi16(d[1], const_neg1);
    b_vec[1] = _mm256_add_epi16(temp2, temp);
    
    // 计算 b[2] = fqmul(a1[0] + a1[1], a[0] + a[2]) - d[0] - d[2]
    temp = _mm256_add_epi16(a1[0], a1[1]);                      // a1[0] + a1[1]
    temp2 = _mm256_add_epi16(a_vec[0], a_vec[2]);            // a[0] + a[2]
    temp = fqmul_avx2(temp, temp2);
    temp = _mm256_sub_epi16(temp, d[0]);
    b_vec[2] = _mm256_sub_epi16(temp, d[2]);
    
    // 计算 b[3] = fqmul(a1[0] + a1[1], -a[1] - a[3]) + d[1] + d[3]
    temp = _mm256_add_epi16(a1[0], a1[1]);                      // a1[0] + a1[1]
    temp2 = _mm256_add_epi16(a_vec[1], a_vec[3]);            // a[1] + a[3]
    temp2 = _mm256_mullo_epi16(temp2, const_neg1);           // -(a[1] + a[3])
    temp = fqmul_avx2(temp, temp2);
    temp = _mm256_add_epi16(temp, d[1]);
    b_vec[3] = _mm256_add_epi16(temp, d[3]);
    
    // 计算返回值: r = (uint16_t)det; r = (uint32_t)(-r) >> 31;
    // 这相当于检查 det 是否为0: det==0 则返回-1, 否则返回0
    int r = _mm256_testz_si256(det, det) ? -1 : 0;
    
    return r;  // 返回-1(失败)或0(成功)的向量
}

int baseinv_avx2(int16_t *b, const int16_t *a, const int16_t *zetas){
    int r = 0;
  for(int i = 0; i< DTRU_N/(4*16);i++){
    __m256i b_vec[4],a_vec[4],zeta_vec;
    for(int j = 0; j < 4; ++j)
    {
      a_vec[j] = _mm256_loadu_si256((__m256i *) (a + i * 64 + 16 * j));
    }
    zeta_vec = _mm256_loadu_si256((__m256i *) (zetas + i * 16));
    shuffle2_avx2(&a_vec[0], &a_vec[2]);
    shuffle2_avx2(&a_vec[1], &a_vec[3]);
    shuffle1_avx2(&a_vec[0], &a_vec[1]);
    shuffle1_avx2(&a_vec[2], &a_vec[3]);
    
    r += rq_inverse_recursive_avx2(b_vec,a_vec,zeta_vec);
    
    shuffle1_avx2(&b_vec[0], &b_vec[1]);
    shuffle1_avx2(&b_vec[2], &b_vec[3]);
    shuffle2_avx2(&b_vec[0], &b_vec[2]);
    shuffle2_avx2(&b_vec[1], &b_vec[3]);

    if(r != 0)   return r;
    for(int j = 0; j < 4; ++j)
    {
      _mm256_storeu_si256((__m256i *) (b + i * 64 + 16 * j), b_vec[j]);
    }
  }
  return r;
}