/*
 * Minimal Reed-Solomon encoder API used by the reference signature core.
 * The encoder works over GF(2^16) shards and exposes a scalar fallback plus
 * retained fixed-shape transforms for older packed dimensions.
 *
 * A "block" is one shard buffer containing buffer_bytes bytes.  The caller
 * provides original_count source blocks and recovery_count writable work blocks;
 * successful encoding fills the first recovery_count work blocks.
 */

#ifndef RSENCODE_H
#define RSENCODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_VERSION 2

/*
 * Initialize GF tables and retained fixed-transform self tests.
 * Must be called before rs_encode(); the version parameter guards against
 * accidentally linking incompatible headers and objects.
 */
int rs_init_(int version);
#define rs_init() rs_init_(RS_VERSION)

/* Negative values are explicit validation or initialization failures. */
typedef enum RsencodeResultT
{
    Rsencode_Success = 0,
    Rsencode_NeedMoreData = -1,
    Rsencode_TooMuchData = -2,
    Rsencode_InvalidSize = -3,
    Rsencode_InvalidCounts = -4,
    Rsencode_InvalidInput = -5,
    Rsencode_Platform = -6,
    Rsencode_CallInitialize = -7
} RsencodeResult;

const char *rs_result_string(RsencodeResult result);

/* Return the number of recovery buffers the caller must provide. */
unsigned rs_encode_work_count(unsigned original_count, unsigned recovery_count);

/*
 * Encode original_data shards into work_data recovery shards.
 * All buffers must point to buffer_bytes bytes, and buffer_bytes must be a
 * multiple of sizeof(uint16_t) because symbols are GF(2^16) elements.
 */
RsencodeResult rs_encode(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count,
    const void *const *original_data,
    void **work_data);

#ifdef __cplusplus
}
#endif

#endif
