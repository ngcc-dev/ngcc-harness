#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "drng.h"
#include "sign.h"

#ifdef MEASURE_HEURISTICS
  int32_t num_rejects_w = 0;
  int32_t num_rejects_y = 0;
  int32_t num_rejects_ct0 = 0;
  int32_t num_rejects_hint = 0;
  int32_t max_hint_weight = 0;
  int32_t num_rounds = 0;
#endif

DRNG_ctx drng_algorithm;
#define RNG_SEED_LENGTH 32
#define NUMBER_OF_POSITIVE_TESTS 1000
#define NUMBER_OF_NEGATIVE_TESTS 1
#define TEST_MESSAGE_LEN 256

static int test_sign() {

  unsigned char sk[RRLWR_SIGN_SK_LEN];
  unsigned char pk[RRLWR_SIGN_PK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN];
  unsigned char m[TEST_MESSAGE_LEN] = {0};
  unsigned long long pk_len_bytes, sk_len_bytes, m_len_bytes = TEST_MESSAGE_LEN, sn_len_bytes;

  sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes);
  assert(!sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));

  assert(pk_len_bytes == RRLWR_SIGN_PK_LEN);
  assert(sk_len_bytes == RRLWR_SIGN_SK_LEN);
  assert(sn_len_bytes == RRLWR_SIGN_SIG_LEN);

  return 0;
}

static int test_sign_negative() {

  unsigned char sk[RRLWR_SIGN_SK_LEN];
  unsigned char pk[RRLWR_SIGN_PK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN];
  unsigned char m[TEST_MESSAGE_LEN] = {0};
  unsigned long long pk_len_bytes, sk_len_bytes, m_len_bytes = TEST_MESSAGE_LEN, sn_len_bytes;

  sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes);

  // Flip a bit in the signature
  for(unsigned int i = 0; i < 8*RRLWR_SIGN_SIG_LEN; i++) {
    if((i % 1000) == 0) {
      printf("Signature bit flip: %d\n", i);
    }
    sn[(i>>3)] ^= (1 << (i%8)); // Flip bit and check that signature verification fails
    assert(sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
    sn[(i>>3)] ^= (1 << (i%8)); // Unflip bit and check that signature verification works
    assert(!sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
  }

  // Flip a bit in the public key
  for(unsigned int i = 0; i < 8*RRLWR_SIGN_PK_LEN; i++) {
    if((i % 1000) == 0) {
      printf("Public key bit flip: %d\n", i);
    }
    pk[(i>>3)] ^= (1 << (i%8)); // Flip bit and check that signature verification fails
    assert(sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
    pk[(i>>3)] ^= (1 << (i%8)); // Unflip bit and check that signature verification works
    assert(!sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
  }

  // Flip a bit in the message
  for(unsigned int i = 0; i < 8*32; i++) {
    if((i % 1000) == 0) {
      printf("Message bit flip: %d\n", i);
    }
    m[(i>>3)] ^= (1 << (i%8)); // Flip bit and check that signature verification fails
    assert(sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
    m[(i>>3)] ^= (1 << (i%8)); // Unflip bit and check that signature verification works
    assert(!sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes));
  }

  return 0;
}

int main() {

  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_POSITIVE_TESTS; i++) {
    test_sign();
    if((i % 1000) == 0) {
      printf("Positive test %d complete\n", i);
    }
  }

  for(unsigned int i = 0; i < NUMBER_OF_NEGATIVE_TESTS; i++) {
    test_sign_negative();
    if((i % 1000) == 0) {
      printf("Negative test %d complete\n", i);
    }
  }

  printf("Success!\n");

  #ifdef MEASURE_HEURISTICS
    printf("max number of hints: %d\n", max_hint_weight);
    printf("number of rejections based on hints: %d\n", num_rejects_hint);
    printf("number of rejections based on w: %d\n", num_rejects_w);
    printf("number of rejections based on y: %d\n", num_rejects_y);
    printf("number of rejections based on ct0: %d\n", num_rejects_ct0);
    printf("number of rounds: %f\n", (float)num_rounds/(float)NUMBER_OF_POSITIVE_TESTS);
  #endif

  printf("sk len: %d\n", RRLWR_SIGN_SK_LEN);
  printf("pk len: %d\n", RRLWR_SIGN_PK_LEN);
  printf("sn len: %d\n", RRLWR_SIGN_SIG_LEN);

  return 0;
}