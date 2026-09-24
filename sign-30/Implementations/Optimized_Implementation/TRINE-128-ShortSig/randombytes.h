#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#define RANDOMBYTES_SUCCESS 0
#define RANDOMBYTES_BAD_OUTPUT -1
#define RANDOMBYTES_BAD_LENGTH -2
#define RANDOMBYTES_PROVIDER_FAILURE -3

int randombytes(unsigned char *out, unsigned long long outlen);

#endif
