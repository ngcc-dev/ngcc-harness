#include <stdint.h>
#include "cpucycles.h"
#include "speed_print.h"
#include "drng.h"
#include "sign.h"

#define NUMBER_OF_TESTS 1000
#define RNG_SEED_LENGTH 32
#define BENCHMARK_MESSAGE_LEN 32

uint64_t t[NUMBER_OF_TESTS + 1];
DRNG_ctx drng_algorithm;

int main() {
  unsigned char sk[RRLWR_SIGN_SK_LEN];
  unsigned char pk[RRLWR_SIGN_PK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN];
  unsigned char m[BENCHMARK_MESSAGE_LEN] = {0};
  unsigned long long pk_len_bytes, sk_len_bytes, m_len_bytes = BENCHMARK_MESSAGE_LEN, sn_len_bytes;

  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    t[i] = cpucycles();
    sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
  }
  t[NUMBER_OF_TESTS] = cpucycles();
  print_results("keygen: ", t, NUMBER_OF_TESTS + 1);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    t[i] = cpucycles();
    sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes);
  }
  t[NUMBER_OF_TESTS] = cpucycles();
  print_results("sign: ", t, NUMBER_OF_TESTS + 1);

  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) {
    t[i] = cpucycles();
    sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
  }
  t[NUMBER_OF_TESTS] = cpucycles();
  print_results("verify: ", t, NUMBER_OF_TESTS + 1);

  return 0;
}
