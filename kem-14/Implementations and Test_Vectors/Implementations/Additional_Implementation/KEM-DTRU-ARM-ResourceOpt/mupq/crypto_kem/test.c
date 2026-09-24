// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "dtru_kem.h"
#include "hal.h"
#include "randombytes.h"
#include "sendfn.h"

#include <stdint.h>
#include <string.h>

#define NTESTS 10

const uint8_t canary[8] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

static void write_canary(uint8_t *d)
{
  for (size_t i = 0; i < 8; i++)
    d[i] = canary[i];
}

static int check_canary(const uint8_t *d)
{
  for (size_t i = 0; i < 8; i++)
    if (d[i] != canary[i])
      return -1;
  return 0;
}

static void seed_drng_from_randombytes(void)
{
  unsigned char seed[DTRU_TEST_SEED_BYTES];
  randombytes(seed, sizeof(seed));
  dtru_init_drng(seed);
}

static int test_keys(void)
{
  unsigned char key_a[DTRU_SHAREDKEYBYTES + 16], key_b[DTRU_SHAREDKEYBYTES + 16];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES + 16];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES + 16];
  unsigned char sk[DTRU_KEM_SECRETKEYBYTES + 16];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (int i = 0; i < NTESTS; i++) {
    write_canary(key_a);
    write_canary(key_a + sizeof(key_a) - 8);
    write_canary(key_b);
    write_canary(key_b + sizeof(key_b) - 8);
    write_canary(pk);
    write_canary(pk + sizeof(pk) - 8);
    write_canary(ct);
    write_canary(ct + sizeof(ct) - 8);
    write_canary(sk);
    write_canary(sk + sizeof(sk) - 8);

    kem_keygen(pk + 8, &pk_byts, sk + 8, &sk_byts);
    hal_send_str("DONE key pair generation!");

    kem_enc(pk + 8, pk_byts, key_b + 8, &ss_byts1, ct + 8, &ct_byts);
    hal_send_str("DONE encapsulation!");

    kem_dec(sk + 8, sk_byts, ct + 8, ct_byts, key_a + 8, &ss_byts2);
    hal_send_str("DONE decapsulation!");

    if (memcmp(key_a + 8, key_b + 8, DTRU_SHAREDKEYBYTES)) {
      hal_send_str("ERROR KEYS\n");
      return -1;
    }
    if (check_canary(key_a) || check_canary(key_a + sizeof(key_a) - 8) ||
        check_canary(key_b) || check_canary(key_b + sizeof(key_b) - 8) ||
        check_canary(pk) || check_canary(pk + sizeof(pk) - 8) ||
        check_canary(ct) || check_canary(ct + sizeof(ct) - 8) ||
        check_canary(sk) || check_canary(sk + sizeof(sk) - 8)) {
      hal_send_str("ERROR canary overwritten\n");
      return -1;
    }

    hal_send_str("OK KEYS\n");
    hal_send_str("+");
  }
  return 0;
}

static int test_invalid_sk(void)
{
  unsigned char sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char key_a[DTRU_SHAREDKEYBYTES], key_b[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (int i = 0; i < NTESTS; i++) {
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    kem_enc(pk, pk_byts, key_b, &ss_byts1, ct, &ct_byts);
    randombytes(sk, DTRU_KEM_SECRETKEYBYTES);
    kem_dec(sk, sk_byts, ct, ct_byts, key_a, &ss_byts2);

    if (!memcmp(key_a, key_b, DTRU_SHAREDKEYBYTES)) {
      hal_send_str("ERROR invalid sk\n");
      return -1;
    }
    hal_send_str("OK invalid sk\n");
    hal_send_str("+");
  }
  return 0;
}

static int test_invalid_ciphertext(void)
{
  unsigned char sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char key_a[DTRU_SHAREDKEYBYTES], key_b[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  for (int i = 0; i < NTESTS; i++) {
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    kem_enc(pk, pk_byts, key_b, &ss_byts1, ct, &ct_byts);
    randombytes(ct, sizeof(ct));
    kem_dec(sk, sk_byts, ct, ct_byts, key_a, &ss_byts2);

    if (!memcmp(key_a, key_b, DTRU_SHAREDKEYBYTES)) {
      hal_send_str("ERROR invalid ciphertext\n");
      return -1;
    }
    hal_send_str("OK invalid ciphertext\n");
    hal_send_str("+");
  }
  return 0;
}

int main(void)
{
  hal_setup(CLOCK_BENCHMARK);

  hal_send_str("==========================");
  send_unsigned("DTRU-", DTRU_N);
  send_unsigned("q=", DTRU_Q);
  seed_drng_from_randombytes();

  if (test_keys() || test_invalid_sk() || test_invalid_ciphertext())
    hal_send_str("ERROR\n");

  hal_send_str("#");
  return 0;
}
