#include "vole_commit.inc"

namespace sig
{

// clang-format off

#define INSTANTIATE_VOLE_COMMIT(P)                                                                 \
template void vole_commit<P>(block_secpar<P::secpar_v>, block128,                                  \
                             block_secpar<P::secpar_v>* __restrict__, unsigned char* __restrict__, \
                             vole_block* __restrict__, vole_block* __restrict__,                  \
                             uint8_t* __restrict__, const uint8_t* __restrict__,                   \
                             const uint8_t* __restrict__, uint8_t* __restrict__);                  \
template bool vole_reconstruct<P>(block128, vole_block* __restrict__, const uint8_t*,              \
                                  const uint8_t* __restrict__, const uint8_t* __restrict__,        \
                                  const uint8_t* __restrict__, const uint8_t* __restrict__,        \
                                  uint8_t* __restrict__);


INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_160_s)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_160_f)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_256_s)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_256_f)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_384_s)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_384_f)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_512_s)
INSTANTIATE_VOLE_COMMIT(resolved_alpha::resolved_alpha_512_f)

#undef INSTANTIATE_VOLE_COMMIT

// clang-format on

} // namespace sig
