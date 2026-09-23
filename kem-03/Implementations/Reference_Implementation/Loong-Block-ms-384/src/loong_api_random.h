#ifndef LOONG_API_RANDOM_H
#define LOONG_API_RANDOM_H

int loong_api_random_bits(unsigned char *out, unsigned long long out_len_bits);
int loong_api_random_bytes(unsigned char *out, unsigned long long out_len_bytes);

#endif
