#include "inverse_avx2.h"
int16_t zetas4inv[DTRU_N/8]={
1987,-1987,3254,-3254,2648,-2648,1090,-1090,2454,-2454,2018,-2018,1026,-1026,438,-438,
1826,-1826,3347,-3347,2082,-2082,1374,-1374,2624,-2624,1383,-1383,1731,-1731,1770,-1770,
2825,-2825,1954,-1954,226,-226,986,-986,152,-152,449,-449,427,-427,1557,-1557,
2229,-2229,1740,-1740,1008,-1008,1522,-1522,2177,-2177,2951,-2951,589,-589,2172,-2172,
2621,-2621,2716,-2716,2837,-2837,79,-79,2214,-2214,1491,-1491,3081,-3081,3438,-3438,
126,-126,2783,-2783,1946,-1946,1882,-1882,1370,-1370,2000,-2000,801,-801,160,-160,
2853,-2853,1036,-1036,2579,-2579,636,-636,2377,-2377,2814,-2814,605,-605,3129,-3129,
2721,-2721,919,-919,2845,-2845,2286,-2286,1271,-1271,1048,-1048,2729,-2729,3126,-3126
};

static __m256i fqmul_avx2(__m256i l, __m256i h) 
{
    __m256i temp1 = _mm256_mulhi_epi16(l, h);          // vpmulhw
    __m256i temp2 = _mm256_mullo_epi16(l, h);          // vpmullw
    __m256i temp3 = _mm256_mullo_epi16(qinv_vec, temp2); // vpmullw with qinv
    temp3 = _mm256_mulhi_epi16(q_vec, temp3);          // vpmulhw with q
    temp3 = _mm256_sub_epi16(temp1, temp3);             // vpsubw
    return temp3;
}

static void barrett_reduce_avx2(__m256i *input){
    __m256i tmp0;
    tmp0 = _mm256_mulhi_epi16(v_vec, *input);
    tmp0 = _mm256_srai_epi16(tmp0, 10);
    tmp0 = _mm256_mullo_epi16(q_vec, tmp0);
    *input = _mm256_sub_epi16(*input, tmp0);
}

__m256i fqinv_avx2(__m256i a)
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

#define CAL_D_avx2(a, b, x, y, d) \
    _mm256_sub_epi16(_mm256_sub_epi16( \
        fqmul_avx2(_mm256_add_epi16(a[x], a[y]), _mm256_add_epi16(b[x], b[y])), \
        d[x]), d[y])

// AVX2 版本的 basemul4
void basemul4_avx2(__m256i c_vec[4],const __m256i a_vec[4],const __m256i b_vec[4],__m256i zeta_vec)
{
    __m256i d[4];
    __m256i temp, temp2;
    
    // 计算 d[i] = fqmul(a[i], b[i])
    for (int i = 0; i < 4; i++)
        d[i] = fqmul_avx2(a_vec[i], b_vec[i]);

    // c[0] = barrett_reduce(d[0] + fqmul((CAL_D(a, b, 1, 3, d) + d[2]), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 1, 3, d);           // CAL_D(a, b, 1, 3, d)
    temp = _mm256_add_epi16(temp, d[2]);                 // + d[2]
    temp = fqmul_avx2(temp, zeta_vec);                   // fqmul(..., zeta)
    temp = _mm256_add_epi16(d[0], temp);                 // d[0] + ...
    barrett_reduce_avx2(&temp);
    c_vec[0] = temp;

    // c[1] = barrett_reduce(CAL_D(a, b, 0, 1, d) + fqmul(CAL_D(a, b, 2, 3, d), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 0, 1, d);           // CAL_D(a, b, 0, 1, d)
    temp2 = CAL_D_avx2(a_vec, b_vec, 2, 3, d);          // CAL_D(a, b, 2, 3, d)
    temp2 = fqmul_avx2(temp2, zeta_vec);                 // fqmul(..., zeta)
    temp = _mm256_add_epi16(temp, temp2);                // +
    barrett_reduce_avx2(&temp);
    c_vec[1] = temp;

    // c[2] = barrett_reduce(CAL_D(a, b, 0, 2, d) + d[1] + fqmul(d[3], zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 0, 2, d);           // CAL_D(a, b, 0, 2, d)
    temp = _mm256_add_epi16(temp, d[1]);                 // + d[1]
    temp2 = fqmul_avx2(d[3], zeta_vec);                  // fqmul(d[3], zeta)
    temp = _mm256_add_epi16(temp, temp2);                // +
     barrett_reduce_avx2(&temp);
    c_vec[2] = temp;

    // c[3] = barrett_reduce(CAL_D(a, b, 0, 3, d) + CAL_D(a, b, 1, 2, d))
    temp = CAL_D_avx2(a_vec, b_vec, 0, 3, d);           // CAL_D(a, b, 0, 3, d)
    temp2 = CAL_D_avx2(a_vec, b_vec, 1, 2, d);          // CAL_D(a, b, 1, 2, d)
    temp = _mm256_add_epi16(temp, temp2);                // +
     barrett_reduce_avx2(&temp);
    c_vec[3] = temp;
}

