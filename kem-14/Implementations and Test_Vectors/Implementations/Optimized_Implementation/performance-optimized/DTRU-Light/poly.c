#include <stdint.h>
#include <stdio.h>
#include <immintrin.h>
#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "coding.h"
#include "cbd.h"
#include "inverse.h"
#include "inverse_avx2.h"
#include "ntt.h"
#include "ntt_avx2.h"

void poly_reduce(poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = barrett_reduce(a->coeffs[i]);
}

void poly_freeze(poly *a)
{
  poly_reduce(a);
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
}

void poly_add(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; ++i)
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

void poly_multi_p(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N / 2; ++i)
  {
    b->coeffs[i         ] = a->coeffs[i+DTRU_N/2] + a->coeffs[i];
    b->coeffs[i+DTRU_N/2] = a->coeffs[i+DTRU_N/2] - a->coeffs[i];
  }
}


void poly_sample_keygen_f(poly *a, const unsigned char *buf)
{
  cbd1_avx2(a, buf);
}

void poly_sample_keygen_g(poly *a, const unsigned char *buf)
{
  cbd2_avx2(a, buf);
}

void poly_sample_enc_r(poly *a, const unsigned char *buf)
{
  cbd1_avx2(a, buf);
}

void poly_sample_enc_e(poly *a, const unsigned char *buf)
{
  cbd2_avx2(a, buf);
}

void poly_ntt(poly *b)
{
  ntt(b->coeffs);
}

void poly_invntt(poly *b)
{
  invntt(b->coeffs);
}

void poly_basemul(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N / 8; ++i)
  {
    basemul(c->coeffs + 8 * i,
            a->coeffs + 8 * i,
            b->coeffs + 8 * i,
            zetas[64 + i]);
    basemul(c->coeffs + 8 * i + 4,
            a->coeffs + 8 * i + 4,
            b->coeffs + 8 * i + 4,
            -zetas[64 + i]);
  }
}

// int poly_baseinv(poly *b, const poly *a)
// {
//   int r = 0;
//   for (int i = 0; i < DTRU_N / 4; ++i)
//   {
//     if(r != 0)   return r;
//     r += rq_inverse(b->coeffs + 4 * i,
//                     a->coeffs + 4 * i,
//                     zetas_base_nega[i]);
//   }
//   return r;
// }

// int poly_baseinv(poly *b, const poly *a)
// {
//   int r = 0;
//   for (int i = 0; i < DTRU_N / 8; ++i)
//   {
//     if(r != 0)   return r;
//     r += rq_inverse_det(b->coeffs + 8 * i,
//                     a->coeffs + 8 * i,
//                     zetas[64 + i]);
//     if(r != 0)   return r;
//     r += rq_inverse_det(b->coeffs + 8 * i + 4,
//                     a->coeffs + 8 * i + 4,
//                     -zetas[64 + i]);
//   }
//   return r;
// }

int poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 8; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_recursive(b->coeffs + 8 * i,
                    a->coeffs + 8 * i,
                    zetas[64 + i]);
    if(r != 0)   return r;
    r += rq_inverse_recursive(b->coeffs + 8 * i + 4,
                    a->coeffs + 8 * i + 4,
                    -zetas[64 + i]);
  }
  return r;
}


void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg)
{
  unsigned int i, j;
  int16_t mask;
  uint8_t mh[DTRU_N / 8];
  uint8_t tmp;
  int16_t s;
  int32_t t;
  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    tmp = msg[i] & 0xF;
    mh[2 * i] = encode_e8(tmp);
    mh[2 * i + DTRU_N / 16] = mh[2 * i];

    tmp = (msg[i] >> 4) & 0xF;
    mh[2 * i + 1] = encode_e8(tmp);
    mh[2 * i + 1 + DTRU_N / 16] = mh[2 * i + 1];
  }

  for (i = 0; i < DTRU_N / 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      mask = -(int16_t)((mh[i] >> j) & 1);
      s = sigma->coeffs[8 * i + j] + (mask & ((DTRU_Q + 1) >> 1));
      t = ((int32_t)(s << DTRU_LOGQ2) + (DTRU_Q >> 1)) / DTRU_Q;
      c->coeffs[8 * i + j] = t & (DTRU_Q2 - 1);
    }
  }
}

