/// @file blas_matrix_sse.c
/// @brief Implementations for blas_matrix_sse.h
///

#include "gf16.h"

#include "config.h"

#include "gf16_sse.h"

#include "blas_sse.h"

#include "blas_comm.h"

#include "blas_matrix_sse.h"


#include <emmintrin.h>
#include <tmmintrin.h>
#include "string.h"


#include "params.h"  // for macro _USE_GF16


#if defined(_USE_GF16)

static
void gf16mat_blockmat_madd_sse( __m128i *dest, const uint8_t *org_mat, unsigned mat_vec_byte, unsigned blk_st_idx, unsigned blk_vec_byte,
                                const __m128i *multab_vec_ele, unsigned n_vec_ele ) {
    unsigned n_full_xmm = blk_vec_byte >> 4;
    unsigned n_rem_byte = blk_vec_byte & 15;
    __m128i mask_f = _mm_set1_epi8(0xf);

    for (unsigned i = 0; i < n_vec_ele; i++ ) {
        __m128i tab_l = multab_vec_ele[0];
        __m128i tab_h = multab_vec_ele[1];
        multab_vec_ele += 2;

        for (unsigned j = 0; j < n_full_xmm; j++) {
            __m128i mj = _mm_loadu_si128( (__m128i *)(org_mat + blk_st_idx + j * 16) );
            dest[j] ^= linear_transform_8x8_128b( tab_l, tab_h, mj, mask_f );
        }
        if ( n_rem_byte ) {
            __m128i mj = _load_xmm( org_mat + blk_st_idx + (n_full_xmm * 16), n_rem_byte );
            dest[n_full_xmm] ^= linear_transform_8x8_128b( tab_l, tab_h, mj, mask_f );
        }

        org_mat += mat_vec_byte;
    }
}


void gf16mat_prod_multab_sse( uint8_t *c, const uint8_t *matA, unsigned matA_vec_byte, unsigned matA_n_vec, const uint8_t *multab_b ) {
    const __m128i *multabs = (const __m128i *) multab_b;
    gf256v_set_zero( c, matA_vec_byte );

    __m128i blockmat_vec[8];

    while (matA_n_vec) {
        unsigned n_ele = (matA_n_vec >= 32) ? 32 : matA_n_vec;

        unsigned vec_len_to_go = matA_vec_byte;
        while ( vec_len_to_go ) {
            unsigned block_len = (vec_len_to_go >= 8 * 16) ? 8 * 16 : vec_len_to_go;
            unsigned block_st_idx = matA_vec_byte - vec_len_to_go;

            loadu_xmm( blockmat_vec, c + block_st_idx, block_len );
            gf16mat_blockmat_madd_sse( blockmat_vec, matA, matA_vec_byte, block_st_idx, block_len, multabs, n_ele );
            storeu_xmm( c + block_st_idx, block_len, blockmat_vec );

            vec_len_to_go -= block_len;
        }

        matA_n_vec -= n_ele;
        matA += n_ele * matA_vec_byte;
        multabs += 2 * n_ele;
    }
}



void gf16mat_prod_sse( uint8_t *c, const uint8_t *matA, unsigned matA_vec_byte, unsigned matA_n_vec, const uint8_t *b ) {
    __m128i multabs[32 * 2];
    gf256v_set_zero( c, matA_vec_byte );

    __m128i blockmat_vec[8];

    while (matA_n_vec) {

        __m128i x[2];
        unsigned n_ele;
        if (matA_n_vec >= 32) {
            n_ele = 32;
            x[0] = _mm_loadu_si128( (__m128i *)b );
        } else {
            n_ele = matA_n_vec;
            x[0] = _load_xmm( b, (n_ele + 1) >> 1 );
        }
        gf16v_split_16to32_sse( x, x[0] );
        if (n_ele <= 16) {
            gf16v_generate_multab_16_sse( multabs, x[0], n_ele );
        } else {
            gf16v_generate_multab_16_sse( multabs, x[0], 16 );
            gf16v_generate_multab_16_sse( multabs + 32, x[1], n_ele - 16 );
        }

        unsigned vec_len_to_go = matA_vec_byte;
        while ( vec_len_to_go ) {
            unsigned block_len = (vec_len_to_go >= 8 * 16) ? 8 * 16 : vec_len_to_go;
            unsigned block_st_idx = matA_vec_byte - vec_len_to_go;

            loadu_xmm( blockmat_vec, c + block_st_idx, block_len );
            gf16mat_blockmat_madd_sse( blockmat_vec, matA, matA_vec_byte, block_st_idx, block_len, multabs, n_ele );
            storeu_xmm( c + block_st_idx, block_len, blockmat_vec );

            vec_len_to_go -= block_len;
        }

        matA_n_vec -= n_ele;
        b += (n_ele >> 1);
        matA += n_ele * matA_vec_byte;
    }
}


