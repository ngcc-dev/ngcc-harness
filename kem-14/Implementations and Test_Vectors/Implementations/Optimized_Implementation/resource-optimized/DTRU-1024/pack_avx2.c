#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "pack.h"

/*
 * pack_sk_avx2 / unpack_sk_avx2
 *
 * 与 pack_sk / unpack_sk 完全兼容（相同的输出格式）。
 * 将 f 的系数 x ∈ [BOUND-2*ETA, BOUND+2*ETA] 编码为 (DTRU_BOUND - x)，
 * 每个系数占 4 bit，两个系数共享一个字节（低4位/高4位）。
 * 输入：1024 × int16_t → 输出：512 bytes
 */
void pack_sk_avx2(unsigned char *r, const poly *a)
{
    const __m256i bound  = _mm256_set1_epi16(DTRU_BOUND);
    const __m256i lomask = _mm256_set1_epi16(0x00FF);   /* 每个 16-bit word 的低字节掩码 */

    for (int i = 0; i < DTRU_N / 32; i++) {
        /* 一次处理 32 个系数 */
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i     ));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i + 16));

        /* t[k] = BOUND - coeff[k]，值域 [0, 10]，放入 int16_t */
        v0 = _mm256_sub_epi16(bound, v0);
        v1 = _mm256_sub_epi16(bound, v1);

        /* 32 × int16_t → 32 × int8_t（值 ≤ 10 ≤ 127，安全） */
        __m256i packed = _mm256_packs_epi16(v0, v1);
        /* packs_epi16 在 lane 内交叉：[v0[0..7],v1[0..7] | v0[8..15],v1[8..15]]
         * 修正为：[v0[0..15] | v1[0..15]]                                       */
        packed = _mm256_permute4x64_epi64(packed, 0xD8);   /* 0b11_01_10_00 */

        /* nibble 打包：out[k] = packed[2k] | (packed[2k+1] << 4)
         * 利用 16-bit 视图：低字节为偶系数，高字节为奇系数                       */
        __m256i lo  = _mm256_and_si256(packed, lomask);          /* 偶字节 → 低 nibble */
        __m256i hi  = _mm256_srli_epi16(packed, 8);               /* 奇字节 → 高 nibble */
        __m256i nib = _mm256_or_si256(lo, _mm256_slli_epi16(hi, 4));

        /* 16 个有效 16-bit → 16 个字节（packus 安全，值 ≤ 10|(10<<4)=170 < 256） */
        __m256i res = _mm256_packus_epi16(nib, _mm256_setzero_si256());
        res = _mm256_permute4x64_epi64(res, 0xD8);   /* 16 个有效字节汇集到低 128 位 */

        _mm_storeu_si128((__m128i *)(r + 16*i), _mm256_castsi256_si128(res));
    }
}

void unpack_sk_avx2(poly *r, const unsigned char *a)
{
    const __m256i bound   = _mm256_set1_epi16(DTRU_BOUND);
    const __m256i nibmask = _mm256_set1_epi8(0x0F);

    for (int i = 0; i < DTRU_N / 32; i++) {
        /* 16 字节 → 32 个 nibble */
        __m128i bytes = _mm_loadu_si128((const __m128i *)(a + 16*i));
        __m256i v     = _mm256_cvtepu8_epi16(bytes);   /* 16 × uint16_t，零扩展 */

        /* lo[i] = byte[i] & 0x0F = t[2i]（偶号系数的编码值）
         * hi[i] = byte[i] >> 4   = t[2i+1]（奇号系数的编码值）               */
        __m256i lo = _mm256_and_si256(v, nibmask);
        __m256i hi = _mm256_srli_epi16(v, 4);

        /* 在每个 128-bit lane 内交织：[lo[0],hi[0], lo[1],hi[1], ...]
         * lane0 of ilo: t[0..7]   lane1 of ilo: t[16..23]
         * lane0 of ihi: t[8..15]  lane1 of ihi: t[24..31]          */
        __m256i ilo = _mm256_unpacklo_epi16(lo, hi);
        __m256i ihi = _mm256_unpackhi_epi16(lo, hi);

        /* 重组为连续的 t[0..15] 和 t[16..31] */
        __m256i r0 = _mm256_permute2x128_si256(ilo, ihi, 0x20);   /* lo lanes */
        __m256i r1 = _mm256_permute2x128_si256(ilo, ihi, 0x31);   /* hi lanes */

        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i     ),
                            _mm256_sub_epi16(bound, r0));
        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i + 16),
                            _mm256_sub_epi16(bound, r1));
    }
}

