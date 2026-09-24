#include "rsencode.h"
#include "rsencode_common.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum
{
    kFieldBits = 16,
    kFieldSize = 1 << kFieldBits,
    kFieldOrder = kFieldSize - 1,
    kPrimitive = 0x1002D,
    kGenerator = 2,
    kFixedOriginalCount = 11,
    kFixedRecoveryCount = 44
};

static int g_initialized = 0;
static int g_tables_ready = 0;
static int g_fixed_self_test_ready = 0;
static uint16_t g_exp_table[(kFieldOrder * 2)];
static uint16_t g_log_table[kFieldSize];

static size_t elements_per_block(uint64_t buffer_bytes);

static uint16_t mul_basic(uint16_t a, uint16_t b)
{
    int i;
    uint16_t result = 0;

    for (i = 0; i < kFieldBits; ++i)
    {
        if ((b & 1u) != 0u)
        {
            result ^= a;
        }
        if ((a & 0x8000u) != 0u)
        {
            a = (uint16_t)(a << 1);
            a ^= (uint16_t)(kPrimitive & 0xFFFFu);
        }
        else
        {
            a = (uint16_t)(a << 1);
        }
        b = (uint16_t)(b >> 1);
    }
    return result;
}

static void ensure_tables(void)
{
    uint16_t value;
    uint32_t i;

    if (g_tables_ready)
    {
        return;
    }

    value = 1;
    for (i = 0; i < (uint32_t)kFieldOrder; ++i)
    {
        g_exp_table[i] = value;
        g_log_table[value] = (uint16_t)i;
        value = mul_basic(value, (uint16_t)kGenerator);
    }
    for (i = (uint32_t)kFieldOrder; i < (uint32_t)(kFieldOrder * 2); ++i)
    {
        g_exp_table[i] = g_exp_table[i - kFieldOrder];
    }
    g_log_table[0] = 0;
    g_tables_ready = 1;
}

static uint16_t gf_add(uint16_t a, uint16_t b)
{
    return (uint16_t)(a ^ b);
}

static uint16_t gf_mul(uint16_t a, uint16_t b)
{
    uint32_t sum;

    if (a == 0u || b == 0u)
    {
        return 0u;
    }
    sum = (uint32_t)g_log_table[a] + (uint32_t)g_log_table[b];
    return g_exp_table[sum];
}

static uint16_t generator_pow(uint32_t exponent)
{
    return g_exp_table[exponent % (uint32_t)kFieldOrder];
}

