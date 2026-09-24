#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce_avx.h"
#include "symmetric.h"
#include "consts.h" 
#include "fips202x4.h"
#include <stdalign.h>
#include <immintrin.h>

#define _mm256_blendv_epi32(a,b,mask) \
  _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(a), \
                                       _mm256_castsi256_ps(b), \
                                       _mm256_castsi256_ps(mask)))

// 定义多项式的模约化----------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_freeze
 *
 * Description: For all coefficients of in/out polynomial compute standard
 *              representative r = a mod^+ Q
 *
 * Arguments:   - poly *a: pointer to input/output polynomial
 **************************************************/
void poly_freeze(poly *a) {
    unsigned int i;
    const __m256i v_qrec = _mm256_load_si256((const __m256i *)&qdata[_8XQREC]);
    const __m256i v_q    = _mm256_load_si256((const __m256i *)&qdata[_8XQ]);
    const __m256i v_dq   = _mm256_load_si256((const __m256i *)&qdata[_8XDQ]);

    for (i = 0; i < N; i += 8) {

        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        __m256i t = _mm256_mulhi_epi32_custom(va, v_qrec);
        __m256i t_Q = _mm256_mullo_epi32(t, v_q);

        t = _mm256_sub_epi32(va, t_Q);

        __m256i sign_t = _mm256_srai_epi32(t, 31);
        t = _mm256_add_epi32(t, _mm256_and_si256(sign_t, v_dq));

        __m256i t_minus_Q = _mm256_sub_epi32(t, v_q);
        __m256i shift = _mm256_srai_epi32(t_minus_Q, 31);
        __m256i not_shift_and_Q = _mm256_andnot_si256(shift, v_q);
        t = _mm256_sub_epi32(t, not_shift_and_Q);
        
        _mm256_storeu_si256((__m256i *)&a->coeffs[i], t);
    }
}

// 定义多项式的压缩与解压缩---------------------------------------------------------------------------------

/*************************************************
* Name:        poly_compress
*
* Description: Compression of a polynomial
*
* Arguments:   - poly *a_high: pointer to output polynomial
*              - poly *a:      pointer to input polynomial
**************************************************/
void poly_compress(poly *a_high, const poly *a) {
    unsigned int i;

    for (i = 0; i < N; i++){
        a_high->coeffs[i] = (((a->coeffs[i] << D) + Q/2) / Q) & ((1 << D) - 1);
    }
}

/*************************************************
* Name:        poly_decompress
*
* Description: Decompression of a polynomial
*
* Arguments:   - poly *a_mid:  pointer to output polynomial
*              - poly *a_high: pointer to input polynomial
**************************************************/
void poly_decompress(poly *a_mid, const poly *a_high) {
    unsigned int i;
    for (i = 0; i < N; i++) {
        int32_t t = a_high->coeffs[i] * Q + (1 << (D - 1));
        a_mid->coeffs[i] = t >> D;
    }
}

/*************************************************
* Name:        poly_lowbits
*
* Description: Compute the low bits of a polynomial
*
* Arguments:   - poly *a_low:   pointer to output polynomial (low bits)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_lowbits(poly *a_low, const poly *a) {
    unsigned int i;
    poly a_high;
    poly a_mid;

    poly_compress(&a_high, a);
    poly_decompress(&a_mid, &a_high);

    const __m256i v_q = _mm256_load_si256((const __m256i *)&qdata[_8XQ]);
    
    const __m256i v_half_q = _mm256_set1_epi32(Q >> 1); 
    const __m256i v_zero   = _mm256_setzero_si256();

    for (i = 0; i < N; i += 8) {

        __m256i va   = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        __m256i vmid = _mm256_loadu_si256((__m256i *)&a_mid.coeffs[i]);
        __m256i vdiff = _mm256_sub_epi32(va, vmid);
        __m256i vt = freeze_avx2(vdiff);
        __m256i vb = _mm256_sub_epi32(vt, v_half_q);
        __m256i vmask = _mm256_cmpgt_epi32(vb, v_zero);
        __m256i v_sub_q = _mm256_and_si256(vmask, v_q);

        vt = _mm256_sub_epi32(vt, v_sub_q);

        _mm256_storeu_si256((__m256i *)&a_low->coeffs[i], vt);
    }
}

/*************************************************
 * Name:        poly_compose
 * 
 * Description: Compose a polynomial from its low bits and high bits
 * 
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const poly *a_low: pointer to low bits polynomial
 *              - const poly *a_high: pointer to high bits polynomial
 **************************************************/
void poly_compose(poly *r, const poly *a_low, const poly *a_high) {
    unsigned int i;
    poly a_mid;

    poly_decompress(&a_mid, a_high);

    for (i = 0; i < N; i++)
        r->coeffs[i] = a_low->coeffs[i] + a_mid.coeffs[i];
}

// 定义的多项式之间的运算-------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_mul_halfq
 *
 * Description: Multiply polynomial by (Q+1)/2
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_mul_halfq(poly *r, const poly *a) {
    unsigned int i;

    for (i = 0; i < N; i++) {
        r->coeffs[i] = a->coeffs[i] * HALF_Q;
    }
}

/*************************************************
 * Name:        poly_equal
 *
 * Description: Check whether two polynomials are equal
 *
 * Arguments:   - const poly *a: pointer to first input polynomial
 *              - const poly *b: pointer to second input polynomial
 *
 * Returns:     - 1 if equal, 0 otherwise
 **************************************************/
int poly_equal(const poly *a, const poly *b) {
    size_t i;

    __m256i acc = _mm256_setzero_si256(); 

    for (i = 0; i < N; i += 8) {

        __m256i va = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_loadu_si256((const __m256i *)&b->coeffs[i]);

        acc = _mm256_or_si256(acc, _mm256_xor_si256(va, vb));
    }

    return _mm256_testz_si256(acc, acc);
}

/*************************************************
 * Name:        poly_mul_1_b
 *
 * Description: Multiply polynomial by (1 - b), where b is a bit (0 or 1)
 *
 * Arguments:   - poly *r:       pointer to output polynomial
 *              - const poly *a: pointer to input polynomial
 *              - uint8_t b:     bit (0 or 1)
 **************************************************/
void poly_mul_1_b(poly *r, const poly *a, uint8_t b) {

    unsigned int i;
    int32_t mask = (int32_t)b - 1;      // 精简掩码生成逻辑，减少一条计算指令
    __m256i vm = _mm256_set1_epi32(mask);

    for (i = 0; i < N; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        __m256i vr = _mm256_and_si256(va, vm);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i], vr);
    }
}


/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r) {
	ntt(r->coeffs);
	poly_freeze(r);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r) {
	invntt_tomont(r->coeffs);
}   

