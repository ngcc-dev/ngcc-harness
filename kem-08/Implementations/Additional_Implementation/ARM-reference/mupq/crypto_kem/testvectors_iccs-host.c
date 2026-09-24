// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
//
// Host-side counterpart of testvectors_iccs.c. It produces the exact same
// ICCS-format KAT body, but prints it to stdout instead of the HAL UART and
// without the start/end markers. kat.py runs this on the host to obtain a
// trusted reference, then checks the vectors generated on the target against
// it. The reference output itself is never written to disk.

#include "api.h"
#include "drng.h"
#include "KEM_AlgorithmInstance.h"
#include "randombytes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED_LEN_BYTES 64
#define NTESTS 10

// Consumed (via extern) by KEM_AlgorithmInstance.c as the KEM randomness source.
DRNG_ctx drng_algorithm;

// The scheme library references randombytes() through the non-derand
// crypto_kem_keypair/enc, which the ICCS KAT never calls. Provide a definition
// so the host link succeeds; it must never actually run.
int randombytes(uint8_t *out, size_t outlen)
{
  (void)out;
  (void)outlen;
  fprintf(stderr, "ERROR: randombytes() must not be called in the ICCS KAT\n");
  abort();
}

static void print_hex(const char *id, const unsigned char *msg, unsigned long long len)
{
  fputs(id, stdout);
  for (unsigned long long i = 0; i < len; i++)
    printf("%02X", msg[i]);
  putchar('\n');
}

static void print_uint(const char *id, unsigned long long value)
{
  printf("%s%u\n", id, (unsigned int)value);
}

int main(void)
{
  unsigned char nonce[SEED_LEN_BYTES];
  unsigned char seed[SEED_LEN_BYTES];
  DRNG_ctx drng_seed;

  static unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  static unsigned char sk[CRYPTO_SECRETKEYBYTES];
  static unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
  static unsigned char ss[CRYPTO_BYTES];
  static unsigned char ss1[CRYPTO_BYTES];
  unsigned long long pk_len, sk_len, ss_len, ct_len;

  for (int i = 0; i < SEED_LEN_BYTES / 4; i++)
    memcpy(nonce + 4 * i, "seed", 4);
  init_random_number(&drng_seed, nonce, SEED_LEN_BYTES);

  for (int i = 0; i < NTESTS; i++)
  {
    print_uint("Count = ", (unsigned long long)i);

    get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
    print_uint("Seed_Len = ", SEED_LEN_BYTES);
    print_hex("Seed = ", seed, SEED_LEN_BYTES);

    init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

    if (kem_keygen(pk, &pk_len, sk, &sk_len))
    {
      puts("ERROR: kem_keygen");
      return -1;
    }
    print_uint("PK_Len = ", pk_len);
    print_hex("PK = ", pk, pk_len);
    print_uint("SK_Len = ", sk_len);
    print_hex("SK = ", sk, sk_len);

    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len))
    {
      puts("ERROR: kem_enc");
      return -1;
    }
    print_uint("CT_Len = ", ct_len);
    print_hex("CT = ", ct, ct_len);
    print_uint("SS_Len = ", ss_len);
    print_hex("SS = ", ss, ss_len);

    if (kem_dec(sk, sk_len, ct, ct_len, ss1, &ss_len))
    {
      puts("ERROR: kem_dec");
      return -1;
    }
    if (memcmp(ss, ss1, ss_len) != 0)
    {
      puts("ERROR: shared secret mismatch");
      return -1;
    }

    putchar('\n');
  }

  return 0;
}