void poly_decode(unsigned char *msg,
                 const poly *cf)
{
  unsigned int i, j;
  int16_t tmp_mp[16];

  for (i = 0; i < DTRU_MSGBYTES; i++)
  {
    msg[i] = 0;
  }

  for (i = 0; i < DTRU_N / 16; i++)
  {
    for (j = 0; j < 8; j++)
    {
      tmp_mp[j] = cf->coeffs[8 * i + j];
      tmp_mp[j + 8] = cf->coeffs[8 * i + j + DTRU_N / 2];
    }
    msg[i >> 1] |= decode_e8(tmp_mp) << ((i & 1) << 2);
  }
}

// AVX2 implementations
#include "consts.h"

void poly_ntt_avx(poly *a)
{
  ntt_avx2(a->coeffs, a->coeffs);
}

void poly_invntt_avx(poly *a)
{
  invntt_avx2(a->coeffs, a->coeffs);
}

void poly_basemul_avx(poly *c, const poly *a, const poly *b)
{
  unsigned int i;

  for (i = 0; i < DTRU_N / 4 / 16; i++)
  {
    basemul_avx2(c->coeffs + 64 * i,
                 a->coeffs + 64 * i,
                 b->coeffs + 64 * i,
                 zetas4inv + 16 * i);
  }
}

int poly_baseinv_avx(poly *b, const poly *a)
{
  return baseinv_avx2(b->coeffs, a->coeffs);
}

void poly_freeze_avx(poly *a)
{
  poly_reduce_avx(a);
  fqcsubq_avx(a);
}

void poly_reduce_avx(poly *a)
{
  barrett_reduce_avx(a);
}

void fqcsubq_avx(poly *a)
{
  const __m256i qv = _mm256_set1_epi16(DTRU_Q);

  for (int i = 0; i < DTRU_N; i += 16)
  {
    __m256i coeffs = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    coeffs = fqcsubq_avx2(coeffs, qv);
    _mm256_storeu_si256((__m256i *)(a->coeffs + i), coeffs);
  }
}

void poly_fqcsubq(poly *a)
{
  for (int i = 0; i < DTRU_N; ++i)
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
}

void poly_add_avx(poly *c, const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; i += 16)
  {
    __m256i av = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    __m256i bv = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
    _mm256_storeu_si256((__m256i *)(c->coeffs + i), _mm256_add_epi16(av, bv));
  }
}

void poly_double_avx(poly *b, const poly *a)
{
  for (int i = 0; i < DTRU_N; i += 16)
  {
    __m256i av = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
    _mm256_storeu_si256((__m256i *)(b->coeffs + i), _mm256_add_epi16(av, av));
  }
}

static void barrett_reduce_avx256(__m256i *input){
    __m256i tmp0;
    tmp0 = _mm256_mulhi_epi16(_mm256_set1_epi16(BARRETT_V), *input);
    tmp0 = _mm256_srai_epi16(tmp0, 8);
    tmp0 = _mm256_mullo_epi16(_mm256_set1_epi16(DTRU_Q), tmp0);
    *input = _mm256_sub_epi16(*input, tmp0);
}

void barrett_reduce_avx(poly *b)
{
  for (int i = 0; i < DTRU_N; i += 16)
  {
    __m256i coeffs = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
    barrett_reduce_avx256(&coeffs);
    _mm256_storeu_si256((__m256i *)(b->coeffs + i), coeffs);
  }
}

static void poly_reduce_avx2(poly *a) {
    __m256i *coeffs = (__m256i *)a->coeffs;
    const int vec_count = DTRU_N / 16;
    for (int i = 0; i < vec_count; ++i) {
        barrett_reduce_avx256(&coeffs[i]);
    }
}

void poly_freeze_avx2(poly *a)
{  
  poly_reduce_avx2(a);
    //const __m256i Q_vec = _mm256_set1_epi16(DTRU_Q);
    int16_t *c = a->coeffs;
    const int vec_count = DTRU_N / 16;
    for (int i = 0; i < vec_count; ++i) {
        __m256i x = _mm256_loadu_si256((__m256i*)(c + 16 * i));
        __m256i mask = _mm256_srai_epi16(x, 15);
        x = _mm256_add_epi16(x, _mm256_and_si256(mask, _mm256_set1_epi16(DTRU_Q)));
        x = _mm256_sub_epi16(x, _mm256_set1_epi16(DTRU_Q));
        mask = _mm256_srai_epi16(x, 15);
        x = _mm256_add_epi16(x, _mm256_and_si256(mask, _mm256_set1_epi16(DTRU_Q)));
        _mm256_storeu_si256((__m256i*)(c + 16 * i), x);
    }
}

