#ifndef HASH_HPP
#define HASH_HPP

#include "block.hpp"
#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <vector>

#if defined(XOF_PSEUDO)

extern "C"
{
#include "auxfunc.h"
}

#endif

extern "C"
{
#include "sha3/KeccakHashtimes4.h"
#include <KeccakHash.h>
}

namespace sig
{

inline int keccak_shake_init(Keccak_HashInstance* ctx, secpar s)
{
    switch (s)
    {
    case secpar::s128:
        return Keccak_HashInitialize_SHAKE128(ctx);
    case secpar::s160:
    case secpar::s192:
    case secpar::s256:
        return Keccak_HashInitialize_SHAKE256(ctx);
    case secpar::s384:
        return Keccak_HashInitialize(ctx, 832, 768, 0, 0x1F);
    case secpar::s512:
        return Keccak_HashInitialize(ctx, 576, 1024, 0, 0x1F);
    }

    __builtin_unreachable();
}

inline void keccak_shake_init_x4(Keccak_HashInstancetimes4* ctx, secpar s)
{
    switch (s)
    {
    case secpar::s128:
        Keccak_HashInitializetimes4_SHAKE128(ctx);
        return;
    case secpar::s160:
    case secpar::s192:
    case secpar::s256:
        Keccak_HashInitializetimes4_SHAKE256(ctx);
        return;
    case secpar::s384:
        Keccak_HashInitializetimes4(ctx, 832, 768, 0, 0x1F);
        return;
    case secpar::s512:
        Keccak_HashInitializetimes4(ctx, 576, 1024, 0, 0x1F);
        return;
    }

    __builtin_unreachable();
}

#if defined(XOF_PSEUDO)

struct hash_state
{
    std::vector<uint8_t> msg;
    std::vector<uint8_t> output;
    size_t squeezed = 0;
    bool finalized = false;

    inline int init(secpar)
    {
        msg.clear();
        output.clear();
        squeezed = 0;
        finalized = false;
        return 0;
    }

    inline int update(const void* input, size_t bytes)
    {
        if (bytes == 0)
            return 0;

        const auto* data = static_cast<const uint8_t*>(input);
        msg.insert(msg.end(), data, data + bytes);
        return 0;
    }

    inline int update_byte(uint8_t b) { return this->update(&b, 1); }

    inline int generate_output(size_t bytes)
    {
        constexpr size_t block_size = 32;
        const size_t required_blocks = (bytes + block_size - 1) / block_size;
        const size_t generated_blocks = output.size() / block_size;
        if (required_blocks <= generated_blocks)
            return 0;

        const size_t msg_bytes = msg.size();
        std::vector<uint8_t> input(msg_bytes + 4);
        if (msg_bytes != 0)
            std::memcpy(input.data(), msg.data(), msg_bytes);

        output.resize(required_blocks * block_size);
        for (size_t block = generated_blocks; block < required_blocks; ++block)
        {
            const uint32_t counter = static_cast<uint32_t>(block + 1);
            input[msg_bytes] = static_cast<uint8_t>(counter >> 24);
            input[msg_bytes + 1] = static_cast<uint8_t>(counter >> 16);
            input[msg_bytes + 2] = static_cast<uint8_t>(counter >> 8);
            input[msg_bytes + 3] = static_cast<uint8_t>(counter);

            const int ret =
                sm3hash(256, input.data(), static_cast<unsigned long long>(input.size()) * 8,
                        output.data() + block * block_size);
            if (ret != 0)
                return ret;
        }
        return 0;
    }

    inline int finalize(void* digest, size_t bytes)
    {
        if (bytes == 0)
            return 0;

        finalized = true;
        const size_t out_bytes = squeezed + bytes;
        int ret = generate_output(out_bytes);
        if (ret != 0)
            return ret;

        std::memcpy(digest, output.data() + squeezed, bytes);
        squeezed += bytes;
        return 0;
    }
};

struct hash_state_x4
{
    std::array<hash_state, 4> ctx;

    inline void init(secpar s)
    {
        for (auto& state : ctx)
            state.init(s);
    }

    inline void update(const void** data, size_t size)
    {
        for (size_t i = 0; i < ctx.size(); ++i)
            ctx[i].update(data[i], size);
    }

    inline void update_4(const void* data0, const void* data1, const void* data2, const void* data3,
                         size_t size)
    {
        const void* data[4] = {data0, data1, data2, data3};
        this->update(data, size);
    }

    inline void update_1(const void* data, size_t size)
    {
        this->update_4(data, data, data, data, size);
    }

    inline void update_1_byte(uint8_t b) { this->update_1(&b, 1); }

