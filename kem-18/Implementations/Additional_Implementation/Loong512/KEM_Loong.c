/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdint.h>
#include <stddef.h>


#include "KEM_Loong.h"
#include "drng.h"
#include "params.h"
#include "poly.h"
#include "auxfunc.h"


// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return (unsigned long long)PUBLICKEY_BYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return (unsigned long long)SECRETKEY_BYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return (unsigned long long)SHARED_KEY_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return (unsigned long long)CIPHERTEXT_BYTES;
}


static void indcpa_keypair(uint8_t pk[PUBLICKEY_BYTES],
                    uint8_t sk[NOISE_POLY_BYTES])
{
    unsigned int i;
    uint8_t buf[3*SEED_BYTES];
    const uint8_t *publicseed = buf;
    const uint8_t *sseed = buf+SEED_BYTES;
    const uint8_t *eseed = buf+2*SEED_BYTES;

    int16_t A[K1*K1*N+2*K1*K2*N+K2*K2*N*N];
    int16_t s[K1*N+K2*N];
    int16_t e[K1*N+K2*N*N];
    int16_t S2[K2*N*N];

    int status = get_random_number(&drng_algorithm, buf, SEED_BYTES * 3*8);
    for (i = 0; i < SEED_BYTES; i++) {
        pk[i] = buf[i];
    }
    
    size_t xlen = K1*K1*N+K1*K2*N+K2*K1*N+K2*K2*N*N;
    uint8_t x[3*xlen];
	status = pseudoXOF(3*8*xlen, publicseed, SEED_BYTES*8, x); 
    sample_vector(x, A, xlen);

    size_t ylen = 2*(K1*N + K2*N)*ETA/8;
    uint8_t y[ylen];
	status = pseudoXOF(8*ylen, sseed, SEED_BYTES*8, y);   
    sample_noise_vector(y, s, K1*N+K2*N);
    polys_to_block_negacyclic_matrix(s+K1*N, S2, K2, 1);

    size_t zlen = 2*(K1*N + K2*N*N)*ETA/8;
    uint8_t z[zlen];
    status = pseudoXOF(8*zlen, eseed, SEED_BYTES*8, z);
    sample_noise_vector(z, e, K1*N+K2*N*N);

    int16_t t1[K1*N], b1[K1*N];
    poly_mat_vec_mul(A, s, t1, K1, K1);
    poly_mat_vec_mul(A+K1*K1*N, s+K1*N, b1, K1, K2);
    poly_vec_add(t1, b1, b1, K1);
    poly_vec_add(b1, e, b1, K1);

    int16_t t5[K2*N], T5[K2*N*N], T6[K2*N*N], B2[K2*N*N];
    poly_mat_vec_mul(A+K1*K1*N+K1*K2*N, s, t5, K2, K1);
    polys_to_block_negacyclic_matrix(t5, T5, K2, 1);
    mat_mul(A+K1*K1*N+2*K1*K2*N, S2, T6, K2*N, K2*N, N);
    mat_add(T5, T6, B2, K2*N, N);
    mat_add(B2, e+K1*N, B2, K2*N, N);

    int16_t b[K1*N], B[K2*N*N];
    compress(b1, K1*N, b, QBITS-DB);
    compress(B2, K2*N*N, B, QBITS-DB);

    encode_11bit_lsb(b, K1*N, pk+SEED_BYTES);
    encode_11bit_lsb(B,  K2*N*N, pk+SEED_BYTES+(K1*N*DB/8));

    encode_noise_vector(s, sk, K1*N+K2*N);
}

