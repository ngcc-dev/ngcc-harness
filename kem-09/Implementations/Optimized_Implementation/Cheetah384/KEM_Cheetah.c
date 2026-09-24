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
#include <stdio.h>


#include "params.h"
#include "drng.h"
#include "auxfunc.h"
#include "poly.h"
#include "ntt.h"
#include "KEM_Cheetah.h"



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

    int16_t A[CHEETAH_K*CHEETAH_K*N];
    int16_t s[CHEETAH_K*N];
    int16_t e[CHEETAH_K*N];

    int status = get_random_number(&drng_algorithm, buf, SEED_BYTES * 3*8);
    for (i = 0; i < SEED_BYTES; i++) {
        pk[i] = buf[i];
    }
    
    size_t xlen = CHEETAH_K*CHEETAH_K*N;
    uint8_t x[3*xlen];
	status = pseudoXOF(3*8*xlen, publicseed, SEED_BYTES*8, x); 
    sample_vector(x, A, xlen);

    size_t ylen = 2*(CHEETAH_K*N)*ETA/8;
    uint8_t y[ylen];
	status = pseudoXOF(8*ylen, sseed, SEED_BYTES*8, y);   
    sample_noise_vector(y, s, CHEETAH_K*N);

    size_t zlen = 2*(CHEETAH_K*N)*ETA/8;
    uint8_t z[zlen];
    status = pseudoXOF(8*zlen, eseed, SEED_BYTES*8, z);
    sample_noise_vector(z, e, CHEETAH_K*N);
 
    for (i = 0; i < CHEETAH_K; i++) {
        ntt_640(s+i*N, NTT_FORWARD);
    }

    int16_t t1[CHEETAH_K*N];
    poly_mat_vec_pwmul(A, s, t1);

    int16_t  b1[CHEETAH_K*N];
    poly_vec_add(t1, e, b1, CHEETAH_K);

    int16_t b[CHEETAH_K*N];
    compress(b1, CHEETAH_K*N, b, QBITS-DB);

    encode_11bit_lsb(b, CHEETAH_K*N, pk+SEED_BYTES);
    encode_13bit_lsb(s, CHEETAH_K*N, sk);
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

    size_t xlen =  CHEETAH_K*CHEETAH_K*N;
    int16_t A[xlen];
    uint8_t x[3*xlen];
	int status = pseudoXOF(3*8*xlen, publicseed, SEED_BYTES*8, x);  
    sample_vector(x, A, xlen);

    int16_t b[CHEETAH_K*N];
    decode_11bit_lsb(pk+SEED_BYTES, CHEETAH_K*N, b);

    int16_t b1[CHEETAH_K*N];
    decompress(b, CHEETAH_K*N, b1, QBITS-DB);   

    int16_t r[CHEETAH_K*N];
    size_t ylen = 2*(CHEETAH_K*N)*ETA/8;
    uint8_t y[ylen];
	status = pseudoXOF(8*ylen, coins, SEED_BYTES*8, y);     
    sample_noise_vector(y, r, CHEETAH_K*N);

    int16_t e1[CHEETAH_K*N];
    size_t zlen = 2*(CHEETAH_K*N)*ETA/8;
    uint8_t z[zlen];
	status = pseudoXOF(8*zlen, coins+SEED_BYTES, SEED_BYTES*8, z);
    sample_noise_vector(z, e1, CHEETAH_K*N);

    int16_t e2[N];
    size_t wlen = 2*N*ETA/8;
    uint8_t w[wlen];
	status = pseudoXOF(8*wlen, coins+2*SEED_BYTES, SEED_BYTES*8, w);
    sample_noise_vector(w, e2, N);

    for (i = 0; i < CHEETAH_K; i++) {
        ntt_640(r+i*N, NTT_FORWARD);
    }
    
    int16_t t1[CHEETAH_K*N];
    poly_vec_mat_pwmul( r, A, t1);

    int16_t  u1[CHEETAH_K*N], u[CHEETAH_K*N];
    poly_vec_add(t1, e1, u1, CHEETAH_K);
    compress(u1, CHEETAH_K*N, u, QBITS-DU);

    for (i = 0; i < CHEETAH_K; i++) {
        ntt_640(b1+i*N, NTT_FORWARD);
    }
    
    int16_t v1[N];
    poly_vec_pwmul(r, b1, v1);
    poly_vec_add(v1, e2, v1, 1);

    int16_t msgpoly[N];
    decode_msg(m, msgpoly);
    poly_vec_add(v1, msgpoly, v1, 1);

    int16_t v[SHARED_KEY_BYTES*8];
    compress(v1, SHARED_KEY_BYTES*8, v, QBITS-DV);

    encode_11bit_lsb(u, CHEETAH_K*N, c);
    encode_4bit_lsb(v, SHARED_KEY_BYTES*8, c+(CHEETAH_K*N)*DU/8);
}


static void indcpa_decrypt(const uint8_t c[CIPHERTEXT_BYTES],
                    const uint8_t sk[NOISE_POLY_BYTES],
                    uint8_t m[MSG_BYTES]) 
{

    int16_t s[CHEETAH_K*N];
    decode_13bit_lsb(sk, CHEETAH_K*N, s);

    int16_t u[CHEETAH_K*N], v[CHEETAH_K*N];
    decode_11bit_lsb(c, CHEETAH_K*N, u);
    decode_4bit_lsb(c+(CHEETAH_K*N)*DU/8, SHARED_KEY_BYTES*8, v);
    int16_t u1[CHEETAH_K*N], v1[SHARED_KEY_BYTES*8];
    decompress(u, CHEETAH_K*N, u1, QBITS-DU);
    decompress(v, SHARED_KEY_BYTES*8, v1, QBITS-DV);

    for (int i = 0; i < CHEETAH_K; i++) {
        ntt_640(u1+i*N, NTT_FORWARD);
    }

    int16_t t[N];
    poly_vec_pwmul(u1, s, t);   

    for (int i = 0; i < SHARED_KEY_BYTES*8; i++) {
        v1[i] = (v1[i] - t[i]);
    }

    vector_mul_2(v1, SHARED_KEY_BYTES*8);
    vector_centered_mod(v1, SHARED_KEY_BYTES*8);

    vector_mod_2(v1, SHARED_KEY_BYTES*8);
    encode_msg(v1, m);
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