/*************************************************
* Name:        poly_mul_mont
*
* Description: Multiply each coefficient with MONT
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_mul_mont(poly *r) {
    unsigned int i;

    const __m256i v_montsq = _mm256_load_si256((const __m256i *)&qdata[_8XMONTSQ]);

    for (i = 0; i < N; i += 8) {
        __m256i va = _mm256_loadu_si256((__m256i *)&r->coeffs[i]);
        __m256i vr = fqmul_avx(va, v_montsq);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i], vr);
    }
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b) {
    unsigned int i;

    for (i = 0; i < N; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_loadu_si256((const __m256i *)&b->coeffs[i]);
        __m256i vr = _mm256_add_epi32(va, vb);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i], vr);
    }
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b) {
    unsigned int i;

    for (i = 0; i < N; i += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_loadu_si256((const __m256i *)&b->coeffs[i]);
        __m256i vr = _mm256_sub_epi32(va, vb);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i], vr);
    }
}


/*************************************************
* Name:        poly_caddq
*
* Description: Conditional addition of q to each coefficient
*
* Arguments: - poly *r:       pointer to output polynomial
**************************************************/
void poly_caddq(poly *a) {
    unsigned int i;
    __m256i f, g;
    
    const __m256i q = _mm256_load_si256((const __m256i *)&qdata[_8XQ]);
    const __m256i zero = _mm256_setzero_si256();

    for(i = 0; i < N / 8; ++i) {

        f = _mm256_load_si256((__m256i *)&a->coeffs[8 * i]);
        g = _mm256_blendv_epi32(zero, q, f);
        f = _mm256_add_epi32(f, g);
        
        _mm256_store_si256((__m256i *)&a->coeffs[8 * i], f);
    }
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
*************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b) {
    unsigned int i;

#if SIGN_MODE == 128 || SIGN_MODE == 256
    const unsigned int ZETA_OFFSET = 64;
#elif SIGN_MODE == 512
    const unsigned int ZETA_OFFSET = 128;
#endif

    const __m256i v_zero = _mm256_setzero_si256();

    for (i = 0; i < N / 8; i++) {
        int32_t zeta = qdata[_ZETAS + ZETA_OFFSET + i];
        __m256i vz = _mm256_set1_epi32(zeta);
        __m256i vzeta = _mm256_blend_epi32(vz, _mm256_sub_epi32(v_zero, vz), 0xF0);

        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
        __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[8 * i]);
        

        __m256i vbz = fqmul_avx(vb, vzeta);

        __m256i a0 = _mm256_shuffle_epi32(va, 0x00); 
        __m256i vr = fqmul_avx(a0, vb);

        __m256i a1 = _mm256_shuffle_epi32(va, 0x55);
        __m256i b_mix1 = _mm256_blend_epi32(_mm256_shuffle_epi32(vb, 0x93),
                                            _mm256_shuffle_epi32(vbz, 0x93), 0x11);
        vr = _mm256_add_epi32(vr, fqmul_avx(a1, b_mix1));

        __m256i a2 = _mm256_shuffle_epi32(va, 0xAA);
        __m256i b_mix2 = _mm256_blend_epi32(_mm256_shuffle_epi32(vb, 0x4E),
                                            _mm256_shuffle_epi32(vbz, 0x4E), 0x33);
        vr = _mm256_add_epi32(vr, fqmul_avx(a2, b_mix2));

        __m256i a3 = _mm256_shuffle_epi32(va, 0xFF);
        __m256i b_mix3 = _mm256_blend_epi32(_mm256_shuffle_epi32(vb, 0x39),
                                            _mm256_shuffle_epi32(vbz, 0x39), 0x77);
        vr = _mm256_add_epi32(vr, fqmul_avx(a3, b_mix3));


        _mm256_storeu_si256((__m256i *)&r->coeffs[8 * i], vr);
    }
}

/*************************************************
* Name:        poly_baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^8-zeta)
*              used for inversion of element in Rq in NTT domain
*  
* Arguments:   - poly *b: pointer to the output polynomial
*              - const poly *a: pointer to the input polynomial
***************************************************/
int poly_baseinv(poly *b, const poly *a) {
    int result = 0;
    unsigned int i;

    // 提取不同安全等级下的 Zeta 偏移量
#if SIGN_MODE == 128 || SIGN_MODE == 256
    const unsigned int ZETA_OFFSET = 64;
#elif SIGN_MODE == 512
    const unsigned int ZETA_OFFSET = 128;
#endif

    // 提取到循环外，作为常量 0 和比较基准
    const __m256i vzero = _mm256_setzero_si256();

    // AVX2 步长为 8，每次处理两个 4x4 的多项式求逆块
    for(i = 0; i < N / 8; i++) {

        int32_t zeta = qdata[_ZETAS + ZETA_OFFSET + i];

        __m256i vzeta = _mm256_set_epi32(-zeta, -zeta, -zeta, -zeta, 
                                          zeta,  zeta,  zeta,  zeta);
        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
        __m256i vb = vzero;
        __m256i a0 = _mm256_shuffle_epi32(va, 0x00);
        __m256i a1 = _mm256_shuffle_epi32(va, 0x55);
        __m256i a2 = _mm256_shuffle_epi32(va, 0xAA);
        __m256i a3 = _mm256_shuffle_epi32(va, 0xFF);

        __m256i a1_x2 = _mm256_add_epi32(a1, a1); //
        __m256i a0_x2 = _mm256_add_epi32(a0, a0);

        __m256i t0 = _mm256_sub_epi32(fqmul_avx(a2, a2), fqmul_avx(a1_x2, a3));
        t0 = _mm256_add_epi32(fqmul_avx(a0, a0), fqmul_avx(t0, vzeta));

        __m256i t1 = fqmul_avx(a3, a3);
        __m256i t1_part1 = _mm256_sub_epi32(fqmul_avx(a0_x2, a2), fqmul_avx(a1, a1));
        t1 = _mm256_sub_epi32(t1_part1, fqmul_avx(t1, vzeta));

        __m256i t2 = fqmul_avx(t1, t1);
        t2 = _mm256_sub_epi32(fqmul_avx(t0, t0), fqmul_avx(t2, vzeta));

        __m256i cmp = _mm256_cmpeq_epi32(t2, vzero);
        if (!_mm256_testz_si256(cmp, cmp)) {
            result -= 1; 
            _mm256_storeu_si256((__m256i *)&b->coeffs[8 * i], vb);
            continue; 
        }

        t2 = fqinv_avx(t2);
        t0 = montgomery_reduce_avx(fqmul_avx(t0, t2));
        t1 = montgomery_reduce_avx(fqmul_avx(t1, t2));
        t0 = montgomery_reduce_avx(t0);
        t1 = montgomery_reduce_avx(t1);
        t2 = fqmul_avx(t1, vzeta);

        __m256i b0 = _mm256_sub_epi32(fqmul_avx(a0, t0), fqmul_avx(a2, t2));
        
        __m256i b1_neg = _mm256_sub_epi32(vzero, fqmul_avx(a1, t0));
        __m256i b1 = _mm256_add_epi32(b1_neg, fqmul_avx(a3, t2));
        
        __m256i b2 = _mm256_sub_epi32(fqmul_avx(a2, t0), fqmul_avx(a0, t1));
        
        __m256i b3_neg = _mm256_sub_epi32(vzero, fqmul_avx(a3, t0));
        __m256i b3 = _mm256_add_epi32(b3_neg, fqmul_avx(a1, t1));

        __m256i mix01 = _mm256_blend_epi32(b0, b1, 0x22); 
        __m256i mix23 = _mm256_blend_epi32(b2, b3, 0x88); 
        vb = _mm256_blend_epi32(mix01, mix23, 0xCC); 

        _mm256_storeu_si256((__m256i *)&b->coeffs[8 * i], vb);
    }

    return result;
}

/*************************************************
 * Name:        poly_neg
 *
 * Description: Negate a polynomial
 *
 * Arguments:   - poly *r: pointer to output polynomial
 **************************************************/
