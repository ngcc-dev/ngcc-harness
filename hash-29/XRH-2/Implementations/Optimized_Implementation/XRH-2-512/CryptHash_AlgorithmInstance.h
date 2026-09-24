/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

#include <stdint.h>

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "XRH-2-512-AVX2"
#define DIGEST_BIT_LENGTH 512

#define STATE_LANES 20
#define STATE_CAPACITY (DIGEST_BIT_LENGTH + 64)
#define STATE_RATE (STATE_LANES * 64 - DIGEST_BIT_LENGTH - 64)

typedef struct {
    uint64_t state[STATE_LANES];
    uint32_t rate_bits;
} XRH_state;

#ifdef __cplusplus
extern "C" {
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif
