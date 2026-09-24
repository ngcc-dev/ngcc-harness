#ifndef SYDO_KEYS_INC
#define SYDO_KEYS_INC

#include "keys.hpp"

#include "hash.hpp"

#include <cassert>
#include <algorithm>
#include <cstring>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>

namespace sydo
{

namespace sydo_detail
{

template <typename P>
inline void sydo_hash_domain(uint8_t* output, size_t output_len, uint8_t domain,
                               const uint8_t* part0, size_t part0_len,
                               const uint8_t* part1 = nullptr, size_t part1_len = 0,
                               const uint8_t* part2 = nullptr, size_t part2_len = 0)
{
    hash_state hasher;
    hasher.init(P::secpar_v);
    if (part0 && part0_len)
        hasher.update(part0, part0_len);
    if (part1 && part1_len)
        hasher.update(part1, part1_len);
    if (part2 && part2_len)
        hasher.update(part2, part2_len);
    hasher.update_byte(domain);
    hasher.finalize(output, output_len);
}

template <typename P>
constexpr std::size_t packed_vector_bytes = P::packed_public_key_bytes - P::secpar_bytes;

template <typename P> inline uint8_t* vector_bytes(sydo_public_key<P>* pk)
{
    return reinterpret_cast<uint8_t*>(pk->y.data());
}

template <typename P> inline const uint8_t* vector_bytes(const sydo_public_key<P>* pk)
{
    return reinterpret_cast<const uint8_t*>(pk->y.data());
}

template <typename P> inline void clear_unused_y_bits(sydo_public_key<P>* pk)
{
    constexpr std::size_t used_bits = P::rsd_codim;
    constexpr std::size_t used_bytes = (used_bits + 7) / 8;
    constexpr std::size_t total_bytes = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P> * P::secpar_bytes;

    static_assert(used_bits > 0, "rsd_codim must be positive");
    static_assert(used_bytes <= total_bytes, "rsd_codim exceeds y storage");

    uint8_t* y_bytes = vector_bytes(pk);
    if constexpr ((used_bits & 7u) != 0)
    {
        constexpr uint8_t keep_mask = static_cast<uint8_t>((1u << (used_bits & 7u)) - 1u);
        y_bytes[used_bytes - 1] &= keep_mask;
    }
    if constexpr (used_bytes < total_bytes)
        memset(y_bytes + used_bytes, 0, total_bytes - used_bytes);
}

template <typename P> constexpr std::size_t sk_pk_offset = 0;
template <typename P> constexpr std::size_t sk_seed_offset = SYDO_PACKED_PUBLIC_KEY_BYTES<P>;
template <typename P>
constexpr std::size_t sk_witness_offset =
    SYDO_PACKED_PUBLIC_KEY_BYTES<P> + SYDO_SEED_SK_BYTES<P>;

inline bool get_bit_lsb_first(const uint8_t* bytes, std::size_t bit_index)
{
    return ((bytes[bit_index >> 3] >> (bit_index & 7)) & 0x01) != 0;
}

inline void set_bit_lsb_first(uint8_t* bytes, std::size_t bit_index, bool bit)
{
    const uint8_t mask = static_cast<uint8_t>(1u << (bit_index & 7));
    if (bit)
        bytes[bit_index >> 3] |= mask;
    else
        bytes[bit_index >> 3] &= static_cast<uint8_t>(~mask);
}

inline uint32_t load_bits_lsb_first(const uint8_t* bytes, std::size_t bit_offset,
                                    std::size_t bit_len)
{
    uint32_t v = 0;
    for (std::size_t i = 0; i < bit_len; ++i)
        v |= static_cast<uint32_t>(get_bit_lsb_first(bytes, bit_offset + i)) << i;
    return v;
}

inline void store_bits_lsb_first(uint8_t* bytes, std::size_t bit_offset, std::size_t bit_len,
                                 uint32_t v)
{
    for (std::size_t i = 0; i < bit_len; ++i)
        set_bit_lsb_first(bytes, bit_offset + i, ((v >> i) & 0x1u) != 0);
}

constexpr std::size_t ceil_log2_u64(std::size_t x)
{
    std::size_t bits = 0;
    std::size_t v = (x <= 1) ? 0 : (x - 1);
    while (v)
    {
        ++bits;
        v >>= 1;
    }
    return bits;
}

constexpr std::size_t sydo_ncr(std::size_t n, std::size_t r)
{
    if (r > n)
        return 0;
    if (r > n - r)
        r = n - r;
    std::size_t res = 1;
    for (std::size_t i = 1; i <= r; ++i)
        res = (res * (n - r + i)) / i;
    return res;
}

template <typename P> constexpr std::size_t domain_term_count_for_block(std::size_t b)
{
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
    {
        return P::ncr(P::block_sizes[b], P::block_wts[b]);
    }
    else
    {
        std::size_t total = 0;
        for (std::size_t t = 0; t <= P::block_wts[b]; ++t)
            total += P::ncr(P::block_sizes[b], t);
        return total;
    }
}

inline bool write_reduced_exact_weight_bits(uint8_t* out, std::size_t n, std::size_t w,
                                            std::size_t rank, std::size_t dst_off)
{
    std::size_t left = w;
    for (std::size_t pos = 0; pos < n; ++pos)
    {
        if (left == 0)
        {
            if (pos + 1 < n)
            {
                for (std::size_t t = pos; t + 1 < n; ++t)
                    set_bit_lsb_first(out, dst_off + t, false);
            }
            return rank == 0;
        }
        if ((n - pos) < left)
            return false;

        const std::size_t skip_count = sydo_ncr(n - pos - 1, left);
        const bool take = (rank >= skip_count);
        if (take)
        {
            rank -= skip_count;
            --left;
        }

        if (pos + 1 < n)
            set_bit_lsb_first(out, dst_off + pos, take);
    }
    return (left == 0) && (rank == 0);
}

inline bool write_full_subset_bits(uint8_t* out, std::size_t n, std::size_t w, std::size_t rank,
                                   std::size_t dst_off)
{
    for (std::size_t pos = 0; pos < n; ++pos)
        set_bit_lsb_first(out, dst_off + pos, false);

    std::size_t left = w;
    for (std::size_t pos = 0; pos < n; ++pos)
    {
        if (left == 0)
            return rank == 0;
        if ((n - pos) < left)
            return false;

        const std::size_t skip_count = sydo_ncr(n - pos - 1, left);
        const bool take = (rank >= skip_count);
        if (take)
        {
            rank -= skip_count;
            --left;
            set_bit_lsb_first(out, dst_off + pos, true);
        }
    }
    return (left == 0) && (rank == 0);
}

inline bool write_hamming_ball_bits(uint8_t* out, std::size_t n, std::size_t r,
                                    std::size_t rank, std::size_t dst_off)
{
    for (std::size_t w = 0; w <= r; ++w)
    {
        const std::size_t count = sydo_ncr(n, w);
        if (rank < count)
            return write_full_subset_bits(out, n, w, rank, dst_off);
        rank -= count;
    }
    return false;
}

template <typename P>
inline bool write_elementary_encoding_bits(uint8_t* out, std::size_t block_idx,
                                           std::size_t rank, std::size_t dst_off)
{
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
    {
        return write_reduced_exact_weight_bits(out, P::block_sizes[block_idx],
                                               P::block_wts[block_idx], rank, dst_off);
    }
    else
    {
        return write_hamming_ball_bits(out, P::block_sizes[block_idx], P::block_wts[block_idx],
                                       rank, dst_off);
    }
}

template <typename P> constexpr std::size_t stored_block_bits(std::size_t block_idx)
{
    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_sphere)
        return P::block_sizes[block_idx] - 1;
    else
        return P::block_sizes[block_idx];
}

template <typename P> constexpr std::size_t rsd_block_len = P::rsd_n / P::rsd_w;
template <typename P> constexpr std::size_t rsd_pos_bits = ceil_log2_u64(rsd_block_len<P>);
template <typename P> constexpr std::size_t rsd_pos_stream_bits = P::rsd_w * rsd_pos_bits<P>;
template <typename P> constexpr std::size_t rsd_pos_stream_bytes = (rsd_pos_stream_bits<P> + 7) / 8;
template <typename P> constexpr std::size_t rsd_x_bits = P::rsd_n;
template <typename P> constexpr std::size_t rsd_x_bytes = (rsd_x_bits<P> + 7) / 8;
template <typename P>
constexpr std::size_t h_prg_blocks_per_elem =
    (P::secpar_bytes + sizeof(typename P::prg_t::block_t) - 1) / sizeof(typename P::prg_t::block_t);

template <typename P>
inline block_secpar<P::secpar_v>
compose_secpar_block_from_parts(const typename P::prg_t::block_t* parts)
{
    using h_block_t = typename P::prg_t::block_t;
    using y_block_t = block_secpar<P::secpar_v>;

    if constexpr (std::is_same_v<h_block_t, y_block_t> && h_prg_blocks_per_elem<P> == 1)
    {
        return parts[0];
    }

    y_block_t out = y_block_t::set_zero();
    constexpr std::size_t y_bytes = sizeof(y_block_t);
    constexpr std::size_t h_bytes = sizeof(h_block_t);

    uint8_t* out_bytes = reinterpret_cast<uint8_t*>(&out);
    std::size_t copied = 0;
    for (std::size_t i = 0; i < h_prg_blocks_per_elem<P> && copied < y_bytes; ++i)
    {
        const std::size_t rem = y_bytes - copied;
        const std::size_t n = (rem < h_bytes) ? rem : h_bytes;
        memcpy(out_bytes + copied, reinterpret_cast<const uint8_t*>(&parts[i]), n);
        copied += n;
    }

    return out;
}

template <typename P>
inline void sydo_sample_x_positions_from_seed(uint8_t* x_pos_out, const uint8_t* seed,
                                                std::size_t seed_len)
{
    static_assert(P::rsd_w > 0, "rsd_w must be positive");
    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w");
    static_assert(rsd_pos_bits<P> > 0, "rsd block length must be > 1");

    static_assert(rsd_block_len<P> <= std::numeric_limits<uint32_t>::max(),
                  "rsd block length must fit in uint32_t");

    memset(x_pos_out, 0, rsd_pos_stream_bytes<P>);

    constexpr std::size_t stream_block_bytes = 32;
    std::array<uint8_t, stream_block_bytes> stream_block{};
    std::size_t stream_off = stream_block_bytes;
    uint32_t stream_counter = 0;

    const auto refill_stream = [&]()
    {
        std::array<uint8_t, 4> counter_bytes{};
        counter_bytes[0] = static_cast<uint8_t>(stream_counter & 0xffu);
        counter_bytes[1] = static_cast<uint8_t>((stream_counter >> 8) & 0xffu);
        counter_bytes[2] = static_cast<uint8_t>((stream_counter >> 16) & 0xffu);
        counter_bytes[3] = static_cast<uint8_t>((stream_counter >> 24) & 0xffu);
        sydo_hash_domain<P>(stream_block.data(), stream_block.size(), 0x58, seed, seed_len,
                              counter_bytes.data(), counter_bytes.size());
        ++stream_counter;
        stream_off = 0;
    };

    const auto get_byte = [&]()
    {
        if (stream_off == stream_block.size())
            refill_stream();
        return stream_block[stream_off++];
    };

    constexpr uint32_t m = static_cast<uint32_t>(rsd_block_len<P>);
    constexpr uint64_t two32 = (uint64_t{1} << 32);
    constexpr uint64_t t_max = (two32 / m) * m;

    std::size_t block_i = 0;
    while (block_i < P::rsd_w)
    {
        uint32_t v = static_cast<uint32_t>(get_byte());
        v |= static_cast<uint32_t>(get_byte()) << 8;
        v |= static_cast<uint32_t>(get_byte()) << 16;
        v |= static_cast<uint32_t>(get_byte()) << 24;

        if (static_cast<uint64_t>(v) >= t_max)
            continue;

        const uint32_t pos = v % m;
        const std::size_t off = block_i * rsd_pos_bits<P>;
        store_bits_lsb_first(x_pos_out, off, rsd_pos_bits<P>, pos);
        ++block_i;
    }
}

template <typename P, std::size_t parallel_blocks = 8>
inline void sydo_expand_pubkey_prg_blocks(const typename P::prg_t::key_t& prg_key,
                                            std::vector<typename P::prg_t::block_t>& prg_blocks_out)
{
    using prg_t = typename P::prg_t;
    constexpr std::size_t num_keys = 1;
    using count_t = typename prg_t::count_t;

    const std::size_t total_blocks_sz =
        P::rsd_n * SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P> * h_prg_blocks_per_elem<P>;

    prg_blocks_out.resize(total_blocks_sz);
    if (total_blocks_sz == 0)
        return;

    typename prg_t::key_t key[num_keys] = {prg_key};
    typename prg_t::expanded_key_t expanded[num_keys];
    typename prg_t::tweak_t tweaks[num_keys] = {0};
    typename prg_t::iv_t iv = prg_t::iv_t::set_zero();
    count_t counters[num_keys] = {0};

    const std::size_t full_blocks = (total_blocks_sz / parallel_blocks) * parallel_blocks;
    if (full_blocks != 0)
    {
        prg_t::template init<num_keys, parallel_blocks>(key, expanded, iv, tweaks, counters,
                                                        prg_blocks_out.data());
    }
    else
    {
        typename prg_t::block_t tail[parallel_blocks];
        prg_t::template init<num_keys, parallel_blocks>(key, expanded, iv, tweaks, counters,
                                                        tail);
        memcpy(prg_blocks_out.data(), tail, total_blocks_sz * sizeof(typename prg_t::block_t));
        return;
    }

    for (std::size_t i = parallel_blocks; i < full_blocks; i += parallel_blocks)
    {
        counters[0] = static_cast<count_t>(i);
        prg_t::template gen<num_keys, parallel_blocks>(expanded, iv, tweaks, counters,
                                                       prg_blocks_out.data() + i);
    }

    if (full_blocks != total_blocks_sz)
    {
        typename prg_t::block_t tail[parallel_blocks];
        counters[0] = static_cast<count_t>(full_blocks);
        prg_t::template gen<num_keys, parallel_blocks>(expanded, iv, tweaks, counters, tail);
        memcpy(prg_blocks_out.data() + full_blocks, tail,
               (total_blocks_sz - full_blocks) * sizeof(typename prg_t::block_t));
    }
}

template <typename P> inline void sydo_expand_x_positions(uint8_t* x_out, const uint8_t* x_pos)
{
    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w");

    memset(x_out, 0, rsd_x_bytes<P>);
    for (std::size_t block_i = 0; block_i < P::rsd_w; ++block_i)
    {
        const std::size_t pos_off = block_i * rsd_pos_bits<P>;
        const uint32_t raw = load_bits_lsb_first(x_pos, pos_off, rsd_pos_bits<P>);
        const std::size_t pos = raw % rsd_block_len<P>;
        const std::size_t bit_index = block_i * rsd_block_len<P> + pos;
        set_bit_lsb_first(x_out, bit_index, true);
    }
}

template <typename P>
inline void sydo_compute_y_from_x(sydo_public_key<P>* pk, const uint8_t* x,
                                    const std::vector<typename P::prg_t::block_t>& h_blocks)
{
    using h_block_t = typename P::prg_t::block_t;
    using y_block_t = block_secpar<P::secpar_v>;

    constexpr std::size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    constexpr std::size_t cols = P::rsd_n;
    constexpr std::size_t blocks_per_elem = h_prg_blocks_per_elem<P>;

    assert(h_blocks.size() == rows * cols * blocks_per_elem);

    pk->y.fill(y_block_t::set_zero());

    h_block_t elem_parts[blocks_per_elem];

    for (std::size_t row = 0; row < rows; ++row)
    {
        y_block_t acc = y_block_t::set_zero();
        const std::size_t row_offset = row * cols;
        for (std::size_t col = 0; col < cols; ++col)
        {
            if (get_bit_lsb_first(x, col))
            {
                const std::size_t elem_idx = row_offset + col;
                const std::size_t base = elem_idx * blocks_per_elem;
                for (std::size_t p = 0; p < blocks_per_elem; ++p)
                    elem_parts[p] = h_blocks[base + p];
                acc = acc ^ compose_secpar_block_from_parts<P>(elem_parts);
            }
        }
        pk->y[row] = acc;
    }

    clear_unused_y_bits<P>(pk);
}

template <typename P>
inline void
sydo_compute_y_from_x_positions(sydo_public_key<P>* pk, const uint8_t* x_pos,
                                  const std::vector<typename P::prg_t::block_t>& h_blocks)
{
    using h_block_t = typename P::prg_t::block_t;
    using y_block_t = block_secpar<P::secpar_v>;

    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w");

    constexpr std::size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    constexpr std::size_t cols = P::rsd_n;
    constexpr std::size_t block_len = rsd_block_len<P>;
    constexpr std::size_t blocks_per_elem = h_prg_blocks_per_elem<P>;

    assert(h_blocks.size() == rows * cols * blocks_per_elem);

    std::array<std::size_t, P::rsd_w> selected_cols{};
    for (std::size_t block_i = 0; block_i < P::rsd_w; ++block_i)
    {
        const std::size_t pos_off = block_i * rsd_pos_bits<P>;
        const uint32_t raw = load_bits_lsb_first(x_pos, pos_off, rsd_pos_bits<P>);
        assert(raw < block_len);
        const std::size_t pos = raw;
        selected_cols[block_i] = block_i * block_len + pos;
    }

    pk->y.fill(y_block_t::set_zero());

    h_block_t elem_parts[blocks_per_elem];

    for (std::size_t row = 0; row < rows; ++row)
    {
        y_block_t acc = y_block_t::set_zero();
        const std::size_t row_offset = row * cols;
        for (std::size_t block_i = 0; block_i < P::rsd_w; ++block_i)
        {
            const std::size_t elem_idx = row_offset + selected_cols[block_i];
            const std::size_t base = elem_idx * blocks_per_elem;
            for (std::size_t p = 0; p < blocks_per_elem; ++p)
                elem_parts[p] = h_blocks[base + p];
            acc = acc ^ compose_secpar_block_from_parts<P>(elem_parts);
        }
        pk->y[row] = acc;
    }

    clear_unused_y_bits<P>(pk);
}

template <typename P>
inline void sydo_compute_y_from_x_positions_on_demand(sydo_public_key<P>* pk,
                                                        const uint8_t* x_pos,
                                                        const typename P::prg_t::key_t& prg_key)
{
    using prg_t = typename P::prg_t;
    using h_block_t = typename prg_t::block_t;
    using y_block_t = block_secpar<P::secpar_v>;
    using count_t = typename prg_t::count_t;

    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w");

    constexpr std::size_t rows = SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>;
    constexpr std::size_t cols = P::rsd_n;
    constexpr std::size_t block_len = rsd_block_len<P>;
    constexpr std::size_t blocks_per_elem = h_prg_blocks_per_elem<P>;

    std::array<std::size_t, P::rsd_w> selected_cols{};
    for (std::size_t block_i = 0; block_i < P::rsd_w; ++block_i)
    {
        const std::size_t pos_off = block_i * rsd_pos_bits<P>;
        const uint32_t raw = load_bits_lsb_first(x_pos, pos_off, rsd_pos_bits<P>);
        assert(raw < block_len);
        selected_cols[block_i] = block_i * block_len + static_cast<std::size_t>(raw);
    }

    typename prg_t::key_t key[1] = {prg_key};
    typename prg_t::expanded_key_t expanded[1];
    typename prg_t::tweak_t tweaks[1] = {0};
    typename prg_t::iv_t iv = prg_t::iv_t::set_zero();
    count_t counters[1] = {0};
    h_block_t elem_parts[blocks_per_elem];
    prg_t::template init<1, blocks_per_elem>(key, expanded, iv, tweaks, counters, elem_parts);

    pk->y.fill(y_block_t::set_zero());

    for (std::size_t row = 0; row < rows; ++row)
    {
        y_block_t acc = y_block_t::set_zero();
        const std::size_t row_offset = row * cols;
        for (std::size_t block_i = 0; block_i < P::rsd_w; ++block_i)
        {
            const std::size_t idx = row_offset + selected_cols[block_i];
            counters[0] = static_cast<count_t>(idx * blocks_per_elem);
            prg_t::template gen<1, blocks_per_elem>(expanded, iv, tweaks, counters, elem_parts);
            acc = acc ^ compose_secpar_block_from_parts<P>(elem_parts);
        }
        pk->y[row] = acc;
    }

    clear_unused_y_bits<P>(pk);
}

template <typename P>
inline bool
compute_witness_from_x_positions(std::array<uint8_t, SYDO_WITNESS_BYTES<P>>* witness_out,
                                 const uint8_t* x_positions)
{
    if (!witness_out || !x_positions)
        return false;

    static_assert(P::num_var_blocks >= 2, "num_var_blocks must be >= 2");
    static_assert(P::rsd_n % P::rsd_w == 0, "rsd_n must be divisible by rsd_w");

    constexpr std::size_t num_blocks = P::num_var_blocks;
    constexpr std::size_t chunk_bits = sydo_detail::rsd_pos_bits<P>;
    constexpr std::size_t row_bits = P::input_reduced_bits;
    constexpr std::size_t required_bits = P::rsd_w * row_bits;

    constexpr std::array<std::size_t, num_blocks> block_term_counts = []
    {
        std::array<std::size_t, num_blocks> out{};
        for (std::size_t b = 0; b < num_blocks; ++b)
            out[b] = domain_term_count_for_block<P>(b);
        return out;
    }();
    std::array<std::size_t, num_blocks> block_strides{};
    std::size_t input_size = 1;
    {
        std::size_t stride = 1;
        for (std::size_t rev = 0; rev < num_blocks; ++rev)
        {
            const std::size_t b = num_blocks - 1 - rev;
            block_strides[b] = stride;
            stride *= block_term_counts[b];
        }
        input_size = stride;
    }

    if (required_bits > SYDO_WITNESS_BYTES<P> * 8)
        return false;
    if (P::elementary_vector_len > input_size)
        return false;

    for (std::size_t b = 0; b < num_blocks; ++b)
    {
        if (P::block_wts[b] > P::block_sizes[b])
            return false;
        if (block_term_counts[b] == 0)
            return false;
    }

    witness_out->fill(0);

    for (std::size_t subvec_i = 0; subvec_i < P::rsd_w; ++subvec_i)
    {
        const std::size_t off = subvec_i * chunk_bits;
        const std::size_t idx = static_cast<std::size_t>(
            sydo_detail::load_bits_lsb_first(x_positions, off, chunk_bits));
        if (idx >= P::elementary_vector_len)
            return false;

        std::size_t rem = idx;
        std::size_t raw_off = subvec_i * row_bits;
        for (std::size_t b = 0; b < num_blocks; ++b)
        {
            const std::size_t local = rem / block_strides[b];
            rem %= block_strides[b];

            if (!write_elementary_encoding_bits<P>(witness_out->data(), b, local, raw_off))
                return false;
            raw_off += stored_block_bits<P>(b);
        }
        if (rem != 0)
            return false;
    }

    return true;
}

template <typename P>
inline bool
compute_witness_from_column_indices(std::array<uint8_t, SYDO_WITNESS_BYTES<P>>* witness_out,
                                    const uint16_t* column_indices)
{
    if (!witness_out || !column_indices)
        return false;

    constexpr std::size_t num_blocks = P::num_var_blocks;
    constexpr std::size_t row_bits = P::input_reduced_bits;
    constexpr std::size_t required_bits = P::witness_rows * row_bits;

    std::array<std::size_t, num_blocks> block_term_counts{};
    std::array<std::size_t, num_blocks> block_strides{};
    std::size_t input_size = 1;
    for (std::size_t b = 0; b < num_blocks; ++b)
        block_term_counts[b] = domain_term_count_for_block<P>(b);
    {
        std::size_t stride = 1;
        for (std::size_t rev = 0; rev < num_blocks; ++rev)
        {
            const std::size_t b = num_blocks - 1 - rev;
            block_strides[b] = stride;
            stride *= block_term_counts[b];
        }
        input_size = stride;
    }

    if (required_bits > SYDO_WITNESS_BYTES<P> * 8)
        return false;
    if (input_size < P::witness_rows)
        return false;

    witness_out->fill(0);

    for (std::size_t subvec_i = 0; subvec_i < P::witness_rows; ++subvec_i)
    {
        std::size_t idx = column_indices[subvec_i];
        if (idx >= input_size)
            return false;

        std::size_t raw_off = subvec_i * row_bits;
        for (std::size_t b = 0; b < num_blocks; ++b)
        {
            const std::size_t local = idx / block_strides[b];
            idx %= block_strides[b];

            if (!write_elementary_encoding_bits<P>(witness_out->data(), b, local, raw_off))
                return false;
            raw_off += stored_block_bits<P>(b);
        }
        if (idx != 0)
            return false;
    }

    return true;
}

template <typename P>
inline void
compute_witness(uint8_t* witness_extended_out, const uint8_t* witness_raw)
{
    memset(witness_extended_out, 0, SYDO_WITNESS_EXTENDED_BYTES<P>);

    constexpr std::size_t raw_subvector_bits = P::input_reduced_bits;
    constexpr std::size_t ext_subvector_bits = SYDO_WITNESS_EXTENDED_SUBVECTOR_BITS<P>;

    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
    {
        static_assert(raw_subvector_bits == ext_subvector_bits,
                      "Hamming-ball witnesses are stored without block compression.");
        memcpy(witness_extended_out, witness_raw, SYDO_WITNESS_BYTES<P>);
        return;
    }

    for (std::size_t subvector_idx = 0; subvector_idx < P::witness_rows; ++subvector_idx)
    {
        const std::size_t subvector_raw_off = subvector_idx * raw_subvector_bits;
        const std::size_t subvector_ext_off = subvector_idx * ext_subvector_bits;

        std::size_t raw_off = 0;
        std::size_t ext_off = 0;
        for (std::size_t b = 0; b < P::num_var_blocks; ++b)
        {
            const std::size_t reduced_len = P::block_sizes[b] - 1;
            bool t = (P::block_wts[b] & 1u) != 0;
            for (std::size_t k = 0; k < reduced_len; ++k)
            {
                const bool bit =
                    get_bit_lsb_first(witness_raw, subvector_raw_off + raw_off + k);
                set_bit_lsb_first(witness_extended_out, subvector_ext_off + ext_off + k, bit);
                t ^= bit;
            }
            set_bit_lsb_first(witness_extended_out, subvector_ext_off + ext_off + reduced_len, t);

            raw_off += reduced_len;
            ext_off += reduced_len + 1;
        }
    }
}

template <typename P>
inline void
compute_witness(std::array<uint8_t, SYDO_WITNESS_EXTENDED_BYTES<P>>* witness_extended_out,
                const std::array<uint8_t, SYDO_WITNESS_BYTES<P>>& witness_raw)
{
    compute_witness<P>(witness_extended_out->data(), witness_raw.data());
}

} // namespace sydo_detail