    inline void init_prefix(secpar s, const uint8_t prefix)
    {
        this->init(s);
        this->update_1(&prefix, sizeof(prefix));
    }

    inline void finalize(void** buffer, size_t buflen)
    {
        for (size_t i = 0; i < ctx.size(); ++i)
            ctx[i].finalize(buffer[i], buflen);
    }

    inline void finalize_4(void* buffer0, void* buffer1, void* buffer2, void* buffer3,
                           size_t buflen)
    {
        void* buffer[4] = {buffer0, buffer1, buffer2, buffer3};
        this->finalize(buffer, buflen);
    }
};

#else

// TODO: Is the error checking really needed?
// Either add it to hash_state_x4, or remove it from hash_state.

struct hash_state
{
    Keccak_HashInstance ctx;
    bool finalized = false;

    inline int init(secpar s)
    {
        finalized = false;
        return keccak_shake_init(&this->ctx, s);
    }

    inline int update(const void* input, size_t bytes)
    {
        return Keccak_HashUpdate(&this->ctx, (const uint8_t*)input, bytes * 8);
    }

    inline int update_byte(uint8_t b) { return this->update(&b, 1); }

    inline int finalize(void* digest, size_t bytes)
    {
        if (!finalized)
        {
            int ret = Keccak_HashFinal(&this->ctx, NULL);
            if (ret != KECCAK_SUCCESS)
                return ret;
            finalized = true;
        }
        return Keccak_HashSqueeze(&this->ctx, (uint8_t*)digest, bytes * 8);
    }
};

// Mostly copied from Picnic: Instances that work with 4 states in parallel.
struct hash_state_x4
{
    Keccak_HashInstancetimes4 ctx;

    inline void init(secpar s)
    {
        keccak_shake_init_x4(&this->ctx, s);
    }

    inline void update(const void** data, size_t size)
    {
        const uint8_t* data_casted[4] = {(const uint8_t*)data[0], (const uint8_t*)data[1],
                                         (const uint8_t*)data[2], (const uint8_t*)data[3]};
        Keccak_HashUpdatetimes4(&this->ctx, data_casted, size << 3);
    }

    inline void update_4(const void* data0, const void* data1, const void* data2, const void* data3,
                         size_t size)
    {
        const void* data[4] = {data0, data1, data2, data3};
        this->update(data, size);
    }

    inline void update_1(const void* data, size_t size)
    {
        this->update_4(data, data, data, data, size);
    }

    inline void update_1_byte(uint8_t b) { this->update_1(&b, 1); }

    inline void init_prefix(secpar s, const uint8_t prefix)
    {
        this->init(s);
        this->update_1(&prefix, sizeof(prefix));
    }

    inline void finalize(void** buffer, size_t buflen)
    {
        uint8_t* buffer_casted[4] = {(uint8_t*)buffer[0], (uint8_t*)buffer[1], (uint8_t*)buffer[2],
                                     (uint8_t*)buffer[3]};
        Keccak_HashFinaltimes4(&this->ctx, NULL);
        Keccak_HashSqueezetimes4(&this->ctx, buffer_casted, buflen << 3);
    }

    inline void finalize_4(void* buffer0, void* buffer1, void* buffer2, void* buffer3,
                           size_t buflen)
    {
        void* buffer[4] = {buffer0, buffer1, buffer2, buffer3};
        this->finalize(buffer, buflen);
    }
};

#endif

template <secpar S>
inline void shake_prg_impl(const block_secpar<S>* __restrict__ keys, const block128& iv,
                           const uint32_t& tweak, size_t num_keys, size_t num_bytes,
                           uint8_t* __restrict__ output)
{
    size_t i;
    for (i = 0; i + 4 <= num_keys; i += 4)
    {
        const void* key_arr[4];
        void* output_arr[4];
        for (size_t j = 0; j < 4; ++j)
        {
            key_arr[j] = &keys[i + j];
            output_arr[j] = output + (i + j) * num_bytes;
        }

        hash_state_x4 hasher;
        hasher.init(S);
        hasher.update(key_arr, sizeof(keys[i]));
        hasher.update_1(&iv, sizeof(iv));
        hasher.update_1(&tweak, sizeof(tweak));
        hasher.update_1_byte(0);
        hasher.finalize(output_arr, num_bytes);
    }

    for (; i < num_keys; ++i)
    {
        hash_state hasher;
        hasher.init(S);
        hasher.update(&keys[i], sizeof(keys[i]));
        hasher.update(&iv, sizeof(iv));
        hasher.update(&tweak, sizeof(tweak));
        hasher.update_byte(0);
        hasher.finalize(output + i * num_bytes, num_bytes);
    }
}

} // namespace sig

#endif
