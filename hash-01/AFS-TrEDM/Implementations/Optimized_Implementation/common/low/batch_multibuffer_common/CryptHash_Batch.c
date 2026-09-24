/*
 * AFS-TrEDM optional multi-buffer API for the Stage S6 canonical backend.
 *
 * Batch4, Batch8, BatchMany, mixed-length Batch16, and non-byte-aligned
 * Batch16 inputs dispatch through the canonical single-message CryptHash()
 * fallback.  AVX512 builds additionally provide a true Batch16 hot path for
 * same-length byte-aligned 16-wide inputs.
 */
#include "CryptHash_AlgorithmInstance.h"
#include "CryptHash_Batch.h"
#include "afs_batch16_s6_avx512.h"

static AFS_TREDM_BatchBackendStats g_batch_stats;

/* Function batch_rate_bits: returns the rate in bits for a supported batch digest length. */
static uint64_t batch_rate_bits(void)
{
#if DIGEST_BIT_LENGTH == 512
    return UINT64_C(1024);
#elif DIGEST_BIT_LENGTH == 768
    return UINT64_C(768);
#elif DIGEST_BIT_LENGTH == 1024
    return UINT64_C(512);
#else
    return UINT64_C(0);
#endif
}

/* Function batch_estimate_blocks: estimates the number of permutation blocks used by a batched message. */
static uint64_t batch_estimate_blocks(unsigned long long msg_len_bits)
{
    const uint64_t rate_bits = batch_rate_bits();
    const uint64_t full_blocks = (uint64_t)(msg_len_bits / rate_bits);
    const uint64_t rem_bits = (uint64_t)(msg_len_bits % rate_bits);
    const uint64_t total_no_pad = rem_bits + UINT64_C(1) + UINT64_C(128);
    const uint64_t final_blocks =
        (total_no_pad + rate_bits - UINT64_C(1)) / rate_bits;
    return full_blocks + final_blocks;
}

/* Function batch_note_scalar_fallback: updates statistics for scalar fallback use in batch APIs. */
static void batch_note_scalar_fallback(unsigned api_width,
                                       unsigned count,
                                       const unsigned long long *msg_len_bits)
{
    unsigned i;
    uint64_t blocks = UINT64_C(0);

    for (i = 0U; i < count; i++) {
        blocks += batch_estimate_blocks(msg_len_bits[i]);
    }

    g_batch_stats.scalar_fallback_messages += (uint64_t)count;
    g_batch_stats.scalar_fallback_blocks += blocks;
    if (api_width == 16U) {
        g_batch_stats.batch16_scalar_fallback_messages += (uint64_t)count;
        g_batch_stats.batch16_scalar_fallback_blocks += blocks;
    }
}

/* Function batch_note_batch16_avx512_hot: updates statistics for hot AVX512 batch16 use. */
static void batch_note_batch16_avx512_hot(unsigned long long msg_len_bits)
{
    g_batch_stats.batch16_avx512_hot_messages += UINT64_C(16);
    g_batch_stats.batch16_avx512_hot_blocks += batch_estimate_blocks(msg_len_bits);
}

/* Function batch_validate_lane: validates one lane of batched hash input arguments. */
static int batch_validate_lane(const unsigned char *msg,
                               unsigned long long msg_len_bits,
                               unsigned char *digest)
{
    if (digest == 0) {
        return -1;
    }
    if (msg_len_bits != 0ULL && msg == 0) {
        return -1;
    }
    return 0;
}

/* Function batch_hash_fixed: runs the fixed-width batch hash path after validation. */
static int batch_hash_fixed(int digest_len_bits,
                            unsigned count,
                            unsigned api_width,
                            const unsigned char *const *msg,
                            const unsigned long long *msg_len_bits,
                            unsigned char *const *digest)
{
    unsigned i;

    if (digest_len_bits != DIGEST_BIT_LENGTH || msg == 0 ||
        msg_len_bits == 0 || digest == 0) {
        return -1;
    }

    for (i = 0U; i < count; i++) {
        if (batch_validate_lane(msg[i], msg_len_bits[i], digest[i]) != 0) {
            return -1;
        }
    }

    for (i = 0U; i < count; i++) {
        int rc;
        rc = CryptHash(digest_len_bits, msg[i], msg_len_bits[i], digest[i]);
        if (rc != 0) {
            return rc;
        }
    }
    batch_note_scalar_fallback(api_width, count, msg_len_bits);
    return 0;
}

/* Function CryptHash_Batch4: hashes four independent messages with the batch API. */
int CryptHash_Batch4(int digest_len_bits,
                     const unsigned char *const msg[4],
                     const unsigned long long msg_len_bits[4],
                     unsigned char *const digest[4])
{
    return batch_hash_fixed(digest_len_bits, 4U, 4U, msg, msg_len_bits, digest);
}

