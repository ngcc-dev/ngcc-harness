#include "pack.h"
#include "params.h"

void pack_pk(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 4; i++)
    {
        r[5 * i + 0] = (a->coeffs[4 * i] >> 0);
        r[5 * i + 1] = (a->coeffs[4 * i] >> 8) | (a->coeffs[4 * i + 1] << 2);
        r[5 * i + 2] = (a->coeffs[4 * i + 1] >> 6) | (a->coeffs[4 * i + 2] << 4);
        r[5 * i + 3] = (a->coeffs[4 * i + 2] >> 4) | (a->coeffs[4 * i + 3] << 6);
        r[5 * i + 4] = (a->coeffs[4 * i + 3] >> 2);
    }
}

void unpack_pk(poly *r, const unsigned char *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 4; i++)
    {
        r->coeffs[4 * i] = ((a[5 * i + 0] >> 0) | ((uint16_t)a[5 * i + 1] << 8)) & 0x3FF;
        r->coeffs[4 * i + 1] = ((a[5 * i + 1] >> 2) | ((uint16_t)a[5 * i + 2] << 6)) & 0x3FF;
        r->coeffs[4 * i + 2] = ((a[5 * i + 2] >> 4) | ((uint16_t)a[5 * i + 3] << 4)) & 0x3FF;
        r->coeffs[4 * i + 3] = ((a[5 * i + 3] >> 6) | ((uint16_t)a[5 * i + 4] << 2)) & 0x3FF;
    }
}

void pack_sk(unsigned char *r, const poly *a)
{
    unsigned int i;
    uint8_t t[8];

    for (i = 0; i < DTRU_N / 8; i++)
    {
        t[0] = DTRU_BOUND - a->coeffs[8 * i + 0];
        t[1] = DTRU_BOUND - a->coeffs[8 * i + 1];
        t[2] = DTRU_BOUND - a->coeffs[8 * i + 2];
        t[3] = DTRU_BOUND - a->coeffs[8 * i + 3];
        t[4] = DTRU_BOUND - a->coeffs[8 * i + 4];
        t[5] = DTRU_BOUND - a->coeffs[8 * i + 5];
        t[6] = DTRU_BOUND - a->coeffs[8 * i + 6];
        t[7] = DTRU_BOUND - a->coeffs[8 * i + 7];
        r[3 * i + 0] = (t[0] >> 0) | (t[1] << 3) | (t[2] << 6) ;
        r[3 * i + 1] = (t[2] >> 2) | (t[3] << 1)  | (t[4] << 4) | (t[5] << 7);
        r[3 * i + 2] = (t[5] >> 1) | (t[6] << 2)| (t[7] << 5);
    }
}

void unpack_sk(poly *r, const unsigned char *a)
{
    int i;
    for (i = 0; i < DTRU_N / 8; ++i)
    {
        r->coeffs[8 * i + 0] = DTRU_BOUND - ((a[3*i] >> 0)& 0x7);
        r->coeffs[8 * i + 1] = DTRU_BOUND - ((a[3*i] >> 3)& 0x7);
        r->coeffs[8 * i + 2] = DTRU_BOUND - (((a[3*i] >> 6) | (a[3*i+1] << 2 ))& 0x7);
        r->coeffs[8 * i + 3] = DTRU_BOUND - ((a[3*i+1] >> 1)& 0x7);
        r->coeffs[8 * i + 4] = DTRU_BOUND - ((a[3*i+1] >> 4)& 0x7);
        r->coeffs[8 * i + 5] = DTRU_BOUND - (((a[3*i+1] >> 7) | (a[3*i+2] << 1 ))& 0x7);
        r->coeffs[8 * i + 6] = DTRU_BOUND - ((a[3*i+2] >> 2)& 0x7);
        r->coeffs[8 * i + 7] = DTRU_BOUND - ((a[3*i+2] >> 5)& 0x7);
    }
}

void pack_ct(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N; ++i)
    {
        r[i] = (uint8_t)a->coeffs[i];
    }
}

void unpack_decompress_ct(poly *r, const unsigned char *a)
{
    unsigned int i;
    int32_t temp;
    for (i = 0; i < DTRU_N; ++i)
    {
        temp = ((int32_t)(a[i] * DTRU_Q) + (DTRU_Q2 >> 1)) >> DTRU_LOGQ2;
        r->coeffs[i] = temp;
    }
}


