#include "aes.hpp"

#include <cassert>

namespace faest
{

#define KEYGEN_WIDTH 4

// State for doing KEYGEN_WIDTH AES key schedules at once.
template <secpar S> struct aes_keygen_state
{
    // These represent round keys for subsequent rounds, only with sbox outputs not yet XORed in.
    block128 key_slices[secpar_to_bits(S) / 32];

    // Input to the next SBox.
    block128 next_sbox;
};

ALWAYS_INLINE void cumulative_xor(block128* x, size_t n)
{
#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*KEYGEN_WIDTH)))
#endif
                for (size_t i = 0; i + 1 < n; ++i) x[i + 1] = x[i] ^ x[i + 1];
}

// output_x[(i + 2) % 6] = input_x[i];
ALWAYS_INLINE void shift2_mod6(block128* x)
{
    block128 output[6];
    memcpy(output + 2, x, 4 * sizeof(block128));
    memcpy(output, x + 4, 2 * sizeof(block128));
    memcpy(x, output, 6 * sizeof(block128));
}

// output_x[(i + 4) % 8] = input_x[i];
ALWAYS_INLINE void shift4_mod8(block128* x)
{
    block128 output[8];
    memcpy(output + 4, x, 4 * sizeof(block128));
    memcpy(output, x + 4, 4 * sizeof(block128));
    memcpy(x, output, 8 * sizeof(block128));
}

// Starts the key schedule on num_keys keys.
template <secpar S>
ALWAYS_INLINE void aes_keygen_init(aes_keygen_state<S>* keygen_states, aes_round_keys<S>* aeses,
                                   const block_secpar<S>* keys_in, size_t num_keys)
{
#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH / KEYGEN_WIDTH)))
#endif
        for (size_t i = 0; i < num_keys;
             i += KEYGEN_WIDTH, keys_in += KEYGEN_WIDTH, aeses += KEYGEN_WIDTH, ++keygen_states)
    {
        size_t chunk_size = num_keys - i < KEYGEN_WIDTH ? num_keys - i : KEYGEN_WIDTH;
        block_secpar<S> keys[KEYGEN_WIDTH];

#ifdef __GNUC__
        _Pragma(STRINGIZE(GCC unroll (KEYGEN_WIDTH)))
#endif
            for (size_t j = 0; j < chunk_size; ++j)
        {
            keys[j] = keys_in[j];

            // Copy out the first round keys
            memcpy(&aeses[j].keys[0], &keys[j], sizeof(block_secpar<S>));
        }
        for (size_t j = chunk_size; j < KEYGEN_WIDTH; ++j)
            keys[j] = block_secpar<S>::set_zero();

        if constexpr (S == secpar::s128)
        {
            transpose4x4_32(&keygen_states->key_slices[0], &keys[0]);
        }
        else if constexpr (S == secpar::s192)
        {
            block128 low128s[KEYGEN_WIDTH];
            for (size_t j = 0; j < KEYGEN_WIDTH; ++j)
                memcpy(&low128s[j], &keys[j], sizeof(block128));
            block128 hi64_01 = {_mm_set_epi64x(keys[1].data[2], keys[0].data[2])};
            block128 hi64_23 = {_mm_set_epi64x(keys[3].data[2], keys[2].data[2])};

            transpose4x4_32(&keygen_states->key_slices[0], &low128s[0]);
            transpose4x2_32(&keygen_states->key_slices[4], hi64_01, hi64_23);
        }
        else if constexpr (S == secpar::s256)
        {
            block128 low128s[KEYGEN_WIDTH];
            block128 hi128s[KEYGEN_WIDTH];
            for (size_t j = 0; j < KEYGEN_WIDTH; ++j)
            {
                memcpy(&low128s[j], &keys[j], sizeof(block128));
                memcpy(&hi128s[j], ((const char*)&keys[j]) + sizeof(block128), sizeof(block128));
            }

            transpose4x4_32(&keygen_states->key_slices[0], &low128s[0]);
            transpose4x4_32(&keygen_states->key_slices[4], &hi128s[0]);
        }
        else
        {
            static_assert(false, "unsupported security parameter for AES");
        }

        keygen_states->next_sbox = keygen_states->key_slices[secpar_to_bits(S) / 32 - 1];

        // Get ready for next rounds (starting with round 1 for 128 or 192 bit keys, or round 2 for
        // 256 bit keys).
        if constexpr (S == secpar::s128)
        {
            cumulative_xor(&keygen_states->key_slices[0], 4);
        }
        else if constexpr (S == secpar::s192)
        {
            shift2_mod6(&keygen_states->key_slices[0]);
            cumulative_xor(&keygen_states->key_slices[2], 4);
        }
        else if constexpr (S == secpar::s256)
        {
            cumulative_xor(&keygen_states->key_slices[0], 4);
            cumulative_xor(&keygen_states->key_slices[4], 4);
        }
        else
        {
            static_assert(false, "unsupported security parameter for AES");
        }
    }
}

template <secpar S>
ALWAYS_INLINE void aes_keygen_round(aes_keygen_state<S>* keygen_states, aes_round_keys<S>* aeses,
                                    size_t num_keys, size_t round)
{
    if (round < secpar_to_bits(S) / 128)
        return;

#ifdef __GNUC__
    _Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH / KEYGEN_WIDTH)))
