#ifndef RSENCODE_H
#define RSENCODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_VERSION 2

int rs_init_(int version);
#define rs_init() rs_init_(RS_VERSION)

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

unsigned rs_encode_work_count(unsigned original_count, unsigned recovery_count);

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
