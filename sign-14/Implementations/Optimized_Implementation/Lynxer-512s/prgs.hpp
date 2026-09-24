#ifndef PRGS_HPP
#define PRGS_HPP

#include "aes.hpp"
#include "hash.hpp"
#include "parameters.hpp"
#include "tccr_hash.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <type_traits>

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
#include <immintrin.h>
#include <wmmintrin.h>
#endif

namespace sig
{

// Thread-local NGCC TCCR context. VOLE commit/reconstruct sets this while expanding BAVC trees.
inline thread_local const uint8_t* g_tccr_s = nullptr;
inline thread_local size_t g_tccr_commit_leaves = 0;
#define SIG_HAS_TCCR_CONTEXT 1

template <typename T>
struct prg_trait;

template <typename Derived>
struct prg_base
{
    static constexpr secpar secpar_v = prg_trait<Derived>::secpar_v;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = prg_trait<Derived>::PREFERRED_WIDTH_SHIFT;
    static constexpr size_t PREFERRED_WIDTH = 1 << PREFERRED_WIDTH_SHIFT;

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
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 3;
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

// Forward declaration for ref_prg (defined later, used by ref PRG types).
namespace ref_prg { namespace detail {
void prg_aes_ctr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                 uint8_t* out, unsigned int csp, size_t outlen);
void prg_tccr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
              uint8_t* out, unsigned int csp, size_t outlen);
void prg_shacal2_ctr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                     uint8_t* out, size_t outlen);
}}

template <secpar S> struct aes_ctr_prg;
template <secpar S> struct ref_tccr_prg;

struct ref_tccr384_expanded_key
{
    block_secpar<secpar::s384> key;
    mutable bool cache_valid;
    mutable size_t cached_block_index;
    mutable uint8_t cached_block[secpar_to_bytes(secpar::s384)];
};
template <secpar S> struct ref_shacal2_prg;
template <secpar S> struct tccr_hash_prg;

template <secpar S>
struct prg_trait<aes_ctr_prg<S>>
{
    using expanded_key_t =
        std::conditional_t<S == secpar::s160, block_secpar<secpar::s192>,
                           aes_round_keys<S>>;
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
    static_assert(S == secpar::s160 || S == secpar::s256,
                  "NGCC AES-CTR PRG is defined for Lynx CSP 160 and 256");

    typedef prg_base<aes_ctr_prg<S>> base;
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

        if constexpr (S == secpar::s160)
        {
            for (size_t i = 0; i < num_keys; ++i)
            {
                expanded_keys[i] = block_secpar<secpar::s192>::set_zero();
                memcpy(&expanded_keys[i], &keys[i], secpar_to_bytes(S));
            }
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
            aes_ctr_keygen_ecb_vaes<num_keys, blocks_per_key>(expanded_keys, output);
#else
            aes_ctr_keygen_ecb_fallback<num_keys, blocks_per_key>(expanded_keys, output);
#endif
        }
        else
        {
            aes_keygen_ecb<secpar::s256, num_keys, blocks_per_key>(keys, expanded_keys, output);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        build_ecb_input<num_keys, blocks_per_key>(iv, tweaks, counters, output);
        if constexpr (S == secpar::s160)
        {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
            aes_ctr_keygen_ecb_vaes<num_keys, blocks_per_key>(expanded_keys, output);
#else
            aes_ctr_keygen_ecb_fallback<num_keys, blocks_per_key>(expanded_keys, output);
#endif
        }
        else
            aes_ecb<secpar::s256, num_keys, blocks_per_key>(expanded_keys, output);
    }

private:
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
    static __m512i aes_ctr_pack4x128(__m128i x0, __m128i x1, __m128i x2, __m128i x3)
    {
        __m512i v = _mm512_castsi128_si512(x0);
        v = _mm512_inserti32x4(v, x1, 1);
        v = _mm512_inserti32x4(v, x2, 2);
        return _mm512_inserti32x4(v, x3, 3);
    }

    static __m512i aes_ctr_splat128(__m128i x)
    {
        return _mm512_broadcast_i32x4(x);
    }

    static __m512i aes_ctr_xor_prefix128_512(__m512i x)
    {
        x = _mm512_xor_si512(x, _mm512_bslli_epi128(x, 4));
        x = _mm512_xor_si512(x, _mm512_bslli_epi128(x, 8));
        return x;
    }

    static __m512i aes_ctr_aes_rot_sub_rcon_word1_512(__m512i k, uint8_t rcon)
    {
        const __m512i rt =
            aes_ctr_splat128(_mm_set_epi32(0x04070605, 0x04070605, 0x04070605, 0x04070605));
        return _mm512_aesenclast_epi128(_mm512_shuffle_epi8(k, rt), _mm512_set1_epi32(rcon));
    }

    static void aes_ctr_aes192_assist512(__m512i& lo, __m512i& hi, uint8_t rcon)
    {
        lo = _mm512_xor_si512(aes_ctr_xor_prefix128_512(lo),
                              aes_ctr_aes_rot_sub_rcon_word1_512(hi, rcon));
        const __m512i rep = _mm512_shuffle_epi32(lo, static_cast<_MM_PERM_ENUM>(0xff));
        hi = _mm512_xor_si512(hi, _mm512_bslli_epi128(hi, 4));
        hi = _mm512_xor_si512(hi, rep);
    }

    static __m512i aes_ctr_dup_low64_512(__m512i x)
    {
        return _mm512_shuffle_epi32(x, static_cast<_MM_PERM_ENUM>(_MM_SHUFFLE(1, 0, 1, 0)));
    }

    static __m512i aes_ctr_dup_high64_512(__m512i x)
    {
        return _mm512_shuffle_epi32(x, static_cast<_MM_PERM_ENUM>(_MM_SHUFFLE(3, 2, 3, 2)));
    }

    static __m512i aes_ctr_aes192_rk_from_hi_lo(__m512i hi, __m512i lo)
    {
        return _mm512_mask_blend_epi32(0xCCCC, hi, aes_ctr_dup_low64_512(lo));
    }

    static __m512i aes_ctr_aes192_rk_from_lo_hi(__m512i lo, __m512i hi)
    {
        return _mm512_mask_blend_epi32(0xCCCC, aes_ctr_dup_high64_512(lo),
                                       aes_ctr_dup_low64_512(hi));
    }

    static __m512i aes_ctr_aes192_encrypt4_ks(__m512i lo, __m512i hi, __m512i pt)
    {
        static const uint8_t rcon[8] = {0x01, 0x02, 0x04, 0x08,
                                        0x10, 0x20, 0x40, 0x80};
        __m512i s = _mm512_xor_si512(pt, lo);

        __m512i old_hi = hi;
        aes_ctr_aes192_assist512(lo, hi, rcon[0]);
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_hi_lo(old_hi, lo));
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_lo_hi(lo, hi));

        aes_ctr_aes192_assist512(lo, hi, rcon[1]);
        s = _mm512_aesenc_epi128(s, lo);
        old_hi = hi;
        aes_ctr_aes192_assist512(lo, hi, rcon[2]);
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_hi_lo(old_hi, lo));
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_lo_hi(lo, hi));

        aes_ctr_aes192_assist512(lo, hi, rcon[3]);
        s = _mm512_aesenc_epi128(s, lo);
        old_hi = hi;
        aes_ctr_aes192_assist512(lo, hi, rcon[4]);
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_hi_lo(old_hi, lo));
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_lo_hi(lo, hi));

        aes_ctr_aes192_assist512(lo, hi, rcon[5]);
        s = _mm512_aesenc_epi128(s, lo);
        old_hi = hi;
        aes_ctr_aes192_assist512(lo, hi, rcon[6]);
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_hi_lo(old_hi, lo));
        s = _mm512_aesenc_epi128(s, aes_ctr_aes192_rk_from_lo_hi(lo, hi));

        aes_ctr_aes192_assist512(lo, hi, rcon[7]);
        return _mm512_aesenclast_epi128(s, lo);
    }

    static __m512i aes_ctr_load4x128(const block128* blocks, size_t i0, size_t i1,
                                     size_t i2, size_t i3)
    {
        return aes_ctr_pack4x128(blocks[i0].data, blocks[i1].data, blocks[i2].data,
                                 blocks[i3].data);
    }

    static __m128i aes_ctr_load_key_hi64(const uint8_t* key)
    {
        return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(key + 16));
    }

    static void aes_ctr_store_blocks(__m512i v, block128* blocks, size_t active)
    {
        alignas(64) uint8_t lanes[4][16];
        _mm512_store_si512(reinterpret_cast<__m512i*>(lanes), v);
        for (size_t i = 0; i < active; ++i)
            memcpy(&blocks[i], lanes[i], 16);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void aes_ctr_keygen_ecb_vaes(const expanded_key_t* expanded_keys, block128* output)
    {
        static_assert(S == secpar::s160);

        constexpr size_t total_blocks = num_keys * blocks_per_key;
        for (size_t base = 0; base < total_blocks; base += 4)
        {
            const size_t active = std::min<size_t>(4, total_blocks - base);
            const size_t idx0 = base;
            const size_t idx1 = active > 1 ? base + 1 : idx0;
            const size_t idx2 = active > 2 ? base + 2 : idx1;
            const size_t idx3 = active > 3 ? base + 3 : idx2;
            const size_t key0 = idx0 / blocks_per_key;
            const size_t key1 = idx1 / blocks_per_key;
            const size_t key2 = idx2 / blocks_per_key;
            const size_t key3 = idx3 / blocks_per_key;

            const auto* k0 = reinterpret_cast<const uint8_t*>(&expanded_keys[key0]);
            const auto* k1 = reinterpret_cast<const uint8_t*>(&expanded_keys[key1]);
            const auto* k2 = reinterpret_cast<const uint8_t*>(&expanded_keys[key2]);
            const auto* k3 = reinterpret_cast<const uint8_t*>(&expanded_keys[key3]);
            const __m512i lo = aes_ctr_pack4x128(
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(k0)),
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(k1)),
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(k2)),
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(k3)));
            const __m512i hi = aes_ctr_pack4x128(aes_ctr_load_key_hi64(k0),
                                                 aes_ctr_load_key_hi64(k1),
                                                 aes_ctr_load_key_hi64(k2),
                                                 aes_ctr_load_key_hi64(k3));
            __m512i state = aes_ctr_load4x128(output, idx0, idx1, idx2, idx3);
            state = aes_ctr_aes192_encrypt4_ks(lo, hi, state);
            aes_ctr_store_blocks(state, output + base, active);
        }
    }
