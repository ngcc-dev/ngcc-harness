#include <stdio.h>
#include <string.h>
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "randombytes.h"
#include "symmetric.h"
#include "sampler.h"
#include "reduce.h"

/*************************************************
* Name:        pack_pk_ntt
*
* Description: Packs the public key into a byte array.
*
* Arguments:   - unsigned char *r:          pointer to the output byte array
* - const unsigned char *seedA:  pointer to the seed for matrix A
* - const poly_crt_vec *b_ntt:   pointer to the polynomial vector b in NTT domain
**************************************************/
static void pack_pk_ntt(unsigned char *r, const unsigned char *seedA, const poly_crt_vec *b_ntt) {

    memcpy(r, seedA, LORE_SYMBYTES);
    unsigned char *main_ptr = r + LORE_SYMBYTES;
    
    unsigned char overflow_buf_all[LORE_K * LORE_N / 8 + 2] = {0};
    size_t total_overflow_bits = 0;

    for (int i = 0; i < LORE_K; i++) {
        unsigned char temp_overflow_buf[LORE_N / 8 + 2] = {0};
        size_t bits_written = lore_poly_pack_q_split(main_ptr, temp_overflow_buf, &b_ntt->vec[i].q_poly);
        
        // Append the generated overflow bits to the total overflow buffer overflow_buf_all
        for (size_t k = 0; k < bits_written; k++) {
            if ((temp_overflow_buf[k / 8] >> (k % 8)) & 1) {
                size_t final_bit_pos = total_overflow_bits + k;
                overflow_buf_all[final_bit_pos / 8] |= (unsigned char)(1 << (final_bit_pos % 8));
            }
        }
        total_overflow_bits += bits_written;
        main_ptr += LORE_N; 
    }

    /* Write overflow bits */
    size_t overflow_byte_len = (total_overflow_bits + 7) / 8;   
    memcpy(main_ptr, overflow_buf_all, overflow_byte_len);

/* Write T-data */
#if LORE_R_BITS > 0
    unsigned char *t_ptr = main_ptr + overflow_byte_len;
    size_t t_poly_len = LORE_POLY_COMPRESSED_BYTES_T;
    for (int i = 0; i < LORE_K; i++) {
        uint8_t p_t_indices[LORE_N];
        for(int j=0; j<LORE_N; ++j) {
            p_t_indices[j] = (uint8_t)find_closest_r_idx(b_ntt->vec[i].t_poly.coeffs[j]);
        }
        pack_t_bits(t_ptr, p_t_indices);
        t_ptr += t_poly_len;
    }
    unsigned char *end_ptr = t_ptr; 
#else
    unsigned char *end_ptr = main_ptr + overflow_byte_len; 
#endif

    size_t bytes_written = end_ptr - r;
    if (bytes_written < LORE_PUBLICKEYBYTES) {
        memset(end_ptr, 0, LORE_PUBLICKEYBYTES - bytes_written);
    }
}

/*************************************************
* Name:        unpack_pk_ntt
*
* Description: Unpacks the public key from a byte array.
*
* Arguments:   - poly_crt_vec *b_ntt:      pointer to the output polynomial vector b in NTT domain
* - unsigned char *seedA:     pointer to the output seed for matrix A
* - const unsigned char *pk:  pointer to the input byte array
**************************************************/
static void unpack_pk_ntt(poly_crt_vec *b_ntt, unsigned char *seedA, const unsigned char *pk) {

    memcpy(seedA, pk, LORE_SYMBYTES);
    const unsigned char *ptr = pk + LORE_SYMBYTES;

    size_t main_data_len = LORE_K * LORE_N;
    size_t total_overflow_bits = 0;
    for(size_t i = 0; i < main_data_len; ++i) {
        if (ptr[i] == 0xFF) {
            total_overflow_bits++;
        }
    }
    
    const unsigned char *main_ptr = ptr;
    const unsigned char *overflow_ptr = ptr + main_data_len;

    int overflow_bit_pos = 0;

    for (int i = 0; i < LORE_K; i++) {
        lore_poly_unpack_q_split(&b_ntt->vec[i].q_poly, main_ptr + (i * LORE_N), overflow_ptr, &overflow_bit_pos);
    }

#if LORE_R_BITS > 0
    size_t overflow_byte_len = (total_overflow_bits + 7) / 8;
    const unsigned char *t_ptr = overflow_ptr + overflow_byte_len; 
    size_t t_poly_len = LORE_POLY_COMPRESSED_BYTES_T;
    for (int i = 0; i < LORE_K; i++) {
        uint8_t p_t_indices[LORE_N];
        unpack_t_bits(p_t_indices, t_ptr);
        t_ptr += t_poly_len;
        for (int j = 0; j < LORE_N; ++j) {
            b_ntt->vec[i].t_poly.coeffs[j] = (int16_t)(2 * p_t_indices[j]);
        }
    }
#else
    for (int i = 0; i < LORE_K; i++) {
        for (int j = 0; j < LORE_N; j++) {
            b_ntt->vec[i].t_poly.coeffs[j] = 0;
        }
    }
#endif
}


