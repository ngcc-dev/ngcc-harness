#include "inverse_avx2.h"
int16_t zetas4inv[DTRU_N/16]={
1987,-1987,3254,-3254,2648,-2648,1090,-1090,2454,-2454,2018,-2018,1026,-1026,438,-438,
1826,-1826,3347,-3347,2082,-2082,1374,-1374,2624,-2624,1383,-1383,1731,-1731,1770,-1770,
2825,-2825,1954,-1954,226,-226,986,-986,152,-152,449,-449,427,-427,1557,-1557,
2229,-2229,1740,-1740,1008,-1008,1522,-1522,2177,-2177,2951,-2951,589,-589,2172,-2172,
2621,-2621,2716,-2716,2837,-2837,79,-79,2214,-2214,1491,-1491,3081,-3081,3438,-3438,
126,-126,2783,-2783,1946,-1946,1882,-1882,1370,-1370,2000,-2000,801,-801,160,-160,
2853,-2853,1036,-1036,2579,-2579,636,-636,2377,-2377,2814,-2814,605,-605,3129,-3129,
2721,-2721,919,-919,2845,-2845,2286,-2286,1271,-1271,1048,-1048,2729,-2729,3126,-3126,
};

__m256i fqmul_avx2(__m256i l, __m256i h) 
{
    __m256i temp1 = _mm256_mulhi_epi16(l, h);          // vpmulhw
    __m256i temp2 = _mm256_mullo_epi16(l, h);          // vpmullw
    __m256i temp3 = _mm256_mullo_epi16(qinv_vec, temp2); // vpmullw with qinv
    temp3 = _mm256_mulhi_epi16(q_vec, temp3);          // vpmulhw with q
    temp3 = _mm256_sub_epi16(temp1, temp3);             // vpsubw
    return temp3;
}

void barrett_reduce_avx2(__m256i *input){
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

// AVX2 版本的 basemul8
void basemul8_avx2(__m256i c_vec[8], const __m256i a_vec[8], const __m256i b_vec[8], __m256i zeta_vec)
{
    __m256i d[8];
    __m256i temp;
    
    // 计算 d[i] = fqmul(a[i], b[i])
    for (int i = 0; i < 8; i++)
        d[i] = fqmul_avx2(a_vec[i], b_vec[i]);

    // c[0] = barrett_reduce(d[0] + fqmul((CAL_D(a, b, 1, 7, d) + CAL_D(a, b, 2, 6, d) + CAL_D(a, b, 3, 5, d) + d[4]), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 1, 7, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 2, 6, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 3, 5, d));
    temp = _mm256_add_epi16(temp, d[4]);
    temp = fqmul_avx2(temp, zeta_vec);
    temp = _mm256_add_epi16(d[0], temp);
    barrett_reduce_avx2(&temp);
    c_vec[0] = temp;
        
    // c[1] = barrett_reduce(CAL_D(a, b, 0, 1, d) + fqmul((CAL_D(a, b, 2, 7, d) + CAL_D(a, b, 3, 6, d) + CAL_D(a, b, 4, 5, d)), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 2, 7, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 3, 6, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 4, 5, d));
    temp = fqmul_avx2(temp, zeta_vec);
    temp = _mm256_add_epi16(CAL_D_avx2(a_vec, b_vec, 0, 1, d), temp);
    barrett_reduce_avx2(&temp);
    c_vec[1] = temp;

    // c[2] = barrett_reduce(CAL_D(a, b, 0, 2, d) + d[1] + fqmul((CAL_D(a, b, 3, 7, d) + CAL_D(a, b, 4, 6, d) + d[5]), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 3, 7, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 4, 6, d));
    temp = _mm256_add_epi16(temp, d[5]);
    temp = fqmul_avx2(temp, zeta_vec);
    temp = _mm256_add_epi16(CAL_D_avx2(a_vec, b_vec, 0, 2, d), temp);
    temp = _mm256_add_epi16(temp, d[1]);
    barrett_reduce_avx2(&temp);
    c_vec[2] = temp;

    // c[3] = barrett_reduce(CAL_D(a, b, 0, 3, d) + CAL_D(a, b, 1, 2, d) + fqmul((CAL_D(a, b, 4, 7, d) + CAL_D(a, b, 5, 6, d)), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 4, 7, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 5, 6, d));
    temp = fqmul_avx2(temp, zeta_vec);
    temp = _mm256_add_epi16(CAL_D_avx2(a_vec, b_vec, 0, 3, d), temp);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 1, 2, d));
    barrett_reduce_avx2(&temp);
    c_vec[3] = temp;

    // c[4] = barrett_reduce(CAL_D(a, b, 0, 4, d) + CAL_D(a, b, 1, 3, d) + d[2] + fqmul((CAL_D(a, b, 5, 7, d) + d[6]), zeta))
    temp = CAL_D_avx2(a_vec, b_vec, 5, 7, d);
    temp = _mm256_add_epi16(temp, d[6]);
    temp = fqmul_avx2(temp, zeta_vec);
    temp = _mm256_add_epi16(CAL_D_avx2(a_vec, b_vec, 0, 4, d), temp);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 1, 3, d));
    temp = _mm256_add_epi16(temp, d[2]);
    barrett_reduce_avx2(&temp);
    c_vec[4] = temp;

    // c[5] = barrett_reduce(CAL_D(a, b, 0, 5, d) + CAL_D(a, b, 1, 4, d) + CAL_D(a, b, 2, 3, d)) + fqmul(CAL_D(a, b, 6, 7, d), zeta)
    temp = CAL_D_avx2(a_vec, b_vec, 0, 5, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 1, 4, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 2, 3, d));
    barrett_reduce_avx2(&temp);
    temp = _mm256_add_epi16(temp, fqmul_avx2(CAL_D_avx2(a_vec, b_vec, 6, 7, d), zeta_vec));
    c_vec[5] = temp;

    // c[6] = barrett_reduce(CAL_D(a, b, 0, 6, d) + CAL_D(a, b, 1, 5, d) + CAL_D(a, b, 2, 4, d)) + d[3] + fqmul(d[7], zeta)
    temp = CAL_D_avx2(a_vec, b_vec, 0, 6, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 1, 5, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 2, 4, d));
    barrett_reduce_avx2(&temp);
    temp = _mm256_add_epi16(temp, d[3]);
    temp = _mm256_add_epi16(temp, fqmul_avx2(d[7], zeta_vec));
    c_vec[6] = temp;

    // c[7] = barrett_reduce(CAL_D(a, b, 0, 7, d) + CAL_D(a, b, 1, 6, d) + CAL_D(a, b, 2, 5, d) + CAL_D(a, b, 3, 4, d))
    temp = CAL_D_avx2(a_vec, b_vec, 0, 7, d);
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 1, 6, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 2, 5, d));
    temp = _mm256_add_epi16(temp, CAL_D_avx2(a_vec, b_vec, 3, 4, d));
    barrett_reduce_avx2(&temp);
    c_vec[7] = temp;
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

