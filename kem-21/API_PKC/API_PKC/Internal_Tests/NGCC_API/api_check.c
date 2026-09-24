#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

#ifndef EXPECTED_PK
#error EXPECTED_PK required
#endif

static int error_in_range(int r) { return r >= -99 && r <= -1; }
static int seed_drng(void) {
  unsigned char seed[64];
  for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)(i + 1U);
  return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

int main(void) {
  if (kem_get_pk_len_bytes() != EXPECTED_PK || kem_get_sk_len_bytes() != EXPECTED_SK ||
      kem_get_ss_len_bytes() != EXPECTED_SS || kem_get_ct_len_bytes() != EXPECTED_CT) {
    fprintf(stderr, "length query mismatch\n"); return 1;
  }
  unsigned char *pk = calloc(EXPECTED_PK, 1), *sk = calloc(EXPECTED_SK, 1);
  unsigned char *ss = calloc(EXPECTED_SS, 1), *ss2 = calloc(EXPECTED_SS, 1), *badss = calloc(EXPECTED_SS, 1);
  unsigned char *ct = calloc(EXPECTED_CT, 1), *badct = calloc(EXPECTED_CT, 1);
  unsigned long long pk_len=0, sk_len=0, ss_len=0, ct_len=0, badss_len=0;
  if (!pk || !sk || !ss || !ss2 || !badss || !ct || !badct) return 2;
  if (seed_drng()) return 3;
  if (kem_keygen(pk, &pk_len, sk, &sk_len) || pk_len != EXPECTED_PK || sk_len != EXPECTED_SK) return 4;
  if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) || ss_len != EXPECTED_SS || ct_len != EXPECTED_CT) return 5;
  if (kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len) || ss_len != EXPECTED_SS) return 6;
  if (memcmp(ss, ss2, EXPECTED_SS)) return 7;
  memcpy(badct, ct, EXPECTED_CT); badct[0] ^= 1;
  int dec_bad = kem_dec(sk, sk_len, badct, ct_len, badss, &badss_len);
  if (dec_bad != 0 || badss_len != EXPECTED_SS) { fprintf(stderr, "implicit rejection failure r=%d len=%llu\n", dec_bad, badss_len); return 8; }
  if (!error_in_range(kem_enc(pk, pk_len + 1, ss, &ss_len, ct, &ct_len))) return 9;
  if (!error_in_range(kem_dec(sk, sk_len + 1, ct, ct_len, ss2, &ss_len))) return 10;
  if (!error_in_range(kem_dec(sk, sk_len, ct, ct_len + 1, ss2, &ss_len))) return 11;
  if (!error_in_range(kem_keygen(NULL, &pk_len, sk, &sk_len))) return 12;
  if (!error_in_range(kem_enc(NULL, pk_len, ss, &ss_len, ct, &ct_len))) return 13;
  if (!error_in_range(kem_dec(NULL, sk_len, ct, ct_len, ss2, &ss_len))) return 14;
  free(pk); free(sk); free(ss); free(ss2); free(badss); free(ct); free(badct);
  puts("api_check PASS");
  return 0;
}
