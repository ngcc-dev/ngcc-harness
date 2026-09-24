// The open-source code from https://github.com/jedisct1/vistrutah

#include "vistrutah.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void print_hex(const char* label, const uint8_t* data, size_t len) {
  printf("%s: ", label);
  for (size_t i = 0; i < len; ++i) {
    printf("%02X", data[i]);
  }
  printf("\n");
}

// Keep only Vistrutah-512 / 512-bit-key / long(18-round) vector.
static int test_vector_512_512_long(void) {
  uint8_t key[64];
  uint8_t plaintext[64];
  uint8_t ciphertext[64];

  for (int i = 0; i < 64; ++i) {
    key[i] = (uint8_t)i;
    plaintext[i] = (uint8_t)i;
  }

  static const uint8_t expected_ciphertext[64] = {
      0xCA, 0xD5, 0x1A, 0x08, 0x8C, 0x27, 0x8D, 0xBA, 0xF3, 0x7F, 0x2A,
      0xDB, 0xE2, 0xF9, 0x72, 0xBC, 0x9A, 0x3F, 0xBF, 0x24, 0x2F, 0xEB,
      0x02, 0x3F, 0x83, 0x02, 0x9D, 0xF8, 0x88, 0x11, 0x55, 0x54, 0x76,
      0x02, 0xD5, 0x9D, 0xBE, 0x95, 0x21, 0x4C, 0x6F, 0x68, 0xE1, 0x90,
      0xBE, 0xDB, 0x96, 0x3A, 0x57, 0x63, 0xCB, 0x6C, 0x48, 0x25, 0x1E,
      0xB1, 0xAC, 0xA8, 0x77, 0xCA, 0xEA, 0xE4, 0xA7, 0xB2};

  vistrutah_512_encrypt(plaintext, ciphertext, key);

  if (memcmp(ciphertext, expected_ciphertext, sizeof(ciphertext)) != 0) {
    printf("FAIL: ciphertext mismatch with 512-512 long test vector\n");
    print_hex("expected  ", expected_ciphertext, sizeof(expected_ciphertext));
    print_hex("got       ", ciphertext, sizeof(ciphertext));
    return 1;
  }

  printf("PASS: 512-512 long test vector\n");
  print_hex("ciphertext", ciphertext, sizeof(ciphertext));
  return 0;
}

int main(void) {
  printf("Vistrutah test: 512-512 long only\n");
  printf("Implementation: %s\n", vistrutah_get_impl_name());
  return test_vector_512_512_long();
}
