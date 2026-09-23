#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"
#include "params.h"

#define SEED_LEN_BYTES 64
#define MAX_LINE_LEN 8192

DRNG_ctx drng_algorithm;

static int hex_char_to_nibble(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static void hex_decode(unsigned char *dst, const char *src, unsigned long long len)
{
  for (unsigned long long i = 0; i < len; i++)
    dst[i] = (hex_char_to_nibble(src[2 * i]) << 4) | hex_char_to_nibble(src[2 * i + 1]);
}

static void test_correctness_from_file(const char *file_path)
{
  FILE *fp = fopen(file_path, "r");
  if (!fp)
  {
    printf("ERROR: Cannot open file: %s\n\n", file_path);
    return;
  }

  char line[MAX_LINE_LEN];
  unsigned char *seed = NULL, *pk_exp = NULL, *sk_exp = NULL, *ct_exp = NULL, *ss_exp = NULL;
  unsigned char *pk = NULL, *sk = NULL, *ct = NULL, *ss_enc = NULL, *ss_dec = NULL;
  unsigned long long pk_len = 0, sk_len = 0, ct_len = 0, ss_len = 0;
  unsigned long long seed_len = 0;
  unsigned long long pk_byts, sk_byts, ct_byts, ss_byts;
  int count = -1, pass = 0, fail = 0;
  int have_seed = 0, have_pk = 0, have_sk = 0, have_ct = 0, have_ss = 0;

  while (fgets(line, sizeof(line), fp))
  {
    size_t line_len = strlen(line);
    while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
      line[--line_len] = '\0';

    if (strncmp(line, "Count = ", 8) == 0)
    {
      count = atoi(line + 8);
      have_seed = have_pk = have_sk = have_ct = have_ss = 0;
      seed_len = pk_len = sk_len = ct_len = ss_len = 0;
      free(seed); free(pk_exp); free(sk_exp); free(ct_exp); free(ss_exp);
      free(pk); free(sk); free(ct); free(ss_enc); free(ss_dec);
      seed = pk_exp = sk_exp = ct_exp = ss_exp = NULL;
      pk = sk = ct = ss_enc = ss_dec = NULL;
    }
    else if (strncmp(line, "Seed = ", 7) == 0)
    {
      seed_len = SEED_LEN_BYTES;
      seed = (unsigned char *)malloc(seed_len);
      hex_decode(seed, line + 7, seed_len);
      have_seed = 1;
    }
    else if (strncmp(line, "PK = ", 5) == 0)
    {
      char *hex = line + 5;
      pk_len = strlen(hex) / 2;
      pk_exp = (unsigned char *)malloc(pk_len);
      hex_decode(pk_exp, hex, pk_len);
      have_pk = 1;
    }
    else if (strncmp(line, "SK = ", 5) == 0)
    {
      char *hex = line + 5;
      sk_len = strlen(hex) / 2;
      sk_exp = (unsigned char *)malloc(sk_len);
      hex_decode(sk_exp, hex, sk_len);
      have_sk = 1;
    }
    else if (strncmp(line, "CT = ", 5) == 0)
    {
      char *hex = line + 5;
      ct_len = strlen(hex) / 2;
      ct_exp = (unsigned char *)malloc(ct_len);
      hex_decode(ct_exp, hex, ct_len);
      have_ct = 1;
    }
    else if (strncmp(line, "SS = ", 5) == 0)
    {
      char *hex = line + 5;
      ss_len = strlen(hex) / 2;
      ss_exp = (unsigned char *)malloc(ss_len);
      hex_decode(ss_exp, hex, ss_len);
      have_ss = 1;
    }

    if (have_seed && have_pk && have_sk && have_ct && have_ss)
    {
      int record_pass = 1;

      init_random_number(&drng_algorithm, seed, seed_len);

      pk = (unsigned char *)malloc(pk_len);
      sk = (unsigned char *)malloc(sk_len);
      ss_enc = (unsigned char *)malloc(ss_len);
      ct = (unsigned char *)malloc(ct_len);
      ss_dec = (unsigned char *)malloc(ss_len);

      if (kem_keygen(pk, &pk_byts, sk, &sk_byts))
      {
        printf("Count %d: kem_keygen failed\n", count);
        record_pass = 0;
      }
      else if (pk_byts != pk_len || memcmp(pk, pk_exp, pk_len))
      {
        printf("Count %d: PK mismatch\n", count);
        record_pass = 0;
      }
      else if (sk_byts != sk_len || memcmp(sk, sk_exp, sk_len))
      {
        printf("Count %d: SK mismatch\n", count);
        record_pass = 0;
      }

      if (record_pass && kem_enc(pk_exp, pk_len, ss_enc, &ss_byts, ct, &ct_byts))
      {
        printf("Count %d: kem_enc failed\n", count);
        record_pass = 0;
      }
      else if (record_pass && (ct_byts != ct_len || memcmp(ct, ct_exp, ct_len)))
      {
        printf("Count %d: CT mismatch\n", count);
        record_pass = 0;
      }
      else if (record_pass && (ss_byts != ss_len || memcmp(ss_enc, ss_exp, ss_len)))
      {
        printf("Count %d: SS mismatch\n", count);
        record_pass = 0;
      }

      if (record_pass && kem_dec(sk_exp, sk_len, ct_exp, ct_len, ss_dec, &ss_byts))
      {
        printf("Count %d: kem_dec failed\n", count);
        record_pass = 0;
      }
      else if (record_pass && (ss_byts != ss_len || memcmp(ss_dec, ss_exp, ss_len)))
      {
        printf("Count %d: decapsulated SS mismatch\n", count);
        record_pass = 0;
      }

      if (record_pass)
        pass++;
      else
        fail++;

      have_seed = have_pk = have_sk = have_ct = have_ss = 0;
    }
  }

  free(seed); free(pk_exp); free(sk_exp); free(ct_exp); free(ss_exp);
  free(pk); free(sk); free(ct); free(ss_enc); free(ss_dec);
  fclose(fp);

  if (fail == 0)
    printf("kem test pass (%d/%d)\n\n", pass, pass + fail);
  else
    printf("kem test fail (%d/%d)\n\n", pass, pass + fail);
}

void test_correctness(void)
{
  if (PK_PACK_OPT == 1)
  {
    printf("=== test_correctness (Test_Vectors_PACK_PK) ===\n");
    test_correctness_from_file("../../../Test_Vectors_PACK_PK/KAT_KEM_" ALGORITHM_INSTANCE ".txt");
  }
  else
  {
    printf("=== test_correctness (Test_Vectors) ===\n");
    test_correctness_from_file("../../../Test_Vectors/KAT_KEM_" ALGORITHM_INSTANCE ".txt");
  }
}

int main(void)
{
  test_correctness();
  return 0;
}
