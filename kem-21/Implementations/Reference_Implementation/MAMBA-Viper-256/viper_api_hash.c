#include "viper_api_hash.h"
#include "auxfunc.h"
#include <stdlib.h>
#include <string.h>

static void ds_xof(unsigned char *out, unsigned long long outlen, const unsigned char *in, unsigned long long inlen, const char *label) {
  const unsigned long long label_len = (unsigned long long)strlen(label);
  unsigned char stack_buf[256];
  unsigned char *buf = stack_buf;
  unsigned long long total = label_len + inlen;
  if (total > sizeof(stack_buf)) {
    buf = (unsigned char *)malloc((size_t)total);
    if (!buf) { pseudoXOF(outlen * 8ULL, in, inlen * 8ULL, out); return; }
  }
  memcpy(buf, label, (size_t)label_len);
  if (inlen) memcpy(buf + label_len, in, (size_t)inlen);
  pseudoXOF(outlen * 8ULL, buf, total * 8ULL, out);
  if (buf != stack_buf) free(buf);
}
void shake128(unsigned char *output, unsigned long long outlen, const unsigned char *input, unsigned long long inlen) { ds_xof(output, outlen, input, inlen, "Viper-shake128"); }
void shake256(unsigned char *output, unsigned long long outlen, const unsigned char *input, unsigned long long inlen) { ds_xof(output, outlen, input, inlen, "Viper-shake256"); }
void sha3_256(unsigned char *output, const unsigned char *input, unsigned long long inlen) { if (sm3hash(256, input, inlen * 8ULL, output)) pseudoXOF(256, input, inlen * 8ULL, output); }
void sha3_512(unsigned char *output, const unsigned char *input, unsigned long long inlen) { pseudohash(512, input, inlen * 8ULL, output); }