#endif
        for (size_t i = 0; i < num_keys; i += KEYGEN_WIDTH, aeses += KEYGEN_WIDTH, ++keygen_states)
    {
        size_t chunk_size = num_keys - i < KEYGEN_WIDTH ? num_keys - i : KEYGEN_WIDTH;

        if (S != secpar::s192 || round % 3 < 2)
        {
            // Undo ShiftRows operation, then apply RotWord.
            __m128i inv_shift_rows =
                _mm_setr_epi8(0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3);
            __m128i rot_word_then_inv_shift_rows =
                _mm_setr_epi8(1, 14, 11, 4, 5, 2, 15, 8, 9, 6, 3, 12, 13, 10, 7, 0);

            __m128i perm, round_constant;
            if (S == secpar::s256 && round % 2 == 1)
            {
                perm = inv_shift_rows;
                round_constant = _mm_setzero_si128();
            }
            else
            {
                perm = rot_word_then_inv_shift_rows;

                // Tell the compiler explicitly how to evaluate for each case of S.
                size_t idx; //= (2 * round + 1) / (secpar_to_bits(S) / 64);
                if (S == secpar::s128)
                    idx = round;
                else if (S == secpar::s192)
                    idx = (2 * round + 1) / 3;
                else if (S == secpar::s256)
                    idx = round / 2;

                round_constant = _mm_set1_epi32(aes_round_constants[idx - 1]);
            }

            block128 sbox_out = {_mm_aesenclast_si128(
                _mm_shuffle_epi8(keygen_states->next_sbox.data, perm), round_constant)};

            size_t range_start = (S == secpar::s192 && round % 3 == 1) ? 2 : 0;
            size_t range_end = (S == secpar::s192) ? 6 : 4;
            PRAGMA_UNROLL(6)
            for (size_t j = range_start; j < range_end; ++j)
                keygen_states->key_slices[j] = keygen_states->key_slices[j] ^ sbox_out;
        }

        if (S != secpar::s192 || round % 3 == 2)
            keygen_states->next_sbox = keygen_states->key_slices[3];
        else if (round % 3 == 0)
            keygen_states->next_sbox = keygen_states->key_slices[5];

        // Unslice the current KEYGEN_WIDTH keys slices to get this round's keys.
        block128 round_keys[KEYGEN_WIDTH];
        transpose4x4_32(round_keys, &keygen_states->key_slices[0]);
#ifdef __GNUC__
        _Pragma(STRINGIZE(GCC unroll (KEYGEN_WIDTH)))
#endif
                              for (size_t j = 0; j < chunk_size; ++j) aeses[j]
                                  .keys[round] = round_keys[j];

        // Update round keys slices for next round.
        if constexpr (S == secpar::s192)
        {
            shift2_mod6(&keygen_states->key_slices[0]);
            size_t next_sbox_idx = (round % 3 + 1) * 2;
            cumulative_xor(&keygen_states->key_slices[1], next_sbox_idx - 1);
            cumulative_xor(&keygen_states->key_slices[next_sbox_idx], 6 - next_sbox_idx);
        }
        else
        {
            cumulative_xor(&keygen_states->key_slices[0], 4);
            if constexpr (S == secpar::s256)
                shift4_mod8(&keygen_states->key_slices[0]);
        }
    }
}

