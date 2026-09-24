#include "polymat.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include <stdint.h>

/*************************************************
 * Name:        polymat_expand
 *
 * Description: Implementation of ExpandA. Generates matrix A with uniformly
 *              random coefficients a_{i,j} by performing rejection
 *              sampling on the output stream of SHAKE128(seed|j|i)
 *              or AES256CTR(seed,j|i).
 *
 * Arguments:   - polyvecl mat[K]: output matrix k \times l
 *              - const uint8_t seed[]: byte array containing seed seed
 **************************************************/
void polymatkl_expand(polyvecl mat[K], const uint8_t seed[SEEDBYTES]) {
    unsigned int i, j;
    unsigned int batch_count = 0;

    poly *p[4];
    uint16_t n[4];

    for (i = 0; i < K; ++i) {
        for (j = 0; j < L - 1; ++j) {
            p[batch_count] = &mat[i].vec[j + 1];
            n[batch_count] = (i << 8) + j;
            batch_count++;

            if (batch_count == 4) {
                poly_uniform_4x(p[0], p[1], p[2], p[3], 
                                seed, n[0], n[1], n[2], n[3]);
                batch_count = 0; 
            }
        }
    }

    for (int k = 0; k < batch_count; ++k) {
        poly_uniform(p[k], seed, n[k]);
    }
}

/*************************************************
 * Name:        polymat_expand
 *
 * Description: Implementation of ExpandA. Generates matrix A with uniformly
 *              random coefficients a_{i,j} by performing rejection
 *              sampling on the output stream of SHAKE128(seed|j|i)
 *              or AES256CTR(seed,j|i).
 *
 * Arguments:   - polyvecm mat[K]: output matrix k \times l-1
 *              - const uint8_t seed[]: byte array containing seed seed
 **************************************************/
void polymatkl_1_expand(polyvecl_1 mat[K], const uint8_t seed[SEEDBYTES]) {
    unsigned int i, j;
    unsigned int batch_count = 0;

    poly *p[4];
    uint16_t n[4];

    for (i = 0; i < K; ++i) {
        for (j = 0; j < L - 1; ++j) {
            p[batch_count] = &mat[i].vec[j];  
            n[batch_count] = (i << 8) + j;
            batch_count++;

            if (batch_count == 4) {
                poly_uniform_4x(p[0], p[1], p[2], p[3], 
                                seed, n[0], n[1], n[2], n[3]);
                batch_count = 0; 
            }
        }
    }

    for (int k = 0; k < batch_count; ++k) {
        poly_uniform(p[k], seed, n[k]);
    }
}

/*************************************************
 * Name:        polymatkl_1_pointwise_montgomery
 *
 * Description: Pointwise multiplication of matrix A and vector v in
 *              NTT domain, where A has dimension k x (l-1)
 *
 * Arguments:   - polyveck *t: output vector of polynomials of length K
 *              - const polyvecl_1 mat[K]: input matrix k x (l-1)
 *              - const polyvecl_1 *v: input vector of polynomials of length l-1
 **************************************************/
void polymatkl_1_pointwise_montgomery(polyveck *r, const polyvecl_1 mat[K], const polyvecl_1 *a) {
    unsigned int i;

    for (i = 0; i < K; ++i) {
        polyvecl_1_basemul_acc_montgomery(&r->vec[i], &mat[i], a);
    }
}

/*************************************************
 * Name:        polymatkl_pointwise_montgomery
 *
 * Description: Pointwise multiplication of matrix A and vector v in
 *              NTT domain, where A has dimension k x l
 *
 * Arguments:   - polyveck *r: output vector of polynomials of length K
 *              - const polyvecl mat[K]: input matrix k x l
 *              - const polyvecl *a: input vector of polynomials of length l
 **************************************************/
void polymatkl_pointwise_montgomery(polyveck *r, const polyvecl mat[K], const polyvecl *a) {
    unsigned int i;

    for (i = 0; i < K; ++i) {
        polyvecl_basemul_acc_montgomery(&r->vec[i], &mat[i], a);
    }
}

/*************************************************
 * Name:        polymatk1_pointwise_montgomery
 *
 * Description: Pointwise multiplication of matrix A and vector v in
 *              NTT domain, where A has dimension k x 1
 *
 * Arguments:   - polyveck *r: output vector of polynomials of length K
 *              - const polyveck *a: input matrix k x 1
 *              - const poly *b: input vector of polynomials of length 1
 **************************************************/
void polymatk1_pointwise_montgomery(polyveck *r, const polyveck *a, const poly *b) {
    unsigned int i;

    for (i = 0; i < K; ++i) {
        poly_basemul_montgomery(&r->vec[i], &a->vec[i], b);
    }
}