#endif

    template <size_t num_keys, count_t blocks_per_key>
    static void aes_ctr_keygen_ecb_fallback(const expanded_key_t* expanded_keys, block128* output)
    {
        static_assert(S == secpar::s160);

        for (size_t key_idx = 0; key_idx < num_keys; ++key_idx)
        {
            aes_round_keys<secpar::s192> round_keys;
            aes_keygen<secpar::s192>(&round_keys, expanded_keys[key_idx]);
            aes_ecb<secpar::s192, 1, blocks_per_key>(&round_keys,
                                                     output + key_idx * blocks_per_key);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void build_ecb_input(const iv_t& iv, const tweak_t* tweaks,
                                const count_t* counters, block_t* output)
    {
        block128 iv_template = iv;
        reinterpret_cast<uint8_t*>(&iv_template)[15] = 0;

        for (size_t i = 0; i < num_keys; ++i)
        {
            for (count_t j = 0; j < blocks_per_key; ++j)
            {
                const auto tag = static_cast<uint8_t>(tweaks[i] + counters[i] + j);
                output[i * blocks_per_key + j] =
                    iv_template ^ block128::set_low_high32(0, static_cast<uint32_t>(tag) << 24);
            }
        }
    }
};

// NGCC BAVC tree PRG. This wraps the Lynx TCCR GGM rule so the existing
// expand_one_roots/expand_one_tree code can drive tree expansion unchanged.
template <secpar S>
struct prg_trait<tccr_hash_prg<S>>
{
    using expanded_key_t = block_secpar<S>;
    using iv_t = block128;
    using block_t = block_secpar<S>;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = 4;
};

template <secpar S>
struct tccr_hash_prg : public prg_base<tccr_hash_prg<S>>
{
    typedef prg_base<tccr_hash_prg<S>> base;
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
        static_assert(blocks_per_key == 2, "TCCR GGM expansion is binary only");
        memcpy(expanded_keys, keys, num_keys * sizeof(key_t));
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        static_assert(blocks_per_key == 2, "TCCR GGM expansion is binary only");
        if (!g_tccr_s || g_tccr_commit_leaves == 0)
            abort();

        if constexpr (S == secpar::s160 || S == secpar::s256)
        {
            gen_impl_aes<num_keys>(expanded_keys, iv, tweaks, counters, output);
            return;
        }

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        if constexpr (S == secpar::s384)
        {
            gen_impl_vaes384<num_keys>(expanded_keys, iv_bytes, tweaks, counters, output);
            return;
        }
#endif

        if constexpr (S == secpar::s512)
        {
            gen_impl_shacal512<num_keys>(expanded_keys, iv_bytes, tweaks, counters, output);
            return;
        }

        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        constexpr unsigned int lambda_bits = secpar_to_bits(S);

        for (size_t i = 0; i < num_keys; ++i)
        {
            const size_t id = static_cast<size_t>(tweaks[i]) + counters[i] + 1;
            const uint8_t* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);

            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();

            if (id < g_tccr_commit_leaves / 2)
            {
                tccr_hash_dispatch(parent, g_tccr_s, iv_bytes, left, lambda_bits);
                for (size_t b = 0; b < lambda_bytes; ++b)
                    right[b] = left[b] ^ parent[b];
            }
            else
            {
                tccr_hash_x0_x1_dispatch(parent, g_tccr_s, iv_bytes, left, right, lambda_bits);
            }
        }
    }

