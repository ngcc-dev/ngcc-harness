#ifndef PRGS_HPP
#define PRGS_HPP

#include "aes.hpp"
#include "hash.hpp"
#include "parameters.hpp"

#include <blake2.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>

namespace sydo
{

template <typename T>
struct prg_trait;

template <typename Derived>
struct prg_base
{
    static constexpr secpar secpar_v = prg_trait<Derived>::secpar_v;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = prg_trait<Derived>::PREFERRED_WIDTH_SHIFT;
    static constexpr size_t PREFERRED_WIDTH = 1 << PREFERRED_WIDTH_SHIFT;
    static constexpr bool HAS_INIT_NO_STORE = false;
    static constexpr bool HAS_INIT_NO_STORE_2X = false;
    static constexpr bool HAS_INIT_NO_STORE_SAME_TWEAK = false;
    static constexpr bool HAS_SAME_TWEAK_COUNTER = false;

    using key_t = block_secpar<secpar_v>; // Key before key schedule
    using expanded_key_t = typename prg_trait<Derived>::expanded_key_t; // Output of key schedule
    using iv_t = typename prg_trait<Derived>::iv_t;
    using block_t = typename prg_trait<Derived>::block_t;
    using tweak_t = typename prg_trait<Derived>::tweak_t;
    using count_t = typename prg_trait<Derived>::count_t;

    // First, given num_keys keys, expand them all into num_keys expanded_keys. Then generate
    // blocks_per_key blocks of output from each, using the IV and num_keys tweaks and initial
    // counters. The first key gives the first blocks_per_key blocks of output, and so on. The
    // second part is equivalent to subsequently running gen(). However, this function does both to
    // allow instruction level parallism between the two parts.
    template <size_t num_keys, count_t blocks_per_key>
    static void init(const key_t* keys, expanded_key_t* expanded_keys,
                     const iv_t& iv, const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        return Derived::template init_impl<num_keys, blocks_per_key>
            (keys, expanded_keys, iv, tweaks, counters, output);
    }

    // Similar, but using the same initial counter for all keys.
    template <size_t num_keys, size_t blocks_per_key>
    static void init(const key_t* keys, expanded_key_t* expanded_keys,
                     const iv_t& iv, const tweak_t* tweaks, count_t counter, block_t* output)
    {
        std::array<count_t, num_keys> counters;
        counters.fill(counter);
        return init<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks, counters.data(), output);
    }

    // Similar, but using the same initial counter and tweak for all keys.
    template <size_t num_keys, size_t blocks_per_key>
    static void init(const key_t* keys, expanded_key_t* expanded_keys,
                     const iv_t& iv, tweak_t tweak, count_t counter, block_t* output)
    {
        std::array<tweak_t, num_keys> tweaks;
        tweaks.fill(tweak);
        return init<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks.data(), counter, output);
    }

    // Similar, but for a single key.
    template <size_t blocks>
    static void init(const key_t* keys, expanded_key_t& expanded_key,
                     const iv_t& iv, size_t counter, block_t* output)
    {
        return init<1, blocks>(keys, &expanded_key, &iv, &counter, output);
    }

    // Given an IV and num_keys expanded keys, tweaks, and initial counters, generate blocks_per_key
    // blocks of PRG output from each key. The first key gives the first blocks_per_key blocks of
    // output, and so on.
    template <size_t num_keys, count_t blocks_per_key>
    static void gen(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                    const count_t* counters, block_t* output)
    {
        return Derived::template gen_impl<num_keys, blocks_per_key>
            (expanded_keys, iv, tweaks, counters, output);
    }

    // Similar, but using the same initial counter for all keys.
    template <size_t num_keys, size_t blocks_per_key>
    static void gen(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                    count_t counter, block_t* output)
    {
        std::array<count_t, num_keys> counters;
        counters.fill(counter);
        return gen<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters.data(), output);
    }

    // Similar, but using the same initial counter and tweak for all keys.
    template <size_t num_keys, size_t blocks_per_key>
    static void gen(const expanded_key_t* expanded_keys, const iv_t& iv, tweak_t tweak,
                    count_t counter, block_t* output)
    {
        std::array<tweak_t, num_keys> tweaks;
        tweaks.fill(tweak);
        return gen<num_keys, blocks_per_key>(expanded_keys, iv, tweaks.data(), counter, output);
    }

    // Similar, but for a single key.
    template <size_t blocks>
    static void gen(const expanded_key_t& expanded_key, const iv_t& iv, size_t counter,
                    block_t* output)
    {
        return gen<1, blocks>(&expanded_key, &iv, &counter, output);
    }
};

/* Example implementation of prg_base:

struct prg;

struct prg_trait<prg>
{
    using expanded_key_t = aes_round_keys<secpar::s256>; // Output of key schedule
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s256;

    // log2(Preferred # of blocks to generate at once.)
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 5;
};

struct prg : public prg_base<prg>
{
    // Explicit using declaration so that you can use the types inside init() and gen(). (Otherwise
    // the compiler cannot tell in general whether they are types or values.)
    typedef prg_base<prg> base;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters, block_t* output);

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                         const count_t* counters, block_t* output);
};
 */

struct blake2b_512_prg;
struct blake2b_512_scalar_prg;
struct blake2xb_512_prg;
struct blake2s_512_bc_ref_prg;
struct blake2s_512_bc_prg;

struct blake2b_512_expanded_key
{
    uint64_t h[8];
#if defined(__AVX2__)
    // Valid for the first entry of each 4-key group produced by init_keys_avx2().
    alignas(32) __m256i h4[8];
#endif
};

template <>
struct prg_trait<blake2b_512_prg>
{
    using expanded_key_t = blake2b_512_expanded_key;
    using iv_t = block512;
    using block_t = block512;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s512;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
};