/*************************************************
* Name:        pack_sk
*
* Description: Packs the secret key into a byte array with lossless compression.
*
* Arguments:   - unsigned char *r:          pointer to output byte array
* - const poly_sparse *s_t_sparse: pointer to input sparse T-ring
* - const poly_crt_vec *s_crt_ntt: pointer to input secret vector
**************************************************/
static void pack_sk(unsigned char *r, const poly_sparse *s_t_sparse, const poly_crt_vec *s_crt_ntt) {
    unsigned char *ptr = r;
    for(int i = 0; i < LORE_K; i++) {
        memcpy(ptr, &s_t_sparse[i].n_coeffs, sizeof(int16_t));
        ptr += sizeof(int16_t);
    }
    for (int i = 0; i < LORE_K; i++) {
        size_t pos_size = (size_t)s_t_sparse[i].n_coeffs * sizeof(s_t_sparse[i].pos[0]);
        size_t val_size = (size_t)s_t_sparse[i].n_coeffs * sizeof(s_t_sparse[i].val[0]);
        memcpy(ptr, s_t_sparse[i].pos, pos_size);
        ptr += pos_size;
        memcpy(ptr, s_t_sparse[i].val, val_size);
        ptr += val_size;
    }

    unsigned char *main_ptr = ptr;
    unsigned char overflow_buf_all[LORE_K * LORE_N / 8 + 2] = {0};
    size_t total_overflow_bits = 0;

    for (int i = 0; i < LORE_K; i++) {
        unsigned char temp_overflow_buf[LORE_N / 8 + 2] = {0};
        size_t bits_written = lore_poly_pack_q_split(main_ptr, temp_overflow_buf, &s_crt_ntt->vec[i].q_poly);
        
        for (size_t k = 0; k < bits_written; k++) {
            if ((temp_overflow_buf[k / 8] >> (k % 8)) & 1) {
                size_t final_bit_pos = total_overflow_bits + k;
                overflow_buf_all[final_bit_pos / 8] |= (unsigned char)(1 << (final_bit_pos % 8));
            }
        }
        total_overflow_bits += bits_written;
        main_ptr += LORE_N; 
    }

    size_t overflow_byte_len = (total_overflow_bits + 7) / 8;
    memcpy(main_ptr, overflow_buf_all, overflow_byte_len);

    unsigned char *end_ptr = main_ptr + overflow_byte_len;
    size_t bytes_written = end_ptr - r;
    if (bytes_written < LORE_SECRETKEYBYTES) {
        memset(end_ptr, 0, LORE_SECRETKEYBYTES - bytes_written);
    }
}