// void pack_sk_f(unsigned char *r, const poly *a)
// {
//   int i;
//   unsigned char c;

//   for(i=0; i<DTRU_N/5; i++)
//   {
//     c =       (a->coeffs[5*i+4] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+3] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+2] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+1] + DTRU_ETA) & 255;
//     c = (3*c + a->coeffs[5*i+0] + DTRU_ETA) & 255;
//     r[i] = c;
//   }
// #if DTRU_N > (DTRU_N / 5) * 5  
//   int j;
//   i = DTRU_N / 5;
//   c = 0;
//   for(j = DTRU_N - (5*i) - 1; j>=0; j--)
//     c = (3*c + a->coeffs[5*i+j] + DTRU_ETA) & 255;
//   r[i] = c;
// #endif
// }

// void unpack_sk_f(poly *r, const unsigned char *a)
// {
//   int i;
//   unsigned char c;

//   for(i=0; i<DTRU_N/5; i++)
//   {
//     c = a[i];
//     r->coeffs[5*i+0] = (c%3 - DTRU_ETA) << 1; c/=3; 
//     r->coeffs[5*i+1] = (c%3 - DTRU_ETA) << 1; c/=3;  
//     r->coeffs[5*i+2] = (c%3 - DTRU_ETA) << 1; c/=3; 
//     r->coeffs[5*i+3] = (c%3 - DTRU_ETA) << 1; c/=3;  
//     r->coeffs[5*i+4] = (c%3 - DTRU_ETA) << 1;  
//   }
// #if DTRU_N > (DTRU_N / 5) * 5  // if 5 does not divide NTRU_N-1
//   i = DTRU_N/5;
//   int j;
//   c = a[i];
//   for(j=0; (5*i+j)<DTRU_N; j++)
//   {
//     r->coeffs[5*i+j] =(c%3 - DTRU_ETA) << 1; c /= 3;
//   }
// #endif
//   r->coeffs[0] += 1; 
// }

void pack_pk_avx(unsigned char *r, const poly *a)
{
    unsigned int i;
    
    // AVX2 优化路径: 每次处理 16 个系数 (产生 20 字节输出)
    // DTRU_N 为 512，可以被 16 整除，不需要处理尾部
    for (i = 0; i < DTRU_N / 16; i++)
    {
        // 1. 加载 16 个系数 (256 bits)
        __m256i v = _mm256_loadu_si256((__m256i const *)&a->coeffs[16 * i]);

        // 2. 准备掩码和常量
        // 每个 64-bit lane 包含 4 个 int16: [c3 c2 c1 c0]
        // 目标是将它们压缩成: c0 | c1<<10 | c2<<20 | c3<<30
        
        // 掩码: 0x3FF (10 bits)
        __m256i mask_0 = _mm256_set1_epi64x(0x3FF);
        __m256i mask_1 = _mm256_set1_epi64x(0x3FF << 10);
        __m256i mask_2 = _mm256_set1_epi64x(0x3FF << 20);
        __m256i mask_3 = _mm256_set1_epi64x((long long)0x3FF << 30);

        // 3. 并行位移和组合 (处理 4 个 qword 通道)
        // c0 不需要移动 (offset 0 -> 0)
        __m256i t0 = _mm256_and_si256(v, mask_0);
        
        // c1 在 bits 16-31, 需要移动到 10-25 (右移 6)
        __m256i t1 = _mm256_and_si256(_mm256_srli_epi64(v, 6), mask_1);

        // c2 在 bits 32-47, 需要移动到 20-35 (右移 12)
        __m256i t2 = _mm256_and_si256(_mm256_srli_epi64(v, 12), mask_2);

        // c3 在 bits 48-63, 需要移动到 30-45 (右移 18)
        __m256i t3 = _mm256_and_si256(_mm256_srli_epi64(v, 18), mask_3);

        // 合并结果
        __m256i res = _mm256_or_si256(_mm256_or_si256(t0, t1), _mm256_or_si256(t2, t3));

        // 此时，每个 64-bit lane 的低 40 bits 包含有效的 5 个字节
        // 我们需要将这些分散的字节压缩到一起
        // Lane 布局: [有效5B][垃圾3B] [有效5B][垃圾3B] ...
        
        // 构建 Shuffle 掩码
        // 输入 qword0 的字节: 0,1,2,3,4 (我们需要)
        // 输入 qword1 的字节: 8,9,10,11,12 (我们需要) -> 映射到目标 5,6,7,8,9
        const __m256i shuffle_mask = _mm256_set_epi8(
            // High 128-bit lane (对应 qword 2 和 3)
            -1, -1, -1, -1, -1, -1,     // 填充
            12, 11, 10, 9, 8,           // qword 3 的有效字节
            4, 3, 2, 1, 0,              // qword 2 的有效字节
            
            // Low 128-bit lane (对应 qword 0 和 1)
            -1, -1, -1, -1, -1, -1,     // 填充
            12, 11, 10, 9, 8,           // qword 1 的有效字节 (相对于 lane start 的偏移)
            4, 3, 2, 1, 0               // qword 0 的有效字节
        );

        res = _mm256_shuffle_epi8(res, shuffle_mask);

        // 4. 存储结果
        // 现在的布局:
        // Low 128 lane: [10 bytes VALID] [6 bytes GARBAGE]
        // High 128 lane: [10 bytes VALID] [6 bytes GARBAGE]
        
        // 存储低 128 位中的 10 字节
        __m128i low_lane = _mm256_castsi256_si128(res);
        _mm_storel_epi64((__m128i*)(r + 20 * i), low_lane); // 存前 8 字节
        *(uint16_t*)(r + 20 * i + 8) = (uint16_t)_mm_cvtsi128_si32(_mm_srli_si128(low_lane, 8)); // 存接下来的 2 字节

        // 存储高 128 位中的 10 字节
        __m128i high_lane = _mm256_extracti128_si256(res, 1);
        _mm_storel_epi64((__m128i*)(r + 20 * i + 10), high_lane); // 存前 8 字节
        *(uint16_t*)(r + 20 * i + 18) = (uint16_t)_mm_cvtsi128_si32(_mm_srli_si128(high_lane, 8)); // 存最后 2 字节
    }
}

