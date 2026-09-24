#ifndef LYNX_CONSTANTS_HPP
#define LYNX_CONSTANTS_HPP

// Binary field arithmetic for Lynx OWF
// Ported from lynx-sig/fields.h

#include "parameters.hpp"
#include "polynomials.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace sig
{
namespace lynx
{

// Field modulus constants for different security levels
// GF(2^128) with X^128 + X^7 + X^2 + X^1 + 1
constexpr uint64_t bf128_modulus = (UINT64_C(1) << 7) | (UINT64_C(1) << 2) | (UINT64_C(1) << 1) | 1;
// GF(2^160) with X^160 + X^5 + X^3 + X^2 + 1
constexpr uint64_t bf160_modulus = (UINT64_C(1) << 5) | (UINT64_C(1) << 3) | (UINT64_C(1) << 2) | 1;
// GF(2^192) with X^192 + X^7 + X^2 + X^1 + 1
constexpr uint64_t bf192_modulus = (UINT64_C(1) << 7) | (UINT64_C(1) << 2) | (UINT64_C(1) << 1) | 1;
// GF(2^256) with X^256 + X^10 + X^5 + X^2 + 1
constexpr uint64_t bf256_modulus = (UINT64_C(1) << 10) | (UINT64_C(1) << 5) | (UINT64_C(1) << 2) | 1;
// GF(2^384) with X^384 + X^16 + X^15 + X^6 + 1
constexpr uint64_t bf384_modulus = (UINT64_C(1) << 16) | (UINT64_C(1) << 15) | (UINT64_C(1) << 6) | 1;
// GF(2^512) with X^512 + X^8 + X^5 + X^2 + 1
constexpr uint64_t bf512_modulus = (UINT64_C(1) << 8) | (UINT64_C(1) << 5) | (UINT64_C(1) << 2) | 1;

// Binary field element type
template <std::size_t N>
class bf_t
{
public:
    static constexpr std::size_t bits = N;
    static constexpr std::size_t num_words = (N + 63) / 64;
    std::array<uint64_t, num_words> data{};

    constexpr bf_t() = default;

    constexpr explicit bf_t(uint64_t v)
    {
        data[0] = v;
        for (std::size_t i = 1; i < num_words; ++i)
            data[i] = 0;
    }

    static constexpr bf_t zero()
    {
        bf_t r;
        for (std::size_t i = 0; i < num_words; ++i)
            r.data[i] = 0;
        return r;
    }

    static constexpr bf_t one()
    {
        bf_t r;
        r.data[0] = 1;
        for (std::size_t i = 1; i < num_words; ++i)
            r.data[i] = 0;
        return r;
    }

    static bf_t load(const uint8_t* src)
    {
        bf_t r;
        std::memcpy(r.data.data(), src, N / 8);
        return r;
    }

    void store(uint8_t* dst) const
    {
        std::memcpy(dst, data.data(), N / 8);
    }

    constexpr bf_t& operator^=(const bf_t& rhs)
    {
        for (std::size_t i = 0; i < num_words; ++i)
            data[i] ^= rhs.data[i];
        return *this;
    }

    constexpr bf_t operator^(const bf_t& rhs) const
    {
        bf_t r = *this;
        r ^= rhs;
        return r;
    }

    constexpr bool is_zero() const
    {
        for (std::size_t i = 0; i < num_words; ++i)
        {
            if (data[i] != 0)
                return false;
        }
        return true;
    }

    constexpr bool operator==(const bf_t& rhs) const
    {
        for (std::size_t i = 0; i < num_words; ++i)
        {
            if (data[i] != rhs.data[i])
                return false;
        }
        return true;
    }
};

// Type aliases for common field sizes
using bf128_t = bf_t<128>;
using bf160_t = bf_t<160>;
using bf192_t = bf_t<192>;
using bf256_t = bf_t<256>;
using bf384_t = bf_t<384>;
using bf512_t = bf_t<512>;

// Helper to get the high bit
template <std::size_t N>
inline uint64_t get_high_bit(const bf_t<N>& x)
{
    constexpr std::size_t high_word = (N - 1) / 64;
    constexpr std::size_t high_bit_pos = (N - 1) % 64;
    return (x.data[high_word] >> high_bit_pos) & 1;
}

// Left shift by 1 for binary field multiplication
template <std::size_t N>
inline void shift_left_1(bf_t<N>& x)
{
    constexpr std::size_t num_words = bf_t<N>::num_words;
    for (std::size_t i = num_words - 1; i > 0; --i)
    {
        x.data[i] = (x.data[i] << 1) | (x.data[i - 1] >> 63);
    }
    x.data[0] <<= 1;
}

// Binary field multiplication - generic implementation
template <std::size_t N>
bf_t<N> bf_mul(bf_t<N> lhs, const bf_t<N>& rhs);

template <>
inline bf128_t bf_mul<128>(bf128_t lhs, const bf128_t& rhs)
{
    bf128_t result;
    result.data[0] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[0];
    result.data[1] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[1];

    for (unsigned int idx = 1; idx < 128; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[1] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf128_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        result.data[0] ^= bit_mask & lhs.data[0];
        result.data[1] ^= bit_mask & lhs.data[1];
    }
    return result;
}

template <>
inline bf160_t bf_mul<160>(bf160_t lhs, const bf160_t& rhs)
{
    bf160_t result;
    for (std::size_t i = 0; i < 3; ++i)
        result.data[i] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[i];

    for (unsigned int idx = 1; idx < 160; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[2] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf160_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        for (std::size_t i = 0; i < 3; ++i)
            result.data[i] ^= bit_mask & lhs.data[i];
    }
    return result;
}

template <>
inline bf192_t bf_mul<192>(bf192_t lhs, const bf192_t& rhs)
{
    bf192_t result;
    for (std::size_t i = 0; i < 3; ++i)
        result.data[i] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[i];

    for (unsigned int idx = 1; idx < 192; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[2] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf192_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        for (std::size_t i = 0; i < 3; ++i)
            result.data[i] ^= bit_mask & lhs.data[i];
    }
    return result;
}

template <>
inline bf256_t bf_mul<256>(bf256_t lhs, const bf256_t& rhs)
{
    bf256_t result;
    for (std::size_t i = 0; i < 4; ++i)
        result.data[i] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[i];

    for (unsigned int idx = 1; idx < 256; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[3] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf256_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        for (std::size_t i = 0; i < 4; ++i)
            result.data[i] ^= bit_mask & lhs.data[i];
    }
    return result;
}

template <>
inline bf384_t bf_mul<384>(bf384_t lhs, const bf384_t& rhs)
{
    bf384_t result;
    for (std::size_t i = 0; i < 6; ++i)
        result.data[i] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[i];

    for (unsigned int idx = 1; idx < 384; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[5] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf384_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        for (std::size_t i = 0; i < 6; ++i)
            result.data[i] ^= bit_mask & lhs.data[i];
    }
    return result;
}

template <>
inline bf512_t bf_mul<512>(bf512_t lhs, const bf512_t& rhs)
{
    bf512_t result;
    for (std::size_t i = 0; i < 8; ++i)
        result.data[i] = -(static_cast<int64_t>(rhs.data[0] & 1)) & lhs.data[i];

    for (unsigned int idx = 1; idx < 512; ++idx)
    {
        const uint64_t mask = -(static_cast<int64_t>(lhs.data[7] >> 63) & 1);
        shift_left_1(lhs);
        lhs.data[0] ^= (mask & bf512_modulus);

        const uint64_t bit_mask = -(static_cast<int64_t>((rhs.data[idx / 64] >> (idx % 64)) & 1));
        for (std::size_t i = 0; i < 8; ++i)
            result.data[i] ^= bit_mask & lhs.data[i];
    }
    return result;
}

// Square operation using Itoh-Tsujii
template <std::size_t N>
bf_t<N> bf_sq(bf_t<N> x)
{
    return bf_mul<N>(x, x);
}

// Multiplication by bit
template <std::size_t N>
bf_t<N> bf_mul_bit(bf_t<N> x, uint8_t bit)
{
    if (bit & 1)
        return x;
    return bf_t<N>::zero();
}

// S-box functions for Lynx OWF.
// Ported from lynx-sig/lynx_owf.c.

template <std::size_t N>
bf_t<N> bf_s1(bf_t<N> x);
template <std::size_t N>
bf_t<N> bf_s2(bf_t<N> x);

// Generic binary exponentiation: x^e in GF(2^N)
template <std::size_t N, std::size_t W>
inline bf_t<N> bf_pow(bf_t<N> x, const uint64_t (&e)[W])
{
    bf_t<N> result = bf_t<N>::one();
    for (int i = static_cast<int>(N) - 1; i >= 0; --i)
    {
        result = bf_sq<N>(result);
        if ((e[i / 64] >> (i % 64)) & UINT64_C(1))
            result = bf_mul<N>(result, x);
    }
    return result;
}

// Raw Lynx exponent maps. S1/S2 wrappers below select the final assignment for each lambda.

// ==================== GF(2^160): x^(2^67-1) ====================
// S2: x^(2^67 - 1) via Itoh-Tsujii
// 67 = 64 + 3, so x^(2^67-1) = r64^(2^3) * r3 where r3 = r2^2 * r1
// NOTE: bf_inv_160 is defined after bf_inv_allones_exp below
inline bf160_t bf_pow_67_minus_1_160(bf160_t x)
{
    bf160_t t;

    bf160_t r1  = x;
    bf160_t r2  = bf_mul<160>(bf_sq<160>(r1), r1);
    t           = bf_sq<160>(bf_sq<160>(r2));
    bf160_t r4  = bf_mul<160>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = bf_sq<160>(t);
    bf160_t r8  = bf_mul<160>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = bf_sq<160>(t);
    bf160_t r16 = bf_mul<160>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = bf_sq<160>(t);
    bf160_t r32 = bf_mul<160>(t, r16);
    t = r32; for (int i = 0; i < 32; i++) t = bf_sq<160>(t);
    bf160_t r64 = bf_mul<160>(t, r32);
    bf160_t r3  = bf_mul<160>(bf_sq<160>(r2), r1);
    t = r64; for (int i = 0; i < 3; i++) t = bf_sq<160>(t);
    return bf_mul<160>(t, r3);
}

// ==================== GF(2^128): x^(-1) ====================
// x^(-1) = (x^(2^127 - 1))^2
inline bf128_t bf_inv_128(bf128_t x)
{
    bf128_t t;

    bf128_t r1  = x;
    bf128_t r2  = bf_mul<128>(bf_sq<128>(r1), r1);
    t           = bf_sq<128>(bf_sq<128>(r2));
    bf128_t r4  = bf_mul<128>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = bf_sq<128>(t);
    bf128_t r8  = bf_mul<128>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = bf_sq<128>(t);
    bf128_t r16 = bf_mul<128>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = bf_sq<128>(t);
    bf128_t r32 = bf_mul<128>(t, r16);
    t = r32; for (int i = 0; i < 32; i++) t = bf_sq<128>(t);
    bf128_t r64 = bf_mul<128>(t, r32);

    // Build r127 = x^(2^127 - 1)
    // 127 = 64+63, 63 = 32+31, 31 = 16+15, 15 = 8+7, 7 = 4+3, 3 = 2+1
    bf128_t r3  = bf_mul<128>(bf_sq<128>(r2), r1);
    t = r4; for (int i = 0; i < 3; i++) t = bf_sq<128>(t);
    bf128_t r7  = bf_mul<128>(t, r3);
    t = r8; for (int i = 0; i < 7; i++) t = bf_sq<128>(t);
    bf128_t r15 = bf_mul<128>(t, r7);
    t = r16; for (int i = 0; i < 15; i++) t = bf_sq<128>(t);
    bf128_t r31 = bf_mul<128>(t, r15);
    t = r32; for (int i = 0; i < 31; i++) t = bf_sq<128>(t);
    bf128_t r63 = bf_mul<128>(t, r31);
    t = r64; for (int i = 0; i < 63; i++) t = bf_sq<128>(t);
    bf128_t r127 = bf_mul<128>(t, r63);

    return bf_sq<128>(r127); // x^(2^128 - 2) = x^(-1)
}

// ==================== GF(2^192): x^(2^59-1) ====================
// Built via: 59 = 32+27, 27 = 16+11, 11 = 8+3, 3 = 2+1
inline bf192_t bf_pow_2_59_minus_1_192(bf192_t x)
{
    bf192_t t;

    bf192_t r1  = x;
    bf192_t r2  = bf_mul<192>(bf_sq<192>(r1), r1);
    t           = bf_sq<192>(bf_sq<192>(r2));
    bf192_t r4  = bf_mul<192>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = bf_sq<192>(t);
    bf192_t r8  = bf_mul<192>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = bf_sq<192>(t);
    bf192_t r16 = bf_mul<192>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = bf_sq<192>(t);
    bf192_t r32 = bf_mul<192>(t, r16);

    bf192_t r3  = bf_mul<192>(bf_sq<192>(r2), r1);
    t = r8; for (int i = 0; i < 3; i++) t = bf_sq<192>(t);
    bf192_t r11 = bf_mul<192>(t, r3);
    t = r16; for (int i = 0; i < 11; i++) t = bf_sq<192>(t);
    bf192_t r27 = bf_mul<192>(t, r11);
    t = r32; for (int i = 0; i < 27; i++) t = bf_sq<192>(t);
    return bf_mul<192>(t, r27); // x^(2^59 - 1)
}

// x^e where e = (2^13-1)^(-1) mod (2^192-1)
inline bf192_t bf_inv_2_13_minus_1_192(bf192_t x)
{
    static const uint64_t e[3] = {
        UINT64_C(0xb6ddb6edb76dbb6d),
        UINT64_C(0x6dbb6ddb6edb76db),
        UINT64_C(0xdb76dbb6ddb6edb7),
    };
    return bf_pow<192>(x, e);
}

// ==================== GF(2^256): x^(2^53-1) ====================
inline bf256_t bf_pow_2_53_minus_1_256(bf256_t x)
{
    bf256_t t;

    bf256_t r1  = x;
    bf256_t r2  = bf_mul<256>(bf_sq<256>(r1), r1);
    t           = bf_sq<256>(bf_sq<256>(r2));
    bf256_t r4  = bf_mul<256>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = bf_sq<256>(t);
    bf256_t r8  = bf_mul<256>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = bf_sq<256>(t);
    bf256_t r16 = bf_mul<256>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = bf_sq<256>(t);
    bf256_t r32 = bf_mul<256>(t, r16);

    t          = bf_sq<256>(r4);
    bf256_t r5 = bf_mul<256>(t, r1);
    t = r16; for (int i = 0; i < 5; i++) t = bf_sq<256>(t);
    bf256_t r21 = bf_mul<256>(t, r5);

    t = r32; for (int i = 0; i < 21; i++) t = bf_sq<256>(t);
    return bf_mul<256>(t, r21); // x^(2^53 - 1)
}

// x^e where e = (2^5-1)^(-1) mod (2^256-1)
inline bf256_t bf_inv_2_5_minus_1_256(bf256_t x)
{
    static const uint64_t e[4] = {
        UINT64_C(0xdef7bdef7bdef7bd),
        UINT64_C(0xbdef7bdef7bdef7b),
        UINT64_C(0x7bdef7bdef7bdef7),
        UINT64_C(0xf7bdef7bdef7bdef),
    };
    return bf_pow<256>(x, e);
}

// ==================== GF(2^384): x^(2^60-2^30+1) ====================
inline bf384_t bf_pow_2_60_minus_2_30_plus_1_384(bf384_t x)
{
    bf384_t t;

    bf384_t r1  = x;
    bf384_t r2  = bf_mul<384>(bf_sq<384>(r1), r1);
    t           = bf_sq<384>(bf_sq<384>(r2));
    bf384_t r4  = bf_mul<384>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = bf_sq<384>(t);
    bf384_t r8  = bf_mul<384>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = bf_sq<384>(t);
    bf384_t r16 = bf_mul<384>(t, r8);

    bf384_t r6  = bf_mul<384>(bf_sq<384>(bf_sq<384>(r4)), r2);
    t = r8; for (int i = 0; i < 6; i++) t = bf_sq<384>(t);
    bf384_t r14 = bf_mul<384>(t, r6);
    t = r16; for (int i = 0; i < 14; i++) t = bf_sq<384>(t);
    bf384_t r30 = bf_mul<384>(t, r14);

    t = r30; for (int i = 0; i < 30; i++) t = bf_sq<384>(t);
    return bf_mul<384>(t, x); // x^(2^60 - 2^30 + 1)
}

// x^e where e = (2^13-1)^(-1) mod (2^384-1)
inline bf384_t bf_inv_2_13_minus_1_384(bf384_t x)
{
    static const uint64_t e[6] = {
        UINT64_C(0xf7dfbefdf7efbf7d),
        UINT64_C(0xefbf7dfbefdf7efb),
        UINT64_C(0xdf7efbf7dfbefdf7),
        UINT64_C(0xbefdf7efbf7dfbef),
        UINT64_C(0x7dfbefdf7efbf7df),
        UINT64_C(0xfbf7dfbefdf7efbf),
    };
    return bf_pow<384>(x, e);
}

// ==================== GF(2^512): x^(2^180-2^90+1) ====================
inline bf512_t bf_pow_2_180_minus_2_90_plus_1_512(bf512_t x)
{
    static const uint64_t e[8] = {
        UINT64_C(0x0000000000000001),
        UINT64_C(0xfffffffffc000000),
        UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
    };
    return bf_pow<512>(x, e);
}

// x^e where e = (2^12-2^6+1)^(-1) mod (2^512-1)
inline bf512_t bf_inv_2_12_minus_2_6_plus_1_512(bf512_t x)
{
    static const uint64_t e[8] = {
        UINT64_C(0xc9898d9d9c9898d9),
        UINT64_C(0x898d9d9c9898d9d9),
        UINT64_C(0x8d9d9c9898d9d9c9),
        UINT64_C(0x9d9c9898d9d9c989),
        UINT64_C(0x9c9898d9d9c9898d),
        UINT64_C(0x9898d9d9c9898d9d),
        UINT64_C(0x98d9d9c9898d9d9c),
        UINT64_C(0xd9d9c9898d9d9c98),
    };
    return bf_pow<512>(x, e);
}

// Forward declarations for 160-bit (defined after bf_inv_allones_exp)
inline bf160_t bf_inv_160(bf160_t x);

// Final Lynx S-box assignments:
//   128: S1 = inv, S2 = inv
//   160: S1 = inv (all-ones exp), S2 = 2^67-1
//   192: S1 = inv((2^13-1)), S2 = 2^59-1
//   256: S1 = inv((2^5-1)),  S2 = 2^53-1
//   384: S1 = inv((2^13-1)), S2 = 2^60-2^30+1
//   512: S1 = inv((2^12-2^6+1)), S2 = 2^180-2^90+1

template <>
inline bf128_t bf_s1<128>(bf128_t x)
{
    return bf_inv_128(x);
}

template <>
inline bf128_t bf_s2<128>(bf128_t x)
{
    return bf_inv_128(x);
}

template <>
inline bf160_t bf_s1<160>(bf160_t x)
{
    return bf_inv_160(x);
}

template <>
inline bf160_t bf_s2<160>(bf160_t x)
{
    return bf_pow_67_minus_1_160(x);
}

template <>
inline bf192_t bf_s1<192>(bf192_t x)
{
    return bf_inv_2_13_minus_1_192(x);
}

template <>
inline bf192_t bf_s2<192>(bf192_t x)
{
    return bf_pow_2_59_minus_1_192(x);
}

template <>
inline bf256_t bf_s1<256>(bf256_t x)
{
    return bf_inv_2_5_minus_1_256(x);
}

template <>
inline bf256_t bf_s2<256>(bf256_t x)
{
    return bf_pow_2_53_minus_1_256(x);
}

template <>
inline bf384_t bf_s1<384>(bf384_t x)
{
    return bf_inv_2_13_minus_1_384(x);
}

template <>
inline bf384_t bf_s2<384>(bf384_t x)
{
    return bf_pow_2_60_minus_2_30_plus_1_384(x);
}

template <>
inline bf512_t bf_s1<512>(bf512_t x)
{
    return bf_inv_2_12_minus_2_6_plus_1_512(x);
}

template <>
inline bf512_t bf_s2<512>(bf512_t x)
{
    return bf_pow_2_180_minus_2_90_plus_1_512(x);
}

template <std::size_t N>
inline bf_t<N> bf_inv_allones_exp(bf_t<N> x)
{
    auto result = bf_t<N>::one();
    for (std::size_t i = N; i-- > 0;)
    {
        result = bf_sq<N>(result);
        if (i != 0)
            result = bf_mul<N>(result, x);
    }
    return result;
}

inline bf160_t bf_inv_160(bf160_t x) { return bf_inv_allones_exp<160>(x); }
inline bf256_t bf_inv_256(bf256_t x) { return bf_inv_allones_exp<256>(x); }
inline bf384_t bf_inv_384(bf384_t x) { return bf_inv_allones_exp<384>(x); }
inline bf512_t bf_inv_512(bf512_t x) { return bf_inv_allones_exp<512>(x); }

// Helper to select field type based on security parameter
template <secpar S>
struct secpar_to_bf;

template <>
struct secpar_to_bf<secpar::s128>
{
    using type = bf128_t;
    static constexpr std::size_t bits = 128;
};

template <>
struct secpar_to_bf<secpar::s160>
{
    using type = bf160_t;
    static constexpr std::size_t bits = 160;
};

template <>
struct secpar_to_bf<secpar::s192>
{
    using type = bf192_t;
    static constexpr std::size_t bits = 192;
};

template <>
struct secpar_to_bf<secpar::s256>
{
    using type = bf256_t;
    static constexpr std::size_t bits = 256;
};

template <>
struct secpar_to_bf<secpar::s384>
{
    using type = bf384_t;
    static constexpr std::size_t bits = 384;
};

template <>
struct secpar_to_bf<secpar::s512>
{
    using type = bf512_t;
    static constexpr std::size_t bits = 512;
};

template <secpar S>
using secpar_to_bf_t = typename secpar_to_bf<S>::type;

template <secpar S>
inline poly_secpar<S> bf_to_poly(const secpar_to_bf_t<S>& x)
{
    return poly_secpar<S>::load(x.data.data());
}

template <secpar S>
inline secpar_to_bf_t<S> bf_from_poly(const poly_secpar<S>& x)
{
    secpar_to_bf_t<S> out;
    x.store(out.data.data());
    return out;
}

template <secpar S>
inline poly_secpar<S> lynx_poly_mul(poly_secpar<S> lhs, const poly_secpar<S>& rhs)
{
    return (lhs * rhs).template reduce_to<secpar_to_bits(S)>();
}

template <secpar S>
inline poly_secpar<S> lynx_poly_sq(poly_secpar<S> x)
{
    return lynx_poly_mul<S>(x, x);
}

template <secpar S, std::size_t W>
inline poly_secpar<S> lynx_poly_pow(poly_secpar<S> x, const uint64_t (&e)[W])
{
    constexpr std::size_t N = secpar_to_bits(S);
    auto result = poly_secpar<S>::set_low32(1);
    for (int i = static_cast<int>(N) - 1; i >= 0; --i)
    {
        result = lynx_poly_sq<S>(result);
        if ((e[i / 64] >> (i % 64)) & UINT64_C(1))
            result = lynx_poly_mul<S>(result, x);
    }
    return result;
}

inline poly_secpar<secpar::s128> lynx_poly_inv_128(poly_secpar<secpar::s128> x)
{
    using gf = poly_secpar<secpar::s128>;
    gf t;

    gf r1 = x;
    gf r2 = lynx_poly_mul<secpar::s128>(lynx_poly_sq<secpar::s128>(r1), r1);
    t = lynx_poly_sq<secpar::s128>(lynx_poly_sq<secpar::s128>(r2));
    gf r4 = lynx_poly_mul<secpar::s128>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r8 = lynx_poly_mul<secpar::s128>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r16 = lynx_poly_mul<secpar::s128>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r32 = lynx_poly_mul<secpar::s128>(t, r16);
    t = r32; for (int i = 0; i < 32; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r64 = lynx_poly_mul<secpar::s128>(t, r32);

    gf r3 = lynx_poly_mul<secpar::s128>(lynx_poly_sq<secpar::s128>(r2), r1);
    t = r4; for (int i = 0; i < 3; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r7 = lynx_poly_mul<secpar::s128>(t, r3);
    t = r8; for (int i = 0; i < 7; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r15 = lynx_poly_mul<secpar::s128>(t, r7);
    t = r16; for (int i = 0; i < 15; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r31 = lynx_poly_mul<secpar::s128>(t, r15);
    t = r32; for (int i = 0; i < 31; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r63 = lynx_poly_mul<secpar::s128>(t, r31);
    t = r64; for (int i = 0; i < 63; i++) t = lynx_poly_sq<secpar::s128>(t);
    gf r127 = lynx_poly_mul<secpar::s128>(t, r63);

    return lynx_poly_sq<secpar::s128>(r127);
}

inline poly_secpar<secpar::s192> lynx_poly_pow_2_59_minus_1_192(poly_secpar<secpar::s192> x)
{
    using gf = poly_secpar<secpar::s192>;
    gf t;

    gf r1 = x;
    gf r2 = lynx_poly_mul<secpar::s192>(lynx_poly_sq<secpar::s192>(r1), r1);
    t = lynx_poly_sq<secpar::s192>(lynx_poly_sq<secpar::s192>(r2));
    gf r4 = lynx_poly_mul<secpar::s192>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = lynx_poly_sq<secpar::s192>(t);
    gf r8 = lynx_poly_mul<secpar::s192>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = lynx_poly_sq<secpar::s192>(t);
    gf r16 = lynx_poly_mul<secpar::s192>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = lynx_poly_sq<secpar::s192>(t);
    gf r32 = lynx_poly_mul<secpar::s192>(t, r16);

    gf r3 = lynx_poly_mul<secpar::s192>(lynx_poly_sq<secpar::s192>(r2), r1);
    t = r8; for (int i = 0; i < 3; i++) t = lynx_poly_sq<secpar::s192>(t);
    gf r11 = lynx_poly_mul<secpar::s192>(t, r3);
    t = r16; for (int i = 0; i < 11; i++) t = lynx_poly_sq<secpar::s192>(t);
    gf r27 = lynx_poly_mul<secpar::s192>(t, r11);
    t = r32; for (int i = 0; i < 27; i++) t = lynx_poly_sq<secpar::s192>(t);
    return lynx_poly_mul<secpar::s192>(t, r27);
}

inline poly_secpar<secpar::s192> lynx_poly_inv_2_13_minus_1_192(poly_secpar<secpar::s192> x)
{
    static const uint64_t e[3] = {
        UINT64_C(0xb6ddb6edb76dbb6d),
        UINT64_C(0x6dbb6ddb6edb76db),
        UINT64_C(0xdb76dbb6ddb6edb7),
    };
    return lynx_poly_pow<secpar::s192>(x, e);
}

inline poly_secpar<secpar::s256> lynx_poly_pow_2_53_minus_1_256(poly_secpar<secpar::s256> x)
{
    using gf = poly_secpar<secpar::s256>;
    gf t;

    gf r1 = x;
    gf r2 = lynx_poly_mul<secpar::s256>(lynx_poly_sq<secpar::s256>(r1), r1);
    t = lynx_poly_sq<secpar::s256>(lynx_poly_sq<secpar::s256>(r2));
    gf r4 = lynx_poly_mul<secpar::s256>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = lynx_poly_sq<secpar::s256>(t);
    gf r8 = lynx_poly_mul<secpar::s256>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = lynx_poly_sq<secpar::s256>(t);
    gf r16 = lynx_poly_mul<secpar::s256>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = lynx_poly_sq<secpar::s256>(t);
    gf r32 = lynx_poly_mul<secpar::s256>(t, r16);

    t = lynx_poly_sq<secpar::s256>(r4);
    gf r5 = lynx_poly_mul<secpar::s256>(t, r1);
    t = r16; for (int i = 0; i < 5; i++) t = lynx_poly_sq<secpar::s256>(t);
    gf r21 = lynx_poly_mul<secpar::s256>(t, r5);

    t = r32; for (int i = 0; i < 21; i++) t = lynx_poly_sq<secpar::s256>(t);
    return lynx_poly_mul<secpar::s256>(t, r21);
}

inline poly_secpar<secpar::s256> lynx_poly_inv_2_5_minus_1_256(poly_secpar<secpar::s256> x)
{
    static const uint64_t e[4] = {
        UINT64_C(0xdef7bdef7bdef7bd),
        UINT64_C(0xbdef7bdef7bdef7b),
        UINT64_C(0x7bdef7bdef7bdef7),
        UINT64_C(0xf7bdef7bdef7bdef),
    };
    return lynx_poly_pow<secpar::s256>(x, e);
}

inline poly_secpar<secpar::s384> lynx_poly_pow_2_60_minus_2_30_plus_1_384(poly_secpar<secpar::s384> x)
{
    using gf = poly_secpar<secpar::s384>;
    gf t;

    gf r1 = x;
    gf r2 = lynx_poly_mul<secpar::s384>(lynx_poly_sq<secpar::s384>(r1), r1);
    t = lynx_poly_sq<secpar::s384>(lynx_poly_sq<secpar::s384>(r2));
    gf r4 = lynx_poly_mul<secpar::s384>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = lynx_poly_sq<secpar::s384>(t);
    gf r8 = lynx_poly_mul<secpar::s384>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = lynx_poly_sq<secpar::s384>(t);
    gf r16 = lynx_poly_mul<secpar::s384>(t, r8);

    gf r6 = lynx_poly_mul<secpar::s384>(lynx_poly_sq<secpar::s384>(lynx_poly_sq<secpar::s384>(r4)), r2);
    t = r8; for (int i = 0; i < 6; i++) t = lynx_poly_sq<secpar::s384>(t);
    gf r14 = lynx_poly_mul<secpar::s384>(t, r6);
    t = r16; for (int i = 0; i < 14; i++) t = lynx_poly_sq<secpar::s384>(t);
    gf r30 = lynx_poly_mul<secpar::s384>(t, r14);

    t = r30; for (int i = 0; i < 30; i++) t = lynx_poly_sq<secpar::s384>(t);
    return lynx_poly_mul<secpar::s384>(t, x);
}

inline poly_secpar<secpar::s384> lynx_poly_inv_2_13_minus_1_384(poly_secpar<secpar::s384> x)
{
    static const uint64_t e[6] = {
        UINT64_C(0xf7dfbefdf7efbf7d),
        UINT64_C(0xefbf7dfbefdf7efb),
        UINT64_C(0xdf7efbf7dfbefdf7),
        UINT64_C(0xbefdf7efbf7dfbef),
        UINT64_C(0x7dfbefdf7efbf7df),
        UINT64_C(0xfbf7dfbefdf7efbf),
    };
    return lynx_poly_pow<secpar::s384>(x, e);
}

inline poly_secpar<secpar::s512> lynx_poly_pow_2_180_minus_2_90_plus_1_512(poly_secpar<secpar::s512> x)
{
    static const uint64_t e[8] = {
        UINT64_C(0x0000000000000001),
        UINT64_C(0xfffffffffc000000),
        UINT64_C(0x000fffffffffffff),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
        UINT64_C(0x0000000000000000),
    };
    return lynx_poly_pow<secpar::s512>(x, e);
}

inline poly_secpar<secpar::s512> lynx_poly_inv_2_12_minus_2_6_plus_1_512(poly_secpar<secpar::s512> x)
{
    static const uint64_t e[8] = {
        UINT64_C(0xc9898d9d9c9898d9),
        UINT64_C(0x898d9d9c9898d9d9),
        UINT64_C(0x8d9d9c9898d9d9c9),
        UINT64_C(0x9d9c9898d9d9c989),
        UINT64_C(0x9c9898d9d9c9898d),
        UINT64_C(0x9898d9d9c9898d9d),
        UINT64_C(0x98d9d9c9898d9d9c),
        UINT64_C(0xd9d9c9898d9d9c98),
    };
    return lynx_poly_pow<secpar::s512>(x, e);
}

template <secpar S>
inline poly_secpar<S> lynx_poly_inv_allones_exp(poly_secpar<S> x)
{
    constexpr std::size_t N = secpar_to_bits(S);
    auto result = poly_secpar<S>::set_low32(1);
    for (std::size_t i = N; i-- > 0;)
    {
        result = lynx_poly_sq<S>(result);
        if (i != 0)
            result = lynx_poly_mul<S>(result, x);
    }
    return result;
}

template <secpar S>
poly_secpar<S> lynx_poly_s1(poly_secpar<S> x);

template <secpar S>
poly_secpar<S> lynx_poly_s2(poly_secpar<S> x);

template <>
inline poly_secpar<secpar::s128> lynx_poly_s1<secpar::s128>(poly_secpar<secpar::s128> x)
{
    return lynx_poly_inv_128(x);
}

template <>
inline poly_secpar<secpar::s128> lynx_poly_s2<secpar::s128>(poly_secpar<secpar::s128> x)
{
    return lynx_poly_inv_128(x);
}

// 160-bit: x^(2^67 - 1) = x^(2^64 - 1)^(2^3) * x^(2^3 - 1) = r64^(2^3) * x^7
// 67 = 64 + 3, x^7 = x^3^2 * x = sq(r2) * x
inline poly_secpar<secpar::s160> lynx_poly_pow_2_67_minus_1_160(poly_secpar<secpar::s160> x)
{
    using gf = poly_secpar<secpar::s160>;
    gf t;
    gf r1  = x;
    gf r2  = lynx_poly_mul<secpar::s160>(lynx_poly_sq<secpar::s160>(r1), r1);
    t      = lynx_poly_sq<secpar::s160>(lynx_poly_sq<secpar::s160>(r2));
    gf r4  = lynx_poly_mul<secpar::s160>(t, r2);
    t = r4; for (int i = 0; i < 4; i++) t = lynx_poly_sq<secpar::s160>(t);
    gf r8  = lynx_poly_mul<secpar::s160>(t, r4);
    t = r8; for (int i = 0; i < 8; i++) t = lynx_poly_sq<secpar::s160>(t);
    gf r16 = lynx_poly_mul<secpar::s160>(t, r8);
    t = r16; for (int i = 0; i < 16; i++) t = lynx_poly_sq<secpar::s160>(t);
    gf r32 = lynx_poly_mul<secpar::s160>(t, r16);
    t = r32; for (int i = 0; i < 32; i++) t = lynx_poly_sq<secpar::s160>(t);
    gf r64 = lynx_poly_mul<secpar::s160>(t, r32);
    // r67 = x^(2^67 - 1) = r64^(2^3) * x^(2^3 - 1) = sq^3(r64) * x^7
    t = r64; for (int i = 0; i < 3; i++) t = lynx_poly_sq<secpar::s160>(t);
    // x^7 = x^6 * x = sq(r2) * x
    gf x7 = lynx_poly_mul<secpar::s160>(lynx_poly_sq<secpar::s160>(r2), r1);
    return lynx_poly_mul<secpar::s160>(t, x7);
}

template <>
inline poly_secpar<secpar::s160> lynx_poly_s1<secpar::s160>(poly_secpar<secpar::s160> x)
{
    return lynx_poly_inv_allones_exp<secpar::s160>(x);
}

template <>
inline poly_secpar<secpar::s160> lynx_poly_s2<secpar::s160>(poly_secpar<secpar::s160> x)
{
    return lynx_poly_pow_2_67_minus_1_160(x);
}

template <>
inline poly_secpar<secpar::s192> lynx_poly_s1<secpar::s192>(poly_secpar<secpar::s192> x)
{
    return lynx_poly_inv_2_13_minus_1_192(x);
}

template <>
inline poly_secpar<secpar::s192> lynx_poly_s2<secpar::s192>(poly_secpar<secpar::s192> x)
{
    return lynx_poly_pow_2_59_minus_1_192(x);
}

template <>
inline poly_secpar<secpar::s256> lynx_poly_s1<secpar::s256>(poly_secpar<secpar::s256> x)
{
    return lynx_poly_inv_2_5_minus_1_256(x);
}

template <>
inline poly_secpar<secpar::s256> lynx_poly_s2<secpar::s256>(poly_secpar<secpar::s256> x)
{
    return lynx_poly_pow_2_53_minus_1_256(x);
}

template <>
inline poly_secpar<secpar::s384> lynx_poly_s1<secpar::s384>(poly_secpar<secpar::s384> x)
{
    return lynx_poly_inv_2_13_minus_1_384(x);
}

template <>
inline poly_secpar<secpar::s384> lynx_poly_s2<secpar::s384>(poly_secpar<secpar::s384> x)
{
    return lynx_poly_pow_2_60_minus_2_30_plus_1_384(x);
}

template <>
inline poly_secpar<secpar::s512> lynx_poly_s1<secpar::s512>(poly_secpar<secpar::s512> x)
{
    return lynx_poly_inv_2_12_minus_2_6_plus_1_512(x);
}

template <>
inline poly_secpar<secpar::s512> lynx_poly_s2<secpar::s512>(poly_secpar<secpar::s512> x)
{
    return lynx_poly_pow_2_180_minus_2_90_plus_1_512(x);
}

inline poly_secpar<secpar::s256> lynx_poly_inv_256(poly_secpar<secpar::s256> x)
{
    return lynx_poly_inv_allones_exp<secpar::s256>(x);
}

inline poly_secpar<secpar::s384> lynx_poly_inv_384(poly_secpar<secpar::s384> x)
{
    return lynx_poly_inv_allones_exp<secpar::s384>(x);
}

inline poly_secpar<secpar::s512> lynx_poly_inv_512(poly_secpar<secpar::s512> x)
{
    return lynx_poly_inv_allones_exp<secpar::s512>(x);
}

// Compute the Frobenius matrix for x -> x^(2^power) in GF(2^N).
// M[row][word] is the bit-packed matrix: column col maps basis element alpha^col
// to alpha^(col * 2^power) mod p(alpha).
template <std::size_t N>
inline void lynx_frobenius_matrix(unsigned int power,
                                  std::array<std::array<uint64_t, (N + 63) / 64>, N>& M)
{
    constexpr std::size_t W = (N + 63) / 64;

    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t w = 0; w < W; ++w)
            M[i][w] = 0;

    for (std::size_t col = 0; col < N; ++col)
    {
        bf_t<N> basis = bf_t<N>::zero();
        basis.data[col / 64] = UINT64_C(1) << (col % 64);

        for (unsigned int i = 0; i < power; ++i)
            basis = bf_sq<N>(basis);

        for (std::size_t row = 0; row < N; ++row)
        {
            if ((basis.data[row / 64] >> (row % 64)) & 1)
                M[row][col / 64] |= UINT64_C(1) << (col % 64);
        }
    }
}

// Apply Frobenius to a bf_t value (for computing constant Frobenius images).
template <std::size_t N>
inline bf_t<N> bf_frobenius(bf_t<N> x, unsigned int power)
{
    for (unsigned int i = 0; i < power; ++i)
        x = bf_sq<N>(x);
    return x;
}

} // namespace lynx
} // namespace sig

#endif
