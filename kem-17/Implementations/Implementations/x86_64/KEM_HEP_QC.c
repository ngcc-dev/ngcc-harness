/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_HEP_QC.h"

#include <drng.h>
#include <stdint.h>
#include <string.h>
#include "PKE_HEP_QC.h"
#include "api.h"
#include "crypto_memset.h"
#include "parameters.h"
#include "parsing.h"
#include "symmetric.h"
#include "vector.h"

#ifdef VERBOSE
#include <stdio.h>
#endif


// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return PUBLIC_KEY_BYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return SECRET_KEY_BYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return SHARED_SECRET_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return CIPHERTEXT_BYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
#ifdef VERBOSE
    printf("\n\n\n### KEYGEN ###");
#endif
    uint8_t seed_kem[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t seed_pke[SEED_BYTES] = {0};
    shake256_xof_ctx ctx_kem = {0};

    uint8_t *ek_pke = calloc(PUBLIC_KEY_BYTES, sizeof(uint8_t));
    uint8_t dk_pke[SEED_BYTES] = {0};

    // Sample seed_kem
    prng_get_bytes(seed_kem, SEED_BYTES);

    // Compute seed_pke and randomness sigma
    xof_init(&ctx_kem, seed_kem, SEED_BYTES);
    xof_get_bytes(&ctx_kem, seed_pke, SEED_BYTES);
    xof_get_bytes(&ctx_kem, sigma, PARAM_SECURITY_BYTES);

    // Compute HEP-QC-PKE keypair
    hep_qc_pke_keygen(ek_pke, dk_pke, seed_pke);

    // Compute HEP-QC-KEM keypair
    memcpy(pk, ek_pke, PUBLIC_KEY_BYTES);
    memcpy(sk, pk, PUBLIC_KEY_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES, dk_pke, SEED_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES, sigma, PARAM_SECURITY_BYTES);
    memcpy(sk + PUBLIC_KEY_BYTES + SEED_BYTES + PARAM_SECURITY_BYTES, seed_kem, SEED_BYTES);

#ifdef VERBOSE
    printf("\n\nseed_kem: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_kem[i]);
    printf("\n\nseed_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", seed_pke[i]);
    printf("\n\nsigma: ");
    for (int i = 0; i < PARAM_SECURITY_BYTES; ++i) printf("%02x", sigma[i]);
#endif

    (void)pk_len_bytes;
    (void)sk_len_bytes;
    // Zeroize sensitive data
    memset_zero(seed_kem, sizeof seed_kem);
    memset_zero(sigma, sizeof sigma);
    memset_zero(seed_pke, sizeof seed_pke);
    memset_zero(dk_pke, sizeof dk_pke);
    free(ek_pke);

    return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
#ifdef VERBOSE
    printf("\n\n\n\n### ENCAPS ###");
#endif
    uint8_t m[PARAM_SECURITY_BYTES] = {0};
    uint8_t ss_theta[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t theta[SEED_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};

    // Sample message m and salt
    prng_get_bytes(m, PARAM_SECURITY_BYTES);
    prng_get_bytes(c_kem_t.salt, SALT_BYTES);

    // Compute shared key ss and ciphertext ct
    hash_h(hash_ek_kem, pk);
    hash_g(ss_theta, hash_ek_kem, m, c_kem_t.salt);
    memcpy(theta, ss_theta + SHARED_SECRET_BYTES, SEED_BYTES);
    hep_qc_pke_encrypt(&c_kem_t.c_pke, pk, (uint64_t *)m, theta);

    hep_qc_c_kem_to_string(ct, &c_kem_t);
    memcpy(ss, ss_theta, SHARED_SECRET_BYTES);

#ifdef VERBOSE
    printf("\n\npk: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", pk[i]);
    printf("\n\nm: ");
    vect_print((uint64_t *)m, PARAM_SECURITY_BYTES);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_t.salt[i]);
    printf("\n\nH(pk): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta[i]);
    printf("\n\nct: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", ct[i]);
    printf("\n\nss: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", ss[i]);
#endif

    (void)pk_len_bytes;
    (void)ss_len_bytes;
    (void)ct_len_bytes;
    // Zeroize sensitive data
    memset_zero(m, sizeof m);
    memset_zero(ss_theta, sizeof ss_theta);
    memset_zero(theta, sizeof theta);

    return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
#ifdef VERBOSE
    printf("\n\n\n\n### DECAPS ###");
#endif
    uint8_t *ek_pke = calloc(PUBLIC_KEY_BYTES, sizeof(uint8_t));
    uint8_t dk_pke[SEED_BYTES] = {0};
    uint8_t sigma[PARAM_SECURITY_BYTES] = {0};
    uint8_t m_prime[PARAM_SECURITY_BYTES] = {0};
    uint8_t hash_ek_kem[SEED_BYTES] = {0};
    uint8_t ss_theta_prime[SHARED_SECRET_BYTES + SEED_BYTES] = {0};
    uint8_t ss_bar[SHARED_SECRET_BYTES] = {0};
    uint8_t theta_prime[SEED_BYTES] = {0};
    ciphertext_kem_t c_kem_t = {0};
    ciphertext_kem_t c_kem_prime_t = {0};
    uint8_t result;

    // Parse decapsulation key sk
    memcpy(ek_pke, sk, PUBLIC_KEY_BYTES);
    memcpy(dk_pke, sk + PUBLIC_KEY_BYTES, SEED_BYTES);
    memcpy(sigma, sk + PUBLIC_KEY_BYTES + SEED_BYTES, PARAM_SECURITY_BYTES);

    // Parse ciphertext ct
    hep_qc_c_kem_from_string(&c_kem_t.c_pke, c_kem_t.salt, ct);

    // Compute message m_prime
    result = hep_qc_pke_decrypt((uint64_t *)m_prime, dk_pke, &c_kem_t.c_pke);

    // Compute shared key ss and ciphertext c_kem_prime
    hash_h(hash_ek_kem, ek_pke);
    hash_g(ss_theta_prime, hash_ek_kem, m_prime, c_kem_t.salt);
    memcpy(ss, ss_theta_prime, SHARED_SECRET_BYTES);
    memcpy(theta_prime, ss_theta_prime + SHARED_SECRET_BYTES, SEED_BYTES);

    hep_qc_pke_encrypt(&c_kem_prime_t.c_pke, ek_pke, (uint64_t *)m_prime, theta_prime);
    memcpy(c_kem_prime_t.salt, c_kem_t.salt, SALT_BYTES);

    // Compute rejection key ss_bar
    hash_j(ss_bar, hash_ek_kem, sigma, &c_kem_t);
    result |= vect_compare((uint8_t *)c_kem_t.c_pke.u, (uint8_t *)c_kem_prime_t.c_pke.u, VEC_N_SIZE_BYTES);
    result |= vect_compare((uint8_t *)c_kem_t.c_pke.v, (uint8_t *)c_kem_prime_t.c_pke.v, VEC_N1N2_SIZE_BYTES);
    result |= vect_compare(c_kem_t.salt, c_kem_prime_t.salt, SALT_BYTES);
    result -= 1;
    for (size_t i = 0; i < SHARED_SECRET_BYTES; ++i) {
        ss[i] = (ss[i] & result) ^ (ss_bar[i] & ~result);
    }

#ifdef VERBOSE
    printf("\n\nek_pke: ");
    for (int i = 0; i < PUBLIC_KEY_BYTES; ++i) printf("%02x", ek_pke[i]);
    printf("\n\ndk_pke: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", dk_pke[i]);
    printf("\n\nct: ");
    for (int i = 0; i < CIPHERTEXT_BYTES; ++i) printf("%02x", ct[i]);
    printf("\n\nm_prime: ");
    vect_print((uint64_t *)m_prime, PARAM_SECURITY_BYTES);
    printf("\n\nH(ek_kem): ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", hash_ek_kem[i]);
    printf("\n\ntheta_prime: ");
    for (int i = 0; i < SEED_BYTES; ++i) printf("%02x", theta_prime[i]);
    printf("\n\n\n# Checking Ciphertext - Begin #");
    printf("\n\nc_kem_prime_t.c_pke.u: ");
    vect_print(c_kem_prime_t.c_pke.u, VEC_N_SIZE_BYTES);
    printf("\n\nc_kem_prime_t.c_pke.v: ");
    vect_print(c_kem_prime_t.c_pke.v, VEC_N1N2_SIZE_BYTES);
    printf("\n\nsalt: ");
    for (int i = 0; i < SALT_BYTES; ++i) printf("%02x", c_kem_prime_t.salt[i]);
    printf("\n\n# Checking Ciphertext - End #\n");
    printf("\n\nss: ");
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) printf("%02x", ss[i]);
#endif

    (void)sk_len_bytes;
    (void)ct_len_bytes;
    (void)ss_len_bytes;
    // Zeroize sensitive data
    memset_zero(dk_pke, sizeof dk_pke);
    memset_zero(sigma, sizeof sigma);
    memset_zero(m_prime, sizeof m_prime);
    memset_zero(ss_theta_prime, sizeof ss_theta_prime);
    memset_zero(ss_bar, sizeof ss_bar);
    memset_zero(theta_prime, sizeof theta_prime);

    free(ek_pke);

    return 0;
}