// AVX2 版本的 rq_inverse_recursive16
// 输入: a_vec[16] - 16个__m256i，每个包含16个不同16维多项式的同一位置元素
// 输出: b_vec[16]
// zeta_vec: 16个zeta值
int rq_inverse_recursive16_avx2(__m256i b_vec[16], const __m256i a_vec[16], __m256i zeta_vec)
{
    __m256i d[16], a0[8], a1[8], c0[8], c1[8], b0[8], b1[8];
    __m256i temp;
    __m256i const_neg1 = _mm256_set1_epi16(-1);
    int r;

    // 计算 d[i] = a[2*i], d[i+8] = a[2*i+1]
    // 奇偶分离
    for (int i = 0; i < 8; i++)
    {
        d[i] = a_vec[2 * i];        // d[0..7] = a[0,2,4,6,8,10,12,14]
        d[i + 8] = a_vec[2 * i + 1]; // d[8..15] = a[1,3,5,7,9,11,13,15]
    }

    // basemul8(a0, d, d, zeta)
    basemul8_avx2(a0, d, d, zeta_vec);

    // basemul8(a1, d + 8, d + 8, zeta)
    basemul8_avx2(a1, d + 8, d + 8, zeta_vec);

    // a0[0] -= fqmul(zeta, a1[7])
    temp = fqmul_avx2(zeta_vec, a1[7]);
    a0[0] = _mm256_sub_epi16(a0[0], temp);

    // for (i = 1; i < 8; i++)
    //     a0[i] -= a1[i - 1];
    for (int i = 1; i < 8; i++)
        a0[i] = _mm256_sub_epi16(a0[i], a1[i - 1]);

    // 递归调用: r = rq_inverse_recursive8(a1, a0, zeta)
    r = rq_inverse_recursive8_avx2(a1, a0, zeta_vec);

    // 计算 c0[i] = a[2*i], c1[i] = -a[2*i+1]
    for (int i = 0; i < 8; i++)
    {
        c0[i] = a_vec[2 * i];
        c1[i] = _mm256_mullo_epi16(a_vec[2 * i + 1], const_neg1);  // -a[2*i+1]
    }

    // basemul8(b0, c0, a1, zeta)
    basemul8_avx2(b0, c0, a1, zeta_vec);

    // basemul8(b1, c1, a1, zeta)
    basemul8_avx2(b1, c1, a1, zeta_vec);

    // 重新交错输出
    for (int i = 0; i < 8; i++)
    {
        b_vec[2 * i] = b0[i];
        b_vec[2 * i + 1] = b1[i];
    }

    return r;
}



