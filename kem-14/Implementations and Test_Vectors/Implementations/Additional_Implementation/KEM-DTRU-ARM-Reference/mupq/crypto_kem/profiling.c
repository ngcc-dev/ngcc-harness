// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "dtru_kem.h"
#include "hal.h"
#include "randombytes.h"
#include "sendfn.h"

#include <string.h>

#define printcycles(S, U) send_unsignedll((S), (U))

static void seed_drng_from_randombytes(void)
{
  unsigned char seed[DTRU_TEST_SEED_BYTES];
  randombytes(seed, sizeof(seed));
  dtru_init_drng(seed);
}

static void print_zero_hash_cycles(const char *op)
{
  char label[32];
  int i = 0;

  while (*op && i < (int)sizeof(label) - 14)
    label[i++] = *op++;
  label[i++] = ' ';
  label[i++] = 'h';
  label[i++] = 'a';
  label[i++] = 's';
  label[i++] = 'h';
  label[i++] = ' ';
  label[i++] = 'c';
  label[i++] = 'y';
  label[i++] = 'c';
  label[i++] = 'l';
  label[i++] = 'e';
  label[i++] = 's';
  label[i++] = ':';
  label[i] = 0;
  printcycles(label, 0);
}

int main(void)
{
  unsigned char key_a[DTRU_SHAREDKEYBYTES], key_b[DTRU_SHAREDKEYBYTES];
  unsigned char sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;
  unsigned long long t0, t1;

  hal_setup(CLOCK_BENCHMARK);
  hal_send_str("==========================");
  seed_drng_from_randombytes();

  for (int i = 0; i < MUPQ_ITERATIONS; i++) {
    t0 = hal_get_time();
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    t1 = hal_get_time();
    printcycles("keypair cycles:", t1 - t0);
    print_zero_hash_cycles("keypair");

    t0 = hal_get_time();
    kem_enc(pk, pk_byts, key_a, &ss_byts1, ct, &ct_byts);
    t1 = hal_get_time();
    printcycles("encaps cycles:", t1 - t0);
    print_zero_hash_cycles("encaps");

    t0 = hal_get_time();
    kem_dec(sk, sk_byts, ct, ct_byts, key_b, &ss_byts2);
    t1 = hal_get_time();
    printcycles("decaps cycles:", t1 - t0);
    print_zero_hash_cycles("decaps");

    if (memcmp(key_a, key_b, DTRU_SHAREDKEYBYTES))
      hal_send_str("ERROR KEYS\n");
    else
      hal_send_str("OK KEYS\n");
    hal_send_str("+");
  }

  hal_send_str("#");
  return 0;
}