struct blake2b_512_prg : public prg_base<blake2b_512_prg>
{
    typedef prg_base<blake2b_512_prg> base;
    static constexpr bool HAS_INIT_NO_STORE = true;
    static constexpr bool HAS_INIT_NO_STORE_SAME_TWEAK = true;
    static constexpr bool HAS_SAME_TWEAK_COUNTER = true;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    static block_t make_input_block(const iv_t& iv, tweak_t tweak, count_t counter)
    {
        return iv.add32(block_t::set_low_high32(counter, tweak));
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
#if defined(__AVX2__)
        init_keys_avx2<num_keys>(keys, expanded_keys);
#else
        init_keys_scalar<num_keys>(keys, expanded_keys);
#endif
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
#if defined(__AVX2__)
        gen_avx2<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
#else
        gen_scalar<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
#endif
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store(const key_t* keys, const iv_t& iv, const tweak_t* tweaks,
                              const count_t* counters, block_t* output)
    {
#if defined(__AVX2__)
        if constexpr (blocks_per_key == 2 && (num_keys % 4) == 0)
        {
            init_no_store_blocks2_avx2<num_keys>(keys, iv, tweaks, counters, output);
            return;
        }
#endif
        expanded_key_t expanded_keys[num_keys];
        init_impl<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store_same_tweak(const key_t* keys, const iv_t& iv, tweak_t tweak,
                                         count_t counter, block_t* output)
    {
#if defined(__AVX2__)
        if constexpr (blocks_per_key == 2 && (num_keys % 4) == 0)
        {
            init_no_store_blocks2_same_tweak_avx2<num_keys>(keys, iv, tweak, counter, output);
            return;
        }
#endif
        std::array<tweak_t, num_keys> tweaks;
        std::array<count_t, num_keys> counters;
        tweaks.fill(tweak);
        counters.fill(counter);
        init_no_store<num_keys, blocks_per_key>(keys, iv, tweaks.data(), counters.data(), output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_same_tweak_counter(const key_t* keys, expanded_key_t* expanded_keys,
                                        const iv_t& iv, tweak_t tweak, count_t counter,
                                        block_t* output)
    {
#if defined(__AVX2__)
        init_keys_avx2<num_keys>(keys, expanded_keys);
        gen_same_tweak_counter_avx2<num_keys, blocks_per_key>(
            expanded_keys, iv, tweak, counter, output);
#else
        std::array<tweak_t, num_keys> tweaks;
        std::array<count_t, num_keys> counters;
        tweaks.fill(tweak);
        counters.fill(counter);
        init_impl<num_keys, blocks_per_key>(
            keys, expanded_keys, iv, tweaks.data(), counters.data(), output);
#endif
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_same_tweak_counter(const expanded_key_t* expanded_keys, const iv_t& iv,
                                       tweak_t tweak, count_t counter, block_t* output)
    {
#if defined(__AVX2__)
        gen_same_tweak_counter_avx2<num_keys, blocks_per_key>(
            expanded_keys, iv, tweak, counter, output);
#else
        std::array<tweak_t, num_keys> tweaks;
        std::array<count_t, num_keys> counters;
        tweaks.fill(tweak);
        counters.fill(counter);
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks.data(), counters.data(), output);
#endif
    }

    friend struct blake2xb_512_prg;

private:
    template <size_t num_keys, count_t blocks_per_key>
    static void gen_scalar(const expanded_key_t* expanded_keys, const iv_t& iv,
                           const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        static_assert(sizeof(key_t) == BLAKE2B_KEYBYTES);
        static_assert(sizeof(block_t) == BLAKE2B_OUTBYTES);
        for (size_t i = 0; i < num_keys; ++i)
        {
            uint8_t* out = reinterpret_cast<uint8_t*>(&output[i * blocks_per_key]);
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
            {
                block_t input = make_input_block(iv, tweaks[i], cnt);
                blake2b_state state;
                init_state_from_expanded_key(expanded_keys[i], state);
                if (blake2b_update(&state, &input, sizeof(input)) != 0)
                    std::abort();
                if (blake2b_final(&state, out + j * sizeof(block_t), sizeof(block_t)) != 0)
                    std::abort();
            }
        }
    }

#if defined(__AVX2__)
    static constexpr uint64_t IV0 = 0x6a09e667f3bcc908ULL;
    static constexpr uint64_t IV1 = 0xbb67ae8584caa73bULL;
    static constexpr uint64_t IV2 = 0x3c6ef372fe94f82bULL;
    static constexpr uint64_t IV3 = 0xa54ff53a5f1d36f1ULL;
    static constexpr uint64_t IV4 = 0x510e527fade682d1ULL;
    static constexpr uint64_t IV5 = 0x9b05688c2b3e6c1fULL;
    static constexpr uint64_t IV6 = 0x1f83d9abfb41bd6bULL;
    static constexpr uint64_t IV7 = 0x5be0cd19137e2179ULL;

    static constexpr uint8_t SIGMA[12][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
        {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
        {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
        {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
        {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    };

    static ALWAYS_INLINE __m256i set4(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3)
    {
        return _mm256_set_epi64x((long long)x3, (long long)x2, (long long)x1, (long long)x0);
    }

    static ALWAYS_INLINE __m256i rotr32(__m256i x)
    {
        return _mm256_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1));
    }

    static ALWAYS_INLINE __m256i rotr24(__m256i x)
    {
        const __m256i r = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10,
                                           3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);
        return _mm256_shuffle_epi8(x, r);
    }

    static ALWAYS_INLINE __m256i rotr16(__m256i x)
    {
        const __m256i r = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9,
                                           2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
        return _mm256_shuffle_epi8(x, r);
    }

    static ALWAYS_INLINE __m256i rotr63(__m256i x)
    {
        return _mm256_or_si256(_mm256_srli_epi64(x, 63), _mm256_add_epi64(x, x));
    }

    static ALWAYS_INLINE void g(__m256i& a, __m256i& b, __m256i& c, __m256i& d,
                                const __m256i& x, const __m256i& y)
    {
        a = _mm256_add_epi64(_mm256_add_epi64(a, b), x);
        d = rotr32(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr24(_mm256_xor_si256(b, c));
        a = _mm256_add_epi64(_mm256_add_epi64(a, b), y);
        d = rotr16(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr63(_mm256_xor_si256(b, c));
    }

    template <size_t X, size_t Y, size_t VALID_WORDS>
    static ALWAYS_INLINE void g_msg(__m256i& a, __m256i& b, __m256i& c, __m256i& d,
                                    const __m256i m[16])
    {
        if constexpr (X < VALID_WORDS)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m[X]);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr32(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr24(_mm256_xor_si256(b, c));
        if constexpr (Y < VALID_WORDS)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m[Y]);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr16(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr63(_mm256_xor_si256(b, c));
    }

    template <size_t R, size_t VALID_WORDS>
    static ALWAYS_INLINE void round_vars(__m256i& v0, __m256i& v1, __m256i& v2, __m256i& v3,
                                         __m256i& v4, __m256i& v5, __m256i& v6, __m256i& v7,
                                         __m256i& v8, __m256i& v9, __m256i& v10, __m256i& v11,
                                         __m256i& v12, __m256i& v13, __m256i& v14,
                                         __m256i& v15, const __m256i m[16])
    {
        g_msg<SIGMA[R][0], SIGMA[R][1], VALID_WORDS>(v0, v4, v8, v12, m);
        g_msg<SIGMA[R][2], SIGMA[R][3], VALID_WORDS>(v1, v5, v9, v13, m);
        g_msg<SIGMA[R][4], SIGMA[R][5], VALID_WORDS>(v2, v6, v10, v14, m);
        g_msg<SIGMA[R][6], SIGMA[R][7], VALID_WORDS>(v3, v7, v11, v15, m);
        g_msg<SIGMA[R][8], SIGMA[R][9], VALID_WORDS>(v0, v5, v10, v15, m);
        g_msg<SIGMA[R][10], SIGMA[R][11], VALID_WORDS>(v1, v6, v11, v12, m);
        g_msg<SIGMA[R][12], SIGMA[R][13], VALID_WORDS>(v2, v7, v8, v13, m);
        g_msg<SIGMA[R][14], SIGMA[R][15], VALID_WORDS>(v3, v4, v9, v14, m);
    }

    template <size_t X, size_t Y>
    static ALWAYS_INLINE void g_msg4(__m256i& a, __m256i& b, __m256i& c, __m256i& d,
                                     const __m256i& m0, const __m256i& m1,
                                     const __m256i& m2, const __m256i& m3)
    {
        if constexpr (X == 0)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m0);
        else if constexpr (X == 1)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m1);
        else if constexpr (X == 2)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m2);
        else if constexpr (X == 3)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m3);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr32(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr24(_mm256_xor_si256(b, c));
        if constexpr (Y == 0)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m0);
        else if constexpr (Y == 1)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m1);
        else if constexpr (Y == 2)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m2);
        else if constexpr (Y == 3)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m3);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr16(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr63(_mm256_xor_si256(b, c));
    }

    template <size_t R>
    static ALWAYS_INLINE void round_vars4(__m256i& v0, __m256i& v1, __m256i& v2, __m256i& v3,
                                          __m256i& v4, __m256i& v5, __m256i& v6, __m256i& v7,
                                          __m256i& v8, __m256i& v9, __m256i& v10,
                                          __m256i& v11, __m256i& v12, __m256i& v13,
                                          __m256i& v14, __m256i& v15, const __m256i& m0,
                                          const __m256i& m1, const __m256i& m2,
                                          const __m256i& m3)
    {
        g_msg4<SIGMA[R][0], SIGMA[R][1]>(v0, v4, v8, v12, m0, m1, m2, m3);
        g_msg4<SIGMA[R][2], SIGMA[R][3]>(v1, v5, v9, v13, m0, m1, m2, m3);
        g_msg4<SIGMA[R][4], SIGMA[R][5]>(v2, v6, v10, v14, m0, m1, m2, m3);
        g_msg4<SIGMA[R][6], SIGMA[R][7]>(v3, v7, v11, v15, m0, m1, m2, m3);
        g_msg4<SIGMA[R][8], SIGMA[R][9]>(v0, v5, v10, v15, m0, m1, m2, m3);
        g_msg4<SIGMA[R][10], SIGMA[R][11]>(v1, v6, v11, v12, m0, m1, m2, m3);
        g_msg4<SIGMA[R][12], SIGMA[R][13]>(v2, v7, v8, v13, m0, m1, m2, m3);
        g_msg4<SIGMA[R][14], SIGMA[R][15]>(v3, v4, v9, v14, m0, m1, m2, m3);
    }

    template <size_t X, size_t Y>
    static ALWAYS_INLINE void g_msg8(__m256i& a, __m256i& b, __m256i& c, __m256i& d,
                                     const __m256i& m0, const __m256i& m1,
                                     const __m256i& m2, const __m256i& m3,
                                     const __m256i& m4, const __m256i& m5,
                                     const __m256i& m6, const __m256i& m7)
    {
        if constexpr (X == 0)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m0);
        else if constexpr (X == 1)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m1);
        else if constexpr (X == 2)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m2);
        else if constexpr (X == 3)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m3);
        else if constexpr (X == 4)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m4);
        else if constexpr (X == 5)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m5);
        else if constexpr (X == 6)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m6);
        else if constexpr (X == 7)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m7);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr32(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr24(_mm256_xor_si256(b, c));
        if constexpr (Y == 0)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m0);
        else if constexpr (Y == 1)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m1);
        else if constexpr (Y == 2)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m2);
        else if constexpr (Y == 3)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m3);
        else if constexpr (Y == 4)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m4);
        else if constexpr (Y == 5)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m5);
        else if constexpr (Y == 6)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m6);
        else if constexpr (Y == 7)
            a = _mm256_add_epi64(_mm256_add_epi64(a, b), m7);
        else
            a = _mm256_add_epi64(a, b);
        d = rotr16(_mm256_xor_si256(d, a));
        c = _mm256_add_epi64(c, d);
        b = rotr63(_mm256_xor_si256(b, c));
    }

    template <size_t R>
    static ALWAYS_INLINE void round_vars8(__m256i& v0, __m256i& v1, __m256i& v2, __m256i& v3,
                                          __m256i& v4, __m256i& v5, __m256i& v6, __m256i& v7,
                                          __m256i& v8, __m256i& v9, __m256i& v10,
                                          __m256i& v11, __m256i& v12, __m256i& v13,
                                          __m256i& v14, __m256i& v15, const __m256i& m0,
                                          const __m256i& m1, const __m256i& m2,
                                          const __m256i& m3, const __m256i& m4,
                                          const __m256i& m5, const __m256i& m6,
                                          const __m256i& m7)
    {
        g_msg8<SIGMA[R][0], SIGMA[R][1]>(v0, v4, v8, v12, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][2], SIGMA[R][3]>(v1, v5, v9, v13, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][4], SIGMA[R][5]>(v2, v6, v10, v14, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][6], SIGMA[R][7]>(v3, v7, v11, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][8], SIGMA[R][9]>(v0, v5, v10, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][10], SIGMA[R][11]>(v1, v6, v11, v12, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][12], SIGMA[R][13]>(v2, v7, v8, v13, m0, m1, m2, m3, m4, m5, m6, m7);
        g_msg8<SIGMA[R][14], SIGMA[R][15]>(v3, v4, v9, v14, m0, m1, m2, m3, m4, m5, m6, m7);
    }

    template <size_t VALID_WORDS>
    static ALWAYS_INLINE void compress4_vec(const __m256i* __restrict__ h,
                                            const __m256i* __restrict__ m,
                                            uint64_t t0, uint64_t f0,
                                            __m256i* __restrict__ out)
    {
        __m256i v0 = h[0];
        __m256i v1 = h[1];
        __m256i v2 = h[2];
        __m256i v3 = h[3];
        __m256i v4 = h[4];
        __m256i v5 = h[5];
        __m256i v6 = h[6];
        __m256i v7 = h[7];
        __m256i v8 = _mm256_set1_epi64x((long long)IV0);
        __m256i v9 = _mm256_set1_epi64x((long long)IV1);
        __m256i v10 = _mm256_set1_epi64x((long long)IV2);
        __m256i v11 = _mm256_set1_epi64x((long long)IV3);
        __m256i v12 = _mm256_set1_epi64x((long long)(IV4 ^ t0));
        __m256i v13 = _mm256_set1_epi64x((long long)IV5);
        __m256i v14 = _mm256_set1_epi64x((long long)(IV6 ^ f0));
        __m256i v15 = _mm256_set1_epi64x((long long)IV7);

        round_vars<0, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<1, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<2, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<3, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<4, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<5, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<6, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<7, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<8, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<9, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                   v14, v15, m);
        round_vars<10, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                    v14, v15, m);
        round_vars<11, VALID_WORDS>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                                    v14, v15, m);

        out[0] = _mm256_xor_si256(h[0], _mm256_xor_si256(v0, v8));
        out[1] = _mm256_xor_si256(h[1], _mm256_xor_si256(v1, v9));
        out[2] = _mm256_xor_si256(h[2], _mm256_xor_si256(v2, v10));
        out[3] = _mm256_xor_si256(h[3], _mm256_xor_si256(v3, v11));
        out[4] = _mm256_xor_si256(h[4], _mm256_xor_si256(v4, v12));
        out[5] = _mm256_xor_si256(h[5], _mm256_xor_si256(v5, v13));
        out[6] = _mm256_xor_si256(h[6], _mm256_xor_si256(v6, v14));
        out[7] = _mm256_xor_si256(h[7], _mm256_xor_si256(v7, v15));
    }

    static ALWAYS_INLINE void compress4_vec4(const __m256i* __restrict__ h,
                                             const __m256i& m0, const __m256i& m1,
                                             const __m256i& m2, const __m256i& m3,
                                             uint64_t t0, uint64_t f0,
                                             __m256i* __restrict__ out)
    {
        __m256i v0 = h[0];
        __m256i v1 = h[1];
        __m256i v2 = h[2];
        __m256i v3 = h[3];
        __m256i v4 = h[4];
        __m256i v5 = h[5];
        __m256i v6 = h[6];
        __m256i v7 = h[7];
        __m256i v8 = _mm256_set1_epi64x((long long)IV0);
        __m256i v9 = _mm256_set1_epi64x((long long)IV1);
        __m256i v10 = _mm256_set1_epi64x((long long)IV2);
        __m256i v11 = _mm256_set1_epi64x((long long)IV3);
        __m256i v12 = _mm256_set1_epi64x((long long)(IV4 ^ t0));
        __m256i v13 = _mm256_set1_epi64x((long long)IV5);
        __m256i v14 = _mm256_set1_epi64x((long long)(IV6 ^ f0));
        __m256i v15 = _mm256_set1_epi64x((long long)IV7);

        round_vars4<0>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<1>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<2>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<3>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<4>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<5>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<6>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<7>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<8>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<9>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3);
        round_vars4<10>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                        v14, v15, m0, m1, m2, m3);
        round_vars4<11>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                        v14, v15, m0, m1, m2, m3);

        out[0] = _mm256_xor_si256(h[0], _mm256_xor_si256(v0, v8));
        out[1] = _mm256_xor_si256(h[1], _mm256_xor_si256(v1, v9));
        out[2] = _mm256_xor_si256(h[2], _mm256_xor_si256(v2, v10));
        out[3] = _mm256_xor_si256(h[3], _mm256_xor_si256(v3, v11));
        out[4] = _mm256_xor_si256(h[4], _mm256_xor_si256(v4, v12));
        out[5] = _mm256_xor_si256(h[5], _mm256_xor_si256(v5, v13));
        out[6] = _mm256_xor_si256(h[6], _mm256_xor_si256(v6, v14));
        out[7] = _mm256_xor_si256(h[7], _mm256_xor_si256(v7, v15));
    }

    static ALWAYS_INLINE void compress4_vec8(const __m256i* __restrict__ h,
                                             const __m256i& m0, const __m256i& m1,
                                             const __m256i& m2, const __m256i& m3,
                                             const __m256i& m4, const __m256i& m5,
                                             const __m256i& m6, const __m256i& m7,
                                             uint64_t t0, uint64_t f0,
                                             __m256i* __restrict__ out)
    {
        __m256i v0 = h[0];
        __m256i v1 = h[1];
        __m256i v2 = h[2];
        __m256i v3 = h[3];
        __m256i v4 = h[4];
        __m256i v5 = h[5];
        __m256i v6 = h[6];
        __m256i v7 = h[7];
        __m256i v8 = _mm256_set1_epi64x((long long)IV0);
        __m256i v9 = _mm256_set1_epi64x((long long)IV1);
        __m256i v10 = _mm256_set1_epi64x((long long)IV2);
        __m256i v11 = _mm256_set1_epi64x((long long)IV3);
        __m256i v12 = _mm256_set1_epi64x((long long)(IV4 ^ t0));
        __m256i v13 = _mm256_set1_epi64x((long long)IV5);
        __m256i v14 = _mm256_set1_epi64x((long long)(IV6 ^ f0));
        __m256i v15 = _mm256_set1_epi64x((long long)IV7);

        round_vars8<0>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<1>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<2>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<3>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<4>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<5>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<6>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<7>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<8>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<9>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                       v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<10>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                        v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);
        round_vars8<11>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                        v14, v15, m0, m1, m2, m3, m4, m5, m6, m7);

        out[0] = _mm256_xor_si256(h[0], _mm256_xor_si256(v0, v8));
        out[1] = _mm256_xor_si256(h[1], _mm256_xor_si256(v1, v9));
        out[2] = _mm256_xor_si256(h[2], _mm256_xor_si256(v2, v10));
        out[3] = _mm256_xor_si256(h[3], _mm256_xor_si256(v3, v11));
        out[4] = _mm256_xor_si256(h[4], _mm256_xor_si256(v4, v12));
        out[5] = _mm256_xor_si256(h[5], _mm256_xor_si256(v5, v13));
        out[6] = _mm256_xor_si256(h[6], _mm256_xor_si256(v6, v14));
        out[7] = _mm256_xor_si256(h[7], _mm256_xor_si256(v7, v15));
    }

    template <size_t VALID_WORDS>
    static ALWAYS_INLINE void compress4(const __m256i h[8], const __m256i m[16],
                                        uint64_t t0, uint64_t f0, uint64_t lanes[8][4])
    {
        __m256i out[8];
        compress4_vec<VALID_WORDS>(h, m, t0, f0, out);
        for (size_t r = 0; r < 8; ++r)
            _mm256_storeu_si256((__m256i*)lanes[r], out[r]);
    }

    static ALWAYS_INLINE void transpose4x4(__m256i a, __m256i b, __m256i c, __m256i d,
                                           __m256i out[4])
    {
        const __m256i ab_lo = _mm256_unpacklo_epi64(a, b);
        const __m256i ab_hi = _mm256_unpackhi_epi64(a, b);
        const __m256i cd_lo = _mm256_unpacklo_epi64(c, d);
        const __m256i cd_hi = _mm256_unpackhi_epi64(c, d);
        out[0] = _mm256_permute2x128_si256(ab_lo, cd_lo, 0x20);
        out[1] = _mm256_permute2x128_si256(ab_hi, cd_hi, 0x20);
        out[2] = _mm256_permute2x128_si256(ab_lo, cd_lo, 0x31);
        out[3] = _mm256_permute2x128_si256(ab_hi, cd_hi, 0x31);
    }

    static ALWAYS_INLINE void store_block(block_t& dst, __m256i lo, __m256i hi)
    {
        _mm256_storeu_si256((__m256i*)&dst, lo);
        _mm256_storeu_si256((__m256i*)((uint8_t*)&dst + 32), hi);
    }

    static ALWAYS_INLINE void store4_blocks(block_t* __restrict__ output, size_t idx0, size_t idx1,
                                            size_t idx2, size_t idx3, const __m256i out[8])
    {
        __m256i lo[4];
        __m256i hi[4];
        transpose4x4(out[0], out[1], out[2], out[3], lo);
        transpose4x4(out[4], out[5], out[6], out[7], hi);

        store_block(output[idx0], lo[0], hi[0]);
        store_block(output[idx1], lo[1], hi[1]);
        store_block(output[idx2], lo[2], hi[2]);
        store_block(output[idx3], lo[3], hi[3]);
    }

    static ALWAYS_INLINE void store4_expanded_keys(expanded_key_t* __restrict__ expanded_keys,
                                                   const __m256i* __restrict__ out)
    {
        static_assert(sizeof(expanded_keys[0].h) == 64);
        for (size_t r = 0; r < 8; ++r)
            expanded_keys[0].h4[r] = out[r];

        __m256i lo[4];
        __m256i hi[4];
        transpose4x4(out[0], out[1], out[2], out[3], lo);
        transpose4x4(out[4], out[5], out[6], out[7], hi);

        _mm256_storeu_si256((__m256i*)expanded_keys[0].h, lo[0]);
        _mm256_storeu_si256((__m256i*)&expanded_keys[0].h[4], hi[0]);
        _mm256_storeu_si256((__m256i*)expanded_keys[1].h, lo[1]);
        _mm256_storeu_si256((__m256i*)&expanded_keys[1].h[4], hi[1]);
        _mm256_storeu_si256((__m256i*)expanded_keys[2].h, lo[2]);
        _mm256_storeu_si256((__m256i*)&expanded_keys[2].h[4], hi[2]);
        _mm256_storeu_si256((__m256i*)expanded_keys[3].h, lo[3]);
        _mm256_storeu_si256((__m256i*)&expanded_keys[3].h[4], hi[3]);
    }

    template <size_t num_keys>
    static void init_keys_avx2(const key_t* keys, expanded_key_t* expanded_keys)
    {
        size_t i = 0;
        for (; i + 4 <= num_keys; i += 4)
            precompress_key_block4(&keys[i], &expanded_keys[i]);
        if constexpr ((num_keys % 4) != 0)
        {
            for (; i < num_keys; ++i)
                init_key_scalar(keys[i], expanded_keys[i]);
        }
    }

    static void precompress_key_block4(const key_t* __restrict__ keys,
                                       expanded_key_t* __restrict__ expanded_keys)
    {
        __m256i out[8];
        precompress_key_block4_vec(keys, out);
        store4_expanded_keys(expanded_keys, out);
    }

    static void precompress_key_block4_vec(const key_t* __restrict__ keys,
                                           __m256i* __restrict__ out)
    {
        const uint64_t* key0 = reinterpret_cast<const uint64_t*>(&keys[0]);
        const uint64_t* key1 = reinterpret_cast<const uint64_t*>(&keys[1]);
        const uint64_t* key2 = reinterpret_cast<const uint64_t*>(&keys[2]);
        const uint64_t* key3 = reinterpret_cast<const uint64_t*>(&keys[3]);

        __m256i h[8] = {
            _mm256_set1_epi64x((long long)(IV0 ^ 0x01014040ULL)),
            _mm256_set1_epi64x((long long)IV1),
            _mm256_set1_epi64x((long long)IV2),
            _mm256_set1_epi64x((long long)IV3),
            _mm256_set1_epi64x((long long)IV4),
            _mm256_set1_epi64x((long long)IV5),
            _mm256_set1_epi64x((long long)IV6),
            _mm256_set1_epi64x((long long)IV7)};
        __m256i m[16];
        for (size_t r = 0; r < 8; ++r)
            m[r] = set4(key0[r], key1[r], key2[r], key3[r]);
        for (size_t r = 8; r < 16; ++r)
            m[r] = _mm256_setzero_si256();

        compress4_vec<8>(h, m, 128, 0, out);
    }

    template <size_t num_keys>
    static ALWAYS_INLINE void init_no_store_blocks2_avx2(const key_t* __restrict__ keys,
                                                         const iv_t& __restrict__ iv,
                                                         const tweak_t* __restrict__ tweaks,
                                                         const count_t* __restrict__ counters,
                                                         block_t* __restrict__ output)
    {
        for (size_t i = 0; i < num_keys; i += 4)
        {
            __m256i h[8];
            precompress_key_block4_vec(&keys[i], h);
            compress_final4_blocks2_no_store(h, iv, &tweaks[i], &counters[i], output, i, 0);
            compress_final4_blocks2_no_store(h, iv, &tweaks[i], &counters[i], output, i, 1);
        }
    }

    template <size_t num_keys>
    static ALWAYS_INLINE void init_no_store_blocks2_same_tweak_avx2(
        const key_t* __restrict__ keys, const iv_t& __restrict__ iv, tweak_t tweak,
        count_t counter, block_t* __restrict__ output)
    {
        for (size_t i = 0; i < num_keys; i += 4)
        {
            __m256i h[8];
            precompress_key_block4_vec(&keys[i], h);
            compress_final4_blocks2_same_tweak_no_store(h, iv, tweak, counter, output, i, 0);
            compress_final4_blocks2_same_tweak_no_store(h, iv, tweak, counter, output, i, 1);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_avx2(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        static_assert(sizeof(key_t) == BLAKE2B_KEYBYTES);
        static_assert(sizeof(block_t) == BLAKE2B_OUTBYTES);

        constexpr size_t total_blocks = num_keys * blocks_per_key;
        size_t k = 0;
        if constexpr (num_keys == 1)
        {
            gen_one_key_avx2<blocks_per_key>(expanded_keys[0], iv, tweaks[0], counters[0], output);
            return;
        }
        else if constexpr (blocks_per_key == 1)
        {
            for (; k + 4 <= total_blocks; k += 4)
                compress_final4_blocks1(expanded_keys, iv, tweaks, counters, output, k);
        }
        else
        {
            for (; k + 4 <= total_blocks; k += 4)
                compress_final4<num_keys, blocks_per_key>(
                    expanded_keys, iv, tweaks, counters, output, k);
        }

        if constexpr ((total_blocks % 4) != 0)
        {
            for (; k < total_blocks; ++k)
            {
                const size_t i = k / blocks_per_key;
                const count_t cnt = counters[i] + (count_t)(k % blocks_per_key);
                gen_one_scalar(expanded_keys[i], iv, tweaks[i], cnt, output[k]);
            }
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_same_tweak_counter_avx2(const expanded_key_t* expanded_keys, const iv_t& iv,
                                            tweak_t tweak, count_t counter, block_t* output)
    {
        if constexpr (blocks_per_key == 1 && (num_keys % 4) == 0)
        {
            for (size_t i = 0; i < num_keys; i += 4)
                compress_final4_blocks1_same_tweak_counter(
                    &expanded_keys[i], iv, tweak, counter, output, i);
        }
        else
        {
            std::array<tweak_t, num_keys> tweaks;
            std::array<count_t, num_keys> counters;
            tweaks.fill(tweak);
            counters.fill(counter);
            gen_avx2<num_keys, blocks_per_key>(
                expanded_keys, iv, tweaks.data(), counters.data(), output);
        }
    }

    static void gen_one_scalar(const expanded_key_t& expanded_key, const iv_t& iv,
                               tweak_t tweak, count_t counter, block_t& output)
    {
        block_t input = make_input_block(iv, tweak, counter);
        blake2b_state state;
        init_state_from_expanded_key(expanded_key, state);
        if (blake2b_update(&state, &input, sizeof(input)) != 0)
            std::abort();
        if (blake2b_final(&state, &output, sizeof(output)) != 0)
            std::abort();
    }

    static ALWAYS_INLINE void compress_final_input4(const __m256i* __restrict__ h,
                                                    const block_t& in0, const block_t& in1,
                                                    const block_t& in2, const block_t& in3,
                                                    __m256i* __restrict__ out)
    {
        const uint64_t* w0 = reinterpret_cast<const uint64_t*>(&in0);
        const uint64_t* w1 = reinterpret_cast<const uint64_t*>(&in1);
        const uint64_t* w2 = reinterpret_cast<const uint64_t*>(&in2);
        const uint64_t* w3 = reinterpret_cast<const uint64_t*>(&in3);
        compress4_vec8(h, set4(w0[0], w1[0], w2[0], w3[0]),
                        set4(w0[1], w1[1], w2[1], w3[1]),
                        set4(w0[2], w1[2], w2[2], w3[2]),
                        set4(w0[3], w1[3], w2[3], w3[3]),
                        set4(w0[4], w1[4], w2[4], w3[4]),
                        set4(w0[5], w1[5], w2[5], w3[5]),
                        set4(w0[6], w1[6], w2[6], w3[6]),
                        set4(w0[7], w1[7], w2[7], w3[7]), 192, UINT64_MAX, out);
    }

    template <count_t blocks_per_key>
    static void gen_one_key_avx2(const expanded_key_t& __restrict__ expanded_key,
                                 const iv_t& __restrict__ iv, tweak_t tweak, count_t counter,
                                 block_t* __restrict__ output)
    {
        __m256i h[8];
        for (size_t r = 0; r < 8; ++r)
            h[r] = _mm256_set1_epi64x((long long)expanded_key.h[r]);

        count_t j = 0;
        for (; j + 4 <= blocks_per_key; j += 4)
        {
            const block_t in0 = make_input_block(iv, tweak, counter + j + 0);
            const block_t in1 = make_input_block(iv, tweak, counter + j + 1);
            const block_t in2 = make_input_block(iv, tweak, counter + j + 2);
            const block_t in3 = make_input_block(iv, tweak, counter + j + 3);
            __m256i out[8];
            compress_final_input4(h, in0, in1, in2, in3, out);
            store4_blocks(output, j + 0, j + 1, j + 2, j + 3, out);
        }

        if constexpr ((blocks_per_key % 4) != 0)
        {
            for (; j < blocks_per_key; ++j)
                gen_one_scalar(expanded_key, iv, tweak, counter + j, output[j]);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void compress_final4(const expanded_key_t* __restrict__ expanded_keys,
                                const iv_t& __restrict__ iv,
                                const tweak_t* __restrict__ tweaks,
                                const count_t* __restrict__ counters,
                                block_t* __restrict__ output, size_t first_block)
    {
        const size_t key0 = (first_block + 0) / blocks_per_key;
        const size_t key1 = (first_block + 1) / blocks_per_key;
        const size_t key2 = (first_block + 2) / blocks_per_key;
        const size_t key3 = (first_block + 3) / blocks_per_key;
        const count_t ctr0 = counters[key0] + (count_t)((first_block + 0) % blocks_per_key);
        const count_t ctr1 = counters[key1] + (count_t)((first_block + 1) % blocks_per_key);
        const count_t ctr2 = counters[key2] + (count_t)((first_block + 2) % blocks_per_key);
        const count_t ctr3 = counters[key3] + (count_t)((first_block + 3) % blocks_per_key);

        const expanded_key_t& s0 = expanded_keys[key0];
        const expanded_key_t& s1 = expanded_keys[key1];
        const expanded_key_t& s2 = expanded_keys[key2];
        const expanded_key_t& s3 = expanded_keys[key3];

        __m256i h[8];
        for (size_t r = 0; r < 8; ++r)
            h[r] = set4(s0.h[r], s1.h[r], s2.h[r], s3.h[r]);

        const block_t in0 = make_input_block(iv, tweaks[key0], ctr0);
        const block_t in1 = make_input_block(iv, tweaks[key1], ctr1);
        const block_t in2 = make_input_block(iv, tweaks[key2], ctr2);
        const block_t in3 = make_input_block(iv, tweaks[key3], ctr3);

        __m256i out[8];
        compress_final_input4(h, in0, in1, in2, in3, out);
        store4_blocks(output, first_block + 0, first_block + 1, first_block + 2, first_block + 3,
                      out);
    }

    static void compress_final4_blocks1(const expanded_key_t* __restrict__ expanded_keys,
                                        const iv_t& __restrict__ iv,
                                        const tweak_t* __restrict__ tweaks,
                                        const count_t* __restrict__ counters,
                                        block_t* __restrict__ output, size_t first_key)
    {
        const block_t in0 = make_input_block(iv, tweaks[first_key + 0], counters[first_key + 0]);
        const block_t in1 = make_input_block(iv, tweaks[first_key + 1], counters[first_key + 1]);
        const block_t in2 = make_input_block(iv, tweaks[first_key + 2], counters[first_key + 2]);
        const block_t in3 = make_input_block(iv, tweaks[first_key + 3], counters[first_key + 3]);

        __m256i out[8];
        compress_final_input4(expanded_keys[first_key].h4, in0, in1, in2, in3, out);
        store4_blocks(output, first_key + 0, first_key + 1, first_key + 2, first_key + 3, out);
    }

    static void compress_final4_blocks2_no_store(const __m256i* __restrict__ h,
                                                 const iv_t& __restrict__ iv,
                                                 const tweak_t* __restrict__ tweaks,
                                                 const count_t* __restrict__ counters,
                                                 block_t* __restrict__ output, size_t first_key,
                                                 count_t counter_offset)
    {
        const block_t in0 = make_input_block(iv, tweaks[0], counters[0] + counter_offset);
        const block_t in1 = make_input_block(iv, tweaks[1], counters[1] + counter_offset);
        const block_t in2 = make_input_block(iv, tweaks[2], counters[2] + counter_offset);
        const block_t in3 = make_input_block(iv, tweaks[3], counters[3] + counter_offset);

        __m256i out[8];
        compress_final_input4(h, in0, in1, in2, in3, out);
        store4_blocks(output, 2 * (first_key + 0) + counter_offset,
                      2 * (first_key + 1) + counter_offset,
                      2 * (first_key + 2) + counter_offset,
                      2 * (first_key + 3) + counter_offset, out);
    }

    static void compress_final4_blocks1_same_tweak_counter(
        const expanded_key_t* __restrict__ expanded_keys, const iv_t& __restrict__ iv,
        tweak_t tweak, count_t counter, block_t* __restrict__ output, size_t first_key)
    {
        const block_t input = make_input_block(iv, tweak, counter);

        __m256i out[8];
        compress_final_input4(expanded_keys[0].h4, input, input, input, input, out);
        store4_blocks(output, first_key + 0, first_key + 1, first_key + 2, first_key + 3, out);
    }

    static void compress_final4_blocks2_same_tweak_no_store(
        const __m256i* __restrict__ h, const iv_t& __restrict__ iv, tweak_t tweak,
        count_t counter, block_t* __restrict__ output, size_t first_key, count_t counter_offset)
    {
        const block_t input = make_input_block(iv, tweak, counter + counter_offset);

        __m256i out[8];
        compress_final_input4(h, input, input, input, input, out);
        store4_blocks(output, 2 * (first_key + 0) + counter_offset,
                      2 * (first_key + 1) + counter_offset,
                      2 * (first_key + 2) + counter_offset,
                      2 * (first_key + 3) + counter_offset, out);
    }

#endif

    template <size_t num_keys>
    static void init_keys_scalar(const key_t* keys, expanded_key_t* expanded_keys)
    {
        for (size_t i = 0; i < num_keys; ++i)
            init_key_scalar(keys[i], expanded_keys[i]);
    }

    static void init_key_scalar(const key_t& key, expanded_key_t& expanded_key)
    {
        blake2b_state state;
        if (blake2b_init_key(&state, BLAKE2B_OUTBYTES, &key, sizeof(key)) != 0)
            std::abort();
        precompress_key_block(&state);
        memcpy(expanded_key.h, state.h, sizeof(expanded_key.h));
    }

    static void init_state_from_expanded_key(const expanded_key_t& expanded_key, blake2b_state& state)
    {
        memset(&state, 0, sizeof(state));
        memcpy(state.h, expanded_key.h, sizeof(expanded_key.h));
        state.t[0] = 128;
        state.outlen = BLAKE2B_OUTBYTES;
    }

    static void precompress_key_block(blake2b_state* state)
    {
        uint8_t dummy = 0;
        if (blake2b_update(state, &dummy, sizeof(dummy)) != 0)
            std::abort();
        state->buflen = 0;
        std::memset(state->buf, 0, sizeof(state->buf));
    }
};

template <>
struct prg_trait<blake2b_512_scalar_prg>
{
    using expanded_key_t = blake2b_512_expanded_key;
    using iv_t = block512;
    using block_t = block512;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s512;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
};

struct blake2b_512_scalar_prg : public prg_base<blake2b_512_scalar_prg>
{
    typedef prg_base<blake2b_512_scalar_prg> base;
    static constexpr bool HAS_INIT_NO_STORE = true;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    static block_t make_input_block(const iv_t& iv, tweak_t tweak, count_t counter)
    {
        return iv.add32(block_t::set_low_high32(counter, tweak));
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        for (size_t i = 0; i < num_keys; ++i)
            init_key(keys[i], expanded_keys[i]);
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
                gen_one(expanded_keys[i], iv, tweaks[i], cnt, output[i * blocks_per_key + j]);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store(const key_t* keys, const iv_t& iv, const tweak_t* tweaks,
                              const count_t* counters, block_t* output)
    {
        expanded_key_t expanded_keys[num_keys];
        init_impl<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks, counters, output);
    }

private:
    static void init_key(const key_t& key, expanded_key_t& expanded_key)
    {
        blake2b_state state;
        if (blake2b_init_key(&state, BLAKE2B_OUTBYTES, &key, sizeof(key)) != 0)
            std::abort();
        uint8_t dummy = 0;
        if (blake2b_update(&state, &dummy, sizeof(dummy)) != 0)
            std::abort();
        state.buflen = 0;
        std::memset(state.buf, 0, sizeof(state.buf));
        std::memcpy(expanded_key.h, state.h, sizeof(expanded_key.h));
    }

    static void init_state_from_expanded_key(const expanded_key_t& expanded_key, blake2b_state& state)
    {
        std::memset(&state, 0, sizeof(state));
        std::memcpy(state.h, expanded_key.h, sizeof(expanded_key.h));
        state.t[0] = 128;
        state.outlen = BLAKE2B_OUTBYTES;
    }

    static void gen_one(const expanded_key_t& expanded_key, const iv_t& iv,
                        tweak_t tweak, count_t counter, block_t& output)
    {
        const block_t input = make_input_block(iv, tweak, counter);
        blake2b_state state;
        init_state_from_expanded_key(expanded_key, state);
        if (blake2b_update(&state, &input, sizeof(input)) != 0)
            std::abort();
        if (blake2b_final(&state, &output, sizeof(output)) != 0)
            std::abort();
    }
};

struct blake2xb_512_expanded_key
{
    blake2xb_state state;
};

template <>
struct prg_trait<blake2xb_512_prg>
{
    using expanded_key_t = blake2xb_512_expanded_key;
    using iv_t = block512;
    using block_t = block1024;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s512;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
};

struct blake2xb_512_prg : public prg_base<blake2xb_512_prg>
{
    typedef prg_base<blake2xb_512_prg> base;
    static constexpr bool HAS_INIT_NO_STORE_2X = true;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            if (blake2xb_init_key(&expanded_keys[i].state, sizeof(block_t), &keys[i],
                                  sizeof(keys[i])) != 0)
                std::abort();
            precompress_key_block(&expanded_keys[i].state);
        }
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        static_assert(sizeof(key_t) == BLAKE2B_KEYBYTES);
        static_assert(sizeof(block_t) == 2 * BLAKE2B_OUTBYTES);
        for (size_t i = 0; i < num_keys; ++i)
        {
            uint8_t* out = reinterpret_cast<uint8_t*>(&output[i * blocks_per_key]);
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
            {
                blake2b_512_prg::block_t input =
                    blake2b_512_prg::make_input_block(iv, tweaks[i], cnt);

                blake2xb_state state = expanded_keys[i].state;
                if (blake2xb_update(&state, &input, sizeof(input)) != 0)
                    std::abort();
                if (blake2xb_final(&state, out + j * sizeof(block_t), sizeof(block_t)) != 0)
                    std::abort();
            }
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store(const key_t* keys, const iv_t& iv, const tweak_t* tweaks,
                              const count_t* counters, block_t* output)
    {
#if defined(__AVX2__)
        if constexpr (blocks_per_key == 1 && (num_keys % 4) == 0)
        {
            init_no_store_blocks1_avx2<num_keys>(keys, iv, tweaks, counters, output);
            return;
        }
#endif
        expanded_key_t expanded_keys[num_keys];
        init_impl<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks, counters, output);
    }

private:
#if defined(__AVX2__)
    static void precompress_key_block4_vec(const key_t* __restrict__ keys,
                                           __m256i* __restrict__ out)
    {
        using b = blake2b_512_prg;
        const uint64_t* key0 = reinterpret_cast<const uint64_t*>(&keys[0]);
        const uint64_t* key1 = reinterpret_cast<const uint64_t*>(&keys[1]);
        const uint64_t* key2 = reinterpret_cast<const uint64_t*>(&keys[2]);
        const uint64_t* key3 = reinterpret_cast<const uint64_t*>(&keys[3]);

        __m256i h[8] = {
            _mm256_set1_epi64x((long long)(b::IV0 ^ 0x01014040ULL)),
            _mm256_set1_epi64x((long long)(b::IV1 ^ (128ULL << 32))),
            _mm256_set1_epi64x((long long)b::IV2),
            _mm256_set1_epi64x((long long)b::IV3),
            _mm256_set1_epi64x((long long)b::IV4),
            _mm256_set1_epi64x((long long)b::IV5),
            _mm256_set1_epi64x((long long)b::IV6),
            _mm256_set1_epi64x((long long)b::IV7)};
        b::compress4_vec8(h, b::set4(key0[0], key1[0], key2[0], key3[0]),
                          b::set4(key0[1], key1[1], key2[1], key3[1]),
                          b::set4(key0[2], key1[2], key2[2], key3[2]),
                          b::set4(key0[3], key1[3], key2[3], key3[3]),
                          b::set4(key0[4], key1[4], key2[4], key3[4]),
                          b::set4(key0[5], key1[5], key2[5], key3[5]),
                          b::set4(key0[6], key1[6], key2[6], key3[6]),
                          b::set4(key0[7], key1[7], key2[7], key3[7]), 128, 0, out);
    }

    static void compress_root_final4(const __m256i* __restrict__ h,
                                     const iv_t& __restrict__ iv,
                                     const tweak_t* __restrict__ tweaks,
                                     const count_t* __restrict__ counters,
                                     __m256i* __restrict__ root)
    {
        using b = blake2b_512_prg;
        const b::block_t in0 = b::make_input_block(iv, tweaks[0], counters[0]);
        const b::block_t in1 = b::make_input_block(iv, tweaks[1], counters[1]);
        const b::block_t in2 = b::make_input_block(iv, tweaks[2], counters[2]);
        const b::block_t in3 = b::make_input_block(iv, tweaks[3], counters[3]);
        b::compress_final_input4(h, in0, in1, in2, in3, root);
    }

    static void compress_output4(const __m256i* __restrict__ root,
                                 block_t* __restrict__ output, size_t first_key,
                                 uint32_t node_offset)
    {
        using b = blake2b_512_prg;
        __m256i h[8] = {
            _mm256_set1_epi64x((long long)(b::IV0 ^ ((64ULL << 32) | 0x40ULL))),
            _mm256_set1_epi64x((long long)(b::IV1 ^ ((128ULL << 32) | node_offset))),
            _mm256_set1_epi64x((long long)(b::IV2 ^ 0x4000ULL)),
            _mm256_set1_epi64x((long long)b::IV3),
            _mm256_set1_epi64x((long long)b::IV4),
            _mm256_set1_epi64x((long long)b::IV5),
            _mm256_set1_epi64x((long long)b::IV6),
            _mm256_set1_epi64x((long long)b::IV7)};
        __m256i out[8];
        b::compress4_vec8(h, root[0], root[1], root[2], root[3], root[4], root[5],
                          root[6], root[7], 64, UINT64_MAX, out);
        block512* output512 = reinterpret_cast<block512*>(output);
        b::store4_blocks(output512, 2 * (first_key + 0) + node_offset,
                         2 * (first_key + 1) + node_offset,
                         2 * (first_key + 2) + node_offset,
                         2 * (first_key + 3) + node_offset, out);
    }

    template <size_t num_keys>
    static void init_no_store_blocks1_avx2(const key_t* __restrict__ keys,
                                           const iv_t& __restrict__ iv,
                                           const tweak_t* __restrict__ tweaks,
                                           const count_t* __restrict__ counters,
                                           block_t* __restrict__ output)
    {
        for (size_t i = 0; i < num_keys; i += 4)
        {
            __m256i root_key[8];
            __m256i root[8];
            precompress_key_block4_vec(&keys[i], root_key);
            compress_root_final4(root_key, iv, &tweaks[i], &counters[i], root);
            compress_output4(root, output, i, 0);
            compress_output4(root, output, i, 1);
        }
    }
#endif

    static void precompress_key_block(blake2xb_state* state)
    {
        uint8_t dummy = 0;
        if (blake2xb_update(state, &dummy, sizeof(dummy)) != 0)
            std::abort();
        state->S->buflen = 0;
        std::memset(state->S->buf, 0, sizeof(state->S->buf));
    }
};

struct blake2s_512_bc_expanded_key
{
    uint32_t m[16];
#if defined(__AVX2__)
    alignas(32) __m256i m8[16];
#endif
};

template <>
struct prg_trait<blake2s_512_bc_ref_prg>
{
    using expanded_key_t = blake2s_512_bc_expanded_key;
    using iv_t = block512;
    using block_t = block512;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s512;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
};

template <>
struct prg_trait<blake2s_512_bc_prg>
{
    using expanded_key_t = blake2s_512_bc_expanded_key;
    using iv_t = block512;
    using block_t = block512;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s512;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
};

template <typename Derived, bool USE_AVX2>
struct blake2s_512_bc_base : public prg_base<Derived>
{
    typedef prg_base<Derived> base;
    static constexpr bool HAS_INIT_NO_STORE = true;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    static block_t make_input_block(const iv_t& iv, tweak_t tweak, count_t counter)
    {
        return iv.add32(block_t::set_low_high32(counter, tweak));
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        init_keys<num_keys>(keys, expanded_keys);
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
#if defined(__AVX2__)
        if constexpr (USE_AVX2)
            gen_avx2<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
        else
#endif
            gen_scalar<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store(const key_t* keys, const iv_t& iv, const tweak_t* tweaks,
                              const count_t* counters, block_t* output)
    {
#if defined(__AVX2__)
        if constexpr (USE_AVX2 && blocks_per_key == 2 && (num_keys % 4) == 0)
        {
            init_no_store_blocks2_avx2<num_keys>(keys, iv, tweaks, counters, output);
            return;
        }
#endif
        if constexpr (!USE_AVX2 && blocks_per_key == 2)
        {
            init_no_store_blocks2_scalar<num_keys>(keys, iv, tweaks, counters, output);
            return;
        }

        expanded_key_t expanded_keys[num_keys];
        init_impl<num_keys, blocks_per_key>(keys, expanded_keys, iv, tweaks, counters, output);
    }

private:
    static constexpr uint8_t SIGMA[10][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
        {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
        {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
        {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
        {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
    };

    template <size_t num_keys>
    static void init_keys(const key_t* keys, expanded_key_t* expanded_keys)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            std::memcpy(expanded_keys[i].m, &keys[i], sizeof(expanded_keys[i].m));
#if defined(__AVX2__)
            if constexpr (USE_AVX2)
            {
                for (size_t w = 0; w < 16; ++w)
                    expanded_keys[i].m8[w] = _mm256_set1_epi32((int)expanded_keys[i].m[w]);
            }
#endif
        }
    }

    static ALWAYS_INLINE uint32_t rotr32(uint32_t x, unsigned n)
    {
        return (x >> n) | (x << (32 - n));
    }

    static ALWAYS_INLINE void g(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d,
                                uint32_t x, uint32_t y)
    {
        a = a + b + x;
        d = rotr32(d ^ a, 16);
        c = c + d;
        b = rotr32(b ^ c, 12);
        a = a + b + y;
        d = rotr32(d ^ a, 8);
        c = c + d;
        b = rotr32(b ^ c, 7);
    }

    template <size_t R>
    static ALWAYS_INLINE void round_scalar(uint32_t v[16], const uint32_t m[16])
    {
        g(v[0], v[4], v[8], v[12], m[SIGMA[R][0]], m[SIGMA[R][1]]);
        g(v[1], v[5], v[9], v[13], m[SIGMA[R][2]], m[SIGMA[R][3]]);
        g(v[2], v[6], v[10], v[14], m[SIGMA[R][4]], m[SIGMA[R][5]]);
        g(v[3], v[7], v[11], v[15], m[SIGMA[R][6]], m[SIGMA[R][7]]);
        g(v[0], v[5], v[10], v[15], m[SIGMA[R][8]], m[SIGMA[R][9]]);
        g(v[1], v[6], v[11], v[12], m[SIGMA[R][10]], m[SIGMA[R][11]]);
        g(v[2], v[7], v[8], v[13], m[SIGMA[R][12]], m[SIGMA[R][13]]);
        g(v[3], v[4], v[9], v[14], m[SIGMA[R][14]], m[SIGMA[R][15]]);
    }

    static void permute_scalar(const uint32_t m[16], const block_t& input, block_t& output)
    {
        uint32_t v[16];
        std::memcpy(v, &input, sizeof(v));
        round_scalar<0>(v, m);
        round_scalar<1>(v, m);
        round_scalar<2>(v, m);
        round_scalar<3>(v, m);
        round_scalar<4>(v, m);
        round_scalar<5>(v, m);
        round_scalar<6>(v, m);
        round_scalar<7>(v, m);
        round_scalar<8>(v, m);
        round_scalar<9>(v, m);
        std::memcpy(&output, v, sizeof(v));
    }

    static void permute_scalar_input_words(const uint32_t m[16], const uint32_t iv_words[16],
                                           tweak_t tweak, count_t counter, block_t& output)
    {
        uint32_t v[16];
        std::memcpy(v, iv_words, sizeof(v));
        v[0] += counter;
        v[15] += tweak;
        round_scalar<0>(v, m);
        round_scalar<1>(v, m);
        round_scalar<2>(v, m);
        round_scalar<3>(v, m);
        round_scalar<4>(v, m);
        round_scalar<5>(v, m);
        round_scalar<6>(v, m);
        round_scalar<7>(v, m);
        round_scalar<8>(v, m);
        round_scalar<9>(v, m);
        std::memcpy(&output, v, sizeof(v));
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_scalar(const expanded_key_t* expanded_keys, const iv_t& iv,
                           const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        uint32_t iv_words[16];
        std::memcpy(iv_words, &iv, sizeof(iv_words));
        for (size_t i = 0; i < num_keys; ++i)
        {
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
            {
                permute_scalar_input_words(expanded_keys[i].m, iv_words, tweaks[i], cnt,
                                           output[i * blocks_per_key + j]);
            }
        }
    }

    template <size_t num_keys>
    static void init_no_store_blocks2_scalar(const key_t* keys, const iv_t& iv,
                                             const tweak_t* tweaks, const count_t* counters,
                                             block_t* output)
    {
        uint32_t iv_words[16];
        std::memcpy(iv_words, &iv, sizeof(iv_words));
        for (size_t i = 0; i < num_keys; ++i)
        {
            uint32_t key_words[16];
            std::memcpy(key_words, &keys[i], sizeof(key_words));
            permute_scalar_input_words(key_words, iv_words, tweaks[i], counters[i],
                                       output[2 * i]);
            permute_scalar_input_words(key_words, iv_words, tweaks[i], counters[i] + 1,
                                       output[2 * i + 1]);
        }
    }

#if defined(__AVX2__)
    static ALWAYS_INLINE __m256i rotr32(__m256i x, unsigned n)
    {
        if (n == 16)
        {
            const __m256i shuf = _mm256_setr_epi8(
                2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13,
                2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13);
            return _mm256_shuffle_epi8(x, shuf);
        }
        if (n == 8)
        {
            const __m256i shuf = _mm256_setr_epi8(
                1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12,
                1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12);
            return _mm256_shuffle_epi8(x, shuf);
        }
        return _mm256_or_si256(_mm256_srli_epi32(x, n), _mm256_slli_epi32(x, 32 - n));
    }

    static ALWAYS_INLINE void g(__m256i& a, __m256i& b, __m256i& c, __m256i& d,
                                const __m256i& x, const __m256i& y)
    {
        a = _mm256_add_epi32(_mm256_add_epi32(a, b), x);
        d = rotr32(_mm256_xor_si256(d, a), 16);
        c = _mm256_add_epi32(c, d);
        b = rotr32(_mm256_xor_si256(b, c), 12);
        a = _mm256_add_epi32(_mm256_add_epi32(a, b), y);
        d = rotr32(_mm256_xor_si256(d, a), 8);
        c = _mm256_add_epi32(c, d);
        b = rotr32(_mm256_xor_si256(b, c), 7);
    }

    template <size_t R>
    static ALWAYS_INLINE void round_vec(__m256i v[16], const __m256i m[16])
    {
        g(v[0], v[4], v[8], v[12], m[SIGMA[R][0]], m[SIGMA[R][1]]);
        g(v[1], v[5], v[9], v[13], m[SIGMA[R][2]], m[SIGMA[R][3]]);
        g(v[2], v[6], v[10], v[14], m[SIGMA[R][4]], m[SIGMA[R][5]]);
        g(v[3], v[7], v[11], v[15], m[SIGMA[R][6]], m[SIGMA[R][7]]);
        g(v[0], v[5], v[10], v[15], m[SIGMA[R][8]], m[SIGMA[R][9]]);
        g(v[1], v[6], v[11], v[12], m[SIGMA[R][10]], m[SIGMA[R][11]]);
        g(v[2], v[7], v[8], v[13], m[SIGMA[R][12]], m[SIGMA[R][13]]);
        g(v[3], v[4], v[9], v[14], m[SIGMA[R][14]], m[SIGMA[R][15]]);
    }

    static ALWAYS_INLINE void permute8_vec(const __m256i m[16], const __m256i input[16],
                                           __m256i output[16])
    {
        __m256i v[16];
        for (size_t i = 0; i < 16; ++i)
            v[i] = input[i];
        round_vec<0>(v, m);
        round_vec<1>(v, m);
        round_vec<2>(v, m);
        round_vec<3>(v, m);
        round_vec<4>(v, m);
        round_vec<5>(v, m);
        round_vec<6>(v, m);
        round_vec<7>(v, m);
        round_vec<8>(v, m);
        round_vec<9>(v, m);
        for (size_t i = 0; i < 16; ++i)
            output[i] = v[i];
    }

    static ALWAYS_INLINE __m256i set8(const uint32_t* w, size_t idx)
    {
        return _mm256_setr_epi32((int)w[0 * 16 + idx], (int)w[1 * 16 + idx],
                                 (int)w[2 * 16 + idx], (int)w[3 * 16 + idx],
                                 (int)w[4 * 16 + idx], (int)w[5 * 16 + idx],
                                 (int)w[6 * 16 + idx], (int)w[7 * 16 + idx]);
    }

    static ALWAYS_INLINE __m256i set8_keys(const expanded_key_t* expanded_keys,
                                           const size_t key_idx[8], size_t word)
    {
        return _mm256_setr_epi32(
            (int)expanded_keys[key_idx[0]].m[word], (int)expanded_keys[key_idx[1]].m[word],
            (int)expanded_keys[key_idx[2]].m[word], (int)expanded_keys[key_idx[3]].m[word],
            (int)expanded_keys[key_idx[4]].m[word], (int)expanded_keys[key_idx[5]].m[word],
            (int)expanded_keys[key_idx[6]].m[word], (int)expanded_keys[key_idx[7]].m[word]);
    }

    static ALWAYS_INLINE __m256i set8_words(const uint32_t words[8])
    {
        return _mm256_setr_epi32((int)words[0], (int)words[1], (int)words[2], (int)words[3],
                                 (int)words[4], (int)words[5], (int)words[6], (int)words[7]);
    }

    static void store8_blocks(block_t* __restrict__ output, size_t first_block,
                              const __m256i out[16])
    {
        __m256i lo[8];
        __m256i hi[8];
        transpose8x8(out, lo);
        transpose8x8(out + 8, hi);
        for (size_t lane = 0; lane < 8; ++lane)
        {
            uint32_t* block_words =
                reinterpret_cast<uint32_t*>(&output[first_block + lane]);
            _mm256_storeu_si256((__m256i*)&block_words[0], lo[lane]);
            _mm256_storeu_si256((__m256i*)&block_words[8], hi[lane]);
        }
    }

    static ALWAYS_INLINE void transpose8x8(const __m256i in[8], __m256i out[8])
    {
        const __m256i t0 = _mm256_unpacklo_epi32(in[0], in[1]);
        const __m256i t1 = _mm256_unpackhi_epi32(in[0], in[1]);
        const __m256i t2 = _mm256_unpacklo_epi32(in[2], in[3]);
        const __m256i t3 = _mm256_unpackhi_epi32(in[2], in[3]);
        const __m256i t4 = _mm256_unpacklo_epi32(in[4], in[5]);
        const __m256i t5 = _mm256_unpackhi_epi32(in[4], in[5]);
        const __m256i t6 = _mm256_unpacklo_epi32(in[6], in[7]);
        const __m256i t7 = _mm256_unpackhi_epi32(in[6], in[7]);

        const __m256i u0 = _mm256_unpacklo_epi64(t0, t2);
        const __m256i u1 = _mm256_unpackhi_epi64(t0, t2);
        const __m256i u2 = _mm256_unpacklo_epi64(t1, t3);
        const __m256i u3 = _mm256_unpackhi_epi64(t1, t3);
        const __m256i u4 = _mm256_unpacklo_epi64(t4, t6);
        const __m256i u5 = _mm256_unpackhi_epi64(t4, t6);
        const __m256i u6 = _mm256_unpacklo_epi64(t5, t7);
        const __m256i u7 = _mm256_unpackhi_epi64(t5, t7);

        out[0] = _mm256_permute2x128_si256(u0, u4, 0x20);
        out[1] = _mm256_permute2x128_si256(u1, u5, 0x20);
        out[2] = _mm256_permute2x128_si256(u2, u6, 0x20);
        out[3] = _mm256_permute2x128_si256(u3, u7, 0x20);
        out[4] = _mm256_permute2x128_si256(u0, u4, 0x31);
        out[5] = _mm256_permute2x128_si256(u1, u5, 0x31);
        out[6] = _mm256_permute2x128_si256(u2, u6, 0x31);
        out[7] = _mm256_permute2x128_si256(u3, u7, 0x31);
    }

    static ALWAYS_INLINE void pack_one_key8(const uint32_t iv_words[16], tweak_t tweak,
                                            count_t counter, __m256i in[16])
    {
        for (size_t w = 0; w < 16; ++w)
        {
            in[w] = _mm256_set1_epi32((int)iv_words[w]);
        }
        in[0] = _mm256_setr_epi32(
            (int)(iv_words[0] + counter + 0), (int)(iv_words[0] + counter + 1),
            (int)(iv_words[0] + counter + 2), (int)(iv_words[0] + counter + 3),
            (int)(iv_words[0] + counter + 4), (int)(iv_words[0] + counter + 5),
            (int)(iv_words[0] + counter + 6), (int)(iv_words[0] + counter + 7));
        in[15] = _mm256_set1_epi32((int)(iv_words[15] + tweak));
    }

    static ALWAYS_INLINE void pack_blocks2_4keys(const expanded_key_t* expanded_keys,
                                                 const uint32_t iv_words[16],
                                                 const tweak_t* tweaks,
                                                 const count_t* counters,
                                                 __m256i m[16], __m256i in[16])
    {
        for (size_t w = 0; w < 16; ++w)
        {
            m[w] = _mm256_setr_epi32(
                (int)expanded_keys[0].m[w], (int)expanded_keys[0].m[w],
                (int)expanded_keys[1].m[w], (int)expanded_keys[1].m[w],
                (int)expanded_keys[2].m[w], (int)expanded_keys[2].m[w],
                (int)expanded_keys[3].m[w], (int)expanded_keys[3].m[w]);
            in[w] = _mm256_set1_epi32((int)iv_words[w]);
        }
        in[0] = _mm256_setr_epi32(
            (int)(iv_words[0] + counters[0] + 0), (int)(iv_words[0] + counters[0] + 1),
            (int)(iv_words[0] + counters[1] + 0), (int)(iv_words[0] + counters[1] + 1),
            (int)(iv_words[0] + counters[2] + 0), (int)(iv_words[0] + counters[2] + 1),
            (int)(iv_words[0] + counters[3] + 0), (int)(iv_words[0] + counters[3] + 1));
        in[15] = _mm256_setr_epi32(
            (int)(iv_words[15] + tweaks[0]), (int)(iv_words[15] + tweaks[0]),
            (int)(iv_words[15] + tweaks[1]), (int)(iv_words[15] + tweaks[1]),
            (int)(iv_words[15] + tweaks[2]), (int)(iv_words[15] + tweaks[2]),
            (int)(iv_words[15] + tweaks[3]), (int)(iv_words[15] + tweaks[3]));
    }

    static ALWAYS_INLINE void pack_blocks2_4raw_keys(const key_t* keys,
                                                     const uint32_t iv_words[16],
                                                     const tweak_t* tweaks,
                                                     const count_t* counters,
                                                     __m256i m[16], __m256i in[16])
    {
        const uint32_t* key0 = reinterpret_cast<const uint32_t*>(&keys[0]);
        const uint32_t* key1 = reinterpret_cast<const uint32_t*>(&keys[1]);
        const uint32_t* key2 = reinterpret_cast<const uint32_t*>(&keys[2]);
        const uint32_t* key3 = reinterpret_cast<const uint32_t*>(&keys[3]);
        for (size_t w = 0; w < 16; ++w)
        {
            m[w] = _mm256_setr_epi32((int)key0[w], (int)key0[w], (int)key1[w], (int)key1[w],
                                     (int)key2[w], (int)key2[w], (int)key3[w], (int)key3[w]);
            in[w] = _mm256_set1_epi32((int)iv_words[w]);
        }
        in[0] = _mm256_setr_epi32(
            (int)(iv_words[0] + counters[0] + 0), (int)(iv_words[0] + counters[0] + 1),
            (int)(iv_words[0] + counters[1] + 0), (int)(iv_words[0] + counters[1] + 1),
            (int)(iv_words[0] + counters[2] + 0), (int)(iv_words[0] + counters[2] + 1),
            (int)(iv_words[0] + counters[3] + 0), (int)(iv_words[0] + counters[3] + 1));
        in[15] = _mm256_setr_epi32(
            (int)(iv_words[15] + tweaks[0]), (int)(iv_words[15] + tweaks[0]),
            (int)(iv_words[15] + tweaks[1]), (int)(iv_words[15] + tweaks[1]),
            (int)(iv_words[15] + tweaks[2]), (int)(iv_words[15] + tweaks[2]),
            (int)(iv_words[15] + tweaks[3]), (int)(iv_words[15] + tweaks[3]));
    }

    template <count_t blocks_per_key>
    static ALWAYS_INLINE void pack_generic8(const expanded_key_t* expanded_keys,
                                            const uint32_t iv_words[16],
                                            const tweak_t* tweaks, const count_t* counters,
                                            size_t first_block, __m256i m[16], __m256i in[16])
    {
        size_t key_idx[8];
        uint32_t in0[8];
        uint32_t in15[8];
        for (size_t lane = 0; lane < 8; ++lane)
        {
            key_idx[lane] = (first_block + lane) / blocks_per_key;
            in0[lane] = iv_words[0] + counters[key_idx[lane]] +
                        (uint32_t)((first_block + lane) % blocks_per_key);
            in15[lane] = iv_words[15] + tweaks[key_idx[lane]];
        }
        for (size_t w = 0; w < 16; ++w)
        {
            m[w] = set8_keys(expanded_keys, key_idx, w);
            in[w] = _mm256_set1_epi32((int)iv_words[w]);
        }
        in[0] = set8_words(in0);
        in[15] = set8_words(in15);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_avx2(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        constexpr size_t total_blocks = num_keys * blocks_per_key;
        uint32_t iv_words[16];
        std::memcpy(iv_words, &iv, sizeof(iv_words));

        size_t k = 0;
        for (; k + 8 <= total_blocks; k += 8)
        {
            __m256i m[16];
            __m256i in[16];
            if constexpr (num_keys == 1)
            {
                pack_one_key8(iv_words, tweaks[0], counters[0] + (count_t)k, in);
                __m256i out[16];
                permute8_vec(expanded_keys[0].m8, in, out);
                store8_blocks(output, k, out);
                continue;
            }
            else if constexpr (blocks_per_key == 2)
            {
                const size_t first_key = k / blocks_per_key;
                pack_blocks2_4keys(&expanded_keys[first_key], iv_words, &tweaks[first_key],
                                   &counters[first_key], m, in);
            }
            else
            {
                pack_generic8<blocks_per_key>(expanded_keys, iv_words, tweaks, counters, k, m, in);
            }

            __m256i out[16];
            permute8_vec(m, in, out);
            store8_blocks(output, k, out);
        }

        if constexpr ((total_blocks % 8) != 0)
        {
            for (; k < total_blocks; ++k)
            {
                const size_t key_idx = k / blocks_per_key;
                const count_t counter = counters[key_idx] + (count_t)(k % blocks_per_key);
                const block_t input = make_input_block(iv, tweaks[key_idx], counter);
                permute_scalar(expanded_keys[key_idx].m, input, output[k]);
            }
        }
    }

    template <size_t num_keys>
    static void init_no_store_blocks2_avx2(const key_t* keys, const iv_t& iv,
                                           const tweak_t* tweaks, const count_t* counters,
                                           block_t* output)
    {
        uint32_t iv_words[16];
        std::memcpy(iv_words, &iv, sizeof(iv_words));

        for (size_t i = 0; i < num_keys; i += 4)
        {
            __m256i m[16];
            __m256i in[16];
            pack_blocks2_4raw_keys(&keys[i], iv_words, &tweaks[i], &counters[i], m, in);
            __m256i out[16];
            permute8_vec(m, in, out);
            store8_blocks(output, 2 * i, out);
        }
    }
#endif
};

struct blake2s_512_bc_ref_prg
    : public blake2s_512_bc_base<blake2s_512_bc_ref_prg, false>
{};

struct blake2s_512_bc_prg : public blake2s_512_bc_base<blake2s_512_bc_prg, true>
{};

template <secpar S> struct aes_ctr_prg;

template <secpar S>
struct prg_trait<aes_ctr_prg<S>>
{
    using expanded_key_t = aes_round_keys<S>;
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = AES_PREFERRED_WIDTH_SHIFT;
};

template <secpar S>
struct aes_ctr_prg : public prg_base<aes_ctr_prg<S>>
{
    typedef prg_base<aes_ctr_prg> base;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
        aes_keygen_ecb<S, num_keys, blocks_per_key>(keys, expanded_keys, output);

        //tweak_debug<num_keys, blocks_per_key>(expanded_keys, tweaks, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                         const count_t* counters, block_t* output)
    {
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
        aes_ecb<S, num_keys, blocks_per_key>(expanded_keys, output);

        //tweak_debug<num_keys, blocks_per_key>(expanded_keys, tweaks, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void build_ecb_input_public(const iv_t& __restrict__ iv,
                                       const tweak_t* __restrict__ tweaks,
                                       const count_t* __restrict__ counters,
                                       block_t* __restrict__ ecb_blocks)
    {
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, ecb_blocks);
    }

private:
    template <size_t num_keys, count_t blocks_per_key>
    static void build_ecb_input(const iv_t& __restrict__ iv, const tweak_t* __restrict__ tweaks,
                                const count_t* __restrict__ counters, block_t* __restrict__ ecb_blocks)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            tweak_t twk = tweaks[i];
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
                ecb_blocks[i * blocks_per_key + j] = iv.add32(block_t::set_low_high32(cnt, twk));
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void tweak_debug(const expanded_key_t* expanded_keys, const tweak_t* __restrict__ tweaks,
                            block_t* __restrict__ output)
    {
        if constexpr (S == secpar::s128)
        {
            for (size_t i = 0; i < num_keys; ++i)
            {
                tweak_t twk = tweaks[i];
                block128 shifted_key = {_mm_slli_si128(expanded_keys[i].keys[0].data, 2)};
                for (count_t j = 0; j < blocks_per_key; ++j)
                    output[i * blocks_per_key + j] = shifted_key.add32(block_t::set_low32(twk));
            }
        }
    }

};

struct aes192_ctr_trunc160_prg;

template <>
struct prg_trait<aes192_ctr_trunc160_prg>
{
    using expanded_key_t = aes_round_keys<secpar::s192>;
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s160;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = AES_PREFERRED_WIDTH_SHIFT;
};

struct aes192_ctr_trunc160_prg : public prg_base<aes192_ctr_trunc160_prg>
{
    typedef prg_base<aes192_ctr_trunc160_prg> base;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        block192 padded_keys[num_keys];
        pad_keys<num_keys>(keys, padded_keys);
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
        aes_keygen_ecb<secpar::s192, num_keys, blocks_per_key>(
            padded_keys, expanded_keys, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
        aes_ecb<secpar::s192, num_keys, blocks_per_key>(expanded_keys, output);
    }

private:
    template <size_t num_keys>
    static void pad_keys(const key_t* __restrict__ keys, block192* __restrict__ padded_keys)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            padded_keys[i] = block192::set_zero();
            memcpy(&padded_keys[i], &keys[i], sizeof(keys[i]));
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void build_ecb_input(const iv_t& __restrict__ iv,
                                const tweak_t* __restrict__ tweaks,
                                const count_t* __restrict__ counters,
                                block_t* __restrict__ ecb_blocks)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            tweak_t twk = tweaks[i];
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
                ecb_blocks[i * blocks_per_key + j] = iv.add32(block_t::set_low_high32(cnt, twk));
        }
    }
};

template <secpar S> struct rijndael_ctr_prg;

template <secpar S>
struct prg_trait<rijndael_ctr_prg<S>>
{
    using expanded_key_t = rijndael_round_keys<S>;
    using iv_t = block_secpar<S>;
    using block_t = block_secpar<S>;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = RIJNDAEL_CTR_PREFERRED_WIDTH_SHIFT<S>;
};

template <>
struct prg_trait<rijndael_ctr_prg<secpar::s160>>
{
    using expanded_key_t = rijndael192_round_keys;
    using iv_t = block160;
    using block_t = block160;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = secpar::s160;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = RIJNDAEL_CTR_PREFERRED_WIDTH_SHIFT<secpar::s160>;
};

template <secpar S>
struct rijndael_ctr_prg : public prg_base<rijndael_ctr_prg<S>>
{
    typedef prg_base<rijndael_ctr_prg> base;
    static constexpr bool HAS_INIT_NO_STORE =
        (S == secpar::s160 || S == secpar::s192 || S == secpar::s256);
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        if constexpr (S == secpar::s160)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael192_keygen_ecb_trunc160<num_keys, blocks_per_key>(
                keys, expanded_keys, output);
        }
        else if constexpr (S == secpar::s192)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael192_keygen_ecb<num_keys, blocks_per_key>(keys, expanded_keys, output);
        }
        else if constexpr (S == secpar::s256)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael256_keygen_ecb<num_keys, blocks_per_key>(keys, expanded_keys, output);
        }
        else
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            for (size_t i = 0; i < num_keys; ++i)
                rijndael_keygen<S>(&expanded_keys[i], keys[i]);
            encrypt_ctr_blocks<num_keys, blocks_per_key>(expanded_keys, output);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void init_no_store(const key_t* keys, const iv_t& iv, const tweak_t* tweaks,
                              const count_t* counters, block_t* output)
    {
        if constexpr (S == secpar::s160)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael192_keygen_ecb_trunc160_no_store<num_keys, blocks_per_key>(keys, output);
        }
        else if constexpr (S == secpar::s192)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael192_keygen_ecb_no_store<num_keys, blocks_per_key>(keys, output);
        }
        else if constexpr (S == secpar::s256)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael256_keygen_ecb_no_store<num_keys, blocks_per_key>(keys, output);
        }
        else
            static_assert(S == secpar::s160 || S == secpar::s192 || S == secpar::s256);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        if constexpr (S == secpar::s160)
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            rijndael192_ecb_trunc160<num_keys, blocks_per_key>(expanded_keys, output);
        }
        else
        {
            build_ctr_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
            encrypt_ctr_blocks<num_keys, blocks_per_key>(expanded_keys, output);
        }
    }

