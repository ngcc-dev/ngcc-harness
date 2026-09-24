#ifndef VIPER_API_HASH_H
#define VIPER_API_HASH_H
void shake128(unsigned char *output, unsigned long long outlen, const unsigned char *input, unsigned long long inlen);
void shake256(unsigned char *output, unsigned long long outlen, const unsigned char *input, unsigned long long inlen);
void sha3_256(unsigned char *output, const unsigned char *input, unsigned long long inlen);
void sha3_512(unsigned char *output, const unsigned char *input, unsigned long long inlen);
#endif
