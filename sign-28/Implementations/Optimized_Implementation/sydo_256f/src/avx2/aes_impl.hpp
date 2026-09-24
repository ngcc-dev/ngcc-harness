#ifndef AES_IMPL_HPP
#define AES_IMPL_HPP

#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <inttypes.h>
#include <wmmintrin.h>

#include "../aes_defs.hpp"
#include "../constants.hpp"
#include "../transpose.hpp"
#include "../util.hpp"

namespace sydo
{

template <secpar S, size_t num_keys, uint32_t num_blocks>
void aes_keygen_impl(aes_round_keys<S>* aeses, const block_secpar<S>* keys, block128* output);
template <secpar S, size_t num_keys>
void aes_keygen_only_impl(aes_round_keys<S>* aeses, const block_secpar<S>* keys);
void rijndael192_encrypt_block(const rijndael192_round_keys* __restrict__ fixed_key,
                               block192* __restrict__ block);

template <secpar S>
inline void aes_round_function(const aes_round_keys<S>* __restrict__ round_keys,
                               block128* __restrict__ block, block128* __restrict__ after_sbox,
                               size_t round)
{
    block128 state = *block;
    block128 state_after_sbox = {_mm_aesenclast_si128(state.data, block128::set_zero().data)};
    *after_sbox = state_after_sbox;

    if (round < AES_ROUNDS<S>)
        state = {_mm_aesenc_si128(state.data, round_keys->keys[round].data)};
    else
        state = state_after_sbox ^ round_keys->keys[round];
    *block = state;
}

template <secpar S>
ALWAYS_INLINE void aes_round(const aes_round_keys<S>* aeses, block128* state, size_t num_keys,
                             size_t evals_per_key, size_t round)
{
    PRAGMA_UNROLL(2 * AES_PREFERRED_WIDTH)
    for (size_t i = 0; i < num_keys * evals_per_key; ++i)
        if (round == 0)
            state[i] = state[i] ^ aeses[i / evals_per_key].keys[round];
        else if (round < AES_ROUNDS<S>)
            state[i] = {_mm_aesenc_si128(state[i].data, aeses[i / evals_per_key].keys[round].data)};
        else
            state[i] = {
                _mm_aesenclast_si128(state[i].data, aeses[i / evals_per_key].keys[round].data)};
}

// This implements the rijndael192 RotateRows step, then cancels out the RotateRows of AES so
// that AES-NI can be used for the sbox. The rijndael192 state is represented with the first 4
// columns in the first block128, and then the last two columns are stored twice in the second
// block128.
ALWAYS_INLINE void rijndael192_rotate_rows_undo_128(block128* s)
{
    __m128i mask = _mm_setr_epi8(0, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0);
    __m128i b0_blended = _mm_blendv_epi8(s[0].data, s[1].data, mask);
    __m128i b1_blended = _mm_blendv_epi8(s[1].data, s[0].data, mask);

    __m128i shuffle_b0 = _mm_setr_epi8(0, 1, 2, 11, 4, 5, 6, 7, 8, 9, 10, 3, 12, 13, 14, 15);
    __m128i shuffle_b1 = _mm_setr_epi8(0, 1, 2, 11, 4, 5, 6, 7, 0, 1, 2, 11, 4, 5, 6, 7);
    s[0] = {_mm_shuffle_epi8(b0_blended, shuffle_b0)};
    s[1] = {_mm_shuffle_epi8(b1_blended, shuffle_b1)};
}

ALWAYS_INLINE void rijndael192_cvt_to_2x128(block128* out, const block192* in)
{
    memcpy(&out[0], &in->data[0], sizeof(out[0]));
    out[1] = {_mm_set1_epi64x(in->data[2])};
}

ALWAYS_INLINE void rijndael192_cvt_trunc160_to_2x128(block128* out, const block160* in)
{
    memcpy(&out[0], in, sizeof(out[0]));
    out[1] = {_mm_set1_epi64x(in->data[4])};
}

ALWAYS_INLINE void rijndael192_store_trunc160_from_2x128(block160* out, const block128* in)
{
    memcpy(out, &in[0], sizeof(in[0]));
    out->data[4] = static_cast<uint32_t>(_mm_cvtsi128_si64(in[1].data));
}

ALWAYS_INLINE void rijndael192_round(const rijndael192_round_keys* round_keys, block192* state,
                                     size_t num_keys, size_t evals_per_key, size_t round)
{
#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH)))
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
    {
        block128 s[2], round_key[2];
        rijndael192_cvt_to_2x128(&s[0], &state[i]);
        rijndael192_cvt_to_2x128(&round_key[0], &round_keys[i / evals_per_key].keys[round]);

        if (round == 0)
        {
            s[0] = s[0] ^ round_key[0];
            s[1] = s[1] ^ round_key[1];
        }
        else if (round < RIJNDAEL_ROUNDS<secpar::s192>)
        {
            rijndael192_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenc_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenc_si128(s[1].data, round_key[1].data)};
        }
        else
        {
            rijndael192_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenclast_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenclast_si128(s[1].data, round_key[1].data)};
        }

        memcpy(&state[i], &s[0], sizeof(block192));
    }
}