/*
 * pack_pk_avx2 / unpack_pk_avx2
 *
 * 12-bit 打包，适用于 PK_PACK_OPT=0（DTRU_PKE_PUBLICKEYBYTES = 1536）。
 * 注意：PK_PACK_OPT=1 的熵编码（1506 字节）递归结构无法向量化，
 *       如需使用本函数，请确认 PK_PACK_OPT=0。
 *
 * 每次处理 32 个系数（两个 __m256i）→ 48 字节（3 × 16 字节），
 * 共 32 次迭代，输出 1536 字节。
 *
 * 核心思路：
 *   step1. 将相邻系数对合并为 32-bit word：w = a_even | (a_odd << 12)
 *   step2. 用 pshufb 将每个 32-bit word 的低 3 字节紧凑排列（shuf3）
 *   step3. 将 4 个 12-byte 块（共 48 字节）用移位 + OR 拼成 3 个 16-byte 输出
 */
void pack_pk_avx2(unsigned char *r, const poly *a)
{
    /* shuf3：每个 32-bit word 的字节 [0,1,2] → 输出位置 [3k,3k+1,3k+2]，
     * 位置 12-15 置零。4 个 word / 128-bit → 12 有效字节 + 4 零字节。  */
    const __m128i shuf3 = _mm_set_epi8(
        -1,-1,-1,-1,   /* 位置 15-12: 0 */
        14,13,12,      /* word 3: 输入字节 14,13,12 */
        10, 9, 8,      /* word 2: 输入字节 10,9,8  */
         6, 5, 4,      /* word 1: 输入字节 6,5,4   */
         2, 1, 0       /* word 0: 输入字节 2,1,0   */
    );

    for (int i = 0; i < DTRU_N / 32; i++) {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i     ));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i + 16));

        /* 以 32-bit 为单元分离偶/奇系数并合并 */
        __m256i even0 = _mm256_and_si256(v0, _mm256_set1_epi32(0x0000FFFF));
        __m256i odd0  = _mm256_srli_epi32(v0, 16);
        __m256i c0    = _mm256_or_si256(even0, _mm256_slli_epi32(odd0, 12));

        __m256i even1 = _mm256_and_si256(v1, _mm256_set1_epi32(0x0000FFFF));
        __m256i odd1  = _mm256_srli_epi32(v1, 16);
        __m256i c1    = _mm256_or_si256(even1, _mm256_slli_epi32(odd1, 12));

        /* 对每个 128-bit lane 执行 shuf3，得到 4 × 12-byte 块（各含 4 零尾字节） */
        __m128i a_blk = _mm_shuffle_epi8(_mm256_castsi256_si128(c0),     shuf3); /* a[0..7]   */
        __m128i b_blk = _mm_shuffle_epi8(_mm256_extracti128_si256(c0,1), shuf3); /* a[8..15]  */
        __m128i c_blk = _mm_shuffle_epi8(_mm256_castsi256_si128(c1),     shuf3); /* a[16..23] */
        __m128i d_blk = _mm_shuffle_epi8(_mm256_extracti128_si256(c1,1), shuf3); /* a[24..31] */

        /* 将 4 个 12-byte 块（[valid×12, 0×4]）拼成 3 个对齐的 16-byte 输出：
         *
         *   out0[0..11]  = a_blk[0..11]   out0[12..15] = b_blk[0..3]
         *   out1[0..7]   = b_blk[4..11]   out1[8..15]  = c_blk[0..7]
         *   out2[0..3]   = c_blk[8..11]   out2[4..15]  = d_blk[0..11]
         *
         * 零填充保证 OR 操作不丢数据（两个操作数在对应位置必有一方为 0）。  */
        __m128i out0 = _mm_or_si128(a_blk,                    _mm_slli_si128(b_blk, 12));
        __m128i out1 = _mm_or_si128(_mm_srli_si128(b_blk, 4), _mm_slli_si128(c_blk,  8));
        __m128i out2 = _mm_or_si128(_mm_srli_si128(c_blk, 8), _mm_slli_si128(d_blk,  4));

        _mm_storeu_si128((__m128i *)(r + 48*i     ), out0);
        _mm_storeu_si128((__m128i *)(r + 48*i + 16), out1);
        _mm_storeu_si128((__m128i *)(r + 48*i + 32), out2);
    }
}

