// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "dtru_kem.h"
#include "hal.h"
#include "randombytes.h"
#include "sendfn.h"

#include <string.h>

#define send_stack_usage(S, U) send_unsigned((S), (U))

unsigned char key_a[DTRU_SHAREDKEYBYTES], key_b[DTRU_SHAREDKEYBYTES];
unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;
unsigned int stack_keypair, stack_encaps, stack_decaps;

static void seed_drng_from_randombytes(void)
{
  unsigned char seed[DTRU_TEST_SEED_BYTES];
  randombytes(seed, sizeof(seed));
  dtru_init_drng(seed);
}

static int test_keys(void)
{
  hal_spraystack();
  kem_keygen(pk, &pk_byts, sk, &sk_byts);
  stack_keypair = hal_checkstack();

  hal_spraystack();
  kem_enc(pk, pk_byts, key_b, &ss_byts1, ct, &ct_byts);
  stack_encaps = hal_checkstack();

  hal_spraystack();
  kem_dec(sk, sk_byts, ct, ct_byts, key_a, &ss_byts2);
  stack_decaps = hal_checkstack();

  if (memcmp(key_a, key_b, DTRU_SHAREDKEYBYTES))
    return -1;

  send_stack_usage("keypair stack usage:", stack_keypair);
  send_stack_usage("encaps stack usage:", stack_encaps);
  send_stack_usage("decaps stack usage:", stack_decaps);
  hal_send_str("OK KEYS\n");
  return 0;
}

int main(void)
{
  hal_setup(CLOCK_FAST);
  hal_send_str("==========================");
  seed_drng_from_randombytes();

  for (int i = 0; i < MUPQ_ITERATIONS; i++) {
    if (test_keys())
      hal_send_str("ERROR KEYS\n");
    hal_send_str("+");
  }

  hal_send_str("#");
  return 0;
}
