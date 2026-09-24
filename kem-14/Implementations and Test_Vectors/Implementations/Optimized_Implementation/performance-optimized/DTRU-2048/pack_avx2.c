#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "pack.h"

static inline __m256i compute_words(__m256i v, __m256i sc1, __m256i sc3)
{
    __m256i s1 = _mm256_madd_epi16(v, sc1);
    __m256i s2 = _mm256_mullo_epi32(s1, sc3);
    __m256i sw = _mm256_shuffle_epi32(s2, 0x4E);
    __m256i ab = _mm256_add_epi32(s2, sw);
    __m256i sw2 = _mm256_shuffle_epi32(ab, 0xB1);
    return _mm256_add_epi32(ab, sw2);
}

/* =========================================================================
 * pack_sk_avx2 / unpack_sk_avx2
 *
 * ETA=1, BOUND=3：每个系数 x 存为 t = BOUND-x ∈ {0,2,4}，占 3 bit。
 * 8 个系数打包进 3 个字节（8×3=24 位），格式与标量 pack_sk 完全相同。
 * N=2048 → DTRU_PKE_SECRETKEYBYTES = 768 bytes
 *
 * pack 思路：
 *   每次处理 64 个系数（= 8 组 × 8）→ 24 字节。4 次 AVX2 读取。
 *   对每个 256-bit 寄存器（= 2 组 × 8 系数，一组占一个 128-bit lane）：
 *     step1. madd_epi16([1,8,...])  → 4×int32：p_k = t[2k]+8*t[2k+1]  ∈[0,36]
 *     step2. mullo_epi32([1,64,4096,262144]) → 位移到各自不重叠的位域
 *     step3. 两次 shuffle+add 横向归约（位域不重叠故加法等价于 OR）
 *            → word = t0|(t1<<3)|...|(t7<<21)，24-bit 值放在 int32 中
 *     提取 2 个 word → __m128i [w0,w1,w2,w3]，再用 shuf3 得 12 字节块
 *   四块 (a_blk,b_blk) 合并为 16+8=24 字节输出。
 *
 * unpack 思路：
 *   每次处理 24 字节（8 组）→ 64 个 int16_t。
 *   构造 W01[k]=a[3k]|(a[3k+1]<<8) 和 W12[k]=a[3k+1]|(a[3k+2]<<8)
 *   其中第 5 组（W01[5]=B15|B16<<8）跨越 16 字节 load 边界，
 *   用 alignr(in1,in0,15) 处理该 straddle。
 *   对 W01/W12 各做 5/3 次移位+掩码，得到 8 个 __m128i（每个放一个
 *   系数位置在 8 个组的值），再经 8×8 int16_t 转置恢复原始顺序后存储。
 * ========================================================================= */

