#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <stddef.h>
#include "params.h"
// Constant-time symmetric reduction modulo M.
// Maps x to the range [-M/2, M/2] without timing-dependent branches.
static inline int16_t sym_mod(int32_t x, int32_t M) {
    int32_t val = x % M;
    
    // If val < 0, add M. Extract sign bit using bitwise operations.
    uint32_t sign = (uint32_t)val >> 31; 
    val += (int32_t)(sign * M);
    
    // If val > M / 2, it is equivalent to (M / 2) - val < 0.
    int32_t diff = (M / 2) - val;
    uint32_t diff_sign = (uint32_t)diff >> 31;
    val -= (int32_t)(diff_sign * M);
    
    return (int16_t)val;
}
typedef struct {
    int16_t coeffs[LORE_N];
} poly;

typedef struct {
    poly q_poly;
    poly t_poly;
} poly_crt;

// Move the definition of poly_crt_vec from polyvec.h to here
// to ensure it is always defined before use.
typedef struct {
    poly_crt vec[LORE_K];
} poly_crt_vec;

// Sparse polynomial structure
typedef struct {
    // Uses uint16_t to accommodate indices up to N=768 without truncation.
    uint16_t pos[LORE_K * LORE_HWT_TOTAL]; 
    int8_t   val[LORE_K * LORE_HWT_TOTAL]; 
    int16_t  n_coeffs;          
} poly_sparse;

// Function declarations
#define poly_crt_combine LORE_NAMESPACE(poly_crt_combine)
void poly_crt_combine(poly *r, const poly_crt *pc);
#define poly_crt_decompose LORE_NAMESPACE(poly_crt_decompose)
void poly_crt_decompose(poly_crt *pc, const poly *p);

#define poly_crt_add LORE_NAMESPACE(poly_crt_add)
void poly_crt_add(poly_crt *r, const poly_crt *a, const poly_crt *b);
#define poly_crt_sub LORE_NAMESPACE(poly_crt_sub)
void poly_crt_sub(poly_crt *r, const poly_crt *a, const poly_crt *b);

#define poly_getnoise_uniform LORE_NAMESPACE(poly_getnoise_uniform)
void poly_getnoise_uniform(poly *r, uint16_t t, const unsigned char *seed, unsigned char nonce);

#define poly_add_modt LORE_NAMESPACE(poly_add_modt)
void poly_add_modt(poly *r, const poly *a, const poly *b);

#define poly_add LORE_NAMESPACE(poly_add)
void poly_add(poly *r, const poly *a, const poly *b);
#define poly_from_sparse LORE_NAMESPACE(poly_from_sparse)
void poly_from_sparse(poly *r, const poly_sparse *s);
#define poly_mul_modt LORE_NAMESPACE(poly_mul_modt)
void poly_mul_modt(poly *r, const poly *a, const poly *b);
#define poly_sparse_mul_modt LORE_NAMESPACE(poly_sparse_mul_modt)
void poly_sparse_mul_modt(poly *r, const poly *a_dense, const poly_sparse *b_sparse); 

#define poly_frombytes LORE_NAMESPACE(poly_frombytes)
void poly_frombytes(poly *r, const unsigned char *a);
#define poly_tobytes LORE_NAMESPACE(poly_tobytes)
void poly_tobytes(unsigned char *r, const poly *p);

#define poly_getnoise LORE_NAMESPACE(poly_getnoise)
void poly_getnoise(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, unsigned char *seed, unsigned char nonce);

#define rej_uniform_q LORE_NAMESPACE(rej_uniform_q)
unsigned int rej_uniform_q(int16_t *r, unsigned int len, const uint8_t *buf, unsigned int buflen);
#define rej_uniform_t LORE_NAMESPACE(rej_uniform_t)
unsigned int rej_uniform_t(int16_t *r, unsigned int len, const uint8_t *buf, unsigned int buflen);

#define poly_ntt LORE_NAMESPACE(poly_ntt)
void poly_ntt(poly *r);
#define poly_invntt_tomont LORE_NAMESPACE(poly_invntt_tomont)
void poly_invntt_tomont(poly *r);
#define poly_pointwise_montgomery LORE_NAMESPACE(poly_pointwise_montgomery)
void poly_pointwise_montgomery(poly *r, const poly *a, const poly *b);

#define find_closest_r_idx LORE_NAMESPACE(find_closest_r_idx)
int16_t find_closest_r_idx(int16_t x_t);
#define pack_t_bits LORE_NAMESPACE(pack_t_bits)
void pack_t_bits(unsigned char *r, const uint8_t *t_indices);
#define unpack_t_bits LORE_NAMESPACE(unpack_t_bits)
void unpack_t_bits(uint8_t *t_indices, const unsigned char *r);

#define poly_decode_msg_crt LORE_NAMESPACE(poly_decode_msg_crt)
void poly_decode_msg_crt(unsigned char *msg, const poly_crt *r_crt);

#define poly_encode_msg LORE_NAMESPACE(poly_encode_msg)
void poly_encode_msg(poly *r, const unsigned char *msg);
#define poly_decode_msg LORE_NAMESPACE(poly_decode_msg)
void poly_decode_msg(unsigned char *msg, const poly *r);

#define poly_add_scaled_msg LORE_NAMESPACE(poly_add_scaled_msg)
void poly_add_scaled_msg(poly_crt *r, const poly *msg_poly);

#define lore_poly_pack_q_split LORE_NAMESPACE(lore_poly_pack_q_split)
size_t lore_poly_pack_q_split(unsigned char main_buf[LORE_N], unsigned char *overflow_buf, const poly *p);
#define lore_poly_unpack_q_split LORE_NAMESPACE(lore_poly_unpack_q_split)
void lore_poly_unpack_q_split(poly *p, const unsigned char main_buf[LORE_N], const unsigned char *overflow_buf, int *overflow_bit_pos);
#define print_poly LORE_NAMESPACE(print_poly)
void print_poly(const char* name, const poly* p);

#define print_poly_crt LORE_NAMESPACE(print_poly_crt)
void print_poly_crt(const char* name, const poly_crt* p_crt);

void poly_mul_schoolbook_q(poly *r, const poly *a, const poly *b);
#endif