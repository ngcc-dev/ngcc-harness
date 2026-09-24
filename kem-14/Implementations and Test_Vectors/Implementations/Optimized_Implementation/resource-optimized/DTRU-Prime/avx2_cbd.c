#include "avx2_cbd.h"

// 辅助函数：将 32 字节 (256-bit) 扩展为 16 个 int16_t 并存储
// 输入 bytes 包含 32 个 int8_t，函数将其分为低 16 个和高 16 个分别存储
static inline void store_bytes_as_words(int16_t *dest, __m256i bytes) {
    __m128i lo = _mm256_castsi256_si128(bytes);
    __m128i hi = _mm256_extracti128_si256(bytes, 1);
    
    // 符号扩展 byte -> word (int16)
    _mm256_storeu_si256((__m256i*)dest, _mm256_cvtepi8_epi16(lo));
    _mm256_storeu_si256((__m256i*)(dest + 16), _mm256_cvtepi8_epi16(hi));
}
void cbd1_avx2_fast(poly *r, const uint8_t buf[DTRU_CBD1_BYTES]) {
    int16_t *out = r->coeffs;
    const __m256i ones = _mm256_set1_epi16(1);

    // 循环 17 次，处理 17 * 16 = 272 字节
    // 覆盖 DTRU_CBD1_BYTES (272)
    for (int i = 0; i < 17; ++i) {
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

        __m256i low  = _mm256_unpacklo_epi32(lo01, lo23);
        __m256i high = _mm256_unpackhi_epi32(lo01, lo23);
        __m256i low2 = _mm256_unpacklo_epi32(hi01, hi23);
        __m256i high2 = _mm256_unpackhi_epi32(hi01, hi23);

        // 5. 跨 Lane 重排
        __m256i out0 = _mm256_permute2f128_si256(low,  high, 0x20);
        __m256i out1 = _mm256_permute2f128_si256(low2, high2, 0x20);
        __m256i out2 = _mm256_permute2f128_si256(low,  high, 0x31);
        __m256i out3 = _mm256_permute2f128_si256(low2, high2, 0x31);

        // 6. 存储 (每次 64 个系数)
        _mm256_storeu_si256((__m256i *)(out + i * 64 + 0 * 16), out0);
        _mm256_storeu_si256((__m256i *)(out + i * 64 + 1 * 16), out1);
        _mm256_storeu_si256((__m256i *)(out + i * 64 + 2 * 16), out2);
        _mm256_storeu_si256((__m256i *)(out + i * 64 + 3 * 16), out3);
    }
    
    // 尾部处理：
    // 循环处理了 17 * 16 = 272 字节。
    // DTRU_CBD1_BYTES 正好是 272。
    // 所以 buf 已经读完了。
    // 循环输出了 17 * 64 = 1088 个系数。
    // DTRU_N = 1087。
    // 所以 r->coeffs[1087] 被多写了一个值（这是安全的，只要 poly 结构体有对齐填充）。
    // 不需要额外的标量处理代码。
}
void cbd2_avx2_fast(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]) {
    int16_t *out = r->coeffs;
    const __m256i ones = _mm256_set1_epi16(1);

    // DTRU_CBD2_BYTES = 544
    // 每次循环处理 16 字节输入 -> 生成 32 个系数 (64 字节输出)
    // 544 / 16 = 34 次循环
    // 34 * 32 = 1088 个系数 (覆盖 N=1087)
    for (int i = 0; i < 34; ++i) {
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
    
    // 尾部处理:
    // 34 * 16 = 544 字节，完全覆盖 DTRU_CBD2_BYTES。
    // 34 * 32 = 1088 系数，覆盖 DTRU_N=1087。
    // 无需额外标量代码。
}
void cbd1_avx2_intrinsic(poly *r, const uint8_t buf[DTRU_CBD1_BYTES])
{
    int i;
    const int loop_count = 8;
    const int bytes_per_loop = 32;
    const int coeffs_per_loop = 128;

    __m256i mask_f = _mm256_set1_epi8(0x0F);
    __m256i lut[4];

    // 预计算 LUT
    for(int k=0; k<4; k++) {
        int8_t tmp[32]; 
        for(int j=0; j<16; j++) {
            tmp[j] = (j >> k) & 1;
            tmp[16+j] = tmp[j];
        }
        lut[k] = _mm256_loadu_si256((__m256i*)tmp);
    }

    for (i = 0; i < loop_count; i++) {
        __m256i src = _mm256_loadu_si256((__m256i*)&buf[i * bytes_per_loop]);

        __m256i low = _mm256_and_si256(src, mask_f);
        __m256i high = _mm256_srli_epi16(src, 4);
        high = _mm256_and_si256(high, mask_f);

        __m256i c[4];
        for(int k=0; k<4; k++) {
            __m256i l_bit = _mm256_shuffle_epi8(lut[k], low);
            __m256i h_bit = _mm256_shuffle_epi8(lut[k], high);
            c[k] = _mm256_sub_epi8(l_bit, h_bit);
        }

        // Unpack 阶段
        // Lane 0 包含 input[0..15] 的混合
        // Lane 1 包含 input[16..31] 的混合
        __m256i m0 = _mm256_unpacklo_epi8(c[0], c[1]);
        __m256i m1 = _mm256_unpackhi_epi8(c[0], c[1]);
        __m256i m2 = _mm256_unpacklo_epi8(c[2], c[3]);
        __m256i m3 = _mm256_unpackhi_epi8(c[2], c[3]);

        __m256i r0 = _mm256_unpacklo_epi16(m0, m2); // Low lane: Bytes 0-3, High lane: Bytes 16-19
        __m256i r1 = _mm256_unpackhi_epi16(m0, m2); // Low lane: Bytes 4-7, High lane: Bytes 20-23
        __m256i r2 = _mm256_unpacklo_epi16(m1, m3); // Low lane: Bytes 8-11, High lane: Bytes 24-27
        __m256i r3 = _mm256_unpackhi_epi16(m1, m3); // Low lane: Bytes 12-15, High lane: Bytes 28-31

        // === 重排 Lane ===
        
        // 组合 r0_low (Bytes 0-3) 和 r1_low (Bytes 4-7) -> 结果 (Bytes 0-7)
        __m256i dest0 = _mm256_permute2x128_si256(r0, r1, 0x20);
        // 组合 r2_low (Bytes 8-11) 和 r3_low (Bytes 12-15) -> 结果 (Bytes 8-15)
        __m256i dest1 = _mm256_permute2x128_si256(r2, r3, 0x20);
        // 组合 r0_high (Bytes 16-19) 和 r1_high (Bytes 20-23) -> 结果 (Bytes 16-23)
        __m256i dest2 = _mm256_permute2x128_si256(r0, r1, 0x31);
        // 组合 r2_high (Bytes 24-27) 和 r3_high (Bytes 28-31) -> 结果 (Bytes 24-31)
        __m256i dest3 = _mm256_permute2x128_si256(r2, r3, 0x31);

        // 存储
        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 0, dest0);
        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 32, dest1);
        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 64, dest2);
        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 96, dest3);
    }

    // 标量尾部处理 (保持不变)
    int start_byte = loop_count * bytes_per_loop;
    int j;
    uint8_t t;
    for (j = start_byte; j < DTRU_N / 4; j++) {
        t = buf[j];
        r->coeffs[4 * j + 0] = ((t >> 0) & 1) - ((t >> 4) & 1);
        r->coeffs[4 * j + 1] = ((t >> 1) & 1) - ((t >> 5) & 1);
        r->coeffs[4 * j + 2] = ((t >> 2) & 1) - ((t >> 6) & 1);
        r->coeffs[4 * j + 3] = ((t >> 3) & 1) - ((t >> 7) & 1);
    }
    if (4 * j < DTRU_N) {
        t = buf[j];
        int rem = DTRU_N - 4 * j;
        if(rem >= 1) r->coeffs[4 * j + 0] = ((t >> 0) & 1) - ((t >> 4) & 1);
        if(rem >= 2) r->coeffs[4 * j + 1] = ((t >> 1) & 1) - ((t >> 5) & 1);
        if(rem >= 3) r->coeffs[4 * j + 2] = ((t >> 2) & 1) - ((t >> 6) & 1);
    }
}