static RsencodeResult rs_encode_scalar(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count,
    const void *const *original_data,
    void **work_data)
{
    size_t symbol_count;
    unsigned shard;
    unsigned p;
    size_t s;

    if (buffer_bytes == 0u || (buffer_bytes % 2u) != 0u)
    {
        return Rsencode_InvalidSize;
    }
    if (original_count == 0u || recovery_count == 0u || work_count < recovery_count)
    {
        return Rsencode_InvalidCounts;
    }
    if (original_data == NULL || work_data == NULL)
    {
        return Rsencode_InvalidInput;
    }

    ensure_tables();
    symbol_count = elements_per_block(buffer_bytes);

    const uint16_t *data_shards[original_count];
    uint16_t *parity_shards[recovery_count];
    uint16_t eval_points[recovery_count];

    for (shard = 0; shard < original_count; ++shard)
    {
        if (original_data[shard] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        data_shards[shard] = (const uint16_t *)original_data[shard];
    }
    for (p = 0; p < recovery_count; ++p)
    {
        if (work_data[p] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        parity_shards[p] = (uint16_t *)work_data[p];
        eval_points[p] = generator_pow((uint32_t)p + 1u);
    }
    for (p = 0; p < recovery_count; ++p)
    {
        uint16_t *parity = parity_shards[p];
        uint16_t eval_point = eval_points[p];
        int coeff;

        memset(parity, 0, buffer_bytes);
        for (coeff = (int)original_count - 1; coeff >= 0; --coeff)
        {
            const uint16_t *data = data_shards[coeff];

            for (s = 0; s < symbol_count; ++s)
            {
                parity[s] = gf_add(gf_mul(parity[s], eval_point), data[s]);
            }
        }
    }

    return Rsencode_Success;
}

static RsencodeResult rs_encode_fixed_specialized(
    uint64_t buffer_bytes,
    unsigned work_count,
    const void *const *original_data,
    void **work_data)
{
    const uint16_t *data_shards[kFixedOriginalCount];
    uint16_t *parity_shards[kFixedRecoveryCount];
    size_t symbol_count;
    unsigned shard;
    size_t s;

    if (work_count < kFixedRecoveryCount)
    {
        return Rsencode_InvalidCounts;
    }

    for (shard = 0; shard < kFixedOriginalCount; ++shard)
    {
        if (original_data[shard] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        data_shards[shard] = (const uint16_t *)original_data[shard];
    }
    for (shard = 0; shard < kFixedRecoveryCount; ++shard)
    {
        if (work_data[shard] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        parity_shards[shard] = (uint16_t *)work_data[shard];
    }

    symbol_count = elements_per_block(buffer_bytes);
    for (s = 0; s < symbol_count; ++s)
    {
        uint16_t v0 = data_shards[0][s];
        uint16_t v1 = data_shards[1][s];
        uint16_t v2 = data_shards[2][s];
        uint16_t v3 = data_shards[3][s];
        uint16_t v4 = data_shards[4][s];
        uint16_t v5 = data_shards[5][s];
        uint16_t v6 = data_shards[6][s];
        uint16_t v7 = data_shards[7][s];
        uint16_t v8 = data_shards[8][s];
        uint16_t v9 = data_shards[9][s];
        uint16_t v10 = data_shards[10][s];
        uint32_t exp1 = 0u;
        uint32_t exp2 = 0u;
        uint32_t exp3 = 0u;
        uint32_t exp4 = 0u;
        uint32_t exp5 = 0u;
        uint32_t exp6 = 0u;
        uint32_t exp7 = 0u;
        uint32_t exp8 = 0u;
        uint32_t exp9 = 0u;
        uint32_t exp10 = 0u;
        int nz1 = v1 != 0u;
        int nz2 = v2 != 0u;
        int nz3 = v3 != 0u;
        int nz4 = v4 != 0u;
        int nz5 = v5 != 0u;
        int nz6 = v6 != 0u;
        int nz7 = v7 != 0u;
        int nz8 = v8 != 0u;
        int nz9 = v9 != 0u;
        int nz10 = v10 != 0u;
        unsigned p;

        if (nz1) exp1 = (uint32_t)g_log_table[v1] + 1u;
        if (nz2) exp2 = (uint32_t)g_log_table[v2] + 2u;
        if (nz3) exp3 = (uint32_t)g_log_table[v3] + 3u;
        if (nz4) exp4 = (uint32_t)g_log_table[v4] + 4u;
        if (nz5) exp5 = (uint32_t)g_log_table[v5] + 5u;
        if (nz6) exp6 = (uint32_t)g_log_table[v6] + 6u;
        if (nz7) exp7 = (uint32_t)g_log_table[v7] + 7u;
        if (nz8) exp8 = (uint32_t)g_log_table[v8] + 8u;
        if (nz9) exp9 = (uint32_t)g_log_table[v9] + 9u;
        if (nz10) exp10 = (uint32_t)g_log_table[v10] + 10u;

        for (p = 0; p < kFixedRecoveryCount; ++p)
        {
            uint16_t acc = v0;

            if (nz1)
            {
                acc ^= g_exp_table[exp1];
                exp1 += 1u;
            }
            if (nz2)
            {
                acc ^= g_exp_table[exp2];
                exp2 += 2u;
            }
            if (nz3)
            {
                acc ^= g_exp_table[exp3];
                exp3 += 3u;
            }
            if (nz4)
            {
                acc ^= g_exp_table[exp4];
                exp4 += 4u;
            }
            if (nz5)
            {
                acc ^= g_exp_table[exp5];
                exp5 += 5u;
            }
            if (nz6)
            {
                acc ^= g_exp_table[exp6];
                exp6 += 6u;
            }
            if (nz7)
            {
                acc ^= g_exp_table[exp7];
                exp7 += 7u;
            }
            if (nz8)
            {
                acc ^= g_exp_table[exp8];
                exp8 += 8u;
            }
            if (nz9)
            {
                acc ^= g_exp_table[exp9];
                exp9 += 9u;
            }
            if (nz10)
            {
                acc ^= g_exp_table[exp10];
                exp10 += 10u;
            }
            parity_shards[p][s] = acc;
        }
    }

    return Rsencode_Success;
}

static int is_fixed_specialized_shape(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count)
{
    return buffer_bytes != 0u &&
           (buffer_bytes % 2u) == 0u &&
           original_count == kFixedOriginalCount &&
           recovery_count == kFixedRecoveryCount &&
           work_count >= kFixedRecoveryCount;
}

static RsencodeResult self_test_fixed_specialized(void)
{
    enum
    {
        kSelfTestSymbols = 288
    };

    uint16_t original_storage[kFixedOriginalCount][kSelfTestSymbols];
    uint16_t fast_storage[kFixedRecoveryCount][kSelfTestSymbols];
    uint16_t scalar_storage[kFixedRecoveryCount][kSelfTestSymbols];
    const void *original_ptrs[kFixedOriginalCount];
    void *fast_ptrs[kFixedRecoveryCount];
    void *scalar_ptrs[kFixedRecoveryCount];
    unsigned shard;
    unsigned symbol;
    RsencodeResult fast_result;
    RsencodeResult scalar_result;

    if (g_fixed_self_test_ready)
    {
        return Rsencode_Success;
    }

    for (shard = 0; shard < kFixedOriginalCount; ++shard)
    {
        original_ptrs[shard] = original_storage[shard];
        for (symbol = 0; symbol < kSelfTestSymbols; ++symbol)
        {
            original_storage[shard][symbol] = (uint16_t)(shard * 577u + symbol * 29u + 17u);
        }
    }
    for (shard = 0; shard < kFixedRecoveryCount; ++shard)
    {
        memset(fast_storage[shard], 0, sizeof(fast_storage[shard]));
        memset(scalar_storage[shard], 0, sizeof(scalar_storage[shard]));
        fast_ptrs[shard] = fast_storage[shard];
        scalar_ptrs[shard] = scalar_storage[shard];
    }

    fast_result = rs_encode_fixed_specialized(
        (uint64_t)(kSelfTestSymbols * sizeof(uint16_t)),
        kFixedRecoveryCount,
        original_ptrs,
        fast_ptrs);
    scalar_result = rs_encode_scalar(
        (uint64_t)(kSelfTestSymbols * sizeof(uint16_t)),
        kFixedOriginalCount,
        kFixedRecoveryCount,
        kFixedRecoveryCount,
        original_ptrs,
        scalar_ptrs);
    if (fast_result != Rsencode_Success || scalar_result != Rsencode_Success)
    {
        return Rsencode_Platform;
    }
    if (memcmp(fast_storage, scalar_storage, sizeof(fast_storage)) != 0)
    {
        return Rsencode_Platform;
    }

    g_fixed_self_test_ready = 1;
    return Rsencode_Success;
}

static size_t elements_per_block(uint64_t buffer_bytes)
{
    return (size_t)(buffer_bytes / sizeof(uint16_t));
}

int rs_init_(int version)
{
    RsencodeResult result;

    if (version != RS_VERSION)
    {
        return Rsencode_InvalidInput;
    }

    ensure_tables();
    InitializeCPUArch();
    g_initialized = 1;
    result = self_test_fixed_specialized();
    if (result != Rsencode_Success)
    {
        return result;
    }
    return Rsencode_Success;
}

const char *rs_result_string(RsencodeResult result)
{
    switch (result)
    {
    case Rsencode_Success:
        return "Operation succeeded";
    case Rsencode_NeedMoreData:
        return "Not enough recovery data received";
    case Rsencode_TooMuchData:
        return "Buffer counts are too high";
    case Rsencode_InvalidSize:
        return "Buffer size must be a multiple of 2 bytes";
    case Rsencode_InvalidCounts:
        return "Invalid counts provided";
    case Rsencode_InvalidInput:
        return "A function parameter was invalid";
    case Rsencode_Platform:
        return "Platform is unsupported";
    case Rsencode_CallInitialize:
        return "Call rs_init() first";
    default:
        return "Unknown";
    }
}

unsigned rs_encode_work_count(unsigned original_count, unsigned recovery_count)
{
    if (original_count == 0u || recovery_count == 0u)
    {
        return 0u;
    }
    return recovery_count;
}

RsencodeResult rs_encode(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count,
    const void *const *original_data,
    void **work_data)
{
    if (!g_initialized)
    {
        return Rsencode_CallInitialize;
    }
    if (buffer_bytes == 0u || (buffer_bytes % 2u) != 0u)
    {
        return Rsencode_InvalidSize;
    }
    if (original_count == 0u || recovery_count == 0u || work_count < recovery_count)
    {
        return Rsencode_InvalidCounts;
    }
    if (original_data == NULL || work_data == NULL)
    {
        return Rsencode_InvalidInput;
    }
    if (is_fixed_specialized_shape(buffer_bytes, original_count, recovery_count, work_count))
    {
        return rs_encode_fixed_specialized(buffer_bytes, work_count, original_data, work_data);
    }
    return rs_encode_scalar(
        buffer_bytes,
        original_count,
        recovery_count,
        work_count,
        original_data,
        work_data);
}
