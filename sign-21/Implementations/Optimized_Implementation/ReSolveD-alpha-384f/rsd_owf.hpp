#ifndef RSD_OWF_HPP
#define RSD_OWF_HPP

#include "hash.hpp"
#include "parameters.hpp"
#include "prgs.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sig
{
namespace rsd
{

constexpr std::size_t IV_SIZE = 15;

constexpr std::size_t ceil_bytes(std::size_t bits) { return (bits + 7) / 8; }

inline uint8_t ptr_get_bit(const uint8_t* value, std::size_t index)
{
    return (value[index / 8] >> (index % 8)) & 1;
}

inline uint8_t ptr_get_bits(const uint8_t* value, std::size_t byte_len, std::size_t index,
                            std::size_t width)
{
    const std::size_t byte_index = index / 8;
    uint32_t out = value[byte_index];
    if (byte_index + 1 < byte_len)
        out |= static_cast<uint32_t>(value[byte_index + 1]) << 8;
    return static_cast<uint8_t>((out >> (index % 8)) & ((uint32_t(1) << width) - 1));
}

inline uint64_t ptr_get_u64_bits(const uint8_t* value, std::size_t byte_len, std::size_t index,
                                 std::size_t width)
{
    const std::size_t byte_index = index / 8;
    const std::size_t bit_index = index % 8;
    const std::size_t remaining = byte_index < byte_len ? byte_len - byte_index : 0;

    uint64_t out = 0;
    std::memcpy(&out, value + byte_index, remaining < sizeof(out) ? remaining : sizeof(out));
    out >>= bit_index;

    if (bit_index && remaining > sizeof(out))
        out |= static_cast<uint64_t>(value[byte_index + sizeof(out)]) << (64 - bit_index);

    if (width == 64)
        return out;
    return out & ((uint64_t(1) << width) - 1);
}

inline void ptr_set_bit(uint8_t* dst, std::size_t index, uint8_t value)
{
    const std::size_t bit = index % 8;
    dst[index / 8] =
        static_cast<uint8_t>((dst[index / 8] & ~(uint8_t(1) << bit)) | ((value & 1) << bit));
}

template <typename P> uint8_t reduce_mod_block_size(const uint8_t* bytes)
{
    using OC = typename P::OWF_CONSTS;

    static_assert(OC::RSD_BLOCK_SIZE == 6);

    static constexpr auto table = []
    {
        std::array<std::array<uint8_t, 256>, OC::RSD_BLOCK_SIZE> t{};
        for (std::size_t r = 0; r < OC::RSD_BLOCK_SIZE; ++r)
            for (std::size_t byte = 0; byte < 256; ++byte)
                t[r][byte] = static_cast<uint8_t>((4 * r + byte) % OC::RSD_BLOCK_SIZE);
        return t;
    }();

    uint8_t r = 0;
    for (std::size_t i = P::secpar_bytes; i-- > 0;)
        r = table[r][bytes[i]];
    return r;
}

template <typename P> void sample_noise(uint8_t* e, const uint8_t* seed_sk)
{
    using OC = typename P::OWF_CONSTS;

    std::memset(e, 0, ceil_bytes(OC::RSD_CODE_LENGTH));

    if constexpr (P::secpar_v == secpar::s160)
    {
        block192 key_block{};
        std::memcpy(&key_block, seed_sk, P::secpar_bytes);
        aes_round_keys<secpar::s192> round_keys;
        aes_keygen<secpar::s192>(&round_keys, key_block);

        for (std::size_t i = 0; i < OC::RSD_NOISE_WEIGHT; ++i)
        {
            std::array<block128, 2> blocks{};
            auto* first = reinterpret_cast<uint8_t*>(&blocks[0]);
            auto* second = reinterpret_cast<uint8_t*>(&blocks[1]);
            first[0] = static_cast<uint8_t>(i);
            first[1] = static_cast<uint8_t>(i >> 8);
            second[0] = first[0];
            second[1] = first[1];
            second[IV_SIZE] = 1;

            aes_ecb<secpar::s192, 1, 2>(&round_keys, blocks.data());

            std::array<uint8_t, P::secpar_bytes> r{};
            std::memcpy(r.data(), &blocks[0], sizeof(block128));
            std::memcpy(r.data() + sizeof(block128), &blocks[1],
                        P::secpar_bytes - sizeof(block128));

            const uint8_t pos = reduce_mod_block_size<P>(r.data());
            ptr_set_bit(e, i * OC::RSD_BLOCK_SIZE + pos, 1);
        }
        return;
    }
    else if constexpr (P::secpar_v == secpar::s256)
    {
        block256 key_block;
        std::memcpy(&key_block, seed_sk, P::secpar_bytes);
        aes_round_keys<secpar::s256> round_keys;
        aes_keygen<secpar::s256>(&round_keys, key_block);

        for (std::size_t i = 0; i < OC::RSD_NOISE_WEIGHT; ++i)
        {
            std::array<block128, 2> blocks{};
            auto* first = reinterpret_cast<uint8_t*>(&blocks[0]);
            auto* second = reinterpret_cast<uint8_t*>(&blocks[1]);
            first[0] = static_cast<uint8_t>(i);
            first[1] = static_cast<uint8_t>(i >> 8);
            second[0] = first[0];
            second[1] = first[1];
            second[IV_SIZE] = 1;

            aes_ecb<secpar::s256, 1, 2>(&round_keys, blocks.data());

            std::array<uint8_t, P::secpar_bytes> r{};
            std::memcpy(r.data(), blocks.data(), r.size());

            const uint8_t pos = reduce_mod_block_size<P>(r.data());
            ptr_set_bit(e, i * OC::RSD_BLOCK_SIZE + pos, 1);
        }
        return;
    }

    for (std::size_t i = 0; i < OC::RSD_NOISE_WEIGHT; ++i)
    {
        std::array<uint8_t, IV_SIZE> iv{};
        std::array<uint8_t, P::secpar_bytes> r{};
        iv[0] = static_cast<uint8_t>(i);
        iv[1] = static_cast<uint8_t>(i >> 8);
        ref_prg::prg(seed_sk, iv.data(), 0, r.data(), P::secpar_bits, r.size());

        const uint8_t pos = reduce_mod_block_size<P>(r.data());
        ptr_set_bit(e, i * OC::RSD_BLOCK_SIZE + pos, 1);
    }
}

template <typename P> std::vector<uint8_t> sample_matrix_b(const uint8_t* seed_pk)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t cols = OC::RSD_DIMENSION;

    std::vector<uint8_t> matrix(ceil_bytes(rows * cols));
    hash_state h;
    h.init(P::secpar_v);
    h.update_byte(3);
    h.update(seed_pk, P::secpar_bytes);
    h.finalize(matrix.data(), matrix.size());
    return matrix;
}

template <typename P> const std::vector<uint8_t>& cached_matrix_b(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint8_t> matrix;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        cache.matrix = sample_matrix_b<P>(seed_pk);
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.matrix;
}

template <typename P> std::vector<uint8_t> matrix_b_idx8(const uint8_t* matrix)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t cols = OC::RSD_DIMENSION;
    constexpr std::size_t idx_bits = 8;
    constexpr std::size_t chunks = (cols + idx_bits - 1) / idx_bits;
    constexpr std::size_t matrix_bytes = ceil_bytes(rows * cols);

    std::vector<uint8_t> idx8(chunks * rows);
    for (std::size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const std::size_t col_base = chunk * idx_bits;
        const std::size_t width = cols - col_base < idx_bits ? cols - col_base : idx_bits;
        uint8_t* out = idx8.data() + chunk * rows;
        for (std::size_t row = 0; row < rows; ++row)
            out[row] = ptr_get_bits(matrix, matrix_bytes, row * cols + col_base, width);
    }
    return idx8;
}