template <size_t num_keys, size_t evals_per_key>
ALWAYS_INLINE void rijndael192_round_batched(const rijndael192_round_keys* round_keys,
                                             block192* state, size_t round)
{
    PRAGMA_UNROLL(num_keys)
    for (size_t key_i = 0; key_i < num_keys; ++key_i)
    {
        block128 round_key[2];
        rijndael192_cvt_to_2x128(&round_key[0], &round_keys[key_i].keys[round]);

        PRAGMA_UNROLL(evals_per_key)
        for (size_t eval_i = 0; eval_i < evals_per_key; ++eval_i)
        {
            const size_t state_i = key_i * evals_per_key + eval_i;
            block128 s[2];
            rijndael192_cvt_to_2x128(&s[0], &state[state_i]);

            if (round == 0)
            {
                s[0] = s[0] ^ round_key[0];
                s[1] = s[1] ^ round_key[1];
            }
            else if (round < RIJNDAEL_ROUNDS<secpar::s192>)
            {
                rijndael192_rotate_rows_undo_128(&s[0]);
                s[0] = {_mm_aesenc_si128(s[0].data, round_key[0].data)};
                s[1] = {_mm_aesenc_si128(s[1].data, round_key[1].data)};
            }
            else
            {
                rijndael192_rotate_rows_undo_128(&s[0]);
                s[0] = {_mm_aesenclast_si128(s[0].data, round_key[0].data)};
                s[1] = {_mm_aesenclast_si128(s[1].data, round_key[1].data)};
            }

            memcpy(&state[state_i], &s[0], sizeof(block192));
        }
    }
}