void unpack_pk_avx(poly *r, const unsigned char *a)
{
    unsigned int i;
    // 掩码: 保留低 10 位
    __m256i mask_3ff = _mm256_set1_epi16(0x3FF);
    
    // Shuffle 掩码: 负责将字节分发到 16-bit 字中
    // 模式: [b0 b1], [b1 b2], [b2 b3], [b3 b4], [b5 b6] ...
    // _mm256_set_epi8 参数顺序从高字节到低字节
    // 该掩码应用于两个 128-bit lane，定义了如何从输入的 16 字节中抓取数据形成 8 个 uint16
    __m256i shuf_mask = _mm256_set_epi8(
        9, 8, 8, 7, 7, 6, 6, 5, 4, 3, 3, 2, 2, 1, 1, 0, // High 128-bit lane definition
        9, 8, 8, 7, 7, 6, 6, 5, 4, 3, 3, 2, 2, 1, 1, 0  // Low 128-bit lane definition
    );
    
    // 每次处理 16 个系数 (20 字节输入)
    for(i = 0; i < DTRU_N / 16; i++)
    {
        // 1. 加载数据
        // 我们利用两次 128-bit 加载来构造 256-bit 向量，以处理非对齐的偏移
        // Low lane 需要处理字节 0-9 (覆盖在 0-15 中)
        __m128i lo = _mm_loadu_si128((__m128i const *)(a + 20 * i));
        
        // High lane 需要处理字节 10-19 (覆盖在 10-25 中)
        // 注意：如果你有非常严格的边界检查工具，最后一次迭代可能会读取越界几个字节，
        // 但标准加密实现中缓冲区通常有少量 padding，或者在栈上分配稍大空间。
        __m128i hi = _mm_loadu_si128((__m128i const *)(a + 20 * i + 10));
        
        __m256i v = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
        
        // 2. 字节重排
        // 现在 v 中的每个 16-bit 字包含了正确的数据，但是位偏移不对
        v = _mm256_shuffle_epi8(v, shuf_mask);
        
        // 3. 位对齐 (Shift & Blend)
        // 我们需要的右移量模式是 0, 2, 4, 6, 0, 2, 4, 6 ...
        
        // 计算右移版本
        __m256i v2 = _mm256_srli_epi16(v, 2);
        __m256i v4 = _mm256_srli_epi16(v2, 2); // v >> 4
        __m256i v6 = _mm256_srli_epi16(v4, 2); // v >> 6
        
        // 使用 Blend 选择正确的结果
        // 初始 v 对应偏移 0 的系数 (Indices: 0, 4, 8, 12)
        
        // 混合偏移 2 的系数 (Indices: 1, 5, 9, 13) -> Mask: 0x22 (00100010)
        v = _mm256_blend_epi16(v, v2, 0x22);
        
        // 混合偏移 4 的系数 (Indices: 2, 6, 10, 14) -> Mask: 0x44 (01000100)
        v = _mm256_blend_epi16(v, v4, 0x44);
        
        // 混合偏移 6 的系数 (Indices: 3, 7, 11, 15) -> Mask: 0x88 (10001000)
        v = _mm256_blend_epi16(v, v6, 0x88);
        
        // 4. 最终掩码
        v = _mm256_and_si256(v, mask_3ff);
        
        // 5. 存储
        _mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], v);
    }
}

