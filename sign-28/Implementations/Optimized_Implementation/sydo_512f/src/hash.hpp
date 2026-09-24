#ifndef HASH_HPP
#define HASH_HPP

#include "block.hpp"
#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

extern "C"
{
#include "auxfunc.h"
}

namespace sydo
{

inline int auxfunc_pseudo_xof_bytes(unsigned long long output_len_bits, const unsigned char* msg,
                                    size_t msg_len, unsigned char* output)
{
    static const unsigned char empty_msg = 0;
    if (msg_len == 0)
        msg = &empty_msg;

    return pseudoXOF(output_len_bits, msg, msg_len * 8, output);
}

inline int auxfunc_pseudo_xof_bytes_with_suffix(unsigned long long output_len_bits,
                                                unsigned char* msg, size_t msg_len,
                                                unsigned char suffix, unsigned char* output)
{
    const unsigned char saved = msg[msg_len];
    msg[msg_len] = suffix;
    const int rc = auxfunc_pseudo_xof_bytes(output_len_bits, msg, msg_len + 1, output);
    msg[msg_len] = saved;
    return rc;
}

#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
static void hash_append_copy(unsigned char* dst, const unsigned char* src, size_t bytes)
{
    std::memcpy(dst, src, bytes);
}

inline int auxfunc_pseudo_xof_concat(unsigned long long output_len_bits, const void* part0,
                                     size_t part0_len, const void* part1, size_t part1_len,
                                     const void* part2, size_t part2_len, unsigned char* output)
{
    static const unsigned char empty_msg = 0;
    if (part0_len == 0 && part2_len == 0)
    {
        const unsigned char* msg =
            part1_len == 0 ? &empty_msg : static_cast<const unsigned char*>(part1);
        return auxfunc_pseudo_xof_bytes(output_len_bits, msg, part1_len, output);
    }

    const size_t input_len = part0_len + part1_len + part2_len;
    constexpr size_t STACK_INPUT_BYTES = 256;
    unsigned char stack_input[STACK_INPUT_BYTES];
    std::vector<unsigned char> heap_input;
    unsigned char* input = stack_input;
    if (input_len > STACK_INPUT_BYTES)
    {
        heap_input.resize(input_len);
        input = heap_input.data();
    }

    size_t offset = 0;
    if (part0_len != 0)
    {
        std::memcpy(input + offset, part0, part0_len);
        offset += part0_len;
    }
    if (part1_len != 0)
    {
        std::memcpy(input + offset, part1, part1_len);
        offset += part1_len;
    }
    if (part2_len != 0)
        std::memcpy(input + offset, part2, part2_len);

    const unsigned char* msg = input_len == 0 ? &empty_msg : input;
    return auxfunc_pseudo_xof_bytes(output_len_bits, msg, input_len, output);
}

struct hash_state
{
    static constexpr size_t stack_input_bytes = 512;
    std::array<unsigned char, stack_input_bytes> stack_input{};
    std::vector<unsigned char> heap_input;
    size_t input_size = 0;
    bool using_heap = false;

    inline const unsigned char* data() const
    {
        return using_heap ? heap_input.data() : stack_input.data();
    }

    inline void reserve(size_t bytes)
    {
        if (bytes <= stack_input_bytes)
            return;
        heap_input.reserve(bytes);
        if (!using_heap)
        {
            heap_input.resize(input_size);
            if (input_size != 0)
                hash_append_copy(heap_input.data(), stack_input.data(), input_size);
            using_heap = true;
        }
    }

    inline int init(secpar s)
    {
        (void)s;
        heap_input.clear();
        input_size = 0;
        using_heap = false;
        return 0;
    }

    inline int update(const void* input_ptr, size_t bytes)
    {
        if (bytes == 0)
            return 0;

        const auto* bytes_ptr = static_cast<const unsigned char*>(input_ptr);
        const size_t old_size = input_size;
        const size_t new_size = old_size + bytes;
        if (using_heap || new_size > stack_input_bytes)
        {
            if (!using_heap)
            {
                heap_input.resize(old_size);
                if (old_size != 0)
                    hash_append_copy(heap_input.data(), stack_input.data(), old_size);
                using_heap = true;
            }
            heap_input.resize(new_size);
            hash_append_copy(heap_input.data() + old_size, bytes_ptr, bytes);
        }
        else
        {
            hash_append_copy(stack_input.data() + old_size, bytes_ptr, bytes);
        }
        input_size = new_size;
        return 0;
    }

    inline int update_byte(uint8_t b) { return this->update(&b, 1); }

