/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdio.h>
#include <string.h>

#include "../SIG_AlgorithmInstance.h"
#include "../params.h"
#include "../drng.h"
#include "../symmetric.h"
#include "test_common.h"

#define MAX_MSG_BYTES 512

#if BIT_USE_SHAKE
#define XOF_TEST_BYTES (3 * SHAKE128_RATE + 37)

static int test_xof128_stream_wrapper(void) {
  uint8_t seed[BIT_SEEDBYTES + 2];
  uint8_t extseed[sizeof(seed) + 1];
  uint8_t expected[XOF_TEST_BYTES];
  uint8_t actual[XOF_TEST_BYTES];
  const size_t chunks[] = {1, SHAKE128_RATE - 3, 5, SHAKE128_RATE, 19, 2 * SHAKE128_RATE + 15};
  bit_xof128_state st;
  size_t off = 0;

  test_fill_seed(seed, sizeof(seed), 7, 0);
  memcpy(extseed, seed, sizeof(seed));
  extseed[sizeof(seed)] = 23;
  shake128(expected, sizeof(expected), extseed, sizeof(extseed));

  bit_xof128_init(&st, extseed, sizeof(extseed));
  for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++) {
    size_t take = chunks[i];
    if (take > sizeof(actual) - off) take = sizeof(actual) - off;
    if (take == 0) break;
    bit_xof128_squeeze(&st, actual + off, take);
    off += take;
  }
  if (off < sizeof(actual)) {
    bit_xof128_squeeze(&st, actual + off, sizeof(actual) - off);
  }
  bit_xof128_zeroize(&st);
  return memcmp(actual, expected, sizeof(expected));
}
#endif

static int test_keygen_sign_verify(unsigned int seed_domain, unsigned int msg_domain, size_t msg_len) {
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t sig[BIT_SIGNBYTES];
  uint8_t msg[MAX_MSG_BYTES];
  uint8_t seed[55];
  unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

  test_fill_seed(seed, sizeof(seed), seed_domain, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -1;

  if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return -2;
  if (pk_len != BIT_PUBLICKEYBYTES || sk_len != BIT_SECRETKEYBYTES) return -3;

  test_fill_message(msg, msg_len, msg_domain, 0);
  if (sig_sign(sk, sk_len, msg, msg_len, sig, &sig_len) != 0) return -4;
  if (sig_len != BIT_SIGNBYTES) return -5;

  if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) != 0) return -6;

  /* tampered message */
  if (msg_len > 0) {
    msg[msg_len / 2] ^= 0x80U;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0) return -7;
    msg[msg_len / 2] ^= 0x80U;
  }

  /* tampered signature — use positions safely away from potential padding bits */
  sig[sig_len / 3] ^= 0x40U;
  if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0) return -8;
  sig[sig_len / 3] ^= 0x40U;

  sig[sig_len * 2 / 3] ^= 0x02U;
  if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0) return -9;
  sig[sig_len * 2 / 3] ^= 0x02U;

  /* tampered public key */
  pk[pk_len / 2] ^= 0x55U;
  if (sig_verify(pk, pk_len, sig, sig_len, msg, msg_len) == 0) return -10;
  pk[pk_len / 2] ^= 0x55U;

  return 0;
}