void pack_sk_avx2(unsigned char *r, const poly *a)
{
    const __m256i bound  = _mm256_set1_epi16(DTRU_BOUND);
    /* madd 乘数 [1,8,1,8,...]：将相邻对合并为 t[2k]+8*t[2k+1] */
    const __m256i sc1    = _mm256_set1_epi32(1 | (8 << 16));
    /* mullo 乘数：将 4 个 pair-sum 移位到 [0-5],[6-11],[12-17],[18-23] 位域 */
    const __m256i sc3    = _mm256_setr_epi32(1,64,4096,262144, 1,64,4096,262144);
    /* shuf3：每个 32-bit word 提取低 3 字节到输出位置，高字节置零 */
    const __m128i shuf3  = _mm_set_epi8(-1,-1,-1,-1, 14,13,12, 10,9,8, 6,5,4, 2,1,0);

    for (int i = 0; i < DTRU_N / 64; i++) {
        __m256i v0 = _mm256_sub_epi16(bound,
            _mm256_loadu_si256((const __m256i *)(a->coeffs + 64*i     )));
        __m256i v1 = _mm256_sub_epi16(bound,
            _mm256_loadu_si256((const __m256i *)(a->coeffs + 64*i + 16)));
        __m256i v2 = _mm256_sub_epi16(bound,
            _mm256_loadu_si256((const __m256i *)(a->coeffs + 64*i + 32)));
        __m256i v3 = _mm256_sub_epi16(bound,
            _mm256_loadu_si256((const __m256i *)(a->coeffs + 64*i + 48)));

        /* COMPUTE_WORDS(v):
         * 输入：一个 128-bit lane 中的 8 个 t 值（int16_t）
         * 输出：该 lane 中所有 4 个 int32_t 都等于
         *       word = t0|(t1<<3)|...|(t7<<21)（24-bit）
         *
         * 由于各位域不重叠（p0 ≤ 36 < 64 = 2^6，乘以对应的 2^(6k) 后分布在
         * [0-5],[6-11],[12-17],[18-23]），加法即等价于 OR，不需要专门用 OR。 */
        __m256i r0 = compute_words(v0, sc1, sc3);
        __m256i r1 = compute_words(v1, sc1, sc3);
        __m256i r2 = compute_words(v2, sc1, sc3);
        __m256i r3 = compute_words(v3, sc1, sc3);

        /* 从各寄存器取出 2 个 word，重排为 [w0,w1,w2,w3] 和 [w4,w5,w6,w7]。
         * 每个 __m256i lane0 = [word,word,word,word]，lane1 类似。
         * blend(lo, hi, 0xA=0b1010) → [lo[0],hi[1],lo[2],hi[3]] = [w,w',w,w']
         * unpacklo_epi64(a,b) → [a[0:1], b[0:1]] = [w0,w1,w2,w3] ✓           */
#define EXTRACT_PAIR(rx, idx) \
    _mm_blend_epi32(_mm256_castsi256_si128(rx), _mm256_extracti128_si256(rx,1), 0xA)

        __m128i r01 = _mm_unpacklo_epi64(EXTRACT_PAIR(r0,0), EXTRACT_PAIR(r1,1));
        __m128i r23 = _mm_unpacklo_epi64(EXTRACT_PAIR(r2,2), EXTRACT_PAIR(r3,3));
#undef EXTRACT_PAIR

        /* shuf3：每个 32-bit word 提取 3 字节 → 两个 12-byte 块（后 4 字节为零） */
        __m128i a_blk = _mm_shuffle_epi8(r01, shuf3);
        __m128i b_blk = _mm_shuffle_epi8(r23, shuf3);

        /* 将 [a_blk(12B), b_blk(12B)] 拼为 24 字节输出：
         *   out0[0..11]  = a_blk[0..11]    (来自 a_blk，a_blk[12..15]=0)
         *   out0[12..15] = b_blk[0..3]     (slli(b_blk,12) 将其移至高 4 字节)
         *   out1[0..7]   = b_blk[4..11]    (srli(b_blk,4) 移至低 8 字节)      */
        __m128i out0 = _mm_or_si128(a_blk, _mm_slli_si128(b_blk, 12));
        __m128i out1 = _mm_srli_si128(b_blk, 4);

        _mm_storeu_si128((__m128i *)(r + 24*i     ), out0);
        _mm_storel_epi64((__m128i *)(r + 24*i + 16), out1);
    }
}

