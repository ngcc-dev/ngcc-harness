#ifndef SM3_H
#define SM3_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM3_OUTLEN 32

void sm3(uint8_t *out, const uint8_t *in, size_t inlen);

void sm3_kdf(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

#ifdef __cplusplus
}
#endif

#endif /* SM3_H */
