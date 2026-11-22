/* CryptHash() at any byte-aligned output length over C Hash's Lane-Dedicated
 * XOF for short messages (hash-03, specification Appendix B). The submitted
 * reference code defines C_HASH_LDXOF() but the ICCS CryptHash entry point
 * never calls it, so this adapter is outside the submitted interface. The
 * submitted CryptHash_AlgorithmInstance.c is compiled unchanged, with its own
 * CryptHash renamed. -DCHASH_LDXOF_VARIANT=512|1024 selects the lane.
 * LDXOF accepts messages shorter than 1472 bits and outputs below 2^24 bytes. */
#define CryptHash chash_fixed_length_CryptHash
#include "CryptHash_AlgorithmInstance.c"
#undef CryptHash

#include "iccs_api.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits,
              unsigned char *digest)
{
    if (digest_len_bits <= 0 || digest_len_bits % 8)
        return -1;
    size_t want = (size_t)digest_len_bits / 8;
    size_t got = C_HASH_LDXOF(CHASH_LDXOF_VARIANT, msg, (uint64_t)msg_len_bits, (uint32_t)want, digest);
    return got == want ? 0 : -1;
}