void unpack_sk_avx2(poly *r, const unsigned char *a)
{
    /* pshufb 掩码：从两次 load 的 24 字节中拼出 W01 和 W12。
     *
     * 8 个组，每组 k 占输入字节 {3k, 3k+1, 3k+2}。
     * W01[k] = a[3k]   | (a[3k+1] << 8)  — 组 5 跨越 16 字节边界（straddle）
     * W12[k] = a[3k+1] | (a[3k+2] << 8)  — 组 5-7 来自 in1，无跨边界
     *
     * in0 = bytes 0..15, in1 = bytes 16..23（零扩展至 128-bit）
     * straddle = alignr(in1, in0, 15) = [B15, B16, B17, ...]
     *
     * W01 输出字节位置：
     *   0,1  ← in0[0,1]     组 0
     *   2,3  ← in0[3,4]     组 1
     *   4,5  ← in0[6,7]     组 2
     *   6,7  ← in0[9,10]    组 3
     *   8,9  ← in0[12,13]   组 4
     *   10,11← straddle[0,1] 组 5 (B15, B16)
     *   12,13← in1[2,3]     组 6
     *   14,15← in1[5,6]     组 7
     *
     * W12 输出字节位置：
     *   0,1  ← in0[1,2]     组 0
     *   2,3  ← in0[4,5]     组 1
     *   4,5  ← in0[7,8]     组 2
     *   6,7  ← in0[10,11]   组 3
     *   8,9  ← in0[13,14]   组 4
     *   10,11← in1[0,1]     组 5
     *   12,13← in1[3,4]     组 6
     *   14,15← in1[6,7]     组 7                                              */

    /* W01：组 0-4 来自 in0 */
    static const int8_t shuf_w01_lo_arr[16] = {0,1,3,4,6,7,9,10,12,13,-1,-1,-1,-1,-1,-1};
    /* W01：组 5 straddle，来自 alignr 寄存器的字节 0,1 放到位置 10,11 */
    static const int8_t shuf_w01_str_arr[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,-1,-1,-1,-1};
    /* W01：组 6-7 来自 in1 */
    static const int8_t shuf_w01_hi_arr[16]  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,3,5,6};
    /* W12：组 0-4 来自 in0 */
    static const int8_t shuf_w12_lo_arr[16] = {1,2,4,5,7,8,10,11,13,14,-1,-1,-1,-1,-1,-1};
    /* W12：组 5-7 来自 in1（无 straddle） */
    static const int8_t shuf_w12_hi_arr[16]  = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,3,4,6,7};

    const __m128i shuf_w01_lo  = _mm_loadu_si128((const __m128i *)shuf_w01_lo_arr);
    const __m128i shuf_w01_str = _mm_loadu_si128((const __m128i *)shuf_w01_str_arr);
    const __m128i shuf_w01_hi  = _mm_loadu_si128((const __m128i *)shuf_w01_hi_arr);
    const __m128i shuf_w12_lo  = _mm_loadu_si128((const __m128i *)shuf_w12_lo_arr);
    const __m128i shuf_w12_hi  = _mm_loadu_si128((const __m128i *)shuf_w12_hi_arr);

    const __m128i mask7  = _mm_set1_epi16(7);
    const __m128i bound  = _mm_set1_epi16(DTRU_BOUND);

    for (int i = 0; i < DTRU_N / 64; i++) {
        /* 加载 24 字节 = 8 组 × 3 字节 */
        __m128i in0 = _mm_loadu_si128((const __m128i *)(a + 24*i));
        /* _mm_loadl_epi64: 只加载低 64 位（8 字节），高 64 位置零 */
        __m128i in1 = _mm_loadl_epi64((const __m128i *)(a + 24*i + 16));

        /* alignr(in1, in0, 15) = bytes [15..30] of [in1:in0]
         * = [in0[15], in1[0..14]] = [B15, B16, ...]                          */
        __m128i straddle = _mm_alignr_epi8(in1, in0, 15);

        /* 构造 W01（8 个 16-bit 值） */
        __m128i W01 = _mm_or_si128(
            _mm_shuffle_epi8(in0,      shuf_w01_lo),
            _mm_or_si128(
                _mm_shuffle_epi8(straddle, shuf_w01_str),
                _mm_shuffle_epi8(in1,      shuf_w01_hi)
            )
        );

        /* 构造 W12（组 5-7 无 straddle，全部来自 in0/in1） */
        __m128i W12 = _mm_or_si128(
            _mm_shuffle_epi8(in0, shuf_w12_lo),
            _mm_shuffle_epi8(in1, shuf_w12_hi)
        );

        /* 从 W01 提取系数位置 0-4（每组 8 个值 → 8 个 int16_t per 向量）
         * 从 W12 提取系数位置 5-7
         *
         * W01[k] = a[3k] | (a[3k+1]<<8)：
         *   c0 = (W01>>0)&7  c1 = (W01>>3)&7  c2 = (W01>>6)&7
         *   c3 = (W01>>9)&7  c4 = (W01>>12)&7
         * W12[k] = a[3k+1] | (a[3k+2]<<8)：
         *   c5 = (W12>>7)&7  c6 = (W12>>10)&7  c7 = (W12>>13)&7            */
        __m128i v0 = _mm_and_si128(W01,                      mask7);
        __m128i v1 = _mm_and_si128(_mm_srli_epi16(W01,  3),  mask7);
        __m128i v2 = _mm_and_si128(_mm_srli_epi16(W01,  6),  mask7);
        __m128i v3 = _mm_and_si128(_mm_srli_epi16(W01,  9),  mask7);
        __m128i v4 = _mm_and_si128(_mm_srli_epi16(W01, 12),  mask7);
        __m128i v5 = _mm_and_si128(_mm_srli_epi16(W12,  7),  mask7);
        __m128i v6 = _mm_and_si128(_mm_srli_epi16(W12, 10),  mask7);
        __m128i v7 = _mm_and_si128(_mm_srli_epi16(W12, 13),  mask7);

        /* 应用 BOUND - t */
        v0 = _mm_sub_epi16(bound, v0);
        v1 = _mm_sub_epi16(bound, v1);
        v2 = _mm_sub_epi16(bound, v2);
        v3 = _mm_sub_epi16(bound, v3);
        v4 = _mm_sub_epi16(bound, v4);
        v5 = _mm_sub_epi16(bound, v5);
        v6 = _mm_sub_epi16(bound, v6);
        v7 = _mm_sub_epi16(bound, v7);

        /* 8×8 int16_t 转置：将"每个系数位置对应 8 个组"的布局
         * 转换为"每个组对应 8 个系数"的布局，然后顺序写入输出。
         *
         *  输入：v0..v7 各有 8 个 int16_t
         *    v_c[k] = [c_g0, c_g1, ..., c_g7]（c 位置的系数在各组的值）
         *  输出：t0..t7 各有 8 个 int16_t
         *    t_g[i] = [c0,c1,...,c7] 对应组 g 的所有系数                    */

        /* Step1: 16-bit 交织 */
        __m128i a0 = _mm_unpacklo_epi16(v0, v1); /* [c0g0,c1g0, c0g1,c1g1, ...g3] */
        __m128i a1 = _mm_unpackhi_epi16(v0, v1); /* [c0g4,c1g4, ...g7] */
        __m128i a2 = _mm_unpacklo_epi16(v2, v3);
        __m128i a3 = _mm_unpackhi_epi16(v2, v3);
        __m128i a4 = _mm_unpacklo_epi16(v4, v5);
        __m128i a5 = _mm_unpackhi_epi16(v4, v5);
        __m128i a6 = _mm_unpacklo_epi16(v6, v7);
        __m128i a7 = _mm_unpackhi_epi16(v6, v7);

        /* Step2: 32-bit 交织 */
        __m128i b0 = _mm_unpacklo_epi32(a0, a2); /* [c0-c3 for g0, c0-c3 for g1] */
        __m128i b1 = _mm_unpackhi_epi32(a0, a2);
        __m128i b2 = _mm_unpacklo_epi32(a1, a3);
        __m128i b3 = _mm_unpackhi_epi32(a1, a3);
        __m128i b4 = _mm_unpacklo_epi32(a4, a6); /* [c4-c7 for g0, c4-c7 for g1] */
        __m128i b5 = _mm_unpackhi_epi32(a4, a6);
        __m128i b6 = _mm_unpacklo_epi32(a5, a7);
        __m128i b7 = _mm_unpackhi_epi32(a5, a7);

        /* Step3: 64-bit 交织 → 最终得到每个组的完整 8 系数向量 */
        __m128i t0 = _mm_unpacklo_epi64(b0, b4); /* [c0..c7 for g0] */
        __m128i t1 = _mm_unpackhi_epi64(b0, b4); /* [c0..c7 for g1] */
        __m128i t2 = _mm_unpacklo_epi64(b1, b5);
        __m128i t3 = _mm_unpackhi_epi64(b1, b5);
        __m128i t4 = _mm_unpacklo_epi64(b2, b6);
        __m128i t5 = _mm_unpackhi_epi64(b2, b6);
        __m128i t6 = _mm_unpacklo_epi64(b3, b7);
        __m128i t7 = _mm_unpackhi_epi64(b3, b7);

        /* 写入输出（每个 __m128i = 8 个 int16_t = 一个组的所有系数） */
        int16_t *base = r->coeffs + 64*i;
        _mm_storeu_si128((__m128i *)(base     ), t0);
        _mm_storeu_si128((__m128i *)(base +  8), t1);
        _mm_storeu_si128((__m128i *)(base + 16), t2);
        _mm_storeu_si128((__m128i *)(base + 24), t3);
        _mm_storeu_si128((__m128i *)(base + 32), t4);
        _mm_storeu_si128((__m128i *)(base + 40), t5);
        _mm_storeu_si128((__m128i *)(base + 48), t6);
        _mm_storeu_si128((__m128i *)(base + 56), t7);
    }
}

