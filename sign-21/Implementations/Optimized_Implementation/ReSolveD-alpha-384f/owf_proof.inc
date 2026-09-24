#ifndef OWF_PROOF_INC
#define OWF_PROOF_INC

#include "aes.hpp"
#include "constants.hpp"
#include "owf_proof.hpp"
#include "parameters.hpp"
#include "quicksilver.hpp"
#include "rsd_owf.hpp"
#include "voleith_keys.hpp"
#include <algorithm>
#include <cstring>

namespace sig
{

template <typename QS> static quicksilver_gf2<QS> rsd_public_bit(bool bit, const QS* state)
{
    return quicksilver_gf2<QS>(bit ? poly1::set_one() : poly1::set_zero(), state);
}

template <secpar S> constexpr uint32_t rsd_field_modulus()
{
    if constexpr (S == secpar::s160)
        return gf160_modulus;
    else if constexpr (S == secpar::s256)
        return gf256_modulus;
    else if constexpr (S == secpar::s384)
        return gf384_modulus;
    else if constexpr (S == secpar::s512)
        return gf512_modulus;
    else
        static_assert(dependent_false<S>, "unsupported security parameter for RSD sketch");
}

template <secpar S> [[gnu::always_inline]] static inline uint32_t rsd_top_bit(poly_secpar<S> x)
{
    if constexpr (S == secpar::s160)
        return static_cast<uint32_t>(_mm_cvtsi128_si32(x.data[1].data.data)) >> 31;
    else if constexpr (S == secpar::s256)
        return static_cast<uint64_t>(_mm_extract_epi64(x.data[1].data.data, 1)) >> 63;
    else if constexpr (S == secpar::s384)
        return static_cast<uint64_t>(_mm_extract_epi64(x.data[2].data.data, 1)) >> 63;
    else if constexpr (S == secpar::s512)
        return static_cast<uint64_t>(_mm_extract_epi64(x.data[3].data.data, 1)) >> 63;
    else
        static_assert(dependent_false<S>, "unsupported security parameter for RSD sketch");
}

template <secpar S>
[[gnu::always_inline]] static inline poly_secpar<S> rsd_shift_left_1_unreduced(poly_secpar<S> x)
{
    if constexpr (S == secpar::s160)
    {
        detail::poly_shift_left_1(&x.data[0].data, 2);
        x.data[1] = {_mm_and_si128(x.data[1].data.data, _mm_cvtsi32_si128(-1))};
        return x;
    }
    else
    {
        return x.shift_left_1();
    }
}

template <secpar S> [[gnu::always_inline]] static inline poly_secpar<S> rsd_mul_x(poly_secpar<S> x)
{
    const uint32_t carry_mask = 0u - rsd_top_bit<S>(x);
    auto out = rsd_shift_left_1_unreduced<S>(x);
    out += poly_secpar<S>::set_low32(rsd_field_modulus<S>() & carry_mask);
    return out;
}

template <secpar S>
[[gnu::always_inline]] static inline poly_secpar<S> rsd_mul_x_pow(poly_secpar<S> x,
                                                                  unsigned int exp)
{
    PRAGMA_UNROLL(18)
    for (unsigned int i = 0; i < exp; ++i)
        x = rsd_mul_x<S>(x);
    return x;
}

template <typename QS, std::size_t deg>
[[gnu::always_inline]] static inline quicksilver_gfsecpar<QS, deg>
rsd_mul_x_pow(quicksilver_gfsecpar<QS, deg> x, unsigned int exp)
{
    if constexpr (QS::is_verifier)
    {
        x.mac = rsd_mul_x_pow<QS::secpar_v>(x.mac, exp);
    }
    else
    {
        PRAGMA_UNROLL(4)
        for (auto& coeff : x.mac.coeffs)
            coeff = rsd_mul_x_pow<QS::secpar_v>(coeff, exp);
    }
    return x;
}

template <typename QS>
[[gnu::always_inline]] static inline quicksilver_gfsecpar<QS>
rsd_lc_bits_stride(const quicksilver_gf2<QS>* bits, std::size_t count, unsigned int stride)
{
    auto out = quicksilver_gfsecpar<QS>(bits[count - 1]);
    for (std::size_t i = count - 1; i-- > 0;)
    {
        out = rsd_mul_x_pow(out, stride);
        out += quicksilver_gfsecpar<QS>(bits[i]);
    }
    return out;
}

template <typename QS, std::size_t rows, std::size_t cols>
static void rsd_gf2_matvec(quicksilver_gf2<QS>* output, const uint8_t* public_syndrome,
                           const uint8_t* matrix_idx8, const uint64_t* matrix_u64,
                           const quicksilver_gf2<QS>* input, const QS* state)
{
    using qs_gf2_t = quicksilver_gf2<QS>;
    constexpr std::size_t block = 8;
    constexpr std::size_t mac_bytes = sizeof(decltype(output[0].mac));
    constexpr std::size_t padded_mac_bytes = (mac_bytes + 31) & ~std::size_t(31);
    constexpr std::size_t ymms = padded_mac_bytes / 32;
    constexpr std::size_t value_chunks = (cols + 63) / 64;

    static_assert(padded_mac_bytes % 32 == 0);

    for (std::size_t row = 0; row < rows; ++row)
        output[row] = qs_gf2_t(poly1::set_zero(), state);

    alignas(32) uint8_t output_macs[rows][padded_mac_bytes] = {};
    alignas(32) uint8_t table[1 << block][padded_mac_bytes] = {};

    for (std::size_t col_base = 0; col_base < cols; col_base += block)
    {
        const std::size_t width = std::min(block, cols - col_base);

        for (std::size_t y = 0; y < ymms; ++y)
            _mm256_store_si256(reinterpret_cast<__m256i*>(&table[0][y * 32]),
                               _mm256_setzero_si256());

        for (std::size_t bit = 0; bit < width; ++bit)
        {
            __m256i input_ymm[ymms];
            if constexpr (mac_bytes == padded_mac_bytes)
            {
                const auto* input_mac =
                    reinterpret_cast<const uint8_t*>(&input[col_base + bit].mac);
                for (std::size_t y = 0; y < ymms; ++y)
                    input_ymm[y] =
                        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input_mac[y * 32]));
            }
            else
            {
                alignas(32) uint8_t input_mac[padded_mac_bytes] = {};
                std::memcpy(input_mac, &input[col_base + bit].mac, mac_bytes);
                for (std::size_t y = 0; y < ymms; ++y)
                    input_ymm[y] =
                        _mm256_load_si256(reinterpret_cast<const __m256i*>(&input_mac[y * 32]));
            }

            const std::size_t half = std::size_t(1) << bit;
            for (std::size_t idx = 0; idx < half; ++idx)
            {
                for (std::size_t y = 0; y < ymms; ++y)
                {
                    const __m256i prev =
                        _mm256_load_si256(reinterpret_cast<const __m256i*>(&table[idx][y * 32]));
                    _mm256_store_si256(reinterpret_cast<__m256i*>(&table[idx + half][y * 32]),
                                       _mm256_xor_si256(prev, input_ymm[y]));
                }
            }
        }

