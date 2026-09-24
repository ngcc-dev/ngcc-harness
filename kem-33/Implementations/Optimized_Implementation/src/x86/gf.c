/**
 * @file gf.c
 * @brief Galois field implementation with multiplication using the pclmulqdq instruction
 */

#include "gf.h"
#include <stdint.h>
#include "parameters.h"

static uint16_t gf_reduce(uint16_t x);

/**
 * @brief Generates exp and log lookup tables of GF(2^m).
 *
 * @note   this function is not used in the code; it was used to generate
 *         the lookup table for GF(2^8).
 *
 * The logarithm of 0 is defined as 2^PARAM_M by convention. <br>
 * The last two elements of the exp table are needed by the gf_mul function from gf_lutmul.c
 * (for example if both elements to multiply are zero).
 * @param[out] exp Array of size 2^PARAM_M + 2 receiving the powers of the primitive element
 * @param[out] log Array of size 2^PARAM_M receiving the logarithms of the elements of GF(2^m)
 * @param[in] m Parameter of Galois field GF(2^m)
 */
void gf_generate(uint16_t *exp, uint16_t *log, const int16_t m) {
    uint16_t elt = 1;
    uint16_t alpha = 2;  // primitive element of GF(2^PARAM_M)
    uint16_t gf_poly = PARAM_GF_POLY;

    for (size_t i = 0; i < (1U << m) - 1; ++i) {
        exp[i] = elt;
        log[elt] = i;

        elt *= alpha;
        if (elt >= 1 << m)
            elt ^= gf_poly;
    }

    exp[(1 << m) - 1] = 1;
    exp[1 << m] = 2;
    exp[(1 << m) + 1] = 4;
    log[0] = 0;  // by convention
}

/**
 * @brief Feedback bit positions used for modular reduction by PARAM_GF_POLY = 0x11D.
 *
 * These values are derived from the binary form of the polynomial:
 *     0x11D = 0b100011101 → bits set at positions: 8, 4, 3, 1, 0
 *
 * To reduce a polynomial modulo this irreducible polynomial:
 * - Bit 8 (the leading term) is handled via shifting: mod = x >> PARAM_M
 * - Bit 0 (constant term) is handled by the initial XOR
 *
 * The remaining set bits at positions 4, 3, and 2 define where the shifted
 * high bits (mod) must be XORed back into the result. These represent the
 * feedback positions used during reduction.
 */
static const uint8_t gf_reduction_taps[] = {4, 3, 2};

/**
 * @brief Reduce a polynomial modulo PARAM_GF_POLY in GF(2^8).
 *
 * This function performs modular reduction of a 16-bit polynomial `x`
 * by the irreducible polynomial PARAM_GF_POLY = 0x11D
 * (i.e., x⁸ + x⁴ + x³ + x + 1), used in GF(2^8).
 *
 * It assumes the input polynomial has degree ≤ 14 and uses a fixed
 * number of reduction steps and fixed feedback tap positions
 * ({4, 3, 2}) to produce a result of degree < 8.
 *
 * @param x 16-bit input polynomial to reduce (deg(x) ≤ 14)
 * @return Reduced 8-bit polynomial modulo PARAM_GF_POLY (deg(x) < 8)
 */
uint16_t gf_reduce(uint16_t x) {
    uint64_t mod;
    const int reduction_steps = 2;            // For deg(x) = 2 * (PARAM_M - 1) = 14, reduce twice to bring degree < 8
    const size_t gf_reduction_tap_count = 3;  // Number of feedback positions

    for (int i = 0; i < reduction_steps; ++i) {
        mod = x >> PARAM_M;       // Extract upper bits
        x &= (1 << PARAM_M) - 1;  // Keep lower bits
        x ^= mod;                 // Pre-XOR with no shift

        uint16_t z1 = 0;
        for (size_t j = gf_reduction_tap_count; j; --j) {
            uint16_t z2 = gf_reduction_taps[j - 1];
            uint16_t dist = z2 - z1;
            mod <<= dist;
            x ^= mod;
            z1 = z2;
        }
    }

    return x;
}