void unpack_pk_avx2(poly *r, const unsigned char *a)
{
    /* shuf3_inv：将 12-byte 块还原为 4 个 32-bit word（byte[3] = 0）
     * 输入：[w0b0,w0b1,w0b2, w1b0,w1b1,w1b2, w2b0,w2b1,w2b2, w3b0,w3b1,w3b2, 0,0,0,0]
     * 输出：[w0b0,w0b1,w0b2,0, w1b0,w1b1,w1b2,0, w2b0,w2b1,w2b2,0, w3b0,w3b1,w3b2,0] */
    const __m128i shuf3_inv = _mm_set_epi8(
        -1,11,10, 9,   /* word 3 */
        -1, 8, 7, 6,   /* word 2 */
        -1, 5, 4, 3,   /* word 1 */
        -1, 2, 1, 0    /* word 0 */
    );

    /* 各种字节范围掩码 */
    const __m128i mask12 = _mm_set_epi32(     0, -1, -1, -1);   /* bytes 0-11 有效 */
    const __m128i mask8  = _mm_set_epi64x(    0,          -1);   /* bytes 0-7  有效 */
    const __m128i mask4  = _mm_set_epi32( 0,  0,  0,     -1);   /* bytes 0-3  有效 */

    const __m256i mask12bit = _mm256_set1_epi32(0x00000FFF);      /* 提取 12-bit 值 */

    for (int i = 0; i < DTRU_N / 32; i++) {
        __m128i in0 = _mm_loadu_si128((const __m128i *)(a + 48*i     ));
        __m128i in1 = _mm_loadu_si128((const __m128i *)(a + 48*i + 16));
        __m128i in2 = _mm_loadu_si128((const __m128i *)(a + 48*i + 32));

        /* 从 3 个打包后的 16-byte 寄存器中还原出 4 个 12-byte 块
         *
         * pack 时的映射（逆过来读）：
         *   a_blk[0..11]  → in0[0..11]
         *   b_blk[0..3]   → in0[12..15]   b_blk[4..11]  → in1[0..7]
         *   c_blk[0..7]   → in1[8..15]    c_blk[8..11]  → in2[0..3]
         *   d_blk[0..11]  → in2[4..15]                                     */
        __m128i a_blk = _mm_and_si128(in0, mask12);

        __m128i b_blk = _mm_or_si128(
            _mm_srli_si128(in0, 12),                            /* b[0..3]  ← in0[12..15] */
            _mm_slli_si128(_mm_and_si128(in1, mask8), 4)        /* b[4..11] ← in1[0..7]   */
        );

        __m128i c_blk = _mm_or_si128(
            _mm_srli_si128(in1, 8),                             /* c[0..7]  ← in1[8..15]  */
            _mm_slli_si128(_mm_and_si128(in2, mask4), 8)        /* c[8..11] ← in2[0..3]   */
        );

        __m128i d_blk = _mm_srli_si128(in2, 4);                 /* d[0..11] ← in2[4..15]  */

        /* 用 shuf3_inv 将各 12-byte 块展开为 4 × 32-bit word（低 24 位有效，高字节 = 0） */
        a_blk = _mm_shuffle_epi8(a_blk, shuf3_inv);
        b_blk = _mm_shuffle_epi8(b_blk, shuf3_inv);
        c_blk = _mm_shuffle_epi8(c_blk, shuf3_inv);
        d_blk = _mm_shuffle_epi8(d_blk, shuf3_inv);

        /* 合并为两个 256-bit 寄存器以批量提取系数
         * ab: lane0 = a_blk（a[0..7] 的 4 个 word），lane1 = b_blk（a[8..15] 的 4 个 word）
         * cd: lane0 = c_blk，lane1 = d_blk                                      */
        __m256i ab = _mm256_set_m128i(b_blk, a_blk);
        __m256i cd = _mm256_set_m128i(d_blk, c_blk);

        /* word[j] = coeff_even | (coeff_odd << 12)，提取各自 12 位 */
        __m256i even_ab = _mm256_and_si256(ab, mask12bit);
        __m256i odd_ab  = _mm256_and_si256(_mm256_srli_epi32(ab, 12), mask12bit);
        __m256i even_cd = _mm256_and_si256(cd, mask12bit);
        __m256i odd_cd  = _mm256_and_si256(_mm256_srli_epi32(cd, 12), mask12bit);

        /* 在每个 128-bit lane 内交织偶/奇恢复原始顺序：
         *   lo_ab lane0: [a[0],a[1],a[2],a[3]]   lo_ab lane1: [a[8],a[9],a[10],a[11]]
         *   hi_ab lane0: [a[4],a[5],a[6],a[7]]   hi_ab lane1: [a[12],a[13],a[14],a[15]] */
        __m256i lo_ab = _mm256_unpacklo_epi32(even_ab, odd_ab);
        __m256i hi_ab = _mm256_unpackhi_epi32(even_ab, odd_ab);
        __m256i lo_cd = _mm256_unpacklo_epi32(even_cd, odd_cd);
        __m256i hi_cd = _mm256_unpackhi_epi32(even_cd, odd_cd);

        /* 32-bit → 16-bit 打包（值 ≤ 3456 < 65535，packus 无溢出）
         * result_ab: lane0 = a[0..7]，lane1 = a[8..15]，
         * 写入内存顺序正好是 coeffs[0..15]                                       */
        __m256i result_ab = _mm256_packus_epi32(lo_ab, hi_ab);
        __m256i result_cd = _mm256_packus_epi32(lo_cd, hi_cd);

        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i     ), result_ab);
        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i + 16), result_cd);
    }
}
