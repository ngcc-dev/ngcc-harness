#ifndef WCHAIN_C_H
#define WCHAIN_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void wchain_v1_hash_c(const uint8_t *msg, size_t len, uint8_t digest[64]);
void wchain_v2_hash_c(const uint8_t *msg, size_t len, uint8_t digest[128]);
void wchain_v1_hash_bits_c(const uint8_t *msg, unsigned long long msg_len_bits, uint8_t digest[64]);
void wchain_v2_hash_bits_c(const uint8_t *msg, unsigned long long msg_len_bits, uint8_t digest[128]);
int wchain_crypt_hash_c(int digest_len_bits, const unsigned char *msg,
                        unsigned long long msg_len_bits, unsigned char *digest);

void wchain_v1_compress_c(uint64_t out[9], const uint64_t v[9], const uint8_t block[144]);
void wchain_v2_compress_c(uint64_t out[18], const uint64_t v[18], const uint8_t block[288]);

#if !defined(WCHAIN_DISABLE_SIMD)
void wchain_v1_compress4_avx2(uint64_t out[4][9], uint64_t v[4][9], uint8_t blocks[4][144]);
void wchain_v2_compress4_avx2(uint64_t out[4][18], uint64_t v[4][18], uint8_t blocks[4][288]);
void wchain_v1_compress8_avx512(uint64_t out[8][9], uint64_t v[8][9], uint8_t blocks[8][144]);
void wchain_v2_compress8_avx512(uint64_t out[8][18], uint64_t v[8][18], uint8_t blocks[8][288]);
#endif

#ifdef __cplusplus
}
#endif

#endif
