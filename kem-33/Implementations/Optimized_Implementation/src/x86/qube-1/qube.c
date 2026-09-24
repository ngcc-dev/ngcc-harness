/**
 * @file qube.c
 * @brief High-level QUBE-PKE API implementation
 */

#include "qube.h"
#include <stdint.h>
#include <string.h>
#include "code.h"
#include "crypto_memset.h"
#include "gf2x.h"
#include "parameters.h"
#include "symmetric.h"
#include "vector.h"
#include "auxfunc.h"
#include "randombytes.h"
#include <stdio.h>
#include "benchmark.h"
#include <stddef.h>
#include <immintrin.h>

/**
 * @brief Generates a key pair for the QUBE public-key encryption (PKE) scheme.
 *
 * This function creates a public encryption key (`ek_pke`) and a private decryption key (`dk_pke`)
 * for use in the QUBE PKE scheme. The key generation process is seeded with the given `seed` input.
 *
 * @param[out] ek_pke  Pointer to the buffer that will receive the encryption key.
 * @param[out] dk_pke  Pointer to the buffer that will receive the decryption key.
 * @param[in]  seed    Pointer to the seed used to deterministically generate the key pair.
 *
 */
void qube_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed) {
    uint8_t keypair_seed[2 * SEED_BYTES] = {0};
    uint8_t *seed_dk = keypair_seed;
    uint8_t *seed_ek = keypair_seed + SEED_BYTES;
    shake256_xof_ctx dk_xof_ctx = {0};
    shake256_xof_ctx ek_xof_ctx = {0};

    static __m256i h1[VEC_N_256_NUM_WORDS];static __m256i h2[VEC_N_256_NUM_WORDS];
    static __m256i x1[VEC_N_256_NUM_WORDS];static __m256i x2[VEC_N_256_NUM_WORDS];
    static __m256i y[VEC_N_256_NUM_WORDS];
    static uint64_t s1[VEC_N_256_SIZE_64] = {0};static uint64_t s2[VEC_N_256_SIZE_64] = {0};
    static __m256i tmp[VEC_N_256_NUM_WORDS] = {0};

#ifdef __STDC_LIB_EXT1__
    memset_s(x1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset_s(x2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(y, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(h1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset_s(h2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#else
    memset(x1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset(x2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(y, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(h1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset(h2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#endif

    // Derive keypair seeds
    hash_i(keypair_seed, seed);

    ////////////////////////////////////////////////////////////
    /////////////benchmark for encode//////////////////////////////
    // int TEST_RUN = 1000;
    // uint64_t rec_encode[TEST_RUN];unsigned len_encode[1] = {0};
    // memset(rec_encode, 0, sizeof(rec_encode));
    // char mesg[256];
    // for(int i=0;i<TEST_RUN;i++){
    // REC_TIMING( rec_encode , len_encode , {
    // 	// Compute encryption key
    // xof_init(&ek_xof_ctx, seed_ek, SEED_BYTES);
    // vect_set_random(&ek_xof_ctx, (uint64_t *)h1);vect_set_random(&ek_xof_ctx, (uint64_t *)h2);
    // vect_set_random(&ek_xof_ctx, (uint64_t *)h3);vect_set_random(&ek_xof_ctx, (uint64_t *)h4);

    //     });
    // }
    // report(mesg,sizeof(mesg),rec_encode,len_encode[0]);
    // printf("encode: %s\n", mesg);    
    //////////////////////////////////////////////////////////////
    // Compute decryption key
    xof_init(&dk_xof_ctx, seed_dk, SEED_BYTES);
    vect_sample_fixed_weight1(&dk_xof_ctx, y, PARAM_OMEGA_Y1);
    vect_sample_fixed_weight1(&dk_xof_ctx, x1, PARAM_OMEGA_X1);
    vect_sample_fixed_weight1(&dk_xof_ctx, x2, PARAM_OMEGA_X2);

    // Compute encryption key
    xof_init(&ek_xof_ctx, seed_ek, SEED_BYTES);
    vect_set_random(&ek_xof_ctx, (uint64_t *)h1);
    vect_set_random(&ek_xof_ctx, (uint64_t *)h2);

    vect_mul(tmp, y, h1);
    //ring_mul((uint8_t *)tmp, (uint8_t *)y, (uint8_t *)h1);
    vect_add(s1, (uint64_t *)tmp, (uint64_t *)x1, VEC_N_256_SIZE_64);

    //////////////////////////////////////////////////////////////
    ///////////benchmark for mul//////////////////////////////
    // int TEST_RUN = 1000;
    // uint64_t rec_mul[TEST_RUN];unsigned len_mul[1] = {0};
    // memset(rec_mul, 0, sizeof(rec_mul));
    // char mesg[256];
    // for(int i=0;i<TEST_RUN;i++){
    // REC_TIMING( rec_mul , len_mul , {
    // 	vect_mul(tmp, y, h1);
    //     });
    // }
    // report(mesg,sizeof(mesg),rec_mul,len_mul[0]);
    // printf("mul: %s\n", mesg);    
    ////////////////////////////////////////////////////////////////

    vect_mul(tmp, y, h2);
    //ring_mul((uint8_t *)tmp, (uint8_t *)y, (uint8_t *)h2);
    vect_add(s2, (uint64_t *)tmp, (uint64_t *)x2, VEC_N_256_SIZE_64);
    
    // Parse encryption key to string
    memcpy(ek_pke, seed_ek, SEED_BYTES);
    memcpy(ek_pke + SEED_BYTES, s1, VEC_N_SIZE_BYTES);
    memcpy(ek_pke + SEED_BYTES + VEC_N_SIZE_BYTES, s2, VEC_N_SIZE_BYTES);
    
    // Parse decryption key to string
    memcpy(dk_pke, seed_dk, SEED_BYTES);

    // Zeroize sensitive data
    memset_zero(keypair_seed, sizeof keypair_seed);
    memset_zero(x1, sizeof x1);memset_zero(x1, sizeof x2);
    memset_zero(&dk_xof_ctx, sizeof dk_xof_ctx);
}

/**
 * @brief Encrypts a message using the QUBE public-key encryption (PKE) scheme.
 *
 * This function performs encryption in the QUBE PKE scheme. It uses the given encryption key (`ek_pke`)
 * and encryption randomness (`theta`) to encrypt the message `m`, producing a ciphertext `c_pke`.
 *
 * @param[out] c_pke     Pointer to the output ciphertext structure (PKE ciphertext).
 * @param[in]  ek_pke    Pointer to the encryption key.
 * @param[in]  m         Pointer to the message to be encrypted.
 * @param[in]  theta     Pointer to the encryption randomness used during encryption.
 *
 */
void qube_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta) {

    shake256_xof_ctx theta_xof_ctx = {0};
    static __m256i h1[VEC_N_256_NUM_WORDS];static __m256i h2[VEC_N_256_NUM_WORDS];
    static __m256i s1[VEC_N_256_NUM_WORDS];static __m256i s2[VEC_N_256_NUM_WORDS];
    static __m256i r1[VEC_N_256_NUM_WORDS];
    static __m256i r2_1[VEC_N_256_NUM_WORDS];static __m256i r2_2[VEC_N_256_NUM_WORDS];
    static __m256i e[VEC_N_256_NUM_WORDS];

#ifdef __STDC_LIB_EXT1__
    memset_s(r2_1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset_s(r2_2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(h1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset_s(h2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(s1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset_s(s2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(r1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(e, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#else
    memset(r2_1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset(r2_2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(h1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset(h2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(s1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));memset(s2, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(r1, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(e, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#endif

    static __m256i tmp1[VEC_N_256_NUM_WORDS];
    static __m256i tmp2[VEC_N_256_NUM_WORDS];
    static uint64_t tmp3[VEC_N_256_SIZE_64];

    // Initialize Xof using theta
    xof_init(&theta_xof_ctx, theta, SEED_BYTES);

    // Retrieve h and s from public key
    qube_ek_pke_from_string((uint64_t *)h1, (uint64_t *)h2, (uint64_t *)s1, (uint64_t *)s2, ek_pke);
    
    // Generate re, e and r1
    vect_sample_fixed_weight2(&theta_xof_ctx, r2_1, PARAM_OMEGA_R21);vect_sample_fixed_weight2(&theta_xof_ctx, r2_2, PARAM_OMEGA_R22);
    vect_sample_fixed_weight1(&theta_xof_ctx, e, PARAM_OMEGA_E);
    vect_sample_fixed_weight2(&theta_xof_ctx, r1, PARAM_OMEGA_R11);
    
    // Compute u = r1 + h.r2
    vect_mul(tmp1, h1, r2_1);vect_mul(tmp2, h2, r2_2);
    //ring_mul((uint8_t *)tmp1, (uint8_t *)h1, (uint8_t *)r2_1);ring_mul((uint8_t *)tmp2, (uint8_t *)h2, (uint8_t *)r2_2);
    vect_add(tmp3, (uint64_t *)tmp1, (uint64_t *)tmp2, VEC_N_256_SIZE_64);
    vect_add(c_pke->u, tmp3, (uint64_t *)r1, VEC_N_256_SIZE_64);
    
    // Compute v = C.encode(m)
    code_encode(c_pke->v, m);
    
    // Compute v = C.encode(m) + Truncate(s.r2 + e)
    vect_mul(tmp1, s1, r2_1);vect_mul(tmp2, s2, r2_2);
    //ring_mul((uint8_t *)tmp1, (uint8_t *)s1, (uint8_t *)r2_1);ring_mul((uint8_t *)tmp2, (uint8_t *)s2, (uint8_t *)r2_2);
    vect_add(tmp3, (uint64_t *)tmp1, (uint64_t *)tmp2, VEC_N_256_SIZE_64);
    vect_add(tmp3, (uint64_t *)e, tmp3, VEC_N_256_SIZE_64);
    vect_truncate(tmp3);
    vect_add(c_pke->v, c_pke->v, tmp3, VEC_N1N2_SIZE_64);

    // Zeroize sensitive data
    memset_zero(r1, sizeof r1);
    memset_zero(r2_1, sizeof r2_1);memset_zero(r2_2, sizeof r2_2);
    memset_zero(e, sizeof e);

    // memset_zero(tmp, sizeof tmp);
    memset_zero(&theta_xof_ctx, sizeof theta_xof_ctx);
    memset_zero(tmp1, sizeof tmp1);
    memset_zero(tmp2, sizeof tmp2);
    memset_zero(tmp3, sizeof tmp3);
}

/**
 * @brief Decrypts a ciphertext using the QUBE public-key encryption (PKE) scheme.
 *
 * This function performs decryption in the QUBE PKE scheme. It uses the given decryption key (`dk_pke`)
 * to decrypt the ciphertext `c_pke`, recovering the original message `m`.
 *
 * @param[out] m         Pointer to the output buffer where the decrypted message will be stored.
 * @param[in]  dk_pke    Pointer to the decryption key.
 * @param[in]  c_pke     Pointer to the input ciphertext structure (PKE ciphertext).
 *
 * @return Returns 0 on success.
 *
 */
uint8_t qube_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke) {
    static __m256i y[VEC_N_256_NUM_WORDS] = {0};
    __m256i u[VEC_N_256_NUM_WORDS] = {0};

#ifdef __STDC_LIB_EXT1__
    memset_s(y, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset_s(u, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#else
    memset(y, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
    memset(u, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#endif

    static __m256i tmp1[VEC_N_256_NUM_WORDS];
    static uint64_t tmp2[VEC_N_256_SIZE_64] = {0};

    // Parse decryption key dk_pke
    qube_dk_pke_from_string((uint64_t *)y, dk_pke);

    for (size_t i = 0; i < (VEC_N_256_NUM_WORDS); ++i) {
        u[i] = _mm256_set_epi64x(c_pke->u[i * 4 + 3], c_pke->u[i * 4 + 2], c_pke->u[i * 4 + 1], c_pke->u[i * 4 + 0]);
    }

    // Compute u.y
    vect_mul(tmp1, y, u);
    //ring_mul((uint8_t *)tmp1, (uint8_t *)y, (uint8_t *)c_pke->u);

    // Truncate(u.y)
    vect_truncate(tmp1);

    // Compute v - Truncate(u.y)
    vect_add(tmp2, c_pke->v, tmp1, VEC_N1N2_SIZE_64);

    //////////////////////////////////////////////////////////
    ///////////benchmark for decode//////////////////////////////
    // int TEST_RUN = 1000;
    // uint64_t rec_decode[TEST_RUN];unsigned len_decode[1] = {0};
    // memset(rec_decode, 0, sizeof(rec_decode));
    // char mesg[256];
    // for(int i=0;i<TEST_RUN;i++){
    // REC_TIMING( rec_decode , len_decode , {
    // 	code_decode(m, tmp1);
    //     });
    // }
    // report(mesg,sizeof(mesg),rec_decode,len_decode[0]);
    // printf("decode: %s\n", mesg);    
    ////////////////////////////////////////////////////////////

    // Compute plaintext m
    code_decode(m, tmp2);
    
    // Zeroize sensitive data
    memset_zero(y, sizeof y);
    memset_zero(tmp1, sizeof tmp1);
    memset_zero(tmp2, sizeof tmp2);
    
    return 0;
}