/* =========================================================================
 * pack_pk_avx2 / unpack_pk_avx2
 *
 * 12-bit 打包，PK_PACK_OPT=0，输出 DTRU_N*12/8 = 3072 字节。
 * 算法与 DTRU-tern-1024 版本完全相同，仅循环次数不同（N/32=64 次）。
 *
 * 每次处理 32 个系数 → 48 字节（3×16B），64 次迭代。
 * 核心：combined[j] = a[2j] | (a[2j+1]<<12)（相邻系数对打包为 24-bit word）
 *       shuf3 从每个 32-bit word 提取低 3 字节，
 *       4 个 12-byte 块用移位+OR 拼成 3 个对齐的 16-byte 输出。
 * ========================================================================= */
void pack_pk_avx2(unsigned char *r, const poly *a)
{
    const __m128i shuf3 = _mm_set_epi8(
        -1,-1,-1,-1,   /* 位置 15-12: 0 */
        14,13,12,      /* word 3 */
        10, 9, 8,      /* word 2 */
         6, 5, 4,      /* word 1 */
         2, 1, 0       /* word 0 */
    );

    for (int i = 0; i < DTRU_N / 32; i++) {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i     ));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(a->coeffs + 32*i + 16));

        __m256i even0 = _mm256_and_si256(v0, _mm256_set1_epi32(0x0000FFFF));
        __m256i odd0  = _mm256_srli_epi32(v0, 16);
        __m256i c0    = _mm256_or_si256(even0, _mm256_slli_epi32(odd0, 12));

        __m256i even1 = _mm256_and_si256(v1, _mm256_set1_epi32(0x0000FFFF));
        __m256i odd1  = _mm256_srli_epi32(v1, 16);
        __m256i c1    = _mm256_or_si256(even1, _mm256_slli_epi32(odd1, 12));

        __m128i a_blk = _mm_shuffle_epi8(_mm256_castsi256_si128(c0),     shuf3);
        __m128i b_blk = _mm_shuffle_epi8(_mm256_extracti128_si256(c0,1), shuf3);
        __m128i c_blk = _mm_shuffle_epi8(_mm256_castsi256_si128(c1),     shuf3);
        __m128i d_blk = _mm_shuffle_epi8(_mm256_extracti128_si256(c1,1), shuf3);

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
    const __m128i shuf3_inv = _mm_set_epi8(
        -1,11,10, 9,
        -1, 8, 7, 6,
        -1, 5, 4, 3,
        -1, 2, 1, 0
    );

    const __m128i mask12 = _mm_set_epi32(     0, -1, -1, -1);
    const __m128i mask8  = _mm_set_epi64x(    0,          -1);
    const __m128i mask4  = _mm_set_epi32( 0,  0,  0,     -1);

    const __m256i mask12bit = _mm256_set1_epi32(0x00000FFF);

    for (int i = 0; i < DTRU_N / 32; i++) {
        __m128i in0 = _mm_loadu_si128((const __m128i *)(a + 48*i     ));
        __m128i in1 = _mm_loadu_si128((const __m128i *)(a + 48*i + 16));
        __m128i in2 = _mm_loadu_si128((const __m128i *)(a + 48*i + 32));

        __m128i a_blk = _mm_and_si128(in0, mask12);
        __m128i b_blk = _mm_or_si128(
            _mm_srli_si128(in0, 12),
            _mm_slli_si128(_mm_and_si128(in1, mask8), 4)
        );
        __m128i c_blk = _mm_or_si128(
            _mm_srli_si128(in1, 8),
            _mm_slli_si128(_mm_and_si128(in2, mask4), 8)
        );
        __m128i d_blk = _mm_srli_si128(in2, 4);

        a_blk = _mm_shuffle_epi8(a_blk, shuf3_inv);
        b_blk = _mm_shuffle_epi8(b_blk, shuf3_inv);
        c_blk = _mm_shuffle_epi8(c_blk, shuf3_inv);
        d_blk = _mm_shuffle_epi8(d_blk, shuf3_inv);

        __m256i ab = _mm256_set_m128i(b_blk, a_blk);
        __m256i cd = _mm256_set_m128i(d_blk, c_blk);

        __m256i even_ab = _mm256_and_si256(ab, mask12bit);
        __m256i odd_ab  = _mm256_and_si256(_mm256_srli_epi32(ab, 12), mask12bit);
        __m256i even_cd = _mm256_and_si256(cd, mask12bit);
        __m256i odd_cd  = _mm256_and_si256(_mm256_srli_epi32(cd, 12), mask12bit);

        __m256i lo_ab = _mm256_unpacklo_epi32(even_ab, odd_ab);
        __m256i hi_ab = _mm256_unpackhi_epi32(even_ab, odd_ab);
        __m256i lo_cd = _mm256_unpacklo_epi32(even_cd, odd_cd);
        __m256i hi_cd = _mm256_unpackhi_epi32(even_cd, odd_cd);

        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i     ),
                            _mm256_packus_epi32(lo_ab, hi_ab));
        _mm256_storeu_si256((__m256i *)(r->coeffs + 32*i + 16),
                            _mm256_packus_epi32(lo_cd, hi_cd));
    }
}