static int test_wrong_key_pair(void) {
  uint8_t pk1[BIT_PUBLICKEYBYTES], pk2[BIT_PUBLICKEYBYTES];
  uint8_t sk1[BIT_SECRETKEYBYTES], sk2[BIT_SECRETKEYBYTES];
  uint8_t sig[BIT_SIGNBYTES];
  uint8_t msg[64];
  uint8_t seed[55];
  unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

  /* keypair 1 */
  test_fill_seed(seed, sizeof(seed), 100, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -1;
  if (sig_keygen(pk1, &pk_len, sk1, &sk_len) != 0) return -2;

  /* keypair 2 */
  test_fill_seed(seed, sizeof(seed), 200, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -3;
  if (sig_keygen(pk2, &pk_len, sk2, &sk_len) != 0) return -4;

  test_fill_message(msg, sizeof(msg), 1, 0);
  if (sig_sign(sk1, sk_len, msg, sizeof(msg), sig, &sig_len) != 0) return -5;

  /* verify with wrong public key */
  if (sig_verify(pk2, pk_len, sig, sig_len, msg, sizeof(msg)) == 0) return -6;

  return 0;
}

static int test_empty_message(void) {
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t sig[BIT_SIGNBYTES];
  uint8_t seed[55];
  unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

  test_fill_seed(seed, sizeof(seed), 42, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -1;
  if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return -2;
  if (sig_sign(sk, sk_len, NULL, 0, sig, &sig_len) != 0) return -3;
  if (sig_len != BIT_SIGNBYTES) return -4;
  if (sig_verify(pk, pk_len, sig, sig_len, NULL, 0) != 0) return -5;
  return 0;
}

static int test_padding_rejection(void) {
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t sig[BIT_SIGNBYTES];
  uint8_t msg[64];
  uint8_t seed[55];
  unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

  test_fill_seed(seed, sizeof(seed), 99, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -1;
  if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return -2;

  test_fill_message(msg, sizeof(msg), 99, 0);
  if (sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0) return -3;

  /* flip padding bits in last byte of overflow buffer — must be rejected */
  sig[sig_len - 1] ^= 0xE0U;
  if (sig_verify(pk, pk_len, sig, sig_len, msg, sizeof(msg)) == 0) return -4;
  sig[sig_len - 1] ^= 0xE0U;

  return 0;
}

static int test_multi_sign_verify(unsigned int seed_domain, unsigned int rounds) {
  uint8_t pk[BIT_PUBLICKEYBYTES];
  uint8_t sk[BIT_SECRETKEYBYTES];
  uint8_t sig[BIT_SIGNBYTES];
  uint8_t msg[MAX_MSG_BYTES];
  uint8_t seed[55];
  unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;

  test_fill_seed(seed, sizeof(seed), seed_domain, 0);
  if (init_random_number(&test_rng, seed, (unsigned long long)sizeof(seed)) != 0) return -1;
  if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return -2;

  for (unsigned int r = 0; r < rounds; r++) {
    test_fill_message(msg, sizeof(msg), seed_domain, r);
    if (sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0) return -3;
    if (sig_len != BIT_SIGNBYTES) return -4;
    if (sig_verify(pk, pk_len, sig, sig_len, msg, sizeof(msg)) != 0) return -5;
  }
  return 0;
}

int main(void) {
  int failures = 0;

#if BIT_USE_SHAKE
  if (test_xof128_stream_wrapper() != 0) {
    printf("FAIL: xof128 stream wrapper\n");
    failures++;
  } else {
    printf("PASS: xof128 stream wrapper\n");
  }
#endif

  /* multiple seed domains */
  {
    const unsigned int domains[] = {0, 1, 2, 3, 5, 7, 13, 19, 31, 63, 127, 255};
    for (size_t i = 0; i < sizeof(domains) / sizeof(domains[0]); i++) {
      int rc = test_keygen_sign_verify(domains[i], domains[i], 64);
      if (rc != 0) {
        printf("FAIL: keygen/sign/verify domain=%u (rc=%d)\n", domains[i], rc);
        failures++;
      } else {
        printf("PASS: keygen/sign/verify domain=%u\n", domains[i]);
      }
    }
  }

  /* multiple message lengths */
  {
    const size_t msg_lens[] = {0, 1, 2, 3, 5, 8, 13, 21, 32, 47, 64, 100, 128, 200, 256, 400, 512};
    for (size_t i = 0; i < sizeof(msg_lens) / sizeof(msg_lens[0]); i++) {
      int rc = test_keygen_sign_verify(77, 77, msg_lens[i]);
      if (rc != 0) {
        printf("FAIL: message length %zu (rc=%d)\n", msg_lens[i], rc);
        failures++;
      } else {
        printf("PASS: message length %zu\n", msg_lens[i]);
      }
    }
  }

  /* wrong key pair */
  if (test_wrong_key_pair() != 0) {
    printf("FAIL: wrong key pair\n");
    failures++;
  } else {
    printf("PASS: wrong key pair rejection\n");
  }

  /* padding rejection */
  if (test_padding_rejection() != 0) {
    printf("FAIL: padding bit rejection\n");
    failures++;
  } else {
    printf("PASS: padding bit rejection\n");
  }

  /* empty message */
  if (test_empty_message() != 0) {
    printf("FAIL: empty message\n");
    failures++;
  } else {
    printf("PASS: empty message sign/verify\n");
  }

  /* multi sign/verify */
  {
    const unsigned int rounds[] = {1, 5, 10};
    for (size_t i = 0; i < sizeof(rounds) / sizeof(rounds[0]); i++) {
      int rc = test_multi_sign_verify(50 + (unsigned int)i, rounds[i]);
      if (rc != 0) {
        printf("FAIL: multi sign/verify rounds=%u (rc=%d)\n", rounds[i], rc);
        failures++;
      } else {
        printf("PASS: multi sign/verify rounds=%u\n", rounds[i]);
      }
    }
  }

  if (failures == 0) {
    printf("\nAll correctness tests passed.\n");
  } else {
    printf("\n%d test(s) failed.\n", failures);
  }

  printf("\n--- Sizes ---\n");
  printf("SEEDBYTES       = %d\n",  BIT_SEEDBYTES);
  printf("CHALLENGEBYTES  = %d\n",  BIT_CHALLENGEBYTES);
  printf("TRBYTES         = %d\n",  BIT_TRBYTES);
  printf("MESSAGEBYTES    = %d\n",  BIT_MESSAGEBYTES);
#ifdef BIT_POLYVECK_H_BASE3_BYTES
  printf("HINT  base3     = %d B  (K=%d * ceil(N=%d/%d) groups)\n",
         BIT_POLYVECK_H_BASE3_BYTES, BIT_K, BIT_N, BIT_H_PACK_GROUP);
  printf("HINT  overflow  = %d B  (MAX=%d * %d bits + count)\n",
         BIT_POLYVECK_H_OVERFLOW_BYTES, BIT_H_OVERFLOW_MAX, BIT_H_OVERFLOW_BITS);
  printf("HINT  total     = %d B\n", BIT_POLYVECK_H_BYTES);
#else
  printf("HINT  bits/coeff= %d\n", BIT_POLY_H_BITS);
  printf("HINT  total     = %d B  (K=%d * N=%d * %d / 8)\n",
         BIT_POLYVECK_H_BYTES, BIT_K, BIT_N, BIT_POLY_H_BITS);
#endif
  printf("pk             = %d B  (seed + K * b1) = %d + %d * %d\n",
         BIT_PUBLICKEYBYTES, BIT_SEEDBYTES, BIT_K, BIT_POLY_B1_BYTES);
  printf("sig            = %d B  (challenge + z0 + L*z1 + hint)\n",
         BIT_SIGNBYTES);
  printf("                  = %d + %d + %d*%d + %d\n",
         BIT_CHALLENGEBYTES, BIT_POLY_Z0_BYTES, BIT_L, BIT_POLY_Z1_BYTES, BIT_POLYVECK_H_BYTES);
  printf("sk             = %d B\n", BIT_SECRETKEYBYTES);

  return failures != 0 ? 1 : 0;
}