private:
    friend struct ref_tccr_prg<secpar::s384>;

    static void tccr512_build_key(const uint8_t* x_bar, const uint8_t* iv_bytes, uint8_t key[64])
    {
        static_assert(S == secpar::s512);

        memset(key, 0, 64);
        memcpy(key, x_bar + 31, 33);
        memcpy(key + 33, iv_bytes, 15);
    }

    static void tccr512_build_plaintext(uint8_t plaintext[32], const uint8_t* x_bar, uint8_t tag)
    {
        static_assert(S == secpar::s512);

        plaintext[0] = tag;
        memcpy(plaintext + 1, x_bar, 31);
    }

    static void tccr512_xor_ciphertext(uint8_t* out, const uint8_t* ciphertext,
                                       const uint8_t* plaintext)
    {
        for (size_t b = 0; b < 32; ++b)
            out[b] = ciphertext[b] ^ plaintext[b];
    }

    template <size_t num_keys>
    static void gen_impl_shacal512_inner_group(const expanded_key_t* expanded_keys,
                                               uint8_t x_bars[][secpar_to_bytes(secpar::s512)],
                                               const uint8_t* iv_bytes,
                                               block_t* output, size_t base, size_t active)
    {
        static_assert(S == secpar::s512);

        uint32_t schedules_storage[4][64];
        alignas(16) uint32_t wk_storage[4][16][4];
        uint8_t key_storage[4][64];
        [[maybe_unused]] const uint32_t* schedules[4];
        const uint32_t* wk_pairs[4];
        const uint8_t* keys[4];
        uint32_t* schedule_out[4];
        uint32_t* wk_out[4];
        alignas(16) uint8_t plaintext_storage[2][4][32];
        const uint8_t* plaintexts[4];
        uint8_t* ciphertexts[4];
        alignas(16) uint8_t ciphertext_storage[4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            plaintexts[lane] = plaintext_storage[0][0];
            ciphertexts[lane] = ciphertext_storage[0];
        }

        for (size_t lane = 0; lane < active; ++lane)
        {
            const size_t i = base + lane;
            tccr512_build_key(x_bars[i], iv_bytes, key_storage[lane]);
            tccr512_build_plaintext(plaintext_storage[0][lane], x_bars[i], 0);
            tccr512_build_plaintext(plaintext_storage[1][lane], x_bars[i], 1);
            keys[lane] = key_storage[lane];
            schedule_out[lane] = schedules_storage[lane];
            wk_out[lane] = &wk_storage[lane][0][0];
            schedules[lane] = schedules_storage[lane];
            wk_pairs[lane] = &wk_storage[lane][0][0];
            ciphertexts[lane] = ciphertext_storage[lane];
        }
        tccr_detail::shacal2_key_schedule_wk_x4_interleaved(keys, schedule_out, wk_out, active);

        for (uint8_t tag = 0; tag < 2; ++tag)
        {
            for (size_t lane = 0; lane < active; ++lane)
                plaintexts[lane] = plaintext_storage[tag][lane];
#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
            tccr_detail::shacal2_encrypt_wk_x4(wk_pairs, plaintexts, ciphertexts, active);
#else
            tccr_detail::shacal2_encrypt_scheduled_x4(schedules, plaintexts, ciphertexts, active);
#endif
            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t i = base + lane;
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                tccr512_xor_ciphertext(left + 32 * tag, ciphertext_storage[lane],
                                       plaintext_storage[tag][lane]);
            }
        }

        for (size_t lane = 0; lane < active; ++lane)
        {
            const size_t i = base + lane;
            output[2 * i + 1] = output[2 * i] ^ expanded_keys[i];
        }
    }

    template <size_t num_keys>
    static void gen_impl_shacal512_last_group(uint8_t x_bars[][secpar_to_bytes(secpar::s512)],
                                              const uint8_t* iv_bytes,
                                              block_t* output, size_t base, size_t active)
    {
        static_assert(S == secpar::s512);

        uint32_t schedules_storage[4][64];
        alignas(16) uint32_t wk_storage[4][16][4];
        uint8_t key_storage[4][64];
        [[maybe_unused]] const uint32_t* schedules[4];
        const uint32_t* wk_pairs[4];
        const uint8_t* keys[4];
        uint32_t* schedule_out[4];
        uint32_t* wk_out[4];
        alignas(16) uint8_t plaintext_storage[4][4][32];
        const uint8_t* plaintexts[4];
        uint8_t* ciphertexts[4];
        alignas(16) uint8_t ciphertext_storage[4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            plaintexts[lane] = plaintext_storage[0][0];
            ciphertexts[lane] = ciphertext_storage[0];
        }

        for (size_t lane = 0; lane < active; ++lane)
        {
            const size_t i = base + lane;
            alignas(16) uint8_t flipped[64];
            memcpy(flipped, x_bars[i], sizeof(flipped));
            flipped[0] ^= 1;

            tccr512_build_key(x_bars[i], iv_bytes, key_storage[lane]);
            tccr512_build_plaintext(plaintext_storage[0][lane], x_bars[i], 0);
            tccr512_build_plaintext(plaintext_storage[1][lane], flipped, 0);
            tccr512_build_plaintext(plaintext_storage[2][lane], x_bars[i], 1);
            tccr512_build_plaintext(plaintext_storage[3][lane], flipped, 1);
            keys[lane] = key_storage[lane];
            schedule_out[lane] = schedules_storage[lane];
            wk_out[lane] = &wk_storage[lane][0][0];
            schedules[lane] = schedules_storage[lane];
            wk_pairs[lane] = &wk_storage[lane][0][0];
            ciphertexts[lane] = ciphertext_storage[lane];
        }
        tccr_detail::shacal2_key_schedule_wk_x4_interleaved(keys, schedule_out, wk_out, active);

        for (size_t which = 0; which < 4; ++which)
        {
            for (size_t lane = 0; lane < active; ++lane)
                plaintexts[lane] = plaintext_storage[which][lane];

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
            tccr_detail::shacal2_encrypt_wk_x4(wk_pairs, plaintexts, ciphertexts, active);
#else
            tccr_detail::shacal2_encrypt_scheduled_x4(schedules, plaintexts, ciphertexts, active);
#endif
            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t i = base + lane;
                const size_t child = which & 1;
                const size_t tag = which >> 1;
                auto* out = reinterpret_cast<uint8_t*>(&output[2 * i + child]);
                tccr512_xor_ciphertext(out + 32 * tag, ciphertext_storage[lane],
                                       plaintext_storage[which][lane]);
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_shacal512(const expanded_key_t* expanded_keys, const uint8_t* iv_bytes,
                                   const tweak_t* tweaks, const count_t* counters,
                                   block_t* output)
    {
        static_assert(S == secpar::s512);

        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        alignas(64) uint8_t x_bars[num_keys][lambda_bytes];
        size_t ids[num_keys];
        for (size_t i = 0; i < num_keys; ++i)
        {
            ids[i] = static_cast<size_t>(tweaks[i]) + counters[i] + 1;
            const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            for (size_t b = 0; b < lambda_bytes; ++b)
                x_bars[i][b] = parent[b] ^ g_tccr_s[b];
            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();
        }

        size_t i = 0;
        while (i < num_keys)
        {
            const bool inner = ids[i] < g_tccr_commit_leaves / 2;
            size_t active = 1;
            while (i + active < num_keys && active < 4 &&
                   ((ids[i + active] < g_tccr_commit_leaves / 2) == inner))
                ++active;

            if (inner)
            {
                gen_impl_shacal512_inner_group<num_keys>(
                    expanded_keys, x_bars, iv_bytes, output, i, active);
            }
            else
            {
                gen_impl_shacal512_last_group<num_keys>(
                    x_bars, iv_bytes, output, i, active);
            }

            i += active;
        }
    }

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
    static __m128i tccr_loadu128(const uint8_t* x)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(x));
    }

    static __m512i tccr_pack4x128(__m128i x0, __m128i x1, __m128i x2, __m128i x3)
    {
        __m512i v = _mm512_castsi128_si512(x0);
        v = _mm512_inserti32x4(v, x1, 1);
        v = _mm512_inserti32x4(v, x2, 2);
        return _mm512_inserti32x4(v, x3, 3);
    }

    static __m512i tccr_splat128(__m128i x)
    {
        return tccr_pack4x128(x, x, x, x);
    }

    static __m512i tccr_load4x128(const uint8_t* x0, const uint8_t* x1, const uint8_t* x2,
                                  const uint8_t* x3)
    {
        return tccr_pack4x128(tccr_loadu128(x0), tccr_loadu128(x1), tccr_loadu128(x2),
                              tccr_loadu128(x3));
    }

    static void tccr_build_plain128(uint8_t plaintext[16], const uint8_t* x_bar, uint8_t tag)
    {
        plaintext[0] = tag;
        memcpy(plaintext + 1, x_bar, 15);
    }

    static void tccr_build_plain256(uint8_t plaintext[32], const uint8_t* x_bar, uint8_t tag)
    {
        plaintext[0] = tag;
        memcpy(plaintext + 1, x_bar, 31);
    }

    static void tccr_build_aes_key_bytes(uint8_t* key, const uint8_t* x_bar,
                                         const uint8_t* iv_bytes)
    {
        if constexpr (S == secpar::s160)
        {
            memset(key, 0, 24);
            memcpy(key, x_bar + 15, 5);
            memcpy(key + 5, iv_bytes, 15);
        }
        else
        {
            memset(key, 0, 32);
            memcpy(key, x_bar + 15, 17);
            memcpy(key + 17, iv_bytes, 15);
        }
    }

    static __m128i tccr_load_aes_key_lo(const uint8_t* key)
    {
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(key));
    }

    static __m128i tccr_load_aes_key_hi(const uint8_t* key)
    {
        if constexpr (S == secpar::s160)
            return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(key + 16));
        else
            return _mm_loadu_si128(reinterpret_cast<const __m128i*>(key + 16));
    }

    static __m128i tccr_make_plain128_xor(const uint8_t* parent, uint8_t tag)
    {
        const __m128i xb = _mm_xor_si128(tccr_loadu128(parent), tccr_loadu128(g_tccr_s));
        __m128i p = _mm_bslli_si128(xb, 1);
        return _mm_insert_epi8(p, tag, 0);
    }

    static __m128i tccr_make_aes256_key_lo_xor(const uint8_t* parent)
    {
        static_assert(S == secpar::s256);
        return _mm_xor_si128(tccr_loadu128(parent + 15), tccr_loadu128(g_tccr_s + 15));
    }

    static __m128i tccr_make_aes160_key_lo_xor(const uint8_t* parent, const uint8_t* iv_bytes)
    {
        static_assert(S == secpar::s160);
        const __m128i xr = _mm_xor_si128(_mm_maskz_loadu_epi8(0x001f, parent + 15),
                                         _mm_maskz_loadu_epi8(0x001f, g_tccr_s + 15));
        const __m128i iv = _mm_bslli_si128(_mm_maskz_loadu_epi8(0x07ff, iv_bytes), 5);
        return _mm_or_si128(xr, iv);
    }

    static __m128i tccr_make_aes160_key_hi(const uint8_t* iv_bytes)
    {
        static_assert(S == secpar::s160);
        return _mm_maskz_loadu_epi8(0x000f, iv_bytes + 11);
    }

    static __m128i tccr_make_aes256_key_hi_xor(const uint8_t* parent, const uint8_t* iv_bytes)
    {
        static_assert(S == secpar::s256);
        __m128i hi = _mm_bslli_si128(_mm_maskz_loadu_epi8(0x7fff, iv_bytes), 1);
        return _mm_insert_epi8(hi, parent[31] ^ g_tccr_s[31], 0);
    }

    static __m512i tccr_flip_plain_x0_mask()
    {
        return tccr_splat128(_mm_set_epi64x(0, 0x0000000000000100ULL));
    }

    static __mmask64 tccr_repeat_lane_mask16(uint16_t lane_mask)
    {
        return static_cast<__mmask64>(lane_mask) |
               (static_cast<__mmask64>(lane_mask) << 16) |
               (static_cast<__mmask64>(lane_mask) << 32) |
               (static_cast<__mmask64>(lane_mask) << 48);
    }

    static __m512i tccr_xor_prefix128_512(__m512i k)
    {
        __m512i f = _mm512_bslli_epi128(k, 4);
        k = _mm512_xor_si512(k, f);
        f = _mm512_bslli_epi128(f, 4);
        k = _mm512_xor_si512(k, f);
        f = _mm512_bslli_epi128(f, 4);
        return _mm512_xor_si512(k, f);
    }

    static __m512i tccr_aes_rot_sub_rcon512(__m512i k, uint8_t rcon)
    {
        const __m512i rt =
            tccr_splat128(_mm_set_epi32(0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d));
        return _mm512_aesenclast_epi128(_mm512_shuffle_epi8(k, rt), _mm512_set1_epi32(rcon));
    }

    static __m512i tccr_aes_subword_last512(__m512i k)
    {
        const __m512i ls =
            tccr_splat128(_mm_set_epi32(0x0f0e0d0c, 0x0f0e0d0c, 0x0f0e0d0c, 0x0f0e0d0c));
        return _mm512_aesenclast_epi128(_mm512_shuffle_epi8(k, ls), _mm512_setzero_si512());
    }

    static __m512i tccr_aes_rot_sub_rcon_word1_512(__m512i k, uint8_t rcon)
    {
        const __m512i rt =
            tccr_splat128(_mm_set_epi32(0x04070605, 0x04070605, 0x04070605, 0x04070605));
        return _mm512_aesenclast_epi128(_mm512_shuffle_epi8(k, rt), _mm512_set1_epi32(rcon));
    }

    static void tccr_aes192_assist512(__m512i& lo, __m512i& hi, uint8_t rcon)
    {
        lo = _mm512_xor_si512(tccr_xor_prefix128_512(lo),
                              tccr_aes_rot_sub_rcon_word1_512(hi, rcon));
        const __m512i rep = _mm512_shuffle_epi32(lo, static_cast<_MM_PERM_ENUM>(0xff));
        hi = _mm512_xor_si512(hi, _mm512_bslli_epi128(hi, 4));
        hi = _mm512_xor_si512(hi, rep);
    }

    static void tccr_aes256_assist_g512(__m512i& lo, __m512i hi, uint8_t rcon)
    {
        lo = _mm512_xor_si512(tccr_xor_prefix128_512(lo), tccr_aes_rot_sub_rcon512(hi, rcon));
    }

    static void tccr_aes256_assist_h512(__m512i lo, __m512i& hi)
    {
        hi = _mm512_xor_si512(tccr_xor_prefix128_512(hi), tccr_aes_subword_last512(lo));
    }

    static __m512i tccr_dup_low64_512(__m512i x)
    {
        return _mm512_shuffle_epi32(x, static_cast<_MM_PERM_ENUM>(_MM_SHUFFLE(1, 0, 1, 0)));
    }

    static __m512i tccr_dup_high64_512(__m512i x)
    {
        return _mm512_shuffle_epi32(x, static_cast<_MM_PERM_ENUM>(_MM_SHUFFLE(3, 2, 3, 2)));
    }

    static __m512i tccr_aes192_rk_from_hi_lo(__m512i hi, __m512i lo)
    {
        return _mm512_mask_blend_epi32(0xCCCC, hi, tccr_dup_low64_512(lo));
    }

    static __m512i tccr_aes192_rk_from_lo_hi(__m512i lo, __m512i hi)
    {
        return _mm512_mask_blend_epi32(0xCCCC, tccr_dup_high64_512(lo),
                                       tccr_dup_low64_512(hi));
    }

    static void tccr_rijndael256_rotate_rows_undo_512(__m512i& s0, __m512i& s1)
    {
        const __mmask64 bm = tccr_repeat_lane_mask16(0x8cce);
        const __m512i b0 = _mm512_mask_blend_epi8(bm, s0, s1);
        const __m512i b1 = _mm512_mask_blend_epi8(bm, s1, s0);
        const __m512i sh = tccr_splat128(_mm_setr_epi8(0, 1, 6, 7, 4, 5, 10, 11,
                                                       8, 9, 14, 15, 12, 13, 2, 3));
        s0 = _mm512_shuffle_epi8(b0, sh);
        s1 = _mm512_shuffle_epi8(b1, sh);
    }

    template <size_t num_states>
    static void tccr_aes256_encrypt4_ks_many(__m512i lo, __m512i hi,
                                             __m512i (&states)[num_states])
    {
        static const uint8_t rcon[7] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40};
        for (size_t s = 0; s < num_states; ++s)
        {
            states[s] = _mm512_xor_si512(states[s], lo);
            states[s] = _mm512_aesenc_epi128(states[s], hi);
        }
        for (int i = 0; i < 6; ++i)
        {
            tccr_aes256_assist_g512(lo, hi, rcon[i]);
            for (size_t s = 0; s < num_states; ++s)
                states[s] = _mm512_aesenc_epi128(states[s], lo);
            tccr_aes256_assist_h512(lo, hi);
            for (size_t s = 0; s < num_states; ++s)
                states[s] = _mm512_aesenc_epi128(states[s], hi);
        }
        tccr_aes256_assist_g512(lo, hi, rcon[6]);
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_aesenclast_epi128(states[s], lo);
    }

    template <size_t num_states>
    static void tccr_aes192_encrypt4_ks_many(__m512i lo, __m512i hi,
                                             __m512i (&states)[num_states])
    {
        static const uint8_t rcon[8] = {0x01, 0x02, 0x04, 0x08,
                                        0x10, 0x20, 0x40, 0x80};
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_xor_si512(states[s], lo);

        __m512i old_hi = hi;
        tccr_aes192_assist512(lo, hi, rcon[0]);
        for (size_t s = 0; s < num_states; ++s)
        {
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_hi_lo(old_hi, lo));
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_lo_hi(lo, hi));
        }

        tccr_aes192_assist512(lo, hi, rcon[1]);
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_aesenc_epi128(states[s], lo);
        old_hi = hi;
        tccr_aes192_assist512(lo, hi, rcon[2]);
        for (size_t s = 0; s < num_states; ++s)
        {
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_hi_lo(old_hi, lo));
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_lo_hi(lo, hi));
        }

        tccr_aes192_assist512(lo, hi, rcon[3]);
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_aesenc_epi128(states[s], lo);
        old_hi = hi;
        tccr_aes192_assist512(lo, hi, rcon[4]);
        for (size_t s = 0; s < num_states; ++s)
        {
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_hi_lo(old_hi, lo));
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_lo_hi(lo, hi));
        }

        tccr_aes192_assist512(lo, hi, rcon[5]);
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_aesenc_epi128(states[s], lo);
        old_hi = hi;
        tccr_aes192_assist512(lo, hi, rcon[6]);
        for (size_t s = 0; s < num_states; ++s)
        {
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_hi_lo(old_hi, lo));
            states[s] = _mm512_aesenc_epi128(states[s], tccr_aes192_rk_from_lo_hi(lo, hi));
        }

        tccr_aes192_assist512(lo, hi, rcon[7]);
        for (size_t s = 0; s < num_states; ++s)
            states[s] = _mm512_aesenclast_epi128(states[s], lo);
    }

    template <size_t num_states>
    static void tccr_rijndael256_encrypt4_ks_many(__m512i lo, __m512i hi,
                                                  __m512i (&s0)[num_states],
                                                  __m512i (&s1)[num_states])
    {
        static const uint8_t rcon[14] = {
            0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
            0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d,
        };

        for (size_t g = 0; g < num_states; ++g)
        {
            s0[g] = _mm512_xor_si512(s0[g], lo);
            s1[g] = _mm512_xor_si512(s1[g], hi);
        }

        for (int r = 1; r < 14; ++r)
        {
            tccr_aes256_assist_g512(lo, hi, rcon[r - 1]);
            tccr_aes256_assist_h512(lo, hi);
            for (size_t g = 0; g < num_states; ++g)
            {
                tccr_rijndael256_rotate_rows_undo_512(s0[g], s1[g]);
                s0[g] = _mm512_aesenc_epi128(s0[g], lo);
                s1[g] = _mm512_aesenc_epi128(s1[g], hi);
            }
        }

        tccr_aes256_assist_g512(lo, hi, rcon[13]);
        tccr_aes256_assist_h512(lo, hi);
        for (size_t g = 0; g < num_states; ++g)
        {
            tccr_rijndael256_rotate_rows_undo_512(s0[g], s1[g]);
            s0[g] = _mm512_aesenclast_epi128(s0[g], lo);
            s1[g] = _mm512_aesenclast_epi128(s1[g], hi);
        }
    }

    static void tccr_build_384_key_bytes(uint8_t key[32], const uint8_t* x_bar,
                                         const uint8_t* iv_bytes)
    {
        memcpy(key, x_bar + 31, 17);
        memcpy(key + 17, iv_bytes, 15);
    }

    static void tccr_hash384_ctr4_same_key(const uint8_t* key, const uint8_t* iv_bytes,
                                           uint32_t tweak, size_t block_index, size_t active,
                                           uint8_t out[4][48])
    {
        static_assert(S == secpar::s384);

        alignas(64) uint8_t x_bars[4][48];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            const size_t src_lane = lane < active ? lane : 0;
            memcpy(x_bars[lane], key, 48);
            x_bars[lane][0] ^=
                static_cast<uint8_t>(tweak + block_index + src_lane);
        }

        alignas(64) uint8_t key_bytes[4][32];
        alignas(64) uint8_t plaintexts[2][4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            tccr_build_384_key_bytes(key_bytes[lane], x_bars[lane], iv_bytes);
            tccr_build_plain256(plaintexts[0][lane], x_bars[lane], 0);
            tccr_build_plain256(plaintexts[1][lane], x_bars[lane], 1);
        }

        const __m512i lo = tccr_pack4x128(
            tccr_loadu128(key_bytes[0]), tccr_loadu128(key_bytes[1]),
            tccr_loadu128(key_bytes[2]), tccr_loadu128(key_bytes[3]));
        const __m512i hi = tccr_pack4x128(
            tccr_loadu128(key_bytes[0] + 16), tccr_loadu128(key_bytes[1] + 16),
            tccr_loadu128(key_bytes[2] + 16), tccr_loadu128(key_bytes[3] + 16));

        __m512i c0[2] = {
            tccr_load4x128(plaintexts[0][0], plaintexts[0][1], plaintexts[0][2],
                           plaintexts[0][3]),
            tccr_load4x128(plaintexts[1][0], plaintexts[1][1], plaintexts[1][2],
                           plaintexts[1][3]),
        };
        __m512i c1[2] = {
            tccr_load4x128(plaintexts[0][0] + 16, plaintexts[0][1] + 16,
                           plaintexts[0][2] + 16, plaintexts[0][3] + 16),
            tccr_load4x128(plaintexts[1][0] + 16, plaintexts[1][1] + 16,
                           plaintexts[1][2] + 16, plaintexts[1][3] + 16),
        };
        tccr_rijndael256_encrypt4_ks_many(lo, hi, c0, c1);

        alignas(64) uint8_t enc_lo[2][4][16];
        alignas(64) uint8_t enc_hi[2][4][16];
        for (size_t tag = 0; tag < 2; ++tag)
        {
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_lo[tag]), c0[tag]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_hi[tag]), c1[tag]);
        }

        for (size_t lane = 0; lane < active; ++lane)
        {
            for (size_t b = 0; b < 16; ++b)
            {
                out[lane][b] = enc_lo[0][lane][b] ^ plaintexts[0][lane][b];
                out[lane][16 + b] = enc_hi[0][lane][b] ^ plaintexts[0][lane][16 + b];
                out[lane][32 + b] = enc_lo[1][lane][b] ^ plaintexts[1][lane][b];
            }
        }
    }

    static void tccr_hash384_ctr4_keys(const uint8_t* const keys[4], const uint8_t* iv_bytes,
                                       uint32_t tweak, size_t block_index, size_t active,
                                       uint8_t out[4][48])
    {
        static_assert(S == secpar::s384);

        alignas(64) uint8_t x_bars[4][48];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            const size_t src_lane = lane < active ? lane : 0;
            memcpy(x_bars[lane], keys[src_lane], 48);
            x_bars[lane][0] ^= static_cast<uint8_t>(tweak + block_index);
        }

        alignas(64) uint8_t key_bytes[4][32];
        alignas(64) uint8_t plaintexts[2][4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            tccr_build_384_key_bytes(key_bytes[lane], x_bars[lane], iv_bytes);
            tccr_build_plain256(plaintexts[0][lane], x_bars[lane], 0);
            tccr_build_plain256(plaintexts[1][lane], x_bars[lane], 1);
        }

        const __m512i lo = tccr_pack4x128(
            tccr_loadu128(key_bytes[0]), tccr_loadu128(key_bytes[1]),
            tccr_loadu128(key_bytes[2]), tccr_loadu128(key_bytes[3]));
        const __m512i hi = tccr_pack4x128(
            tccr_loadu128(key_bytes[0] + 16), tccr_loadu128(key_bytes[1] + 16),
            tccr_loadu128(key_bytes[2] + 16), tccr_loadu128(key_bytes[3] + 16));

        __m512i c0[2] = {
            tccr_load4x128(plaintexts[0][0], plaintexts[0][1], plaintexts[0][2],
                           plaintexts[0][3]),
            tccr_load4x128(plaintexts[1][0], plaintexts[1][1], plaintexts[1][2],
                           plaintexts[1][3]),
        };
        __m512i c1[2] = {
            tccr_load4x128(plaintexts[0][0] + 16, plaintexts[0][1] + 16,
                           plaintexts[0][2] + 16, plaintexts[0][3] + 16),
            tccr_load4x128(plaintexts[1][0] + 16, plaintexts[1][1] + 16,
                           plaintexts[1][2] + 16, plaintexts[1][3] + 16),
        };
        tccr_rijndael256_encrypt4_ks_many(lo, hi, c0, c1);

        alignas(64) uint8_t enc_lo[2][4][16];
        alignas(64) uint8_t enc_hi[2][4][16];
        for (size_t tag = 0; tag < 2; ++tag)
        {
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_lo[tag]), c0[tag]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_hi[tag]), c1[tag]);
        }

        for (size_t lane = 0; lane < active; ++lane)
        {
            for (size_t b = 0; b < 16; ++b)
            {
                out[lane][b] = enc_lo[0][lane][b] ^ plaintexts[0][lane][b];
                out[lane][16 + b] = enc_hi[0][lane][b] ^ plaintexts[0][lane][16 + b];
                out[lane][32 + b] = enc_lo[1][lane][b] ^ plaintexts[1][lane][b];
            }
        }
    }

    static void fill_tccr384_ctr_from_offset_vaes(const uint8_t* key, const uint8_t* iv,
                                                  uint32_t tweak, size_t start_byte,
                                                  uint8_t* out, size_t out_bytes)
    {
        static_assert(S == secpar::s384);

        constexpr size_t csp_bytes = secpar_to_bytes(S);
        size_t block_index = start_byte / csp_bytes;
        size_t block_offset = start_byte % csp_bytes;
        size_t written = 0;

        while (written < out_bytes)
        {
            alignas(64) uint8_t blocks[4][48];
            const size_t required_blocks =
                (block_offset + out_bytes - written + csp_bytes - 1) / csp_bytes;
            const size_t active = std::min<size_t>(4, required_blocks);
            tccr_hash384_ctr4_same_key(key, iv, tweak, block_index, active, blocks);

            for (size_t lane = 0; lane < active && written < out_bytes; ++lane)
            {
                const size_t take = std::min(csp_bytes - block_offset, out_bytes - written);
                memcpy(out + written, blocks[lane] + block_offset, take);
                written += take;
                ++block_index;
                block_offset = 0;
            }
        }
    }

    static void gen_impl_vaes384_inner4(const expanded_key_t* expanded_keys,
                                        uint8_t x_bars[][secpar_to_bytes(secpar::s384)],
                                        const uint8_t* iv_bytes, block_t* output, size_t base)
    {
        static_assert(S == secpar::s384);

        alignas(64) uint8_t key_bytes[4][32];
        alignas(64) uint8_t plaintexts[2][4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            tccr_build_384_key_bytes(key_bytes[lane], x_bars[base + lane], iv_bytes);
            tccr_build_plain256(plaintexts[0][lane], x_bars[base + lane], 0);
            tccr_build_plain256(plaintexts[1][lane], x_bars[base + lane], 1);
        }

        const __m512i lo = tccr_pack4x128(
            tccr_loadu128(key_bytes[0]), tccr_loadu128(key_bytes[1]),
            tccr_loadu128(key_bytes[2]), tccr_loadu128(key_bytes[3]));
        const __m512i hi = tccr_pack4x128(
            tccr_loadu128(key_bytes[0] + 16), tccr_loadu128(key_bytes[1] + 16),
            tccr_loadu128(key_bytes[2] + 16), tccr_loadu128(key_bytes[3] + 16));
        __m512i c0[2] = {
            tccr_load4x128(plaintexts[0][0], plaintexts[0][1], plaintexts[0][2],
                           plaintexts[0][3]),
            tccr_load4x128(plaintexts[1][0], plaintexts[1][1], plaintexts[1][2],
                           plaintexts[1][3]),
        };
        __m512i c1[2] = {
            tccr_load4x128(plaintexts[0][0] + 16, plaintexts[0][1] + 16,
                           plaintexts[0][2] + 16, plaintexts[0][3] + 16),
            tccr_load4x128(plaintexts[1][0] + 16, plaintexts[1][1] + 16,
                           plaintexts[1][2] + 16, plaintexts[1][3] + 16),
        };
        tccr_rijndael256_encrypt4_ks_many(lo, hi, c0, c1);

        alignas(64) uint8_t enc_lo[2][4][16];
        alignas(64) uint8_t enc_hi[2][4][16];
        for (size_t tag = 0; tag < 2; ++tag)
        {
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_lo[tag]), c0[tag]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_hi[tag]), c1[tag]);
        }
        for (size_t lane = 0; lane < 4; ++lane)
        {
            const size_t i = base + lane;
            output[2 * i] = block_t::set_zero();
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            for (size_t b = 0; b < 16; ++b)
            {
                left[b] = enc_lo[0][lane][b] ^ plaintexts[0][lane][b];
                left[16 + b] = enc_hi[0][lane][b] ^ plaintexts[0][lane][16 + b];
                left[32 + b] = enc_lo[1][lane][b] ^ plaintexts[1][lane][b];
            }
            output[2 * i + 1] = output[2 * i] ^ expanded_keys[i];
        }
    }

    static void gen_impl_vaes384_last2(uint8_t x_bars[][secpar_to_bytes(secpar::s384)],
                                       const uint8_t* iv_bytes, block_t* output, size_t base)
    {
        static_assert(S == secpar::s384);

        alignas(64) uint8_t key_bytes[4][32];
        alignas(64) uint8_t plaintexts[4][4][32];
        for (size_t lane = 0; lane < 4; ++lane)
        {
            const size_t i = base + (lane < 2 ? lane : 0);
            alignas(32) uint8_t flipped[secpar_to_bytes(secpar::s384)];
            memcpy(flipped, x_bars[i], sizeof(flipped));
            flipped[0] ^= 1;

            tccr_build_384_key_bytes(key_bytes[lane], x_bars[i], iv_bytes);
            tccr_build_plain256(plaintexts[0][lane], x_bars[i], 0);
            tccr_build_plain256(plaintexts[1][lane], flipped, 0);
            tccr_build_plain256(plaintexts[2][lane], x_bars[i], 1);
            tccr_build_plain256(plaintexts[3][lane], flipped, 1);
        }

        const __m512i lo = tccr_pack4x128(
            tccr_loadu128(key_bytes[0]), tccr_loadu128(key_bytes[1]),
            tccr_loadu128(key_bytes[2]), tccr_loadu128(key_bytes[3]));
        const __m512i hi = tccr_pack4x128(
            tccr_loadu128(key_bytes[0] + 16), tccr_loadu128(key_bytes[1] + 16),
            tccr_loadu128(key_bytes[2] + 16), tccr_loadu128(key_bytes[3] + 16));
        __m512i c0[4] = {
            tccr_load4x128(plaintexts[0][0], plaintexts[0][1], plaintexts[0][2],
                           plaintexts[0][3]),
            tccr_load4x128(plaintexts[1][0], plaintexts[1][1], plaintexts[1][2],
                           plaintexts[1][3]),
            tccr_load4x128(plaintexts[2][0], plaintexts[2][1], plaintexts[2][2],
                           plaintexts[2][3]),
            tccr_load4x128(plaintexts[3][0], plaintexts[3][1], plaintexts[3][2],
                           plaintexts[3][3]),
        };
        __m512i c1[4] = {
            tccr_load4x128(plaintexts[0][0] + 16, plaintexts[0][1] + 16,
                           plaintexts[0][2] + 16, plaintexts[0][3] + 16),
            tccr_load4x128(plaintexts[1][0] + 16, plaintexts[1][1] + 16,
                           plaintexts[1][2] + 16, plaintexts[1][3] + 16),
            tccr_load4x128(plaintexts[2][0] + 16, plaintexts[2][1] + 16,
                           plaintexts[2][2] + 16, plaintexts[2][3] + 16),
            tccr_load4x128(plaintexts[3][0] + 16, plaintexts[3][1] + 16,
                           plaintexts[3][2] + 16, plaintexts[3][3] + 16),
        };
        tccr_rijndael256_encrypt4_ks_many(lo, hi, c0, c1);

        alignas(64) uint8_t enc_lo[4][4][16];
        alignas(64) uint8_t enc_hi[4][4][16];
        for (size_t st = 0; st < 4; ++st)
        {
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_lo[st]), c0[st]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(enc_hi[st]), c1[st]);
        }
        for (size_t lane = 0; lane < 2; ++lane)
        {
            const size_t i = base + lane;
            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);
            for (size_t b = 0; b < 16; ++b)
            {
                left[b] = enc_lo[0][lane][b] ^ plaintexts[0][lane][b];
                left[16 + b] = enc_hi[0][lane][b] ^ plaintexts[0][lane][16 + b];
                left[32 + b] = enc_lo[2][lane][b] ^ plaintexts[2][lane][b];
                right[b] = enc_lo[1][lane][b] ^ plaintexts[1][lane][b];
                right[16 + b] = enc_hi[1][lane][b] ^ plaintexts[1][lane][16 + b];
                right[32 + b] = enc_lo[3][lane][b] ^ plaintexts[3][lane][b];
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_vaes160_inner(const expanded_key_t* expanded_keys,
                                       const uint8_t* iv_bytes, block_t* output)
    {
        static_assert(S == secpar::s160);
        static_assert(num_keys == 8);

        for (size_t base = 0; base < num_keys; base += 4)
        {
            const auto* p0 = reinterpret_cast<const uint8_t*>(&expanded_keys[base]);
            const auto* p1 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 1]);
            const auto* p2 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 2]);
            const auto* p3 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 3]);
            const __m512i lo = tccr_pack4x128(
                tccr_make_aes160_key_lo_xor(p0, iv_bytes),
                tccr_make_aes160_key_lo_xor(p1, iv_bytes),
                tccr_make_aes160_key_lo_xor(p2, iv_bytes),
                tccr_make_aes160_key_lo_xor(p3, iv_bytes));
            const __m512i hi = tccr_pack4x128(
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes));
            const __m512i plain0 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 0),
                tccr_make_plain128_xor(p1, 0),
                tccr_make_plain128_xor(p2, 0),
                tccr_make_plain128_xor(p3, 0));
            const __m512i plain1 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 1),
                tccr_make_plain128_xor(p1, 1),
                tccr_make_plain128_xor(p2, 1),
                tccr_make_plain128_xor(p3, 1));
            __m512i states[2] = {
                plain0,
                plain1,
            };
            tccr_aes192_encrypt4_ks_many(lo, hi, states);

            alignas(64) uint8_t encrypted0[4][16];
            alignas(64) uint8_t encrypted1[4][16];
            alignas(64) uint8_t plaintext0[4][16];
            alignas(64) uint8_t plaintext1[4][16];
            _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted0), states[0]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted1), states[1]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintext0), plain0);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintext1), plain1);
            for (size_t lane = 0; lane < 4; ++lane)
            {
                const size_t i = base + lane;
                output[2 * i] = block_t::set_zero();
                output[2 * i + 1] = block_t::set_zero();
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                for (size_t b = 0; b < 16; ++b)
                    left[b] = encrypted0[lane][b] ^ plaintext0[lane][b];
                for (size_t b = 0; b < secpar_to_bytes(S) - 16; ++b)
                    left[16 + b] = encrypted1[lane][b] ^ plaintext1[lane][b];
                output[2 * i + 1] = output[2 * i] ^ expanded_keys[i];
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_vaes160_last(const expanded_key_t* expanded_keys,
                                      const uint8_t* iv_bytes, block_t* output)
    {
        static_assert(S == secpar::s160);
        static_assert(num_keys == 8);

        for (size_t base = 0; base < num_keys; base += 4)
        {
            const auto* p0 = reinterpret_cast<const uint8_t*>(&expanded_keys[base]);
            const auto* p1 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 1]);
            const auto* p2 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 2]);
            const auto* p3 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 3]);
            const __m512i lo = tccr_pack4x128(
                tccr_make_aes160_key_lo_xor(p0, iv_bytes),
                tccr_make_aes160_key_lo_xor(p1, iv_bytes),
                tccr_make_aes160_key_lo_xor(p2, iv_bytes),
                tccr_make_aes160_key_lo_xor(p3, iv_bytes));
            const __m512i hi = tccr_pack4x128(
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes),
                tccr_make_aes160_key_hi(iv_bytes));
            const __m512i plain0 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 0),
                tccr_make_plain128_xor(p1, 0),
                tccr_make_plain128_xor(p2, 0),
                tccr_make_plain128_xor(p3, 0));
            const __m512i plain1 = _mm512_xor_si512(plain0, tccr_flip_plain_x0_mask());
            const __m512i plain2 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 1),
                tccr_make_plain128_xor(p1, 1),
                tccr_make_plain128_xor(p2, 1),
                tccr_make_plain128_xor(p3, 1));
            const __m512i plain3 = _mm512_xor_si512(plain2, tccr_flip_plain_x0_mask());
            __m512i states[4] = {
                plain0,
                plain1,
                plain2,
                plain3,
            };
            tccr_aes192_encrypt4_ks_many(lo, hi, states);

            alignas(64) uint8_t encrypted[4][4][16];
            alignas(64) uint8_t plaintexts[4][4][16];
            for (size_t st = 0; st < 4; ++st)
                _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted[st]), states[st]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[0]), plain0);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[1]), plain1);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[2]), plain2);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[3]), plain3);
            for (size_t lane = 0; lane < 4; ++lane)
            {
                const size_t i = base + lane;
                output[2 * i] = block_t::set_zero();
                output[2 * i + 1] = block_t::set_zero();
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);
                for (size_t b = 0; b < 16; ++b)
                {
                    left[b] = encrypted[0][lane][b] ^ plaintexts[0][lane][b];
                    right[b] = encrypted[1][lane][b] ^ plaintexts[1][lane][b];
                }
                for (size_t b = 0; b < secpar_to_bytes(S) - 16; ++b)
                {
                    left[16 + b] = encrypted[2][lane][b] ^ plaintexts[2][lane][b];
                    right[16 + b] = encrypted[3][lane][b] ^ plaintexts[3][lane][b];
                }
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_vaes256_inner(const expanded_key_t* expanded_keys,
                                       const uint8_t* iv_bytes, block_t* output)
    {
        static_assert(S == secpar::s256);
        static_assert(num_keys == 8);

        for (size_t base = 0; base < num_keys; base += 4)
        {
            const auto* p0 = reinterpret_cast<const uint8_t*>(&expanded_keys[base]);
            const auto* p1 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 1]);
            const auto* p2 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 2]);
            const auto* p3 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 3]);
            const __m512i lo = tccr_pack4x128(
                tccr_make_aes256_key_lo_xor(p0),
                tccr_make_aes256_key_lo_xor(p1),
                tccr_make_aes256_key_lo_xor(p2),
                tccr_make_aes256_key_lo_xor(p3));
            const __m512i hi = tccr_pack4x128(
                tccr_make_aes256_key_hi_xor(p0, iv_bytes),
                tccr_make_aes256_key_hi_xor(p1, iv_bytes),
                tccr_make_aes256_key_hi_xor(p2, iv_bytes),
                tccr_make_aes256_key_hi_xor(p3, iv_bytes));
            const __m512i plain0 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 0),
                tccr_make_plain128_xor(p1, 0),
                tccr_make_plain128_xor(p2, 0),
                tccr_make_plain128_xor(p3, 0));
            const __m512i plain1 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 1),
                tccr_make_plain128_xor(p1, 1),
                tccr_make_plain128_xor(p2, 1),
                tccr_make_plain128_xor(p3, 1));
            __m512i states[2] = {
                plain0,
                plain1,
            };
            tccr_aes256_encrypt4_ks_many(lo, hi, states);

            alignas(64) uint8_t encrypted0[4][16];
            alignas(64) uint8_t encrypted1[4][16];
            alignas(64) uint8_t plaintext0[4][16];
            alignas(64) uint8_t plaintext1[4][16];
            _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted0), states[0]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted1), states[1]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintext0), plain0);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintext1), plain1);
            for (size_t lane = 0; lane < 4; ++lane)
            {
                const size_t i = base + lane;
                output[2 * i] = block_t::set_zero();
                output[2 * i + 1] = block_t::set_zero();
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                for (size_t b = 0; b < 16; ++b)
                {
                    left[b] = encrypted0[lane][b] ^ plaintext0[lane][b];
                    left[16 + b] = encrypted1[lane][b] ^ plaintext1[lane][b];
                }
                output[2 * i + 1] = output[2 * i] ^ expanded_keys[i];
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_vaes256_last(const expanded_key_t* expanded_keys,
                                      const uint8_t* iv_bytes, block_t* output)
    {
        static_assert(S == secpar::s256);
        static_assert(num_keys == 8);

        for (size_t base = 0; base < num_keys; base += 4)
        {
            const auto* p0 = reinterpret_cast<const uint8_t*>(&expanded_keys[base]);
            const auto* p1 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 1]);
            const auto* p2 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 2]);
            const auto* p3 = reinterpret_cast<const uint8_t*>(&expanded_keys[base + 3]);
            const __m512i lo = tccr_pack4x128(
                tccr_make_aes256_key_lo_xor(p0),
                tccr_make_aes256_key_lo_xor(p1),
                tccr_make_aes256_key_lo_xor(p2),
                tccr_make_aes256_key_lo_xor(p3));
            const __m512i hi = tccr_pack4x128(
                tccr_make_aes256_key_hi_xor(p0, iv_bytes),
                tccr_make_aes256_key_hi_xor(p1, iv_bytes),
                tccr_make_aes256_key_hi_xor(p2, iv_bytes),
                tccr_make_aes256_key_hi_xor(p3, iv_bytes));
            const __m512i plain0 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 0),
                tccr_make_plain128_xor(p1, 0),
                tccr_make_plain128_xor(p2, 0),
                tccr_make_plain128_xor(p3, 0));
            const __m512i plain1 = _mm512_xor_si512(plain0, tccr_flip_plain_x0_mask());
            const __m512i plain2 = tccr_pack4x128(
                tccr_make_plain128_xor(p0, 1),
                tccr_make_plain128_xor(p1, 1),
                tccr_make_plain128_xor(p2, 1),
                tccr_make_plain128_xor(p3, 1));
            const __m512i plain3 = _mm512_xor_si512(plain2, tccr_flip_plain_x0_mask());
            __m512i states[4] = {
                plain0,
                plain1,
                plain2,
                plain3,
            };
            tccr_aes256_encrypt4_ks_many(lo, hi, states);

            alignas(64) uint8_t encrypted[4][4][16];
            alignas(64) uint8_t plaintexts[4][4][16];
            for (size_t st = 0; st < 4; ++st)
                _mm512_store_si512(reinterpret_cast<__m512i*>(encrypted[st]), states[st]);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[0]), plain0);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[1]), plain1);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[2]), plain2);
            _mm512_store_si512(reinterpret_cast<__m512i*>(plaintexts[3]), plain3);
            for (size_t lane = 0; lane < 4; ++lane)
            {
                const size_t i = base + lane;
                output[2 * i] = block_t::set_zero();
                output[2 * i + 1] = block_t::set_zero();
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);
                for (size_t b = 0; b < 16; ++b)
                {
                    left[b] = encrypted[0][lane][b] ^ plaintexts[0][lane][b];
                    right[b] = encrypted[1][lane][b] ^ plaintexts[1][lane][b];
                    left[16 + b] = encrypted[2][lane][b] ^ plaintexts[2][lane][b];
                    right[16 + b] = encrypted[3][lane][b] ^ plaintexts[3][lane][b];
                }
            }
        }
    }

    template <size_t num_keys>
    static void gen_impl_vaes384(const expanded_key_t* expanded_keys, const uint8_t* iv_bytes,
                                 const tweak_t* tweaks, const count_t* counters,
                                 block_t* output)
    {
        static_assert(S == secpar::s384);

        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        alignas(64) uint8_t x_bars[num_keys][lambda_bytes];
        for (size_t i = 0; i < num_keys; ++i)
        {
            const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            for (size_t b = 0; b < lambda_bytes; ++b)
                x_bars[i][b] = parent[b] ^ g_tccr_s[b];
        }

        size_t i = 0;
        while (i < num_keys)
        {
            const size_t id0 = static_cast<size_t>(tweaks[i]) + counters[i] + 1;
            const bool inner0 = id0 < g_tccr_commit_leaves / 2;
            if (inner0)
            {
                if (i + 3 < num_keys)
                {
                    const size_t id1 = static_cast<size_t>(tweaks[i + 1]) + counters[i + 1] + 1;
                    const size_t id2 = static_cast<size_t>(tweaks[i + 2]) + counters[i + 2] + 1;
                    const size_t id3 = static_cast<size_t>(tweaks[i + 3]) + counters[i + 3] + 1;
                    if (id1 < g_tccr_commit_leaves / 2 && id2 < g_tccr_commit_leaves / 2 &&
                        id3 < g_tccr_commit_leaves / 2)
                    {
                        gen_impl_vaes384_inner4(expanded_keys, x_bars, iv_bytes, output, i);
                        i += 4;
                        continue;
                    }
                }

                size_t active = 1;
                if (i + 1 < num_keys)
                {
                    const size_t id1 = static_cast<size_t>(tweaks[i + 1]) + counters[i + 1] + 1;
                    active += id1 < g_tccr_commit_leaves / 2 ? 1 : 0;
                }
                for (size_t lane = 0; lane < active; ++lane)
                {
                    const size_t j = i + lane;
                    const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[j]);
                    auto* left = reinterpret_cast<uint8_t*>(&output[2 * j]);
                    output[2 * j] = block_t::set_zero();
                    output[2 * j + 1] = block_t::set_zero();
                    tccr_hash_384(parent, g_tccr_s, iv_bytes, left);
                    output[2 * j + 1] = output[2 * j] ^ expanded_keys[j];
                }
                i += active;
            }
            else
            {
                if (i + 1 < num_keys)
                {
                    const size_t id1 = static_cast<size_t>(tweaks[i + 1]) + counters[i + 1] + 1;
                    if (id1 >= g_tccr_commit_leaves / 2)
                    {
                        gen_impl_vaes384_last2(x_bars, iv_bytes, output, i);
                        i += 2;
                        continue;
                    }
                }

                const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
                auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
                auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);
                output[2 * i] = block_t::set_zero();
                output[2 * i + 1] = block_t::set_zero();
                tccr_hash_x0_x1_384(parent, g_tccr_s, iv_bytes, left, right);
                ++i;
            }
        }
    }
