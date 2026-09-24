#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

// Constants that are used in the implementation

#include "avx2/constants_impl.hpp"
#include "parameters.hpp"

namespace sig
{

template <std::size_t bits> struct block;
template <secpar S> using block_secpar = block<secpar_to_bits(S)>;
using block128 = block<128>;

// AES-related constants

// Number of AES rounds depending on the security parameter
template <secpar S>
constexpr std::size_t AES_ROUNDS = []
{
    if constexpr (S == secpar::s128)
    {
        return 10;
    }
    else if constexpr (S == secpar::s192)
    {
        return 12;
    }
    else
    {
        return 14; // AES-256 rounds (also used as placeholder for s384/s512 in Lynx)
    }
}();

// Number of Rijndael rounds depending on the security parameter
template <secpar S> constexpr std::size_t RIJNDAEL_ROUNDS = AES_ROUNDS<S>;

// Number of AES blocks to run in parallel, for maximum performance.
constexpr std::size_t AES_PREFERRED_WIDTH = (1 << AES_PREFERRED_WIDTH_SHIFT);

// Number of Rijndael256 blocks to run in parallel, for maximum performance.
constexpr std::size_t RIJNDAEL256_PREFERRED_WIDTH = (1 << RIJNDAEL256_PREFERRED_WIDTH_SHIFT);

// TODO: documentation, what about 192 bits?
template <secpar S>
constexpr std::size_t FIXED_KEY_PREFERRED_WIDTH_SHIFT = []
{
    if constexpr (S == secpar::s128)
    {
        return AES_PREFERRED_WIDTH_SHIFT;
    }
    else if constexpr (S == secpar::s256)
    {
        return RIJNDAEL256_PREFERRED_WIDTH_SHIFT;
    }
    else
    {
        static_assert(dependent_false<S>, "unsupported security parameter for fixed-key AES");
    }
}();
template <secpar S>
constexpr std::size_t FIXED_KEY_PREFERRED_WIDTH = (1 << FIXED_KEY_PREFERRED_WIDTH_SHIFT<S>);

// Transpose-related constants
constexpr std::size_t TRANSPOSE_BITS_ROWS = 1 << TRANSPOSE_BITS_ROWS_SHIFT;

// Template containing constants that depend only on the security parameter and the one-way function
template <secpar S, owf O> struct OWF_CONSTANTS;

namespace detail
{

// Compute the number of key schedule constraints for an OWF.
template <secpar S, owf O> constexpr std::size_t compute_owf_num_key_schedule_constraints()
{
    using OC = OWF_CONSTANTS<S, O>;

    if constexpr (is_owf_with_zero_sboxes(O))
        return 2 * OC::OWF_KEY_SCHEDULE_SBOXES;
    else
        return OC::OWF_KEY_SCHEDULE_SBOXES;
}

// Compute the number of encryption constraints for an OWF.
template <secpar S, owf O> constexpr std::size_t compute_owf_num_enc_constraints()
{
    using OC = OWF_CONSTANTS<S, O>;

    if constexpr (is_owf_with_norm_proof(O))
        return 3 * OC::OWF_BLOCKS * OC::OWF_BLOCK_SIZE * OC::OWF_ROUNDS / 2;
    else if constexpr (is_owf_with_zero_sboxes(O))
        return 2 * OC::OWF_BLOCKS * OC::OWF_BLOCK_SIZE * OC::OWF_ROUNDS;
    else
        return OC::OWF_BLOCKS * OC::OWF_BLOCK_SIZE * OC::OWF_ROUNDS;
}

// Compute the total number of constraints for an OWF.
template <secpar S, owf O> constexpr std::size_t compute_owf_num_constraints()
{
    std::size_t num_constraints =
        compute_owf_num_key_schedule_constraints<S, O>() + compute_owf_num_enc_constraints<S, O>();

    if constexpr (is_owf_with_shrunk_keyspace(O))
        num_constraints += 1;

    return num_constraints;
}

template <secpar S, owf O> constexpr std::size_t compute_owf_key_witness_bits()
{
    using OC = OWF_CONSTANTS<S, O>;
    if constexpr (is_owf_with_aes_ecb(O))
        return secpar_to_bits(S) + 8 * OC::OWF_KEY_SCHEDULE_SBOXES;
    else if constexpr (is_owf_with_aes_em(O))
        return secpar_to_bits(S);
    else
        static_assert("unsupported OWF");
}

template <secpar S, owf O> constexpr std::size_t compute_owf_enc_witness_bits_per_block()
{
    using OC = OWF_CONSTANTS<S, O>;
    if constexpr (is_owf_with_norm_proof(O))
        return 8 * (OC::OWF_BLOCK_SIZE * OC::OWF_ROUNDS * 3 / 4 - OC::OWF_BLOCK_SIZE);
    else
        return 8 * OC::OWF_BLOCK_SIZE * (OC::OWF_ROUNDS - 1);
}

template <secpar S, owf O> constexpr std::size_t compute_owf_witness_bits()
{
    using OC = OWF_CONSTANTS<S, O>;
    return OC::OWF_KEY_WITNESS_BITS + OC::OWF_BLOCKS * OC::OWF_ENC_WITNESS_BITS_PER_BLOCK;
}

} // namespace detail

// Specialization: Constants for the Lynx one-way function
template <secpar S, owf O>
    requires(is_owf_with_lynx(O))
struct OWF_CONSTANTS<S, O>
{
    // Lynx has no rounds in the traditional sense
    constexpr static std::size_t OWF_ROUNDS = 0;
    // Block size equals security parameter in bytes
    constexpr static std::size_t OWF_BLOCK_SIZE = secpar_to_bytes(S);
    // Single block
    constexpr static std::size_t OWF_BLOCKS = 1;
    // Lynx has no key schedule constraints.
    constexpr static std::size_t OWF_KEY_SCHEDULE_SBOXES = 0;
    constexpr static std::size_t OWF_KEY_SCHEDULE_CONSTRAINTS = 0;
    // Witness key bits = lambda (just the key k)
    constexpr static std::size_t OWF_KEY_WITNESS_BITS = secpar_to_bits(S);
    // S-boxes: 2 inversions (S1 and S2)
    constexpr static std::size_t OWF_ENC_SBOXES = 2;
    // Lynx constraints (NOT computed from AES formulas):
    // - 128-bit: 2 inverse constraints + 1 output = 3
    // - 192/256/384/512-bit: 2 Frobenius constraints + 1 output = 3
    constexpr static std::size_t OWF_ENC_CONSTRAINTS = 3;
    // Witness per block: v1 and v2 = 2 * lambda bits
    constexpr static std::size_t OWF_ENC_WITNESS_BITS_PER_BLOCK = 2 * secpar_to_bits(S);
    constexpr static std::size_t OWF_ENC_WITNESS_BITS = OWF_ENC_WITNESS_BITS_PER_BLOCK;
    // Total constraints.
    constexpr static std::size_t OWF_NUM_CONSTRAINTS =
        OWF_KEY_SCHEDULE_CONSTRAINTS + OWF_ENC_CONSTRAINTS;
    // Total witness: k || v1 || v2 = 3 * lambda bits
    constexpr static std::size_t WITNESS_BITS = 3 * secpar_to_bits(S);
    // Lynx uses only degree-2 constraints.
    constexpr static std::size_t QS_DEGREE = 2;

