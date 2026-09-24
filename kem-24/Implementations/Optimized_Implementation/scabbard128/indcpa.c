#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>

#include "indcpa.h"
#include "drng.h"
#include "auxfunc.h"
#include "polyvec.h"
#include "poly.h"
#include "packing.h"

extern DRNG_ctx drng_algorithm;
const int shift_amt = SCABBARD_EQ - SCABBARD_EP;
const int shift_amt_enc = SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B);

void indcpa_keypair(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    size_t i;

    ALIGN8 uint8_t buf[2 * SCABBARD_SYMBYTES];
    const uint8_t *publicseed = buf;
    ALIGN8 const uint8_t *noiseseed = buf + SCABBARD_SYMBYTES;
    polyvec a[SCABBARD_L], pkpv, skpv;

    get_random_number(&drng_algorithm, buf, SCABBARD_SYMBYTES * 8);
    pseudoXOF(SCABBARD_SYMBYTES * 8, buf, SCABBARD_SYMBYTES * 8, buf); /* for not revealing RNG state when publicseed is revealed */
    get_random_number(&drng_algorithm, buf + SCABBARD_SYMBYTES, SCABBARD_SYMBYTES * 8);

    gen_a(a, publicseed);
    gen_s(&skpv, noiseseed);
    
    matrix_vector_mul(&pkpv, a, &skpv, 1);
    
    __m256i v_h1 = _mm256_set1_epi16(h1);
    __m256i v_q  = _mm256_set1_epi16(SCABBARD_Q);
    __m256i v_coeffs;
    
    for(i = 0; i < SCABBARD_L; i++){
        // Load 16 coefficients from memory
        v_coeffs = _mm256_loadu_si256((__m256i*)&pkpv.vec[i].coeffs[0]);
        v_coeffs = _mm256_add_epi16(v_coeffs, v_h1);
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_srli_epi16(v_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&pkpv.vec[i].coeffs[0], v_coeffs);

        v_coeffs = _mm256_loadu_si256((__m256i*)&pkpv.vec[i].coeffs[16]);
        v_coeffs = _mm256_add_epi16(v_coeffs, v_h1);
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_srli_epi16(v_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&pkpv.vec[i].coeffs[16], v_coeffs);

        v_coeffs = _mm256_loadu_si256((__m256i*)&pkpv.vec[i].coeffs[32] );
        v_coeffs = _mm256_add_epi16(v_coeffs, v_h1);
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_srli_epi16(v_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&pkpv.vec[i].coeffs[32], v_coeffs);

        v_coeffs = _mm256_loadu_si256((__m256i*)&pkpv.vec[i].coeffs[48]);
        v_coeffs = _mm256_add_epi16(v_coeffs, v_h1);
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_srli_epi16(v_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&pkpv.vec[i].coeffs[48], v_coeffs);
    }
    pack_sk(sk, &skpv);
    pack_pk(pk, &pkpv, publicseed);
}

void indcpa_enc(
    uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const uint8_t r[SCABBARD_SYMBYTES])
{
    size_t i;

    uint8_t seed[SCABBARD_SYMBYTES];
    poly v, m;
    polyvec a[SCABBARD_L], pkpv, skpv, u;

    unpack_pk(&pkpv, seed, pk);
    poly_frommsg(&m, msg);

    gen_a(a, seed);
    gen_s(&skpv, r);

    matrix_vector_mul(&u, a, &skpv, 0);
    /* rounding */
    __m256i v_q  = _mm256_set1_epi16(SCABBARD_Q);
    __m256i v_p  = _mm256_set1_epi16(SCABBARD_P);
    const int shift_amt = SCABBARD_EQ - SCABBARD_EP;
    const int shift_amt_enc = SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B);
    __m256i u_coeffs;

    for(i = 0; i < SCABBARD_L; i++){
        u_coeffs = _mm256_loadu_si256((__m256i*)&u.vec[i].coeffs[0]);
        u_coeffs = _mm256_and_si256(u_coeffs, v_q);
        u_coeffs = _mm256_srli_epi16(u_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&u.vec[i].coeffs[0], u_coeffs);

        u_coeffs = _mm256_loadu_si256((__m256i*)&u.vec[i].coeffs[16]);
        u_coeffs = _mm256_and_si256(u_coeffs, v_q);
        u_coeffs = _mm256_srli_epi16(u_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&u.vec[i].coeffs[16], u_coeffs);

        u_coeffs = _mm256_loadu_si256((__m256i*)&u.vec[i].coeffs[32]);
        u_coeffs = _mm256_and_si256(u_coeffs, v_q);
        u_coeffs = _mm256_srli_epi16(u_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&u.vec[i].coeffs[32], u_coeffs);

        u_coeffs = _mm256_loadu_si256((__m256i*)&u.vec[i].coeffs[48]);
        u_coeffs = _mm256_and_si256(u_coeffs, v_q);
        u_coeffs = _mm256_srli_epi16(u_coeffs, shift_amt);
        _mm256_storeu_si256((__m256i*)&u.vec[i].coeffs[48], u_coeffs);
    }
    inner_prod(&v, &pkpv, &skpv);

    for (i = 0; i+16 <= SCABBARD_N; i+=16) {
        __m256i v_coeffs = _mm256_loadu_si256((__m256i*)&v.coeffs[i]);
        v_coeffs = _mm256_and_si256(v_coeffs, v_p);
        v_coeffs = _mm256_slli_epi16(v_coeffs, shift_amt);
        v_coeffs = _mm256_sub_epi16(v_coeffs, _mm256_loadu_si256((__m256i*)&m.coeffs[i]));
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_srli_epi16(v_coeffs, shift_amt_enc);
        _mm256_storeu_si256((__m256i*)&v.coeffs[i], v_coeffs);
    }
    pack_ciphertext(ct, &u, &v);
}

void indcpa_dec(
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    size_t i;

    polyvec skpv, u;
    poly v, v_prime;

    unpack_ciphertext(&u, &v, ct);
    unpack_sk(&skpv, sk);

    inner_prod(&v_prime, &u, &skpv);

    __m256i v_q  = _mm256_set1_epi16(SCABBARD_Q);
    __m256i v_p  = _mm256_set1_epi16(SCABBARD_P);
    
    for (i=0; i+16 <= SCABBARD_N; i += 16) {    
        __m256i v_coeffs = _mm256_loadu_si256((__m256i*)&v.coeffs[i]);
        __m256i v_prime_coeffs = _mm256_loadu_si256((__m256i*)&v_prime.coeffs[i]);
        v_coeffs = _mm256_and_si256(v_coeffs, v_q);
        v_coeffs = _mm256_slli_epi16(v_coeffs, shift_amt_enc);
        v_prime_coeffs = _mm256_and_si256(v_prime_coeffs, v_p);
        v_prime_coeffs = _mm256_slli_epi16(v_prime_coeffs, shift_amt);
        v_prime_coeffs = _mm256_sub_epi16(v_prime_coeffs, v_coeffs);
        v_prime_coeffs = _mm256_and_si256(v_prime_coeffs, v_q);
        _mm256_storeu_si256((__m256i*)&v_prime.coeffs[i], v_prime_coeffs);
    }
    poly_tomsg(msg, &v_prime);
}
