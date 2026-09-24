#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "cbd.h"

void cbd1(poly *r, const uint8_t buf[DTRU_CBD1_BYTES])
{
  int i;
  uint8_t t;
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t = buf[i];
    r->coeffs[4 * i + 0] = ((t >> 0) & 1) - ((t >> 4) & 1);
    r->coeffs[4 * i + 1] = ((t >> 1) & 1) - ((t >> 5) & 1);
    r->coeffs[4 * i + 2] = ((t >> 2) & 1) - ((t >> 6) & 1);
    r->coeffs[4 * i + 3] = ((t >> 3) & 1) - ((t >> 7) & 1);
  }
}

void cbd2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES])
{
  int i;
  uint8_t t;
  for (i = 0; i < DTRU_N / 2; i++)
  {
    t = buf[i];
    r->coeffs[2 * i + 0] = ((t >> 0) & 1) + ((t >> 1) & 1) - ((t >> 2) & 1) - ((t >> 3) & 1);
    r->coeffs[2 * i + 1] = ((t >> 4) & 1) + ((t >> 5) & 1) - ((t >> 6) & 1) - ((t >> 7) & 1);
  }
}

void cbd3(poly *r, const uint8_t buf[DTRU_CBD3_BYTES])
{
  int i;
  uint8_t t[3];
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t[0] = buf[3 * i + 0];
    t[1] = buf[3 * i + 1];
    t[2] = buf[3 * i + 2];
    r->coeffs[4 * i + 0] = ((t[0] >> 0) & 1) + ((t[1] >> 0) & 1) + ((t[2] >> 0) & 1) - 
                           ((t[0] >> 1) & 1) - ((t[1] >> 1) & 1) - ((t[2] >> 1) & 1);
    r->coeffs[4 * i + 1] = ((t[0] >> 2) & 1) + ((t[1] >> 2) & 1) + ((t[2] >> 2) & 1) - 
                           ((t[0] >> 3) & 1) - ((t[1] >> 3) & 1) - ((t[2] >> 3) & 1);
    r->coeffs[4 * i + 2] = ((t[0] >> 4) & 1) + ((t[1] >> 4) & 1) + ((t[2] >> 4) & 1) - 
                           ((t[0] >> 5) & 1) - ((t[1] >> 5) & 1) - ((t[2] >> 5) & 1);
    r->coeffs[4 * i + 3] = ((t[0] >> 6) & 1) + ((t[1] >> 6) & 1) + ((t[2] >> 6) & 1) - 
                           ((t[0] >> 7) & 1) - ((t[1] >> 7) & 1) - ((t[2] >> 7) & 1);
  }
}

void cbd1_avx2(poly *r, const uint8_t buf[DTRU_CBD1_BYTES])
{
  int16_t *out = r->coeffs;
  const __m256i ones = _mm256_set1_epi16(1);

  // 循环 32 次，处理 32 * 16 = 512 字节
  // 覆盖 DTRU_CBD1_BYTES (384)
  for (int i = 0; i < DTRU_CBD1_BYTES / 16; ++i)
  {
    // 1. 加载并扩展为 int16
    __m128i input_16 = _mm_loadu_si128((const __m128i *)(buf + i * 16));
    __m256i input = _mm256_cvtepu8_epi16(input_16);

    // 2. 提取所有位 (利用多端口并发优势)
    __m256i b0 = _mm256_and_si256(input, ones); // bit 0
    __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(input, 1), ones);
    __m256i b2 = _mm256_and_si256(_mm256_srli_epi16(input, 2), ones);
    __m256i b3 = _mm256_and_si256(_mm256_srli_epi16(input, 3), ones);

    __m256i b4 = _mm256_and_si256(_mm256_srli_epi16(input, 4), ones);
    __m256i b5 = _mm256_and_si256(_mm256_srli_epi16(input, 5), ones);
    __m256i b6 = _mm256_and_si256(_mm256_srli_epi16(input, 6), ones);
    __m256i b7 = _mm256_and_si256(_mm256_srli_epi16(input, 7), ones);

    // 3. 计算 DTRU 系数 (修正为正确公式)
    // coeff = low_bit - high_bit
    __m256i d0 = _mm256_sub_epi16(b0, b4);
    __m256i d1 = _mm256_sub_epi16(b1, b5);
    __m256i d2 = _mm256_sub_epi16(b2, b6);
    __m256i d3 = _mm256_sub_epi16(b3, b7);

    // 4. 数据交织 (保持不变，逻辑是通用的)
    __m256i lo01 = _mm256_unpacklo_epi16(d0, d1);
    __m256i hi01 = _mm256_unpackhi_epi16(d0, d1);
    __m256i lo23 = _mm256_unpacklo_epi16(d2, d3);
    __m256i hi23 = _mm256_unpackhi_epi16(d2, d3);

    __m256i low = _mm256_unpacklo_epi32(lo01, lo23);
    __m256i high = _mm256_unpackhi_epi32(lo01, lo23);
    __m256i low2 = _mm256_unpacklo_epi32(hi01, hi23);
    __m256i high2 = _mm256_unpackhi_epi32(hi01, hi23);

    // 5. 跨 Lane 重排
    __m256i out0 = _mm256_permute2f128_si256(low, high, 0x20);
    __m256i out1 = _mm256_permute2f128_si256(low2, high2, 0x20);
    __m256i out2 = _mm256_permute2f128_si256(low, high, 0x31);
    __m256i out3 = _mm256_permute2f128_si256(low2, high2, 0x31);

    // 6. 存储 (每次 64 个系数)
    _mm256_storeu_si256((__m256i *)(out + i * 64 + 0 * 16), out0);
    _mm256_storeu_si256((__m256i *)(out + i * 64 + 1 * 16), out1);
    _mm256_storeu_si256((__m256i *)(out + i * 64 + 2 * 16), out2);
    _mm256_storeu_si256((__m256i *)(out + i * 64 + 3 * 16), out3);
  }
}