    using block_t = block_secpar<S>;
};

// Constants for the QuickSilver implementation
template <secpar S, size_t max_deg> struct QS_CONSTANTS
{
    constexpr static size_t CHALLENGE_BYTES = ((3 * secpar_to_bits(S) + 64) / 8);
    constexpr static size_t PROOF_BYTES = (max_deg - 1) * secpar_to_bytes(S);
    constexpr static size_t CHECK_BYTES = secpar_to_bytes(S);
};

// Constants for the VOLE check
template <secpar S> struct VOLE_CHECK_CONSTANTS
{
    constexpr static std::size_t HASH_BYTES = secpar_to_bytes(S) + 2;
    constexpr static std::size_t CHALLENGE_BYTES = (5 * secpar_to_bits(S) + 64) / 8;
    constexpr static std::size_t PROOF_BYTES = HASH_BYTES;
    constexpr static std::size_t CHECK_BYTES = 2 * secpar_to_bytes(S);
};

// Constants related to the vector commitments
template <size_t TAU, size_t DELTA_BITS> struct VECTOR_COMMITMENT_CONSTANTS
{
    constexpr static size_t tau_v = TAU;
    constexpr static size_t delta_bits_v = DELTA_BITS;

    // The homomorphic commitments use small field VOLE with a mix of two values of k: MIN_K and
    // MAX_K. k is the number of bits of Delta input to a single VOLE.
    constexpr static std::size_t MIN_K = DELTA_BITS / TAU;
    constexpr static std::size_t MAX_K = (DELTA_BITS + TAU - 1) / TAU;

    // Number of VOLEs that use MIN_K and MAX_K.
    constexpr static std::size_t NUM_MAX_K = DELTA_BITS % TAU;
    constexpr static std::size_t NUM_MIN_K = TAU - NUM_MAX_K;
};

// Implementation constants that depend on the parameter set
template <typename P> struct CONSTANTS
{

    using QS = QS_CONSTANTS<P::secpar_v, P::OWF_CONSTS::QS_DEGREE>;
    using VOLE_CHECK = VOLE_CHECK_CONSTANTS<P::secpar_v>;
    using VEC_COM = VECTOR_COMMITMENT_CONSTANTS<P::tau_v, P::delta_bits_v>;

    // VOLE-related constants
    constexpr static std::size_t VOLE_BLOCK = 1 << VOLE_BLOCK_SHIFT;
    constexpr static std::size_t WITNESS_BLOCKS =
        (P::OWF_CONSTS::WITNESS_BITS + 128 * VOLE_BLOCK - 1) / (128 * VOLE_BLOCK);

    constexpr static std::size_t QUICKSILVER_ROW_PAD_TO =
        (128 * VOLE_BLOCK > TRANSPOSE_BITS_ROWS) ? (128 * VOLE_BLOCK) : TRANSPOSE_BITS_ROWS;
    constexpr static std::size_t QUICKSILVER_ROWS =
        P::OWF_CONSTS::WITNESS_BITS + (P::OWF_CONSTS::QS_DEGREE - 1) * P::secpar_bits;
    constexpr static std::size_t QUICKSILVER_ROWS_PADDED =
        ((QUICKSILVER_ROWS + QUICKSILVER_ROW_PAD_TO - 1) / QUICKSILVER_ROW_PAD_TO) *
        QUICKSILVER_ROW_PAD_TO;

    constexpr static std::size_t VOLE_ROWS = QUICKSILVER_ROWS + VOLE_CHECK::HASH_BYTES * 8;

    constexpr static std::size_t VOLE_COL_BLOCKS =
        (VOLE_ROWS + 128 * VOLE_BLOCK - 1) / (128 * VOLE_BLOCK);
    constexpr static std::size_t VOLE_COL_STRIDE = VOLE_COL_BLOCKS * 16 * VOLE_BLOCK;
    constexpr static std::size_t VOLE_ROWS_PADDED = VOLE_COL_BLOCKS * 128 * VOLE_BLOCK;

    // vole-commit-related constants
    constexpr static std::size_t VOLE_COMMIT_SIZE = (VOLE_ROWS / 8) * (P::tau_v - 1);
    constexpr static std::size_t VOLE_COMMIT_CHECK_SIZE = 2 * P::secpar_bytes;

    constexpr static std::size_t PRG_VOLE_BLOCK_SIZE_SHIFT = []
    {
        if constexpr (P::vole_prg_v == prg::rijndael_fixed_key_ctr && P::secpar_v == secpar::s256)
            return 1;
        else
            return 0;
    }();
    // Number of block128s in a prg_vole_block.
    constexpr static std::size_t PRG_VOLE_BLOCK_SIZE = 1 << PRG_VOLE_BLOCK_SIZE_SHIFT;

    // Number of prg_vole_block in a vole_block.
    constexpr static std::size_t PRG_VOLE_BLOCKS_SHIFT =
        VOLE_BLOCK_SHIFT - PRG_VOLE_BLOCK_SIZE_SHIFT;
    constexpr static std::size_t PRG_VOLE_BLOCKS = 1 << PRG_VOLE_BLOCKS_SHIFT;

    // VOLE is performed in chunks of VOLE_WIDTH keys, with each column consisting of 1
    // vole_block.
    constexpr static std::size_t VOLE_WIDTH_SHIFT =
        AES_PREFERRED_WIDTH_SHIFT - PRG_VOLE_BLOCKS_SHIFT;
    constexpr static std::size_t VOLE_WIDTH = 1 << VOLE_WIDTH_SHIFT;

    // Compile-time consistency checks
    using check_witness_bits_ = std::enable_if_t<P::OWF_CONSTS::WITNESS_BITS % 8 == 0>;
    // static_assert(PRG_VOLE_BLOCK_SIZE * 16 == sizeof(typename P::vole_prg_t::block_t), "a
    // `P::vole_prg_t::block_t` must be 16 * PRG_VOLE_BLOCK_SIZE");
};

} // namespace sig

#endif
