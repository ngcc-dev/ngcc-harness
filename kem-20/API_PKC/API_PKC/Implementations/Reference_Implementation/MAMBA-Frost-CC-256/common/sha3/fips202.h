#ifndef FIPS202_H
#define FIPS202_H

void shake128(unsigned char *output, unsigned long long outlen,
              const unsigned char *input, unsigned long long inlen);
void shake256(unsigned char *output, unsigned long long outlen,
              const unsigned char *input, unsigned long long inlen);

#endif