// AVX2版本的 rq_inverse_recursive
int rq_inverse_recursive4_avx2(__m256i b_vec[4], const __m256i a_vec[4], __m256i zeta_vec)           
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

// AVX2版本的 rq_inverse_recursive8
// 输入: a_vec[8] - 每个__m256i包含16个不同8元组的同一位置元素
// 输出: b_vec[8] - 同样的布局
// zeta_vec: 16个zeta值(如果相同可以用broadcast)
int rq_inverse_recursive8_avx2(__m256i b_vec[8], const __m256i a_vec[8], __m256i zeta_vec)
{
    __m256i a0[4], a1[4], c0[4], c1[4], b0[4], b1[4];
    __m256i temp, temp2, temp3;
    __m256i const_2 = _mm256_set1_epi16(2);
    __m256i const_neg1 = _mm256_set1_epi16(-1);
    int r;

    // 计算 a0[0] = CAL_S(a,0,0)+fqmul(zeta,2*CAL_S(a,2,6)+CAL_S(a,4,4)-2*CAL_S(a,1,7)-2*CAL_S(a,3,5))
    temp = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 2, 6), const_2);  // 2*CAL_S(a,2,6)
    temp = _mm256_add_epi16(temp, CAL_S_avx2(a_vec, 4, 4));       // + CAL_S(a,4,4)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 1, 7), const_2); // 2*CAL_S(a,1,7)
    temp = _mm256_sub_epi16(temp, temp2);                          // - 2*CAL_S(a,1,7)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 3, 5), const_2); // 2*CAL_S(a,3,5)
    temp = _mm256_sub_epi16(temp, temp2);                          // - 2*CAL_S(a,3,5)
    temp = fqmul_avx2(zeta_vec, temp);                             // fqmul(zeta, ...)
    a0[0] = _mm256_add_epi16(CAL_S_avx2(a_vec, 0, 0), temp);      // CAL_S(a,0,0) + ...

    // 计算 a0[1] = 2*CAL_S(a,0,2)-CAL_S(a,1,1)+fqmul(zeta,2*CAL_S(a,4,6)-2*CAL_S(a,3,7)-CAL_S(a,5,5))
    temp = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 0, 2), const_2);  // 2*CAL_S(a,0,2)
    temp = _mm256_sub_epi16(temp, CAL_S_avx2(a_vec, 1, 1));       // - CAL_S(a,1,1)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 4, 6), const_2); // 2*CAL_S(a,4,6)
    temp3 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 3, 7), const_2); // 2*CAL_S(a,3,7)
    temp2 = _mm256_sub_epi16(temp2, temp3);                        // 2*CAL_S(a,4,6) - 2*CAL_S(a,3,7)
    temp2 = _mm256_sub_epi16(temp2, CAL_S_avx2(a_vec, 5, 5));     // - CAL_S(a,5,5)
    temp2 = fqmul_avx2(zeta_vec, temp2);                           // fqmul(zeta, ...)
    a0[1] = _mm256_add_epi16(temp, temp2);

    // 计算 a0[2] = 2*CAL_S(a,0,4)+CAL_S(a,2,2)-2*CAL_S(a,1,3)+fqmul(zeta,CAL_S(a,6,6)-2*CAL_S(a,5,7))
    temp = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 0, 4), const_2);  // 2*CAL_S(a,0,4)
    temp = _mm256_add_epi16(temp, CAL_S_avx2(a_vec, 2, 2));       // + CAL_S(a,2,2)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 1, 3), const_2); // 2*CAL_S(a,1,3)
    temp = _mm256_sub_epi16(temp, temp2);                          // - 2*CAL_S(a,1,3)
    temp2 = CAL_S_avx2(a_vec, 6, 6);                               // CAL_S(a,6,6)
    temp3 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 5, 7), const_2); // 2*CAL_S(a,5,7)
    temp2 = _mm256_sub_epi16(temp2, temp3);                        // CAL_S(a,6,6) - 2*CAL_S(a,5,7)
    temp2 = fqmul_avx2(zeta_vec, temp2);                           // fqmul(zeta, ...)
    a0[2] = _mm256_add_epi16(temp, temp2);

    // 计算 a0[3] = 2*CAL_S(a,0,6)+2*CAL_S(a,2,4)-CAL_S(a,3,3)-2*CAL_S(a,1,5)-fqmul(zeta,CAL_S(a,7,7))
    temp = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 0, 6), const_2);  // 2*CAL_S(a,0,6)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 2, 4), const_2); // 2*CAL_S(a,2,4)
    temp = _mm256_add_epi16(temp, temp2);                          // 2*CAL_S(a,0,6) + 2*CAL_S(a,2,4)
    temp = _mm256_sub_epi16(temp, CAL_S_avx2(a_vec, 3, 3));       // - CAL_S(a,3,3)
    temp2 = _mm256_mullo_epi16(CAL_S_avx2(a_vec, 1, 5), const_2); // 2*CAL_S(a,1,5)
    temp = _mm256_sub_epi16(temp, temp2);                          // - 2*CAL_S(a,1,5)
    temp2 = fqmul_avx2(zeta_vec, CAL_S_avx2(a_vec, 7, 7));        // fqmul(zeta, CAL_S(a,7,7))
    a0[3] = _mm256_sub_epi16(temp, temp2);                         // - fqmul(...)

    // barrett_reduce for a0[i]
    for (int i = 0; i < 4; i++)
        barrett_reduce_avx2(&a0[i]);

    // 递归调用: a1 = a0^{-1} mod (x^4 - zeta)
    r = rq_inverse_recursive4_avx2(a1, a0, zeta_vec);

    // 计算 c0[i] = a[2*i], c1[i] = -a[2*i+1]
    for (int i = 0; i < 4; i++)
    {
        c0[i] = a_vec[2 * i];
        c1[i] = _mm256_mullo_epi16(a_vec[2 * i + 1], const_neg1);  // -a[2*i+1]
    }
    
    // basemul4(b0, c0, a1, zeta)
    basemul4_avx2(b0, c0, a1, zeta_vec);
    
    // basemul4(b1, c1, a1, zeta)
    basemul4_avx2(b1, c1, a1, zeta_vec);

    // 重新排列输出
    for (int i = 0; i < 4; i++)
    {
        b_vec[2 * i] = b0[i];
        b_vec[2 * i + 1] = b1[i];
    }

    return r;
}

