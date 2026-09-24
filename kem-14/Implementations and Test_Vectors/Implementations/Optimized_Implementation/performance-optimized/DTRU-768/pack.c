#include "pack.h"
#include "params.h"

#if PK_PACK_OPT

    #include "inverse.h"

    uint16_t uint32_mod_uint14(uint32_t x, uint16_t m)
    {
        uint32_t q;
        uint16_t r;
        uint32_divmod_uint14(&q, &r, x, m);
        return r;
    }

    static inline __m128i pack_low_u16_from_u32(__m256i x)
    {
        x = _mm256_and_si256(x, _mm256_set1_epi32(0x0000FFFF));
        __m128i lo = _mm256_castsi256_si128(x);
        __m128i hi = _mm256_extracti128_si256(x, 1);
        return _mm_packus_epi32(lo, hi);
    }

    static inline __m128i pack_low_u8_from_u16(__m128i x)
    {
        x = _mm_and_si128(x, _mm_set1_epi16(0x00FF));
        return _mm_packus_epi16(x, _mm_setzero_si128());
    }

    void Encode(unsigned char *out, const unsigned char *out_end, const uint16_t *R, const uint16_t *M, const long long len)
    {
        if (len == 1)
        {
            uint16_t r = R[0];
            uint16_t m = M[0];
            while (m > 1)
            {
                if (out >= out_end) return;
                *out++ = (unsigned char)r;
                r >>= 8;
                m = (m + 255) >> 8;
            }
        }

        if (len > 1)
        {
            uint16_t R2[(len + 1) / 2];
            uint16_t M2[(len + 1) / 2];
            long long i = 0;

            // 1：提取当前层级的统一常数，计算 while 循环在所有通道中的确切执行次数 k
            uint32_t m0_val = M[0];
            uint32_t m_check = m0_val * m0_val;
            int k = 0;
            while (m_check >= 1024) {
                k++;
                m_check = (m_check + 255) >> 8;
            }

            __m256i vec_m0 = _mm256_set1_epi32(m0_val);
            __m128i vec_m_next = _mm_set1_epi16((uint16_t)m_check);

            // 2：根据 k 的固有次数，直接进行全向量展开，减少存储转发停顿
            if (k == 0) {
                for (; i + 15 < len - 1; i += 16) {
                    __m256i r_all = _mm256_loadu_si256((const __m256i *)&R[i]);
                    __m256i vec_R_even = _mm256_and_si256(r_all, _mm256_set1_epi32(0x0000FFFF));
                    __m256i vec_R_odd  = _mm256_srli_epi32(r_all, 16);
                    __m256i vec_r = _mm256_add_epi32(vec_R_even, _mm256_mullo_epi32(vec_R_odd, vec_m0));
                    
                    __m128i r_u16 = pack_low_u16_from_u32(vec_r);
                    
                    long long idx = i / 2;
                    _mm_storeu_si128((__m128i *)&R2[idx], r_u16);
                    _mm_storeu_si128((__m128i *)&M2[idx], vec_m_next);
                }
            }
            else if (k == 1) {
                for (; i + 15 < len - 1; i += 16) {
                    if (out + 8 > out_end) return;
                    __m256i r_all = _mm256_loadu_si256((const __m256i *)&R[i]);
                    __m256i vec_R_even = _mm256_and_si256(r_all, _mm256_set1_epi32(0x0000FFFF));
                    __m256i vec_R_odd  = _mm256_srli_epi32(r_all, 16);
                    __m256i vec_r = _mm256_add_epi32(vec_R_even, _mm256_mullo_epi32(vec_R_odd, vec_m0));
                    
                    __m128i r_u16 = pack_low_u16_from_u32(vec_r);
                    
                    __m128i r_u8 = pack_low_u8_from_u16(r_u16);
                    _mm_storel_epi64((__m128i *)out, r_u8);
                    out += 8;
                    
                    vec_r = _mm256_srli_epi32(vec_r, 8);
                    r_u16 = pack_low_u16_from_u32(vec_r);
                    
                    long long idx = i / 2;
                    _mm_storeu_si128((__m128i *)&R2[idx], r_u16);
                    _mm_storeu_si128((__m128i *)&M2[idx], vec_m_next);
                }
            }
            else if (k == 2) {
                for (; i + 15 < len - 1; i += 16) {
                    if (out + 16 > out_end) return;
                    __m256i r_all = _mm256_loadu_si256((const __m256i *)&R[i]);
                    __m256i vec_R_even = _mm256_and_si256(r_all, _mm256_set1_epi32(0x0000FFFF));
                    __m256i vec_R_odd  = _mm256_srli_epi32(r_all, 16);
                    __m256i vec_r = _mm256_add_epi32(vec_R_even, _mm256_mullo_epi32(vec_R_odd, vec_m0));
                    
                    __m128i r_u16 = pack_low_u16_from_u32(vec_r);
                    
                    // k=2 时，Little Endian 的 uint16_t 数组刚好契合两个连续字节的排布要求
                    _mm_storeu_si128((__m128i *)out, r_u16);
                    out += 16;
                    
                    vec_r = _mm256_srli_epi32(vec_r, 16);
                    r_u16 = pack_low_u16_from_u32(vec_r);
                    
                    long long idx = i / 2;
                    _mm_storeu_si128((__m128i *)&R2[idx], r_u16);
                    _mm_storeu_si128((__m128i *)&M2[idx], vec_m_next);
                }
            }

            // 标量余数尾部处理
            for (; i < len - 1; i += 2)
            {
                uint32_t m0 = M[i];
                uint32_t r = R[i] + R[i + 1] * m0;
                uint32_t m = M[i + 1] * m0;

                while (m >= 1024)
                {
                    if (out >= out_end) return;
                    *out++ = (unsigned char)r;
                    r >>= 8;
                    m = (m + 255) >> 8;
                }
                R2[i / 2] = r;
                M2[i / 2] = m;
            }

            if (i < len)
            {
                R2[i / 2] = R[i];
                M2[i / 2] = M[i];
            }

            Encode(out, out_end, R2, M2, (len + 1) / 2);
        }
    }

    void Decode(uint16_t *out, const unsigned char *S, const uint16_t *M, const long long len)
    {
        if (len == 1)
        {
            if (M[0] == 1)
                *out = 0;
            else if (M[0] <= 256)
                *out = uint32_mod_uint14(S[0], M[0]);
            else
                *out = uint32_mod_uint14(S[0] + (((uint16_t)S[1]) << 8), M[0]);
        }

        if (len > 1)
        {
            uint16_t R2[(len + 1) / 2];
            uint16_t M2[(len + 1) / 2];
            uint16_t bottomr[len / 2];
            uint32_t bottomt[len / 2];
            long long pairs = (len - 1) / 2;
            long long p = 0;

            // 3：由于 m 的全局同一性，直接将 if-else 条件分支从循环内部分离剥离到最外层
            uint32_t m_val = M[0] * (uint32_t)M[0];

            if (m_val > 256 * 1023)
            {
                uint32_t b_t = 256 * 256;
                uint16_t m2_v = (((m_val + 255) >> 8) + 255) >> 8;
                __m256i vec_bt = _mm256_set1_epi32(b_t);
                __m128i vec_m2 = _mm_set1_epi16(m2_v);

                for (; p + 7 < pairs; p += 8)
                {
                    _mm256_storeu_si256((__m256i *)&bottomt[p], vec_bt);
                    _mm_storeu_si128((__m128i *)&M2[p], vec_m2);
                    __m128i s_bytes = _mm_loadu_si128((const __m128i *)S);
                    _mm_storeu_si128((__m128i *)&bottomr[p], s_bytes);
                    S += 16;
                }
            }
            else if (m_val >= 1024)
            {
                uint32_t b_t = 256;
                uint16_t m2_v = (m_val + 255) >> 8;
                __m256i vec_bt = _mm256_set1_epi32(b_t);
                __m128i vec_m2 = _mm_set1_epi16(m2_v);

                for (; p + 7 < pairs; p += 8)
                {
                    _mm256_storeu_si256((__m256i *)&bottomt[p], vec_bt);
                    _mm_storeu_si128((__m128i *)&M2[p], vec_m2);
                    __m128i s_bytes = _mm_loadl_epi64((const __m128i *)S);
                    __m128i s_u16 = _mm_cvtepu8_epi16(s_bytes);
                    _mm_storeu_si128((__m128i *)&bottomr[p], s_u16);
                    S += 8;
                }
            }
            else
            {
                __m256i vec_bt = _mm256_set1_epi32(1);
                __m128i vec_m2 = _mm_set1_epi16((uint16_t)m_val);
                __m128i vec_br = _mm_setzero_si128();

                for (; p + 7 < pairs; p += 8)
                {
                    _mm256_storeu_si256((__m256i *)&bottomt[p], vec_bt);
                    _mm_storeu_si128((__m128i *)&M2[p], vec_m2);
                    _mm_storeu_si128((__m128i *)&bottomr[p], vec_br);
                }
            }

            // 第一步的第一部分标量余数尾部
            long long i = p * 2;
            for (; i < len - 1; i += 2)
            {
                uint32_t m = M[i] * (uint32_t)M[i + 1];
                if (m > 256 * 1023)
                {
                    bottomt[i / 2] = 256 * 256;
                    bottomr[i / 2] = S[0] + 256 * S[1];
                    S += 2;
                    M2[i / 2] = (((m + 255) >> 8) + 255) >> 8;
                }
                else if (m >= 1024)
                {
                    bottomt[i / 2] = 256;
                    bottomr[i / 2] = S[0];
                    S += 1;
                    M2[i / 2] = (m + 255) >> 8;
                }
                else
                {
                    bottomt[i / 2] = 1;
                    bottomr[i / 2] = 0;
                    M2[i / 2] = m;
                }
            }
            if (i < len)
                M2[i / 2] = M[i];

            Decode(R2, S, M2, (len + 1) / 2);

            // 4：完全重写并并行化系统的 Barrett 乘法约减除法器
            p = 0;
            uint32_t mi = M[0];
            uint32_t mi1 = M[0];
            uint32_t w_i = 0x80000000 / mi;
            uint32_t w_i1 = 0x80000000 / mi1;
            
            __m256i vec_mi = _mm256_set1_epi32(mi);
            __m256i vec_mi1 = _mm256_set1_epi32(mi1);
            __m256i vec_wi = _mm256_set1_epi32(w_i);
            __m256i vec_wi1 = _mm256_set1_epi32(w_i1);
            __m256i mask_32 = _mm256_set1_epi64x(0x00000000FFFFFFFFULL);
            __m256i vec_one = _mm256_set1_epi32(1);
            __m256i vec_zero = _mm256_setzero_si256();

            for (; p + 7 < pairs; p += 8)
            {
                __m128i m128_bottomr = _mm_loadu_si128((const __m128i *)&bottomr[p]);
                __m256i vec_r = _mm256_cvtepu16_epi32(m128_bottomr);
                __m256i vec_bottomt = _mm256_loadu_si256((const __m256i *)&bottomt[p]);
                __m128i m128_R2 = _mm_loadu_si128((const __m128i *)&R2[p]);
                __m256i vec_R2 = _mm256_cvtepu16_epi32(m128_R2);
                
                // r += bottomt * R2
                vec_r = _mm256_add_epi32(vec_r, _mm256_mullo_epi32(vec_bottomt, vec_R2));
                
                // -------------------- 纯向量并行执行第 1 次 divmod --------------------
                __m256i vec_r1 = _mm256_setzero_si256();
                
                // 模拟 qpart = (x * (uint64_t)w) >> 31 并合并高低32位
                __m256i prod_even = _mm256_mul_epu32(vec_r, vec_wi);
                __m256i prod_odd  = _mm256_mul_epu32(_mm256_srli_epi64(vec_r, 32), vec_wi);
                __m256i q_even = _mm256_srli_epi64(prod_even, 31);
                __m256i q_odd  = _mm256_srli_epi64(prod_odd, 31);
                __m256i vec_qpart = _mm256_or_si256(_mm256_and_si256(q_even, mask_32), _mm256_slli_epi64(q_odd, 32));
                vec_r  = _mm256_sub_epi32(vec_r, _mm256_mullo_epi32(vec_qpart, vec_mi));
                vec_r1 = _mm256_add_epi32(vec_r1, vec_qpart);
                
                prod_even = _mm256_mul_epu32(vec_r, vec_wi);
                prod_odd  = _mm256_mul_epu32(_mm256_srli_epi64(vec_r, 32), vec_wi);
                q_even = _mm256_srli_epi64(prod_even, 31);
                q_odd  = _mm256_srli_epi64(prod_odd, 31);
                vec_qpart = _mm256_or_si256(_mm256_and_si256(q_even, mask_32), _mm256_slli_epi64(q_odd, 32));
                vec_r  = _mm256_sub_epi32(vec_r, _mm256_mullo_epi32(vec_qpart, vec_mi));
                vec_r1 = _mm256_add_epi32(vec_r1, vec_qpart);
                
                // 无分支向量化修正逻辑 mask = -(x >> 31);
                vec_r  = _mm256_sub_epi32(vec_r, vec_mi);
                vec_r1 = _mm256_add_epi32(vec_r1, vec_one);
                __m256i vec_mask = _mm256_cmpgt_epi32(vec_zero, vec_r);
                vec_r  = _mm256_add_epi32(vec_r, _mm256_and_si256(vec_mask, vec_mi));
                vec_r1 = _mm256_add_epi32(vec_r1, vec_mask);
                
                __m256i vec_r0 = vec_r;
                
                // -------------------- 纯向量并行执行第 2 次 mod --------------------
                prod_even = _mm256_mul_epu32(vec_r1, vec_wi1);
                prod_odd  = _mm256_mul_epu32(_mm256_srli_epi64(vec_r1, 32), vec_wi1);
                q_even = _mm256_srli_epi64(prod_even, 31);
                q_odd  = _mm256_srli_epi64(prod_odd, 31);
                vec_qpart = _mm256_or_si256(_mm256_and_si256(q_even, mask_32), _mm256_slli_epi64(q_odd, 32));
                vec_r1 = _mm256_sub_epi32(vec_r1, _mm256_mullo_epi32(vec_qpart, vec_mi1));
                
                prod_even = _mm256_mul_epu32(vec_r1, vec_wi1);
                prod_odd  = _mm256_mul_epu32(_mm256_srli_epi64(vec_r1, 32), vec_wi1);
                q_even = _mm256_srli_epi64(prod_even, 31);
                q_odd  = _mm256_srli_epi64(prod_odd, 31);
                vec_qpart = _mm256_or_si256(_mm256_and_si256(q_even, mask_32), _mm256_slli_epi64(q_odd, 32));
                vec_r1 = _mm256_sub_epi32(vec_r1, _mm256_mullo_epi32(vec_qpart, vec_mi1));
                
                vec_r1 = _mm256_sub_epi32(vec_r1, vec_mi1);
                vec_mask = _mm256_cmpgt_epi32(vec_zero, vec_r1);
                vec_r1 = _mm256_add_epi32(vec_r1, _mm256_and_si256(vec_mask, vec_mi1));
                
                // -------------------- 高低交叉重组，完美序列写入 --------------------
                __m256i vec_interleave_lo = _mm256_unpacklo_epi32(vec_r0, vec_r1); 
                __m256i vec_interleave_hi = _mm256_unpackhi_epi32(vec_r0, vec_r1); 
                
                __m128i block0 = _mm256_castsi256_si128(vec_interleave_lo); 
                __m128i block1 = _mm256_castsi256_si128(vec_interleave_hi); 
                __m128i block2 = _mm256_extracti128_si256(vec_interleave_lo, 1); 
                __m128i block3 = _mm256_extracti128_si256(vec_interleave_hi, 1); 
                
                __m128i out_0123 = _mm_packus_epi32(block0, block1); 
                __m128i out_4567 = _mm_packus_epi32(block2, block3); 
                
                _mm_storeu_si128((__m128i *)out, out_0123);
                _mm_storeu_si128((__m128i *)(out + 8), out_4567);
                out += 16;
            }

            // 第二步的第一部分标量余数尾部
            i = p * 2;
            for (; i < len - 1; i += 2)
            {
                uint32_t r = bottomr[i / 2];
                uint32_t r1;
                uint16_t r0;
                r += bottomt[i / 2] * R2[i / 2];
                uint32_divmod_uint14(&r1, &r0, r, M[i]);
                r1 = uint32_mod_uint14(r1, M[i + 1]);
                *out++ = r0;
                *out++ = r1;
            }
            if (i < len)
                *out++ = R2[i / 2];
        }
    }

    void pack_pk(unsigned char *r, const poly *a)
    {
        uint16_t R[DTRU_N], M[DTRU_N];
        int i = 0;

        __m256i v_q = _mm256_set1_epi16((short)DTRU_Q);
        for (; i + 15 < DTRU_N; i += 16)
        {
            __m256i v_coeffs = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
            _mm256_storeu_si256((__m256i *)&R[i], v_coeffs);
            _mm256_storeu_si256((__m256i *)&M[i], v_q);
        }

        for (; i < DTRU_N; ++i)
        {
            R[i] = (uint16_t)a->coeffs[i];
            M[i] = DTRU_Q;
        }

        Encode(r, r + DTRU_PKE_PUBLICKEYBYTES, R, M, DTRU_N);
    }

    void unpack_pk(poly *r, const unsigned char *a)
    {
        uint16_t R[DTRU_N], M[DTRU_N];
        int i = 0;

        __m256i v_q = _mm256_set1_epi16((short)DTRU_Q);
        for (; i + 15 < DTRU_N; i += 16)
        {
            _mm256_storeu_si256((__m256i *)&M[i], v_q);
        }
        for (; i < DTRU_N; ++i)
        {
            M[i] = DTRU_Q;
        }

        Decode(R, a, M, DTRU_N);

        i = 0;
        for (; i + 15 < DTRU_N; i += 16)
        {
            __m256i v_R = _mm256_loadu_si256((const __m256i *)&R[i]);
            _mm256_storeu_si256((__m256i *)&r->coeffs[i], v_R);
        }
        for (; i < DTRU_N; ++i)
        {
            r->coeffs[i] = ((int16_t)R[i]);
        }
    }