/**
 * Multiplies two elements of GF(2^8).
 * @returns the product a*b
 * @param[in] a Element of GF(2^8)
 * @param[in] b Element of GF(2^8)
 */
uint16_t gf_mul(uint16_t a, uint16_t b) {
    __m128i va = _mm_cvtsi32_si128(a);
    __m128i vb = _mm_cvtsi32_si128(b);
    __m128i vab = _mm_clmulepi64_si128(va, vb, 0);
    uint32_t ab = _mm_cvtsi128_si32(vab);

    return gf_reduce(ab);
}

/**
 *  Compute 16 products in GF(2^8).
 *  @returns the product (a0b0,a1b1,...,a15b15) , ai,bi in GF(2^8)
 *  @param[in] a 256-bit register where a0,..,a15 are stored as 16 bit integers
 *  @param[in] b 256-bit register where b0,..,b15 are stored as 16 bit integer
 *
 */
__m256i gf_mul_vect(__m256i a, __m256i b) {
    __m128i al = _mm256_extractf128_si256(a, 0);
    __m128i ah = _mm256_extractf128_si256(a, 1);
    __m128i bl = _mm256_extractf128_si256(b, 0);
    __m128i bh = _mm256_extractf128_si256(b, 1);

    __m128i abl0 = _mm_clmulepi64_si128(al & maskl, bl & maskl, 0x0);
    abl0 &= middlemaskl;
    abl0 ^= (_mm_clmulepi64_si128(al & maskh, bl & maskh, 0x0) & middlemaskh);

    __m128i abh0 = _mm_clmulepi64_si128(al & maskl, bl & maskl, 0x11);
    abh0 &= middlemaskl;
    abh0 ^= (_mm_clmulepi64_si128(al & maskh, bl & maskh, 0x11) & middlemaskh);

    abl0 = _mm_shuffle_epi8(abl0, indexl);
    abl0 ^= _mm_shuffle_epi8(abh0, indexh);

    __m128i abl1 = _mm_clmulepi64_si128(ah & maskl, bh & maskl, 0x0);
    abl1 &= middlemaskl;
    abl1 ^= (_mm_clmulepi64_si128(ah & maskh, bh & maskh, 0x0) & middlemaskh);

    __m128i abh1 = _mm_clmulepi64_si128(ah & maskl, bh & maskl, 0x11);
    abh1 &= middlemaskl;
    abh1 ^= (_mm_clmulepi64_si128(ah & maskh, bh & maskh, 0x11) & middlemaskh);

    abl1 = _mm_shuffle_epi8(abl1, indexl);
    abl1 ^= _mm_shuffle_epi8(abh1, indexh);

    __m256i ret = _mm256_set_m128i(abl1, abl0);

    __m256i aux = mr0;

    for (int32_t i = 0; i < 7; i++) {
        ret ^= red[i] & _mm256_cmpeq_epi16((ret & aux), aux);
        aux = aux << 1;
    }

    ret &= lastMask;
    return ret;
}

/**
 * Squares an element of GF(2^8).
 * @returns a^2
 * @param[in] a Element of GF(2^8)
 */
uint16_t gf_square(uint16_t a) {
    uint32_t b = a;
    uint32_t s = b & 1;
    for (size_t i = 1; i < PARAM_M; ++i) {
        b <<= 1;
        s ^= b & (1 << 2 * i);
    }

    return gf_reduce(s);
}

/**
 * Computes the inverse of an element of GF(2^8),
 * using the addition chain 1 2 3 4 7 11 15 30 60 120 127 254
 * @returns the inverse of a
 * @param[in] a Element of GF(2^8)
 */
uint16_t gf_inverse(uint16_t a) {
    uint16_t inv = a;
    uint16_t tmp1, tmp2;

    inv = gf_square(a);       /* a^2 */
    tmp1 = gf_mul(inv, a);    /* a^3 */
    inv = gf_square(inv);     /* a^4 */
    tmp2 = gf_mul(inv, tmp1); /* a^7 */
    tmp1 = gf_mul(inv, tmp2); /* a^11 */
    inv = gf_mul(tmp1, inv);  /* a^15 */
    inv = gf_square(inv);     /* a^30 */
    inv = gf_square(inv);     /* a^60 */
    inv = gf_square(inv);     /* a^120 */
    inv = gf_mul(inv, tmp2);  /* a^127 */
    inv = gf_square(inv);     /* a^254 */
    return inv;
}

