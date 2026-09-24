#include "../constants.hpp"
#include "../transpose.hpp"
#include "../util.hpp"

#include <cassert>
#include <cstring>

namespace sydo
{

template <secpar S> inline constexpr size_t TRANSPOSE_BITS_COLS_SHIFT = 8;
template <> inline constexpr size_t TRANSPOSE_BITS_COLS_SHIFT<secpar::s128> = 7;

template <secpar S> inline constexpr size_t TRANSPOSE_BITS_COLS = (1 << TRANSPOSE_BITS_COLS_SHIFT<S>);

// TODO: test 3 and 4
#define TRANSPOSE_CHUNK_SHIFT 4
#define TRANSPOSE_CHUNK_SIZE (1 << TRANSPOSE_CHUNK_SHIFT)

// Helper for transposing byte matrices.
template <secpar S>
static ALWAYS_INLINE void transposeVxN_8_chunk(block256* __restrict__ matrix,
                                               unsigned char* __restrict__ output_mat,
                                               unsigned int i_start, unsigned int i, size_t j_chunk)
{
    size_t bit = 1 << (i % TRANSPOSE_BITS_COLS_SHIFT<S>);
    j_chunk = rotate_left(j_chunk, i_start, TRANSPOSE_BITS_COLS_SHIFT<S>);

#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
#endif
        for (size_t j_unrotated = 0; j_unrotated < TRANSPOSE_CHUNK_SIZE; ++j_unrotated)
    {
        size_t j_in_chunk = rotate_left(j_unrotated, i_start, TRANSPOSE_BITS_COLS_SHIFT<S>);

        // Iterate over all indexes with bit i cleared.
        if (j_in_chunk & bit)
            continue;

        size_t j = j_chunk + j_in_chunk;

        block256 x = matrix[j];
        block256 y = matrix[j + bit];
        block256 out[2];

        switch (i)
        {
        case 3:
            out[0] = {_mm256_unpacklo_epi8(x.data, y.data)};
            out[1] = {_mm256_unpackhi_epi8(x.data, y.data)};
            break;
        case 4:
            out[0] = {_mm256_unpacklo_epi16(x.data, y.data)};
            out[1] = {_mm256_unpackhi_epi16(x.data, y.data)};
            break;
        case 5:
            out[0] = {_mm256_unpacklo_epi32(x.data, y.data)};
            out[1] = {_mm256_unpackhi_epi32(x.data, y.data)};
            break;
        case 6:
            out[0] = {_mm256_unpacklo_epi64(x.data, y.data)};
            out[1] = {_mm256_unpackhi_epi64(x.data, y.data)};
            break;
        case 7:
            transpose2x2_128(&out[0], x, y);
            break;
        }

        if (i < 7)
        {
            matrix[j] = out[0];
            matrix[j + bit] = out[1];
        }
        else
        {
            // Write out the final result, after fixing up the order of the blocks.

            j = rotate_right(j, 8, TRANSPOSE_BITS_COLS_SHIFT<S>);

            // Need to bit reverse in positions [3, 7), because the unpack instructions work
            // differently than transposes.
            size_t reverse_mask = 0xf << (TRANSPOSE_BITS_COLS_SHIFT<S> - 5);
            size_t shift2_mask = 0xc << (TRANSPOSE_BITS_COLS_SHIFT<S> - 5);
            size_t shift1_mask = 0xa << (TRANSPOSE_BITS_COLS_SHIFT<S> - 5);
            size_t reversed = j & reverse_mask;
            reversed = ((reversed & shift2_mask) >> 2) | ((reversed << 2) & shift2_mask);
            reversed = ((reversed & shift1_mask) >> 1) | ((reversed << 1) & shift1_mask);
            j = (j & ~reverse_mask) | reversed;

            size_t used_bytes = secpar_to_bytes(S);
            memcpy(output_mat + j * used_bytes, &out[0], used_bytes);
            memcpy(output_mat + (j + TRANSPOSE_BITS_COLS<S> / 2) * used_bytes, &out[1], used_bytes);
        }
    }
}

// Transpose the bits within each 8x8 block in a V x TRANSPOSE_CHUNK_SIZE bit matrix.
static ALWAYS_INLINE void transposeVxN_block8x8(block256* matrix)
{
#ifdef __GNUC__
#pragma GCC unroll(3)
#endif
    for (int i = 0; i < 3; ++i)
    {
        size_t bit = 1 << i;

#ifdef __GNUC__
        _Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
#endif
            for (size_t j = 0; j < TRANSPOSE_CHUNK_SIZE; ++j)
        {
            if (j & bit)
                continue;

            block256 x = matrix[j];
            block256 y = matrix[j + bit];

            // Mask consisting of alternating 2^i 0s and 2^i 1s. Least significant bit is 0.
            unsigned char mask = 0xf0;
            for (int k = 1; k >= (int)i; --k)
                mask ^= mask >> (1 << k);

            __m256i diff = _mm256_xor_si256(x.data, _mm256_slli_epi16(y.data, 1 << i));
            diff = _mm256_and_si256(diff, _mm256_set1_epi8(mask));
            matrix[j] = {_mm256_xor_si256(x.data, diff)};
            matrix[j + bit] = {_mm256_xor_si256(y.data, _mm256_srli_epi16(diff, 1 << i))};
        }
    }
}

// Transpose the bits within each 8x8 bit block, using Eklundh's algorithm.
template <secpar S>
static void transposeVxN_blocks8x8(const unsigned char* input, block256* output, size_t stride)
{
    static_assert(TRANSPOSE_CHUNK_SIZE >= 8, "");
    static_assert(secpar_to_bits(S) % TRANSPOSE_CHUNK_SIZE == 0, "");

    for (size_t j = 0; j < secpar_to_bits(S); j += TRANSPOSE_CHUNK_SIZE)
    {
#ifdef __GNUC__
        _Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SIZE)))
