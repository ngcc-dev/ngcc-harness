// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#include "dtru_kem.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 2

typedef uint32_t uint32;

static uint32 seed[32] = {
    3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3,
    2, 3, 8, 4, 6, 2, 6, 4, 3, 3, 8, 3, 2, 7, 9, 5};
static uint32 in[12];
static uint32 out[8];
static int outleft = 0;

#define ROTATE(x, b) (((x) << (b)) | ((x) >> (32 - (b))))
#define MUSH(i, b) x = t[i] += (((x ^ seed[i]) + sum) ^ ROTATE(x, b));

static void surf(void)
{
  uint32 t[12];
  uint32 x;
  uint32 sum = 0;

  for (int i = 0; i < 12; ++i)
    t[i] = in[i] ^ seed[12 + i];
  for (int i = 0; i < 8; ++i)
    out[i] = seed[24 + i];
  x = t[11];
  for (int loop = 0; loop < 2; ++loop) {
    for (int r = 0; r < 16; ++r) {
      sum += 0x9e3779b9;
      MUSH(0, 5) MUSH(1, 7) MUSH(2, 9) MUSH(3, 13)
      MUSH(4, 5) MUSH(5, 7) MUSH(6, 9) MUSH(7, 13)
      MUSH(8, 5) MUSH(9, 7) MUSH(10, 9) MUSH(11, 13)
    }
    for (int i = 0; i < 8; ++i)
      out[i] ^= t[i + 4];
  }
}

int randombytes(uint8_t *x, size_t xlen)
{
  while (xlen > 0) {
    if (!outleft) {
      if (!++in[0])
        if (!++in[1])
          if (!++in[2])
            ++in[3];
      surf();
      outleft = 8;
    }
    *x = out[--outleft];
    ++x;
    --xlen;
  }
  return 0;
}

static void printbytes(const unsigned char *x, unsigned long long xlen)
{
  for (unsigned long long i = 0; i < xlen; i++)
    printf("%02x", x[i]);
  putchar('\n');
}

static void seed_drng_from_randombytes(void)
{
  unsigned char seed[DTRU_TEST_SEED_BYTES];
  randombytes(seed, sizeof(seed));
  dtru_init_drng(seed);
}

int main(void)
{
  unsigned char key_a[DTRU_SHAREDKEYBYTES], key_b[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned char sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;

  seed_drng_from_randombytes();

  for (int i = 0; i < NTESTS; i++) {
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    printbytes(pk, pk_byts);
    printbytes(sk, sk_byts);

    kem_enc(pk, pk_byts, key_b, &ss_byts1, ct, &ct_byts);
    printbytes(ct, ct_byts);
    printbytes(key_b, ss_byts1);

    kem_dec(sk, sk_byts, ct, ct_byts, key_a, &ss_byts2);
    printbytes(key_a, ss_byts2);

    if (memcmp(key_a, key_b, DTRU_SHAREDKEYBYTES)) {
      puts("ERROR");
      return -1;
    }
  }

  return 0;
}