void poly_neg(poly *r) {
    unsigned int i;
    __m256i vzero = _mm256_setzero_si256();

    for (i = 0; i < N; i += 8) {
        __m256i vr = _mm256_loadu_si256((__m256i *)&r->coeffs[i]);
        vr = _mm256_sub_epi32(vzero, vr);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i], vr);
    }
}


// 定义多项式的封装------------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_q_pack
 *
 * Description: Pack polynomial with coefficients in [0, Q - 1].
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            POLY_Q_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_q_pack(uint8_t *r, const poly *a) {
    unsigned int out = 0;
    
    union {
        __m256i vec;
        uint32_t arr[8];
    } t;

#if SIGN_MODE == 128 || SIGN_MODE == 256
    // q = 130817, 17 bits
    for (unsigned int i = 0; i < N; i += 8) {
        // 安全加载 8 个 32-bit 系数
        __m256i a_vec = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        
        // 并行 freeze 并通过 union 直接写入
        t.vec = freeze_avx2(a_vec);

        // 从 union 的数组端安全读取，屏蔽掉符号位
        uint32_t t0 = t.arr[0] & 0x1FFFFu;
        uint32_t t1 = t.arr[1] & 0x1FFFFu;
        uint32_t t2 = t.arr[2] & 0x1FFFFu;
        uint32_t t3 = t.arr[3] & 0x1FFFFu;
        uint32_t t4 = t.arr[4] & 0x1FFFFu;
        uint32_t t5 = t.arr[5] & 0x1FFFFu;
        uint32_t t6 = t.arr[6] & 0x1FFFFu;
        uint32_t t7 = t.arr[7] & 0x1FFFFu;

        r[out +  0] = (uint8_t)(t0);
        r[out +  1] = (uint8_t)(t0 >> 8);
        r[out +  2] = (uint8_t)((t0 >> 16) | (t1 << 1));
        r[out +  3] = (uint8_t)(t1 >> 7);
        r[out +  4] = (uint8_t)((t1 >> 15) | (t2 << 2));
        r[out +  5] = (uint8_t)(t2 >> 6);
        r[out +  6] = (uint8_t)((t2 >> 14) | (t3 << 3));
        r[out +  7] = (uint8_t)(t3 >> 5);
        r[out +  8] = (uint8_t)((t3 >> 13) | (t4 << 4));
        r[out +  9] = (uint8_t)(t4 >> 4);
        r[out + 10] = (uint8_t)((t4 >> 12) | (t5 << 5));
        r[out + 11] = (uint8_t)(t5 >> 3);
        r[out + 12] = (uint8_t)((t5 >> 11) | (t6 << 6));
        r[out + 13] = (uint8_t)(t6 >> 2);
        r[out + 14] = (uint8_t)((t6 >> 10) | (t7 << 7));
        r[out + 15] = (uint8_t)(t7 >> 1);
        r[out + 16] = (uint8_t)(t7 >> 9);

        out += 17;
    }

#elif SIGN_MODE == 512
    // q = 260609, 18 bits
    for (unsigned int i = 0; i < N; i += 8) {
        __m256i a_vec = _mm256_loadu_si256((const __m256i *)&a->coeffs[i]);
        t.vec = freeze_avx2(a_vec);

        // -- 处理前 4 个系数 --
        uint32_t t0 = t.arr[0] & 0x3FFFFu;
        uint32_t t1 = t.arr[1] & 0x3FFFFu;
        uint32_t t2 = t.arr[2] & 0x3FFFFu;
        uint32_t t3 = t.arr[3] & 0x3FFFFu;

        r[out + 0] = (uint8_t)(t0);
        r[out + 1] = (uint8_t)(t0 >> 8);
        r[out + 2] = (uint8_t)((t0 >> 16) | (t1 << 2));
        r[out + 3] = (uint8_t)(t1 >> 6);
        r[out + 4] = (uint8_t)((t1 >> 14) | (t2 << 4));
        r[out + 5] = (uint8_t)(t2 >> 4);
        r[out + 6] = (uint8_t)((t2 >> 12) | (t3 << 6));
        r[out + 7] = (uint8_t)(t3 >> 2);
        r[out + 8] = (uint8_t)(t3 >> 10);
        out += 9;

        // -- 处理后 4 个系数 --
        uint32_t t4 = t.arr[4] & 0x3FFFFu;
        uint32_t t5 = t.arr[5] & 0x3FFFFu;
        uint32_t t6 = t.arr[6] & 0x3FFFFu;
        uint32_t t7 = t.arr[7] & 0x3FFFFu;

        r[out + 0] = (uint8_t)(t4);
        r[out + 1] = (uint8_t)(t4 >> 8);
        r[out + 2] = (uint8_t)((t4 >> 16) | (t5 << 2));
        r[out + 3] = (uint8_t)(t5 >> 6);
        r[out + 4] = (uint8_t)((t5 >> 14) | (t6 << 4));
        r[out + 5] = (uint8_t)(t6 >> 4);
        r[out + 6] = (uint8_t)((t6 >> 12) | (t7 << 6));
        r[out + 7] = (uint8_t)(t7 >> 2);
        r[out + 8] = (uint8_t)(t7 >> 10);
        out += 9;
    }
#endif
}
/*************************************************
 * Name:        poly_q_unpack
 *
 * Description: Unpack polynomial with coefficients in [0, Q - 1].
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array with 
 *                                  at least POLY_Q_PACKEDBYTES bytes
 **************************************************/
void poly_q_unpack(poly *r, const uint8_t *a) {
    unsigned int i;
    __m256i f;

#if SIGN_MODE == 128 || SIGN_MODE == 256
    // q = 130817, 17 bits
    // 上下通道的 Shuffle 掩码完全一样
    const __m256i shufbidx = _mm256_set_epi8(
        -1, 8, 7, 6, -1, 6, 5, 4, -1, 4, 3, 2, -1, 2, 1, 0, // Upper lane
        -1, 8, 7, 6, -1, 6, 5, 4, -1, 4, 3, 2, -1, 2, 1, 0  // Lower lane
    );
    // 17 bits 的移位是连续的 0 到 7
    const __m256i srlvdidx = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    const __m256i mask = _mm256_set1_epi32(0x1FFFF);

    for (i = 0; i < N / 8; i++) {
        // 1. 一次性吸入 32 字节
        f = _mm256_loadu_si256((__m256i *)&a[17 * i]);
        // 2. 跨通道复制：将字节 8~15 复制到上通道
        f = _mm256_permute4x64_epi64(f, 0x94);
        // 3. 字节重排
        f = _mm256_shuffle_epi8(f, shufbidx);
        // 4. 可变位移
        f = _mm256_srlv_epi32(f, srlvdidx);
        // 5. 掩码截断
        f = _mm256_and_si256(f, mask);
        // 6. 输出 8 个 32-bit 整数
        _mm256_storeu_si256((__m256i *)&r->coeffs[8 * i], f);
    }

#elif SIGN_MODE == 512
    // q = 260609, 18 bits
    // 注意：18 bits 时，上通道需要从字节 9 开始，所以掩码与下通道不同
    const __m256i shufbidx = _mm256_set_epi8(
        -1, 9, 8, 7, -1, 7, 6, 5, -1, 5, 4, 3, -1, 3, 2, 1, // Upper lane 
        -1, 8, 7, 6, -1, 6, 5, 4, -1, 4, 3, 2, -1, 2, 1, 0  // Lower lane
    );
    // 18 bits 的移位是 0,2,4,6 的循环
    const __m256i srlvdidx = _mm256_setr_epi32(0, 2, 4, 6, 0, 2, 4, 6);
    const __m256i mask = _mm256_set1_epi32(0x3FFFF);

    for (i = 0; i < N / 8; i++) {
        f = _mm256_loadu_si256((__m256i *)&a[18 * i]);
        f = _mm256_permute4x64_epi64(f, 0x94);
        f = _mm256_shuffle_epi8(f, shufbidx);
        f = _mm256_srlv_epi32(f, srlvdidx);
        f = _mm256_and_si256(f, mask);
        _mm256_storeu_si256((__m256i *)&r->coeffs[8 * i], f);
    }
#endif
}