#endif
            for (size_t j_low = 0; j_low < TRANSPOSE_CHUNK_SIZE; ++j_low)
                memcpy(&output[j + j_low], &input[stride * (j + j_low)], sizeof(block256));

        transposeVxN_block8x8(&output[j]);
    }

    if constexpr (S != secpar::s256)
        memset(&output[secpar_to_bits(S)], 0,
               (TRANSPOSE_BITS_COLS<S> - secpar_to_bits(S)) * sizeof(block256));
}

// Transpose NxN bit matrices in a VxN column-major matrix, for N = TRANSPOSE_BITS_COLS.
template <secpar S>
static ALWAYS_INLINE void transposeVxN(const unsigned char* __restrict__ input,
                                       unsigned char* __restrict__ output, size_t stride)
{
    block256 tmp[TRANSPOSE_BITS_COLS<S>];

    // First transpose the bits in 8x8 blocks.
    transposeVxN_blocks8x8<S>(input, tmp, stride);

    // Then transpose at the byte level.

#ifdef __GNUC__
#pragma GCC unroll(8)
#endif
    for (unsigned int i_start = 3; i_start < 8; i_start += TRANSPOSE_CHUNK_SHIFT)
    {
        unsigned int i_end =
            i_start + TRANSPOSE_CHUNK_SHIFT < 8 ? i_start + TRANSPOSE_CHUNK_SHIFT : 8;
        for (size_t j_chunk = 0; j_chunk < TRANSPOSE_BITS_COLS<S>; j_chunk += TRANSPOSE_CHUNK_SIZE)
        {
#ifdef __GNUC__
            _Pragma(STRINGIZE(GCC unroll (TRANSPOSE_CHUNK_SHIFT)))
#endif
                for (unsigned int i = i_start; i < i_end; ++i)
                    transposeVxN_8_chunk<S>(tmp, output, i_start, i, j_chunk);
        }
    }
}

template <secpar S>
void transpose_secpar(const void* input, void* output, size_t stride, size_t rows)
{
    const uint8_t* in = reinterpret_cast<const uint8_t*>(input);
    uint8_t* out = reinterpret_cast<uint8_t*>(output);
    if constexpr (S == secpar::s512)
    {
        alignas(block256) uint8_t low[TRANSPOSE_BITS_ROWS * secpar_to_bytes(secpar::s256)];
        alignas(block256) uint8_t high[TRANSPOSE_BITS_ROWS * secpar_to_bytes(secpar::s256)];
        for (size_t i = 0; i < rows / 8; i += TRANSPOSE_BITS_ROWS / 8)
        {
            transposeVxN<secpar::s256>(in + i, low, stride);
            transposeVxN<secpar::s256>(in + i + 256 * stride, high, stride);
            uint8_t* group_out = out + i * secpar_to_bits(S);
            for (size_t row = 0; row < TRANSPOSE_BITS_ROWS; ++row)
            {
                memcpy(group_out + row * secpar_to_bytes(S),
                       low + row * secpar_to_bytes(secpar::s256),
                       secpar_to_bytes(secpar::s256));
                memcpy(group_out + row * secpar_to_bytes(S) + secpar_to_bytes(secpar::s256),
                       high + row * secpar_to_bytes(secpar::s256),
                       secpar_to_bytes(secpar::s256));
            }
        }
    }
    else
    {
        for (size_t i = 0; i < rows / 8; i += TRANSPOSE_BITS_ROWS / 8)
            transposeVxN<S>(in + i, out + i * secpar_to_bits(S), stride);
    }
}

} // namespace sydo