static void indcpa_encrypt(const uint8_t m[MSG_BYTES],
                    const uint8_t pk[PUBLICKEY_BYTES],
                    const uint8_t coins[COIN_BYTES],
                    uint8_t c[CIPHERTEXT_BYTES]) {
    unsigned int i;
    uint8_t publicseed[SEED_BYTES];
    
    for (i = 0; i < SEED_BYTES; i++) {
        publicseed[i] = pk[i];
    }

    size_t xlen = K1*K1*N+K1*K2*N+K2*K1*N+K2*K2*N*N;
    int16_t A[xlen];
    uint8_t x[3*xlen];
	int status = pseudoXOF(3*8*xlen, publicseed, SEED_BYTES*8, x);  
    sample_vector(x, A, xlen);

    int16_t b[K1*N], B[K2*N*N];
    decode_11bit_lsb(pk+SEED_BYTES, K1*N, b);
    decode_11bit_lsb(pk+SEED_BYTES+(K1*N*DB/8),K2*N*N,  B);

    int16_t b1[K1*N], B1[K2*N*N];
    decompress(b, K1*N, b1, QBITS-DB);
    decompress(B, K2*N*N, B1, QBITS-DB);
    
    int16_t R2[K2*N*N];
    int16_t r[K1*N + K2*N];
    size_t ylen = 2*(K1*N + K2*N)*ETA/8;
    uint8_t y[ylen];
	status = pseudoXOF(8*ylen, coins, SEED_BYTES*8, y);     
    sample_noise_vector(y, r, K2*N+K1*N);
    polys_to_block_negacyclic_matrix(r+K1*N, R2, 1, K2);
   
    int16_t e3[K1*N+K2*N*N];
    size_t zlen = 2*(K1*N + K2*N*N)*ETA/8;
    uint8_t z[zlen];
	status = pseudoXOF(8*zlen, coins+SEED_BYTES, SEED_BYTES*8, z);
    sample_noise_vector(z, e3, K1*N+K2*N*N);

    int16_t t1[K1*N],  u1[K1*N];
    poly_vec_mat_mul(r, A, t1, K1, K1);
    poly_vec_mat_mul(r+K1*N, A+K1*K1*N+K1*K2*N, u1, K2, K1);
    poly_vec_add(t1, u1, u1, K1);
    poly_vec_add(u1, e3, u1, K1);

    int16_t t5[K2*N], T5[K2*N*N], T7[K2*N*N], U2[K2*N*N];
    poly_vec_mat_mul(r, A+K1*K1*N, t5, K1, K2);
    polys_to_block_negacyclic_matrix(t5, T5, 1, K2);
    mat_mul(R2, A+K1*K1*N+2*K1*K2*N, T7, N, K2*N, K2*N);
    mat_add(T5, T7, U2, N, K2*N);
    mat_add(U2, e3+K1*N, U2, K2*N, N);

    int16_t  E5[N*N];
    size_t wlen = 2*(N*N)*ETA/8;
    uint8_t w[wlen];
	status = pseudoXOF(8*wlen, coins+2*SEED_BYTES, SEED_BYTES*8, w);
    sample_noise_vector(w, E5, N*N);    

    int16_t t8[N], T8[N*N], T0[N*N], V1[K2*N*N];
    poly_vec_inner_product(r, b1, t8, K1);
    poly_to_negacyclic_matrix(t8, T8);
    mat_mul(R2, B1, T0, N, K2*N, N);
    mat_add(T8, T0, V1, N, N);
    mat_add(V1, E5, V1, N, N);

    int16_t M[N*N];
    decode_msg(m, M);
    mat_add(V1, M, V1, N, N);  

    int16_t u[K1*N], U[K2*N*N], V[N*N];
    compress(u1, K1*N, u, QBITS-DU);
    compress(U2, K2*N*N, U, QBITS-DU);
    compress(V1, N*N, V, QBITS-DV);

    encode_11bit_lsb(u, K1*N, c);
    encode_11bit_lsb(U,  K2*N*N, c+K1*N*DU/8);
    encode_4bit_lsb(V,  N*N, c+(K1*N+K2*N*N)*DU/8);
}

