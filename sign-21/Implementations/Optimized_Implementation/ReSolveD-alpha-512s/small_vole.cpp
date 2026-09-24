#include "small_vole.inc"

namespace sig
{

// clang-format off

#define INSTANTIATE_SMALL_VOLE(P)                                                                  \
template void vole_sender<P>(unsigned int, const block_secpar<P::secpar_v>* __restrict__,          \
                             block128, uint32_t, const vole_block* __restrict__,                   \
                             vole_block* __restrict__, vole_block* __restrict__);                  \
template void vole_receiver<P>(unsigned int, const block_secpar<P::secpar_v>* __restrict__,        \
                               block128, uint32_t, const vole_block* __restrict__,                 \
                               vole_block* __restrict__, const uint8_t* __restrict__);             \
template void vole_receiver_apply_correction<P>(size_t, size_t, const vole_block* __restrict__,    \
                                                vole_block* __restrict__,                          \
                                                const uint8_t* __restrict__);


INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_160_s)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_160_f)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_256_s)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_256_f)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_384_s)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_384_f)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_512_s)
INSTANTIATE_SMALL_VOLE(resolved_alpha::resolved_alpha_512_f)

#undef INSTANTIATE_SMALL_VOLE

// clang-format on

} // namespace sig
