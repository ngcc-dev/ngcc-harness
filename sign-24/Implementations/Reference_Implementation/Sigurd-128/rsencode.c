/*
 * GF(2^16) Reed-Solomon encoder for SIG128_REF.
 * The retained fixed transform covers the historical 11-to-44 packed shape;
 * all other shapes use the portable scalar encoder.
 *
 * The signature core calls this through a compact API that treats each shard as
 * an array of uint16_t symbols.  This file deliberately keeps the older
 * fixed-shape transform because the self-test protects against accidental
 * divergence, while the active submitted parameter set can still fall back to
 * the generic scalar path.
 */

#include "rsencode.h"
#include "rsencode_common.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Field and fixed-transform constants.  kFixed* values describe only the retained historical fast path. */
enum
{
    kFieldBits = 16,
    kFieldSize = 1 << kFieldBits,
    kFieldOrder = kFieldSize - 1,
    kPrimitive = 0x1002D,
    kGenerator = 2,
    kFixedOriginalCount = 11,
    kFixedRecoveryCount = 44,
    kFixedSymbolCount = 288,
    kFixedBufferBytes = kFixedSymbolCount * sizeof(uint16_t),
    kFixedNTTLen = 85,
    kFixedRadix5 = 5,
    kFixedRadix17 = 17,
    kFixedCRTStride5 = 51,
    kFixedCRTStride17 = 35,
    kFixedScale5 = 3,
    kFixedScale17 = 7,
    kFieldOrderInv2 = 32768
};

/* Global tables are initialized lazily because KAT tools create short-lived processes. */
static int g_initialized = 0;
static int g_tables_ready = 0;
static int g_fixed_tables_ready = 0;
static int g_fixed_self_test_ready = 0;
static uint16_t g_exp_table[(kFieldOrder * 2)];
static uint16_t g_log_table[kFieldSize];
static uint16_t g_fixed_dft5_fwd[kFixedRadix5][kFixedRadix5];
static uint16_t g_fixed_dft5_inv[kFixedRadix5][kFixedRadix5];
static uint16_t g_fixed_dft17_fwd[kFixedRadix17][kFixedRadix17];
static uint16_t g_fixed_dft17_inv[kFixedRadix17][kFixedRadix17];
static uint16_t g_fixed_crt_index[kFixedRadix5][kFixedRadix17];
static uint16_t g_fixed_chirp_input[kFixedOriginalCount];
static uint16_t g_fixed_chirp_output[kFixedRecoveryCount];
static uint16_t g_fixed_kernel_ntt[kFixedNTTLen];

static size_t elements_per_block(uint64_t buffer_bytes);

/* Bit-serial field multiply used while bootstrapping log/exp tables. */
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

/* Build duplicated exponent tables so multiplication can skip modular reduction. */
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
        return 0;
    }
    sum = (uint32_t)g_log_table[a] + (uint32_t)g_log_table[b];
    return g_exp_table[sum];
}

static uint16_t generator_pow(uint32_t exponent)
{
    return g_exp_table[exponent % (uint32_t)kFieldOrder];
}

/* Build one DFT matrix entry; inverse entries use the additive inverse exponent. */
static uint16_t field_pow_signed(uint32_t step, uint32_t scale, uint32_t row, uint32_t col, int inverse)
{
    uint32_t exponent = (uint32_t)(((uint64_t)step * (uint64_t)scale * (uint64_t)row * (uint64_t)col) % (uint64_t)kFieldOrder);

    if (inverse && exponent != 0u)
    {
        exponent = (uint32_t)kFieldOrder - exponent;
    }
    return generator_pow(exponent);
}

/* Bluestein chirp factor for converting polynomial evaluation into convolution. */
static uint16_t chirp_factor_from_square(int value, int inverse)
{
    uint32_t abs_value = (value < 0) ? (uint32_t)(-value) : (uint32_t)value;
    uint32_t square = (uint32_t)(((uint64_t)abs_value * (uint64_t)abs_value) % (uint64_t)kFieldOrder);
    uint32_t exponent = (uint32_t)(((uint64_t)square * (uint64_t)kFieldOrderInv2) % (uint64_t)kFieldOrder);

    if (inverse && exponent != 0u)
    {
        exponent = (uint32_t)kFieldOrder - exponent;
    }
    return generator_pow(exponent);
}

static void small_dft_5(const uint16_t input[kFixedRadix5], uint16_t output[kFixedRadix5], const uint16_t matrix[kFixedRadix5][kFixedRadix5])
{
    unsigned row;

    for (row = 0; row < kFixedRadix5; ++row)
    {
        uint16_t acc = 0u;
        unsigned col;

        for (col = 0; col < kFixedRadix5; ++col)
        {
            if (input[col] != 0u)
            {
                acc ^= gf_mul(input[col], matrix[row][col]);
            }
        }
        output[row] = acc;
    }
}

