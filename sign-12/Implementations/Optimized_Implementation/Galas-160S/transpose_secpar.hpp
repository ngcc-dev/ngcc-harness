#ifndef TRANSPOSE_SECPAR_HPP
#define TRANSPOSE_SECPAR_HPP

#include "parameters.hpp"

#include "avx2/transpose_secpar_impl.hpp"

namespace faest
{

// Convert a column-major rows x SECURITY_PARAM bit matrix into row-major format. Each column must
// start stride bytes from the previous. Rows must be a multiple of TRANSPOSE_BITS_ROWS.
template <secpar S>
void transpose_secpar(const void* input, void* output, size_t stride, size_t rows);

} // namespace faest

#endif
