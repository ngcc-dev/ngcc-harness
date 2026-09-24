#include "small_vole.inc"

namespace sydo
{

// clang-format off

#define SYDO_INSTANTIATE_VOLE_SENDER(...)                                                        \
    template void vole_sender<__VA_ARGS__>(                                                        \
        unsigned int, const block_secpar<__VA_ARGS__::secpar_v>* __restrict__,                    \
        typename __VA_ARGS__::vole_prg_t::iv_t, uint32_t,                                         \
        const vole_block* __restrict__, vole_block* __restrict__, vole_block* __restrict__)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_VOLE_SENDER(SYDO_API_PARAM_TYPE);
#else
#if defined(SYDO_ENABLE_CST_PROFILE)
using sydo_128_s_cst =
    sydo_with_quicksilver_term<sydo_128_s, quicksilver_term::constant>;
using sydo_128_f_cst =
    sydo_with_quicksilver_term<sydo_128_f, quicksilver_term::constant>;
#endif

SYDO_INSTANTIATE_VOLE_SENDER(sydo_128_s);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_VOLE_SENDER(sydo_128_s_cst);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_VOLE_SENDER(sydo_160_s);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_160_f);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_192_s);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_192_f);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_256_s);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_256_f);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_512_s);
SYDO_INSTANTIATE_VOLE_SENDER(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_VOLE_SENDER

// clang-format on

} // namespace sydo