template <secpar S, size_t num_keys, uint32_t num_blocks>
ALWAYS_INLINE void aes_keygen_impl(
    aes_round_keys<S>* __restrict__ aeses,
    const block_secpar<S>* __restrict__ keys,
    block128* __restrict__ output)
{
    aes_keygen_state<S> keygen_states[(num_keys + KEYGEN_WIDTH - 1) / KEYGEN_WIDTH];
    aes_keygen_init(keygen_states, aeses, keys, num_keys);

    constexpr auto round_start = []
    {
        if constexpr (S == secpar::s128 || S == secpar::s192)
        {
            return 1;
        }
        else if constexpr (S == secpar::s256)
        {
            return 2;
        }
        else
        {
            static_assert(false, "unsupported security parameter for AES");
        }
    }();
    constexpr auto round_end = AES_ROUNDS<S> - round_start;
    constexpr auto unroll_rounds = []
    {
        if constexpr (S == secpar::s128)
        {
            return 1;
        }
        else if constexpr (S == secpar::s192)
        {
            return 3;
        }
        else if constexpr (S == secpar::s256)
        {
            return 2;
        }
        else
        {
            static_assert(false, "unsupported security parameter for AES");
        }
    }();

    // Separate out the first and last rounds, as they work differently.
    aes_round(aeses, output, num_keys, num_blocks, 0);
    if (round_start > 1)
        aes_round(aeses, output, num_keys, num_blocks, 1);

    for (size_t round = round_start; round <= round_end; round += unroll_rounds)
    {
        // Unroll the loop, as the key generation follows a pattern that repeats every unroll_rounds
        // iterations.

        aes_keygen_round(keygen_states, aeses, num_keys, round);
        aes_round(aeses, output, num_keys, num_blocks, round);
        if (unroll_rounds > 1)
        {
            if (round_end < round + 1)
                break;
            aes_keygen_round(keygen_states, aeses, num_keys, round + 1);
            aes_round(aeses, output, num_keys, num_blocks, round + 1);
        }
        if (unroll_rounds > 2)
        {
            if (round_end < round + 2)
                break;
            aes_keygen_round(keygen_states, aeses, num_keys, round + 2);
            aes_round(aeses, output, num_keys, num_blocks, round + 2);
        }
    }

    aes_keygen_round(keygen_states, aeses, num_keys, round_end + 1);
    aes_round(aeses, output, num_keys, num_blocks, round_end + 1);
    if (round_end + 2 <= AES_ROUNDS<S>)
    {
        aes_keygen_round(keygen_states, aeses, num_keys, round_end + 2);
        aes_round(aeses, output, num_keys, num_blocks, round_end + 2);
    }
}

// Generate all explicit template instantiations.
#define DEF_AES_KEYGEN_IMPL_SKB(secparam, num_keys, num_blocks)                                    \
    template void aes_keygen_impl<secparam, num_keys, num_blocks>(                                 \
        aes_round_keys<secparam>* __restrict__ aeses,                                              \
        const block_secpar<secparam>* __restrict__ keys, block128* __restrict__ output);
