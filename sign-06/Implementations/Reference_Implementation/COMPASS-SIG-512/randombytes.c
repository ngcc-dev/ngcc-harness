#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "randombytes.h"

/* Minimal OS-entropy randombytes for auxiliary test programs only.
   This is NOT used by KAT generation (which uses deterministic DRNG).
   Uses /dev/urandom on Linux, CryptGenRandom on Windows. */

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
void randombytes(uint8_t *out, size_t outlen) {
  HCRYPTPROV ctx;
  size_t len;
  if(!CryptAcquireContext(&ctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    abort();
  while(outlen > 0) {
    len = (outlen > 1048576) ? 1048576 : outlen;
    if(!CryptGenRandom(ctx, len, (BYTE *)out))
      abort();
    out += len;
    outlen -= len;
  }
  if(!CryptReleaseContext(ctx, 0))
    abort();
}
#else
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
void randombytes(uint8_t *out, size_t outlen) {
  static int fd = -1;
  ssize_t ret;
  while(fd == -1) {
    fd = open("/dev/urandom", O_RDONLY);
    if(fd == -1 && errno == EINTR)
      continue;
    else if(fd == -1)
      abort();
  }
  while(outlen > 0) {
    ret = read(fd, out, outlen);
    if(ret == -1 && errno == EINTR)
      continue;
    else if(ret == -1)
      abort();
    out += ret;
    outlen -= ret;
  }
}
#endif
