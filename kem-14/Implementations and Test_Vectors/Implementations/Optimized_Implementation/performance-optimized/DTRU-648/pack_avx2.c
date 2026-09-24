#include <immintrin.h>
#include <stdint.h>
#include "pack.h"
#include "params.h"

/*
 *   pack_sk/unpack_sk:
 *     一个字节保存两个系数，格式为
 *       (DTRU_BOUND - coeff[2*i])
 *       | ((DTRU_BOUND - coeff[2*i + 1]) << 4).
 *
 *   pack_pk_avx2/unpack_pk_avx2:
 *     三个字节保存两个 12-bit 系数。
 *
 * PK_PACK_OPT=0 时，pack_pk_avx2/unpack_pk_avx2 可用于 972-byte
 * 12-bit 公钥实现；PK_PACK_OPT=1 的 954-byte 递归压缩格式不调用这里的
 * PK AVX2 函数。
 * pack_ct/unpack_decompress_ct 继续使用 pack.c 中的标量实现。
 */

/* 能够由完整 AVX2 处理块覆盖的最大系数数量。 */
#define PACK_BULK_N (DTRU_N & ~31)
#define PK_BULK_N (DTRU_N & ~15)

void pack_sk_avx2(unsigned char *r, const poly *a)
{
    const __m256i bound = _mm256_set1_epi16(DTRU_BOUND);
    const __m256i lomask = _mm256_set1_epi16(0x00FF);

    for (int i = 0; i < PACK_BULK_N; i += 32)
    {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(a->coeffs + i + 16));
        __m256i packed;
        __m256i lo;
        __m256i hi;
        __m256i nib;
        __m256i res;

        /*
         * 对应 pack.c 中的 t[k] = DTRU_BOUND - coeff[k]。
         * 合法 SK 输入得到的 t[k] 范围为 [0, 10]，因此将 int16_t 饱和转换
         * 为字节时不会改变任何值。
         */
        v0 = _mm256_sub_epi16(bound, v0);
        v1 = _mm256_sub_epi16(bound, v1);

        /* 修正 lane 内部打包顺序，恢复连续的 t[0..31] 字节顺序。 */
        packed = _mm256_permute4x64_epi64(_mm256_packs_epi16(v0, v1), 0xD8);

        /*
         * 将连续字节按 16-bit word 查看时，t[2*k] 位于低字节，
         * t[2*k+1] 位于高字节。下面的计算与标量输出字节
         * t[2*k] | (t[2*k+1] << 4) 完全一致。
         */
        lo = _mm256_and_si256(packed, lomask);
        hi = _mm256_srli_epi16(packed, 8);
        nib = _mm256_or_si256(lo, _mm256_slli_epi16(hi, 4));
        res = _mm256_permute4x64_epi64(
            _mm256_packus_epi16(nib, _mm256_setzero_si256()), 0xD8);
        _mm_storeu_si128((__m128i *)(r + i / 2), _mm256_castsi256_si128(res));
    }

    /*
     * DTRU_N=648，PACK_BULK_N=640。最后 8 个系数使用与 pack.c 中
     * pack_sk 相同的双系数公式处理，不会读取 coeffs[647] 之后的数据。
     */
    for (int i = PACK_BULK_N; i < DTRU_N; i += 2)
        r[i / 2] = (uint8_t)(DTRU_BOUND - a->coeffs[i])
                 | (uint8_t)((DTRU_BOUND - a->coeffs[i + 1]) << 4);
}

void unpack_sk_avx2(poly *r, const unsigned char *a)
{
    const __m256i bound = _mm256_set1_epi16(DTRU_BOUND);
    const __m256i nibmask = _mm256_set1_epi16(0x0F);

    for (int i = 0; i < PACK_BULK_N; i += 32)
    {
        __m256i v = _mm256_cvtepu8_epi16(
            _mm_loadu_si128((const __m128i *)(a + i / 2)));

        /*
         * 对应 pack.c 中的 unpack_sk：从每个字节提取低、高 nibble，
         * 然后按 coeff = DTRU_BOUND - nibble 恢复系数。
         */
        __m256i lo = _mm256_and_si256(v, nibmask);
        __m256i hi = _mm256_srli_epi16(v, 4);
        __m256i ilo = _mm256_unpacklo_epi16(lo, hi);
        __m256i ihi = _mm256_unpackhi_epi16(lo, hi);

        /* 将 lane 内交错排列的数据重组为连续系数顺序。 */
        __m256i r0 = _mm256_permute2x128_si256(ilo, ihi, 0x20);
        __m256i r1 = _mm256_permute2x128_si256(ilo, ihi, 0x31);

        _mm256_storeu_si256((__m256i *)(r->coeffs + i),
                            _mm256_sub_epi16(bound, r0));
        _mm256_storeu_si256((__m256i *)(r->coeffs + i + 16),
                            _mm256_sub_epi16(bound, r1));
    }

    /* 系数 640..647 使用与标量 unpack_sk 完全相同的公式。 */
    for (int i = PACK_BULK_N; i < DTRU_N; i += 2)
    {
        r->coeffs[i] = DTRU_BOUND - (a[i / 2] & 0xF);
        r->coeffs[i + 1] = DTRU_BOUND - (a[i / 2] >> 4);
    }
}

