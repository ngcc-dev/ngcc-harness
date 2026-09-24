/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "polyvec.h"
#include "drng.h"
#include "packing.h"
#include "sample.h"
#include "sign.h"
#include "symmetric.h"

extern DRNG_ctx drng_algorithm;

static inline void secure_zero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) *p++ = 0;
}

int bit_sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
                   unsigned char *sk, unsigned long long *sk_len_bytes) {
    unsigned char seed_A[BIT_SEEDBYTES];
    unsigned char secret_seed[BIT_SEEDBYTES];
    unsigned char tr[BIT_TRBYTES];
    poly_matrix_ntt A_0_ntt;
    polyvecl_ntt s_0_ntt;
    polyvecl s_0;
    polyveck e, b, b1, b0;
    
    uint16_t nonce = 0;

    if (get_random_number(&drng_algorithm, seed_A,      BIT_SEEDBYTES*8ULL) != 0) {
        return -1;
    }
    if (get_random_number(&drng_algorithm, secret_seed, BIT_SEEDBYTES*8ULL)) {
        return -1;
    }
    polyvecl_sample_S1(&s_0, secret_seed, &nonce);
    polyveck_sample_S1(&e, secret_seed, &nonce);

    poly_matrix_expand_ntt(&A_0_ntt, seed_A);
    polyvecl_to_ntt(&s_0_ntt, &s_0);
    poly_matrix_mul_vector_ntt(&b, &A_0_ntt, &s_0_ntt);
    polyveck_add_eta1(&b, &b, &e);
    polyveck_decompose_b(&b1, &b0, &b);

    pack_pk(pk, seed_A, &b1);
    bit_h256(tr, pk, BIT_PUBLICKEYBYTES);
    pack_sk(sk, seed_A, &b1, secret_seed, tr, &s_0, &e, &b0);

    *pk_len_bytes = BIT_PUBLICKEYBYTES;
    *sk_len_bytes = BIT_SECRETKEYBYTES;

    return 0;
}