private:
    template <size_t num_keys, count_t blocks_per_key>
    static void build_ctr_input(const iv_t& __restrict__ iv, const tweak_t* __restrict__ tweaks,
                                const count_t* __restrict__ counters,
                                block_t* __restrict__ ctr_blocks)
    {
        const block_t iv_block = pad_iv(iv);
        for (size_t i = 0; i < num_keys; ++i)
        {
            tweak_t twk = tweaks[i];
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
                ctr_blocks[i * blocks_per_key + j] = make_ctr_block(iv_block, cnt, twk);
        }
    }

    static block_t pad_iv(const iv_t& iv)
    {
        if constexpr (sizeof(iv_t) == sizeof(block_t))
        {
            return iv;
        }
        else if constexpr (S == secpar::s192)
        {
            block192 out = {{((uint64_t)iv.data[1] << 32) | iv.data[0],
                              ((uint64_t)iv.data[3] << 32) | iv.data[2],
                              (uint64_t)iv.data[4]}};
            return out;
        }
        else if constexpr (S == secpar::s256)
        {
            return block256::block256_set_low160(iv);
        }
        else
        {
            block_t out = block_t::set_zero();
            memcpy(&out, &iv, sizeof(iv));
            return out;
        }
    }

    static block_t make_ctr_block(const block_t& iv, count_t counter, tweak_t tweak)
    {
        return iv.add32(block_t::set_low_high32(counter, tweak));
    }

    template <size_t num_keys>
    static void pad_keys(const key_t* __restrict__ keys, block192* __restrict__ padded_keys)
    {
        for (size_t i = 0; i < num_keys; ++i)
        {
            padded_keys[i] = block192::set_zero();
            memcpy(&padded_keys[i], &keys[i], sizeof(keys[i]));
        }
    }

    static block192 pad_block(block160 block)
    {
        block192 out = block192::set_zero();
        memcpy(&out, &block, sizeof(block));
        return out;
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void build_ctr_input_padded(const iv_t& __restrict__ iv,
                                       const tweak_t* __restrict__ tweaks,
                                       const count_t* __restrict__ counters,
                                       block192* __restrict__ ctr_blocks)
    {
        const block_t iv_block = pad_iv(iv);
        for (size_t i = 0; i < num_keys; ++i)
        {
            tweak_t twk = tweaks[i];
            for (count_t j = 0, cnt = counters[i]; j < blocks_per_key; ++j, ++cnt)
                ctr_blocks[i * blocks_per_key + j] = pad_block(make_ctr_block(iv_block, cnt, twk));
        }
    }

    template <size_t blocks>
    static void truncate_output(const block192* __restrict__ in, block160* __restrict__ out)
    {
        for (size_t i = 0; i < blocks; ++i)
            memcpy(&out[i], &in[i], sizeof(out[i]));
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void encrypt_ctr_blocks(const expanded_key_t* __restrict__ expanded_keys,
                                   block_t* __restrict__ output)
    {
        rijndael_ecb<S, num_keys, blocks_per_key>(expanded_keys, output);
    }
};

template <secpar S> struct aes_secpar_fixed_key_ctr_prg;

template <secpar S>
struct prg_trait<aes_secpar_fixed_key_ctr_prg<S>>
{
    using expanded_key_t = block_secpar<S>;
    using iv_t = block_secpar<S>;
    using block_t = block_secpar<S>;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = FIXED_KEY_PREFERRED_WIDTH_SHIFT<S>;
};

template <secpar S>
struct aes_secpar_fixed_key_ctr_prg : public prg_base<aes_secpar_fixed_key_ctr_prg<S>>
{
    typedef prg_base<aes_secpar_fixed_key_ctr_prg> base;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        memcpy(expanded_keys, keys, num_keys * sizeof(keys[0]));
        base::template gen<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                         const count_t* counters, block_t* output)
    {
        aes_secpar_fixed_key_ctr<S, num_keys, blocks_per_key>(
            &get_fixed_key(), expanded_keys, iv, tweaks, counters, output);
    }

private:
    struct fixed_key_initializer
    {
        aes_secpar_round_keys<S> round_keys;

        fixed_key_initializer(block_secpar<S> fixed_key)
        {
            aes_secpar_keygen<S>(&round_keys, fixed_key);
        }
    };

    static const aes_secpar_round_keys<S>& get_fixed_key()
    {
        static const fixed_key_initializer initializer(block_secpar<S>::set_zero());
        return initializer.round_keys;
    }
};