/*************************************************
 * Name:        poly_p_pack
 *
 * Description: Pack polynomial with coefficients in [-1, 1].
 *              Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 * 
 * Arguments:   - uint8_t *r: pointer to output byte array with 
 *                            at least POLY_S_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_p_pack(uint8_t *r, const poly *a) {

    __m256i v_one = _mm256_set1_epi32(1);
    __m256i v_mask = _mm256_set1_epi32(0x03);



    for (int i = 0; i < N; i += 32) {

        __m256i r0 = _mm256_loadu_si256((__m256i *)&a->coeffs[i + 0]);  // coeffs 0-7
        __m256i r1 = _mm256_loadu_si256((__m256i *)&a->coeffs[i + 8]);  // coeffs 8-15
        __m256i r2 = _mm256_loadu_si256((__m256i *)&a->coeffs[i + 16]); // coeffs 16-23
        __m256i r3 = _mm256_loadu_si256((__m256i *)&a->coeffs[i + 24]); // coeffs 24-31


        r0 = _mm256_and_si256(_mm256_sub_epi32(v_one, r0), v_mask);
        r1 = _mm256_and_si256(_mm256_sub_epi32(v_one, r1), v_mask);
        r2 = _mm256_and_si256(_mm256_sub_epi32(v_one, r2), v_mask);
        r3 = _mm256_and_si256(_mm256_sub_epi32(v_one, r3), v_mask);

        __m256i p16_0 = _mm256_packus_epi32(r0, r1);
        __m256i p16_1 = _mm256_packus_epi32(r2, r3);
        __m256i p8 = _mm256_packus_epi16(p16_0, p16_1);
        __m256i sorted = _mm256_permute4x64_epi64(p8, 0xD8);

        __m256i indices = _mm256_setr_epi32(0, 2, 1, 3, 4, 6, 5, 7);
        __m256i result = _mm256_permutevar8x32_epi32(sorted, indices);

        __m256i b0 = result;                                     
        __m256i b1 = _mm256_srli_epi32(result, 6);               
        __m256i b2 = _mm256_srli_epi32(result, 12);              
        __m256i b3 = _mm256_srli_epi32(result, 18);

        __m256i res = _mm256_or_si256(
                        _mm256_and_si256(b0, _mm256_set1_epi32(0x03030303)),
                        _mm256_or_si256(
                            _mm256_and_si256(b1, _mm256_set1_epi32(0x0C0C0C0C)),
                            _mm256_or_si256(
                                _mm256_and_si256(b2, _mm256_set1_epi32(0x30303030)),
                                _mm256_and_si256(b3, _mm256_set1_epi32(0xC0C0C0C0))
                            )
                        )
                      );

        // 打包后的字节现在位于 res 的索引 0, 4, 8, 12... 处
        __m256i final_shuffle = _mm256_shuffle_epi8(res, 
            _mm256_setr_epi8(0, 4, 8, 12, 16, 20, 24, 28, -1, -1, -1, -1, -1, -1, -1, -1,
                             0, 4, 8, 12, 16, 20, 24, 28, -1, -1, -1, -1, -1, -1, -1, -1));


        uint32_t low_part = _mm256_extract_epi32(final_shuffle, 0);
        uint32_t high_part = _mm256_extract_epi32(final_shuffle, 4);

        *((uint32_t *)&r[i/4]) = low_part;
        *((uint32_t *)&r[i/4 + 4]) = high_part;

    }

}


/*************************************************
 * Name:        poly_p_unpack
 *
 * Description: Unpack polynomial with coefficients in [-1, 1].
 *              Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: byte array with bit-packed polynomial
 **************************************************/
void poly_p_unpack(poly *r, const uint8_t *a) {
    __m256i v_one = _mm256_set1_epi32(1);
    __m256i v_mask = _mm256_set1_epi32(0x03);

    for (int i = 0; i < N; i += 32) {
        
        __m128i packed_bytes = _mm_loadl_epi64((__m128i const *)&a[i / 4]); 
        __m256i v = _mm256_cvtepu8_epi32(packed_bytes);

        __m256i c0 = _mm256_and_si256(v, v_mask);
        __m256i c1 = _mm256_and_si256(_mm256_srli_epi32(v, 2), v_mask);
        __m256i c2 = _mm256_and_si256(_mm256_srli_epi32(v, 4), v_mask);
        __m256i c3 = _mm256_and_si256(_mm256_srli_epi32(v, 6), v_mask);

        // 4. 逆向变换: coeff = 1 - t
        c0 = _mm256_sub_epi32(v_one, c0);
        c1 = _mm256_sub_epi32(v_one, c1);
        c2 = _mm256_sub_epi32(v_one, c2);
        c3 = _mm256_sub_epi32(v_one, c3);

        
        __m256i c01_lo = _mm256_unpacklo_epi32(c0, c1);
        __m256i c01_hi = _mm256_unpackhi_epi32(c0, c1);
        __m256i c23_lo = _mm256_unpacklo_epi32(c2, c3);
        __m256i c23_hi = _mm256_unpackhi_epi32(c2, c3);

        __m256i res0 = _mm256_unpacklo_epi64(c01_lo, c23_lo); // 0，4
        __m256i res1 = _mm256_unpackhi_epi64(c01_lo, c23_lo); // 1，5
        __m256i res2 = _mm256_unpacklo_epi64(c01_hi, c23_hi); // 2，6
        __m256i res3 = _mm256_unpackhi_epi64(c01_hi, c23_hi); // 3，7

        __m256i final0 = _mm256_permute2x128_si256(res0, res1, 0x20); // 取 res0低 和 res1低 -> [a0..a7]
        __m256i final1 = _mm256_permute2x128_si256(res2, res3, 0x20); // 取 res2低 和 res3低 -> [a8..a15]
        __m256i final2 = _mm256_permute2x128_si256(res0, res1, 0x31); // 取 res0高 和 res1高 -> [a16..a23]
        __m256i final3 = _mm256_permute2x128_si256(res2, res3, 0x31); // 取 res2高 和 res3高 -> [a24..a31]

        _mm256_storeu_si256((__m256i *)&r->coeffs[i + 0], final0);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i + 8], final1);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i + 16], final2);
        _mm256_storeu_si256((__m256i *)&r->coeffs[i + 24], final3);


    }
}


/*************************************************
 * Name:        poly_pack_highbits
 *
 * Description: Pack the high bits of a polynomial (does compression internally)
 *              Input should be in [0, Q-1] range.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            N * D / 8 bytes
 *              - const poly *a: pointer to input polynomial in [0, Q-1]
 **************************************************/
