#ifndef LYNX_OWF_HPP
#define LYNX_OWF_HPP

// Lynx One-Way Function Implementation
// Ported from lynx-sig/lynx_owf.c

#include "lynx_constants.hpp"
#include "lynx_matrices.hpp"
#include "parameters.hpp"
#include <cstdint>

namespace sig
{
namespace lynx
{

/*
 * Lynx OWF circuit:
 *   v_1 = S_1(k  XOR  c_0)
 *   v_2 = S_2(L_0(k)  XOR  c_1)   [for lambda != 160: S2 uses c1]
 *        = S_2(L_0(k)  XOR  c_0)   [for lambda == 160: S2 uses c0]
 *   y   = (L_1(v_1) * L_2(v_2)) XOR L_3(k XOR v_1 XOR v_2)  [for lambda <= 192]
 *       = (k+c_2)^(-1) * (L_1(v_1) * L_2(v_2)) XOR L_3(k XOR v_1 XOR v_2)  [for lambda >= 256]
 *
 * Witness:  w = k || v_1 || v_2   [3*lambda bits]
 */

// OWF evaluation - computes y = OWF(k, seed)
template <secpar S>
inline void owf_lynx(uint8_t* output, const uint8_t* key, const uint8_t* seed, std::size_t seed_len)
{
    using gf = poly_secpar<S>;
    constexpr std::size_t N = secpar_to_bits(S);
    constexpr std::size_t W = (N + 63) / 64;

    lynx_matrices<S> mats;
    lynx_init(mats, seed, seed_len);

    gf k = gf::load(key);
    gf c0 = gf::load(mats.C0.data());

    // v1 = S1(k XOR c0)
    gf v1 = lynx_poly_s1<S>(k + c0);

    // L0k = L0(k), then v2
    gf L0k = lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L0.data()), k);
    gf v2;
    if constexpr (S == secpar::s160)
    {
        // 160-bit: S2 uses c0 (same as S1)
        v2 = lynx_poly_s2<S>(L0k + c0);
    }
    else
    {
        gf c1 = gf::load(mats.C1.data());
        v2 = lynx_poly_s2<S>(L0k + c1);
    }

    // y = (L1(v1) * L2(v2)) XOR L3(k XOR v1 XOR v2)   [lambda <= 192]
    //   = (k+c2)^(-1) * (L1(v1) * L2(v2)) XOR L3(k XOR v1 XOR v2)  [lambda >= 256]
    gf L1v1 = lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L1.data()), v1);
    gf L2v2 = lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L2.data()), v2);
    gf L3_sum = lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L3.data()), k + v1 + v2);

    gf y;
    if constexpr (S == secpar::s256 || S == secpar::s384 || S == secpar::s512)
    {
        gf c2 = gf::load(mats.C2.data());
        gf inv_kc2 = [&]() {
            if constexpr (S == secpar::s256)
                return lynx_poly_inv_256(k + c2);
            else if constexpr (S == secpar::s384)
                return lynx_poly_inv_384(k + c2);
            else
                return lynx_poly_inv_512(k + c2);
        }();
        y = lynx_poly_mul<S>(inv_kc2, lynx_poly_mul<S>(L1v1, L2v2)) + L3_sum;
    }
    else
    {
        y = lynx_poly_mul<S>(L1v1, L2v2) + L3_sum;
    }
    y.store(output);
}

// Witness extension - computes w = k || v1 || v2
template <secpar S>
inline void lynx_extend_witness(uint8_t* w, const uint8_t* key, const uint8_t* seed, std::size_t seed_len)
{
    using gf = poly_secpar<S>;
    constexpr std::size_t N = secpar_to_bits(S);
    constexpr std::size_t W = (N + 63) / 64;
    constexpr std::size_t lambda_bytes = N / 8;

    lynx_matrices<S> mats;
    lynx_init(mats, seed, seed_len);

    gf k = gf::load(key);
    gf c0 = gf::load(mats.C0.data());

    gf v1 = lynx_poly_s1<S>(k + c0);

    gf L0k = lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L0.data()), k);
    gf v2;
    if constexpr (S == secpar::s160)
    {
        v2 = lynx_poly_s2<S>(L0k + c0);
    }
    else
    {
        gf c1 = gf::load(mats.C1.data());
        v2 = lynx_poly_s2<S>(L0k + c1);
    }

    k.store(w);
    v1.store(w + lambda_bytes);
    v2.store(w + 2 * lambda_bytes);
}

} // namespace lynx
} // namespace sig

#endif