#define DEF_AES_KEYGEN_IMPL_SK(secparam, num_keys)                                                 \
    DEF_AES_KEYGEN_IMPL_SKB(secparam, num_keys, 1)                                                 \
    DEF_AES_KEYGEN_IMPL_SKB(secparam, num_keys, 2)                                                 \
    DEF_AES_KEYGEN_IMPL_SKB(secparam, num_keys, 3)                                                 \
    DEF_AES_KEYGEN_IMPL_SKB(secparam, num_keys, 4)
#define DEF_AES_KEYGEN_IMPL_S(secparam)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 1)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 2)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 3)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 4)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 5)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 6)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 7)                                                            \
    DEF_AES_KEYGEN_IMPL_SK(secparam, 8)

DEF_AES_KEYGEN_IMPL_S(secpar::s128)
DEF_AES_KEYGEN_IMPL_S(secpar::s192)
DEF_AES_KEYGEN_IMPL_S(secpar::s256)

template <secpar S> void aes_keygen(aes_round_keys<S>* round_keys, block_secpar<S> key)
{
    // There are more efficient ways to run the key schedule on a single key, but this function
    // isn't used much anyway.
    block128 empty_output;
    aes_keygen_impl<S, 1, 0>(round_keys, &key, &empty_output);
}
template void aes_keygen<secpar::s128>(aes_round_keys<secpar::s128>* round_keys,
                                       block_secpar<secpar::s128> key);
template void aes_keygen<secpar::s192>(aes_round_keys<secpar::s192>* round_keys,
                                       block_secpar<secpar::s192> key);
template void aes_keygen<secpar::s256>(aes_round_keys<secpar::s256>* round_keys,
                                       block_secpar<secpar::s256> key);

static inline __m128i load_high_128(const block256* block)
{
    __m128i out;
    memcpy(&out, ((unsigned char*)block) + sizeof(__m128i), sizeof(out));
    return out;
}

static inline __m128i load_high_64(const block192* block)
{
    return _mm_cvtsi64_si128(block->data[2]);
}

static void rijndael192_keygen_helper(const block192* round_key_in, block128 kga,
                                      block192* round_key_out)
{
    __m128i t1, t2, t4;
    uint64_t t3;

    memcpy(&t1, &round_key_in->data[0], sizeof(t1));
    t2 = kga.data;
    t3 = round_key_in->data[2];

    t2 = _mm_shuffle_epi32(t2, 0x55);
    t4 = _mm_slli_si128(t1, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t1 = _mm_xor_si128(t1, t2);
    t3 ^= (uint32_t)_mm_extract_epi32(t1, 3);
    t3 ^= t3 << 32;

    memcpy(&round_key_out->data[0], &t1, sizeof(t1));
    round_key_out->data[2] = t3;
}

static void rijndael256_keygen_helper(const block256* round_key_in, block128 kga,
                                      block256* round_key_out)
{
    __m128i t1, t2, t3, t4;

    memcpy(&t1, round_key_in, sizeof(t1));
    t3 = load_high_128(round_key_in);
    t2 = kga.data;

    t2 = _mm_shuffle_epi32(t2, 0xff);
    t4 = _mm_slli_si128(t1, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t1 = _mm_xor_si128(t1, t4);
    t1 = _mm_xor_si128(t1, t2);

    memcpy(round_key_out, &t1, sizeof(t1));

    t4 = _mm_aeskeygenassist_si128(t1, 0x00);
    t2 = _mm_shuffle_epi32(t4, 0xaa);
    t4 = _mm_slli_si128(t3, 0x4);
    t3 = _mm_xor_si128(t3, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t3 = _mm_xor_si128(t3, t4);
    t4 = _mm_slli_si128(t4, 0x4);
    t3 = _mm_xor_si128(t3, t4);
    t3 = _mm_xor_si128(t3, t2);

    memcpy(((unsigned char*)round_key_out) + sizeof(t1), &t3, sizeof(t3));
}

void rijndael192_keygen(rijndael192_round_keys* round_keys, block192 key)
{
    round_keys->keys[0] = key;

    block128 kga;
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[0]), 0x01)};
    rijndael192_keygen_helper(&round_keys->keys[0], kga, &round_keys->keys[1]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[1]), 0x02)};
    rijndael192_keygen_helper(&round_keys->keys[1], kga, &round_keys->keys[2]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[2]), 0x04)};
    rijndael192_keygen_helper(&round_keys->keys[2], kga, &round_keys->keys[3]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[3]), 0x08)};
    rijndael192_keygen_helper(&round_keys->keys[3], kga, &round_keys->keys[4]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[4]), 0x10)};
    rijndael192_keygen_helper(&round_keys->keys[4], kga, &round_keys->keys[5]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[5]), 0x20)};
    rijndael192_keygen_helper(&round_keys->keys[5], kga, &round_keys->keys[6]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[6]), 0x40)};
    rijndael192_keygen_helper(&round_keys->keys[6], kga, &round_keys->keys[7]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[7]), 0x80)};
    rijndael192_keygen_helper(&round_keys->keys[7], kga, &round_keys->keys[8]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[8]), 0x1B)};
    rijndael192_keygen_helper(&round_keys->keys[8], kga, &round_keys->keys[9]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[9]), 0x36)};
    rijndael192_keygen_helper(&round_keys->keys[9], kga, &round_keys->keys[10]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[10]), 0x6C)};
    rijndael192_keygen_helper(&round_keys->keys[10], kga, &round_keys->keys[11]);
    kga = {_mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[11]), 0xD8)};
    rijndael192_keygen_helper(&round_keys->keys[11], kga, &round_keys->keys[12]);
}