void poly_pack_highbits(uint8_t *r, const poly *a) {
    unsigned int i;    

#if D == 10
    for (i = 0; i < N / 4; ++i) {
        uint32_t t0 = (uint32_t)(((a->coeffs[4 * i + 0] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t1 = (uint32_t)(((a->coeffs[4 * i + 1] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t2 = (uint32_t)(((a->coeffs[4 * i + 2] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t3 = (uint32_t)(((a->coeffs[4 * i + 3] << D) + Q/2) / Q) & 0x3FFu;

        r[5 * i + 0] = (uint8_t)(t0);
        r[5 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 2));
        r[5 * i + 2] = (uint8_t)(t1 >> 6 | (t2 << 4));
        r[5 * i + 3] = (uint8_t)(t2 >> 4 | (t3 << 6));
        r[5 * i + 4] = (uint8_t)(t3 >> 2);
    }

#elif D == 9
    for (i = 0; i < N / 8; ++i) {
        uint32_t t0 = (uint32_t)(((a->coeffs[8 * i + 0] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t1 = (uint32_t)(((a->coeffs[8 * i + 1] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t2 = (uint32_t)(((a->coeffs[8 * i + 2] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t3 = (uint32_t)(((a->coeffs[8 * i + 3] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t4 = (uint32_t)(((a->coeffs[8 * i + 4] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t5 = (uint32_t)(((a->coeffs[8 * i + 5] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t6 = (uint32_t)(((a->coeffs[8 * i + 6] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t7 = (uint32_t)(((a->coeffs[8 * i + 7] << D) + Q/2) / Q) & 0x1FFu;

        r[9 * i + 0] = (uint8_t)(t0);
        r[9 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 1));
        r[9 * i + 2] = (uint8_t)(t1 >> 7 | (t2 << 2));
        r[9 * i + 3] = (uint8_t)(t2 >> 6 | (t3 << 3));
        r[9 * i + 4] = (uint8_t)(t3 >> 5 | (t4 << 4));
        r[9 * i + 5] = (uint8_t)(t4 >> 4 | (t5 << 5));
        r[9 * i + 6] = (uint8_t)(t5 >> 3 | (t6 << 6));
        r[9 * i + 7] = (uint8_t)(t6 >> 2 | (t7 << 7));
        r[9 * i + 8] = (uint8_t)(t7 >> 1);
    }
#endif
}

/*************************************************
 * Name:        poly_pack_compressed
 *
 * Description: Pack already-compressed D-bit values (no compression performed).
 *              Input should already be in [0, 2^D - 1] range.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            N * D / 8 bytes
 *              - const poly *a: pointer to input polynomial in [0, 2^D - 1]
 **************************************************/
void poly_pack_compressed(uint8_t *r, const poly *a) {

#if D == 10
    __m256i m_mask = _mm256_set1_epi32(0x3FF);
#elif D == 9
    __m256i m_mask = _mm256_set1_epi32(0x1FF);
#endif

    for (int i = 0; i < N; i += 32) {
        __m256i v0 = _mm256_and_si256(_mm256_load_si256((const __m256i *)&a->coeffs[i + 0]),  m_mask);
        __m256i v1 = _mm256_and_si256(_mm256_load_si256((const __m256i *)&a->coeffs[i + 8]),  m_mask);
        __m256i v2 = _mm256_and_si256(_mm256_load_si256((const __m256i *)&a->coeffs[i + 16]), m_mask);
        __m256i v3 = _mm256_and_si256(_mm256_load_si256((const __m256i *)&a->coeffs[i + 24]), m_mask);


#if D == 10
        uint8_t *dest = r + (i * 5 / 4);
        alignas(32) uint32_t buf[32];
        _mm256_store_si256((__m256i*)&buf[0],  v0);
        _mm256_store_si256((__m256i*)&buf[8],  v1);
        _mm256_store_si256((__m256i*)&buf[16], v2);
        _mm256_store_si256((__m256i*)&buf[24], v3);

        for(int k=0; k<8; k++) {
            uint32_t *tk = &buf[k*4];
            uint8_t *dk = dest + (k * 5); // 每 4 个系数占 5 字节

            uint64_t packed = (uint64_t)tk[0] | ((uint64_t)tk[1] << 10) | ((uint64_t)tk[2] << 20) | 
                           ((uint64_t)tk[3] << 30);
            
            *(uint32_t*)dk = packed;
            dk[4] = (uint16_t)(tk[3] >> 2); 
        }

#elif D == 9
        uint8_t *dest = r + (i * 9 / 8);
        alignas(32) uint32_t buf[32];
        _mm256_store_si256((__m256i*)&buf[0],  v0);
        _mm256_store_si256((__m256i*)&buf[8],  v1);
        _mm256_store_si256((__m256i*)&buf[16], v2);
        _mm256_store_si256((__m256i*)&buf[24], v3);

        for(int k = 0; k < 4; k++) { // 32 / 8 = 4 组
            uint32_t *tk = &buf[k * 8];
            uint8_t *dk = dest + (k * 9);

            uint64_t p0 = (uint64_t)(tk[0] & 0x1FF) | 
                          ((uint64_t)(tk[1] & 0x1FF) << 9) | 
                          ((uint64_t)(tk[2] & 0x1FF) << 18) | 
                          ((uint64_t)(tk[3] & 0x1FF) << 27);
            
            uint64_t p1 = (uint64_t)(tk[4] & 0x1FF) | 
                          ((uint64_t)(tk[5] & 0x1FF) << 9) | 
                          ((uint64_t)(tk[6] & 0x1FF) << 18) | 
                          ((uint64_t)(tk[7] & 0x1FF) << 27);

            
            *(uint32_t *)dk = (uint32_t)p0;
            dk[4] = (uint8_t)(p0 >> 32) | (uint8_t)(p1 << 4);
            uint32_t p1_high = (uint32_t)(p1 >> 4);
            memcpy(dk + 5, &p1_high, 4); 
        }
#endif
    }
}

/*************************************************
 * Name:        poly_unpack_highbits
 *
 * Description: Unpack the high bits of a polynomial
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array with at least
 *                                  N * D / 8 bytes
 **************************************************/
void poly_unpack_highbits(poly *r, const uint8_t *a) {
    unsigned int i;

#if D == 10
    for (i = 0; i < N / 4; ++i) {
        uint32_t b0 = a[5 * i + 0];
        uint32_t b1 = a[5 * i + 1];
        uint32_t b2 = a[5 * i + 2];
        uint32_t b3 = a[5 * i + 3];
        uint32_t b4 = a[5 * i + 4];

        uint32_t t0 = b0 | ((b1 & 0x03u) << 8);
        uint32_t t1 = (b1 >> 2) | ((b2 & 0x0Fu) << 6);
        uint32_t t2 = (b2 >> 4) | ((b3 & 0x3Fu) << 4);
        uint32_t t3 = (b3 >> 6) | (b4 << 2);

        r->coeffs[4 * i + 0] = (int32_t)(t0 & 0x3FFu);
        r->coeffs[4 * i + 1] = (int32_t)(t1 & 0x3FFu);
        r->coeffs[4 * i + 2] = (int32_t)(t2 & 0x3FFu);
        r->coeffs[4 * i + 3] = (int32_t)(t3 & 0x3FFu);
    }
#elif D == 9
    for (i = 0; i < N / 8; ++i) {
        uint32_t b0 = a[9 * i + 0];
        uint32_t b1 = a[9 * i + 1];
        uint32_t b2 = a[9 * i + 2];
        uint32_t b3 = a[9 * i + 3];
        uint32_t b4 = a[9 * i + 4];
        uint32_t b5 = a[9 * i + 5];
        uint32_t b6 = a[9 * i + 6];
        uint32_t b7 = a[9 * i + 7];
        uint32_t b8 = a[9 * i + 8];

        uint32_t t0 = b0 | ((b1 & 0x01u) << 8);
        uint32_t t1 = (b1 >> 1) | ((b2 & 0x03u) << 7);
        uint32_t t2 = (b2 >> 2) | ((b3 & 0x07u) << 6);
        uint32_t t3 = (b3 >> 3) | ((b4 & 0x0Fu) << 5);
        uint32_t t4 = (b4 >> 4) | ((b5 & 0x1Fu) << 4);
        uint32_t t5 = (b5 >> 5) | ((b6 & 0x3Fu) << 3);
        uint32_t t6 = (b6 >> 6) | ((b7 & 0x7Fu) << 2);
        uint32_t t7 = (b7 >> 7) | (b8 << 1);

        r->coeffs[8 * i + 0] = (int32_t)(t0 & 0x1FFu);
        r->coeffs[8 * i + 1] = (int32_t)(t1 & 0x1FFu);
        r->coeffs[8 * i + 2] = (int32_t)(t2 & 0x1FFu);
        r->coeffs[8 * i + 3] = (int32_t)(t3 & 0x1FFu);
        r->coeffs[8 * i + 4] = (int32_t)(t4 & 0x1FFu);
        r->coeffs[8 * i + 5] = (int32_t)(t5 & 0x1FFu);
        r->coeffs[8 * i + 6] = (int32_t)(t6 & 0x1FFu);
        r->coeffs[8 * i + 7] = (int32_t)(t7 & 0x1FFu);
    }
#endif
}

/*************************************************
 * Name:        poly_pack_lowbits
 *
 * Description: Pack the low bits of a polynomial using BAT encoding.
 *              For D=10 at 128-bit security: coefficients in [-64,64] are
 *              encoded as x' = x + 64 in [0,128], then decomposed as
 *              x' = 8*x1 + x2 where x1 in [0,16], x2 in [0,7].
 *              Each of 8 consecutive coefficients: 3*8 bits low (x2) + 
 *              base-17 encoding of 8 x1 values (33 bits) = 57 bits total.
 *
 *              For D=9 at 256-bit security: coefficients in [-128,128] are
 *              encoded similarly, reducing 72 bits to 65 bits per 8 coeff.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            POLY_LOWBITS_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_pack_lowbits(uint8_t *r, const poly *a) {
    uint8_t *out = r;

#if SIGN_MODE == 128
    /* 128-bits security: x in [-64,64] -> [0,128] -> 8*x1 + x2 */
    unsigned __int128 pool = 0;
    int pool_bits = 0;
    
    for (unsigned int i = 0; i < N; i += 8) {
        uint32_t x1[8], x2[8];
        uint64_t base17_val = 0;
        
        /* 1. 用极速的位移和掩码代替低效的除法和取模 */
        for (unsigned int j = 0; j < 8; j++) {
            uint32_t x_shifted = (uint32_t)(a->coeffs[i + j] + 64);
            x1[j] = x_shifted >> 3;  // 等价于 / 8
            x2[j] = x_shifted & 7;   // 等价于 % 8
        }
        
        /* 2. Base-17 编码 */
        for (unsigned int j = 0; j < 8; j++) {
            base17_val = base17_val * 17 + x1[j];
        }
        
        /* 3. 极速拼接 24 位的 low_bits */
        uint64_t low_bits = x2[0] | (x2[1] << 3) | (x2[2] << 6) | (x2[3] << 9) |
                           (x2[4] << 12) | (x2[5] << 15) | (x2[6] << 18) | (x2[7] << 21);
        
        /* 4. 将 57 位数据直接砸进水池 */
        unsigned __int128 block57 = ((unsigned __int128)base17_val << 24) | low_bits;
        pool |= (block57 << pool_bits);
        pool_bits += 57;
        
        /* 5. 按字节快速排水，消灭 bit 级循环 */
        while (pool_bits >= 8) {
            *out++ = (uint8_t)(pool & 0xFF);
            pool >>= 8;
            pool_bits -= 8;
        }
    }
    // 处理最后可能剩下的不足 8 bits 的尾巴（如果有）
    if (pool_bits > 0) {
        *out = (uint8_t)pool;
    }
    
#elif SIGN_MODE == 256
    /* 256-bit security: x in [-128,128] -> [0,256] -> 16*x1 + x2 */
    unsigned __int128 pool = 0;
    int pool_bits = 0;

    for (unsigned int i = 0; i < N; i += 8) {
        uint32_t x1[8], x2[8];
        uint64_t base17_val = 0;
        
        for (unsigned int j = 0; j < 8; j++) {
            uint32_t x_shifted = (uint32_t)(a->coeffs[i + j] + 128);
            x1[j] = x_shifted >> 4;   // 等价于 / 16
            x2[j] = x_shifted & 15;   // 等价于 % 16
        }
        
        for (unsigned int j = 0; j < 8; j++) {
            base17_val = base17_val * 17 + x1[j];
        }
        
        uint64_t low_bits = x2[0] | (x2[1] << 4) | (x2[2] << 8) | (x2[3] << 12) |
                           (x2[4] << 16) | (x2[5] << 20) | (x2[6] << 24) | (x2[7] << 28);
        
        /* 合并为 65 位并砸进水池 */
        unsigned __int128 block65 = ((unsigned __int128)base17_val << 32) | low_bits;
        pool |= (block65 << pool_bits);
        pool_bits += 65;
        
        while (pool_bits >= 8) {
            *out++ = (uint8_t)(pool & 0xFF);
            pool >>= 8;
            pool_bits -= 8;
        }
    }
    if (pool_bits > 0) {
        *out = (uint8_t)pool;
    }

#elif SIGN_MODE == 512
    /* 512-bit security: 32 bits to 8 bits compression */
#if defined(__AVX2__)
    // 这里使用 AVX2 的 Pack 指令，将 32个 int32 直接瞬间压缩成 32个 uint8
    const __m256i offset = _mm256_set1_epi32(128);
    for (unsigned int i = 0; i < N; i += 32) {
        __m256i c0 = _mm256_loadu_si256((__m256i*)&a->coeffs[i + 0]);
        __m256i c1 = _mm256_loadu_si256((__m256i*)&a->coeffs[i + 8]);
        __m256i c2 = _mm256_loadu_si256((__m256i*)&a->coeffs[i + 16]);
        __m256i c3 = _mm256_loadu_si256((__m256i*)&a->coeffs[i + 24]);

        c0 = _mm256_add_epi32(c0, offset);
        c1 = _mm256_add_epi32(c1, offset);
        c2 = _mm256_add_epi32(c2, offset);
        c3 = _mm256_add_epi32(c3, offset);

        // 两步压缩：32位 -> 16位 -> 8位
        __m256i pack16_01 = _mm256_packus_epi32(c0, c1);
        __m256i pack16_23 = _mm256_packus_epi32(c2, c3);
        __m256i pack8 = _mm256_packus_epi16(pack16_01, pack16_23);

        // 修复跨通道顺序打乱问题
        const __m256i perm = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);
        pack8 = _mm256_permutevar8x32_epi32(pack8, perm);

        _mm256_storeu_si256((__m256i*)&r[i], pack8);
    }
#else
    // 标量回退版本，加入手工循环展开
    for (unsigned int i = 0; i < N; i += 8) {
        r[i+0] = (uint8_t)(a->coeffs[i+0] + 128);
        r[i+1] = (uint8_t)(a->coeffs[i+1] + 128);
        r[i+2] = (uint8_t)(a->coeffs[i+2] + 128);
        r[i+3] = (uint8_t)(a->coeffs[i+3] + 128);
        r[i+4] = (uint8_t)(a->coeffs[i+4] + 128);
        r[i+5] = (uint8_t)(a->coeffs[i+5] + 128);
        r[i+6] = (uint8_t)(a->coeffs[i+6] + 128);
        r[i+7] = (uint8_t)(a->coeffs[i+7] + 128);
    }
#endif

#endif
}

/*************************************************
 * Name:        poly_unpack_lowbits
 *
 * Description: Unpack the low bits of a polynomial using BAT decoding.
 *              Reverses the encoding from poly_pack_lowbits.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array with at least
 *                                  POLY_LOWBITS_PACKEDBYTES bytes
 **************************************************/
void poly_unpack_lowbits(poly *a, const uint8_t *r) {

#if SIGN_MODE == 128
    // 强制使用 uint64_t 存储 bv，因为 Base-17 超过了 2^32
    uint64_t bv[4];
    uint32_t x2_packed[4];
    
    unsigned __int128 pool = 0;
    int pool_bits = 0;
    const uint8_t *ptr = r;

    for (unsigned int i = 0; i < N; i += 32) {
        //  提取 4 组 57-bit 块
        for (int k = 0; k < 4; k++) {
            while (pool_bits < 57) {
                pool |= ((unsigned __int128)*ptr++) << pool_bits;
                pool_bits += 8;
            }

            // 使用强制转换确保提取完整的 57 位到 uint64_t
            uint64_t block57 = (uint64_t)(pool & (((unsigned __int128)1 << 57) - 1));
            pool >>= 57;
            pool_bits -= 57;

            // 拆分：低 24 位是 x2，高 33 位是 Base-17 (bv)
            x2_packed[k] = (uint32_t)(block57 & 0xFFFFFF);
            bv[k] = block57 >> 24;
        }

        // 还原系数
        for (int k = 0; k < 4; k++) {
            uint64_t val = bv[k]; // 使用 uint64_t 保证除法正确
            uint32_t x2p = x2_packed[k];
            
            for (int j = 0; j < 8; j++) {  //X = x7 + x6*17 + x5*17^2 + ... + x1*17^6 + x0*17^7 ,所以最后得到的系数顺序是7，6，5，4，3，2，1，0
                uint32_t x1 = (uint32_t)(val % 17);
                val /= 17;
                uint32_t x2 = (x2p >> (3 * (7 - j))) & 7;
                a->coeffs[i + k * 8 + (7 - j)] = (int32_t)((x1 << 3) | x2) - 64;
            }
        }
        
    }

#elif SIGN_MODE == 256
    const __m256i v_offset = _mm256_set1_epi32(128);
    unsigned __int128 pool = 0;
    int pool_bits = 0;
    const uint8_t *ptr = r; // 假设 r 是传入的打包后的字节流

    for (unsigned int i = 0; i < N; i += 32) {
        for (int k = 0; k < 4; k++) {
            while (pool_bits < 65) {
                pool |= ((unsigned __int128)*ptr++) << pool_bits;
                pool_bits += 8;
            }
            //pack 时是：block65 = (bv_k << 32) | x2_packed;
            uint32_t x2p = (uint32_t)(pool & 0xFFFFFFFF); 
            pool >>= 32;
            uint64_t b17 = (uint64_t)(pool & (((unsigned __int128)1 << 33) - 1));
            pool >>= 33;
            pool_bits -= 65;

            uint64_t val = b17;
            
            for (int j = 0; j < 8; j++) {
                // 提取 x1 (高 4 位部分)
                uint32_t x1 = (uint32_t)(val % 17);
                val /= 17;
                uint32_t x2 = (x2p >> (4 * (7 - j))) & 15;
                a->coeffs[i + k * 8 + (7 - j)] = (int32_t)((x1 << 4) | x2) - 128;
            }
        }
    }

#elif SIGN_MODE == 512
    const __m256i v_offset_512 = _mm256_set1_epi32(128);
    
    for (unsigned int i = 0; i < N; i += 32) {
        
        __m256i packed8 = _mm256_loadu_si256((__m256i *)&r[i]);

        __m128i low128 = _mm256_castsi256_si128(packed8);
        __m128i high128 = _mm256_extracti128_si256(packed8, 1);

        __m256i c0 = _mm256_cvtepu8_epi32(low128);
        __m256i c1 = _mm256_cvtepu8_epi32(_mm_srli_si128(low128, 8));
        __m256i c2 = _mm256_cvtepu8_epi32(high128);
        __m256i c3 = _mm256_cvtepu8_epi32(_mm_srli_si128(high128, 8));

        _mm256_storeu_si256((__m256i*)&a->coeffs[i + 0],  _mm256_sub_epi32(c0, v_offset_512));
        _mm256_storeu_si256((__m256i*)&a->coeffs[i + 8],  _mm256_sub_epi32(c1, v_offset_512));
        _mm256_storeu_si256((__m256i*)&a->coeffs[i + 16], _mm256_sub_epi32(c2, v_offset_512));
        _mm256_storeu_si256((__m256i*)&a->coeffs[i + 24], _mm256_sub_epi32(c3, v_offset_512));
    }
#endif
}

// 定义多项式的采样------------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_challenge
 *
 * Description: Generate challenge polynomial wiht Hamming weight of TAU.
 *
 * Arguments:   - poly *c: pointer to output challenge polynomial
 *              - const uint8_t highbits[]: pointer to compressed w
 *              - const uint8_t mu[CRHBYTES]: pointer to message hash (64 bytes)
 **************************************************/
void poly_challenge(poly *c, const uint8_t highbits[POLYVECK_HIGHBITS_PACKEDBYTES], const uint8_t mu[CRHBYTES]) {
    unsigned int i, b, pos = 0;
    uint8_t buf[XOF256_BLOCKBYTES];
    xof256_state state;

    // rho = H(Compresss(w,d), mu)
    xof256_absorbe_twice(&state, highbits,
                         POLYVECK_HIGHBITS_PACKEDBYTES, mu,
                         CRHBYTES);
    xof256_squeezeblocks(buf, 1, &state);

    for (i = 0; i < N; ++i)
        c->coeffs[i] = 0;
    for (i = N - TAU; i < N; ++i) {
        do {
#if N == 256
            // N=256: i ≤ 255, need 8 bits, read 1 byte
            if (pos >= XOF256_BLOCKBYTES) {
                xof256_squeezeblocks(buf, 1, &state);
                pos = 0;
            }
            b = buf[pos++];
#elif N == 512
            // N=512: i ≤ 511, need 9 bits, read 2 bytes, mask to 9 bits
            if (pos + 1 >= XOF256_BLOCKBYTES) {
                xof256_squeezeblocks(buf, 1, &state);
                pos = 0;
            }
            b = buf[pos] | ((unsigned int)buf[pos + 1] << 8);
            pos += 2;
            b &= 0x1FFu;
#elif N == 1024
            // N=1024: i ≤ 1023, need 10 bits, read 2 bytes, mask to 10 bits
            if (pos + 1 >= XOF256_BLOCKBYTES) {
                xof256_squeezeblocks(buf, 1, &state);
                pos = 0;
            }
            b = buf[pos] | ((unsigned int)buf[pos + 1] << 8);
            pos += 2;
            b &= 0x3FFu;
#endif
        } while (b > i);

        c->coeffs[i] = c->coeffs[b];
        c->coeffs[b] = 1;
    }
}

void poly_uniform(poly *a, const uint8_t seed[SEEDBYTES], uint16_t nonce) {
    unsigned int blocks_numbers = 7;

    unsigned int i, ctr, off = 0;
    unsigned int buflen = blocks_numbers * STREAM128_BLOCKBYTES;
    uint8_t buf[blocks_numbers * STREAM128_BLOCKBYTES + 2];
    stream128_state state;

    stream128_init(&state, seed, nonce);
    stream128_squeezeblocks(buf, blocks_numbers, &state);

    ctr = rej_uniform(a->coeffs, N, buf, buflen);

    while (ctr < N) {
        off = ((8 * buflen) % Q_BITS + 7) / 8;
        // off = 1 or 2
        for (i = 0; i < off; ++i)
            buf[i] = buf[buflen - off + i];
        
        stream128_squeezeblocks(buf + off, 1, &state);
        buflen = STREAM128_BLOCKBYTES + off;
        ctr += rej_uniform(a->coeffs + ctr, N - ctr, buf, buflen);
    }
}

/*************************************************
 * Name:        poly_uniform
 *
 * Description: Generate a polynomial with uniform random coefficients in [0, Q-1]
 *              by performing rejection sampling on output of a SHAKE128(seed|nonce).
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const uint8_t seed[SEEDBYTES]: byte array with seed of length SEEDBYTES
 *              - uint16_t nonce: 2-byte nonce
 **************************************************/
void poly_uniform_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                     const uint8_t seed[SEEDBYTES],
                     uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3)
{
    unsigned int blocks_numbers = 7;
    unsigned int buflen = blocks_numbers * STREAM128_BLOCKBYTES;
    unsigned int ctr0, ctr1, ctr2, ctr3;

    __attribute__((aligned(32))) uint8_t buf[4][7 * STREAM128_BLOCKBYTES + STREAM128_BLOCKBYTES];

    keccakx4_state state;
    __m256i f;

    /* 拷贝 seed 前 32 字节 */
    f = _mm256_loadu_si256((__m256i *)seed);
    _mm256_store_si256((__m256i *)buf[0], f);
    _mm256_store_si256((__m256i *)buf[1], f);
    _mm256_store_si256((__m256i *)buf[2], f);
    _mm256_store_si256((__m256i *)buf[3], f);

#if SEEDBYTES == 64
    /* 拷贝 seed 后 32 字节（SEEDBYTES=64 时必须） */
    f = _mm256_loadu_si256((__m256i *)(seed + 32));
    _mm256_store_si256((__m256i *)(buf[0] + 32), f);
    _mm256_store_si256((__m256i *)(buf[1] + 32), f);
    _mm256_store_si256((__m256i *)(buf[2] + 32), f);
    _mm256_store_si256((__m256i *)(buf[3] + 32), f);
#endif

    buf[0][SEEDBYTES + 0] = nonce0;
    buf[0][SEEDBYTES + 1] = nonce0 >> 8;
    buf[1][SEEDBYTES + 0] = nonce1;
    buf[1][SEEDBYTES + 1] = nonce1 >> 8;
    buf[2][SEEDBYTES + 0] = nonce2;
    buf[2][SEEDBYTES + 1] = nonce2 >> 8;
    buf[3][SEEDBYTES + 0] = nonce3;
    buf[3][SEEDBYTES + 1] = nonce3 >> 8;

    shake128x4_absorb_once(&state, buf[0], buf[1], buf[2], buf[3], SEEDBYTES + 2);
    shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], blocks_numbers, &state);

    ctr0 = rej_uniform(a0->coeffs, N, buf[0], buflen);
    ctr1 = rej_uniform(a1->coeffs, N, buf[1], buflen);
    ctr2 = rej_uniform(a2->coeffs, N, buf[2], buflen);
    ctr3 = rej_uniform(a3->coeffs, N, buf[3], buflen);

    while (ctr0 < N || ctr1 < N || ctr2 < N || ctr3 < N) {
        shake128x4_squeezeblocks(buf[0], buf[1], buf[2], buf[3], 1, &state);

        ctr0 += rej_uniform(a0->coeffs + ctr0, N - ctr0, buf[0], STREAM128_BLOCKBYTES);
        ctr1 += rej_uniform(a1->coeffs + ctr1, N - ctr1, buf[1], STREAM128_BLOCKBYTES);
        ctr2 += rej_uniform(a2->coeffs + ctr2, N - ctr2, buf[2], STREAM128_BLOCKBYTES);
        ctr3 += rej_uniform(a3->coeffs + ctr3, N - ctr3, buf[3], STREAM128_BLOCKBYTES);
    }
}

/*************************************************
 * Name:        poly_ternary_p
 *
 * Description: Generate a polynomial with coefficients in {-1,0,1}
 *              such that Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 *              using output of a SHAKE128(seed|nonce).
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const uint8_t seed[SEEDBYTES]: byte array with seed of length SEEDBYTES
 *              - uint16_t nonce: 2-byte nonce
 **************************************************/
void poly_ternary_p(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
#if SIGN_MODE == 128
    // Pr[X = 1] = Pr[X = -1] = 0.15, Pr[X = 0] = 0.7
    unsigned int blocks_numbers = 3;
    // (N * 13 / 3 * 8) + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES
#elif SIGN_MODE == 256
    // Pr[X = 1] = Pr[X = -1] = 0.2, Pr[X = 0] = 0.6
    unsigned int blocks_numbers = 3;
    // (N * 13 / 3 * 8) + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES
#elif SIGN_MODE == 512
    // Pr[X = 1] = Pr[X = -1] = 0.15, Pr[X = 0] = 0.7
    unsigned int blocks_numbers = 5;
    // (N * 13 / 3 * 8) + STREAM256_BLOCKBYTES - 1) / STREAM256_BLOCKBYTES
#endif

    unsigned int ctr= 0;
    unsigned int buflen = blocks_numbers * STREAM256_BLOCKBYTES;
    uint8_t buf[blocks_numbers * STREAM256_BLOCKBYTES ];
    stream256_state state;

    stream256_init(&state, seed, nonce);
    stream256_squeezeblocks(buf, blocks_numbers, &state);

    ctr = rej_p(a->coeffs, N, buf, buflen);

    while (ctr < N) {
        stream256_squeezeblocks(buf, 1, &state);
        ctr += rej_p(a->coeffs + ctr, N - ctr, buf, STREAM256_BLOCKBYTES);
    }

}