void cbd2_avx2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES])
{
  int16_t *out = r->coeffs;
  const __m256i ones = _mm256_set1_epi16(1);

  // DTRU_CBD2_BYTES = 1024
  // 每次循环处理 16 字节输入 -> 生成 32 个系数 (64 字节输出)
  // 1024 / 16 = 64 次循环
  // 64 * 32 = 2048 个系数
  for (int i = 0; i < DTRU_CBD2_BYTES / 16; ++i)
  {
    // 1. 加载 16 字节并扩展为 16 个 int16
    __m128i input_16 = _mm_loadu_si128((const __m128i *)(buf + i * 16));
    __m256i input = _mm256_cvtepu8_epi16(input_16);

    // 2. 提取所有位 (利用多端口并发)
    // 这一步虽然指令多，但吞吐量极高
    __m256i b0 = _mm256_and_si256(input, ones);
    __m256i b1 = _mm256_and_si256(_mm256_srli_epi16(input, 1), ones);
    __m256i b2 = _mm256_and_si256(_mm256_srli_epi16(input, 2), ones);
    __m256i b3 = _mm256_and_si256(_mm256_srli_epi16(input, 3), ones);

    __m256i b4 = _mm256_and_si256(_mm256_srli_epi16(input, 4), ones);
    __m256i b5 = _mm256_and_si256(_mm256_srli_epi16(input, 5), ones);
    __m256i b6 = _mm256_and_si256(_mm256_srli_epi16(input, 6), ones);
    __m256i b7 = _mm256_and_si256(_mm256_srli_epi16(input, 7), ones);

    // 3. 计算系数
    // 公式: (bit0 + bit1) - (bit2 + bit3)

    // 低半字节系数 (对应 coeffs 2*k)
    __m256i sum_lo_pos = _mm256_add_epi16(b0, b1);
    __m256i sum_lo_neg = _mm256_add_epi16(b2, b3);
    __m256i c_lo = _mm256_sub_epi16(sum_lo_pos, sum_lo_neg);

    // 高半字节系数 (对应 coeffs 2*k + 1)
    __m256i sum_hi_pos = _mm256_add_epi16(b4, b5);
    __m256i sum_hi_neg = _mm256_add_epi16(b6, b7);
    __m256i c_hi = _mm256_sub_epi16(sum_hi_pos, sum_hi_neg);

    // 4. 交织数据 (Unpack)
    // c_lo: [c0_L, c1_L ... c15_L] (L 表示低半字节生成的系数)
    // c_hi: [c0_H, c1_H ... c15_H] (H 表示高半字节生成的系数)
    // 目标顺序: c0_L, c0_H, c1_L, c1_H ...

    __m256i res_a = _mm256_unpacklo_epi16(c_lo, c_hi);
    // res_a 包含输入字节 0,1,2,3 (Lane0) 和 8,9,10,11 (Lane1) 的结果

    __m256i res_b = _mm256_unpackhi_epi16(c_lo, c_hi);
    // res_b 包含输入字节 4,5,6,7 (Lane0) 和 12,13,14,15 (Lane1) 的结果

    // 5. 跨 Lane 重排 (Permute)
    // 我们需要把 res_a 的 Lane0 和 res_b 的 Lane0 拼在一起 -> 对应输入字节 0-7
    // 我们需要把 res_a 的 Lane1 和 res_b 的 Lane1 拼在一起 -> 对应输入字节 8-15

    // 0x20: 取第一个源的 Lane0 (0) 和第二个源的 Lane0 (0) -> 放在低/高位
    __m256i out0 = _mm256_permute2f128_si256(res_a, res_b, 0x20);

    // 0x31: 取第一个源的 Lane1 (1) 和第二个源的 Lane1 (1) -> 放在低/高位
    __m256i out1 = _mm256_permute2f128_si256(res_a, res_b, 0x31);

    // 6. 存储 (32 个系数，64 字节)
    _mm256_storeu_si256((__m256i *)(out + i * 32 + 0), out0);
    _mm256_storeu_si256((__m256i *)(out + i * 32 + 16), out1);
  }
}

