/*
 * Aligned TrEDM sponge mode for AFS-TrEDM.
 */

#ifndef AFS_TREDM_H
#define AFS_TREDM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AFS_TREDM_SUCCESS 0
#define AFS_TREDM_BAD_DIGEST_LENGTH -1
#define AFS_TREDM_NULL_POINTER -2

int afs_tredm_hash(int digest_len_bits,
                   const unsigned char *msg,
                   unsigned long long msg_len_bits,
                   unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* AFS_TREDM_H */
