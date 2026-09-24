#ifndef GALAS_QS_GADGETS_HPP
#define GALAS_QS_GADGETS_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "parameters.hpp"
#include "polynomials.hpp"
#include "quicksilver.hpp"
#include "gfsmall.hpp"
#include "galas_params_generated.hpp"
namespace faest {

// -----------------------------
// Table selector
// -----------------------------
template <secpar S>
struct galas_bprime_table {
    static constexpr size_t m = galas_params<S>::m;
    static constexpr size_t nb = galas_params<S>::nb;
    static constexpr const uint8_t (*rows)[nb] = galas_params<S>::bprime;
};

// ============================================================
// 0) Bit/Field mapping helpers (ToField / ToBits per your spec)
// ============================================================
// Here we interpret bytes as coefficients over basis {α^i} with α being the
// polynomial basis generator for GF(2^k). In FAEST codebase poly_secpar<S>::load/store1
// is already such a ToField/ToBits mapping for k = secpar bits.

template <secpar S>
ALWAYS_INLINE poly_secpar<S> ToField(const uint8_t* bits_le_bytes) {
    return poly_secpar<S>::load(bits_le_bytes);
}

template <secpar S>
ALWAYS_INLINE void ToBits(uint8_t* out_bits_le_bytes, const poly_secpar<S>& x) {
    x.store(out_bits_le_bytes);
}

// ============================================================
// 1) Gala public constants c0,c1,c2
// ============================================================

template <secpar S>
struct gala_constants {
    static constexpr const std::array<uint8_t, secpar_to_bytes(S)>& c0 = galas_params<S>::c0;
    static constexpr const std::array<uint8_t, secpar_to_bytes(S)>& c1 = galas_params<S>::c1;
    static constexpr const std::array<uint8_t, secpar_to_bytes(S)>& c2 = galas_params<S>::c2;
};
// ============================================================
// 2) GF(2^n) arithmetic helpers
// ============================================================

template <secpar S>
ALWAYS_INLINE poly_secpar<S> gf_mul(const poly_secpar<S>& a, const poly_secpar<S>& b) {
    return (a * b).template reduce_to<secpar_to_bits(S)>();
}

template <secpar S>
ALWAYS_INLINE poly_secpar<S> gf_square(const poly_secpar<S>& a) {
    return (a * a).template reduce_to<secpar_to_bits(S)>();
}

template <secpar S>
ALWAYS_INLINE bool bytes_is_zero(const uint8_t* x) {
    for (size_t i = 0; i < secpar_to_bytes(S); ++i) if (x[i] != 0) return false;
    return true;
}

ALWAYS_INLINE uint8_t get_bit_le(const uint8_t* bytes, size_t bit_idx) {
    return (uint8_t)((bytes[bit_idx / 8] >> (bit_idx % 8)) & 1u);
}

ALWAYS_INLINE void set_bit_le(uint8_t* bytes, size_t bit_idx, uint8_t v01) {
    const uint8_t m = (uint8_t)(1u << (bit_idx % 8));
    uint8_t& b = bytes[bit_idx / 8];
    b = (uint8_t)((b & ~m) | ((v01 ? 1u : 0u) << (bit_idx % 8)));
}
// ============================================================
// 3) GF(2^n) inversion: reuse poly operations (a^{2^n-2})
//    (Yes, FAEST originally has inverses in gfsmall_impl for GF(256)/GF(16) etc,
//     but for GF(2^128/192/256) you normally do exponentiation or Almost-Inverse.
//     Here we keep a single implementation.)
// ============================================================

template <secpar S>
inline poly_secpar<S> gf_pow_2n_minus_2(poly_secpar<S> a) {
    constexpr size_t n = secpar_to_bits(S);

    // exponent = 2^n - 2 = (111..110)_2 (n bits)
    // square-and-multiply scanning low->high bits:
    poly_secpar<S> acc = poly_secpar<S>::from_1(1);

    // bit0 is 0, bits 1..n-1 are 1.
    // So we can do:
    // for bit=0..n-1:
    //   if bit != 0: acc*=a
    //   a = a^2
    for (size_t bit = 0; bit < n; ++bit) {
        if (bit != 0) acc = gf_mul<S>(acc, a);
        a = gf_square<S>(a);
    }
    return acc;
}

template <secpar S>
inline void gala_gf2n_inv(uint8_t* out, const uint8_t* in) {
    constexpr size_t nb = secpar_to_bytes(S);
    if (bytes_is_zero<S>(in)) { std::memset(out, 0, nb); return; }

    poly_secpar<S> a = ToField<S>(in);
    poly_secpar<S> inv = gf_pow_2n_minus_2<S>(a);
    ToBits<S>(out, inv);
}

// ============================================================
// 4) Frobenius power X^{2^j} by j squarings
// ============================================================

template <secpar S>
ALWAYS_INLINE poly_secpar<S> frob_2j(poly_secpar<S> x, size_t j) {
    for (size_t t = 0; t < j; ++t) x = gf_square<S>(x);
    return x;
}

// ============================================================
// 5) Linearized polynomial layer L(X)=Σ a_j X^{2^j}, a_j ∈ F*
// ============================================================

static ALWAYS_INLINE void store_u64_le(uint8_t* dst, uint64_t x, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) dst[i] = (uint8_t)(x >> (8 * i));
}

