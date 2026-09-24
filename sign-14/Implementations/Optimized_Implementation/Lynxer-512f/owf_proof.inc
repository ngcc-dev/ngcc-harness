#ifndef OWF_PROOF_INC
#define OWF_PROOF_INC

#include "aes.hpp"
#include "constants.hpp"
#include "voleith_keys.hpp"
#include "owf_proof.hpp"
#include "parameters.hpp"
#include "quicksilver.hpp"
#include "lynx_constants.hpp"
#include "lynx_matrices.hpp"
#include "lynx_frobenius_consts.hpp"
#include <memory>

namespace sig
{

// ==================== Lynx OWF constraint helpers ====================

// Combine lambda QS GF(2) bits into a QS GF(2^lambda) element using polynomial basis.
// Element = sum(bit[i] * X^i) for i = 0..lambda-1.
// Prover version: builds both value and MAC coefficients.
template <secpar S, size_t max_deg, size_t deg>
static quicksilver_gfsecpar<quicksilver_state<S, false, max_deg>, deg>
lynx_combine_lambda_bits(
    const quicksilver_gf2<quicksilver_state<S, false, max_deg>, deg>* qs_bits)
{
    using QS = quicksilver_state<S, false, max_deg>;
    constexpr size_t lambda = secpar_to_bits(S);

    quicksilver_gfsecpar<QS, deg> out(poly1::set_zero(), qs_bits[0]);

    // Build value: pack lambda single-bit values into poly_secpar
    uint8_t val_bytes[secpar_to_bytes(S)] = {};
    for (size_t j = 0; j < lambda; ++j)
        if (qs_bits[j].value().data & 1)
            val_bytes[j / 8] |= static_cast<uint8_t>(1u << (j % 8));
    out.value() = poly_secpar<S>::load(val_bytes);

    // Build MAC coefficients using Horner's method: sum(mac[j] * X^j)
    for (size_t c = 0; c < deg; ++c)
    {
        poly_2secpar<S> accum = poly_2secpar<S>::set_zero();
        for (size_t j = lambda; j-- > 0;)
            accum = accum.shift_left_1() + poly_2secpar<S>::from(qs_bits[j].mac.coeffs[c]);
        out.mac.coeffs[c] = accum.template reduce_to<secpar_to_bits(S)>();
    }

    return out;
}

// Verifier version: only has MAC (no separate value).
template <secpar S, size_t max_deg, size_t deg>
static quicksilver_gfsecpar<quicksilver_state<S, true, max_deg>, deg>
lynx_combine_lambda_bits(
    const quicksilver_gf2<quicksilver_state<S, true, max_deg>, deg>* qs_bits)
{
    using QS = quicksilver_state<S, true, max_deg>;
    constexpr size_t lambda = secpar_to_bits(S);

    quicksilver_gfsecpar<QS, deg> out;
    out.delta_powers = qs_bits[0].delta_powers;

    poly_2secpar<S> accum = poly_2secpar<S>::set_zero();
    for (size_t j = lambda; j-- > 0;)
        accum = accum.shift_left_1() + poly_2secpar<S>::from(qs_bits[j].mac);
    out.mac = accum.template reduce_to<secpar_to_bits(S)>();

    return out;
}

// GF(2) matrix-vector product on QS GF(2) bits.
// output[i] = sum_j M[i][j] * input[j] (GF(2) additions in QuickSilver).
// Uses M4RM with AVX2: mac fields are packed into aligned arrays and XOR'd with __m256i.
// Value bits (prover) are processed separately using packed uint64_t GF(2) operations.
template <typename QS, size_t deg, size_t N, size_t W>
static void lynx_gf2_matvec(
    quicksilver_gf2<QS, deg>* output,
    const std::array<std::array<uint64_t, W>, N>& M,
    const quicksilver_gf2<QS, deg>* input,
    const QS* state)
{
    using qs_gf2_t = quicksilver_gf2<QS, deg>;
    constexpr size_t BLOCK = 8;
    // MAC field byte count and __m256i dimensions
    constexpr size_t MAC_BYTES = sizeof(decltype(output[0].mac));
    constexpr size_t PADDED = (MAC_BYTES + 31) & ~size_t(31);
    constexpr size_t YMMS = PADDED / 32;

    static_assert(N % BLOCK == 0, "N must be a multiple of BLOCK for M4RM");

    // Initialize output elements (sets delta_powers for verifier, value_ for prover)
    for (size_t i = 0; i < N; ++i)
        output[i] = qs_gf2_t(poly1::set_zero(), state);

    // Packed output mac accumulator (zero-initialized)
    alignas(32) uint8_t out_mac[N][PADDED] = {};

    // M4RM lookup table with aligned entries
    alignas(32) uint8_t table[1 << BLOCK][PADDED];

    for (size_t col_base = 0; col_base < N; col_base += BLOCK)
    {
        // Zero table entry 0
        for (size_t y = 0; y < YMMS; ++y)
            _mm256_store_si256((__m256i*)&table[0][y * 32], _mm256_setzero_si256());

        // Build table entries via XOR combinations of input mac fields
        for (size_t k = 0; k < BLOCK; ++k)
        {
            const size_t half = size_t(1) << k;
            // Load input[col_base + k].mac using unaligned reads (16-byte aligned in struct)
            __m256i inp_ymm[YMMS];
            for (size_t y = 0; y < YMMS; ++y)
                inp_ymm[y] = _mm256_loadu_si256(
                    (const __m256i*)((const uint8_t*)&input[col_base + k].mac + y * 32));

            for (size_t idx = 0; idx < half; ++idx)
            {
                for (size_t y = 0; y < YMMS; ++y)
                {
                    __m256i a = _mm256_load_si256((const __m256i*)&table[idx][y * 32]);
                    _mm256_store_si256((__m256i*)&table[idx + half][y * 32],
                                       _mm256_xor_si256(a, inp_ymm[y]));
                }
            }
        }

        // Row scan: extract 8-bit index per row, XOR table entry into output
        const size_t word_idx = col_base / 64;
        const size_t bit_offset = col_base % 64;

        for (size_t i = 0; i < N; ++i)
        {
            const uint8_t byte =
                static_cast<uint8_t>(M[i][word_idx] >> bit_offset) & 0xFF;

            const __m256i* tab = (const __m256i*)table[byte];
            __m256i* out = (__m256i*)out_mac[i];
            for (size_t y = 0; y < YMMS; ++y)
                out[y] = _mm256_xor_si256(out[y], tab[y]);
        }
    }

    // Copy mac results back to output structs
    for (size_t i = 0; i < N; ++i)
        memcpy(&output[i].mac, out_mac[i], MAC_BYTES);

    // Process value bits separately (prover only)
    if constexpr (!QS::is_verifier)
    {
        // Pack input value bits into uint64_t words
        uint64_t inp_bits[W] = {};
        for (size_t j = 0; j < N; ++j)
            if (input[j].value().data & 1)
                inp_bits[j / 64] |= (1ULL << (j % 64));

        // GF(2) matvec: output_bit[i] = parity(M[i] AND inp_bits)
        for (size_t i = 0; i < N; ++i)
        {
            uint64_t acc = 0;
            for (size_t w = 0; w < W; ++w)
                acc ^= M[i][w] & inp_bits[w];
            output[i].value() = poly1{static_cast<uint8_t>(__builtin_parityll(acc) * 0xFF)};
        }
    }
}

template <typename QS, size_t deg, size_t N, size_t W, size_t SW>
static void lynx_gf2_matvec_padded_reference_stride(
    quicksilver_gf2<QS, deg>* output,
    const std::array<std::array<uint64_t, SW>, N>& M,
    const quicksilver_gf2<QS, deg>* input,
    const QS* state)
{
    std::array<std::array<uint64_t, W>, N> reference_view{};
    const uint64_t* flat = M[0].data();
    for (size_t i = 0; i < N; ++i)
        for (size_t w = 0; w < W; ++w)
            reference_view[i][w] = flat[i * W + w];
    lynx_gf2_matvec(output, reference_view, input, state);
}

// XOR public constant bytes (bit-packed) into a QS GF(2) bit array.
// bits[j] += c_bits[j] where c_bits[j] is a public constant (mac=0).
template <typename QS, size_t N>
static void xor_constant_bits(
    quicksilver_gf2<QS>* bits,
    const uint8_t* constant_bytes,
    const QS* state)
{
    using qs_gf2_t = quicksilver_gf2<QS>;
    for (size_t j = 0; j < N; ++j)
    {
        uint8_t bit = (constant_bytes[j / 8] >> (j % 8)) & 1;
        if (bit)
            bits[j] += qs_gf2_t(poly1{0xFF}, state);
    }
}

// ==================== End Lynx helpers ====================

template <typename P, typename QS>
[[gnu::always_inline]] static inline void lynx_owf_constraints(QS* state, const public_key<P>* pk,
                                 const lynx::lynx_matrices<P::secpar_v>& mats)
{
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;
    constexpr size_t lambda = secpar_to_bits(S);

    static_assert(OC::WITNESS_BITS == 3 * secpar_to_bits(S), "Lynx witness must be 3*lambda bits");

    // Load witness bits: k || v1 || v2
    auto k_bits = state->template const_gf2_array<lambda>();
    auto v1_bits = state->template const_gf2_array<lambda>();
    auto v2_bits = state->template const_gf2_array<lambda>();

    for (size_t j = 0; j < lambda; ++j)
    {
        k_bits[j] = state->get_witness_bit(j);
        v1_bits[j] = state->get_witness_bit(lambda + j);
        v2_bits[j] = state->get_witness_bit(2 * lambda + j);
    }

    // Combine bits into GF(2^lambda) QS elements using polynomial basis
    auto qs_k = lynx_combine_lambda_bits(k_bits.data());
    auto qs_v1 = lynx_combine_lambda_bits(v1_bits.data());
    auto qs_v2 = lynx_combine_lambda_bits(v2_bits.data());

    // GF(2) matrix-vector products on QS bits
    auto L0k_bits = state->template const_gf2_array<lambda>();
    auto L1v1_bits = state->template const_gf2_array<lambda>();
    auto L2v2_bits = state->template const_gf2_array<lambda>();

    // Optimize: L3(k) + L3(v1) + L3(v2) = L3(k + v1 + v2) by linearity
    auto k_v1_v2_bits = state->template const_gf2_array<lambda>();
    for (size_t j = 0; j < lambda; ++j)
        k_v1_v2_bits[j] = k_bits[j] + v1_bits[j] + v2_bits[j];
    auto L3_kv1v2_bits = state->template const_gf2_array<lambda>();

    lynx_gf2_matvec_padded_reference_stride<QS, 1, lambda, lynx::lynx_matrices<S>::words>(
        L0k_bits.data(), mats.L0, k_bits.data(), state);
    lynx_gf2_matvec_padded_reference_stride<QS, 1, lambda, lynx::lynx_matrices<S>::words>(
        L1v1_bits.data(), mats.L1, v1_bits.data(), state);
    lynx_gf2_matvec_padded_reference_stride<QS, 1, lambda, lynx::lynx_matrices<S>::words>(
        L2v2_bits.data(), mats.L2, v2_bits.data(), state);
    lynx_gf2_matvec_padded_reference_stride<QS, 1, lambda, lynx::lynx_matrices<S>::words>(
        L3_kv1v2_bits.data(), mats.L3, k_v1_v2_bits.data(), state);

    // Combine matrix outputs into GF(2^lambda) elements
    auto qs_L0k = lynx_combine_lambda_bits(L0k_bits.data());
    auto qs_L1v1 = lynx_combine_lambda_bits(L1v1_bits.data());
    auto qs_L2v2 = lynx_combine_lambda_bits(L2v2_bits.data());
    auto qs_L3_sum = lynx_combine_lambda_bits(L3_kv1v2_bits.data());

    // ==================== Begin constraints ====================

    // Load public constants from matrices
    auto qs_c0 = quicksilver_gfsecpar<QS>(poly_secpar<S>::load(mats.C0.data()), state);
    auto qs_c1 = quicksilver_gfsecpar<QS>(poly_secpar<S>::load(mats.C1.data()), state);

    // S-box input elements. s160 uses c0 for both S1 and S2 inputs.
    auto qs_s1_input = qs_k + qs_c0;
    auto qs_s2_input = [&]() {
        if constexpr (S == secpar::s160)
            return qs_L0k + qs_c0;
        else
            return qs_L0k + qs_c1;
    }();

    auto s1_input_bits = state->template const_gf2_array<lambda>();
    auto s2_input_bits = state->template const_gf2_array<lambda>();
    for (size_t j = 0; j < lambda; ++j)
    {
        s1_input_bits[j] = k_bits[j];
        s2_input_bits[j] = L0k_bits[j];
    }
    xor_constant_bits<QS, lambda>(s1_input_bits.data(), mats.C0.data(), state);
    if constexpr (S == secpar::s160)
        xor_constant_bits<QS, lambda>(s2_input_bits.data(), mats.C0.data(), state);
    else
        xor_constant_bits<QS, lambda>(s2_input_bits.data(), mats.C1.data(), state);

    // S1 constraint (per CSP)
    if constexpr (S == secpar::s128)
    {
        state->add_inverse_constraints(qs_s1_input, qs_v1);
        state->add_inverse_constraints(qs_s2_input, qs_v2);
    }
    else if constexpr (S == secpar::s160)
    {
        // 160-bit: S1 = full inverse (all-ones exponent), S2 = x^(2^67-1)
        state->add_inverse_constraints(qs_s1_input, qs_v1);
        // S2 constraint: v2 = s2_input^(2^67 - 1) => v2 * s2_input = s2_input^(2^67)
        auto frob67_s2_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob67_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 67>(),
                        s2_input_bits.data(), state);
        auto qs_frob67_s2 = lynx_combine_lambda_bits(frob67_s2_bits.data());
        // Constraint: v2 * s2_input + frob67(s2_input) = 0
        state->add_constraint(qs_v2 * qs_s2_input + qs_frob67_s2);
    }
    else if constexpr (S == secpar::s192)
    {
        // S1: v1 = inv(2^13-1)(k+c0) => v1 * (k+c0) = v1^(2^13)
        auto frob13_v1_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob13_v1_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 13>(),
                        v1_bits.data(), state);
        auto qs_frob13_v1 = lynx_combine_lambda_bits(frob13_v1_bits.data());
        state->add_constraint(qs_s1_input * qs_v1 + qs_frob13_v1);

