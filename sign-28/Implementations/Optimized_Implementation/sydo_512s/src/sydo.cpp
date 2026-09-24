#include "sydo.hpp"
#include "small_vole.inc"
#include "vole_commit.inc"

namespace sydo
{

// clang-format off

#define SYDO_INSTANTIATE_VOLE_RECEIVER(...)                                                      \
    template void vole_receiver<__VA_ARGS__>(                                                      \
        unsigned int, const block_secpar<__VA_ARGS__::secpar_v>* __restrict__,                     \
        typename __VA_ARGS__::vole_prg_t::iv_t, uint32_t,                                          \
        const vole_block* __restrict__, vole_block* __restrict__, const uint8_t* __restrict__);    \
    template void vole_receiver_apply_correction<__VA_ARGS__>(                                    \
        size_t, size_t, const vole_block* __restrict__, vole_block* __restrict__,                 \
        const uint8_t* __restrict__);                                                            \
    template bool vole_reconstruct<__VA_ARGS__>(                                                   \
        typename __VA_ARGS__::vole_prg_t::iv_t, vole_block* __restrict__, const uint8_t*,          \
        const uint8_t* __restrict__, const uint8_t* __restrict__, uint8_t* __restrict__)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_VOLE_RECEIVER(SYDO_API_PARAM_TYPE);
#else
#if defined(SYDO_ENABLE_CST_PROFILE)
using sydo_128_s_cst =
    sydo_with_quicksilver_term<sydo_128_s, quicksilver_term::constant>;
using sydo_128_f_cst =
    sydo_with_quicksilver_term<sydo_128_f, quicksilver_term::constant>;
#endif

SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_128_s);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_128_s_cst);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_160_s);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_160_f);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_192_s);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_192_f);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_256_s);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_256_f);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_512_s);
SYDO_INSTANTIATE_VOLE_RECEIVER(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_VOLE_RECEIVER

// clang-format on

} // namespace sydo