int bit_sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
                 const unsigned char *m, unsigned long long mlen,
                 unsigned char *sig, unsigned long long *sig_len_bytes) {
    unsigned char seed_A[BIT_SEEDBYTES];
    unsigned char challenge[BIT_CHALLENGEBYTES];
    unsigned char message_hash[BIT_MESSAGEBYTES];
    unsigned char secret_seed[BIT_SEEDBYTES];
    unsigned char tr[BIT_TRBYTES];
    unsigned char b_batch[32];
    size_t b_bit_pos;

    poly_matrix_ntt A_0_ntt;
    polyvecl s_0, c_s;
    polyvecl_ntt y1_ntt;
    polyveck_ntt b1_ntt;
    poly_ntt y0_ntt;
    polyveck e, b1, b0, c_e, b0c;
    polyvecy y, z;
    polyveck w, w1, z2, h;
    polyvecm1 z1;
    poly c_poly;
    sparse_challenge c_sparse;

    if (sk_len_bytes != BIT_SECRETKEYBYTES) {
        return -1;
    }

    unpack_sk(seed_A, &b1, secret_seed, tr, &s_0, &e, &b0, sk);
    poly_matrix_expand_ntt(&A_0_ntt, seed_A);
    polyveck_b1_scaled_to_ntt(&b1_ntt, &b1);

    bit_h256_2(message_hash, tr, BIT_TRBYTES, m, (size_t)mlen);
    unsigned char hash_input[BIT_MESSAGEBYTES + BIT_POLYVECK_W1_BYTES];
    memcpy(hash_input, message_hash, BIT_MESSAGEBYTES);

    unsigned char rnd[BIT_SEEDBYTES];
    if (get_random_number(&drng_algorithm, rnd, BIT_SEEDBYTES*8ULL) != 0) return -1;
    unsigned char temp_seed_y[BIT_SEEDBYTES];
    bit_xof256_3(temp_seed_y, BIT_SEEDBYTES,
                 secret_seed, BIT_SEEDBYTES,
                 rnd, BIT_SEEDBYTES,
                 message_hash, BIT_MESSAGEBYTES);

    uint16_t nonce_y = 0;
    polyvecy_sample_triangular(&y, temp_seed_y, &nonce_y);

    b_bit_pos = sizeof(b_batch) * 8;

    uint32_t sign_iter = 0;
    while (1) {
        if (++sign_iter > BIT_SIGN_MAX_ITERS) {
            secure_zero(temp_seed_y, sizeof(temp_seed_y));
            secure_zero(rnd, sizeof(rnd));
            secure_zero(b_batch, sizeof(b_batch));
            secure_zero(&s_0, sizeof(s_0));
            secure_zero(&e, sizeof(e));
            secure_zero(&b0, sizeof(b0));
            secure_zero(&c_s, sizeof(c_s));
            secure_zero(&c_e, sizeof(c_e));
            secure_zero(&y, sizeof(y));
            secure_zero(&b0c, sizeof(b0c));
            return -1;
        }
        polyvecy_y1_to_ntt(&y1_ntt, &y);
        poly_to_ntt(&y0_ntt, &y.vec[0]);
        polyveck_compute_w_ntt(&w, &A_0_ntt, &b1_ntt, &y1_ntt, &y0_ntt, &y);

        polyveck_highbits(&w1, &w);
        polyveck_pack_w1(hash_input + BIT_MESSAGEBYTES, &w1);
        bit_h256(challenge, hash_input, sizeof(hash_input));

        if (b_bit_pos >= sizeof(b_batch) * 8) {
            if (get_random_number(&drng_algorithm, b_batch, (unsigned long long)sizeof(b_batch) * 8ULL) != 0)
                return -1;
            b_bit_pos = 0;
        }
        uint8_t b_val = (uint8_t)((b_batch[b_bit_pos >> 3] >> (b_bit_pos & 7)) & 1U);
        b_bit_pos++;

        poly_challenge(&c_poly, challenge);

        poly_cneg(&c_poly, b_val);
        poly_challenge_to_sparse(&c_sparse, &c_poly);

        /* Phase 1+2: compute z0 + z1 */
        polyvecl_mul_challenge_no_reduce(&c_s, &s_0, &c_sparse);
        poly_add(&z.vec[0], &y.vec[0], &c_poly);

        for(int i = 0; i < BIT_L; i++) {
            poly_add(&z.vec[1 + i], &y.vec[1 + i], &c_s.vec[i]);
        }

        if (check_reject_sample_z0z1(&z, &c_s, &c_poly, &c_sparse, temp_seed_y, &nonce_y)) {
            polyvecy_sample_y0(&y, temp_seed_y, &nonce_y);
            polyvecy_sample_y1(&y, temp_seed_y, &nonce_y);
            continue;
        }

        /* Phase 3: compute z2 */
        polyveck_mul_challenge_no_reduce(&c_e, &e, &c_sparse);

        for(int i = 0; i < BIT_K; i++) {
            poly_add(&z.vec[1 + BIT_L + i], &y.vec[1 + BIT_L + i], &c_e.vec[i]);
        }

        if (check_reject_sample_z2(&z, &c_e, temp_seed_y, &nonce_y)) {
            polyvecy_sample_triangular(&y, temp_seed_y, &nonce_y);
            continue;
        }

        /* Phase 4: hint computation and rejection checks */
        if (poly_check_reject_highbits_w1_sparse(&w1.vec[0], &w.vec[0], &c_sparse)) {
            polyvecy_sample_triangular(&y, temp_seed_y, &nonce_y);
            continue;
        }

        for(int i = 0; i < BIT_L + 1; i++) {
            z1.vec[i] = z.vec[i];
        }
        for(int i = 0; i < BIT_K; i++) {
            z2.vec[i] = z.vec[1 + BIT_L + i];
        }

        polyveck_mul_challenge_no_reduce(&b0c, &b0, &c_sparse);
        polyveck_make_hint_compressed(&h, &w1, &w, &z2, &b0c, &c_poly, b_val);

        if (check_reject_norm(&z1, &h)) {
            polyvecy_sample_triangular(&y, temp_seed_y, &nonce_y);
            continue;
        }

        if (check_reject_hint_range(&h)) {
            polyvecy_sample_triangular(&y, temp_seed_y, &nonce_y);
            continue;
        }

        break;
    }
    secure_zero(temp_seed_y, sizeof(temp_seed_y));
    secure_zero(rnd, sizeof(rnd));
    secure_zero(b_batch, sizeof(b_batch));
    
    secure_zero(&s_0, sizeof(s_0));
    secure_zero(&e, sizeof(e));
    secure_zero(&b0, sizeof(b0));
    secure_zero(&c_s, sizeof(c_s));
    secure_zero(&c_e, sizeof(c_e));
    secure_zero(&y, sizeof(y));
    secure_zero(&b0c, sizeof(b0c));
    pack_sig(sig, &z1, &h, challenge);

    *sig_len_bytes = BIT_SIGNBYTES;
    return 0;
}