template <secpar S, int MAP_ID>
inline const std::array<poly_secpar<S>, secpar_to_bits(S)>& gala_map_coeffs() {
    constexpr size_t n  = secpar_to_bits(S);
    constexpr size_t nb = secpar_to_bytes(S);
    constexpr size_t nw = galas_params<S>::nw;

    static const std::array<poly_secpar<S>, n> coeffs = [](){
        std::array<poly_secpar<S>, n> a{};

        const auto& L =
            []() -> const std::array<std::array<std::uint64_t, nw>, n>& {
                if constexpr (MAP_ID == 0) return galas_params<S>::M0;
                else if constexpr (MAP_ID == 1) return galas_params<S>::M1;
                else if constexpr (MAP_ID == 2) return galas_params<S>::M2;
                else if constexpr (MAP_ID == 3) return galas_params<S>::L0;
                else if constexpr (MAP_ID == 4) return galas_params<S>::L1;
                else if constexpr (MAP_ID == 5) return galas_params<S>::L2;
                else return galas_params<S>::L3;
            }();

        for (size_t i = 0; i < n; ++i) {
            std::array<uint8_t, nb> buf{};
            for (size_t w = 0; w < nw; ++w) {
                store_u64_le(buf.data() + 8 * w, L[i][w], std::min<size_t>(8, nb - 8 * w));
            }
            a[i] = poly_secpar<S>::load(buf.data());
        }
        return a;
    }();

    return coeffs;
}

template <secpar S, int L_ID>
inline const std::array<poly_secpar<S>, secpar_to_bits(S)>& gala_linear_coeffs() {
    static_assert(0 <= L_ID && L_ID < 4);
    return gala_map_coeffs<S, L_ID + 3>();
}

template <secpar S, int M_ID>
inline const std::array<poly_secpar<S>, secpar_to_bits(S)>& gala_input_coeffs() {
    static_assert(0 <= M_ID && M_ID < 3);
    return gala_map_coeffs<S, M_ID>();
}



template <secpar S, int ID>
inline void gala_linear_layer(uint8_t* out, const uint8_t* in) {
    constexpr size_t n = secpar_to_bits(S);

    poly_secpar<S> x   = ToField<S>(in);
    poly_secpar<S> acc = poly_secpar<S>::set_zero();

    const auto& a = gala_linear_coeffs<S, ID>();

    // xpow = X^{2^j} iteratively:
    // xpow_0 = X
    // xpow_{j+1} = (xpow_j)^2 = X^{2^{j+1}}
    poly_secpar<S> xpow = x;

    for (size_t j = 0; j < n; ++j) {
        acc += gf_mul<S>(a[j], xpow);
        xpow = gf_square<S>(xpow);
    }

    ToBits<S>(out, acc);
}