void cbd3_avx2(poly * restrict r, const uint8_t buf[3*DTRU_N/4+8])
{
  unsigned int i;
  __m256i f0, f1, f2, f3;
  const __m256i mask249 = _mm256_set1_epi32(0x249249);
  const __m256i mask6DB = _mm256_set1_epi32(0x6DB6DB);
  const __m256i mask07 = _mm256_set1_epi32(7);
  const __m256i mask70 = _mm256_set1_epi32(7 << 16);
  const __m256i mask3 = _mm256_set1_epi16(3);
  const __m256i shufbidx = _mm256_set_epi8(-1,15,14,13,-1,12,11,10,-1, 9, 8, 7,-1, 6, 5, 4,
                                           -1,11,10, 9,-1, 8, 7, 6,-1, 5, 4, 3,-1, 2, 1, 0);

  for(i = 0; i < DTRU_N/32; i++) {
    f0 = _mm256_loadu_si256((__m256i *)&buf[24*i]);
    f0 = _mm256_permute4x64_epi64(f0,0x94);
    f0 = _mm256_shuffle_epi8(f0,shufbidx);

    f1 = _mm256_srli_epi32(f0,1);
    f2 = _mm256_srli_epi32(f0,2);
    f0 = _mm256_and_si256(mask249,f0);
    f1 = _mm256_and_si256(mask249,f1);
    f2 = _mm256_and_si256(mask249,f2);
    f0 = _mm256_add_epi32(f0,f1);
    f0 = _mm256_add_epi32(f0,f2);

    f1 = _mm256_srli_epi32(f0,3);
    f0 = _mm256_add_epi32(f0,mask6DB);
    f0 = _mm256_sub_epi32(f0,f1);

    f1 = _mm256_slli_epi32(f0,10);
    f2 = _mm256_srli_epi32(f0,12);
    f3 = _mm256_srli_epi32(f0, 2);
    f0 = _mm256_and_si256(f0,mask07);
    f1 = _mm256_and_si256(f1,mask70);
    f2 = _mm256_and_si256(f2,mask07);
    f3 = _mm256_and_si256(f3,mask70);
    f0 = _mm256_add_epi16(f0,f1);
    f1 = _mm256_add_epi16(f2,f3);
    f0 = _mm256_sub_epi16(f0,mask3);
    f1 = _mm256_sub_epi16(f1,mask3);

    f2 = _mm256_unpacklo_epi32(f0,f1);
    f3 = _mm256_unpackhi_epi32(f0,f1);

    f0 = _mm256_permute2x128_si256(f2,f3,0x20);
    f1 = _mm256_permute2x128_si256(f2,f3,0x31);

    // _mm256_store_si256(&r->coeffs[2*i+0], f0);
    // _mm256_store_si256(&r->coeffs[2*i+1], f1);
    _mm256_store_si256((__m256i *)&r->coeffs[32*i+0], f0);
    _mm256_store_si256((__m256i *)&r->coeffs[32*i+16], f1);
  }
}