#endif

    static constexpr secpar aes_secpar_v = S == secpar::s160 ? secpar::s192 : secpar::s256;
    using aes_key_t = block_secpar<aes_secpar_v>;

    static void build_tccr_aes_key(const uint8_t* x_bar, const uint8_t* iv_bytes, aes_key_t* key)
    {
        *key = aes_key_t::set_zero();
        auto* key_bytes = reinterpret_cast<uint8_t*>(key);
        if constexpr (S == secpar::s160)
        {
            memcpy(key_bytes, x_bar + 15, 5);
            memcpy(key_bytes + 5, iv_bytes, 15);
        }
        else
        {
            memcpy(key_bytes, x_bar + 15, 17);
            memcpy(key_bytes + 17, iv_bytes, 15);
        }
    }

    static void build_tccr_aes_plaintext(uint8_t plaintext[16], const uint8_t* x_bar, uint8_t tag)
    {
        plaintext[0] = tag;
        memcpy(plaintext + 1, x_bar, 15);
    }

    static void copy_tccr_block(uint8_t* out, const block128& encrypted, const uint8_t* plaintext,
                                size_t bytes)
    {
        const auto* encrypted_bytes = reinterpret_cast<const uint8_t*>(&encrypted);
        for (size_t b = 0; b < bytes; ++b)
            out[b] = encrypted_bytes[b] ^ plaintext[b];
    }

    template <size_t num_keys>
    static void gen_impl_aes(const expanded_key_t* expanded_keys, const iv_t& iv,
                             const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        if constexpr ((S == secpar::s160 || S == secpar::s256) && num_keys > 8)
        {
            constexpr size_t first_keys = 8;
            gen_impl_aes<first_keys>(expanded_keys, iv, tweaks, counters, output);
            gen_impl_aes<num_keys - first_keys>(
                expanded_keys + first_keys, iv, tweaks + first_keys, counters + first_keys,
                output + 2 * first_keys);
            return;
        }
#endif

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        if constexpr (!((S == secpar::s160 || S == secpar::s256) && num_keys == 8))
#endif
        {
            if constexpr (num_keys > 4)
            {
                constexpr size_t first_keys = 4;
                gen_impl_aes<first_keys>(expanded_keys, iv, tweaks, counters, output);
                gen_impl_aes<num_keys - first_keys>(
                    expanded_keys + first_keys, iv, tweaks + first_keys, counters + first_keys,
                    output + 2 * first_keys);
                return;
            }
        }

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

        size_t ids[num_keys];
        bool all_inner = true;
        bool all_last = true;
        for (size_t i = 0; i < num_keys; ++i)
        {
            ids[i] = static_cast<size_t>(tweaks[i]) + counters[i] + 1;
            all_inner &= ids[i] < g_tccr_commit_leaves / 2;
            all_last &= ids[i] >= g_tccr_commit_leaves / 2;
        }

        if (all_inner)
        {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
            if constexpr (S == secpar::s160 && num_keys == 8)
            {
                gen_impl_vaes160_inner<num_keys>(expanded_keys, iv_bytes, output);
                return;
            }
            if constexpr (S == secpar::s256 && num_keys == 8)
            {
                gen_impl_vaes256_inner<num_keys>(expanded_keys, iv_bytes, output);
                return;
            }
#endif
            gen_impl_aes_inner<num_keys>(expanded_keys, iv_bytes, output);
        }
        else if (all_last)
        {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
            if constexpr (S == secpar::s160 && num_keys == 8)
            {
                gen_impl_vaes160_last<num_keys>(expanded_keys, iv_bytes, output);
                return;
            }
            if constexpr (S == secpar::s256 && num_keys == 8)
            {
                gen_impl_vaes256_last<num_keys>(expanded_keys, iv_bytes, output);
                return;
            }
#endif
            gen_impl_aes_last<num_keys>(expanded_keys, iv_bytes, output);
        }
        else
        {
            gen_impl_scalar_mixed<num_keys>(expanded_keys, iv_bytes, ids, output);
        }
    }

    template <size_t num_keys>
    static void gen_impl_aes_inner(const expanded_key_t* expanded_keys, const uint8_t* iv_bytes,
                                   block_t* output)
    {
        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        aes_key_t aes_keys[num_keys];
        aes_round_keys<aes_secpar_v> round_keys[num_keys];
        block128 states[2 * num_keys];
        uint8_t plaintexts[2 * num_keys][16];

        for (size_t i = 0; i < num_keys; ++i)
        {
            uint8_t x_bar[lambda_bytes];
            const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            for (size_t b = 0; b < lambda_bytes; ++b)
                x_bar[b] = parent[b] ^ g_tccr_s[b];

            build_tccr_aes_plaintext(plaintexts[2 * i], x_bar, 0);
            build_tccr_aes_plaintext(plaintexts[2 * i + 1], x_bar, 1);
            memcpy(&states[2 * i], plaintexts[2 * i], 16);
            memcpy(&states[2 * i + 1], plaintexts[2 * i + 1], 16);
            build_tccr_aes_key(x_bar, iv_bytes, &aes_keys[i]);
        }

        aes_keygen_ecb<aes_secpar_v, num_keys, 2>(aes_keys, round_keys, states);

        for (size_t i = 0; i < num_keys; ++i)
        {
            const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);

            copy_tccr_block(left, states[2 * i], plaintexts[2 * i], 16);
            copy_tccr_block(left + 16, states[2 * i + 1], plaintexts[2 * i + 1],
                            lambda_bytes - 16);
            for (size_t b = 0; b < lambda_bytes; ++b)
                right[b] = left[b] ^ parent[b];
        }
    }

    template <size_t num_keys>
    static void gen_impl_aes_last(const expanded_key_t* expanded_keys, const uint8_t* iv_bytes,
                                  block_t* output)
    {
        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        aes_key_t aes_keys[num_keys];
        aes_round_keys<aes_secpar_v> round_keys[num_keys];
        block128 states[4 * num_keys];
        uint8_t plaintexts[4 * num_keys][16];

        for (size_t i = 0; i < num_keys; ++i)
        {
            uint8_t x_bar[lambda_bytes];
            const auto* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            for (size_t b = 0; b < lambda_bytes; ++b)
                x_bar[b] = parent[b] ^ g_tccr_s[b];

            build_tccr_aes_plaintext(plaintexts[4 * i], x_bar, 0);
            build_tccr_aes_plaintext(plaintexts[4 * i + 2], x_bar, 1);
            x_bar[0] ^= 1;
            build_tccr_aes_plaintext(plaintexts[4 * i + 1], x_bar, 0);
            build_tccr_aes_plaintext(plaintexts[4 * i + 3], x_bar, 1);
            x_bar[0] ^= 1;

            build_tccr_aes_key(x_bar, iv_bytes, &aes_keys[i]);
            memcpy(&states[4 * i], plaintexts[4 * i], 16);
            memcpy(&states[4 * i + 1], plaintexts[4 * i + 1], 16);
            memcpy(&states[4 * i + 2], plaintexts[4 * i + 2], 16);
            memcpy(&states[4 * i + 3], plaintexts[4 * i + 3], 16);
        }

        aes_keygen_ecb<aes_secpar_v, num_keys, 4>(aes_keys, round_keys, states);

        for (size_t i = 0; i < num_keys; ++i)
        {
            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);

            copy_tccr_block(left, states[4 * i], plaintexts[4 * i], 16);
            copy_tccr_block(right, states[4 * i + 1], plaintexts[4 * i + 1], 16);
            copy_tccr_block(left + 16, states[4 * i + 2], plaintexts[4 * i + 2],
                            lambda_bytes - 16);
            copy_tccr_block(right + 16, states[4 * i + 3], plaintexts[4 * i + 3],
                            lambda_bytes - 16);
        }
    }

    template <size_t num_keys>
    static void gen_impl_scalar_mixed(const expanded_key_t* expanded_keys, const uint8_t* iv_bytes,
                                      const size_t* ids, block_t* output)
    {
        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        constexpr unsigned int lambda_bits = secpar_to_bits(S);

        for (size_t i = 0; i < num_keys; ++i)
        {
            const uint8_t* parent = reinterpret_cast<const uint8_t*>(&expanded_keys[i]);
            auto* left = reinterpret_cast<uint8_t*>(&output[2 * i]);
            auto* right = reinterpret_cast<uint8_t*>(&output[2 * i + 1]);

            output[2 * i] = block_t::set_zero();
            output[2 * i + 1] = block_t::set_zero();

            if (ids[i] < g_tccr_commit_leaves / 2)
            {
                tccr_hash_dispatch(parent, g_tccr_s, iv_bytes, left, lambda_bits);
                for (size_t b = 0; b < lambda_bytes; ++b)
                    right[b] = left[b] ^ parent[b];
            }
            else
            {
                tccr_hash_x0_x1_dispatch(parent, g_tccr_s, iv_bytes, left, right, lambda_bits);
            }
        }
    }
};