/*************************************************
* Name:        unpack_sk
*
* Description: Unpacks the secret key from a byte array.
*
* Arguments:   - poly_crt_vec *s_crt_ntt_q: pointer to output secret vector
* - poly_sparse *s_t_sparse:   pointer to output sparse T-ring
* - const unsigned char *sk:   pointer to input byte array
**************************************************/
static void unpack_sk(poly_crt_vec *s_crt_ntt_q, poly_sparse *s_t_sparse, const unsigned char *sk) {
    const unsigned char *ptr = sk;
    for(int i = 0; i < LORE_K; i++) {
        memcpy(&s_t_sparse[i].n_coeffs, ptr, sizeof(int16_t));
        ptr += sizeof(int16_t);
    }
    for (int i = 0; i < LORE_K; i++) {
        size_t pos_size = (size_t)s_t_sparse[i].n_coeffs * sizeof(s_t_sparse[i].pos[0]);
        size_t val_size = (size_t)s_t_sparse[i].n_coeffs * sizeof(s_t_sparse[i].val[0]);
        memcpy(s_t_sparse[i].pos, ptr, pos_size);
        ptr += pos_size;
        memcpy(s_t_sparse[i].val, ptr, val_size);
        ptr += val_size;
    }

    size_t main_data_len = LORE_K * LORE_N;
    size_t total_overflow_bits = 0;
    // Pre-scan main buffer to calculate overflow bits.
    for(size_t i = 0; i < main_data_len; ++i) {
        if (ptr[i] == 0xFF) {
            total_overflow_bits++;
        }
    }

    const unsigned char *main_ptr = ptr;
    const unsigned char *overflow_ptr = ptr + main_data_len;
    int overflow_bit_pos = 0;

    for (int i = 0; i < LORE_K; i++) {
        lore_poly_unpack_q_split(&s_crt_ntt_q->vec[i].q_poly, main_ptr + (i * LORE_N), overflow_ptr, &overflow_bit_pos);
        
        // Reconstruct dense representation of T-ring.
        memset(s_crt_ntt_q->vec[i].t_poly.coeffs, 0, LORE_POLY_BYTES);
        for (int j = 0; j < s_t_sparse[i].n_coeffs; j++) {
            s_crt_ntt_q->vec[i].t_poly.coeffs[s_t_sparse[i].pos[j]] = s_t_sparse[i].val[j];
        }
    }
}

/*************************************************
* Name:        pack_ciphertext_ntt
*
* Description: Packs the ciphertext into a byte array.
*
* Arguments:   - unsigned char *r:          pointer to the output byte array
* - const poly_crt_vec *cu_ntt:  pointer to the polynomial vector cu in NTT domain
* - const poly_crt *cv_crt:    pointer to the polynomial cv in CRT domain
**************************************************/
static void pack_ciphertext_ntt(unsigned char *r, const poly_crt_vec *cu_ntt, const poly_crt *cv_crt) {
    unsigned char *main_ptr = r;
    // Collect overflow bits for Cu.
    unsigned char overflow_buf_all[(LORE_K * LORE_N + 7) / 8 + 2] = {0}; 
    size_t total_overflow_bits = 0;
    

    for(int i = 0; i < LORE_K; ++i) {
        unsigned char temp_overflow_buf[LORE_N / 8 + 2] = {0};
        size_t bits_written = lore_poly_pack_q_split(main_ptr, temp_overflow_buf, &cu_ntt->vec[i].q_poly);
        
        for (size_t k = 0; k < bits_written; k++) {
            if ((temp_overflow_buf[k / 8] >> (k % 8)) & 1) {
                size_t final_bit_pos = total_overflow_bits + k;
                overflow_buf_all[final_bit_pos / 8] |= (unsigned char)(1 << (final_bit_pos % 8));
            }
        }
        total_overflow_bits += bits_written;
        main_ptr += LORE_N;
    }

    size_t overflow_byte_len = (total_overflow_bits + 7) / 8;
    memcpy(main_ptr, overflow_buf_all, overflow_byte_len);
    unsigned char *ptr = main_ptr + overflow_byte_len;

#if LORE_R_BITS > 0
    size_t t_poly_len = LORE_POLY_COMPRESSED_BYTES_T;
    for(int i = 0; i < LORE_K; ++i) {
        uint8_t t_indices[LORE_N];
        for(int j = 0; j < LORE_N; ++j) {
            t_indices[j] = (uint8_t)find_closest_r_idx(cu_ntt->vec[i].t_poly.coeffs[j]);
        }
        pack_t_bits(ptr, t_indices);
        ptr += t_poly_len;
    }
#endif


    memset(ptr, 0, (LORE_N * LORE_L + 7) / 8);
    for (int j = 0; j < LORE_N; ++j) {
        uint16_t val = cv_crt->q_poly.coeffs[j] & ((1 << LORE_L) - 1);
        size_t bit_idx = j * LORE_L;
        ptr[bit_idx / 8] |= (val << (bit_idx % 8));
    }
    ptr += (LORE_N * LORE_L + 7) / 8;


    memset(ptr, 0, (LORE_N * LORE_T_BITS + 7) / 8);
    for (int j = 0; j < LORE_N; ++j) {
        uint16_t val = cv_crt->t_poly.coeffs[j] & ((1 << LORE_T_BITS) - 1);
        size_t bit_idx = j * LORE_T_BITS;
        ptr[bit_idx / 8] |= (val << (bit_idx % 8));
    }
    ptr += (LORE_N * LORE_T_BITS + 7) / 8;
}

