#include "CryptHash_AlgorithmInstance.h"

#include "axis_core.h"
/* Function: CryptHash. ICCS hash API entry point for this AXIS instance; validates the requested digest length and hashes a bit string message. */

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest) {
    if (digest == 0) {
        return -1;
    }
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return -2;
    }
    if (msg_len_bits > 0ULL && msg == 0) {
        return -3;
    }

    axis_core_hash_bits(AXIS_VARIANT_1024, msg, msg_len_bits, digest);
    return 0;
}
