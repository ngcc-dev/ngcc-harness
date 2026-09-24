#include "owf_proof.inc"

namespace sig
{

// clang-format off




template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_s::secpar_v, false, resolved_alpha::resolved_alpha_160_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_160_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_f::secpar_v, false, resolved_alpha::resolved_alpha_160_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_160_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_s::secpar_v, false, resolved_alpha::resolved_alpha_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_256_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_f::secpar_v, false, resolved_alpha::resolved_alpha_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_256_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_s::secpar_v, false, resolved_alpha::resolved_alpha_384_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_384_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_f::secpar_v, false, resolved_alpha::resolved_alpha_384_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_384_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_s::secpar_v, false, resolved_alpha::resolved_alpha_512_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_512_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_f::secpar_v, false, resolved_alpha::resolved_alpha_512_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_512_f>*);

template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_s::secpar_v, false, resolved_alpha::resolved_alpha_160_s::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_160_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_f::secpar_v, false, resolved_alpha::resolved_alpha_160_f::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_160_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_s::secpar_v, false, resolved_alpha::resolved_alpha_256_s::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_256_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_f::secpar_v, false, resolved_alpha::resolved_alpha_256_f::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_256_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_s::secpar_v, false, resolved_alpha::resolved_alpha_384_s::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_384_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_f::secpar_v, false, resolved_alpha::resolved_alpha_384_f::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_384_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_s::secpar_v, false, resolved_alpha::resolved_alpha_512_s::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_512_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_f::secpar_v, false, resolved_alpha::resolved_alpha_512_f::OWF_CONSTS::QS_DEGREE>*, const secret_key<resolved_alpha::resolved_alpha_512_f>*);

template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_s::secpar_v, true, resolved_alpha::resolved_alpha_160_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_160_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_160_f::secpar_v, true, resolved_alpha::resolved_alpha_160_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_160_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_s::secpar_v, true, resolved_alpha::resolved_alpha_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_256_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_256_f::secpar_v, true, resolved_alpha::resolved_alpha_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_256_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_s::secpar_v, true, resolved_alpha::resolved_alpha_384_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_384_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_384_f::secpar_v, true, resolved_alpha::resolved_alpha_384_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_384_f>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_s::secpar_v, true, resolved_alpha::resolved_alpha_512_s::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_512_s>*);
template void owf_constraints(quicksilver_state<resolved_alpha::resolved_alpha_512_f::secpar_v, true, resolved_alpha::resolved_alpha_512_f::OWF_CONSTS::QS_DEGREE>*, const public_key<resolved_alpha::resolved_alpha_512_f>*);

// clang-format on

} // namespace sig