int baseinv_avx2(int16_t *b, const int16_t *a){
    int r = 0;
  for(int i = 0; i < DTRU_N/(16*16); i++){
    __m256i b_vec[16], a_vec[16], zeta_vec;
    __m256i t[16], tt[16];
    // 1. 读入数据（顺序存储）
    for(int j = 0; j < 16; ++j)
    {
        a_vec[j] = _mm256_loadu_si256((__m256i *) (a + i * 256 + 16 * j));
    }
    // 2. 原地转置：16x16 矩阵转置
    // 阶段1: 16位级别交错
    t[0] = _mm256_unpacklo_epi16(a_vec[0], a_vec[1]);
    t[1] = _mm256_unpackhi_epi16(a_vec[0], a_vec[1]);
    t[2] = _mm256_unpacklo_epi16(a_vec[2], a_vec[3]);
    t[3] = _mm256_unpackhi_epi16(a_vec[2], a_vec[3]);
    t[4] = _mm256_unpacklo_epi16(a_vec[4], a_vec[5]);
    t[5] = _mm256_unpackhi_epi16(a_vec[4], a_vec[5]);
    t[6] = _mm256_unpacklo_epi16(a_vec[6], a_vec[7]);
    t[7] = _mm256_unpackhi_epi16(a_vec[6], a_vec[7]);
    t[8] = _mm256_unpacklo_epi16(a_vec[8], a_vec[9]);
    t[9] = _mm256_unpackhi_epi16(a_vec[8], a_vec[9]);
    t[10] = _mm256_unpacklo_epi16(a_vec[10], a_vec[11]);
    t[11] = _mm256_unpackhi_epi16(a_vec[10], a_vec[11]);
    t[12] = _mm256_unpacklo_epi16(a_vec[12], a_vec[13]);
    t[13] = _mm256_unpackhi_epi16(a_vec[12], a_vec[13]);
    t[14] = _mm256_unpacklo_epi16(a_vec[14], a_vec[15]);
    t[15] = _mm256_unpackhi_epi16(a_vec[14], a_vec[15]);
    
    // 阶段2: 32位级别交错
    tt[0] = _mm256_unpacklo_epi32(t[0], t[2]);
    tt[1] = _mm256_unpackhi_epi32(t[0], t[2]);
    tt[2] = _mm256_unpacklo_epi32(t[1], t[3]);
    tt[3] = _mm256_unpackhi_epi32(t[1], t[3]);
    tt[4] = _mm256_unpacklo_epi32(t[4], t[6]);
    tt[5] = _mm256_unpackhi_epi32(t[4], t[6]);
    tt[6] = _mm256_unpacklo_epi32(t[5], t[7]);
    tt[7] = _mm256_unpackhi_epi32(t[5], t[7]);
    tt[8] = _mm256_unpacklo_epi32(t[8], t[10]);
    tt[9] = _mm256_unpackhi_epi32(t[8], t[10]);
    tt[10] = _mm256_unpacklo_epi32(t[9], t[11]);
    tt[11] = _mm256_unpackhi_epi32(t[9], t[11]);
    tt[12] = _mm256_unpacklo_epi32(t[12], t[14]);
    tt[13] = _mm256_unpackhi_epi32(t[12], t[14]);
    tt[14] = _mm256_unpacklo_epi32(t[13], t[15]);
    tt[15] = _mm256_unpackhi_epi32(t[13], t[15]);
    
    // 阶段3: 64位级别交错
    t[0] = _mm256_unpacklo_epi64(tt[0], tt[4]);
    t[1] = _mm256_unpackhi_epi64(tt[0], tt[4]);
    t[2] = _mm256_unpacklo_epi64(tt[1], tt[5]);
    t[3] = _mm256_unpackhi_epi64(tt[1], tt[5]);
    t[4] = _mm256_unpacklo_epi64(tt[2], tt[6]);
    t[5] = _mm256_unpackhi_epi64(tt[2], tt[6]);
    t[6] = _mm256_unpacklo_epi64(tt[3], tt[7]);
    t[7] = _mm256_unpackhi_epi64(tt[3], tt[7]);
    t[8] = _mm256_unpacklo_epi64(tt[8], tt[12]);
    t[9] = _mm256_unpackhi_epi64(tt[8], tt[12]);
    t[10] = _mm256_unpacklo_epi64(tt[9], tt[13]);
    t[11] = _mm256_unpackhi_epi64(tt[9], tt[13]);
    t[12] = _mm256_unpacklo_epi64(tt[10], tt[14]);
    t[13] = _mm256_unpackhi_epi64(tt[10], tt[14]);
    t[14] = _mm256_unpacklo_epi64(tt[11], tt[15]);
    t[15] = _mm256_unpackhi_epi64(tt[11], tt[15]);
    
    // 阶段4: 128位lane重排
    a_vec[0] = _mm256_permute2x128_si256(t[0], t[8], 0x20);
    a_vec[1] = _mm256_permute2x128_si256(t[1], t[9], 0x20);
    a_vec[2] = _mm256_permute2x128_si256(t[2], t[10], 0x20);
    a_vec[3] = _mm256_permute2x128_si256(t[3], t[11], 0x20);
    a_vec[4] = _mm256_permute2x128_si256(t[4], t[12], 0x20);
    a_vec[5] = _mm256_permute2x128_si256(t[5], t[13], 0x20);
    a_vec[6] = _mm256_permute2x128_si256(t[6], t[14], 0x20);
    a_vec[7] = _mm256_permute2x128_si256(t[7], t[15], 0x20);
    a_vec[8] = _mm256_permute2x128_si256(t[0], t[8], 0x31);
    a_vec[9] = _mm256_permute2x128_si256(t[1], t[9], 0x31);
    a_vec[10] = _mm256_permute2x128_si256(t[2], t[10], 0x31);
    a_vec[11] = _mm256_permute2x128_si256(t[3], t[11], 0x31);
    a_vec[12] = _mm256_permute2x128_si256(t[4], t[12], 0x31);
    a_vec[13] = _mm256_permute2x128_si256(t[5], t[13], 0x31);
    a_vec[14] = _mm256_permute2x128_si256(t[6], t[14], 0x31);
    a_vec[15] = _mm256_permute2x128_si256(t[7], t[15], 0x31);
    
    // 3. 加载 zeta 值
    zeta_vec = _mm256_loadu_si256((__m256i *) (zetas4inv + i * 16));
    
    // 4. 执行向量化求逆
    r += rq_inverse_recursive16_avx2(b_vec, a_vec, zeta_vec);
    
    // 5. 逆转置：恢复顺序存储
    t[0] = _mm256_unpacklo_epi16(b_vec[0], b_vec[1]);
    t[1] = _mm256_unpackhi_epi16(b_vec[0], b_vec[1]);
    t[2] = _mm256_unpacklo_epi16(b_vec[2], b_vec[3]);
    t[3] = _mm256_unpackhi_epi16(b_vec[2], b_vec[3]);
    t[4] = _mm256_unpacklo_epi16(b_vec[4], b_vec[5]);
    t[5] = _mm256_unpackhi_epi16(b_vec[4], b_vec[5]);
    t[6] = _mm256_unpacklo_epi16(b_vec[6], b_vec[7]);
    t[7] = _mm256_unpackhi_epi16(b_vec[6], b_vec[7]);
    t[8] = _mm256_unpacklo_epi16(b_vec[8], b_vec[9]);
    t[9] = _mm256_unpackhi_epi16(b_vec[8], b_vec[9]);
    t[10] = _mm256_unpacklo_epi16(b_vec[10], b_vec[11]);
    t[11] = _mm256_unpackhi_epi16(b_vec[10], b_vec[11]);
    t[12] = _mm256_unpacklo_epi16(b_vec[12], b_vec[13]);
    t[13] = _mm256_unpackhi_epi16(b_vec[12], b_vec[13]);
    t[14] = _mm256_unpacklo_epi16(b_vec[14], b_vec[15]);
    t[15] = _mm256_unpackhi_epi16(b_vec[14], b_vec[15]);
    
    // 阶段2: 32位级别交错
    tt[0] = _mm256_unpacklo_epi32(t[0], t[2]);
    tt[1] = _mm256_unpackhi_epi32(t[0], t[2]);
    tt[2] = _mm256_unpacklo_epi32(t[1], t[3]);
    tt[3] = _mm256_unpackhi_epi32(t[1], t[3]);
    tt[4] = _mm256_unpacklo_epi32(t[4], t[6]);
    tt[5] = _mm256_unpackhi_epi32(t[4], t[6]);
    tt[6] = _mm256_unpacklo_epi32(t[5], t[7]);
    tt[7] = _mm256_unpackhi_epi32(t[5], t[7]);
    tt[8] = _mm256_unpacklo_epi32(t[8], t[10]);
    tt[9] = _mm256_unpackhi_epi32(t[8], t[10]);
    tt[10] = _mm256_unpacklo_epi32(t[9], t[11]);
    tt[11] = _mm256_unpackhi_epi32(t[9], t[11]);
    tt[12] = _mm256_unpacklo_epi32(t[12], t[14]);
    tt[13] = _mm256_unpackhi_epi32(t[12], t[14]);
    tt[14] = _mm256_unpacklo_epi32(t[13], t[15]);
    tt[15] = _mm256_unpackhi_epi32(t[13], t[15]);
    
    // 阶段3: 64位级别交错
    t[0] = _mm256_unpacklo_epi64(tt[0], tt[4]);
    t[1] = _mm256_unpackhi_epi64(tt[0], tt[4]);
    t[2] = _mm256_unpacklo_epi64(tt[1], tt[5]);
    t[3] = _mm256_unpackhi_epi64(tt[1], tt[5]);
    t[4] = _mm256_unpacklo_epi64(tt[2], tt[6]);
    t[5] = _mm256_unpackhi_epi64(tt[2], tt[6]);
    t[6] = _mm256_unpacklo_epi64(tt[3], tt[7]);
    t[7] = _mm256_unpackhi_epi64(tt[3], tt[7]);
    t[8] = _mm256_unpacklo_epi64(tt[8], tt[12]);
    t[9] = _mm256_unpackhi_epi64(tt[8], tt[12]);
    t[10] = _mm256_unpacklo_epi64(tt[9], tt[13]);
    t[11] = _mm256_unpackhi_epi64(tt[9], tt[13]);
    t[12] = _mm256_unpacklo_epi64(tt[10], tt[14]);
    t[13] = _mm256_unpackhi_epi64(tt[10], tt[14]);
    t[14] = _mm256_unpacklo_epi64(tt[11], tt[15]);
    t[15] = _mm256_unpackhi_epi64(tt[11], tt[15]);
    
    // 阶段4: 128位lane重排
    b_vec[0] = _mm256_permute2x128_si256(t[0], t[8], 0x20);
    b_vec[1] = _mm256_permute2x128_si256(t[1], t[9], 0x20);
    b_vec[2] = _mm256_permute2x128_si256(t[2], t[10], 0x20);
    b_vec[3] = _mm256_permute2x128_si256(t[3], t[11], 0x20);
    b_vec[4] = _mm256_permute2x128_si256(t[4], t[12], 0x20);
    b_vec[5] = _mm256_permute2x128_si256(t[5], t[13], 0x20);
    b_vec[6] = _mm256_permute2x128_si256(t[6], t[14], 0x20);
    b_vec[7] = _mm256_permute2x128_si256(t[7], t[15], 0x20);
    b_vec[8] = _mm256_permute2x128_si256(t[0], t[8], 0x31);
    b_vec[9] = _mm256_permute2x128_si256(t[1], t[9], 0x31);
    b_vec[10] = _mm256_permute2x128_si256(t[2], t[10], 0x31);
    b_vec[11] = _mm256_permute2x128_si256(t[3], t[11], 0x31);
    b_vec[12] = _mm256_permute2x128_si256(t[4], t[12], 0x31);
    b_vec[13] = _mm256_permute2x128_si256(t[5], t[13], 0x31);
    b_vec[14] = _mm256_permute2x128_si256(t[6], t[14], 0x31);
    b_vec[15] = _mm256_permute2x128_si256(t[7], t[15], 0x31);
    
    for(int j = 0; j < 16; ++j)
    {
        _mm256_storeu_si256((__m256i *) (b + i * 256 + 16 * j), b_vec[j]);
    }
    
    if(r!=0) return r;
}

  return r;
}