template <typename P> const std::vector<uint8_t>& cached_matrix_b_idx8(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint8_t> idx8;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        const auto& matrix = cached_matrix_b<P>(seed_pk);
        cache.idx8 = matrix_b_idx8<P>(matrix.data());
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.idx8;
}

template <typename P> std::vector<uint64_t> matrix_b_u64(const uint8_t* matrix)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t cols = OC::RSD_DIMENSION;
    constexpr std::size_t chunks = (cols + 63) / 64;
    constexpr std::size_t matrix_bytes = ceil_bytes(rows * cols);

    std::vector<uint64_t> chunks64(rows * chunks);
    for (std::size_t row = 0; row < rows; ++row)
    {
        uint64_t* out = chunks64.data() + row * chunks;
        for (std::size_t chunk = 0; chunk < chunks; ++chunk)
        {
            const std::size_t col_base = chunk * 64;
            const std::size_t width = cols - col_base < 64 ? cols - col_base : 64;
            out[chunk] = ptr_get_u64_bits(matrix, matrix_bytes, row * cols + col_base, width);
        }
    }
    return chunks64;
}

template <typename P> const std::vector<uint64_t>& cached_matrix_b_u64(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint64_t> chunks64;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        const auto& matrix = cached_matrix_b<P>(seed_pk);
        cache.chunks64 = matrix_b_u64<P>(matrix.data());
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.chunks64;
}