// Ref-compatible TCCR PRG for CSP 384.
// Uses ref_prg::detail::prg_tccr (tccr_hash in counter mode, 48-byte stride).
template <secpar S>
struct prg_trait<ref_tccr_prg<S>>
{
    using expanded_key_t = ref_tccr384_expanded_key;
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = AES_PREFERRED_WIDTH_SHIFT;
};

template <secpar S>
struct ref_tccr_prg : public prg_base<ref_tccr_prg<S>>
{
    static_assert(S == secpar::s384, "ref TCCR PRG is defined for Lynx CSP 384");

    typedef prg_base<ref_tccr_prg<S>> base;
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
            expanded_keys[i].key = keys[i];
            expanded_keys[i].cache_valid = false;
            expanded_keys[i].cached_block_index = 0;
        }
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        (void)tweaks;

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

        constexpr unsigned int csp = secpar_to_bits(S);
        constexpr uint32_t ref_tweak = (2 * csp + 127) / 128;
        constexpr size_t out_bytes = blocks_per_key * sizeof(block_t);

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        if constexpr (blocks_per_key == 1)
        {
            gen_one_block_cached_vaes<num_keys>(expanded_keys, iv_bytes, ref_tweak, counters,
                                                output);
            return;
        }
#endif

        for (size_t i = 0; i < num_keys; ++i)
        {
            const size_t start_byte = static_cast<size_t>(counters[i]) * sizeof(block_t);
            fill_from_offset(reinterpret_cast<const uint8_t*>(&expanded_keys[i].key), iv_bytes,
                             ref_tweak, start_byte,
                             reinterpret_cast<uint8_t*>(&output[i * blocks_per_key]), out_bytes);
        }
    }

    template <size_t num_keys>
    static void leaf_hash_2lambda(const key_t* keys, const iv_t& iv, uint8_t* output)
    {
        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        static_assert(lambda_bytes == 48);

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        for (size_t base = 0; base < num_keys; base += 4)
        {
            const size_t active = std::min<size_t>(4, num_keys - base);
            const uint8_t* key_ptrs[4];
            for (size_t lane = 0; lane < active; ++lane)
                key_ptrs[lane] = reinterpret_cast<const uint8_t*>(&keys[base + lane]);

            for (size_t block_index = 0; block_index < 2; ++block_index)
            {
                alignas(64) uint8_t blocks[4][lambda_bytes];
                tccr_hash_prg<secpar::s384>::tccr_hash384_ctr4_keys(
                    key_ptrs, iv_bytes, 0, block_index, active, blocks);

                for (size_t lane = 0; lane < active; ++lane)
                    memcpy(output + (base + lane) * 2 * lambda_bytes + block_index * lambda_bytes,
                           blocks[lane], lambda_bytes);
            }
        }
#else
        for (size_t i = 0; i < num_keys; ++i)
            ref_prg::detail::prg_tccr(reinterpret_cast<const uint8_t*>(&keys[i]), iv_bytes, 0,
                                      output + i * 2 * lambda_bytes, secpar_to_bits(S),
                                      2 * lambda_bytes);
#endif
    }