template <typename P>
void sydo_pack_public_key(uint8_t* packed, const sydo_public_key<P>* unpacked)
{
    static_assert(sizeof(typename P::prg_t::key_t) == P::secpar_bytes,
                  "PRG key size must match secpar bytes");

    memcpy(packed, &unpacked->seed_pk, sizeof(unpacked->seed_pk));
    memcpy(packed + P::secpar_bytes, sydo_detail::vector_bytes(unpacked),
           sydo_detail::packed_vector_bytes<P>);
}

template <typename P>
void sydo_unpack_public_key(sydo_public_key<P>* unpacked, const uint8_t* packed)
{
    static_assert(sizeof(typename P::prg_t::key_t) == P::secpar_bytes,
                  "PRG key size must match secpar bytes");

    unpacked->seed_pk = P::prg_t::key_t::set_zero();
    unpacked->y.fill(block_secpar<P::secpar_v>::set_zero());

    memcpy(&unpacked->seed_pk, packed, sizeof(unpacked->seed_pk));
    memcpy(sydo_detail::vector_bytes(unpacked), packed + P::secpar_bytes,
           sydo_detail::packed_vector_bytes<P>);
}

template <typename P>
void sydo_pack_secret_key(uint8_t* packed, const sydo_secret_key<P>* unpacked)
{
    sydo_pack_public_key<P>(packed + sydo_detail::sk_pk_offset<P>, &unpacked->pk);
    memcpy(packed + sydo_detail::sk_seed_offset<P>, &unpacked->seed_sk, SYDO_SEED_SK_BYTES<P>);
    memcpy(packed + sydo_detail::sk_witness_offset<P>, unpacked->witness.data(),
           SYDO_WITNESS_BYTES<P>);
}

