/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LLH_CRYPT_HASH_SUCCESS 0
#define LLH_CRYPT_HASH_UNSUPPORTED_DIGEST -1
#define LLH_CRYPT_HASH_INVALID_ARG -2
#define HIGH_N_BIT_MASK(N) ((unsigned char)((~0U) << (8 - (N))))

#define W 128

#define hash_init llh_hash_init
#define hash_absorb llh_hash_absorb
#define hash_final llh_hash_final
#define hash llh_hash
#include "llh_core.c"

static unsigned char reverse_low_bits(unsigned char x, unsigned int n)
{
    unsigned char y = 0;

    for (unsigned int i = 0; i < n; i++) {
        y = (unsigned char)((y << 1) | (x & 1U));
        x = (unsigned char)(x >> 1);
    }

    return y;
}

/*
 * Official API entry for the current LLH instance. Byte-aligned inputs use the
 * one-shot fast path. For non-byte-aligned inputs, API/KAT provides valid tail
 * bits in the high bits of the final byte; LLH padding injects those bits in
 * low-bit order and fills the remaining high bits with ones.
 */
int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest)
{
    size_t full_bytes;
    unsigned int rem_bits;

    if (!digest) {
        return LLH_CRYPT_HASH_INVALID_ARG;
    }
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return LLH_CRYPT_HASH_UNSUPPORTED_DIGEST;
    }
    if (msg_len_bits > 0ULL && msg == NULL) {
        return LLH_CRYPT_HASH_INVALID_ARG;
    }
    if ((msg_len_bits / 8ULL) > (unsigned long long)SIZE_MAX) {
        return LLH_CRYPT_HASH_INVALID_ARG;
    }

    full_bytes = (size_t)(msg_len_bits / 8ULL);
    rem_bits = (unsigned int)(msg_len_bits & 0x7ULL);

    if (rem_bits == 0U) {
        hash(msg, full_bytes, digest);
        return LLH_CRYPT_HASH_SUCCESS;
    }

    {
        unsigned char last_byte;
        unsigned char tail;
        HashCtx ctx;

        hash_init(&ctx, (uint64_t)msg_len_bits);
        if (full_bytes > 0U) {
            hash_absorb(&ctx, msg, full_bytes);
        }

        tail = (unsigned char)(msg[full_bytes] >> (8U - rem_bits));
        tail = reverse_low_bits(tail, rem_bits);
        last_byte = (unsigned char)(tail | (unsigned char)(0xFFU << rem_bits));
        hash_absorb(&ctx, &last_byte, 1);
        hash_final(&ctx, digest);
    }

    return LLH_CRYPT_HASH_SUCCESS;
}