        const uint8_t* matrix_col_idx8 = matrix_idx8 + (col_base / block) * rows;
        for (std::size_t row = 0; row < rows; ++row)
        {
            const uint8_t idx = matrix_col_idx8[row];
            const __m256i* tab = reinterpret_cast<const __m256i*>(table[idx]);
            __m256i* out = reinterpret_cast<__m256i*>(output_macs[row]);
            for (std::size_t y = 0; y < ymms; ++y)
                out[y] = _mm256_xor_si256(out[y], tab[y]);
        }
    }

    for (std::size_t row = 0; row < rows; ++row)
        std::memcpy(&output[row].mac, output_macs[row], mac_bytes);

    if constexpr (!QS::is_verifier)
    {
        uint64_t input_values[value_chunks] = {};
        for (std::size_t col = 0; col < cols; ++col)
            if (input[col].value().data & 1)
                input_values[col / 64] |= uint64_t(1) << (col % 64);

        for (std::size_t row = 0; row < rows; ++row)
        {
            const uint64_t* matrix_row = matrix_u64 + row * value_chunks;
            uint64_t parity_bits = 0;
            for (std::size_t chunk = 0; chunk < value_chunks; ++chunk)
                parity_bits ^= matrix_row[chunk] & input_values[chunk];
            unsigned int parity = static_cast<unsigned int>(
                __builtin_parityll(static_cast<unsigned long long>(parity_bits)));
            parity ^= rsd::ptr_get_bit(public_syndrome, row);
            output[row].value() = poly1{static_cast<uint8_t>(parity * 0xFF)};
        }
    }
}