void rijndael256_keygen(rijndael256_round_keys* round_keys, block256 key)
{
    round_keys->keys[0] = key;

    block128 kga;
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[0]), 0x01)};
    rijndael256_keygen_helper(&round_keys->keys[0], kga, &round_keys->keys[1]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[1]), 0x02)};
    rijndael256_keygen_helper(&round_keys->keys[1], kga, &round_keys->keys[2]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[2]), 0x04)};
    rijndael256_keygen_helper(&round_keys->keys[2], kga, &round_keys->keys[3]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[3]), 0x08)};
    rijndael256_keygen_helper(&round_keys->keys[3], kga, &round_keys->keys[4]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[4]), 0x10)};
    rijndael256_keygen_helper(&round_keys->keys[4], kga, &round_keys->keys[5]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[5]), 0x20)};
    rijndael256_keygen_helper(&round_keys->keys[5], kga, &round_keys->keys[6]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[6]), 0x40)};
    rijndael256_keygen_helper(&round_keys->keys[6], kga, &round_keys->keys[7]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[7]), 0x80)};
    rijndael256_keygen_helper(&round_keys->keys[7], kga, &round_keys->keys[8]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[8]), 0x1B)};
    rijndael256_keygen_helper(&round_keys->keys[8], kga, &round_keys->keys[9]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[9]), 0x36)};
    rijndael256_keygen_helper(&round_keys->keys[9], kga, &round_keys->keys[10]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[10]), 0x6C)};
    rijndael256_keygen_helper(&round_keys->keys[10], kga, &round_keys->keys[11]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[11]), 0xD8)};
    rijndael256_keygen_helper(&round_keys->keys[11], kga, &round_keys->keys[12]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[12]), 0xAB)};
    rijndael256_keygen_helper(&round_keys->keys[12], kga, &round_keys->keys[13]);
    kga = {_mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[13]), 0x4D)};
    rijndael256_keygen_helper(&round_keys->keys[13], kga, &round_keys->keys[14]);
}

