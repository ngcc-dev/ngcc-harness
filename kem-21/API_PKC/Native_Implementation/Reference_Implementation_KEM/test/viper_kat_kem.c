/* Clean Viper KEM KAT harness for deterministic implementation testing. */

#include "../api.h"
#include "../rng.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NTESTS 100

static void fprintBstr(FILE *fp, const char *label, const unsigned char *buf, size_t len)
{
  size_t i;
  fprintf(fp, "%s", label);
  for (i = 0; i < len; i++) {
    fprintf(fp, "%02X", buf[i]);
  }
  if (len == 0) {
    fprintf(fp, "00");
  }
  fprintf(fp, "\n");
}

int main(void)
{
  char req_name[64];
  char rsp_name[64];
  int pkct_len = CRYPTO_PUBLICKEYBYTES + CRYPTO_CIPHERTEXTBYTES;
  snprintf(req_name, sizeof(req_name), "PQCkemKAT_%d.req", pkct_len);
  snprintf(rsp_name, sizeof(rsp_name), "PQCkemKAT_%d.rsp", pkct_len);
  FILE *fp_req = NULL;
  FILE *fp_rsp = NULL;
  unsigned char entropy_input[48];
  unsigned char seeds[NTESTS][48];
  unsigned char seed[48];
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk[CRYPTO_SECRETKEYBYTES];
  unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
  unsigned char ss[CRYPTO_BYTES];
  unsigned char ss1[CRYPTO_BYTES];
  int i;

  for (i = 0; i < 48; i++) {
    entropy_input[i] = (unsigned char)i;
  }

  fp_req = fopen(req_name, "w");
  if (fp_req == NULL) {
    fprintf(stderr, "Failed to open %s for write\n", req_name);
    return 1;
  }
  fp_rsp = fopen(rsp_name, "w");
  if (fp_rsp == NULL) {
    fclose(fp_req);
    fprintf(stderr, "Failed to open %s for write\n", rsp_name);
    return 1;
  }

  randombytes_init(entropy_input, NULL, 256);
  for (i = 0; i < NTESTS; i++) {
    fprintf(fp_req, "count = %d\n", i);
    randombytes(seed, sizeof(seed));
    memcpy(seeds[i], seed, sizeof(seed));
    fprintBstr(fp_req, "seed = ", seed, sizeof(seed));
    fprintf(fp_req, "pk =\n");
    fprintf(fp_req, "sk =\n");
    fprintf(fp_req, "ct =\n");
    fprintf(fp_req, "ss =\n\n");
  }
  fclose(fp_req);

  fprintf(fp_rsp, "# %s\n\n", CRYPTO_ALGNAME);
  for (i = 0; i < NTESTS; i++) {
    fprintf(fp_rsp, "count = %d\n", i);
    memcpy(seed, seeds[i], sizeof(seed));
    fprintBstr(fp_rsp, "seed = ", seed, sizeof(seed));

    randombytes_init(seed, NULL, 256);

    if (crypto_kem_keypair(pk, sk) != 0) {
      fprintf(stderr, "crypto_kem_keypair failed at count %d\n", i);
      fclose(fp_rsp);
      return 2;
    }
    if (crypto_kem_enc(ct, ss, pk) != 0) {
      fprintf(stderr, "crypto_kem_enc failed at count %d\n", i);
      fclose(fp_rsp);
      return 3;
    }
    if (crypto_kem_dec(ss1, ct, sk) != 0) {
      fprintf(stderr, "crypto_kem_dec failed at count %d\n", i);
      fclose(fp_rsp);
      return 4;
    }
    if (memcmp(ss, ss1, CRYPTO_BYTES) != 0) {
      fprintf(stderr, "shared secret mismatch at count %d\n", i);
      fclose(fp_rsp);
      return 5;
    }

    fprintBstr(fp_rsp, "pk = ", pk, CRYPTO_PUBLICKEYBYTES);
    fprintBstr(fp_rsp, "sk = ", sk, CRYPTO_SECRETKEYBYTES);
    fprintBstr(fp_rsp, "ct = ", ct, CRYPTO_CIPHERTEXTBYTES);
    fprintBstr(fp_rsp, "ss = ", ss, CRYPTO_BYTES);
    fprintf(fp_rsp, "\n");
  }

  fclose(fp_rsp);
  return 0;
}