static inline __m256i linear_transform_8x8_256b( __m256i tab_l, __m256i tab_h, __m256i v, __m256i mask_f ) {
    return _mm256_shuffle_epi8(tab_l, v & mask_f)^_mm256_shuffle_epi8(tab_h, _mm256_srli_epi16(v, 4)&mask_f);
}

static inline
void linearmap_8x8_ymm_store( const uint8_t *a, __m256i ml, __m256i mh, __m256i mask, unsigned _num_byte , uint8_t *dest) {
    unsigned n_32 = _num_byte >> 5;
    unsigned rem = _num_byte & 31;
    if ( rem ) {
        if ( n_32 ) {
            __m256i inp = _mm256_loadu_si256( (__m256i *)a );
            __m256i in1 = _mm256_loadu_si256( (__m256i *)(a+rem) );
            __m256i r0 = linear_transform_8x8_256b( ml, mh, inp, mask );
            _mm256_storeu_si256( (__m256i *)dest, r0 );
            _mm256_storeu_si256( (__m256i *)(dest+rem), in1 );
        } 
        a += rem;
        dest += rem;
    }
    while (n_32--) {
        __m256i inp = _mm256_loadu_si256( (__m256i *)a );
        __m256i r0 = linear_transform_8x8_256b( ml, mh, inp, mask );
        _mm256_storeu_si256( (__m256i *)dest, r0 );
        a += 32;
        dest += 32;
    }
}
const unsigned char __gf256_mulbase[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0,
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0, 0x1d, 0x3d, 0x5d, 0x7d, 0x9d, 0xbd, 0xdd, 0xfd,
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x00, 0x40, 0x80, 0xc0, 0x1d, 0x5d, 0x9d, 0xdd, 0x3a, 0x7a, 0xba, 0xfa, 0x27, 0x67, 0xa7, 0xe7,
    0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x00, 0x80, 0x1d, 0x9d, 0x3a, 0xba, 0x27, 0xa7, 0x74, 0xf4, 0x69, 0xe9, 0x4e, 0xce, 0x53, 0xd3,
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x00, 0x1d, 0x3a, 0x27, 0x74, 0x69, 0x4e, 0x53, 0xe8, 0xf5, 0xd2, 0xcf, 0x9c, 0x81, 0xa6, 0xbb,
    0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0, 0x1d, 0x3d, 0x5d, 0x7d, 0x9d, 0xbd, 0xdd, 0xfd, 0x00, 0x3a, 0x74, 0x4e, 0xe8, 0xd2, 0x9c, 0xa6, 0xcd, 0xf7, 0xb9, 0x83, 0x25, 0x1f, 0x51, 0x6b,
    0x00, 0x40, 0x80, 0xc0, 0x1d, 0x5d, 0x9d, 0xdd, 0x3a, 0x7a, 0xba, 0xfa, 0x27, 0x67, 0xa7, 0xe7, 0x00, 0x74, 0xe8, 0x9c, 0xcd, 0xb9, 0x25, 0x51, 0x87, 0xf3, 0x6f, 0x1b, 0x4a, 0x3e, 0xa2, 0xd6,
    0x00, 0x80, 0x1d, 0x9d, 0x3a, 0xba, 0x27, 0xa7, 0x74, 0xf4, 0x69, 0xe9, 0x4e, 0xce, 0x53, 0xd3, 0x00, 0xe8, 0xcd, 0x25, 0x87, 0x6f, 0x4a, 0xa2, 0x13, 0xfb, 0xde, 0x36, 0x94, 0x7c, 0x59, 0xb1
};