private:
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
    template <size_t num_keys>
    static void gen_one_block_cached_vaes(const expanded_key_t* expanded_keys, const uint8_t* iv,
                                          uint32_t tweak, const count_t* counters,
                                          block_t* output)
    {
        constexpr size_t csp_bytes = secpar_to_bytes(S);
        static_assert(sizeof(block_t) == 16);

        size_t block_indices[num_keys];
        size_t block_offsets[num_keys];
        for (size_t i = 0; i < num_keys; ++i)
        {
            const size_t start_byte = static_cast<size_t>(counters[i]) * sizeof(block_t);
            block_indices[i] = start_byte / csp_bytes;
            block_offsets[i] = start_byte % csp_bytes;
        }

        for (size_t i = 0; i < num_keys; ++i)
        {
            if (expanded_keys[i].cache_valid &&
                expanded_keys[i].cached_block_index == block_indices[i])
                continue;

            size_t lanes[4];
            const uint8_t* keys[4];
            size_t active = 0;
            for (size_t j = i; j < num_keys && active < 4; ++j)
            {
                if ((!expanded_keys[j].cache_valid ||
                     expanded_keys[j].cached_block_index != block_indices[j]) &&
                    block_indices[j] == block_indices[i])
                {
                    lanes[active] = j;
                    keys[active] = reinterpret_cast<const uint8_t*>(&expanded_keys[j].key);
                    ++active;
                }
            }

            alignas(64) uint8_t blocks[4][48];
            tccr_hash_prg<secpar::s384>::tccr_hash384_ctr4_keys(
                keys, iv, tweak, block_indices[i], active, blocks);
            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t idx = lanes[lane];
                memcpy(expanded_keys[idx].cached_block, blocks[lane], csp_bytes);
                expanded_keys[idx].cached_block_index = block_indices[idx];
                expanded_keys[idx].cache_valid = true;
            }
        }

        for (size_t i = 0; i < num_keys; ++i)
        {
            memcpy(&output[i], expanded_keys[i].cached_block + block_offsets[i], sizeof(block_t));
        }
    }