static inline __m256i avx2_interleave_lanes_epi16(__m256i t) {
    __m256i t_swap = _mm256_permute2x128_si256(t, t, 0x01);
    __m256i lo = _mm256_unpacklo_epi16(t, t_swap);
    __m256i hi = _mm256_unpackhi_epi16(t, t_swap);

    return _mm256_permute2x128_si256(lo, hi, 0x20);
}

int baseinv_avx2(int16_t *b, const int16_t *a){
    int r = 0;
  for(int i = 0; i< DTRU_N/(8*16);i++){
    __m256i b_vec[8],a_vec[8],zeta_vec;
    for(int j = 0; j < 8; ++j)
    {
      a_vec[j] = _mm256_loadu_si256((__m256i *) (a + i * 128 + 16 * j));
    }
    
    __m256i t0, t1, t2, t3, t4, t5, t6, t7;
    __m256i tt0, tt1, tt2, tt3, tt4, tt5, tt6, tt7;
    
    // 阶段1: 使用 unpacklo/hi_epi16 交错相邻行
    t0 = _mm256_unpacklo_epi16(a_vec[0], a_vec[1]);
    t1 = _mm256_unpackhi_epi16(a_vec[0], a_vec[1]);
    t2 = _mm256_unpacklo_epi16(a_vec[2], a_vec[3]);
    t3 = _mm256_unpackhi_epi16(a_vec[2], a_vec[3]);
    t4 = _mm256_unpacklo_epi16(a_vec[4], a_vec[5]);
    t5 = _mm256_unpackhi_epi16(a_vec[4], a_vec[5]);
    t6 = _mm256_unpacklo_epi16(a_vec[6], a_vec[7]);
    t7 = _mm256_unpackhi_epi16(a_vec[6], a_vec[7]);
    
    // 阶段2: 使用 unpacklo/hi_epi32
    tt0 = _mm256_unpacklo_epi32(t0, t2);
    tt1 = _mm256_unpackhi_epi32(t0, t2);
    tt2 = _mm256_unpacklo_epi32(t1, t3);
    tt3 = _mm256_unpackhi_epi32(t1, t3);
    tt4 = _mm256_unpacklo_epi32(t4, t6);
    tt5 = _mm256_unpackhi_epi32(t4, t6);
    tt6 = _mm256_unpacklo_epi32(t5, t7);
    tt7 = _mm256_unpackhi_epi32(t5, t7);
    
    // 阶段3: 使用 unpacklo/hi_epi64
    t0 = _mm256_unpacklo_epi64(tt0, tt4);
    t1 = _mm256_unpackhi_epi64(tt0, tt4);
    t2 = _mm256_unpacklo_epi64(tt1, tt5);
    t3 = _mm256_unpackhi_epi64(tt1, tt5);
    t4 = _mm256_unpacklo_epi64(tt2, tt6);
    t5 = _mm256_unpackhi_epi64(tt2, tt6);
    t6 = _mm256_unpacklo_epi64(tt3, tt7);
    t7 = _mm256_unpackhi_epi64(tt3, tt7);
    
    a_vec[0] = avx2_interleave_lanes_epi16(t0);
    a_vec[1] = avx2_interleave_lanes_epi16(t1);
    a_vec[2] = avx2_interleave_lanes_epi16(t2);
    a_vec[3] = avx2_interleave_lanes_epi16(t3);
    a_vec[4] = avx2_interleave_lanes_epi16(t4);
    a_vec[5] = avx2_interleave_lanes_epi16(t5);
    a_vec[6] = avx2_interleave_lanes_epi16(t6);
    a_vec[7] = avx2_interleave_lanes_epi16(t7);
    
    zeta_vec = _mm256_loadu_si256((__m256i *) (zetas4inv + i * 16));
    r += rq_inverse_recursive8_avx2(b_vec,a_vec,zeta_vec);
    
    t0 = _mm256_unpacklo_epi16(b_vec[0], b_vec[1]);
    t1 = _mm256_unpackhi_epi16(b_vec[0], b_vec[1]);
    t2 = _mm256_unpacklo_epi16(b_vec[2], b_vec[3]);
    t3 = _mm256_unpackhi_epi16(b_vec[2], b_vec[3]);
    t4 = _mm256_unpacklo_epi16(b_vec[4], b_vec[5]);
    t5 = _mm256_unpackhi_epi16(b_vec[4], b_vec[5]);
    t6 = _mm256_unpacklo_epi16(b_vec[6], b_vec[7]);
    t7 = _mm256_unpackhi_epi16(b_vec[6], b_vec[7]);
    
    // 阶段2: 使用 unpacklo/hi_epi32
    tt0 = _mm256_unpacklo_epi32(t0, t2);
    tt1 = _mm256_unpackhi_epi32(t0, t2);
    tt2 = _mm256_unpacklo_epi32(t1, t3);
    tt3 = _mm256_unpackhi_epi32(t1, t3);
    tt4 = _mm256_unpacklo_epi32(t4, t6);
    tt5 = _mm256_unpackhi_epi32(t4, t6);
    tt6 = _mm256_unpacklo_epi32(t5, t7);
    tt7 = _mm256_unpackhi_epi32(t5, t7);
    
    // 阶段3: 使用 unpacklo/hi_epi64
    t0 = _mm256_unpacklo_epi64(tt0, tt4);
    t1 = _mm256_unpackhi_epi64(tt0, tt4);
    t2 = _mm256_unpacklo_epi64(tt1, tt5);
    t3 = _mm256_unpackhi_epi64(tt1, tt5);
    t4 = _mm256_unpacklo_epi64(tt2, tt6);
    t5 = _mm256_unpackhi_epi64(tt2, tt6);
    t6 = _mm256_unpacklo_epi64(tt3, tt7);
    t7 = _mm256_unpackhi_epi64(tt3, tt7);
    
    // 阶段4: 使用 permute2x128 重排128位lane
    b_vec[0] = _mm256_permute2x128_si256(t0, t1, 0x20);
    b_vec[1] = _mm256_permute2x128_si256(t2, t3, 0x20);
    b_vec[2] = _mm256_permute2x128_si256(t4, t5, 0x20);
    b_vec[3] = _mm256_permute2x128_si256(t6, t7, 0x20);
    b_vec[4] = _mm256_permute2x128_si256(t0, t1, 0x31);
    b_vec[5] = _mm256_permute2x128_si256(t2, t3, 0x31);
    b_vec[6] = _mm256_permute2x128_si256(t4, t5, 0x31);
    b_vec[7] = _mm256_permute2x128_si256(t6, t7, 0x31);

    for(int j = 0; j < 8; ++j)
    {
      _mm256_storeu_si256((__m256i *) (b + i * 128 + 16 * j), b_vec[j]);
    }
    if(r!=0) return r;
  }
  return r;
}