/*************************************************
* Name:        unpack_ciphertext_ntt
*
* Description: Unpacks the ciphertext from a byte array.
*
* Arguments:   - poly_crt_vec *cu_ntt:      pointer to the output polynomial vector cu in NTT domain
* - poly_crt *cv:             pointer to the output polynomial cv
* - const unsigned char *c:   pointer to the input byte array
**************************************************/
static void unpack_ciphertext_ntt(poly_crt_vec *cu_ntt, poly_crt *cv_crt, const unsigned char *c) {

    size_t main_data_len = LORE_K * LORE_N;
    size_t total_overflow_bits = 0;
    for(size_t i = 0; i < main_data_len; ++i) {
        if (c[i] == 0xFF) total_overflow_bits++;
    }

    const unsigned char *main_ptr = c;
    const unsigned char *overflow_ptr = c + main_data_len;
    int overflow_bit_pos = 0;

    for (int i = 0; i < LORE_K; i++) {
        lore_poly_unpack_q_split(&cu_ntt->vec[i].q_poly, main_ptr + (i * LORE_N), overflow_ptr, &overflow_bit_pos);
    }

    size_t overflow_byte_len = (total_overflow_bits + 7) / 8;
    const unsigned char *ptr = overflow_ptr + overflow_byte_len;

#if LORE_R_BITS > 0
    size_t t_poly_len = LORE_POLY_COMPRESSED_BYTES_T;
    for (int i = 0; i < LORE_K; i++) {
        uint8_t t_indices[LORE_N];
        unpack_t_bits(t_indices, ptr);
        ptr += t_poly_len;
        for(int j=0; j<LORE_N; ++j){
            cu_ntt->vec[i].t_poly.coeffs[j] = (int16_t)(2 * t_indices[j] );
        }
    }
#else
    for(int i=0; i<LORE_K; ++i) {
        for(int j=0; j<LORE_N; ++j) cu_ntt->vec[i].t_poly.coeffs[j] = 0;
    }
#endif

    // Unpack x'_q of Cv and recover x''_q via Algorithm 6.
    for (int j = 0; j < LORE_N; ++j) {
        size_t bit_idx = j * LORE_L;
        uint16_t x_q_prime = (ptr[bit_idx / 8] >> (bit_idx % 8)) & ((1 << LORE_L) - 1);
        
        // Decompression_c: x''_q = round(q * x'_q / 2^l) mod q
        int32_t x_q_prime_prime = ((x_q_prime * LORE_Q) + (1 << (LORE_L - 1))) >> LORE_L;
        cv_crt->q_poly.coeffs[j] = barrett_reduce((int16_t)x_q_prime_prime);
    }
    ptr += (LORE_N * LORE_L + 7) / 8;


    
    for (int j = 0; j < LORE_N; ++j) {
        size_t bit_idx = j * LORE_T_BITS;
        uint16_t x_t_prime = (ptr[bit_idx / 8] >> (bit_idx % 8)) & ((1 << LORE_T_BITS) - 1);
        cv_crt->t_poly.coeffs[j] = (int16_t)x_t_prime;
    }
    ptr += (LORE_N * LORE_T_BITS + 7) / 8;
    
}
/*************************************************
* Name:        indcpa_keypair_derand
*
* Description: Deterministically generates IND-CPA public and private key.
* All randomness is derived from the input coins.
*
* Arguments:   - unsigned char *pk: pointer to output public key
* - unsigned char *sk: pointer to output secret key
* - const unsigned char *coins: pointer to input randomness (32 bytes)
**************************************************/
void indcpa_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins)
{
    poly_crt_vec a_ntt[LORE_K];
    poly_crt_vec s_crt;
    poly_crt_vec b_final;
    poly_sparse s_t_sparse[LORE_K];

    unsigned char buf[2 * LORE_SYMBYTES];
    const unsigned char *seedA = buf;
    const unsigned char *noiseseed = buf + LORE_SYMBYTES;
    
    sm3_xof256(buf, 2 * LORE_SYMBYTES, coins, LORE_SYMBYTES);

    gen_matrix_ntt(a_ntt, seedA, 0);

    poly_getnoise(&s_crt, s_t_sparse, (unsigned char*)noiseseed, 0);

    poly_crt_vec_ntt(&s_crt);

    for (int i = 0; i < LORE_K; i++) {
    #if LORE_LEVEL == 1
        poly_crt_vec_pointwise_acc_montgomery_sparse(&b_final.vec[i], &a_ntt[i], &s_crt, s_t_sparse);
    #else
        poly_crt_vec_pointwise_acc_montgomery(&b_final.vec[i], &a_ntt[i], &s_crt);
    #endif
    }

    for(int i=0; i<LORE_K; ++i){
        poly delta_q_std;
        poly t_poly_orig = b_final.vec[i].t_poly;
        for(int j=0; j<LORE_N; ++j){
            int16_t x_t = t_poly_orig.coeffs[j];
            int16_t r_idx = find_closest_r_idx(x_t);
            int16_t r_val =(int16_t)( 2 * r_idx );
            int16_t delta_val = sym_mod(r_val - x_t, LORE_T);
            delta_q_std.coeffs[j] = delta_val;
        }

        poly_ntt(&delta_q_std);
        poly_add(&b_final.vec[i].q_poly, &b_final.vec[i].q_poly, &delta_q_std);

        for(int j=0; j<LORE_N; ++j){
            int16_t r_idx = find_closest_r_idx(t_poly_orig.coeffs[j]);
            b_final.vec[i].t_poly.coeffs[j] =(int16_t)( 2 * r_idx );
        }
    }

    pack_pk_ntt(pk, seedA, &b_final);
    pack_sk(sk, s_t_sparse, &s_crt);
}