#else

    void pack_pk(unsigned char *r, const poly *a)
    {
        unsigned int i;
        for (i = 0; i < DTRU_N / 2; i++)
        {
            r[3 * i + 0] = (a->coeffs[2 * i + 0] >> 0);
            r[3 * i + 1] = (a->coeffs[2 * i + 0] >> 8) | (a->coeffs[2 * i + 1] << 4);
            r[3 * i + 2] = (a->coeffs[2 * i + 1] >> 4);
        }
    }

    void unpack_pk(poly *r, const unsigned char *a)
    {
        unsigned int i;
        for (i = 0; i < DTRU_N / 2; i++)
        {
            r->coeffs[2 * i + 0] = ((a[3 * i + 0] >> 0) | ((uint16_t)a[3 * i + 1] << 8)) & 0xFFF;
            r->coeffs[2 * i + 1] = ((a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4)) & 0xFFF;
        }
    }

#endif

void pack_sk(unsigned char *r, const poly *a)
{
    unsigned int i;
    uint8_t t[2];

    for (i = 0; i < DTRU_N / 2; i++)
    {
        t[0] = DTRU_BOUND - a->coeffs[2 * i + 0];
        t[1] = DTRU_BOUND - a->coeffs[2 * i + 1];
        r[i] = (t[0] >> 0) | (t[1] << 4);
    }
}

