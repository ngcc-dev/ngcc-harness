#ifndef POLYNOMIALS_CONSTANTS_HPP
#define POLYNOMIALS_CONSTANTS_HPP

#include <cstdint>

namespace sig
{

// Modulus for GF(2^n), without the x^n term.
constexpr uint32_t gf64_modulus  = (1 <<  4) | (1 <<  3) | (1 << 1) | 1; // degree = 4
constexpr uint32_t gf128_modulus = (1 <<  7) | (1 <<  2) | (1 << 1) | 1; // degree = 7
constexpr uint32_t gf160_modulus = (1 <<  5) | (1 <<  3) | (1 <<  2) | 1; // X^5+X^3+X^2+1
constexpr uint32_t gf192_modulus = (1 <<  7) | (1 <<  2) | (1 << 1) | 1; // degree = 7
constexpr uint32_t gf256_modulus = (1 << 10) | (1 <<  5) | (1 << 2) | 1; // degree = 11
constexpr uint32_t gf384_modulus = (1 << 16) | (1 << 15) | (1 << 6) | 1; // degree = 16
constexpr uint32_t gf512_modulus = (1 <<  8) | (1 <<  5) | (1 << 2) | 1; // degree = 8
constexpr uint32_t gf576_modulus = (1 << 13) | (1 <<  4) | (1 << 3) | 1; // degree = 13
constexpr uint32_t gf768_modulus = (1 << 19) | (1 << 17) | (1 << 4) | 1; // degree = 19

extern const unsigned char gf8_in_gf128[7][16];
extern const unsigned char gf8_in_gf160[7][20];
extern const unsigned char gf8_in_gf192[7][24];
extern const unsigned char gf8_in_gf256[7][32];

} // namespace sig

#endif