/*************************************************
* Name:        indcpa_keypair
*
* Description: Randomly generates IND-CPA public and private key.
*
* Arguments:   - unsigned char *pk: pointer to output public key
* - unsigned char *sk: pointer to output secret key
**************************************************/
void indcpa_keypair(unsigned char *pk, unsigned char *sk)
{
    unsigned char coins[LORE_SYMBYTES];
    randombytes(coins, LORE_SYMBYTES);
    indcpa_keypair_derand(pk, sk, coins);
}

/*************************************************
* Name:        indcpa_enc
*
* Description: Encryption function of the IND-CPA secure
* public-key encryption scheme.
*
* Arguments:   - unsigned char *c:         pointer to output ciphertext
* - const unsigned char *m:   pointer to input message
* - const unsigned char *pk:  pointer to public key
* - const unsigned char *coins: pointer to randomness
**************************************************/
void indcpa_enc(unsigned char *c,
                const unsigned char *m,
                const unsigned char *pk,
                const unsigned char *coins) {
    poly_crt_vec at_ntt[LORE_K];
    poly_crt_vec sp_crt;
    poly_crt_vec b_ntt;
    poly_crt_vec cu_final;
    poly_crt cv_crt;
    poly e_pp; 
    poly_sparse sp_t_sparse[LORE_K];

    unsigned char seedA[LORE_SYMBYTES];

    unpack_pk_ntt(&b_ntt, seedA, pk);

    /* Generate transpose matrix A^T */
    gen_matrix_ntt(at_ntt, seedA, 1); 

    poly_getnoise(&sp_crt, sp_t_sparse, (unsigned char*)coins, 0);

    poly_crt_vec_ntt(&sp_crt);

    for (int i = 0; i < LORE_K; i++) {
    #if LORE_LEVEL == 1
        // Level 1: Sparse multiplication.
        poly_crt_vec_pointwise_acc_montgomery_sparse(&cu_final.vec[i], &at_ntt[i], &sp_crt, sp_t_sparse);
    #else
        // Level 2, 3, 4: Dense multiplication.
        poly_crt_vec_pointwise_acc_montgomery(&cu_final.vec[i], &at_ntt[i], &sp_crt);
    #endif
    }

    for(int i=0; i<LORE_K; ++i){
        poly delta_q_std;
        poly t_poly_orig = cu_final.vec[i].t_poly;
        for(int j=0; j<LORE_N; ++j){
            int16_t x_t = t_poly_orig.coeffs[j];
            int16_t r_idx = find_closest_r_idx(x_t);
            int16_t r_val = (int16_t)(2 * r_idx);
            int16_t delta_val = sym_mod(r_val - x_t, LORE_T);
            delta_q_std.coeffs[j] = delta_val;
        }
        
        poly_ntt(&delta_q_std);
        poly_add(&cu_final.vec[i].q_poly, &cu_final.vec[i].q_poly, &delta_q_std);

        for(int j=0; j<LORE_N; ++j){
             int16_t r_idx = find_closest_r_idx(t_poly_orig.coeffs[j]);
             cu_final.vec[i].t_poly.coeffs[j] = (int16_t)(2 * r_idx);
        }
    }

    poly_crt_vec_pointwise_acc_montgomery_sparse(&cv_crt, (const poly_crt_vec*)&b_ntt, &sp_crt, sp_t_sparse);

    poly_invntt_tomont(&cv_crt.q_poly);
    for (int i = 0; i < LORE_N; ++i) {
        cv_crt.q_poly.coeffs[i] = montgomery_reduce((int64_t)cv_crt.q_poly.coeffs[i]);
    }



    poly_getnoise_uniform(&e_pp, LORE_T, coins, (unsigned char)LORE_K);
    for (int i = 0; i < LORE_N; ++i) {
        cv_crt.q_poly.coeffs[i] = barrett_reduce(cv_crt.q_poly.coeffs[i] + e_pp.coeffs[i]);
        cv_crt.t_poly.coeffs[i] = sym_mod(cv_crt.t_poly.coeffs[i] + e_pp.coeffs[i], LORE_T);
    }

    poly msg_poly;
    poly_encode_msg(&msg_poly, m);
    poly_add_scaled_msg(&cv_crt, &msg_poly);

    for (int i = 0; i < LORE_N; ++i) {

        int32_t x_q = cv_crt.q_poly.coeffs[i] % LORE_Q;
        x_q += (x_q >> 31) & LORE_Q;
        
        int32_t x_t = cv_crt.t_poly.coeffs[i] % LORE_T;
        x_t += (x_t >> 31) & LORE_T;

        // x'_q = round(2^l * x_q / q) mod 2^l
        int32_t x_q_prime = ((x_q << LORE_L) + (LORE_Q >> 1)) / LORE_Q;
        x_q_prime &= ((1 << LORE_L) - 1); 

        int32_t x_q_prime_prime = ((x_q_prime * LORE_Q) + (1 << (LORE_L - 1))) >> LORE_L;
        x_q_prime_prime %= LORE_Q;
        x_q_prime_prime += (x_q_prime_prime >> 31) & LORE_Q;


        int32_t delta_q = sym_mod(x_q_prime_prime - x_q, LORE_Q);
        // Compensate truncation error to T-ring.
        int32_t x_t_prime = (x_t + delta_q) % LORE_T;
        x_t_prime += (x_t_prime >> 31) & LORE_T;

        cv_crt.q_poly.coeffs[i] = (int16_t)x_q_prime;
        cv_crt.t_poly.coeffs[i] = (int16_t)x_t_prime;
    }

    // Pack the compressed ciphertext components.
    pack_ciphertext_ntt(c, &cu_final, &cv_crt);

}