template <typename P, typename QS>
[[gnu::always_inline]] static inline void rsd_owf_constraints(QS* state, const public_key<P>* pk)
{
    static_assert(is_owf_with_rsd(P::owf_v), "Resolved-Alpha supports only the RSD OWF");

    using OC = typename P::OWF_CONSTS;
    constexpr std::size_t n = OC::RSD_CODE_LENGTH;
    constexpr std::size_t k = OC::RSD_DIMENSION;
    constexpr std::size_t a_bits = n - k;
    constexpr std::size_t b_blocks = k / OC::RSD_BLOCK_SIZE;

    static_assert(OC::RSD_BLOCK_SIZE == 6);
    static_assert(OC::RSD_PACKED_BLOCK_BITS == 5);
    static_assert(OC::WITNESS_BITS >= OC::RSD_WITNESS_BITS);

    const uint8_t* matrix_b_idx8 = rsd::cached_matrix_b_witness_idx8<P>(pk->seed).data();
    const uint64_t* matrix_b_u64 = rsd::cached_matrix_b_witness_u64<P>(pk->seed).data();
    const uint8_t* matrix_b_constant = rsd::cached_matrix_b_witness_constant<P>(pk->seed).data();

    auto a_part = state->template const_gf2_array<a_bits>();
    auto b_part = state->template const_gf2_array<k>();
    auto w_part = state->template const_gf2_array<OC::RSD_WITNESS_BITS>();

    for (std::size_t block = 0; block < b_blocks; ++block)
    {
        const std::size_t e_base = block * OC::RSD_BLOCK_SIZE;
        const std::size_t w_base = block * OC::RSD_PACKED_BLOCK_BITS;
        auto parity = rsd_public_bit<QS>(true, state);
        for (std::size_t j = 0; j < OC::RSD_PACKED_BLOCK_BITS; ++j)
        {
            const auto witness_bit = state->get_witness_bit(w_base + j);
            w_part[w_base + j] = witness_bit;
            b_part[e_base + j] = witness_bit;
            parity += witness_bit;
        }
        b_part[e_base + OC::RSD_PACKED_BLOCK_BITS] = parity;
    }

    rsd_gf2_matvec<QS, a_bits, OC::RSD_WITNESS_BITS>(a_part.data(), pk->owf_output, matrix_b_idx8,
                                                     matrix_b_u64, w_part.data(), state);
    if constexpr (QS::is_verifier)
    {
        const auto public_one = rsd_public_bit<QS>(true, state);
        for (std::size_t row = 0; row < a_bits; ++row)
            if (rsd::ptr_get_bit(pk->owf_output, row) ^ rsd::ptr_get_bit(matrix_b_constant, row))
                a_part[row] += public_one;
    }
    else
    {
        for (std::size_t row = 0; row < a_bits; ++row)
            if (rsd::ptr_get_bit(matrix_b_constant, row))
                a_part[row].value() += poly1::set_one();
    }

    for (std::size_t block = 0; block < OC::RSD_NOISE_WEIGHT; ++block)
    {
        const std::size_t base = block * OC::RSD_BLOCK_SIZE;
        const auto* bits = base < a_bits ? a_part.data() + base : b_part.data() + (base - a_bits);
        const auto av = rsd_lc_bits_stride(bits, 5, 1);
        const auto bv = rsd_lc_bits_stride(bits, 4, 5);
        const auto cv = rsd_lc_bits_stride(bits, 4, 6);
        state->add_constraint(av * bv + quicksilver_gfsecpar<QS, 2>(cv));
    }

    constexpr std::size_t a_blocks = a_bits / OC::RSD_BLOCK_SIZE;
    static_assert(a_bits % OC::RSD_BLOCK_SIZE == 0);
    static_assert(OC::RSD_LINEAR_CHECK_CONSTRAINTS == 1);

    auto linear_check = quicksilver_gfsecpar<QS>(poly1::set_zero(), state);
    for (std::size_t block = a_blocks; block-- > 0;)
    {
        if (block != a_blocks - 1)
            linear_check = rsd_mul_x_pow(linear_check, 1);

        auto block_sum = rsd_public_bit<QS>(true, state);
        for (std::size_t j = 0; j < OC::RSD_BLOCK_SIZE; ++j)
            block_sum += a_part[block * OC::RSD_BLOCK_SIZE + j];

        linear_check += quicksilver_gfsecpar<QS>(block_sum);
    }
    state->add_constraint(linear_check);
}

template <typename P, bool verifier>
void owf_constraints(quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE>* state,
                     const public_key<P>* pk)
{
    rsd_owf_constraints<P>(state, pk);
}

template <typename P>
void owf_constraints(quicksilver_state<P::secpar_v, false, P::OWF_CONSTS::QS_DEGREE>* state,
                     const secret_key<P>* sk)
{
    rsd_owf_constraints<P>(state, &sk->pk);
}

} // namespace sig

#endif