// AVX2 implementation of poly_decode
#include <string.h>

// 辅助函数：查找向量中的最小值及其索引 (仅针对 4 个 int32)
// 返回: [min_val, min_idx, garbage, garbage ...]
static inline __m128i min_with_index_4(__m128i v)
{
    // 1. 寻找最小值
    __m128i v_shuf = _mm_shuffle_epi32(v, 0xB1); // 1011 0001 [0, 1, 2, 3] -> [1, 0, 3, 2]
    // 比较 [0, 1, 2, 3] 和 [1, 0, 3, 2]
    __m128i min1   = _mm_min_epi32(v, v_shuf);   // min(0,1), min(1,0), min(2,3), min(3,2)
    // [min(0,1), min(1,0), min(2,3), min(3,2)] -> [min(2,3), min(3,2), min(0,1), min(1,0)]
    __m128i min1_s = _mm_shuffle_epi32(min1, 0x4E); // 0100 1110 [23, 23, 01, 01]
    // 比较 [01, 01, 23, 23] 和 [23, 23, 01, 01]
    __m128i min_all = _mm_min_epi32(min1, min1_s);  // [0123, 0123, 0123, 0123]

    // 2. 寻找索引 (比较 v == min_all)
    __m128i mask = _mm_cmpeq_epi32(v, min_all); // 只有最小值的位置是 -1 (0xFFFFFFFF)
    
    // 生成索引掩码: movemask 将提取每个 32位 lane 的最高位
    int mask_bits = _mm_movemask_ps(_mm_castsi128_ps(mask));
    
    // 使用 GCC/Clang 内置指令计算拖尾零的个数 (即第一个置位的位置) -> 得到索引
    // 注意：如果有多个最小值，取最低位的索引
    int min_idx = __builtin_ctz(mask_bits); 

    // 返回 (min_val, min_idx)
    // 注意：我们将 min_idx 放入向量的第二个 32bit 位置，方便后续提取
    return _mm_set_epi32(0, 0, min_idx, _mm_cvtsi128_si32(min_all));
}

// 优化的 AVX 内部解码核心逻辑 (通用部分)
// c0_vec, c1_vec: 包含 4 个迭代的 c[0] 和 c[1] 输入
static inline uint8_t decode_d8_core_avx(__m128i c0_vec, __m128i c1_vec, uint32_t *cost_out)
{
    // 1. r = ((c[1] - c[0]) >> 31) & 1;  // if c1 < c0 then r=1 else r=0
    __m128i cmp = _mm_cmpgt_epi32(c0_vec, c1_vec); // c0 > c1 => -1 (即 r=1), 否则 0

    // 计算 m[0] 的位图
    // m[0] |= r << i;
    int r_mask = _mm_movemask_ps(_mm_castsi128_ps(cmp));
    uint8_t m0 = r_mask & 0xF; 

    // 计算 xor_sum (r 的所有位异或)：popcount 计算 1 的个数，奇数个则 xor_sum=1
    // xor_sum ^= r; // 相当于 mod 2 求和
    uint8_t xor_sum = _mm_popcnt_u32(m0) & 1;
    // uint8_t xor_sum = (0x6996 >> m0) & 1;

    // 2. 计算 tmp 和 tmp_xor
    /**
     *  // min(c[0], c[1])
     *  uint32_t tmp = ((-(r ^ 1)) & (uint32_t)c[0]) ^ ((-(r & 1)) & (uint32_t)c[1]);
     *  // max(c[0], c[1])
     *  uint32_t tmp_xor = ((-(r & 1)) & (uint32_t)c[0]) ^ ((-(r ^ 1)) & (uint32_t)c[1]);
     */
    // 这里直接用 _mm_min_epi32 和 _mm_max_epi32 实现
    __m128i tmp     = _mm_min_epi32(c0_vec, c1_vec);  // tmp = min(c0, c1);
    __m128i tmp_xor = _mm_max_epi32(c0_vec, c1_vec);  // tmp_xor = max(c0, c1);

    // *cost += tmp (横向求和 tmp)
    // 使用 hadd [A, B, C, D] -> [A+B, C+D, ..., ...]
    __m128i sum_tmp = _mm_hadd_epi32(tmp, tmp);
    // [A+B, C+D, ..., ...] -> [A+B+C+D, ..., ..., ...]
    sum_tmp = _mm_hadd_epi32(sum_tmp, sum_tmp);
    // lo 32bit
    int total_tmp = _mm_cvtsi128_si32(sum_tmp);

    // 3. diff = tmp_xor - tmp
    __m128i diffs = _mm_sub_epi32(tmp_xor, tmp);

    // 4. 寻找 diff 中的最小值和索引
    // min_diff 和 min_i
    __m128i res_min = min_with_index_4(diffs);
    int min_val = _mm_cvtsi128_si32(res_min);
    int min_idx = _mm_extract_epi32(res_min, 1);

    // 5. 组合最终结果
    // m[1] = m[0] ^ (1 << min_i)
    uint8_t m1 = m0 ^ (1 << min_idx);

    /*
        *cost_out = total_tmp;
        if (xor_sum) {
            *cost_out += min_val;
            return m1;
        } else {
            return m0;
        }
    */
    uint8_t res = ((-(xor_sum ^ 1)) & (uint32_t)m0) ^ ((-(xor_sum & 1)) & (uint32_t)m1);
    *cost_out = total_tmp + (min_val & (-(uint32_t)xor_sum));

    return res;
}