int bit_sig_verify(const unsigned char *sig, unsigned long long siglen,
                   const unsigned char *m, unsigned long long mlen,
                   const unsigned char *pk, unsigned long long pk_len_bytes) {
    unsigned char seed_A[BIT_SEEDBYTES];
    unsigned char challenge[BIT_CHALLENGEBYTES];
    unsigned char c_prime[BIT_CHALLENGEBYTES];
    unsigned char message_hash[BIT_MESSAGEBYTES];

    unsigned char tr[BIT_TRBYTES];
    poly_matrix_ntt A_0_ntt;
    polyveck b1;
    polyveck_ntt b1_ntt;
    polyvecl z1_tail;
    polyvecl_ntt z1_tail_ntt;
    poly_ntt z0_ntt;
    polyvecm1 z1;
    polyveck h;
    poly c_poly;
    
    polyveck w_hat_prime, w_prime;

    if (siglen != BIT_SIGNBYTES || pk_len_bytes != BIT_PUBLICKEYBYTES) {
        return -1;
    }

    unpack_pk(seed_A, &b1, pk);
    
    if (unpack_sig(&z1, &h, challenge, sig) != 0) return -1;

    if (check_reject_norm(&z1, &h)) {
        return -1; 
    }
    bit_h256(tr, pk, BIT_PUBLICKEYBYTES);
    bit_h256_2(message_hash, tr, BIT_TRBYTES, m, (size_t)mlen);
    poly_matrix_expand_ntt(&A_0_ntt, seed_A);
    polyveck_b1_scaled_to_ntt(&b1_ntt, &b1);
    poly_challenge(&c_poly, challenge);

    for (int i = 0; i < BIT_L; i++) {
        z1_tail.vec[i] = z1.vec[i + 1];
    }
    polyvecl_to_ntt(&z1_tail_ntt, &z1_tail);
    poly_to_ntt(&z0_ntt, &z1.vec[0]);
    polyveck_compute_w_hat_prime_ntt(&w_hat_prime, &A_0_ntt, &b1_ntt, &z1_tail_ntt, &z0_ntt, &z1.vec[0], &c_poly);

    polyveck_compute_w_prime(&w_prime, &w_hat_prime, &h);

    unsigned char hash_input[BIT_MESSAGEBYTES + BIT_POLYVECK_W1_BYTES];
    memcpy(hash_input, message_hash, BIT_MESSAGEBYTES);
    polyveck_pack_w1(hash_input + BIT_MESSAGEBYTES, &w_prime);

    bit_h256(c_prime, hash_input, sizeof(hash_input));
    if (memcmp(challenge, c_prime, BIT_CHALLENGEBYTES) != 0) {
        return -1;
    }
    
    return 0;
}
