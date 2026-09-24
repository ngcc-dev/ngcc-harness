#define _POSIX_C_SOURCE 200809L

#include "utils_sm4/hygon_cis_sm4.h"

#include <stdio.h>

int main(void) {
  if (!hygon_cis_sm4_is_supported()) {
    printf("Hygon CIS SM4 probe: unsupported\n");
    return 77;
  }

  printf("Hygon CIS SM4 probe: ok\n");
  return 0;
}