/*************************************************
* Name:        indcpa_dec
*
* Description: Decryption function of the IND-CPA secure
* public-key encryption scheme.
*
* Arguments:   - unsigned char *m:        pointer to output message
* - const unsigned char *c:  pointer to input ciphertext
* - const unsigned char *sk: pointer to secret key
**************************************************/
void indcpa_dec(unsigned char *m,
                const unsigned char *c,
                const unsigned char *sk) {
    poly_crt_vec cu_ntt;
    poly_crt cv_crt, s_dot_cu;

    poly_crt_vec s_crt;             
    poly_sparse s_t_sparse[LORE_K]; 


    unpack_ciphertext_ntt(&cu_ntt, &cv_crt, c);

    unpack_sk(&s_crt, s_t_sparse, sk);


    poly_crt_vec_pointwise_acc_montgomery_sparse(&s_dot_cu, (const poly_crt_vec*)&cu_ntt, &s_crt, s_t_sparse);

    poly_invntt_tomont(&s_dot_cu.q_poly);

    for (int i = 0; i < LORE_N; ++i) {
      s_dot_cu.q_poly.coeffs[i] = montgomery_reduce((int32_t)s_dot_cu.q_poly.coeffs[i]);
    }

    poly_crt R_crt;
    poly_crt_sub(&R_crt, &cv_crt, &s_dot_cu);

    poly_decode_msg_crt(m, &R_crt);
}