    inline int finalize(void* digest, size_t bytes)
    {
        static const unsigned char empty_msg = 0;
        const unsigned char* msg = input_size == 0 ? &empty_msg : data();
        return auxfunc_pseudo_xof_bytes(bytes * 8, msg, input_size,
                                        static_cast<unsigned char*>(digest));
    }
};

struct hash_state_x4
{
    std::shared_ptr<std::vector<unsigned char>> shared_input;
    std::array<std::vector<unsigned char>, 4> suffixes;
    bool diverged = false;

    inline const unsigned char* shared_data() const { return shared_input->data(); }
    inline size_t shared_size() const { return shared_input->size(); }

    inline void reserve_shared(size_t bytes) { shared_input->reserve(bytes); }

    static inline void append_bytes(std::vector<unsigned char>& dst, const void* data, size_t size)
    {
        if (size == 0)
            return;
        const auto* bytes = static_cast<const unsigned char*>(data);
        const size_t old_size = dst.size();
        dst.resize(old_size + size);
        hash_append_copy(dst.data() + old_size, bytes, size);
    }

    inline void init(secpar s)
    {
        (void)s;
        shared_input = std::make_shared<std::vector<unsigned char>>();
        for (auto& suffix : suffixes)
            suffix.clear();
        diverged = false;
    }

    inline void update(const void** data, size_t size)
    {
        diverged = true;
        for (size_t i = 0; i < 4; ++i)
            append_bytes(suffixes[i], data[i], size);
    }

    inline void update_4(const void* data0, const void* data1, const void* data2, const void* data3,
                         size_t size)
    {
        const void* data[4] = {data0, data1, data2, data3};
        this->update(data, size);
    }

    inline void update_1(const void* data, size_t size)
    {
        if (!diverged)
            append_bytes(*shared_input, data, size);
        else
        {
            for (auto& suffix : suffixes)
                append_bytes(suffix, data, size);
        }
    }

    inline void update_1_byte(uint8_t b) { this->update_1(&b, 1); }

    inline void init_prefix(secpar s, const uint8_t prefix)
    {
        this->init(s);
        this->update_1(&prefix, sizeof(prefix));
    }

    inline void finalize(void** buffer, size_t buflen)
    {
        if (!diverged)
        {
            static const unsigned char empty_msg = 0;
            const unsigned char* msg = shared_input->empty() ? &empty_msg : shared_data();
            auxfunc_pseudo_xof_bytes(buflen * 8, msg, shared_size(),
                                     static_cast<unsigned char*>(buffer[0]));
            for (size_t i = 1; i < 4; ++i)
                std::memcpy(buffer[i], buffer[0], buflen);
            return;
        }

        auto& shared = *shared_input;
        const size_t old_size = shared.size();
        size_t max_suffix_size = 0;
        for (const auto& suffix : suffixes)
            max_suffix_size = std::max(max_suffix_size, suffix.size());
        shared.reserve(old_size + max_suffix_size);

        for (size_t i = 0; i < 4; ++i)
        {
            const size_t suffix_size = suffixes[i].size();
            shared.resize(old_size + suffix_size);
            if (suffix_size != 0)
                hash_append_copy(shared.data() + old_size, suffixes[i].data(), suffix_size);

            static const unsigned char empty_msg = 0;
            const unsigned char* msg = shared.empty() ? &empty_msg : shared.data();
            auxfunc_pseudo_xof_bytes(buflen * 8, msg, shared.size(),
                                     static_cast<unsigned char*>(buffer[i]));
            shared.resize(old_size);
        }
    }

    inline void finalize_4(void* buffer0, void* buffer1, void* buffer2, void* buffer3,
                           size_t buflen)
    {
        void* buffer[4] = {buffer0, buffer1, buffer2, buffer3};
        this->finalize(buffer, buflen);
    }
};

using hash_state_x8 = hash_state_x4;

template <secpar S>
inline void xof_prg_impl(const block_secpar<S>* __restrict__ keys, const block128& iv,
                         const uint32_t& tweak, size_t num_keys, size_t num_bytes,
                         uint8_t* __restrict__ output)
{
    unsigned char suffix[sizeof(iv) + sizeof(tweak) + 1];
    std::memcpy(suffix, &iv, sizeof(iv));
    std::memcpy(suffix + sizeof(iv), &tweak, sizeof(tweak));
    suffix[sizeof(iv) + sizeof(tweak)] = 0;

    for (size_t i = 0; i < num_keys; ++i)
    {
        auxfunc_pseudo_xof_concat(num_bytes * 8, &keys[i], sizeof(keys[i]), suffix,
                                  sizeof(suffix), nullptr, 0, output + i * num_bytes);
    }
}

} // namespace sydo

#endif