static void small_dft_17(const uint16_t input[kFixedRadix17], uint16_t output[kFixedRadix17], const uint16_t matrix[kFixedRadix17][kFixedRadix17])
{
    unsigned row;

    for (row = 0; row < kFixedRadix17; ++row)
    {
        uint16_t acc = 0u;
        unsigned col;

        for (col = 0; col < kFixedRadix17; ++col)
        {
            if (input[col] != 0u)
            {
                acc ^= gf_mul(input[col], matrix[row][col]);
            }
        }
        output[row] = acc;
    }
}

/* Good-Thomas decomposition of the retained length-85 NTT as 5 x 17 small DFTs. */
static void fixed_ntt_85(
    const uint16_t input[kFixedNTTLen],
    uint16_t output[kFixedNTTLen],
    const uint16_t dft17[kFixedRadix17][kFixedRadix17],
    const uint16_t dft5[kFixedRadix5][kFixedRadix5])
{
    uint16_t stage0[kFixedRadix5][kFixedRadix17];
    uint16_t stage1[kFixedRadix5][kFixedRadix17];
    unsigned r5;

    for (r5 = 0; r5 < kFixedRadix5; ++r5)
    {
        unsigned r17;
        for (r17 = 0; r17 < kFixedRadix17; ++r17)
        {
            stage0[r5][r17] = input[g_fixed_crt_index[r5][r17]];
        }
    }

    for (r5 = 0; r5 < kFixedRadix5; ++r5)
    {
        small_dft_17(stage0[r5], stage1[r5], dft17);
    }

    {
        unsigned k17;
        for (k17 = 0; k17 < kFixedRadix17; ++k17)
        {
            uint16_t column_in[kFixedRadix5];
            uint16_t column_out[kFixedRadix5];
            unsigned k5;

            for (k5 = 0; k5 < kFixedRadix5; ++k5)
            {
                column_in[k5] = stage1[k5][k17];
            }
            small_dft_5(column_in, column_out, dft5);
            for (k5 = 0; k5 < kFixedRadix5; ++k5)
            {
                output[g_fixed_crt_index[k5][k17]] = column_out[k5];
            }
        }
    }
}

static int is_fixed_transform_shape(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count)
{
    return buffer_bytes == kFixedBufferBytes &&
           original_count == kFixedOriginalCount &&
           recovery_count == kFixedRecoveryCount &&
           work_count >= kFixedRecoveryCount;
}

/*
 * Portable Horner-style RS encoder used for the active tail-expanded shapes.
 * For each output shard p, the original shards are evaluated at generator^(p+1).
 * This path is simple and shape-agnostic, which is why it remains the reference
 * fallback when current parameters no longer match the retained fixed transform.
 */
static RsencodeResult rs_encode_scalar(
    uint64_t buffer_bytes,
    unsigned original_count,
    unsigned recovery_count,
    unsigned work_count,
    const void *const *original_data,
    void **work_data)
{
    size_t symbol_count;
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

    for (p = 0; p < recovery_count; ++p)
    {
        if (work_data[p] == NULL)
        {
            return Rsencode_InvalidInput;
        }
    }
    for (s = 0; s < symbol_count; ++s)
    {
        for (p = 0; p < recovery_count; ++p)
        {
            uint16_t acc = 0;
            int coeff;
            uint16_t eval_point = generator_pow((uint32_t)p + 1u);
            for (coeff = (int)original_count - 1; coeff >= 0; --coeff)
            {
                const uint16_t *shard;
                uint16_t value;
                if (original_data[coeff] == NULL)
                {
                    return Rsencode_InvalidInput;
                }
                shard = (const uint16_t *)original_data[coeff];
                value = shard[s];
                acc = gf_mul(acc, eval_point);
                acc = gf_add(acc, value);
            }
            ((uint16_t *)work_data[p])[s] = acc;
        }
    }

    return Rsencode_Success;
}

/*
 * Precompute DFT, CRT, chirp, and convolution tables for the retained fast path.
 * These tables are independent of message contents and are initialized once
 * during rs_init_().
 */
static RsencodeResult ensure_fixed_tables(void)
{
    unsigned row;

    if (g_fixed_tables_ready)
    {
        return Rsencode_Success;
    }

    for (row = 0; row < kFixedRadix5; ++row)
    {
        unsigned col;
        for (col = 0; col < kFixedRadix5; ++col)
        {
            g_fixed_dft5_fwd[row][col] = field_pow_signed((uint32_t)(kFieldOrder / kFixedRadix5), kFixedScale5, row, col, 0);
            g_fixed_dft5_inv[row][col] = field_pow_signed((uint32_t)(kFieldOrder / kFixedRadix5), kFixedScale5, row, col, 1);
        }
    }
    for (row = 0; row < kFixedRadix17; ++row)
    {
        unsigned col;
        for (col = 0; col < kFixedRadix17; ++col)
        {
            g_fixed_dft17_fwd[row][col] = field_pow_signed((uint32_t)(kFieldOrder / kFixedRadix17), kFixedScale17, row, col, 0);
            g_fixed_dft17_inv[row][col] = field_pow_signed((uint32_t)(kFieldOrder / kFixedRadix17), kFixedScale17, row, col, 1);
        }
    }
    for (row = 0; row < kFixedRadix5; ++row)
    {
        unsigned col;
        for (col = 0; col < kFixedRadix17; ++col)
        {
            g_fixed_crt_index[row][col] = (uint16_t)((row * kFixedCRTStride5 + col * kFixedCRTStride17) % kFixedNTTLen);
        }
    }
    for (row = 0; row < kFixedOriginalCount; ++row)
    {
        g_fixed_chirp_input[row] = chirp_factor_from_square((int)row, 0);
    }
    for (row = 0; row < kFixedRecoveryCount; ++row)
    {
        g_fixed_chirp_output[row] = chirp_factor_from_square((int)row + 1, 0);
    }
    {
        uint16_t kernel_time[kFixedNTTLen];
        memset(kernel_time, 0, sizeof(kernel_time));
        for (row = 0; row < kFixedNTTLen; ++row)
        {
            int t = (int)row - (kFixedOriginalCount - 1);
            if (t >= -(kFixedOriginalCount - 1) && t <= (int)kFixedRecoveryCount)
            {
                kernel_time[row] = chirp_factor_from_square(t, 1);
            }
        }
        fixed_ntt_85(kernel_time, g_fixed_kernel_ntt, g_fixed_dft17_fwd, g_fixed_dft5_fwd);
    }

    g_fixed_tables_ready = 1;
    return Rsencode_Success;
}