static uint8_t decode_d8_00_avx_internal(uint32_t *cost, const uint32_t *c0_arr, const uint32_t *c1_arr)
{
    // 输入是 8 个元素，我们需要将其压缩为 4 个求和
    // c0_arr: [a0, a1, a2, a3, a4, a5, a6, a7]
    // 目标 c0_vec: [a0+a1, a2+a3, a4+a5, a6+a7]
    // 这正是 _mm256_hadd_epi32 或 _mm_hadd_epi32 擅长的
    
    __m128i v_c0_0 = _mm_loadu_si128((__m128i*)c0_arr);     // [a0, a1, a2, a3]
    __m128i v_c0_1 = _mm_loadu_si128((__m128i*)(c0_arr+4)); // [a4, a5, a6, a7]
    // horizontal add: [a0+a1, a2+a3, a4+a5, a6+a7]
    __m128i vec_c0 = _mm_hadd_epi32(v_c0_0, v_c0_1);

    __m128i v_c1_0 = _mm_loadu_si128((__m128i*)c1_arr);     // [b0, b1, b2, b3]
    __m128i v_c1_1 = _mm_loadu_si128((__m128i*)(c1_arr+4)); // [b4, b5, b6, b7]
    // horizontal add: [b0+b1, b2+b3, b4+b5, b6+b7]  
    __m128i vec_c1 = _mm_hadd_epi32(v_c1_0, v_c1_1);

    return decode_d8_core_avx(vec_c0, vec_c1, cost);
}

static uint8_t decode_d8_10_avx_internal(uint32_t *cost, const uint32_t *c0_arr, const uint32_t *c1_arr)
{
    // 模式 10 需要交叉逻辑
    // c[0] = tmp_cost[i << 1][1] + tmp_cost[i << 1 | 1][0];
    // c[1] = tmp_cost[i << 1][0] + tmp_cost[i << 1 | 1][1];
    
    // 加载数据
    __m128i v_c0_0 = _mm_loadu_si128((__m128i*)c0_arr);     // [a0, a1, a2, a3]
    __m128i v_c0_1 = _mm_loadu_si128((__m128i*)(c0_arr+4)); // [a4, a5, a6, a7]
    __m128i v_c1_0 = _mm_loadu_si128((__m128i*)c1_arr);     // [b0, b1, b2, b3]
    __m128i v_c1_1 = _mm_loadu_si128((__m128i*)(c1_arr+4)); // [b4, b5, b6, b7]
    
    __m128i m0_mix = _mm_blend_epi16(v_c1_0, v_c0_0, 0xCC); // 1100 1100 [b0, a1, b2, a3]
    __m128i m1_mix = _mm_blend_epi16(v_c1_1, v_c0_1, 0xCC); // 1100 1100 [b4, a5, b6, a7]
    
    // vec_c0 = [b0+a1, b2+a3, b4+a5, b6+a7]
    __m128i vec_c0 = _mm_hadd_epi32(m0_mix, m1_mix);

    __m128i m0_mix_inv = _mm_blend_epi16(v_c1_0, v_c0_0, 0x33); // 0011 0011 [a0, b1, a2, b3]
    __m128i m1_mix_inv = _mm_blend_epi16(v_c1_1, v_c0_1, 0x33); // 0011 0011 [a4, b5, a6, b7]
    
    // vec_c1 = [a0+b1, a2+b3, a4+b5, a6+b7]
    __m128i vec_c1 = _mm_hadd_epi32(m0_mix_inv, m1_mix_inv);

    return decode_d8_core_avx(vec_c0, vec_c1, cost);
}