template <size_t num_keys, size_t evals_per_key>
ALWAYS_INLINE void rijndael192_ecb_2x128(const rijndael192_round_keys* __restrict__ round_keys,
                                         block192* __restrict__ data)
{
    block128 state[2 * num_keys * evals_per_key];

#ifdef __GNUC__
    _Pragma("GCC unroll 16")
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
        rijndael192_cvt_to_2x128(&state[2 * i], &data[i]);

#ifdef __GNUC__
    _Pragma("GCC unroll 16")
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
    {
        const size_t key_i = i / evals_per_key;
        block128 round_key[2];
        rijndael192_cvt_to_2x128(&round_key[0], &round_keys[key_i].keys[0]);
        state[2 * i] = state[2 * i] ^ round_key[0];
        state[2 * i + 1] = state[2 * i + 1] ^ round_key[1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s192>; ++round)
    {
#ifdef __GNUC__
        _Pragma("GCC unroll 16")
#endif
            for (size_t i = 0; i < num_keys * evals_per_key; ++i)
        {
            const size_t key_i = i / evals_per_key;
            block128 round_key[2];
            rijndael192_cvt_to_2x128(&round_key[0], &round_keys[key_i].keys[round]);
            rijndael192_rotate_rows_undo_128(&state[2 * i]);
            state[2 * i] = {_mm_aesenc_si128(state[2 * i].data, round_key[0].data)};
            state[2 * i + 1] = {_mm_aesenc_si128(state[2 * i + 1].data, round_key[1].data)};
        }
    }

#ifdef __GNUC__
    _Pragma("GCC unroll 16")
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
    {
        const size_t key_i = i / evals_per_key;
        block128 round_key[2];
        rijndael192_cvt_to_2x128(
            &round_key[0], &round_keys[key_i].keys[RIJNDAEL_ROUNDS<secpar::s192>]);
        rijndael192_rotate_rows_undo_128(&state[2 * i]);
        state[2 * i] = {_mm_aesenclast_si128(state[2 * i].data, round_key[0].data)};
        state[2 * i + 1] = {
            _mm_aesenclast_si128(state[2 * i + 1].data, round_key[1].data)};
    }

#ifdef __GNUC__
    _Pragma("GCC unroll 16")
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
        memcpy(&data[i], &state[2 * i], sizeof(block192));
}

ALWAYS_INLINE block128 rijndael_keygen_rotword_sbox(block128 word, uint32_t round_constant)
{
    __m128i rot_word_then_inv_shift_rows =
        _mm_setr_epi8(1, 14, 11, 4, 5, 2, 15, 8, 9, 6, 3, 12, 13, 10, 7, 0);
    return {_mm_aesenclast_si128(_mm_shuffle_epi8(word.data, rot_word_then_inv_shift_rows),
                                 _mm_set1_epi32(round_constant))};
}

ALWAYS_INLINE block128 rijndael_keygen_sbox(block128 word)
{
    __m128i inv_shift_rows =
        _mm_setr_epi8(0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3);
    return {
        _mm_aesenclast_si128(_mm_shuffle_epi8(word.data, inv_shift_rows), _mm_setzero_si128())};
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael192_store_sliced_round(
    const block128 (*__restrict__ key_slices)[6],
    rijndael192_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128,
    size_t round)
{
    (void) round_keys;
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const size_t first_key = chunk * keygen_width;
        const size_t chunk_size =
            num_keys - first_key < keygen_width ? num_keys - first_key : keygen_width;

        block128 low128s[keygen_width];
        transpose4x4_32(low128s, &key_slices[chunk][0]);

        uint32_t word4[keygen_width];
        uint32_t word5[keygen_width];
        memcpy(word4, &key_slices[chunk][4], sizeof(word4));
        memcpy(word5, &key_slices[chunk][5], sizeof(word5));

        for (size_t j = 0; j < chunk_size; ++j)
        {
            const size_t key_i = first_key + j;
            uint64_t high64 =
                static_cast<uint64_t>(word4[j]) | (static_cast<uint64_t>(word5[j]) << 32);

            if constexpr (store_expanded_keys)
            {
                block192 round_key;
                memcpy(&round_key.data[0], &low128s[j], sizeof(block128));
                round_key.data[2] = high64;
                round_keys[key_i].keys[round] = round_key;
            }
            round_keys_2x128[2 * key_i] = low128s[j];
            round_keys_2x128[2 * key_i + 1] = {_mm_set1_epi64x(high64)};
        }
    }
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael192_keygen_init_sliced(
    const block192* __restrict__ keys,
    block128 (*__restrict__ key_slices)[6],
    rijndael192_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const size_t first_key = chunk * keygen_width;
        const size_t chunk_size =
            num_keys - first_key < keygen_width ? num_keys - first_key : keygen_width;

        block192 chunk_keys[keygen_width];
        for (size_t j = 0; j < chunk_size; ++j)
            chunk_keys[j] = keys[first_key + j];
        for (size_t j = chunk_size; j < keygen_width; ++j)
            chunk_keys[j] = block192::set_zero();

        block128 low128s[keygen_width];
        for (size_t j = 0; j < keygen_width; ++j)
            memcpy(&low128s[j], &chunk_keys[j], sizeof(block128));
        block128 hi64_01 = {_mm_set_epi64x(chunk_keys[1].data[2], chunk_keys[0].data[2])};
        block128 hi64_23 = {_mm_set_epi64x(chunk_keys[3].data[2], chunk_keys[2].data[2])};

        transpose4x4_32(&key_slices[chunk][0], low128s);
        transpose4x2_32(&key_slices[chunk][4], hi64_01, hi64_23);
    }

    rijndael192_store_sliced_round<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, 0);
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael192_keygen_init_sliced_trunc160(
    const block160* __restrict__ keys,
    block128 (*__restrict__ key_slices)[6],
    rijndael192_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const size_t first_key = chunk * keygen_width;
        const size_t chunk_size =
            num_keys - first_key < keygen_width ? num_keys - first_key : keygen_width;

        block160 chunk_keys[keygen_width];
        for (size_t j = 0; j < chunk_size; ++j)
            chunk_keys[j] = keys[first_key + j];
        for (size_t j = chunk_size; j < keygen_width; ++j)
            chunk_keys[j] = block160::set_zero();

        block128 low128s[keygen_width];
        for (size_t j = 0; j < keygen_width; ++j)
            memcpy(&low128s[j], &chunk_keys[j], sizeof(block128));
        block128 hi64_01 = {_mm_set_epi64x(chunk_keys[1].data[4], chunk_keys[0].data[4])};
        block128 hi64_23 = {_mm_set_epi64x(chunk_keys[3].data[4], chunk_keys[2].data[4])};

        transpose4x4_32(&key_slices[chunk][0], low128s);
        transpose4x2_32(&key_slices[chunk][4], hi64_01, hi64_23);
    }

    rijndael192_store_sliced_round<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, 0);
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael192_keygen_round_sliced(
    block128 (*__restrict__ key_slices)[6],
    rijndael192_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128,
    size_t round)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        block128 sbox_out =
            rijndael_keygen_rotword_sbox(key_slices[chunk][5], aes_round_constants[round - 1]);
        key_slices[chunk][0] = key_slices[chunk][0] ^ sbox_out;
        for (size_t word = 1; word < 6; ++word)
            key_slices[chunk][word] = key_slices[chunk][word] ^ key_slices[chunk][word - 1];
    }

    rijndael192_store_sliced_round<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, round);
}

