/* CryptHash() at any byte-aligned output length for a submitted XOF whose own
 * CryptHash ignores digest_len_bits and always produces DIGEST_BIT_LENGTH bits
 * (Laurus-XOF, hash-14; litchi_xof, hash-15). The submitted
 * CryptHash_AlgorithmInstance.c is compiled unchanged with its CryptHash
 * renamed; this one passes the requested length to the submitted XOF function
 * (-DXOF_FUNCTION=laurus_xof|litchi_xof; lengths in bits). supplement.py checks
 * that at DIGEST_BIT_LENGTH the output equals the submitted library's. */
#define CryptHash submitted_fixed_width_CryptHash
#include "CryptHash_AlgorithmInstance.c"
#undef CryptHash

#include "iccs_api.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits,
              unsigned char *digest)
{
    if (digest_len_bits <= 0 || digest_len_bits % 8)
        return -1;
    XOF_FUNCTION(digest, (uint64_t)digest_len_bits, msg, (uint64_t)msg_len_bits);
    return 0;
}
