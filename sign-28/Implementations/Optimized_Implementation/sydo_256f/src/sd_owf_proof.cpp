#include "sd_owf_proof.inc"

namespace sydo
{

// clang-format off

#define SYDO_INSTANTIATE_OWF(...)                                                                \
    template void sydo_owf_constraints<__VA_ARGS__, false>(                                      \
        quicksilver_state<__VA_ARGS__::secpar_v, false, __VA_ARGS__::OWF_CONSTS::QS_DEGREE,       \
                          __VA_ARGS__::quicksilver_term_v>*,                                       \
        const sydo_public_key<__VA_ARGS__>*, uint8_t*);                                          \
    template void sydo_owf_constraints<__VA_ARGS__, true>(                                       \
        quicksilver_state<__VA_ARGS__::secpar_v, true, __VA_ARGS__::OWF_CONSTS::QS_DEGREE,        \
                          __VA_ARGS__::quicksilver_term_v>*,                                       \
        const sydo_public_key<__VA_ARGS__>*, uint8_t*)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_OWF(SYDO_API_PARAM_TYPE);
#else
#if defined(SYDO_ENABLE_CST_PROFILE)
using sydo_128_s_cst =
    sydo_with_quicksilver_term<sydo_128_s, quicksilver_term::constant>;
using sydo_128_f_cst =
    sydo_with_quicksilver_term<sydo_128_f, quicksilver_term::constant>;
#endif

SYDO_INSTANTIATE_OWF(sydo_128_s);
SYDO_INSTANTIATE_OWF(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_OWF(sydo_128_s_cst);
SYDO_INSTANTIATE_OWF(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_OWF(sydo_160_s);
SYDO_INSTANTIATE_OWF(sydo_160_f);
SYDO_INSTANTIATE_OWF(sydo_192_s);
SYDO_INSTANTIATE_OWF(sydo_192_f);
SYDO_INSTANTIATE_OWF(sydo_256_s);
SYDO_INSTANTIATE_OWF(sydo_256_f);
SYDO_INSTANTIATE_OWF(sydo_512_s);
SYDO_INSTANTIATE_OWF(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_OWF

// clang-format on

} // namespace sydo
