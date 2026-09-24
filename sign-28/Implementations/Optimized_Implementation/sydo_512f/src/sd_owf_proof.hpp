#ifndef SYDO_SD_OWF_PROOF_HPP
#define SYDO_SD_OWF_PROOF_HPP

#include "keys.hpp"
#include "hash.hpp"
#include "quicksilver.hpp"
#include "transpose_secpar.hpp"
#include "vole_check.hpp"
#include "vole_commit.hpp"
#include <cstdint>

namespace sydo
{

constexpr std::size_t sydo_secpar_to_bits(secpar s) { return secpar_to_bits(s); }
constexpr std::size_t sydo_secpar_to_bytes(secpar s) { return secpar_to_bytes(s); }

constexpr std::size_t sydo_transpose_bits_rows = TRANSPOSE_BITS_ROWS;

template <secpar S>
inline void sydo_transpose_secpar(const void* input, void* output, std::size_t stride,
                                  std::size_t rows)
{
    transpose_secpar<S>(input, output, stride, rows);
}

template <typename P>
inline void sydo_vole_commit(block_secpar<P::secpar_v> seed, typename P::vole_prg_t::iv_t iv,
                             block_secpar<P::secpar_v>* forest, unsigned char* hashed_leaves,
                             vole_block* u, vole_block* v, uint8_t* com, uint8_t* check)
{
    vole_commit<P>(seed, iv, forest, hashed_leaves, u, v, com, check);
}

template <typename P>
inline void sydo_vole_check_sender(const vole_block* u, const vole_block* v,
                                   const uint8_t* challenge, uint8_t* proof, hash_state& hasher)
{
    vole_check_sender<P>(u, v, challenge, proof, hasher);
}

namespace sydo_detail
{

template <typename P, bool verifier>
void sydo_fold_expanded_h_rows(
    block_secpar<P::secpar_v>* folded_row_out, std::size_t folded_row_len,
    const typename P::prg_t::key_t& seed_pk,
    const quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE,
                            P::quicksilver_term_v>* state);

} // namespace sydo_detail

template <typename P, bool verifier>
void sydo_owf_constraints(
    quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE, P::quicksilver_term_v>*
        state,
    const sydo_public_key<P>* pk, uint8_t* folded_poly_coeffs_out = nullptr);

} // namespace sydo

#endif
