#ifndef POLYNOMIALS_HPP
#define POLYNOMIALS_HPP

#include "avx2/polynomials_impl.hpp"
#include "parameters.hpp"

namespace faest
{

template <secpar S> using poly_secpar = poly<secpar_to_bits(S)>;

template <secpar S> using poly_secpar_plus_64 = poly<secpar_to_bits(S) + 64>;

template <secpar S> using poly_2secpar = poly<2 * secpar_to_bits(S)>;

} // namespace faest

#endif
