#include "owf_proof.inc"

namespace faest
{

// clang-format off

template void owf_constraints(quicksilver_state<v1::faest_128_s::secpar_v, false, v1::faest_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_128_s>*);
template void owf_constraints(quicksilver_state<v1::faest_128_f::secpar_v, false, v1::faest_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_128_f>*);
template void owf_constraints(quicksilver_state<v1::faest_192_s::secpar_v, false, v1::faest_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_192_s>*);
template void owf_constraints(quicksilver_state<v1::faest_192_f::secpar_v, false, v1::faest_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_192_f>*);
template void owf_constraints(quicksilver_state<v1::faest_256_s::secpar_v, false, v1::faest_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_256_s>*);
template void owf_constraints(quicksilver_state<v1::faest_256_f::secpar_v, false, v1::faest_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_256_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_128_s::secpar_v, false, v1::faest_em_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_128_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_128_f::secpar_v, false, v1::faest_em_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_128_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_192_s::secpar_v, false, v1::faest_em_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_192_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_192_f::secpar_v, false, v1::faest_em_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_192_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_256_s::secpar_v, false, v1::faest_em_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_256_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_256_f::secpar_v, false, v1::faest_em_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_256_f>*);
template void owf_constraints(quicksilver_state<v2::faest_128_s::secpar_v, false, v2::faest_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_128_s>*);
template void owf_constraints(quicksilver_state<v2::faest_128_f::secpar_v, false, v2::faest_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_128_f>*);
template void owf_constraints(quicksilver_state<v2::faest_192_s::secpar_v, false, v2::faest_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_192_s>*);
template void owf_constraints(quicksilver_state<v2::faest_192_f::secpar_v, false, v2::faest_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_192_f>*);
template void owf_constraints(quicksilver_state<v2::faest_256_s::secpar_v, false, v2::faest_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_256_s>*);
template void owf_constraints(quicksilver_state<v2::faest_256_f::secpar_v, false, v2::faest_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_256_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_128_s::secpar_v, false, v2::faest_em_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_128_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_128_f::secpar_v, false, v2::faest_em_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_128_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_192_s::secpar_v, false, v2::faest_em_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_192_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_192_f::secpar_v, false, v2::faest_em_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_192_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_256_s::secpar_v, false, v2::faest_em_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_256_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_256_f::secpar_v, false, v2::faest_em_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_256_f>*);
template void owf_constraints(quicksilver_state<galas::galas_128s::secpar_v, false, galas::galas_128s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_128s>*);
template void owf_constraints(quicksilver_state<galas::galas_128f::secpar_v, false, galas::galas_128f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_128f>*);
template void owf_constraints(quicksilver_state<galas::galas_192s::secpar_v, false, galas::galas_192s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_192s>*);
template void owf_constraints(quicksilver_state<galas::galas_192f::secpar_v, false, galas::galas_192f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_192f>*);
template void owf_constraints(quicksilver_state<galas::galas_256s::secpar_v, false, galas::galas_256s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_256s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_160s::secpar_v, false, galas::galas_ngcc_160s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_160s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_384s::secpar_v, false, galas::galas_ngcc_384s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_384s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_512s::secpar_v, false, galas::galas_ngcc_512s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_512s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_256s::secpar_v, false, galas::galas_ngcc_256s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_256s>*);
template void owf_constraints(quicksilver_state<galas::galas_256f::secpar_v, false, galas::galas_256f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_256f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_160f::secpar_v, false, galas::galas_ngcc_160f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_160f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_384f::secpar_v, false, galas::galas_ngcc_384f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_384f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_512f::secpar_v, false, galas::galas_ngcc_512f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_512f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_256f::secpar_v, false, galas::galas_ngcc_256f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_256f>*);

template void owf_constraints(quicksilver_state<v1::faest_128_s::secpar_v, true, v1::faest_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_128_s>*);
template void owf_constraints(quicksilver_state<v1::faest_128_f::secpar_v, true, v1::faest_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_128_f>*);
template void owf_constraints(quicksilver_state<v1::faest_192_s::secpar_v, true, v1::faest_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_192_s>*);
template void owf_constraints(quicksilver_state<v1::faest_192_f::secpar_v, true, v1::faest_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_192_f>*);
template void owf_constraints(quicksilver_state<v1::faest_256_s::secpar_v, true, v1::faest_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_256_s>*);
template void owf_constraints(quicksilver_state<v1::faest_256_f::secpar_v, true, v1::faest_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_256_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_128_s::secpar_v, true, v1::faest_em_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_128_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_128_f::secpar_v, true, v1::faest_em_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_128_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_192_s::secpar_v, true, v1::faest_em_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_192_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_192_f::secpar_v, true, v1::faest_em_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_192_f>*);
template void owf_constraints(quicksilver_state<v1::faest_em_256_s::secpar_v, true, v1::faest_em_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_256_s>*);
template void owf_constraints(quicksilver_state<v1::faest_em_256_f::secpar_v, true, v1::faest_em_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v1::faest_em_256_f>*);
template void owf_constraints(quicksilver_state<v2::faest_128_s::secpar_v, true, v2::faest_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_128_s>*);
template void owf_constraints(quicksilver_state<v2::faest_128_f::secpar_v, true, v2::faest_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_128_f>*);
template void owf_constraints(quicksilver_state<v2::faest_192_s::secpar_v, true, v2::faest_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_192_s>*);
template void owf_constraints(quicksilver_state<v2::faest_192_f::secpar_v, true, v2::faest_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_192_f>*);
template void owf_constraints(quicksilver_state<v2::faest_256_s::secpar_v, true, v2::faest_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_256_s>*);
template void owf_constraints(quicksilver_state<v2::faest_256_f::secpar_v, true, v2::faest_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_256_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_128_s::secpar_v, true, v2::faest_em_128_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_128_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_128_f::secpar_v, true, v2::faest_em_128_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_128_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_192_s::secpar_v, true, v2::faest_em_192_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_192_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_192_f::secpar_v, true, v2::faest_em_192_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_192_f>*);
template void owf_constraints(quicksilver_state<v2::faest_em_256_s::secpar_v, true, v2::faest_em_256_s::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_256_s>*);
template void owf_constraints(quicksilver_state<v2::faest_em_256_f::secpar_v, true, v2::faest_em_256_f::OWF_CONSTS::QS_DEGREE>*, const public_key<v2::faest_em_256_f>*);
template void owf_constraints(quicksilver_state<galas::galas_128s::secpar_v, true, galas::galas_128s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_128s>*);
template void owf_constraints(quicksilver_state<galas::galas_128f::secpar_v, true, galas::galas_128f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_128f>*);
template void owf_constraints(quicksilver_state<galas::galas_192s::secpar_v, true, galas::galas_192s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_192s>*);
template void owf_constraints(quicksilver_state<galas::galas_192f::secpar_v, true, galas::galas_192f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_192f>*);
template void owf_constraints(quicksilver_state<galas::galas_256s::secpar_v, true, galas::galas_256s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_256s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_160s::secpar_v, true, galas::galas_ngcc_160s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_160s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_384s::secpar_v, true, galas::galas_ngcc_384s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_384s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_512s::secpar_v, true, galas::galas_ngcc_512s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_512s>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_256s::secpar_v, true, galas::galas_ngcc_256s::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_256s>*);
template void owf_constraints(quicksilver_state<galas::galas_256f::secpar_v, true, galas::galas_256f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_256f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_160f::secpar_v, true, galas::galas_ngcc_160f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_160f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_384f::secpar_v, true, galas::galas_ngcc_384f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_384f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_512f::secpar_v, true, galas::galas_ngcc_512f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_512f>*);
template void owf_constraints(quicksilver_state<galas::galas_ngcc_256f::secpar_v, true, galas::galas_ngcc_256f::OWF_CONSTS::QS_DEGREE>*, const public_key<galas::galas_ngcc_256f>*);

// clang-format on
} // namespace faest