static inline __m256i tbl32_gf256_multab( uint8_t b ) {
    __m256i bx = _mm256_set1_epi16( b );
    __m256i b1 = _mm256_srli_epi16( bx, 1 );

    __m256i tab0 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 0));
    __m256i tab1 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 1));
    __m256i tab2 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 2));
    __m256i tab3 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 3));
    __m256i tab4 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 4));
    __m256i tab5 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 5));
    __m256i tab6 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 6));
    __m256i tab7 = _mm256_load_si256((__m256i const *) (__gf256_mulbase + 32 * 7));

    __m256i mask_1  = _mm256_set1_epi16(1);
    __m256i mask_4  = _mm256_set1_epi16(4);
    __m256i mask_16 = _mm256_set1_epi16(16);
    __m256i mask_64 = _mm256_set1_epi16(64);
    __m256i mask_0  = _mm256_setzero_si256();

    return ( tab0 & _mm256_cmpgt_epi16( bx & mask_1, mask_0) )
           ^ ( tab1 & _mm256_cmpgt_epi16( b1 & mask_1, mask_0) )
           ^ ( tab2 & _mm256_cmpgt_epi16( bx & mask_4, mask_0) )
           ^ ( tab3 & _mm256_cmpgt_epi16( b1 & mask_4, mask_0) )
           ^ ( tab4 & _mm256_cmpgt_epi16( bx & mask_16, mask_0) )
           ^ ( tab5 & _mm256_cmpgt_epi16( b1 & mask_16, mask_0) )
           ^ ( tab6 & _mm256_cmpgt_epi16( bx & mask_64, mask_0) )
           ^ ( tab7 & _mm256_cmpgt_epi16( b1 & mask_64, mask_0) );
}

void gf256v_mul_scalar_avx2( const uint8_t *a, uint8_t _b, unsigned _num_byte , uint8_t * dest) {
    __m256i m_tab = tbl32_gf256_multab( _b );
    __m256i ml = _mm256_permute2x128_si256( m_tab, m_tab, 0 );
    __m256i mh = _mm256_permute2x128_si256( m_tab, m_tab, 0x11 );
    __m256i mask = _mm256_set1_epi8(0xf);

    linearmap_8x8_ymm_store( a, ml, mh, mask, _num_byte , dest);
}

const unsigned char __gf256_mulbase_avx[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0,
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x00, 0x1d, 0x3a, 0x27, 0x74, 0x69, 0x4e, 0x53, 0xe8, 0xf5, 0xd2, 0xcf, 0x9c, 0x81, 0xa6, 0xbb,
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0, 0x1d, 0x3d, 0x5d, 0x7d, 0x9d, 0xbd, 0xdd, 0xfd,
    0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0, 0x1d, 0x3d, 0x5d, 0x7d, 0x9d, 0xbd, 0xdd, 0xfd, 0x00, 0x3a, 0x74, 0x4e, 0xe8, 0xd2, 0x9c, 0xa6, 0xcd, 0xf7, 0xb9, 0x83, 0x25, 0x1f, 0x51, 0x6b,
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x00, 0x40, 0x80, 0xc0, 0x1d, 0x5d, 0x9d, 0xdd, 0x3a, 0x7a, 0xba, 0xfa, 0x27, 0x67, 0xa7, 0xe7,
    0x00, 0x40, 0x80, 0xc0, 0x1d, 0x5d, 0x9d, 0xdd, 0x3a, 0x7a, 0xba, 0xfa, 0x27, 0x67, 0xa7, 0xe7, 0x00, 0x74, 0xe8, 0x9c, 0xcd, 0xb9, 0x25, 0x51, 0x87, 0xf3, 0x6f, 0x1b, 0x4a, 0x3e, 0xa2, 0xd6,
    0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x00, 0x80, 0x1d, 0x9d, 0x3a, 0xba, 0x27, 0xa7, 0x74, 0xf4, 0x69, 0xe9, 0x4e, 0xce, 0x53, 0xd3,
    0x00, 0x80, 0x1d, 0x9d, 0x3a, 0xba, 0x27, 0xa7, 0x74, 0xf4, 0x69, 0xe9, 0x4e, 0xce, 0x53, 0xd3, 0x00, 0xe8, 0xcd, 0x25, 0x87, 0x6f, 0x4a, 0xa2, 0x13, 0xfb, 0xde, 0x36, 0x94, 0x7c, 0x59, 0xb1
};

