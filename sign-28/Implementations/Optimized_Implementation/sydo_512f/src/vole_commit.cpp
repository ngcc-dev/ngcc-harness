#include "vole_commit.inc"

namespace sydo
{

// clang-format off

#define SYDO_INSTANTIATE_VOLE_COMMIT(...)                                                        \
    template void vole_commit<__VA_ARGS__>(                                                        \
        block_secpar<__VA_ARGS__::secpar_v>, typename __VA_ARGS__::vole_prg_t::iv_t,               \
        block_secpar<__VA_ARGS__::secpar_v>* __restrict__, unsigned char* __restrict__,           \
        vole_block* __restrict__, vole_block* __restrict__, uint8_t* __restrict__,                \
        uint8_t* __restrict__)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_VOLE_COMMIT(SYDO_API_PARAM_TYPE);
#else
#if defined(SYDO_ENABLE_CST_PROFILE)
using sydo_128_s_cst =
    sydo_with_quicksilver_term<sydo_128_s, quicksilver_term::constant>;
using sydo_128_f_cst =
    sydo_with_quicksilver_term<sydo_128_f, quicksilver_term::constant>;
#endif

SYDO_INSTANTIATE_VOLE_COMMIT(sydo_128_s);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_128_s_cst);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_160_s);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_160_f);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_192_s);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_192_f);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_256_s);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_256_f);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_512_s);
SYDO_INSTANTIATE_VOLE_COMMIT(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_VOLE_COMMIT

// clang-format on

} // namespace sydo