template <secpar S, int ID>
inline void gala_input_layer(uint8_t* out, const uint8_t* in) {
    constexpr size_t n = secpar_to_bits(S);

    poly_secpar<S> x   = ToField<S>(in);
    poly_secpar<S> acc = poly_secpar<S>::set_zero();

    const auto& a = gala_input_coeffs<S, ID>();
    poly_secpar<S> xpow = x;
    for (size_t j = 0; j < n; ++j) {
        acc += gf_mul<S>(a[j], xpow);
        xpow = gf_square<S>(xpow);
    }

    ToBits<S>(out, acc);
}
// ============================================================
// 6) InvNorm / subfield compression stub
//    Your spec: y' = ToBits(1/x^{2^{λ/2}+1}), then permute bits.
//    Here we provide a stub interface to avoid duplicates;
//    Replace internals with your basis B'_i and bit-permutation table.
// ============================================================

template <secpar S>
inline void gala_invnorm(uint8_t* out_half, const uint8_t* in_full) {
    constexpr size_t nb = secpar_to_bytes(S);
    constexpr size_t halfb = nb / 2;
    constexpr size_t m_bits = secpar_to_bits(S) / 2;

    if (bytes_is_zero<S>(in_full)) {
        std::memset(out_half, 0, halfb);
        return;
    }

    poly_secpar<S> x = ToField<S>(in_full);
    poly_secpar<S> t = x;
    for (size_t i = 0; i < m_bits; ++i)
        t = gf_square<S>(t);

    poly_secpar<S> u = gf_mul<S>(x, t);

    std::array<uint8_t, nb> u_bytes{};
    std::array<uint8_t, nb> v_bytes{};
    ToBits<S>(u_bytes.data(), u);
    gala_gf2n_inv<S>(v_bytes.data(), u_bytes.data());

    std::memset(out_half, 0, halfb);
    for (size_t j = 0; j < m_bits; ++j) {
        uint16_t src_idx = galas_params<S>::perm[j];
        uint8_t b = get_bit_le(v_bytes.data(), src_idx);
        set_bit_le(out_half, j, b);
    }
}

// ============================================================
// 7) Gala permutation evaluation
// y = S( L0(S(a0)) ⊕ L1(S(a1)) ⊕ L2(S(a2)) ) ⊕ L3(k)
// a0=x⊕k⊕c0, a1=k⊕c1, a2=k⊕c2
// ============================================================

template <secpar S>
// Computes a_i = M_i(rho_i) for the three wide inverse inputs.
inline void gala_wide_inputs(
    std::array<uint8_t, secpar_to_bytes(S)>& a0,
    std::array<uint8_t, secpar_to_bytes(S)>& a1,
    std::array<uint8_t, secpar_to_bytes(S)>& a2,
    const uint8_t* x,
    const uint8_t* k) {
    constexpr size_t nb = secpar_to_bytes(S);

    std::array<uint8_t, nb> r0{}, r1{}, r2{};
    const auto& C0 = gala_constants<S>::c0;
    const auto& C1 = gala_constants<S>::c1;
    const auto& C2 = gala_constants<S>::c2;

    for (size_t i = 0; i < nb; ++i) {
        r0[i] = x[i] ^ k[i] ^ C0[i];
        r1[i] = k[i] ^ C1[i];
        r2[i] = k[i] ^ C2[i];
    }

    gala_input_layer<S, 0>(a0.data(), r0.data());
    gala_input_layer<S, 1>(a1.data(), r1.data());
    gala_input_layer<S, 2>(a2.data(), r2.data());
}

template <secpar S>
inline bool gala_eval_from_wide_inputs(
    uint8_t* out,
    const uint8_t* k,
    const std::array<uint8_t, secpar_to_bytes(S)>& a0,
    const std::array<uint8_t, secpar_to_bytes(S)>& a1,
    const std::array<uint8_t, secpar_to_bytes(S)>& a2,
    bool reject_zero_inputs) {
    constexpr size_t nb = secpar_to_bytes(S);

    if (reject_zero_inputs &&
        (bytes_is_zero<S>(a0.data()) || bytes_is_zero<S>(a1.data()) || bytes_is_zero<S>(a2.data()))) {
        return false;
    }

    std::array<uint8_t, nb> b0{}, b1{}, b2{};
    gala_gf2n_inv<S>(b0.data(), a0.data());
    gala_gf2n_inv<S>(b1.data(), a1.data());
    gala_gf2n_inv<S>(b2.data(), a2.data());

    std::array<uint8_t, nb> l0{}, l1{}, l2{};
    gala_linear_layer<S, 0>(l0.data(), b0.data());
    gala_linear_layer<S, 1>(l1.data(), b1.data());
    gala_linear_layer<S, 2>(l2.data(), b2.data());

    std::array<uint8_t, nb> z{};
    for (size_t i = 0; i < nb; ++i) z[i] = l0[i] ^ l1[i] ^ l2[i];

    if (reject_zero_inputs && bytes_is_zero<S>(z.data())) {
        return false;
    }

    std::array<uint8_t, nb> t{};
    gala_gf2n_inv<S>(t.data(), z.data());

    std::array<uint8_t, nb> lk{};
    gala_linear_layer<S, 3>(lk.data(), k);

    for (size_t i = 0; i < nb; ++i) out[i] = t[i] ^ lk[i];
    return true;
}