#else // defined(_USE_GF16)

///////////////////////////////  GF( 256 ) ////////////////////////////////////////////////////


static
void gf256mat_blockmat_madd_sse( __m128i *dest, const uint8_t *org_mat, unsigned mat_vec_byte, unsigned blk_st_idx, unsigned blk_vec_byte,
                                 const __m128i *multab_vec_ele, unsigned n_vec_ele ) {
    unsigned n_full_xmm = blk_vec_byte >> 4;
    unsigned n_rem_byte = blk_vec_byte & 15;
    __m128i mask_f = _mm_set1_epi8(0xf);

    for (unsigned i = 0; i < n_vec_ele; i++ ) {
        __m128i tab_l = multab_vec_ele[0];
        __m128i tab_h = multab_vec_ele[1];
        multab_vec_ele += 2;

        for (unsigned j = 0; j < n_full_xmm; j++) {
            __m128i mj = _mm_loadu_si128( (__m128i *)(org_mat + blk_st_idx + j * 16) );
            dest[j] ^= linear_transform_8x8_128b( tab_l, tab_h, mj, mask_f );
        }
        if ( n_rem_byte ) {
            __m128i mj = _load_xmm( org_mat + blk_st_idx + (n_full_xmm * 16), n_rem_byte );
            dest[n_full_xmm] ^= linear_transform_8x8_128b( tab_l, tab_h, mj, mask_f );
        }

        org_mat += mat_vec_byte;
    }
}


void gf256mat_prod_multab_sse( uint8_t *c, const uint8_t *matA, unsigned matA_vec_byte, unsigned matA_n_vec, const uint8_t *multab_b ) {
    const __m128i *multabs = (const __m128i *)multab_b;
    gf256v_set_zero( c, matA_vec_byte );

    __m128i blockmat_vec[8];

    while (matA_n_vec) {
        unsigned n_ele = (matA_n_vec >= 16) ? 16 : matA_n_vec;
        unsigned vec_len_to_go = matA_vec_byte;
        while ( vec_len_to_go ) {
            unsigned block_len = (vec_len_to_go >= 8 * 16) ? 8 * 16 : vec_len_to_go;
            unsigned block_st_idx = matA_vec_byte - vec_len_to_go;

            loadu_xmm( blockmat_vec, c + block_st_idx, block_len );
            gf256mat_blockmat_madd_sse( blockmat_vec, matA, matA_vec_byte, block_st_idx, block_len, multabs, n_ele );
            storeu_xmm( c + block_st_idx, block_len, blockmat_vec );

            vec_len_to_go -= block_len;
        }

        matA_n_vec -= n_ele;
        matA += n_ele * matA_vec_byte;
        multabs += n_ele * 2;
    }
}


void gf256mat_prod_sse( uint8_t *c, const uint8_t *matA, unsigned matA_vec_byte, unsigned matA_n_vec, const uint8_t *b ) {

    __m128i multabs[16 * 2];
    gf256v_set_zero( c, matA_vec_byte );

    __m128i blockmat_vec[8];

    while (matA_n_vec) {

        unsigned n_ele = (matA_n_vec >= 16) ? 16 : matA_n_vec;
        __m128i x = (matA_n_vec >= 16) ? _mm_loadu_si128( (const __m128i *)b ) : _load_xmm( b, matA_n_vec );
        gf256v_generate_multab_16_sse( multabs, x, n_ele );

        unsigned vec_len_to_go = matA_vec_byte;
        while ( vec_len_to_go ) {
            unsigned block_len = (vec_len_to_go >= 8 * 16) ? 8 * 16 : vec_len_to_go;
            unsigned block_st_idx = matA_vec_byte - vec_len_to_go;

            loadu_xmm( blockmat_vec, c + block_st_idx, block_len );
            gf256mat_blockmat_madd_sse( blockmat_vec, matA, matA_vec_byte, block_st_idx, block_len, multabs, n_ele );
            storeu_xmm( c + block_st_idx, block_len, blockmat_vec );

            vec_len_to_go -= block_len;
        }

        matA_n_vec -= n_ele;
        b += n_ele;
        matA += n_ele * matA_vec_byte;
    }
}


#endif // defined(_USE_GF16)