/*
 * Guard the retained fixed transform against divergence from the scalar encoder.
 * The self-test runs through the public rs_encode() entrypoint so dispatch,
 * table initialization, and transform output are checked together.
 */
static RsencodeResult self_test_fixed_transform(void)
{
    uint16_t original_storage[kFixedOriginalCount][kFixedSymbolCount];
    uint16_t fast_storage[kFixedRecoveryCount][kFixedSymbolCount];
    uint16_t scalar_storage[kFixedRecoveryCount][kFixedSymbolCount];
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
        for (symbol = 0; symbol < kFixedSymbolCount; ++symbol)
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

    fast_result = rs_encode(
        kFixedBufferBytes,
        kFixedOriginalCount,
        kFixedRecoveryCount,
        kFixedRecoveryCount,
        original_ptrs,
        fast_ptrs);
    scalar_result = rs_encode_scalar(
        kFixedBufferBytes,
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

/*
 * Bluestein/Good-Thomas fixed-shape encoder for the historical packed dimensions.
 * It is used only when buffer_bytes/original_count/recovery_count match the
 * kFixed* constants exactly; otherwise rs_encode() dispatches to the scalar path.
 */
static RsencodeResult rs_encode_fixed_transform(
    const void *const *original_data,
    void **work_data)
{
    const uint16_t *inputs[kFixedOriginalCount];
    uint16_t *outputs[kFixedRecoveryCount];
    unsigned shard;
    unsigned symbol;

    for (shard = 0; shard < kFixedOriginalCount; ++shard)
    {
        if (original_data[shard] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        inputs[shard] = (const uint16_t *)original_data[shard];
    }
    for (shard = 0; shard < kFixedRecoveryCount; ++shard)
    {
        if (work_data[shard] == NULL)
        {
            return Rsencode_InvalidInput;
        }
        outputs[shard] = (uint16_t *)work_data[shard];
    }

    for (symbol = 0; symbol < kFixedSymbolCount; ++symbol)
    {
        uint16_t poly_time[kFixedNTTLen];
        uint16_t poly_ntt[kFixedNTTLen];
        uint16_t conv_ntt[kFixedNTTLen];
        uint16_t conv_time[kFixedNTTLen];
        unsigned idx;

        memset(poly_time, 0, sizeof(poly_time));
        for (idx = 0; idx < kFixedOriginalCount; ++idx)
        {
            poly_time[idx] = gf_mul(inputs[idx][symbol], g_fixed_chirp_input[idx]);
        }

        fixed_ntt_85(poly_time, poly_ntt, g_fixed_dft17_fwd, g_fixed_dft5_fwd);
        for (idx = 0; idx < kFixedNTTLen; ++idx)
        {
            conv_ntt[idx] = gf_mul(poly_ntt[idx], g_fixed_kernel_ntt[idx]);
        }
        fixed_ntt_85(conv_ntt, conv_time, g_fixed_dft17_inv, g_fixed_dft5_inv);

        for (idx = 0; idx < kFixedRecoveryCount; ++idx)
        {
            outputs[idx][symbol] = gf_mul(conv_time[idx + kFixedOriginalCount], g_fixed_chirp_output[idx]);
        }
    }

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
    /* Initialize every backend before marking the encoder ready for public use. */
    ensure_tables();
    result = ensure_fixed_tables();
    if (result != Rsencode_Success)
    {
        return result;
    }
    InitializeCPUArch();
    g_initialized = 1;
    result = self_test_fixed_transform();
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
    if (is_fixed_transform_shape(buffer_bytes, original_count, recovery_count, work_count))
    {
        /* Prefer the retained transform only when the caller's shape matches exactly. */
        return rs_encode_fixed_transform(original_data, work_data);
    }
    return rs_encode_scalar(
        buffer_bytes,
        original_count,
        recovery_count,
        work_count,
        original_data,
        work_data);
}