void pack_sk_avx(unsigned char *r, const poly *a)
{
    unsigned int i;
    // 用于计算 DTRU_BOUND - coeffs
    __m256i bound = _mm256_set1_epi16(DTRU_BOUND);
    
    // 用于 MADD 指令: 将每对系数 [c0, c1] 变为 c0 + 8*c1
    // 0x00080001 表示低16位是1，高16位是8
    __m256i mul_8_1 = _mm256_set1_epi32(0x00080001);

    // 用于变量移位: 将合并后的块 (6 bits) 移到正确位置
    // 需要的位移量: 0, 6, 12, 18
    __m256i shift_mask = _mm256_set_epi32(18, 12, 6, 0, 18, 12, 6, 0);

    // 每次处理 16 个系数 (生成 6 字节输出)
    for (i = 0; i < DTRU_N / 16; i++) 
    {
        // 1. 加载 16 个系数
        __m256i v = _mm256_loadu_si256((__m256i const *)&a->coeffs[16 * i]);

        // 2. 减法预处理: t = BOUND - a
        v = _mm256_sub_epi16(bound, v);

        // 3. 合并相邻对 (Pairwise Merge)
        // 输入: [c0, c1, c2, c3, ...] (16-bit)
        // 输出: [c0+8c1, c2+8c3, ...] (32-bit)
        // 每个 32-bit 结果包含 6 bits 有效数据 (2 个系数)
        __m256i v32 = _mm256_madd_epi16(v, mul_8_1);

        // 4. 变量位移 (Variable Shift)
        // 将 4 个 32-bit 块分别左移 0, 6, 12, 18 位
        v32 = _mm256_sllv_epi32(v32, shift_mask);

        // 5. 水平合并 (Horizontal Fold)
        // 这个 128-bit lane 内现在有 4 个部分，需要 OR 在一起
        // [A, B, C, D] -> [A|B, B|A, C|D, D|C]
        __m256i t1 = _mm256_shuffle_epi32(v32, 0xB1); // Swap adjacent
        __m256i sum = _mm256_or_si256(v32, t1); 
        
        // [AB, BA, CD, DC] -> [AB|CD, ...]
        __m256i t2 = _mm256_shuffle_epi32(sum, 0x4E); // Swap pairs
        sum = _mm256_or_si256(sum, t2);
        
        // 此时，Low 128 lane 的元素 0 包含了 coeffs[0..7] 的打包结果 (24 bits)
        // High 128 lane 的元素 0 (index 4) 包含了 coeffs[8..15] 的打包结果 (24 bits)

        // 6. 提取并存储
        uint32_t val0 = _mm_cvtsi128_si32(_mm256_castsi256_si128(sum));
        uint32_t val1 = _mm_cvtsi128_si32(_mm256_extracti128_si256(sum, 1));

        unsigned char *out = r + 6 * i;
        
        // 存储前 8 系数 (3 字节)
        out[0] = (unsigned char)(val0);
        out[1] = (unsigned char)(val0 >> 8);
        out[2] = (unsigned char)(val0 >> 16);
        
        // 存储后 8 系数 (3 字节)
        out[3] = (unsigned char)(val1);
        out[4] = (unsigned char)(val1 >> 8);
        out[5] = (unsigned char)(val1 >> 16);
    }
}

