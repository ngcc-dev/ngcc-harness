/* MAMBA-Viper implementation and implementation support layer where applicable. */
#include "../api.h"
#include "../viper_params.h"
#include <stdio.h>

typedef struct { int level; int pk; int ct; int sk; int ss; } expected_size_row;
static const expected_size_row expected_sizes[] = {
  {128, 608, 736, 1424, 16},
  {192, 992, 1088, 2200, 24},
  {256, 1312, 1472, 2912, 32},
  {384, 2496, 2656, 5488, 48},
  {512, 3200, 3456, 7040, 64},
};

int main(void) {
  for (size_t i = 0; i < sizeof(expected_sizes) / sizeof(expected_sizes[0]); i++) {
    if (expected_sizes[i].level == VIPER_LEVEL) {
      int ok = CRYPTO_PUBLICKEYBYTES == expected_sizes[i].pk &&
               CRYPTO_CIPHERTEXTBYTES == expected_sizes[i].ct &&
               CRYPTO_SECRETKEYBYTES == expected_sizes[i].sk &&
               CRYPTO_BYTES == expected_sizes[i].ss;
      printf("VIPER_LEVEL=%d pk=%d ct=%d sk=%d ss=%d\n", VIPER_LEVEL,
             CRYPTO_PUBLICKEYBYTES, CRYPTO_CIPHERTEXTBYTES,
             CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES);
      printf("derived pk=%d ct=%d sk=%d ss=%d\n", VIPER_EXPECTED_PUBLICKEYBYTES,
             VIPER_EXPECTED_CIPHERTEXTBYTES, VIPER_EXPECTED_SECRETKEYBYTES,
             VIPER_SSBYTES);
      return ok ? 0 : 1;
    }
  }
  return 1;
}