void unpack_sk(poly *r, const unsigned char *a)
{
    int i;
    for (i = 0; i < DTRU_N / 2; ++i)
    {
        r->coeffs[2 * i + 0] = DTRU_BOUND - ((a[i] >> 0) & 0xF);
        r->coeffs[2 * i + 1] = DTRU_BOUND - ((a[i] >> 4) & 0xF);
    }
}

void pack_ct(unsigned char *r, const poly *a)
{
    unsigned int i;
    for (i = 0; i < DTRU_N / 4; ++i)
    {
        r[5 * i + 0] = (a->coeffs[4 * i + 0] >> 0);
        r[5 * i + 1] = (a->coeffs[4 * i + 0] >> 8) | (a->coeffs[4 * i + 1] << 2);
        r[5 * i + 2] = (a->coeffs[4 * i + 1] >> 6) | (a->coeffs[4 * i + 2] << 4);
        r[5 * i + 3] = (a->coeffs[4 * i + 2] >> 4) | (a->coeffs[4 * i + 3] << 6);
        r[5 * i + 4] = (a->coeffs[4 * i + 3] >> 2);
    }
}

void unpack_decompress_ct(poly *r, const unsigned char *a)
{
    unsigned int i, j;
    int16_t t[4];
    int32_t temp;
    for (i = 0; i < DTRU_N / 4; ++i)
    {
        t[0] = ((a[5 * i + 0] >> 0) | ((uint16_t)a[5 * i + 1] << 8)) & 0x3FF;
        t[1] = ((a[5 * i + 1] >> 2) | ((uint16_t)a[5 * i + 2] << 6)) & 0x3FF;
        t[2] = ((a[5 * i + 2] >> 4) | ((uint16_t)a[5 * i + 3] << 4)) & 0x3FF;
        t[3] = ((a[5 * i + 3] >> 6) | ((uint16_t)a[5 * i + 4] << 2)) & 0x3FF;
        for (j = 0; j < 4; ++j)
        {
            temp = ((int32_t)(t[j] * DTRU_Q) + (DTRU_Q2 >> 1)) >> DTRU_LOGQ2;
            r->coeffs[4 * i + j] = temp;
        }
    }
}
