/* CryptHash() at any byte-aligned output length, for the XOF reference rows:
 *
 *   -DXOF_PSEUDOXOF   ICCS pseudoXOF (KDF-SM3) from api/auxfunc.c, unchanged
 *                     (performance/iccs builds it only at fixed widths)
 *   -DXOF_SHAKE128    SHAKE128 \  pq-crystals fips202.c (upstream/kyber/ref),
 *   -DXOF_SHAKE256    SHAKE256 /  the Keccak code inside the ML-KEM reference
 *
 * digest_len_bits is the requested output length. Inputs and outputs must be
 * whole bytes. */
#include <stddef.h>
#include <stdint.h>
#include "iccs_api.h"

#if defined(XOF_PSEUDOXOF)
#include "auxfunc.h"
#elif defined(XOF_SHAKE128) || defined(XOF_SHAKE256)
#include "fips202.h"
#else
#error "define XOF_PSEUDOXOF, XOF_SHAKE128 or XOF_SHAKE256"
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits,
              unsigned char *digest)
{
    if (digest_len_bits <= 0 || digest_len_bits % 8 || msg_len_bits % 8)
        return -1;
#if defined(XOF_PSEUDOXOF)
    return pseudoXOF((unsigned long long)digest_len_bits, msg, msg_len_bits, digest);
#elif defined(XOF_SHAKE128)
    shake128(digest, (size_t)digest_len_bits / 8, msg, (size_t)(msg_len_bits / 8));
    return 0;
#else
    shake256(digest, (size_t)digest_len_bits / 8, msg, (size_t)(msg_len_bits / 8));
    return 0;
#endif
}
