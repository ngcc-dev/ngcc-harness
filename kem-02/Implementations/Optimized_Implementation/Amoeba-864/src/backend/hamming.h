#ifndef HAMMING_H
#define HAMMING_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Basic Parameters
#define ECC_R 10    // Hamming order
#define ECC_L 512   // message length

// Hamming code
#define ECC_N ((1U << ECC_R) - 1)   // n = 2^r - 1
#define ECC_K (ECC_N - ECC_R)       // k = 2^r - r - 1

// Truncated extended Hamming code
#define ECC_CODE_LEN (ECC_L + ECC_R + 1)
#define ECC_SYNDROME_LEN (ECC_R + 1)

int build_trunc_ext_hamming(int r, int l);

void encode_ECC(const uint8_t x[ECC_L], uint8_t c[ECC_CODE_LEN]);

int decode_ECC(const uint8_t c[ECC_CODE_LEN], uint8_t x[ECC_L]);

#ifdef __cplusplus
}
#endif

#endif // HAMMING_H