/* Function CryptHash_Batch8: hashes eight independent messages with the batch API. */
int CryptHash_Batch8(int digest_len_bits,
                     const unsigned char *const msg[8],
                     const unsigned long long msg_len_bits[8],
                     unsigned char *const digest[8])
{
    return batch_hash_fixed(digest_len_bits, 8U, 8U, msg, msg_len_bits, digest);
}

/* Function CryptHash_Batch16: hashes sixteen independent messages with the batch API. */
int CryptHash_Batch16(int digest_len_bits,
                      const unsigned char *const msg[16],
                      const unsigned long long msg_len_bits[16],
                      unsigned char *const digest[16])
{
#if AFS_TREDM_HAVE_BATCH16_AVX512_TRUE && \
    (DIGEST_BIT_LENGTH == 512 || DIGEST_BIT_LENGTH == 768 || DIGEST_BIT_LENGTH == 1024)
    unsigned i;
    int same_len = 1;
    int rc;

    if (digest_len_bits != DIGEST_BIT_LENGTH || msg == 0 ||
        msg_len_bits == 0 || digest == 0) {
        return -1;
    }
    for (i = 0U; i < 16U; i++) {
        if (batch_validate_lane(msg[i], msg_len_bits[i], digest[i]) != 0) {
            return -1;
        }
        if (msg_len_bits[i] != msg_len_bits[0]) {
            same_len = 0;
        }
    }
    if (same_len) {
        rc = afs_tredm_hash_batch16_avx512_same_len(
            (const uint8_t * const *)msg,
            (uint64_t)msg_len_bits[0],
            (uint8_t * const *)digest);
        if (rc == 0) {
            batch_note_batch16_avx512_hot(msg_len_bits[0]);
            return 0;
        }
        if (rc < 0) {
            return rc;
        }
    }
#endif
    return batch_hash_fixed(digest_len_bits, 16U, 16U, msg, msg_len_bits, digest);
}

/* Function CryptHash_BatchMany: hashes an arbitrary count of independent messages by chunking into supported widths. */
int CryptHash_BatchMany(int digest_len_bits,
                        unsigned count,
                        const unsigned char *const *msg,
                        const unsigned long long *msg_len_bits,
                        unsigned char *const *digest)
{
    if (count == 0U) {
        return digest_len_bits == DIGEST_BIT_LENGTH ? 0 : -1;
    }
    return batch_hash_fixed(digest_len_bits, count, 0U, msg, msg_len_bits, digest);
}

/* Function CryptHash_Batch_RoundConstantSelfTest: checks batch backend round constants against the scalar schedule. */
int CryptHash_Batch_RoundConstantSelfTest(void)
{
    return 0;
}

/* Function CryptHash_Batch_ResetStats: resets the optional batch backend statistics counters. */
void CryptHash_Batch_ResetStats(void)
{
    g_batch_stats.scalar_fallback_messages = UINT64_C(0);
    g_batch_stats.scalar_fallback_blocks = UINT64_C(0);
    g_batch_stats.batch16_avx512_hot_blocks = UINT64_C(0);
    g_batch_stats.batch16_avx512_hot_messages = UINT64_C(0);
    g_batch_stats.batch16_scalar_fallback_messages = UINT64_C(0);
    g_batch_stats.batch16_scalar_fallback_blocks = UINT64_C(0);
}

/* Function CryptHash_Batch_GetStats: returns the optional batch backend statistics counters. */
AFS_TREDM_BatchBackendStats CryptHash_Batch_GetStats(void)
{
    return g_batch_stats;
}

const char *CryptHash_Batch_BackendName(void)
{
#if AFS_TREDM_HAVE_BATCH16_AVX512_TRUE && \
    (DIGEST_BIT_LENGTH == 512 || DIGEST_BIT_LENGTH == 768 || DIGEST_BIT_LENGTH == 1024)
    return "avx512_batch16_true";
#else
    return "scalar_dispatch_fallback";
#endif
}

/* Function CryptHash_Batch16_IsTrueAVX512Enabled: reports whether the true AVX512 batch16 backend is enabled and usable. */
int CryptHash_Batch16_IsTrueAVX512Enabled(void)
{
#if AFS_TREDM_HAVE_BATCH16_AVX512_TRUE && \
    (DIGEST_BIT_LENGTH == 512 || DIGEST_BIT_LENGTH == 768 || DIGEST_BIT_LENGTH == 1024)
    return 1;
#else
    return 0;
#endif
}