template <secpar S> struct rijndael_fixed_key_ctr_prg;

template <secpar S>
struct prg_trait<rijndael_fixed_key_ctr_prg<S>>
{
    using expanded_key_t = block_secpar<S>;
    using iv_t = block_secpar<S>;
    using block_t = block_secpar<S>;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = FIXED_KEY_PREFERRED_WIDTH_SHIFT<S>;
};

template <secpar S>
struct rijndael_fixed_key_ctr_prg : public prg_base<rijndael_fixed_key_ctr_prg<S>>
{
    typedef prg_base<rijndael_fixed_key_ctr_prg> base;
    using typename base::key_t;
    using typename base::expanded_key_t;
    using typename base::iv_t;
    using typename base::block_t;
    using typename base::tweak_t;
    using typename base::count_t;

    template <size_t num_keys, count_t blocks_per_key>
    static void init_impl(const key_t* keys, expanded_key_t* expanded_keys,
                          const iv_t& iv, const tweak_t* tweaks, const count_t* counters,
                          block_t* output)
    {
        memcpy(expanded_keys, keys, num_keys * sizeof(keys[0]));
        base::template gen<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        rijndael_fixed_key_ctr<S, num_keys, blocks_per_key>(
            &get_fixed_key(), expanded_keys, iv, tweaks, counters, output);
    }

private:
    struct fixed_key_initializer
    {
        rijndael_round_keys<S> round_keys;

        fixed_key_initializer(block_secpar<S> fixed_key)
        {
            rijndael_keygen<S>(&round_keys, fixed_key);
        }
    };

    static const rijndael_round_keys<S>& get_fixed_key()
    {
        static const fixed_key_initializer initializer(block_secpar<S>::set_zero());
        return initializer.round_keys;
    }
};

} // namespace sydo

#endif