void unpack_sk_avx(poly *r, const unsigned char *a)
{
    unsigned int i;
    
    // 常量定义
    __m256i bound = _mm256_set1_epi16(DTRU_BOUND);
    __m256i mask_7 = _mm256_set1_epi16(0x7);
    
    // 1. Shuffle 掩码: 
    // 目的是针对每个系数，将其依赖的字节放到对应的 16-bit lane 中。
    // Low 128 lane 处理前 8 系数 (依赖字节 0, 1, 2)
    // High 128 lane 处理后 8 系数 (依赖字节 3, 4, 5)
    // _mm256_set_epi8 是从高到低定义的 (Lane 15 ... Lane 0)
    __m256i shuf;
    // 总是取相邻两字节作为 uint16，
    // 即 [b0, b1], [b0, b1] ... 然后位移。
    // 简化的 Shuffle 模式:
    shuf = _mm256_set_epi8(
        // High Lane (Bytes 3..5)
        6,5, 6,5,                  // c7, c6 (byte pairs 5,6 - byte 6 is next block/garbage safe)
        5,4, 5,4, 5,4,             // c5, c4, c3 (byte pairs 4,5)
        4,3, 4,3, 4,3,             // c2, c1, c0 relative to block (byte pairs 3,4)
        
        // Low Lane (Bytes 0..2)
        3,2, 3,2,                  // c7, c6 (byte pairs 2,3)
        2,1, 2,1, 2,1,             // c5, c4, c3 (byte pairs 1,2)
        1,0, 1,0, 1,0              // c2, c1, c0 (byte pairs 0,1)
    );

    // 2. 可变位移乘数 (Variable Shift via Multiply)
    // 目标是将 3 bits 数据对齐到 bit 13 (即 0xE000 位置)
    // 原始位移: c0(0), c1(3), c2(6), c3(1), c4(4), c5(7), c6(2), c7(5)
    // 目标位移: 13
    // 左移量 = 13 - 原始位移
    // 乘数 = 1 << 左移量
    // Lane:    0      1      2     3     4     5     6     7
    // Shift:   13     10     7     12    9     6     11    8
    __m256i sl_mul = _mm256_set_epi16(
        1<<8, 1<<11, 1<<6, 1<<9, 1<<12, 1<<7, 1<<10, 1<<13, // High lane
        1<<8, 1<<11, 1<<6, 1<<9, 1<<12, 1<<7, 1<<10, 1<<13  // Low lane
    );

    for (i = 0; i < DTRU_N / 16; i++) 
    {
        // 1. 加载 6 个字节 (利用 loadl_epi64 加载 8 字节是安全的，只要 buffer 分配有微小 padding)
        __m128i raw = _mm_loadl_epi64((__m128i const*)(a + 6*i));
        
        // 2. 广播到两个 128-bit lane
        __m256i v = _mm256_broadcastq_epi64(raw);

        // 3. Shuffle: 将字节放入正确位置
        v = _mm256_shuffle_epi8(v, shuf);

        // 4. 左移对齐: 使用乘法将目标 3 bits 移动到 [15:13] 附近的 bit 13 起始处
        // 这一步会将低位的垃圾数据左移到中间，高位垃圾数据移出
        v = _mm256_mullo_epi16(v, sl_mul);

        // 5. 统一右移: 将 bit 13 移回 bit 0
        // 这会自动清除高位数据 (0 填充)
        v = _mm256_srli_epi16(v, 13);

        // 6. 掩码: 清除低位可能的残留垃圾 (来自原始数据的低位被左移后残留)
        v = _mm256_and_si256(v, mask_7);

        // 7. 计算 Bound - x
        v = _mm256_sub_epi16(bound, v);

        // 8. 存储
        _mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], v);
    }
}