template <size_t num_keys, size_t blocks_per_key, bool store_expanded_keys>
ALWAYS_INLINE void rijndael192_keygen_ecb_impl(const block192* __restrict__ keys,
                                               rijndael192_round_keys* __restrict__ round_keys,
                                               block192* __restrict__ data)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    block128 state[2 * num_keys * blocks_per_key];
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        rijndael192_cvt_to_2x128(&state[2 * i], &data[i]);

    block128 key_slices[chunks][6];
    block128 round_keys_2x128[2 * num_keys];
    rijndael192_keygen_init_sliced<num_keys, store_expanded_keys>(
        keys, key_slices, round_keys, round_keys_2x128);

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        state[2 * i] = state[2 * i] ^ round_keys_2x128[2 * key_i];
        state[2 * i + 1] = state[2 * i + 1] ^ round_keys_2x128[2 * key_i + 1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s192>; ++round)
    {
        rijndael192_keygen_round_sliced<num_keys, store_expanded_keys>(
            key_slices, round_keys, round_keys_2x128, round);
        for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        {
            const size_t key_i = i / blocks_per_key;
            rijndael192_rotate_rows_undo_128(&state[2 * i]);
            state[2 * i] =
                {_mm_aesenc_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
            state[2 * i + 1] = {
                _mm_aesenc_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
        }
    }

    rijndael192_keygen_round_sliced<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, RIJNDAEL_ROUNDS<secpar::s192>);
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        rijndael192_rotate_rows_undo_128(&state[2 * i]);
        state[2 * i] =
            {_mm_aesenclast_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
        state[2 * i + 1] = {
            _mm_aesenclast_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
    }

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        memcpy(&data[i], &state[2 * i], sizeof(block192));
}

template <size_t num_keys, size_t blocks_per_key, bool store_expanded_keys>
ALWAYS_INLINE void
rijndael192_keygen_ecb_trunc160_impl(const block160* __restrict__ keys,
                                     rijndael192_round_keys* __restrict__ round_keys,
                                     block160* __restrict__ data)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    block128 state[2 * num_keys * blocks_per_key];
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        rijndael192_cvt_trunc160_to_2x128(&state[2 * i], &data[i]);

    block128 key_slices[chunks][6];
    block128 round_keys_2x128[2 * num_keys];
    rijndael192_keygen_init_sliced_trunc160<num_keys, store_expanded_keys>(
        keys, key_slices, round_keys, round_keys_2x128);

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        state[2 * i] = state[2 * i] ^ round_keys_2x128[2 * key_i];
        state[2 * i + 1] = state[2 * i + 1] ^ round_keys_2x128[2 * key_i + 1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s192>; ++round)
    {
        rijndael192_keygen_round_sliced<num_keys, store_expanded_keys>(
            key_slices, round_keys, round_keys_2x128, round);
        for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        {
            const size_t key_i = i / blocks_per_key;
            rijndael192_rotate_rows_undo_128(&state[2 * i]);
            state[2 * i] =
                {_mm_aesenc_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
            state[2 * i + 1] = {
                _mm_aesenc_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
        }
    }

    rijndael192_keygen_round_sliced<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, RIJNDAEL_ROUNDS<secpar::s192>);
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        rijndael192_rotate_rows_undo_128(&state[2 * i]);
        state[2 * i] =
            {_mm_aesenclast_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
        state[2 * i + 1] = {
            _mm_aesenclast_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
    }

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        rijndael192_store_trunc160_from_2x128(&data[i], &state[2 * i]);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael192_ecb_trunc160_impl(
    const rijndael192_round_keys* __restrict__ round_keys, block160* __restrict__ data)
{
    block128 state[2 * num_keys * blocks_per_key];
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        rijndael192_cvt_trunc160_to_2x128(&state[2 * i], &data[i]);

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        block128 round_key[2];
        rijndael192_cvt_to_2x128(&round_key[0], &round_keys[key_i].keys[0]);
        state[2 * i] = state[2 * i] ^ round_key[0];
        state[2 * i + 1] = state[2 * i + 1] ^ round_key[1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s192>; ++round)
    {
        for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        {
            const size_t key_i = i / blocks_per_key;
            block128 round_key[2];
            rijndael192_cvt_to_2x128(&round_key[0], &round_keys[key_i].keys[round]);
            rijndael192_rotate_rows_undo_128(&state[2 * i]);
            state[2 * i] = {_mm_aesenc_si128(state[2 * i].data, round_key[0].data)};
            state[2 * i + 1] = {_mm_aesenc_si128(state[2 * i + 1].data, round_key[1].data)};
        }
    }

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        block128 round_key[2];
        rijndael192_cvt_to_2x128(&round_key[0],
                                 &round_keys[key_i].keys[RIJNDAEL_ROUNDS<secpar::s192>]);
        rijndael192_rotate_rows_undo_128(&state[2 * i]);
        state[2 * i] = {_mm_aesenclast_si128(state[2 * i].data, round_key[0].data)};
        state[2 * i + 1] = {_mm_aesenclast_si128(state[2 * i + 1].data, round_key[1].data)};
    }

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        rijndael192_store_trunc160_from_2x128(&data[i], &state[2 * i]);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael192_keygen_ecb(const block192* __restrict__ keys,
                                          rijndael192_round_keys* __restrict__ round_keys,
                                          block192* __restrict__ data)
{
    rijndael192_keygen_ecb_impl<num_keys, blocks_per_key, true>(keys, round_keys, data);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael192_keygen_ecb_no_store(const block192* __restrict__ keys,
                                                   block192* __restrict__ data)
{
    rijndael192_keygen_ecb_impl<num_keys, blocks_per_key, false>(keys, nullptr, data);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael192_keygen_ecb_trunc160(
    const block160* __restrict__ keys, rijndael192_round_keys* __restrict__ round_keys,
    block160* __restrict__ data)
{
    rijndael192_keygen_ecb_trunc160_impl<num_keys, blocks_per_key, true>(
        keys, round_keys, data);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void
rijndael192_keygen_ecb_trunc160_no_store(const block160* __restrict__ keys,
                                         block160* __restrict__ data)
{
    rijndael192_keygen_ecb_trunc160_impl<num_keys, blocks_per_key, false>(keys, nullptr, data);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael192_ecb_trunc160(
    const rijndael192_round_keys* __restrict__ round_keys, block160* __restrict__ data)
{
    rijndael192_ecb_trunc160_impl<num_keys, blocks_per_key>(round_keys, data);
}

// This implements the rijndael256 RotateRows step, then cancels out the RotateRows of AES so
// that AES-NI can be used for the sbox.
ALWAYS_INLINE void rijndael256_rotate_rows_undo_128(block128* s, __m128i mask, __m128i perm)
{
    __m128i b0_blended = _mm_blendv_epi8(s[0].data, s[1].data, mask);
    __m128i b1_blended = _mm_blendv_epi8(s[1].data, s[0].data, mask);

    s[0] = {_mm_shuffle_epi8(b0_blended, perm)};
    s[1] = {_mm_shuffle_epi8(b1_blended, perm)};
}

ALWAYS_INLINE void rijndael256_rotate_rows_undo_128(block128* s)
{
    // Swapping bytes between 128-bit halves is equivalent to rotating left overall, then
    // rotating right within each half.
    const __m128i mask =
        _mm_setr_epi8(0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1);
    // The rotations for 128-bit AES are different, so rotate within the halves to match.
    const __m128i perm = _mm_setr_epi8(0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 2, 3);
    rijndael256_rotate_rows_undo_128(s, mask, perm);
}

ALWAYS_INLINE void rijndael256_round(const rijndael256_round_keys* round_keys, block256* state,
                                     size_t num_keys, size_t evals_per_key, size_t round)
{
#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*AES256_PREFERRED_WIDTH)))
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
    {
        block128 s[2], round_key[2];
        memcpy(&s[0], &state[i], sizeof(block256));
        memcpy(&round_key[0], &round_keys[i / evals_per_key].keys[round], sizeof(block256));

        // Use AES-NI to implement the round function.
        if (round == 0)
        {
            s[0] = s[0] ^ round_key[0];
            s[1] = s[1] ^ round_key[1];
        }
        else if (round < AES_ROUNDS<secpar::s256>)
        {
            rijndael256_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenc_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenc_si128(s[1].data, round_key[1].data)};
        }
        else
        {
            rijndael256_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenclast_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenclast_si128(s[1].data, round_key[1].data)};
        }

        memcpy(&state[i], &s[0], sizeof(block256));
    }
}

template <size_t num_keys, size_t evals_per_key>
ALWAYS_INLINE void rijndael256_round_batched(const rijndael256_round_keys* round_keys,
                                             block256* state, size_t round)
{
    PRAGMA_UNROLL(num_keys)
    for (size_t key_i = 0; key_i < num_keys; ++key_i)
    {
        block128 round_key[2];
        memcpy(&round_key[0], &round_keys[key_i].keys[round], sizeof(block256));

        PRAGMA_UNROLL(evals_per_key)
        for (size_t eval_i = 0; eval_i < evals_per_key; ++eval_i)
        {
            const size_t state_i = key_i * evals_per_key + eval_i;
            block128 s[2];
            memcpy(&s[0], &state[state_i], sizeof(block256));

            if (round == 0)
            {
                s[0] = s[0] ^ round_key[0];
                s[1] = s[1] ^ round_key[1];
            }
            else if (round < RIJNDAEL_ROUNDS<secpar::s256>)
            {
                rijndael256_rotate_rows_undo_128(&s[0]);
                s[0] = {_mm_aesenc_si128(s[0].data, round_key[0].data)};
                s[1] = {_mm_aesenc_si128(s[1].data, round_key[1].data)};
            }
            else
            {
                rijndael256_rotate_rows_undo_128(&s[0]);
                s[0] = {_mm_aesenclast_si128(s[0].data, round_key[0].data)};
                s[1] = {_mm_aesenclast_si128(s[1].data, round_key[1].data)};
            }

            memcpy(&state[state_i], &s[0], sizeof(block256));
        }
    }
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael256_ecb_2x128(const rijndael256_round_keys* __restrict__ round_keys,
                                         block256* __restrict__ data)
{
    constexpr size_t total_blocks = num_keys * blocks_per_key;
    block128 state[2 * total_blocks];
    const __m128i rr_mask =
        _mm_setr_epi8(0, -1, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1);
    const __m128i rr_perm =
        _mm_setr_epi8(0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 2, 3);

    PRAGMA_UNROLL(16)
    for (size_t i = 0; i < total_blocks; ++i)
        memcpy(&state[2 * i], &data[i], sizeof(block256));

    PRAGMA_UNROLL(16)
    for (size_t i = 0; i < total_blocks; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        block128 round_key[2];
        memcpy(&round_key[0], &round_keys[key_i].keys[0], sizeof(block256));
        state[2 * i] = state[2 * i] ^ round_key[0];
        state[2 * i + 1] = state[2 * i + 1] ^ round_key[1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s256>; ++round)
    {
        PRAGMA_UNROLL(16)
        for (size_t i = 0; i < total_blocks; ++i)
        {
            const size_t key_i = i / blocks_per_key;
            block128 round_key[2];
            memcpy(&round_key[0], &round_keys[key_i].keys[round], sizeof(block256));
            rijndael256_rotate_rows_undo_128(&state[2 * i], rr_mask, rr_perm);
            state[2 * i] = {_mm_aesenc_si128(state[2 * i].data, round_key[0].data)};
            state[2 * i + 1] =
                {_mm_aesenc_si128(state[2 * i + 1].data, round_key[1].data)};
        }
    }

    PRAGMA_UNROLL(16)
    for (size_t i = 0; i < total_blocks; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        block128 round_key[2];
        memcpy(&round_key[0], &round_keys[key_i].keys[RIJNDAEL_ROUNDS<secpar::s256>],
               sizeof(block256));
        rijndael256_rotate_rows_undo_128(&state[2 * i], rr_mask, rr_perm);
        state[2 * i] = {_mm_aesenclast_si128(state[2 * i].data, round_key[0].data)};
        state[2 * i + 1] =
            {_mm_aesenclast_si128(state[2 * i + 1].data, round_key[1].data)};
    }

    PRAGMA_UNROLL(16)
    for (size_t i = 0; i < total_blocks; ++i)
        memcpy(&data[i], &state[2 * i], sizeof(block256));
}

ALWAYS_INLINE void aes256_round(const aes256_round_keys* round_keys, block256* state,
                                size_t num_keys, size_t evals_per_key, size_t round)
{
#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*AES256_PREFERRED_WIDTH)))
#endif
        for (size_t i = 0; i < num_keys * evals_per_key; ++i)
    {
        block128 s[2], round_key[2];
        memcpy(&s[0], &state[i], sizeof(block256));
        memcpy(&round_key[0], &round_keys[i / evals_per_key].keys[round], sizeof(block256));

        if (round == 0)
        {
            s[0] = s[0] ^ round_key[0];
            s[1] = s[1] ^ round_key[1];
        }
        else if (round < AES_ROUNDS<secpar::s256>)
        {
            rijndael256_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenc_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenc_si128(s[1].data, round_key[1].data)};
        }
        else
        {
            rijndael256_rotate_rows_undo_128(&s[0]);
            s[0] = {_mm_aesenclast_si128(s[0].data, round_key[0].data)};
            s[1] = {_mm_aesenclast_si128(s[1].data, round_key[1].data)};
        }

        memcpy(&state[i], &s[0], sizeof(block256));
    }
}

ALWAYS_INLINE void rijndael256_encrypt_block(const rijndael256_round_keys* __restrict__ fixed_key,
                                             block256* __restrict__ block)
{
    rijndael256_round(fixed_key, block, 1, 1, 0);
    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s256>; ++round)
        rijndael256_round(fixed_key, block, 1, 1, round);
    rijndael256_round(fixed_key, block, 1, 1, RIJNDAEL_ROUNDS<secpar::s256>);
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael256_store_sliced_round(
    const block128 (*__restrict__ key_slices)[8],
    rijndael256_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128,
    size_t round)
{
    (void) round_keys;
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const size_t first_key = chunk * keygen_width;
        const size_t chunk_size =
            num_keys - first_key < keygen_width ? num_keys - first_key : keygen_width;

        block128 low128s[keygen_width];
        block128 high128s[keygen_width];
        transpose4x4_32(low128s, &key_slices[chunk][0]);
        transpose4x4_32(high128s, &key_slices[chunk][4]);

        for (size_t j = 0; j < chunk_size; ++j)
        {
            const size_t key_i = first_key + j;
            if constexpr (store_expanded_keys)
            {
                block256 round_key;
                memcpy(&round_key, &low128s[j], sizeof(block128));
                memcpy(reinterpret_cast<unsigned char*>(&round_key) + sizeof(block128),
                       &high128s[j], sizeof(block128));
                round_keys[key_i].keys[round] = round_key;
            }
            round_keys_2x128[2 * key_i] = low128s[j];
            round_keys_2x128[2 * key_i + 1] = high128s[j];
        }
    }
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael256_keygen_init_sliced(
    const block256* __restrict__ keys,
    block128 (*__restrict__ key_slices)[8],
    rijndael256_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        const size_t first_key = chunk * keygen_width;
        const size_t chunk_size =
            num_keys - first_key < keygen_width ? num_keys - first_key : keygen_width;

        block256 chunk_keys[keygen_width];
        for (size_t j = 0; j < chunk_size; ++j)
            chunk_keys[j] = keys[first_key + j];
        for (size_t j = chunk_size; j < keygen_width; ++j)
            chunk_keys[j] = block256::set_zero();

        block128 low128s[keygen_width];
        block128 high128s[keygen_width];
        for (size_t j = 0; j < keygen_width; ++j)
        {
            memcpy(&low128s[j], &chunk_keys[j], sizeof(block128));
            memcpy(&high128s[j],
                   reinterpret_cast<unsigned char*>(&chunk_keys[j]) + sizeof(block128),
                   sizeof(block128));
        }

        transpose4x4_32(&key_slices[chunk][0], low128s);
        transpose4x4_32(&key_slices[chunk][4], high128s);
    }

    rijndael256_store_sliced_round<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, 0);
}

template <size_t num_keys, bool store_expanded_keys = true>
ALWAYS_INLINE void rijndael256_keygen_round_sliced(
    block128 (*__restrict__ key_slices)[8],
    rijndael256_round_keys* __restrict__ round_keys,
    block128* __restrict__ round_keys_2x128,
    size_t round)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    for (size_t chunk = 0; chunk < chunks; ++chunk)
    {
        block128 sbox_out =
            rijndael_keygen_rotword_sbox(key_slices[chunk][7], aes_round_constants[round - 1]);
        key_slices[chunk][0] = key_slices[chunk][0] ^ sbox_out;
        for (size_t word = 1; word < 4; ++word)
            key_slices[chunk][word] = key_slices[chunk][word] ^ key_slices[chunk][word - 1];

        sbox_out = rijndael_keygen_sbox(key_slices[chunk][3]);
        key_slices[chunk][4] = key_slices[chunk][4] ^ sbox_out;
        for (size_t word = 5; word < 8; ++word)
            key_slices[chunk][word] = key_slices[chunk][word] ^ key_slices[chunk][word - 1];
    }

    rijndael256_store_sliced_round<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, round);
}

template <size_t num_keys, size_t blocks_per_key, bool store_expanded_keys>
ALWAYS_INLINE void rijndael256_keygen_ecb_impl(const block256* __restrict__ keys,
                                               rijndael256_round_keys* __restrict__ round_keys,
                                               block256* __restrict__ data)
{
    constexpr size_t keygen_width = 4;
    constexpr size_t chunks = (num_keys + keygen_width - 1) / keygen_width;

    block128 state[2 * num_keys * blocks_per_key];
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        memcpy(&state[2 * i], &data[i], sizeof(block256));

    block128 key_slices[chunks][8];
    block128 round_keys_2x128[2 * num_keys];
    rijndael256_keygen_init_sliced<num_keys, store_expanded_keys>(
        keys, key_slices, round_keys, round_keys_2x128);

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        state[2 * i] = state[2 * i] ^ round_keys_2x128[2 * key_i];
        state[2 * i + 1] = state[2 * i + 1] ^ round_keys_2x128[2 * key_i + 1];
    }

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s256>; ++round)
    {
        rijndael256_keygen_round_sliced<num_keys, store_expanded_keys>(
            key_slices, round_keys, round_keys_2x128, round);
        for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        {
            const size_t key_i = i / blocks_per_key;
            rijndael256_rotate_rows_undo_128(&state[2 * i]);
            state[2 * i] =
                {_mm_aesenc_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
            state[2 * i + 1] = {
                _mm_aesenc_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
        }
    }

    rijndael256_keygen_round_sliced<num_keys, store_expanded_keys>(
        key_slices, round_keys, round_keys_2x128, RIJNDAEL_ROUNDS<secpar::s256>);
    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
    {
        const size_t key_i = i / blocks_per_key;
        rijndael256_rotate_rows_undo_128(&state[2 * i]);
        state[2 * i] =
            {_mm_aesenclast_si128(state[2 * i].data, round_keys_2x128[2 * key_i].data)};
        state[2 * i + 1] = {
            _mm_aesenclast_si128(state[2 * i + 1].data, round_keys_2x128[2 * key_i + 1].data)};
    }

    for (size_t i = 0; i < num_keys * blocks_per_key; ++i)
        memcpy(&data[i], &state[2 * i], sizeof(block256));
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael256_keygen_ecb(const block256* __restrict__ keys,
                                          rijndael256_round_keys* __restrict__ round_keys,
                                          block256* __restrict__ data)
{
    rijndael256_keygen_ecb_impl<num_keys, blocks_per_key, true>(keys, round_keys, data);
}

template <size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void rijndael256_keygen_ecb_no_store(const block256* __restrict__ keys,
                                                   block256* __restrict__ data)
{
    rijndael256_keygen_ecb_impl<num_keys, blocks_per_key, false>(keys, nullptr, data);
}

template <secpar S, size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void aes_keygen_ecb(const block_secpar<S>* __restrict__ keys,
                                  aes_round_keys<S>* __restrict__ aeses,
                                  block128* __restrict__ data)
{
    // TODO: can probably ditch this wrapper now.
    aes_keygen_impl<S, num_keys, blocks_per_key>(aeses, keys, data);
}

template <secpar S, size_t num_keys, size_t blocks_per_key>
ALWAYS_INLINE void aes_keygen_only_ecb(const block_secpar<S>* __restrict__ keys,
                                       aes_round_keys<S>* __restrict__ aeses)
{
    (void) blocks_per_key;
    aes_keygen_only_impl<S, num_keys>(aeses, keys);
}

template <secpar S, size_t num_keys, size_t blocks_per_key>
inline void aes_ecb(const aes_round_keys<S>* __restrict__ aeses, block128* __restrict__ data)
{
    // IIRC, GCC won't keep state in registers unless I make this a local variable.
    block128 state[num_keys * blocks_per_key];
    memcpy(state, data, sizeof(state));

    // Make it easier for the compiler to optimize by unwinding the first and last rounds. (Since we
    // aren't asking it to unwind the whole loop.)
    aes_round(aeses, state, num_keys, blocks_per_key, 0);
    for (size_t round = 1; round < AES_ROUNDS<S>; ++round)
        aes_round(aeses, state, num_keys, blocks_per_key, round);
    aes_round(aeses, state, num_keys, blocks_per_key, AES_ROUNDS<S>);

    memcpy(data, state, sizeof(state));
}

template <secpar S, size_t num_keys, size_t blocks_per_key>
inline void rijndael_ecb(const rijndael_round_keys<S>* __restrict__ keys,
                         block_secpar<S>* __restrict__ data)
{
    block_secpar<S> state[num_keys * blocks_per_key];
    memcpy(state, data, sizeof(state));

    if constexpr (S == secpar::s128)
    {
        aes_round<S>(keys, state, num_keys, blocks_per_key, 0);
        for (size_t round = 1; round < RIJNDAEL_ROUNDS<S>; ++round)
            aes_round<S>(keys, state, num_keys, blocks_per_key, round);
        aes_round<S>(keys, state, num_keys, blocks_per_key, RIJNDAEL_ROUNDS<S>);
    }
    else if constexpr (S == secpar::s192)
    {
        rijndael192_ecb_2x128<num_keys, blocks_per_key>(keys, state);
    }
    else if constexpr (S == secpar::s256)
    {
        rijndael256_ecb_2x128<num_keys, blocks_per_key>(keys, state);
    }
    else
    {
        static_assert(false, "unsupported security parameter for Rijndael");
    }

    memcpy(data, state, sizeof(state));
}

template <size_t num_keys, uint32_t num_blocks>
inline void aes_fixed_key_ctr(const aes_round_keys<secpar::s128>* __restrict__ fixed_key,
                              const block128* __restrict__ keys, const block128& iv,
                              const uint32_t* __restrict__ tweaks, const uint32_t* counters,
                              block128* __restrict__ output)
{
    block128 state[num_keys * num_blocks];

    for (size_t l = 0; l < num_keys; ++l)
        for (uint32_t m = 0; m < num_blocks; ++m)
            state[l * num_blocks + m] =
                iv.add32(block128::set_low32(counters[l] + m, tweaks[l])) ^ keys[l];

    aes_round(fixed_key, state, 1, num_keys * num_blocks, 0);
    for (size_t round = 1; round < AES_ROUNDS<secpar::s128>; ++round)
        aes_round(fixed_key, state, 1, num_keys * num_blocks, round);
    aes_round(fixed_key, state, 1, num_keys * num_blocks, AES_ROUNDS<secpar::s128>);

    for (size_t l = 0; l < num_keys; ++l)
        for (uint32_t m = 0; m < num_blocks; ++m)
            output[l * num_blocks + m] = state[l * num_blocks + m] ^ keys[l];
}

template <size_t num_keys, uint32_t num_blocks>
inline void rijndael192_fixed_key_ctr(const rijndael192_round_keys* __restrict__ fixed_key,
                                      const block192* __restrict__ keys, const block192& iv,
                                      const uint32_t* __restrict__ tweaks,
                                      const uint32_t* __restrict__ counters,
                                      block192* __restrict__ output)
{
    for (size_t l = 0; l < num_keys; ++l)
    {
        for (uint32_t m = 0; m < num_blocks; ++m)
        {
            block192 state =
                iv.add32(block192::set_low32(counters[l] + m, tweaks[l])) ^ keys[l];
            rijndael192_encrypt_block(fixed_key, &state);
            output[l * num_blocks + m] = state ^ keys[l];
        }
    }
}

template <size_t num_keys, uint32_t num_blocks>
inline void aes256_fixed_key_ctr(const aes256_round_keys* __restrict__ fixed_key,
                                      const block256* __restrict__ keys, const block256& iv,
                                      const uint32_t* __restrict__ tweaks,
                                      const uint32_t* __restrict__ counters,
                                      block256* __restrict__ output)
{
    block256 state[num_keys * num_blocks];

    for (size_t l = 0; l < num_keys; ++l)
        for (uint32_t m = 0; m < num_blocks; ++m)
            state[l * num_blocks + m] =
                iv.add32(block256::set_low32(counters[l] + m, tweaks[l])) ^ keys[l];

    rijndael256_round(fixed_key, state, 1, num_keys * num_blocks, 0);
    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s256>; ++round)
        rijndael256_round(fixed_key, state, 1, num_keys * num_blocks, round);
    rijndael256_round(fixed_key, state, 1, num_keys * num_blocks,
                      RIJNDAEL_ROUNDS<secpar::s256>);

    for (size_t l = 0; l < num_keys; ++l)
        for (uint32_t m = 0; m < num_blocks; ++m)
            output[l * num_blocks + m] = state[l * num_blocks + m] ^ keys[l];
}

template <size_t num_keys, uint32_t num_blocks>
inline void rijndael256_fixed_key_ctr(const rijndael256_round_keys* __restrict__ fixed_key,
                                      const block256* __restrict__ keys, const block256& iv,
                                      const uint32_t* __restrict__ tweaks,
                                      const uint32_t* __restrict__ counters,
                                      block256* __restrict__ output)
{
    aes256_fixed_key_ctr<num_keys, num_blocks>(fixed_key, keys, iv, tweaks, counters, output);
}

} // namespace sydo

#endif