#endif

    static void fill_from_offset(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                                 size_t start_byte, uint8_t* out, size_t out_bytes)
    {
#if (defined(__x86_64__) || defined(__i386__)) && defined(__VAES__) && defined(__AVX512F__) &&     \
    defined(__AVX512BW__) && defined(__AVX512VL__)
        tccr_hash_prg<secpar::s384>::fill_tccr384_ctr_from_offset_vaes(
            key, iv, tweak, start_byte, out, out_bytes);
        return;
#endif

        constexpr size_t csp_bytes = secpar_to_bytes(S);
        size_t block_index = start_byte / csp_bytes;
        size_t block_offset = start_byte % csp_bytes;
        size_t written = 0;

        uint8_t s[64] = {0};
        uint8_t block[64];
        while (written < out_bytes)
        {
            s[0] = static_cast<uint8_t>(tweak + block_index);
            tccr_hash_384(key, s, iv, block);

            const size_t take = std::min(csp_bytes - block_offset, out_bytes - written);
            memcpy(out + written, block + block_offset, take);
            written += take;
            ++block_index;
            block_offset = 0;
        }
    }
};

// Ref-compatible SHACAL-2 CTR PRG for CSP 512.
// Uses ref_prg::detail::prg_shacal2_ctr (SHACAL-2 encrypt, 32-byte stride).
struct shacal2_expanded_key
{
    block512 key;
    uint32_t schedule[64];
    alignas(16) uint32_t wk_pairs[16][4];
    mutable bool cache_valid;
    mutable size_t cached_block_index;
    mutable uint8_t cached_block[32];
};

template <secpar S>
struct prg_trait<ref_shacal2_prg<S>>
{
    using expanded_key_t = shacal2_expanded_key;
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = AES_PREFERRED_WIDTH_SHIFT;
};

template <secpar S>
struct ref_shacal2_prg : public prg_base<ref_shacal2_prg<S>>
{
    static_assert(S == secpar::s512, "ref SHACAL-2 PRG is defined for Lynx CSP 512");

    typedef prg_base<ref_shacal2_prg<S>> base;
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
        for (size_t base = 0; base < num_keys; base += 4)
        {
            const size_t active = std::min<size_t>(4, num_keys - base);
            const uint8_t* key_ptrs[4];
            uint32_t* schedule_out[4];
            uint32_t* wk_out[4];
            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t i = base + lane;
                expanded_keys[i].key = keys[i];
                key_ptrs[lane] = reinterpret_cast<const uint8_t*>(&expanded_keys[i].key);
                schedule_out[lane] = expanded_keys[i].schedule;
                wk_out[lane] = &expanded_keys[i].wk_pairs[0][0];
                expanded_keys[i].cache_valid = false;
                expanded_keys[i].cached_block_index = 0;
            }
            tccr_detail::shacal2_key_schedule_wk_x4_interleaved(
                key_ptrs, schedule_out, wk_out, active);
        }
        gen_impl<num_keys, blocks_per_key>(expanded_keys, iv, tweaks, counters, output);
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv,
                         const tweak_t* tweaks, const count_t* counters, block_t* output)
    {
        (void)tweaks;

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

        constexpr uint32_t ref_tweak = (2 * 512 + 127) / 128;
        constexpr size_t out_bytes = blocks_per_key * sizeof(block_t);

        if constexpr (blocks_per_key == 1)
        {
            gen_one_block_cached_x4<num_keys>(expanded_keys, iv_bytes, ref_tweak, counters,
                                              output);
            return;
        }

        for (size_t i = 0; i < num_keys; ++i)
        {
            const size_t start_byte = static_cast<size_t>(counters[i]) * sizeof(block_t);
            fill_from_offset(expanded_keys[i], iv_bytes, ref_tweak, start_byte,
                             reinterpret_cast<uint8_t*>(&output[i * blocks_per_key]), out_bytes);
        }
    }

    template <size_t num_keys>
    static void leaf_hash_2lambda(const key_t* keys, const iv_t& iv, uint8_t* output)
    {
        constexpr size_t lambda_bytes = secpar_to_bytes(S);
        constexpr size_t block_len = 32;
        static_assert(lambda_bytes == 64);

        uint8_t iv_bytes[16];
        memcpy(iv_bytes, &iv, sizeof(iv_bytes));

        for (size_t base = 0; base < num_keys; base += 4)
        {
            const size_t active = std::min<size_t>(4, num_keys - base);
            uint32_t schedules_storage[4][64];
            alignas(16) uint32_t wk_storage[4][16][4];
            const uint8_t* key_ptrs[4];
            uint32_t* schedule_out[4];
            uint32_t* wk_out[4];
            [[maybe_unused]] const uint32_t* schedules[4];
            const uint32_t* wk_pairs[4];
            const uint8_t* plaintexts[4];
            uint8_t* ciphertexts[4];
            alignas(16) uint8_t plaintext_storage[4][block_len] = {};
            alignas(16) uint8_t ciphertext_storage[4][block_len];

            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t i = base + lane;
                key_ptrs[lane] = reinterpret_cast<const uint8_t*>(&keys[i]);
                schedule_out[lane] = schedules_storage[lane];
                wk_out[lane] = &wk_storage[lane][0][0];
                schedules[lane] = schedules_storage[lane];
                wk_pairs[lane] = &wk_storage[lane][0][0];
                memcpy(plaintext_storage[lane], iv_bytes, 15);
                plaintexts[lane] = plaintext_storage[lane];
                ciphertexts[lane] = ciphertext_storage[lane];
            }

            tccr_detail::shacal2_key_schedule_wk_x4_interleaved(
                key_ptrs, schedule_out, wk_out, active);

            for (size_t block_index = 0; block_index < 4; ++block_index)
            {
                for (size_t lane = 0; lane < active; ++lane)
                    plaintext_storage[lane][15] = static_cast<uint8_t>(block_index);

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
                tccr_detail::shacal2_encrypt_wk_x4(wk_pairs, plaintexts, ciphertexts, active);
#else
                tccr_detail::shacal2_encrypt_scheduled_x4(
                    schedules, plaintexts, ciphertexts, active);
#endif

                for (size_t lane = 0; lane < active; ++lane)
                    memcpy(output + (base + lane) * 2 * lambda_bytes + block_index * block_len,
                           ciphertext_storage[lane], block_len);
            }
        }
    }

private:
    template <size_t num_keys>
    static void gen_one_block_cached_x4(const expanded_key_t* expanded_keys, const uint8_t* iv,
                                        uint32_t tweak, const count_t* counters, block_t* output)
    {
        constexpr size_t block_len = 32;
        static_assert(sizeof(block_t) == 16);

        size_t block_indices[num_keys];
        size_t block_offsets[num_keys];
        for (size_t i = 0; i < num_keys; ++i)
        {
            const size_t start_byte = static_cast<size_t>(counters[i]) * sizeof(block_t);
            block_indices[i] = start_byte / block_len;
            block_offsets[i] = start_byte % block_len;
        }

        for (size_t i = 0; i < num_keys; ++i)
        {
            if (expanded_keys[i].cache_valid &&
                expanded_keys[i].cached_block_index == block_indices[i])
                continue;

            size_t lanes[4];
            [[maybe_unused]] const uint32_t* schedules[4];
            const uint32_t* wk_pairs[4];
            const uint8_t* plaintexts[4];
            uint8_t* ciphertexts[4];
            alignas(16) uint8_t plaintext_storage[4][32] = {};
            alignas(16) uint8_t ciphertext_storage[4][32];
            size_t active = 0;
            for (size_t j = i; j < num_keys && active < 4; ++j)
            {
                if ((!expanded_keys[j].cache_valid ||
                     expanded_keys[j].cached_block_index != block_indices[j]) &&
                    block_indices[j] == block_indices[i])
                {
                    lanes[active] = j;
                    schedules[active] = expanded_keys[j].schedule;
                    wk_pairs[active] = &expanded_keys[j].wk_pairs[0][0];
                    memcpy(plaintext_storage[active], iv, 15);
                    plaintext_storage[active][15] =
                        static_cast<uint8_t>(tweak + block_indices[j]);
                    plaintexts[active] = plaintext_storage[active];
                    ciphertexts[active] = ciphertext_storage[active];
                    ++active;
                }
            }

#if defined(__SHA__) && (defined(__x86_64__) || defined(__i386__))
            tccr_detail::shacal2_encrypt_wk_x4(wk_pairs, plaintexts, ciphertexts, active);
#else
            tccr_detail::shacal2_encrypt_scheduled_x4(schedules, plaintexts, ciphertexts, active);
#endif
            for (size_t lane = 0; lane < active; ++lane)
            {
                const size_t idx = lanes[lane];
                memcpy(expanded_keys[idx].cached_block, ciphertext_storage[lane], block_len);
                expanded_keys[idx].cached_block_index = block_indices[idx];
                expanded_keys[idx].cache_valid = true;
            }
        }

        for (size_t i = 0; i < num_keys; ++i)
            memcpy(&output[i], expanded_keys[i].cached_block + block_offsets[i], sizeof(block_t));
    }

    static void fill_from_offset(const expanded_key_t& expanded_key, const uint8_t* iv, uint32_t tweak,
                                 size_t start_byte, uint8_t* out, size_t out_bytes)
    {
        constexpr size_t block_len = 32;
        size_t block_index = start_byte / block_len;
        size_t block_offset = start_byte % block_len;
        size_t written = 0;

        uint8_t plaintext[32] = {0};
        memcpy(plaintext, iv, 15);

        while (written < out_bytes)
        {
            if (!expanded_key.cache_valid || expanded_key.cached_block_index != block_index)
            {
                plaintext[15] = static_cast<uint8_t>(tweak + block_index);
                tccr_detail::shacal2_encrypt_wk(expanded_key.wk_pairs, expanded_key.schedule,
                                                plaintext, expanded_key.cached_block);
                expanded_key.cached_block_index = block_index;
                expanded_key.cache_valid = true;
            }

            const size_t take = std::min(block_len - block_offset, out_bytes - written);
            memcpy(out + written, expanded_key.cached_block + block_offset, take);
            written += take;
            ++block_index;
            block_offset = 0;
        }
    }
};