template <secpar S>
inline bool gala_eval_checked(uint8_t* out, const uint8_t* x, const uint8_t* k) {
    constexpr size_t nb = secpar_to_bytes(S);
    std::array<uint8_t, nb> a0{}, a1{}, a2{};
    gala_wide_inputs<S>(a0, a1, a2, x, k);
    return gala_eval_from_wide_inputs<S>(out, k, a0, a1, a2, true);
}

template <secpar S>
inline void gala_eval(uint8_t* out, const uint8_t* x, const uint8_t* k) {
    constexpr size_t nb = secpar_to_bytes(S);
    std::array<uint8_t, nb> a0{}, a1{}, a2{};
    gala_wide_inputs<S>(a0, a1, a2, x, k);
    (void)gala_eval_from_wide_inputs<S>(out, k, a0, a1, a2, false);
}
// -----------------------------
// Helpers: build poly_secpar bit-constants
// -----------------------------
template <secpar S>
inline poly_secpar<S> poly_from_single_bit(size_t bit_idx)
{
    constexpr size_t nb = secpar_to_bytes(S);
    std::array<uint8_t, nb> tmp{};
    tmp[bit_idx / 8] = uint8_t(1u << (bit_idx % 8));
    return poly_secpar<S>::load(tmp.data());
}

template <typename QS>
inline void load_witness_bits(const QS& st, size_t start_bit, quicksilver_gf2<QS,1>* out, size_t nbits)
{
    for (size_t i = 0; i < nbits; ++i) out[i] = st.get_witness_bit(start_bit + i);
}

// σ embedding
template <typename QS>
inline quicksilver_gfsecpar<QS,1> qs_embed_sigma_from_bprime(const QS& st, size_t start_bit)
{
    constexpr secpar S = QS::secpar_v;
    constexpr size_t nb = secpar_to_bytes(S);
    constexpr size_t m = galas_bprime_table<S>::m;

    quicksilver_gfsecpar<QS,1> acc(0, &st);

    for (size_t i = 0; i < m; ++i)
    {
        auto b = st.get_witness_bit(start_bit + i);

        std::array<uint8_t, nb> tmp{};
        std::memcpy(tmp.data(), galas_bprime_table<S>::rows[i], nb);

        auto Bi = quicksilver_gfsecpar<QS,0>(poly_secpar<S>::load(tmp.data()), &st);
        acc += quicksilver_gfsecpar<QS,1>(b) * Bi;
    }
    return acc;
}

// -----------------------------
// Linear maps on k bits (precompute T(e_i) constants)
// -----------------------------

template <typename QS>
inline quicksilver_gfsecpar<QS,1> apply_linear_on_k_bits(
    const QS& st,
    const quicksilver_gf2<QS,1>* k_bits,
    const poly_secpar<QS::secpar_v>* tbl)
{
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);
    quicksilver_gfsecpar<QS,1> acc(0, &st);
    for (size_t i = 0; i < n; ++i)
    {
        auto Ti = quicksilver_gfsecpar<QS,0>(tbl[i], &st);
        acc += quicksilver_gfsecpar<QS,1>(k_bits[i]) * Ti;
    }
    return acc;
}

template <secpar S, int MAP_ID>
inline const std::array<std::array<poly_secpar<S>, secpar_to_bits(S)>, secpar_to_bits(S)>&
gala_map_coeffs_frob() {
    constexpr size_t n = secpar_to_bits(S);
    static const std::array<std::array<poly_secpar<S>, n>, n> table = [](){
        std::array<std::array<poly_secpar<S>, n>, n> out{};
        const auto& coeffs = gala_map_coeffs<S, MAP_ID>();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                out[i][j] = frob_2j<S>(coeffs[j], i);
            }
        }
        return out;
    }();
    return table;
}