void pack_ct_avx(unsigned char *r, const poly *a)
{
    unsigned int i;
    
    // Shuffle 掩码: 提取每个 16-bit 整数的低字节
    // Lane 内部索引: 0, 2, 4, 6, 8, 10, 12, 14 (前8字节)
    // 后8字节设置为 -1 (清零/垃圾数据，稍后会忽略)
    __m256i shuf = _mm256_set_epi8(
        -1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0, // High Lane
        -1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0  // Low Lane
    );

    // DTRU_N = 512, 每次处理 16 个系数
    for (i = 0; i < DTRU_N / 16; i++)
    {
        // 1. 加载 16 个系数 (32 字节)
        __m256i v = _mm256_loadu_si256((__m256i const *)&a->coeffs[16 * i]);

        // 2. 压缩: 仅保留低字节
        // 结果 v 的布局 (每个块 64bit): [Garbage, Valid_Hi, Garbage, Valid_Lo]
        v = _mm256_shuffle_epi8(v, shuf);

        // 3. 排列: 将分散的有效 64bit 块聚集到低 128 位
        // Control 0x08 (00 00 10 00) -> 选取索引 0 和 2 到低位
        // 这样结果的低 128 位就变成了 [Valid_Lo, Valid_Hi]
        v = _mm256_permute4x64_epi64(v, 0x08);

        // 4. 存储 16 字节
        _mm_storeu_si128((__m128i *)(r + 16 * i), _mm256_castsi256_si128(v));
    }
}

void unpack_decompress_ct_avx(poly *r, const unsigned char *a)
{
    unsigned int i;
    
    // 准备常量 (广播到整个向量)
    __m256i vec_q = _mm256_set1_epi32(DTRU_Q);
    // 舍入偏移量: DTRU_Q2 >> 1
    __m256i vec_offset = _mm256_set1_epi32(DTRU_Q2 >> 1); 
    
    // DTRU_N = 512, 每次循环处理 16 个系数
    for(i = 0; i < DTRU_N / 16; ++i)
    {
        // 1. 加载 16 个字节 (8-bit)
        __m128i in_u8 = _mm_loadu_si128((__m128i const*)(a + 16*i));
        
        // 2. 扩展到 16-bit (Zero extend)
        __m256i in_16 = _mm256_cvtepu8_epi16(in_u8);
        
        // 3. 拆分并扩展到 32-bit (为了防止乘法溢出)
        // DTRU_Q * 255 肯定超过 UINT16_MAX，必须用 int32
        
        // 低 128 位 (前 8 个系数) 扩展为 32位
        __m256i in_32_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(in_16));
        
        // 高 128 位 (后 8 个系数) 扩展为 32位
        __m256i in_32_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(in_16, 1));
        
        // 4. 并行运算: temp = ((val * Q) + offset) >> LOGQ2
        
        // 计算 Low 部分
        __m256i res_lo = _mm256_mullo_epi32(in_32_lo, vec_q);
        res_lo = _mm256_add_epi32(res_lo, vec_offset);
        res_lo = _mm256_srai_epi32(res_lo, DTRU_LOGQ2); // 算术右移
        
        // 计算 High 部分
        __m256i res_hi = _mm256_mullo_epi32(in_32_hi, vec_q);
        res_hi = _mm256_add_epi32(res_hi, vec_offset);
        res_hi = _mm256_srai_epi32(res_hi, DTRU_LOGQ2);
        
        // 5. 压缩 (Pack) 回 16-bit
        // _mm256_packs_epi32 将两个寄存器的 32位整数压缩为 16位 (带饱和)
        // 输入 A=[Lo0..Lo7], B=[Hi0..Hi7]
        // AVX2 Pack 的输出布局比较特殊 (按 128-bit Lane 独立处理):
        // Lane 0: Lo0..Lo3, Hi0..Hi3
        // Lane 1: Lo4..Lo7, Hi4..Hi7
        __m256i packed = _mm256_packs_epi32(res_lo, res_hi);
        
        // 6. 修正顺序 (Permute)
        // 我们现在的顺序是: [A_low, B_low, A_high, B_high] (每块 64 位)
        // 我们需要的顺序是: [A_low, A_high, B_low, B_high] (即 A 然后 B)
        // 索引模式: 0 (A_low), 2 (A_high), 1 (B_low), 3 (B_high) -> 0xD8 (11 01 10 00)
        packed = _mm256_permute4x64_epi64(packed, 0xD8);
        
        // 7. 存储结果
        _mm256_storeu_si256((__m256i *)&r->coeffs[16*i], packed);
    }
}