/*
 * 以下函数实现唯一支持的 972-byte 12-bit PK 线格式。
 *
 * 对每一对系数，pack.c 输出：
 *   字节 0 = even[7:0]
 *   字节 1 = even[11:8] | odd[3:0] << 4
 *   字节 2 = odd[11:4]
 *
 * 上述三个字节正是 24-bit 值 even | (odd << 12) 的小端表示。
 * AVX2 实现每轮将 16 个系数转换为 8 个 24-bit 值。由于 DTRU_N 不是
 * 16 个系数处理块大小的整数倍，最后 8 个系数单独处理。
 */
static void pack_pk_8(unsigned char *r, const int16_t *a)
{
    const __m128i low16 = _mm_set1_epi32(0xFFFF);
    const __m128i shuffle = _mm_setr_epi8(
        0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);
    __m128i v = _mm_loadu_si128((const __m128i *)a);
    __m128i even = _mm_and_si128(v, low16);
    __m128i odd = _mm_srli_epi32(v, 16);
    __m128i combined = _mm_or_si128(even, _mm_slli_epi32(odd, 12));
    __m128i packed = _mm_shuffle_epi8(combined, shuffle);

    /*
     * 写 16 字节会临时覆盖后续块的前 4 字节；后续向量块或标量尾部会
     * 按 12-bit 格式写回最终值。DTRU_N=648 时不会越过 972-byte PK。
     */
    _mm_storeu_si128((__m128i *)r, packed);
}

void pack_pk_avx2(unsigned char *r, const poly *a)
{
    for (int i = 0; i < PK_BULK_N; i += 16)
    {
        pack_pk_8(r + 3 * (i / 2), a->coeffs + i);
        pack_pk_8(r + 3 * (i / 2) + 12, a->coeffs + i + 8);
    }

    /* 系数 640..647 使用与标量 pack_pk 完全相同的公式。 */
    for (int i = PK_BULK_N; i < DTRU_N; i += 2)
    {
        int j = 3 * (i / 2);
        r[j] = (unsigned char)a->coeffs[i];
        r[j + 1] = (unsigned char)((a->coeffs[i] >> 8) | (a->coeffs[i + 1] << 4));
        r[j + 2] = (unsigned char)(a->coeffs[i + 1] >> 4);
    }
}

static void unpack_pk_8(int16_t *r, const unsigned char *a)
{
    const __m128i expand = _mm_setr_epi8(
        0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
    const __m128i mask12 = _mm_set1_epi32(0xFFF);
    __m128i bytes = _mm_loadu_si128((const __m128i *)a);
    __m128i words = _mm_shuffle_epi8(bytes, expand);
    __m128i even = _mm_and_si128(words, mask12);
    __m128i odd = _mm_and_si128(_mm_srli_epi32(words, 12), mask12);
    __m128i lo = _mm_unpacklo_epi32(even, odd);
    __m128i hi = _mm_unpackhi_epi32(even, odd);
    __m128i coeffs = _mm_packus_epi32(lo, hi);

    _mm_storeu_si128((__m128i *)r, coeffs);
}

void unpack_pk_avx2(poly *r, const unsigned char *a)
{
    for (int i = 0; i < PK_BULK_N; i += 16)
    {
        unpack_pk_8(r->coeffs + i, a + 3 * (i / 2));
        unpack_pk_8(r->coeffs + i + 8, a + 3 * (i / 2) + 12);
    }

    /* 系数 640..647 使用与标量 unpack_pk 完全相同的公式。 */
    for (int i = PK_BULK_N; i < DTRU_N; i += 2)
    {
        int j = 3 * (i / 2);
        r->coeffs[i] = (a[j] | ((uint16_t)a[j + 1] << 8)) & 0xFFF;
        r->coeffs[i + 1] = ((a[j + 1] >> 4) | ((uint16_t)a[j + 2] << 4)) & 0xFFF;
    }
}