template <typename P>
bool sydo_unpack_secret_key(sydo_secret_key<P>* unpacked, const uint8_t* packed)
{
    if (!unpacked || !packed)
        return false;

    sydo_unpack_public_key<P>(&unpacked->pk, packed + sydo_detail::sk_pk_offset<P>);
    unpacked->seed_sk = P::prg_t::key_t::set_zero();
    memcpy(&unpacked->seed_sk, packed + sydo_detail::sk_seed_offset<P>, SYDO_SEED_SK_BYTES<P>);
    memcpy(unpacked->witness.data(), packed + sydo_detail::sk_witness_offset<P>,
           SYDO_WITNESS_BYTES<P>);
    return true;
}

template <typename P> bool sydo_compute_witness(const sydo_secret_key<P>* sk)
{
    // TODO(sydo): witness is derived during key generation via x-positions.
    // Keep this as a placeholder until an explicit recompute/verify API is needed.
    return sk != nullptr;
}

template <typename P>
bool sydo_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed,
                                     sydo_secret_key<P>* sk)
{
    if (!sydo_unpack_secret_key<P>(sk, sk_packed))
        return false;
    sydo_pack_public_key<P>(pk_packed, &sk->pk);
    return true;
}

template <typename P> bool sydo_seckey(const uint8_t* sk_packed) { return sk_packed != nullptr; }

template <typename P> bool sydo_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed)
{
    if (!pk_packed || !sk_packed)
        return false;
    sydo_secret_key<P> sk;
    if (!sydo_unpack_secret_key<P>(&sk, sk_packed))
        return false;

    std::array<uint8_t, sydo_detail::rsd_pos_stream_bytes<P>> x_positions{};
    sydo_detail::sydo_sample_x_positions_from_seed<P>(
        x_positions.data(), reinterpret_cast<const uint8_t*>(&sk.seed_sk), SYDO_SEED_SK_BYTES<P>);

    std::vector<typename P::prg_t::block_t> prg_blocks;
    sydo_detail::sydo_expand_pubkey_prg_blocks<P, 8>(sk.pk.seed_pk, prg_blocks);
    sydo_detail::sydo_compute_y_from_x_positions<P>(&sk.pk, x_positions.data(), prg_blocks);
    sydo_pack_public_key<P>(pk_packed, &sk.pk);
    return true;
}

} // namespace sydo

#endif