template <typename QS, int M_ID>
inline k_frob_cache_t<QS, 1>
qs_precompute_gala_input_frob_all(
    const QS& st,
    const k_frob_cache_t<QS, 1>& k_frob,
    const const_frob_cache_t<QS>& c_frob)
{
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);
    const auto& coeffs_frob = gala_map_coeffs_frob<S, M_ID>();

    k_frob_cache_t<QS, 1> a_frob;
    for (size_t i = 0; i < n; ++i) {
        quicksilver_gfsecpar<QS, 1> acc(0, &st);
        for (size_t j = 0; j < n; ++j) {
            auto rho_frob = qs_rho_frob_from_caches<QS, 1>(st, k_frob, c_frob, (i + j) % n);
            quicksilver_gfsecpar<QS, 0> coeff(coeffs_frob[i][j], &st);
            acc += coeff * rho_frob;
        }
        a_frob[i] = acc;
    }
    return a_frob;
}

// ============================================================
// Galas linear layer
// ============================================================

template <typename QS, int CONST_ID>
inline quicksilver_gfsecpar<QS, 2>
apply_galas_linear_layer_impl(
    const QS& st,
    const quicksilver_gf2<QS,1>* sigma_bits,
    const quicksilver_gf2<QS,1>* k_bits,       // len = λ
    const k_frob_cache_t<QS,1>& a_frob,
    const const_frob_cache_t<QS>& c_frob       // NEW: for THIS rho_const
) {
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);
    (void)k_bits;
    (void)c_frob;

    const auto& coeffs = gala_linear_coeffs<S, CONST_ID>();

    quicksilver_gfsecpar<QS, 2> acc(0, &st);

    for (size_t i = 0; i < n; ++i)
    {
        auto sigma_2i =
            qs_sigma_pow2i_from_bprime_bits<QS,1>(st, sigma_bits, i);

        const size_t idx = (i + (n/2)) % n;

        // OLD (hot):
        // auto rho_2m_i = qs_rho_pow2i_from_k_bits_epow2<QS,1>(st, k_bits, rho_const, idx);

        auto a_2m_i = a_frob[idx];
        auto y_2i = sigma_2i * a_2m_i; // deg = 2

        quicksilver_gfsecpar<QS,0> a(coeffs[i], &st);
        acc += a * y_2i;
    }

    return acc;
}

template <typename QS>
inline quicksilver_gfsecpar<QS, 2>
galas_linear_layer_ID(
    const QS& st,
    int ID,
    const quicksilver_gf2<QS,1>* sigma_bits,
    const quicksilver_gf2<QS,1>* k_bits,
    const k_frob_cache_t<QS,1>& k_frob,
    const const_frob_cache_t<QS>& c_frob
) {
    switch (ID) {
        case 0: return apply_galas_linear_layer_impl<QS,0>(st, sigma_bits, k_bits, k_frob, c_frob);
        case 1: return apply_galas_linear_layer_impl<QS,1>(st, sigma_bits, k_bits, k_frob, c_frob);
        case 2: return apply_galas_linear_layer_impl<QS,2>(st, sigma_bits, k_bits, k_frob, c_frob);
        default:
            return quicksilver_gfsecpar<QS, 2>(0, &st);
    }
}


template <typename QS, int CONST_ID>
inline quicksilver_gfsecpar<QS, 2>
apply_galas_linear_layer_from_a_frob_impl(
    const QS& st,
    const quicksilver_gf2<QS,1>* sigma_bits,
    const k_frob_cache_t<QS,1>& a_frob
) {
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);

    const auto& coeffs = gala_linear_coeffs<S, CONST_ID>();
    quicksilver_gfsecpar<QS, 2> acc(0, &st);

    for (size_t i = 0; i < n; ++i) {
        auto sigma_2i =
            qs_sigma_pow2i_from_bprime_bits<QS,1>(st, sigma_bits, i);

        const size_t idx = (i + (n / 2)) % n;
        auto y_2i = sigma_2i * a_frob[idx];

        quicksilver_gfsecpar<QS,0> a(coeffs[i], &st);
        acc += a * y_2i;
    }

    return acc;
}

