#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "drng.h"
#include "kem.h"
#include "hash_domain.h"

DRNG_ctx drng_algorithm;
#define RNG_SEED_LENGTH 32
#define NUMBER_OF_POSITIVE_TESTS 1000
#define NUMBER_OF_NEGATIVE_TESTS 1

static int test_kem() {

  unsigned char sk[RRLWR_KEM_SK_LEN];
  unsigned char pk[RRLWR_KEM_PK_LEN];
  unsigned char ct[RRLWR_KEM_CT_LEN];
  unsigned char ss1[RRLWR_KEM_SS_LEN];
  unsigned char ss2[RRLWR_KEM_SS_LEN];
  unsigned long long pk_len_bytes, sk_len_bytes, ct_len_bytes, ss1_len_bytes, ss2_len_bytes;

  kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  kem_enc(pk, pk_len_bytes, ss1, &ss1_len_bytes, ct, &ct_len_bytes);
  kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &ss2_len_bytes);

  assert(pk_len_bytes == RRLWR_KEM_PK_LEN);
  assert(sk_len_bytes == RRLWR_KEM_SK_LEN);
  assert(ct_len_bytes == RRLWR_KEM_CT_LEN);
  assert(ss1_len_bytes == RRLWR_KEM_SS_LEN);
  assert(ss2_len_bytes == RRLWR_KEM_SS_LEN);
  assert(!memcmp(ss1, ss2, RRLWR_KEM_SS_LEN));

  return 0;
}

static int test_kem_negative() {

  unsigned char sk[RRLWR_KEM_SK_LEN];
  unsigned char pk[RRLWR_KEM_PK_LEN];
  unsigned char ct[RRLWR_KEM_CT_LEN];
  unsigned char ss1[RRLWR_KEM_SS_LEN];
  unsigned char ss2[RRLWR_KEM_SS_LEN];
  unsigned long long pk_len_bytes, sk_len_bytes, ct_len_bytes, ss1_len_bytes, ss2_len_bytes;

  kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  kem_enc(pk, pk_len_bytes, ss1, &ss1_len_bytes, ct, &ct_len_bytes);

  unsigned char z[RRLWR_KEM_SEED_Z_LEN];
  unsigned char ss3[RRLWR_KEM_SS_LEN];

  // Flip a bit in the ciphertext and check that the implicitly rejected shared secret is returned
  for(unsigned int i = 0; i < RRLWR_KEM_CT_LEN; i++) {
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &ss2_len_bytes);
    assert(!memcmp(ss1, ss2, RRLWR_KEM_SS_LEN)); // Check ss1 = ss2 before bit flip

    ct[i >> 3] ^= (unsigned char)1 << (i & 7); // Bit flip
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &ss2_len_bytes);
    assert(memcmp(ss1, ss2, RRLWR_KEM_SS_LEN)); // Check that ss1 != ss2 after bit flip

    // Compute the returned shared secret on FO failure
    for(unsigned int j = 0; j < RRLWR_KEM_SEED_Z_LEN; j++) {
      z[j] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + j];
    }
    RRLWR_KEM_HASH_H(ss3, RRLWR_KEM_SS_LEN, ct, z);
    assert(!memcmp(ss2, ss3, RRLWR_KEM_SS_LEN)); // Check ss2 = ss3 (rejected FO secret) after bit flip

    ct[i >> 3] ^= (unsigned char)1 << (i & 7); // Undo bit flip
  }

  // Flip a bit in the private key and check that the implicitly rejected shared secret is returned
  for(unsigned int i = 0; i < RRLWR_KEM_SK_LEN; i++) {
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &ss2_len_bytes);
    assert(!memcmp(ss1, ss2, RRLWR_KEM_SS_LEN)); // Check ss1 = ss2 before bit flip

    sk[i >> 3] ^= (unsigned char)1 << (i & 7); // Bit flip
    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &ss2_len_bytes);
    assert(memcmp(ss1, ss2, RRLWR_KEM_SS_LEN)); // Check that ss1 != ss2 after bit flip

    // Compute the returned shared secret on FO failure
    for(unsigned int j = 0; j < RRLWR_KEM_SEED_Z_LEN; j++) {
      z[j] = sk[RRLWR_PKE_SK_LEN + RRLWR_PKE_PK_LEN + RRLWR_KEM_HPK_LEN + j];
    }
    RRLWR_KEM_HASH_H(ss3, RRLWR_KEM_SS_LEN, ct, z);
    assert(!memcmp(ss2, ss3, RRLWR_KEM_SS_LEN)); // Check ss2 = ss3 (rejected FO secret) after bit flip

    sk[i >> 3] ^= (unsigned char)1 << (i & 7); // Undo bit flip
  }

  return 0;
}

static void test_kem_lengths() {
  int l;
  l = kem_get_pk_len_bytes();
  assert(l == RRLWR_PKE_PK_LEN);
  l = kem_get_sk_len_bytes();
  assert(l == RRLWR_KEM_SK_LEN);
  l = kem_get_ss_len_bytes();
  assert(l == RRLWR_KEM_SS_LEN);
  l = kem_get_ct_len_bytes();
  assert(l == RRLWR_KEM_CT_LEN);
}

int main() {

  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_POSITIVE_TESTS; i++) {
    test_kem();
    printf("Positive test %d complete\n", i);
  }

  for(unsigned int i = 0; i < NUMBER_OF_NEGATIVE_TESTS; i++) {
    test_kem_negative();
    printf("Negative test %d complete\n", i);
  }

  for(unsigned int i = 0; i < NUMBER_OF_POSITIVE_TESTS; i++) {
    test_kem_lengths();
  }

  printf("Success!\n");
  printf("sk len: %d\n", RRLWR_KEM_SK_LEN);
  printf("pk len: %d\n", RRLWR_PKE_PK_LEN);
  printf("ct len: %d\n", RRLWR_KEM_CT_LEN);
  printf("ss len: %d\n", RRLWR_KEM_SS_LEN);

  return 0;
}