static inline
__m128i _load_xmm( const uint8_t *a, unsigned _num_byte ) {
    uint8_t temp[32];
    //assert( 16 >= _num_byte );
    //assert( 0 < _num_byte );
    for (unsigned i = 0; i < _num_byte; i++) {
        temp[i] = a[i];
    }
    return _mm_load_si128((__m128i *)temp);
}

static inline
void _store_xmm( uint8_t *a, unsigned _num_byte, __m128i data ) {
    uint8_t temp[32];
    //assert( 16 >= _num_byte );
    //assert( 0 < _num_byte );
    _mm_store_si128((__m128i *)temp, data);
    for (unsigned i = 0; i < _num_byte; i++) {
        a[i] = temp[i];
    }
}
static inline
void gf256v_generate_multab_16_avx2( __m256i *multabs, __m128i a, unsigned len ) {
    __m256i tab0 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 0));
    __m256i tab1 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 1));
    __m256i tab2 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 2));
    __m256i tab3 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 3));
    __m256i tab4 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 4));
    __m256i tab5 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 5));
    __m256i tab6 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 6));
    __m256i tab7 = _mm256_load_si256((__m256i const *) (__gf256_mulbase_avx + 32 * 7));
    __m256i mask_f = _mm256_set1_epi8(0xf);

    __m256i aa = _mm256_setr_m128i( a, a );
    __m256i a_lo = aa & mask_f;
    __m256i a_hi = _mm256_srli_epi16(aa, 4)&mask_f;
    __m256i bx1 =  _mm256_shuffle_epi8( tab0, a_lo) ^ _mm256_shuffle_epi8( tab1, a_hi);
    __m256i bx2 =  _mm256_shuffle_epi8( tab2, a_lo) ^ _mm256_shuffle_epi8( tab3, a_hi);
    __m256i bx4 =  _mm256_shuffle_epi8( tab4, a_lo) ^ _mm256_shuffle_epi8( tab5, a_hi);
    __m256i bx8 =  _mm256_shuffle_epi8( tab6, a_lo) ^ _mm256_shuffle_epi8( tab7, a_hi);

    __m256i broadcast_x1 = _mm256_set_epi8( 0, -16, 0, -16, 0, -16, 0, -16,  0, -16, 0, -16, 0, -16, 0, -16,  0, -16, 0, -16, 0, -16, 0, -16,  0, -16, 0, -16, 0, -16, 0, -16 );
    __m256i broadcast_x2 = _mm256_set_epi8( 0, 0, -16, -16, 0, 0, -16, -16,  0, 0, -16, -16, 0, 0, -16, -16,  0, 0, -16, -16, 0, 0, -16, -16,  0, 0, -16, -16, 0, 0, -16, -16 );
    __m256i broadcast_x4 = _mm256_set_epi8( 0, 0, 0, 0, -16, -16, -16, -16,  0, 0, 0, 0, -16, -16, -16, -16,  0, 0, 0, 0, -16, -16, -16, -16,  0, 0, 0, 0, -16, -16, -16, -16 );
    __m256i broadcast_x8 = _mm256_set_epi8( 0, 0, 0, 0, 0, 0, 0, 0,  -16, -16, -16, -16, -16, -16, -16, -16,  0, 0, 0, 0, 0, 0, 0, 0,  -16, -16, -16, -16, -16, -16, -16, -16 );
    __m256i broadcast_x1_2 = _mm256_set_epi8( 1, -16, 1, -16, 1, -16, 1, -16,  1, -16, 1, -16, 1, -16, 1, -16,  1, -16, 1, -16, 1, -16, 1, -16,  1, -16, 1, -16, 1, -16, 1, -16 );
    __m256i broadcast_x2_2 = _mm256_set_epi8( 1, 1, -16, -16, 1, 1, -16, -16,  1, 1, -16, -16, 1, 1, -16, -16,  1, 1, -16, -16, 1, 1, -16, -16,  1, 1, -16, -16, 1, 1, -16, -16 );
    __m256i broadcast_x4_2 = _mm256_set_epi8( 1, 1, 1, 1, -16, -16, -16, -16,  1, 1, 1, 1, -16, -16, -16, -16,  1, 1, 1, 1, -16, -16, -16, -16,  1, 1, 1, 1, -16, -16, -16, -16 );
    __m256i broadcast_x8_2 = _mm256_set_epi8( 1, 1, 1, 1, 1, 1, 1, 1,  -16, -16, -16, -16, -16, -16, -16, -16,  1, 1, 1, 1, 1, 1, 1, 1,  -16, -16, -16, -16, -16, -16, -16, -16 );

    if ( 0 == (len & 1) ) {
        multabs[0] =  _mm256_shuffle_epi8(bx1, broadcast_x1) ^ _mm256_shuffle_epi8(bx2, broadcast_x2)
                      ^ _mm256_shuffle_epi8(bx4, broadcast_x4) ^ _mm256_shuffle_epi8(bx8, broadcast_x8);
        multabs[1] =  _mm256_shuffle_epi8(bx1, broadcast_x1_2) ^ _mm256_shuffle_epi8(bx2, broadcast_x2_2)
                      ^ _mm256_shuffle_epi8(bx4, broadcast_x4_2) ^ _mm256_shuffle_epi8(bx8, broadcast_x8_2);

        for (unsigned i = 2; i < len; i += 2) {
            bx1 = _mm256_srli_si256( bx1, 2 );
            bx2 = _mm256_srli_si256( bx2, 2 );
            bx4 = _mm256_srli_si256( bx4, 2 );
            bx8 = _mm256_srli_si256( bx8, 2 );
            multabs[i] =  _mm256_shuffle_epi8(bx1, broadcast_x1) ^ _mm256_shuffle_epi8(bx2, broadcast_x2)
                          ^ _mm256_shuffle_epi8(bx4, broadcast_x4) ^ _mm256_shuffle_epi8(bx8, broadcast_x8);
            multabs[i + 1] =  _mm256_shuffle_epi8(bx1, broadcast_x1_2) ^ _mm256_shuffle_epi8(bx2, broadcast_x2_2)
                              ^ _mm256_shuffle_epi8(bx4, broadcast_x4_2) ^ _mm256_shuffle_epi8(bx8, broadcast_x8_2);
        }
    } else {
        multabs[0] =  _mm256_shuffle_epi8(bx1, broadcast_x1) ^ _mm256_shuffle_epi8(bx2, broadcast_x2)
                      ^ _mm256_shuffle_epi8(bx4, broadcast_x4) ^ _mm256_shuffle_epi8(bx8, broadcast_x8);

        for (unsigned i = 1; i < len; i++) {
            bx1 = _mm256_srli_si256( bx1, 1 );
            bx2 = _mm256_srli_si256( bx2, 1 );
            bx4 = _mm256_srli_si256( bx4, 1 );
            bx8 = _mm256_srli_si256( bx8, 1 );

            multabs[i] =  _mm256_shuffle_epi8(bx1, broadcast_x1) ^ _mm256_shuffle_epi8(bx2, broadcast_x2)
                          ^ _mm256_shuffle_epi8(bx4, broadcast_x4) ^ _mm256_shuffle_epi8(bx8, broadcast_x8);
        }
    }
}

void gf256v_generate_multabs_avx2( uint8_t *multabs, const uint8_t *v, unsigned n_ele ) {
    __m128i x;
    while (n_ele >= 16) {
        x = _mm_loadu_si128( (__m128i *)v );
        gf256v_generate_multab_16_avx2( (__m256i *)multabs, x, 16 );
        multabs += 16 * 32;
        v += 16;
        n_ele -= 16;
    }
    if (n_ele) {
        x = _load_xmm( v, n_ele );
        gf256v_generate_multab_16_avx2( (__m256i *)multabs, x, n_ele );
    }
}

__m256i linearmap_8x8_ymm( __m256i inp , __m256i ml , __m256i mh , __m256i mask) {
    __m256i r0 = linear_transform_8x8_256b( ml , mh , inp , mask );
    return r0;
}