void cbd2_avx2_intrinsic(poly *r, const uint8_t buf[DTRU_CBD2_BYTES])
{
    int i;
    const int loop_count = 16;
    const int bytes_per_loop = 32;
    const int coeffs_per_loop = 64;

    __m256i mask_f = _mm256_set1_epi8(0x0F);
    
    int8_t lut_vals[32];
    for(int k=0; k<16; k++) {
        lut_vals[k] = ((k >> 0) & 1) + ((k >> 1) & 1) - ((k >> 2) & 1) - ((k >> 3) & 1);
        lut_vals[16+k] = lut_vals[k];
    }
    __m256i lut = _mm256_loadu_si256((__m256i*)lut_vals);

    for (i = 0; i < loop_count; i++) {
        __m256i src = _mm256_loadu_si256((__m256i*)&buf[i * bytes_per_loop]);

        __m256i low = _mm256_and_si256(src, mask_f);
        __m256i high = _mm256_srli_epi16(src, 4);
        high = _mm256_and_si256(high, mask_f);

        __m256i c0 = _mm256_shuffle_epi8(lut, low);
        __m256i c1 = _mm256_shuffle_epi8(lut, high);

        // Unpack:
        // res0 Low: Bytes 0-7, High: Bytes 16-23
        // res1 Low: Bytes 8-15, High: Bytes 24-31
        __m256i res0 = _mm256_unpacklo_epi8(c0, c1);
        __m256i res1 = _mm256_unpackhi_epi8(c0, c1);

        // === 重排 Lane ===
        
        // 组合 res0_low (Bytes 0-7) 和 res1_low (Bytes 8-15)
        __m256i dest0 = _mm256_permute2x128_si256(res0, res1, 0x20);
        
        // 组合 res0_high (Bytes 16-23) 和 res1_high (Bytes 24-31)
        __m256i dest1 = _mm256_permute2x128_si256(res0, res1, 0x31);

        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 0, dest0);
        store_bytes_as_words(r->coeffs + i * coeffs_per_loop + 32, dest1);
    }

    // 标量尾部处理 (保持不变)
    int start_byte = loop_count * bytes_per_loop;
    int j;
    uint8_t t;
    for (j = start_byte; j < DTRU_N / 2; j++) {
        t = buf[j];
        r->coeffs[2 * j + 0] = ((t >> 0) & 1) + ((t >> 1) & 1) - ((t >> 2) & 1) - ((t >> 3) & 1);
        r->coeffs[2 * j + 1] = ((t >> 4) & 1) + ((t >> 5) & 1) - ((t >> 6) & 1) - ((t >> 7) & 1);
    }
    if (2 * j < DTRU_N) {
        t = buf[j];
        r->coeffs[2 * j + 0] = ((t >> 0) & 1) + ((t >> 1) & 1) - ((t >> 2) & 1) - ((t >> 3) & 1);
    }
}