        // S2: v2 = s2_input^(2^59-1) => v2 * s2_input = s2_input^(2^59)
        auto frob59_s2_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob59_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 59>(),
                        s2_input_bits.data(), state);
        auto qs_frob59_s2 = lynx_combine_lambda_bits(frob59_s2_bits.data());
        state->add_constraint(qs_s2_input * qs_v2 + qs_frob59_s2);
    }
    else if constexpr (S == secpar::s256)
    {
        // S1: v1 = inv(2^5-1)(k+c0) => v1 * (k+c0) = v1^(2^5)
        auto frob5_v1_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob5_v1_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 5>(),
                        v1_bits.data(), state);
        auto qs_frob5_v1 = lynx_combine_lambda_bits(frob5_v1_bits.data());
        state->add_constraint(qs_s1_input * qs_v1 + qs_frob5_v1);

        // S2: x^(2^53) * x = x^2 * v2, where x = L0(k) + c1.
        auto frob53_s2_bits = state->template const_gf2_array<lambda>();
        auto frob1_s2_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob53_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 53>(),
                        s2_input_bits.data(), state);
        static const auto frob1_matrix = []
        {
            std::array<std::array<uint64_t, (lambda + 63) / 64>, lambda> matrix{};
            lynx::lynx_frobenius_matrix<lambda>(1, matrix);
            return matrix;
        }();
        lynx_gf2_matvec(frob1_s2_bits.data(), frob1_matrix, s2_input_bits.data(), state);
        auto qs_frob53_s2 = lynx_combine_lambda_bits(frob53_s2_bits.data());
        auto qs_frob1_s2 = lynx_combine_lambda_bits(frob1_s2_bits.data());
        state->add_constraint(qs_frob53_s2 * qs_s2_input + qs_frob1_s2 * qs_v2);
    }
    else if constexpr (S == secpar::s384)
    {
        // S1: v1 = inv(2^13-1)(k+c0) => v1 * (k+c0) = v1^(2^13)
        auto frob13_v1_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob13_v1_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 13>(),
                        v1_bits.data(), state);
        auto qs_frob13_v1 = lynx_combine_lambda_bits(frob13_v1_bits.data());
        state->add_constraint(qs_s1_input * qs_v1 + qs_frob13_v1);

        // S2: v2 = s2_input^(2^60-2^30+1)
        // => v2 * frob30(s2_input) = frob60(s2_input) * s2_input
        auto frob60_s2_bits = state->template const_gf2_array<lambda>();
        auto frob30_s2_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob60_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 60>(),
                        s2_input_bits.data(), state);
        lynx_gf2_matvec(frob30_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 30>(),
                        s2_input_bits.data(), state);
        auto qs_frob60_s2 = lynx_combine_lambda_bits(frob60_s2_bits.data());
        auto qs_frob30_s2 = lynx_combine_lambda_bits(frob30_s2_bits.data());
        // Constraint: frob60(s2_input)*s2_input + frob30(s2_input)*v2 = 0
        state->add_constraint(qs_frob60_s2 * qs_s2_input + qs_frob30_s2 * qs_v2);
    }
    else if constexpr (S == secpar::s512)
    {
        // S1: v1 = s1^e where e = (2^12-2^6+1)^(-1) mod (2^512-1), s1 = k+c0.
        // v1^(2^12-2^6+1) = s1 => v1^(2^12+1) = s1 * v1^(2^6)
        // => frob12(v1) * v1 = s1 * frob6(v1)
        auto frob12_v1_bits = state->template const_gf2_array<lambda>();
        auto frob6_v1_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob12_v1_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 12>(),
                        v1_bits.data(), state);
        lynx_gf2_matvec(frob6_v1_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 6>(),
                        v1_bits.data(), state);
        auto qs_frob12_v1 = lynx_combine_lambda_bits(frob12_v1_bits.data());
        auto qs_frob6_v1 = lynx_combine_lambda_bits(frob6_v1_bits.data());
        state->add_constraint(qs_frob12_v1 * qs_v1 + qs_frob6_v1 * qs_s1_input);

        // S2: v2 = s2_input^(2^180-2^90+1)
        // => v2 * frob90(s2_input) = frob180(s2_input) * s2_input
        auto frob180_s2_bits = state->template const_gf2_array<lambda>();
        auto frob90_s2_bits = state->template const_gf2_array<lambda>();
        lynx_gf2_matvec(frob180_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 180>(),
                        s2_input_bits.data(), state);
        lynx_gf2_matvec(frob90_s2_bits.data(), lynx::lynx_frobenius_matrix_const<lambda, 90>(),
                        s2_input_bits.data(), state);
        auto qs_frob180_s2 = lynx_combine_lambda_bits(frob180_s2_bits.data());
        auto qs_frob90_s2 = lynx_combine_lambda_bits(frob90_s2_bits.data());
        state->add_constraint(qs_frob180_s2 * qs_s2_input + qs_frob90_s2 * qs_v2);
    }

    auto qs_y = quicksilver_gfsecpar<QS>(poly_secpar<S>::load(pk->owf_output), state);
    if constexpr (S == secpar::s256 || S == secpar::s384 || S == secpar::s512)
    {
        auto qs_c2 = quicksilver_gfsecpar<QS>(poly_secpar<S>::load(mats.C2.data()), state);
        auto qs_C = qs_L3_sum + qs_y;
        auto qs_kc2 = qs_k + qs_c2;
        state->add_constraint(qs_L1v1 * qs_L2v2 + qs_C * qs_kc2);
    }
    else
    {
        state->add_constraint(qs_L1v1 * qs_L2v2 + qs_L3_sum + qs_y);
    }
}

template <typename P, bool verifier>
void owf_constraints(quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE>* state,
                     const public_key<P>* pk)
{
    constexpr auto S = P::secpar_v;
    constexpr size_t lambda_bytes = secpar_to_bytes(S);
    auto mats = std::make_unique<lynx::lynx_matrices<S>>();
    lynx::lynx_init(*mats, pk->seed, lambda_bytes);
    lynx_owf_constraints<P>(state, pk, *mats);
}

template <typename P>
void owf_constraints(quicksilver_state<P::secpar_v, false, P::OWF_CONSTS::QS_DEGREE>* state,
                     const secret_key<P>* sk)
{
    if constexpr (is_owf_with_lynx(P::owf_v))
        lynx_owf_constraints<P>(state, &sk->pk, sk->lynx_mats);
    else
        owf_constraints<P, false>(state, &sk->pk);
}

} // namespace sig

#endif