// Wrapper holding 4 parallel SHAKE states for AVX2 Keccak x4 processing.
// For num_keys keys, only ceil(num_keys/4) entries are used.
// Mutable because SHAKE state advances on squeeze, but callers treat expanded keys
// as logically immutable after init.
struct shake_expanded_key_x4
{
    mutable Keccak_HashInstancetimes4 state;
};

template <secpar S>
struct prg_trait<shake_wide_prg<S>>
{
    using expanded_key_t = shake_expanded_key_x4;
    using iv_t = block128;
    using block_t = block128;
    using tweak_t = uint32_t;
    using count_t = uint32_t;

    static constexpr secpar secpar_v = S;
    static constexpr size_t PREFERRED_WIDTH_SHIFT = AES_PREFERRED_WIDTH_SHIFT;
};

template <secpar S>
struct shake_wide_prg : public prg_base<shake_wide_prg<S>>
{
    static_assert(S == secpar::s384 || S == secpar::s512);

    typedef prg_base<shake_wide_prg<S>> base;
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
        (void)counters; // Counters are implicit in squeeze position
        constexpr size_t output_bytes = blocks_per_key * sizeof(block_t);
        constexpr size_t num_groups = (num_keys + 3) / 4;

        for (size_t g = 0; g < num_groups; ++g)
        {
            const size_t base_idx = g * 4;
            const size_t active = std::min<size_t>(4, num_keys - base_idx);

            block128 internal_ivs[4];
            const uint8_t* key_ptrs[4];
            const uint8_t* iv_ptrs[4];
            uint8_t* out_ptrs[4];
            uint8_t dummy_out[output_bytes];

            for (size_t lane = 0; lane < 4; ++lane)
            {
                // Pad unused lanes with data from key[base_idx]
                size_t idx = (lane < active) ? base_idx + lane : base_idx;
                key_ptrs[lane] = reinterpret_cast<const uint8_t*>(&keys[idx]);
                internal_ivs[lane] = iv.add32(block128::set_low_high32(
                    0, tweaks[(lane < active) ? base_idx + lane : base_idx]));
                iv_ptrs[lane] = reinterpret_cast<const uint8_t*>(&internal_ivs[lane]);
                out_ptrs[lane] = (lane < active)
                    ? reinterpret_cast<uint8_t*>(&output[(base_idx + lane) * blocks_per_key])
                    : dummy_out;
            }

            keccak_shake_init_x4(&expanded_keys[g].state, S);
            Keccak_HashUpdatetimes4(&expanded_keys[g].state, key_ptrs, sizeof(keys[0]) * 8);
            Keccak_HashUpdatetimes4(&expanded_keys[g].state, iv_ptrs, sizeof(block128) * 8);
            Keccak_HashFinaltimes4(&expanded_keys[g].state, NULL);
            Keccak_HashSqueezetimes4(&expanded_keys[g].state, out_ptrs, output_bytes * 8);
        }
    }

    template <size_t num_keys, count_t blocks_per_key>
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                         const count_t* counters, block_t* output)
    {
        (void)iv;       // Output determined by SHAKE state position
        (void)tweaks;   // Absorbed during init
        (void)counters; // Implicit in squeeze position
        constexpr size_t output_bytes = blocks_per_key * sizeof(block_t);
        constexpr size_t num_groups = (num_keys + 3) / 4;

        for (size_t g = 0; g < num_groups; ++g)
        {
            const size_t base_idx = g * 4;
            const size_t active = std::min<size_t>(4, num_keys - base_idx);
            uint8_t dummy_out[output_bytes];
            uint8_t* out_ptrs[4];

            for (size_t lane = 0; lane < 4; ++lane)
                out_ptrs[lane] = (lane < active)
                    ? reinterpret_cast<uint8_t*>(&output[(base_idx + lane) * blocks_per_key])
                    : dummy_out;

            Keccak_HashSqueezetimes4(&expanded_keys[g].state, out_ptrs, output_bytes * 8);
        }
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
    static void gen_impl(const expanded_key_t* expanded_keys, const iv_t& iv, const tweak_t* tweaks,
                         const count_t* counters, block_t* output)
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

// ============================================================================
// Ref-compatible PRG implementations (matching lynx-ref prg.c)
// These produce byte-identical output to the reference implementation.
// Used by BAVC tree expansion and VOLE correlation generation.
// ============================================================================

namespace ref_prg
{

namespace detail
{

// Dispatch to the correct block cipher, matching ref's enc()
inline void enc_dispatch(const uint8_t* key, const uint8_t* plaintext, uint8_t* ciphertext,
                         unsigned int csp)
{
    if (csp == 512)
    {
        tccr_detail::shacal2_encrypt(key, plaintext, ciphertext);
        return;
    }

    if (csp == 160)
    {
        // AES-192: 16-byte block, 24-byte key (key is padded from 20 CSP bytes)
        block192 key_block;
        block128 pt_block;
        memset(&key_block, 0, sizeof(key_block));
        std::memcpy(&key_block, key, 20);  // CSP bytes, rest zero
        std::memcpy(&pt_block, plaintext, 16);

        aes_round_keys<secpar::s192> rk;
        aes_keygen<secpar::s192>(&rk, key_block);
        aes_ecb<secpar::s192, 1, 1>(&rk, &pt_block);

        std::memcpy(ciphertext, &pt_block, 16);
        return;
    }

    if (csp == 256)
    {
        block256 key_block;
        block128 pt_block;
        std::memcpy(&key_block, key, 32);
        std::memcpy(&pt_block, plaintext, 16);

        aes_round_keys<secpar::s256> rk;
        aes_keygen<secpar::s256>(&rk, key_block);
        aes_ecb<secpar::s256, 1, 1>(&rk, &pt_block);

        std::memcpy(ciphertext, &pt_block, 16);
    }
    else if (csp == 384)
    {
        block256 key_block;
        block256 pt_block;
        std::memcpy(&key_block, key, 32);
        std::memcpy(&pt_block, plaintext, 32);

        rijndael256_round_keys rk;
        rijndael256_keygen(&rk, key_block);
        // Encrypt using rijndael256_round_function
        pt_block = pt_block ^ rk.keys[0];
        for (size_t r = 1; r < RIJNDAEL_ROUNDS<secpar::s256>; ++r)
        {
            block256 after_sbox;
            rijndael256_round_function(&rk, &pt_block, &after_sbox, r);
        }
        {
            block256 after_sbox;
            rijndael256_round_function(&rk, &pt_block, &after_sbox, RIJNDAEL_ROUNDS<secpar::s256>);
        }
        std::memcpy(ciphertext, &pt_block, 32);
    }
    // CSP 160 (AES-192) will be added in Phase 4
}

// AES counter mode PRG for CSP 160 and 256.
// NGCC uses a 120-bit IV plus one counter/tweak byte as the 128-bit AES input block.
inline void prg_aes_ctr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                        uint8_t* out, unsigned int csp, size_t outlen)
{
    constexpr size_t IV_SIZE = 15;  // 120-bit IV per NGCC spec
    constexpr size_t AES_BLOCK_SIZE = 16;
    const size_t full_blocks = outlen / AES_BLOCK_SIZE;
    const size_t tail = outlen % AES_BLOCK_SIZE;

    uint8_t enc_key[32] = {0};
    uint8_t plaintext[AES_BLOCK_SIZE] = {0};
    uint8_t block[16];

    size_t csp_bytes = csp / 8;
    std::memcpy(enc_key, key, csp_bytes);
    std::memcpy(plaintext, iv, IV_SIZE);

    for (size_t i = 0; i < full_blocks; ++i)
    {
        plaintext[IV_SIZE] = (uint8_t)(tweak + i);
        enc_dispatch(enc_key, plaintext, out + i * AES_BLOCK_SIZE, csp);
    }

    if (tail)
    {
        plaintext[IV_SIZE] = (uint8_t)(tweak + full_blocks);
        enc_dispatch(enc_key, plaintext, block, csp);
        std::memcpy(out + full_blocks * AES_BLOCK_SIZE, block, tail);
    }
}

// SHACAL-2 counter mode PRG, matching ref's prg_shacal2_counter_mode (CSP 512)
inline void prg_shacal2_ctr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                            uint8_t* out, size_t outlen)
{
    constexpr size_t block_len = 32;
    const size_t full_blocks = outlen / block_len;
    const size_t tail = outlen % block_len;

    uint8_t plaintext[32] = {0};
    uint8_t block[32];

    std::memcpy(plaintext, iv, 15); // IV is 120 bits = 15 bytes

    for (size_t i = 0; i < full_blocks; ++i)
    {
        plaintext[15] = (uint8_t)(tweak + i);
        tccr_detail::shacal2_encrypt(key, plaintext, out + i * block_len);
    }

    if (tail)
    {
        plaintext[15] = (uint8_t)(tweak + full_blocks);
        tccr_detail::shacal2_encrypt(key, plaintext, block);
        std::memcpy(out + full_blocks * block_len, block, tail);
    }
}

// TCCR mode PRG, matching ref's prg_tccr_mode (CSP 384)
inline void prg_tccr(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                     uint8_t* out, unsigned int csp, size_t outlen)
{
    const size_t csp_bytes = csp / 8;
    const size_t full_blocks = outlen / csp_bytes;
    const size_t tail = outlen % csp_bytes;

    uint8_t s[64] = {0};
    uint8_t block[64];

    for (size_t i = 0; i < full_blocks; ++i)
    {
        s[0] = (uint8_t)(tweak + i);
        tccr_hash_384(key, s, iv, out + i * csp_bytes);
    }

    if (tail)
    {
        s[0] = (uint8_t)(tweak + full_blocks);
        tccr_hash_384(key, s, iv, block);
        std::memcpy(out + full_blocks * csp_bytes, block, tail);
    }
}

} // namespace detail

// Main PRG dispatch, matching ref's prg()
inline void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                uint8_t* out, unsigned int csp, size_t outlen)
{
    if (csp == 384)
    {
        detail::prg_tccr(key, iv, tweak, out, csp, outlen);
        return;
    }

    if (csp == 512)
    {
        detail::prg_shacal2_ctr(key, iv, tweak, out, outlen);
        return;
    }

    // CSP 160 and 256 use AES counter mode
    detail::prg_aes_ctr(key, iv, tweak, out, csp, outlen);
}

// Convenience: prg_2_lambda outputs 2 * csp/8 bytes, matching ref
inline void prg_2_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                         uint8_t* out, unsigned int csp)
{
    prg(key, iv, tweak, out, csp, csp * 2 / 8);
}

// Convenience: prg_4_lambda outputs 4 * csp/8 bytes, matching ref
inline void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                         uint8_t* out, unsigned int csp)
{
    prg(key, iv, tweak, out, csp, csp * 4 / 8);
}

} // namespace ref_prg

} // namespace sig

#endif