static inline void cvt192_to_2x128(block128* out, const block192* in)
{
    memcpy(&out[0], &in->data[0], sizeof(out[0]));
    out[1] = {_mm_set1_epi64x(in->data[2])};
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

// Just do 1 block at a time because this function shouldn't be used much.
void rijndael192_encrypt_block(const rijndael192_round_keys* __restrict__ fixed_key,
                               block192* __restrict__ block)
{
    block192 xored_block = *block ^ fixed_key->keys[0];
    block128 state[2], round_key[2];
    cvt192_to_2x128(&state[0], &xored_block);

    for (size_t round = 1; round < RIJNDAEL_ROUNDS<secpar::s192>; ++round)
    {
        cvt192_to_2x128(&round_key[0], &fixed_key->keys[round]);
        rijndael192_rotate_rows_undo_128(&state[0]);
        state[0] = {_mm_aesenc_si128(state[0].data, round_key[0].data)};
        state[1] = {_mm_aesenc_si128(state[1].data, round_key[1].data)};
    }

    rijndael192_rotate_rows_undo_128(&state[0]);
    cvt192_to_2x128(&round_key[0], &fixed_key->keys[RIJNDAEL_ROUNDS<secpar::s192>]);
    state[0] = {_mm_aesenclast_si128(state[0].data, round_key[0].data)};
    state[1] = {_mm_aesenclast_si128(state[1].data, round_key[1].data)};

    memcpy(block, &state[0], sizeof(*block));
}

void rijndael192_round_function(const rijndael192_round_keys* __restrict__ round_keys,
                                block192* __restrict__ block, block192* __restrict__ after_sbox,
                                size_t round)
{
    block128 state[2], state_after_sbox[2], round_key[2];
    cvt192_to_2x128(&state[0], block);
    cvt192_to_2x128(&round_key[0], &round_keys->keys[round]);

    rijndael192_rotate_rows_undo_128(&state[0]);
    state_after_sbox[0] = {_mm_aesenclast_si128(state[0].data, block128::set_zero().data)};
    state_after_sbox[1] = {_mm_aesenclast_si128(state[1].data, block128::set_zero().data)};

    if (round < AES_ROUNDS<secpar::s192>)
    {
        state[0] = {_mm_aesenc_si128(state[0].data, round_key[0].data)};
        state[1] = {_mm_aesenc_si128(state[1].data, round_key[1].data)};
    }
    else
    {
        state[0] = state_after_sbox[0] ^ round_key[0];
        state[1] = state_after_sbox[1] ^ round_key[1];
    }

    memcpy(after_sbox, &state_after_sbox[0], sizeof(*after_sbox));
    memcpy(block, &state[0], sizeof(*block));
}

void rijndael256_round_function(const rijndael256_round_keys* __restrict__ round_keys,
                                block256* __restrict__ block, block256* __restrict__ after_sbox,
                                size_t round)
{
    block128 state[2], state_after_sbox[2], round_key[2];
    memcpy(&state[0], block, sizeof(*block));
    memcpy(&round_key[0], &round_keys->keys[round], sizeof(round_key));

    // Use AES-NI to implement the round function.
    rijndael256_rotate_rows_undo_128(&state[0]);
    state_after_sbox[0] = {_mm_aesenclast_si128(state[0].data, block128::set_zero().data)};
    state_after_sbox[1] = {_mm_aesenclast_si128(state[1].data, block128::set_zero().data)};

    if (round < AES_ROUNDS<secpar::s256>)
    {
        state[0] = {_mm_aesenc_si128(state[0].data, round_key[0].data)};
        state[1] = {_mm_aesenc_si128(state[1].data, round_key[1].data)};
    }
    else
    {
        state[0] = state_after_sbox[0] ^ round_key[0];
        state[1] = state_after_sbox[1] ^ round_key[1];
    }

    memcpy(after_sbox, &state_after_sbox[0], sizeof(*after_sbox));
    memcpy(block, &state[0], sizeof(*block));
}

} // namespace faest
