/*
 * Optional AFS-TrEDM multi-buffer API.
 *
 * These entry points are not part of the ICCS CryptHash() interface.  They
 * are provided as Additional_Implementation helpers for batch-throughput
 * measurements. Batch4, Batch8, BatchMany, mixed-length Batch16, and
 * non-byte-aligned Batch16 inputs preserve output order by using scalar
 * CryptHash() fallback. AVX512 Batch16 builds additionally expose a true
 * same-length byte-aligned 16-wide hot path.
 */
#ifndef AFS_TREDM_CRYPTHASH_BATCH_H
#define AFS_TREDM_CRYPTHASH_BATCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t scalar_fallback_messages;
    uint64_t scalar_fallback_blocks;
    uint64_t batch16_avx512_hot_blocks;
    uint64_t batch16_avx512_hot_messages;
    uint64_t batch16_scalar_fallback_messages;
    uint64_t batch16_scalar_fallback_blocks;
} AFS_TREDM_BatchBackendStats;

int CryptHash_Batch4(int digest_len_bits,
                     const unsigned char *const msg[4],
                     const unsigned long long msg_len_bits[4],
                     unsigned char *const digest[4]);

int CryptHash_Batch8(int digest_len_bits,
                     const unsigned char *const msg[8],
                     const unsigned long long msg_len_bits[8],
                     unsigned char *const digest[8]);

int CryptHash_Batch16(int digest_len_bits,
                      const unsigned char *const msg[16],
                      const unsigned long long msg_len_bits[16],
                      unsigned char *const digest[16]);

int CryptHash_BatchMany(int digest_len_bits,
                        unsigned count,
                        const unsigned char *const *msg,
                        const unsigned long long *msg_len_bits,
                        unsigned char *const *digest);

int CryptHash_Batch_RoundConstantSelfTest(void);

void CryptHash_Batch_ResetStats(void);
AFS_TREDM_BatchBackendStats CryptHash_Batch_GetStats(void);
const char *CryptHash_Batch_BackendName(void);
int CryptHash_Batch16_IsTrueAVX512Enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* AFS_TREDM_CRYPTHASH_BATCH_H */