void poly_decode_avx(unsigned char *msg, const poly *cf)
{
    unsigned int i;
    __m256i Q_vec = _mm256_set1_epi16(DTRU_Q);
    __m256i offset_vec = _mm256_set1_epi16((DTRU_Q + 1) >> 1);
    
    // 临时存储计算出的 cost
    // c0_arr 存储 (|v|)^2 + (|v_pair|)^2 对应 tmp_mp[j]
    // c1_arr 存储 (|v-off|)^2 + (|v_pair-off|)^2 对应 tmp_mp[j + 8 * i]
    uint32_t c0_buf[8]; 
    uint32_t c1_buf[8];
    
    // 清零 msg (因为后面是 |= 操作)
    memset(msg, 0, DTRU_MSGBYTES);
    
    for (i = 0; i < DTRU_N / 16; i++)
    {
        // 1. 加载 16 个系数 (8 对)
        // 从 cf->coeffs[8*i] 加载 8个 (Low)
        // 从 cf->coeffs[8*i + N/2] 加载 8个 (High)
        __m128i lo = _mm_loadu_si128((__m128i const *)&cf->coeffs[8 * i]);
        __m128i hi = _mm_loadu_si128((__m128i const *)&cf->coeffs[8 * i + DTRU_N / 2]);
        __m256i v = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
        
        // -------------------------------------------------
        // 此处开始完全替代 decode_e8 的前半部分 (Cost Calc)
        // -------------------------------------------------
        
        // ---- （tmp_cost[i][0]）路径 0: 计算 abs_q(v)^2 + abs_q(v_pair)^2 ----
        
        // abs_q(x) 等价于 min(x, Q-x) 对于 x in [0, Q)
        __m256i v_sub = _mm256_sub_epi16(Q_vec, v);
        __m256i v_abs0 = _mm256_min_epu16(v, v_sub);
        
        // 我们现在需要计算 squares 并成对相加 (0与8配对, 1与9配对...)
        // v_abs0 结构: [A0..A7 | B0..B7]
        // 我们需要 A0^2 + B0^2
        
        // 对齐 0100 1110 (0x4E)
        __m256i v_perm = _mm256_permute4x64_epi64(v_abs0, 0x4E); // Swap lanes -> [B0..B7 | A0..A7]
        
        __m256i pairs_lo = _mm256_unpacklo_epi16(v_abs0, v_perm); // [A0 B0 A1 B1 A2 B2 A3 B3 | ...]
        __m256i pairs_hi = _mm256_unpackhi_epi16(v_abs0, v_perm); // [A4 B4 A5 B5 A6 B6 A7 B7 | ...]
        
        // dst[i+31:i] := SignExtend32(a[i+31:i+16]*b[i+31:i+16]) + SignExtend32(a[i+15:i]*b[i+15:i])
        // 结果是 32位整数
        // 注意：unpack 的高 128 bit 是冗余或镜像的，但 madd 会一并计算，我们只取我们需要的部分（也就是前半部分或者低 128 bit）
        __m256i cost0_vec_lo = _mm256_madd_epi16(pairs_lo, pairs_lo); // A0^2+B0^2, A1^2+B1^2...
        __m256i cost0_vec_hi = _mm256_madd_epi16(pairs_hi, pairs_hi); // A4^2+B4^2, A5^2+B5^2...
        

        // ---- （tmp_cost[i][1]）路径 1: 计算 abs_q(v - offset)^2 ... ----
        
        // tmp_cost[i][1] = sqr(abs_q(fqcsubq(vec[i    ] - ((DTRU_Q + 1) >> 1)))) +
        //           sqr(abs_q(fqcsubq(vec[i + 8] - ((DTRU_Q + 1) >> 1))));
        __m256i t = _mm256_sub_epi16(v, offset_vec); // v - offset
        // 修正负数: t += (t >> 15) & Q, 使 t 在 [0, Q) 范围内
        __m256i mask_neg = _mm256_srai_epi16(t, 15);
        t = _mm256_add_epi16(t, _mm256_and_si256(mask_neg, Q_vec));
        
        // 现在 t 在 [0, Q) 范围内 (或 [-Q, Q]?)
        // fqcsubq 的输出是 centered，但我们需要 abs.
        // abs_q(t) 在 t 修正为 [0, Q) 后仍然是 min(t, Q-t)
        // 假如 v=0, offset=1000. t = -1000 -> +1048. min(1048, 2048-1048=1000). Correct.
        __m256i t_sub = _mm256_sub_epi16(Q_vec, t);
        __m256i v_abs1 = _mm256_min_epu16(t, t_sub);
        
        // 同样的平方和逻辑
        v_perm = _mm256_permute4x64_epi64(v_abs1, 0x4E);
        pairs_lo = _mm256_unpacklo_epi16(v_abs1, v_perm);
        pairs_hi = _mm256_unpackhi_epi16(v_abs1, v_perm);
        
        __m256i cost1_vec_lo = _mm256_madd_epi16(pairs_lo, pairs_lo);
        __m256i cost1_vec_hi = _mm256_madd_epi16(pairs_hi, pairs_hi);
        
        // ---- 存出数据 ----
        // 此时我们有 4 个 YMM 寄存器包含结果。
        // cost0_vec_lo 的 Low 128bit 包含 cost0[0..3]
        // cost0_vec_hi 的 Low 128bit 包含 cost0[4..7]
        
        // Store costs 0
        _mm_storeu_si128((__m128i*)c0_buf, _mm256_castsi256_si128(cost0_vec_lo));
        _mm_storeu_si128((__m128i*)(c0_buf + 4), _mm256_castsi256_si128(cost0_vec_hi));
        
        // Store costs 1
        _mm_storeu_si128((__m128i*)c1_buf, _mm256_castsi256_si128(cost1_vec_lo));
        _mm_storeu_si128((__m128i*)(c1_buf + 4), _mm256_castsi256_si128(cost1_vec_hi));
        
        // -------------------------------------------------
        // 后半部分: 标量解码 (Trellis)
        // -------------------------------------------------
        uint32_t cost[2], r;
        uint8_t m[2], res;
        
        m[0] = decode_d8_00_avx_internal(cost + 0, c0_buf, c1_buf);
        m[1] = decode_d8_10_avx_internal(cost + 1, c0_buf, c1_buf);
        
        r = ((cost[1] - cost[0]) >> 31) & 1;

        res = ((-(r ^ 1)) & (uint32_t)m[0]) ^ ((-(r & 1)) & (uint32_t)m[1]);
        res = ((((res ^ (res << 1)) & 0x3) | ((res >> 1) & 4)) << 1) | r;
        
        // 原逻辑: msg[i >> 1] |= decode_e8(...) << ((i & 1) << 2);
        msg[i >> 1] |= res << ((i & 1) << 2);
    }
}

void poly_ntt_avx2(poly *b)
{
  ntt_avx2(b->coeffs, b->coeffs);
}

void poly_invntt_avx2(poly *b)
{
  invntt_avx2(b->coeffs, b->coeffs);
}

void poly_basemul_avx2(poly *c, const poly *a, const poly *b)
{
  unsigned int i;
  for (i = 0; i < DTRU_N / 4 / 16; i++)
  {
    basemul_avx2(c->coeffs + 64 * i,
                  a->coeffs + 64 * i,
                  b->coeffs + 64 * i,
                  zetas4inv + 16 * i);
  }
}

int poly_baseinv_avx2(poly *b, const poly *a)
{
  int r = 0;
    r=baseinv_avx2(b->coeffs,a->coeffs);
    return r;
}