static void indcpa_decrypt(const uint8_t c[CIPHERTEXT_BYTES],
                    const uint8_t sk[NOISE_POLY_BYTES],
                    uint8_t m[MSG_BYTES]) 
{
    int16_t S2[K2*N*N];
    int16_t s[K1*N+K2*N];
    decode_noise_vector(sk, s, K1*N+K2*N);
    polys_to_block_negacyclic_matrix(s+K1*N, S2, K2, 1);

    int16_t u[K1*N], U[K2*N*N], V[N*N];
    decode_11bit_lsb(c,  K1*N, u);
    decode_11bit_lsb(c+K1*N*DU/8,  K2*N*N, U);
    decode_4bit_lsb(c+(K1*N+K2*N*N)*DU/8,  N*N, V);

    int16_t u1[K1*N], U1[K2*N*N], V1[N*N];
    decompress(u, K1*N, u1, QBITS-DU);
    decompress(U, K2*N*N, U1, QBITS-DU);
    decompress(V, N*N, V1, QBITS-DV);

    int16_t w[N], W[N*N], T0[N*N];
    poly_vec_inner_product(u1, s, w, K1);
    poly_to_negacyclic_matrix(w, W);

    mat_mul(U1, S2, T0, N, K2*N, N);
    mat_add(W, T0, W, N, N);
    mat_sub(V1, W, W, N, N);
    vector_mul_2(W, N*N);
    vector_central_modulo_q(W, N*N);

    vector_mod_2(W, N*N);
    encode_msg(W, m);
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
    size_t i;
    
    indcpa_keypair(pk, sk); 

    for (i = 0; i < PUBLICKEY_BYTES; i++) {
        sk[i+NOISE_POLY_BYTES] =  pk[i];
    }

	int status =  sm3hash(HASH_BYTES*8, pk, PUBLICKEY_BYTES*8, sk+NOISE_POLY_BYTES+PUBLICKEY_BYTES);
    status = get_random_number(&drng_algorithm, sk+NOISE_POLY_BYTES+PUBLICKEY_BYTES+HASH_BYTES, SEED_BYTES * 8);
    *pk_len_bytes = PUBLICKEY_BYTES;
	*sk_len_bytes = SECRETKEY_BYTES;
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
    size_t i;
    unsigned char buf[MSG_BYTES+HASH_BYTES];
    unsigned char k_gamma[SHARED_KEY_BYTES+3*SEED_BYTES];

    int status = get_random_number(&drng_algorithm, buf, MSG_BYTES * 8);
    
	status =  sm3hash(HASH_BYTES*8, pk, PUBLICKEY_BYTES*8, buf+MSG_BYTES);
    status = pseudoXOF((SHARED_KEY_BYTES+3*SEED_BYTES)*8, buf, (MSG_BYTES+HASH_BYTES)*8, k_gamma); 

    for (i = 0; i < SHARED_KEY_BYTES; i++) {
        ss[i] = k_gamma[i];
    }

    indcpa_encrypt(buf, pk, k_gamma+SHARED_KEY_BYTES, ct);
	*ss_len_bytes = SHARED_KEY_BYTES;
	*ct_len_bytes = CIPHERTEXT_BYTES;

	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
    size_t i;

    unsigned char buf[MSG_BYTES+HASH_BYTES];
    unsigned char salt_ct[SEED_BYTES+CIPHERTEXT_BYTES];
    unsigned char k_gamma[SHARED_KEY_BYTES+3*SEED_BYTES];
    unsigned char kbar[SHARED_KEY_BYTES];
    const unsigned char *pk = sk+NOISE_POLY_BYTES;

    indcpa_decrypt(ct, sk, buf);

    for (i = 0; i < HASH_BYTES; i++) {
        buf[i+MSG_BYTES] = sk[i+NOISE_POLY_BYTES+PUBLICKEY_BYTES];
    }

	int status = pseudoXOF((SHARED_KEY_BYTES+3*SEED_BYTES)*8, buf, (MSG_BYTES+HASH_BYTES)*8, k_gamma); 

    for (i = 0; i < SEED_BYTES; i++) {
        salt_ct[i] = sk[i+NOISE_POLY_BYTES+PUBLICKEY_BYTES+HASH_BYTES];
    }

    indcpa_encrypt(buf, pk, k_gamma+SHARED_KEY_BYTES, salt_ct+SEED_BYTES);

    for (i = 0; i < SHARED_KEY_BYTES; i++) {
        ss[i] = k_gamma[i];
    }

	status = pseudoXOF((SHARED_KEY_BYTES)*8, salt_ct, (SEED_BYTES+CIPHERTEXT_BYTES)*8, kbar); 

    unsigned char fail = 0;
    for (i = 0; i < CIPHERTEXT_BYTES; i++) {
        fail |= ct[i] ^ salt_ct[i+SEED_BYTES];
    }

    fail = (unsigned char)(-fail);
    for (i = 0; i < SHARED_KEY_BYTES; i++) {
        ss[i] ^= fail & (kbar[i]^ss[i]);
    }

	*ss_len_bytes = SHARED_KEY_BYTES;

	return 0;
}