template <typename QS>
inline quicksilver_gfsecpar<QS, 2>
galas_linear_layer_from_a_frob_ID(
    const QS& st,
    int ID,
    const quicksilver_gf2<QS,1>* sigma_bits,
    const k_frob_cache_t<QS,1>& a_frob
) {
    switch (ID) {
        case 0: return apply_galas_linear_layer_from_a_frob_impl<QS,0>(st, sigma_bits, a_frob);
        case 1: return apply_galas_linear_layer_from_a_frob_impl<QS,1>(st, sigma_bits, a_frob);
        case 2: return apply_galas_linear_layer_from_a_frob_impl<QS,2>(st, sigma_bits, a_frob);
        default:
            return quicksilver_gfsecpar<QS, 2>(0, &st);
    }
}


template <typename QS, size_t deg>
ALWAYS_INLINE quicksilver_gfsecpar<QS, deg>
qs_k_pow2i_from_k_bits_epow2(
    const QS& st,
    const quicksilver_gf2<QS, deg>* k_bits,
    size_t i
){
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);
    FAEST_ASSERT(i < n);

    const auto& tab = galas_frobenius_tables<S>::instance();

    quicksilver_gfsecpar<QS, deg> acc(0, &st);
    for (size_t t = 0; t < n; ++t)
    {
        quicksilver_gfsecpar<QS,0> Et(tab.epow2[i][t], &st);
        acc += quicksilver_gfsecpar<QS, deg>(k_bits[t]) * Et;
    }
    return acc;
}

template <typename QS, int CONST_ID>
inline quicksilver_gfsecpar<QS, 1>
apply_galas_linear_layer_on_k_only(
    const QS& st,
    const k_frob_cache_t<QS,1>& k_frob   // NEW
){
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);

    const auto& coeffs = gala_linear_coeffs<S, CONST_ID>();

    quicksilver_gfsecpar<QS, 1> acc(0, &st);

    for (size_t i = 0; i < n; ++i)
    {
        // k^{2^i} already cached
        const auto& k_2i = k_frob[i]; // deg1
        quicksilver_gfsecpar<QS,0> a(coeffs[i], &st);
        acc += a * k_2i;
    }
    return acc;
}

template <typename QS>
inline quicksilver_gfsecpar<QS, 1>
galas_linear_layer_L3k(
    const QS& st,
    const k_frob_cache_t<QS,1>& k_frob
){
    return apply_galas_linear_layer_on_k_only<QS, 3>(st, k_frob);
}





static inline void compress_by_P256(uint8_t out_half[16], const uint8_t v_bytes[32]) {
    std::memset(out_half, 0, 16);
    for (size_t j = 0; j < 128; ++j) {
        uint16_t src = galas_params<secpar::s256>::perm[j];
        uint8_t b = get_bit_le(v_bytes, src);
        set_bit_le(out_half, j, b);
    }
}

static inline int first_diff_bit_128(const uint8_t a[16], const uint8_t b[16]) {
    for (int i = 0; i < 128; ++i) {
        if (get_bit_le(a, i) != get_bit_le(b, i))
            return i;
    }
    return -1;
}

static inline void dump_hex(const char* tag, const uint8_t* x, size_t n) {
    std::cerr << tag << ": ";
    for (size_t i = 0; i < n; ++i)
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << (unsigned)x[i];
    std::cerr << std::dec << "\n";
}
template <secpar S>
inline poly_secpar<S> embed_from_witness_bits(const uint8_t* u_halfbytes)
{
    constexpr size_t n  = secpar_to_bits(S);
    constexpr size_t m  = n / 2;
    constexpr size_t nb = secpar_to_bytes(S);

    poly_secpar<S> acc = poly_secpar<S>::set_zero();

    for (size_t i = 0; i < m; ++i)
    {
        const uint8_t bi = (u_halfbytes[i / 8] >> (i % 8)) & 1u;
        if (!bi) continue;

        std::array<uint8_t, nb> row{};
        std::memcpy(row.data(), galas_params<S>::bprime[i], nb);

        acc += poly_secpar<S>::load(row.data());
    }

    return acc;
}


}

#endif