template <typename P>
uint8_t matrix_b_witness_bit(const uint8_t* matrix, std::size_t row, std::size_t witness_col)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t cols = OC::RSD_DIMENSION;

    const std::size_t block = witness_col / OC::RSD_PACKED_BLOCK_BITS;
    const std::size_t offset = witness_col % OC::RSD_PACKED_BLOCK_BITS;
    const std::size_t original_col = block * OC::RSD_BLOCK_SIZE + offset;
    const std::size_t parity_col = block * OC::RSD_BLOCK_SIZE + OC::RSD_PACKED_BLOCK_BITS;
    return ptr_get_bit(matrix, row * cols + original_col) ^
           ptr_get_bit(matrix, row * cols + parity_col);
}

template <typename P> std::vector<uint8_t> matrix_b_witness_idx8(const uint8_t* matrix)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t witness_cols = OC::RSD_WITNESS_BITS;
    constexpr std::size_t chunks = (witness_cols + 7) / 8;

    std::vector<uint8_t> idx8(chunks * rows);
    for (std::size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const std::size_t col_base = chunk * 8;
        const std::size_t width = witness_cols - col_base < 8 ? witness_cols - col_base : 8;
        uint8_t* out = idx8.data() + chunk * rows;
        for (std::size_t row = 0; row < rows; ++row)
        {
            uint8_t idx = 0;
            for (std::size_t bit = 0; bit < width; ++bit)
                idx |= static_cast<uint8_t>(matrix_b_witness_bit<P>(matrix, row, col_base + bit)
                                            << bit);
            out[row] = idx;
        }
    }
    return idx8;
}

template <typename P>
const std::vector<uint8_t>& cached_matrix_b_witness_idx8(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint8_t> idx8;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        const auto& matrix = cached_matrix_b<P>(seed_pk);
        cache.idx8 = matrix_b_witness_idx8<P>(matrix.data());
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.idx8;
}

template <typename P> std::vector<uint64_t> matrix_b_witness_u64(const uint8_t* matrix)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t witness_cols = OC::RSD_WITNESS_BITS;
    constexpr std::size_t chunks = (witness_cols + 63) / 64;

    std::vector<uint64_t> chunks64(rows * chunks);
    for (std::size_t row = 0; row < rows; ++row)
    {
        uint64_t* out = chunks64.data() + row * chunks;
        for (std::size_t chunk = 0; chunk < chunks; ++chunk)
        {
            const std::size_t col_base = chunk * 64;
            const std::size_t width = witness_cols - col_base < 64 ? witness_cols - col_base : 64;
            uint64_t bits = 0;
            for (std::size_t bit = 0; bit < width; ++bit)
                bits |= static_cast<uint64_t>(matrix_b_witness_bit<P>(matrix, row, col_base + bit))
                        << bit;
            out[chunk] = bits;
        }
    }
    return chunks64;
}

template <typename P>
const std::vector<uint64_t>& cached_matrix_b_witness_u64(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint64_t> chunks64;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        const auto& matrix = cached_matrix_b<P>(seed_pk);
        cache.chunks64 = matrix_b_witness_u64<P>(matrix.data());
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.chunks64;
}

template <typename P> std::vector<uint8_t> matrix_b_witness_constant(const uint8_t* matrix)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t cols = OC::RSD_DIMENSION;
    constexpr std::size_t blocks = OC::RSD_DIMENSION / OC::RSD_BLOCK_SIZE;

    std::vector<uint8_t> constant(OC::OWF_OUTPUT_BYTES);
    for (std::size_t row = 0; row < rows; ++row)
    {
        uint8_t bit = 0;
        for (std::size_t block = 0; block < blocks; ++block)
            bit ^= ptr_get_bit(matrix,
                               row * cols + block * OC::RSD_BLOCK_SIZE + OC::RSD_PACKED_BLOCK_BITS);
        ptr_set_bit(constant.data(), row, bit);
    }
    return constant;
}

template <typename P>
const std::vector<uint8_t>& cached_matrix_b_witness_constant(const uint8_t* seed_pk)
{
    struct cache_t
    {
        bool valid = false;
        std::array<uint8_t, P::secpar_bytes> seed{};
        std::vector<uint8_t> constant;
    };

    thread_local cache_t cache;
    if (!cache.valid || std::memcmp(cache.seed.data(), seed_pk, P::secpar_bytes) != 0)
    {
        const auto& matrix = cached_matrix_b<P>(seed_pk);
        cache.constant = matrix_b_witness_constant<P>(matrix.data());
        std::memcpy(cache.seed.data(), seed_pk, P::secpar_bytes);
        cache.valid = true;
    }
    return cache.constant;
}

template <typename P>
void syndrome_from_noise(uint8_t* syndrome, const uint8_t* matrix_b, const uint8_t* e)
{
    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t rows = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t cols = OC::RSD_DIMENSION;
    constexpr std::size_t matrix_bytes = ceil_bytes(rows * cols);
    constexpr std::size_t e_bytes = ceil_bytes(OC::RSD_CODE_LENGTH);

    std::memset(syndrome, 0, OC::OWF_OUTPUT_BYTES);
    for (std::size_t row = 0; row < rows; ++row)
    {
        unsigned int parity = ptr_get_bit(e, row);
        for (std::size_t col = 0; col < cols; col += 64)
        {
            const std::size_t width = cols - col < 64 ? cols - col : 64;
            const uint64_t matrix_bits =
                ptr_get_u64_bits(matrix_b, matrix_bytes, row * cols + col, width);
            const uint64_t noise_bits = ptr_get_u64_bits(e, e_bytes, rows + col, width);
            parity ^= static_cast<unsigned int>(
                __builtin_parityll(static_cast<unsigned long long>(matrix_bits & noise_bits)));
        }
        ptr_set_bit(syndrome, row, static_cast<uint8_t>(parity));
    }
}

template <typename P> void owf(uint8_t* syndrome, const uint8_t* seed_sk, const uint8_t* seed_pk)
{
    using OC = typename P::OWF_CONSTS;

    std::vector<uint8_t> e(ceil_bytes(OC::RSD_CODE_LENGTH));
    auto matrix_b = sample_matrix_b<P>(seed_pk);

    sample_noise<P>(e.data(), seed_sk);
    syndrome_from_noise<P>(syndrome, matrix_b.data(), e.data());
}

template <typename P> void extend_witness_from_noise(uint8_t* witness, const uint8_t* e)
{
    using OC = typename P::OWF_CONSTS;

    std::memset(witness, 0, OC::WITNESS_BITS / 8);
    constexpr std::size_t a_bits = OC::RSD_CODE_LENGTH - OC::RSD_DIMENSION;
    constexpr std::size_t b_blocks = OC::RSD_DIMENSION / OC::RSD_BLOCK_SIZE;
    for (std::size_t block = 0; block < b_blocks; ++block)
    {
        for (std::size_t j = 0; j < OC::RSD_PACKED_BLOCK_BITS; ++j)
        {
            const uint8_t bit = ptr_get_bit(e, a_bits + block * OC::RSD_BLOCK_SIZE + j);
            ptr_set_bit(witness, block * OC::RSD_PACKED_BLOCK_BITS + j, bit);
        }
    }
}

template <typename P>
void extend_witness(uint8_t* witness, const uint8_t* seed_sk, const uint8_t* seed_pk)
{
    (void)seed_pk;
    using OC = typename P::OWF_CONSTS;

    std::vector<uint8_t> e(ceil_bytes(OC::RSD_CODE_LENGTH));
    sample_noise<P>(e.data(), seed_sk);

    extend_witness_from_noise<P>(witness, e.data());
}

template <typename P>
void owf_and_extend_witness(uint8_t* syndrome, uint8_t* witness, const uint8_t* seed_sk,
                            const uint8_t* seed_pk)
{
    using OC = typename P::OWF_CONSTS;

    std::vector<uint8_t> e(ceil_bytes(OC::RSD_CODE_LENGTH));
    auto matrix_b = sample_matrix_b<P>(seed_pk);

    sample_noise<P>(e.data(), seed_sk);
    syndrome_from_noise<P>(syndrome, matrix_b.data(), e.data());
    extend_witness_from_noise<P>(witness, e.data());
}

template <typename P>
void owf_and_extend_witness_with_matrix(uint8_t* syndrome, uint8_t* witness, const uint8_t* seed_sk,
                                        const uint8_t* matrix_b)
{
    using OC = typename P::OWF_CONSTS;

    std::vector<uint8_t> e(ceil_bytes(OC::RSD_CODE_LENGTH));
    sample_noise<P>(e.data(), seed_sk);
    syndrome_from_noise<P>(syndrome, matrix_b, e.data());
    extend_witness_from_noise<P>(witness, e.data());
}

} // namespace rsd
} // namespace sig

#endif
