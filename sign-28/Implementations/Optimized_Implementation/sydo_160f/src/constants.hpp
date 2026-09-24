#ifndef SYDO_CONSTANTS_HPP
#define SYDO_CONSTANTS_HPP

#include "avx2/constants_impl.hpp"
#include "parameters.hpp"

#include <cstddef>
#include <type_traits>

namespace sydo
{

template <std::size_t bits> struct block;
template <secpar S> using block_secpar = block<secpar_to_bits(S)>;
using block128 = block<128>;

template <secpar S>
constexpr std::size_t AES_ROUNDS = []
{
    if constexpr (S == secpar::s128)
        return 10;
    else if constexpr (S == secpar::s192)
        return 12;
    else if constexpr (S == secpar::s256)
        return 14;
    else
        static_assert(false, "unsupported security parameter for AES");
}();

template <secpar S> constexpr std::size_t RIJNDAEL_ROUNDS = AES_ROUNDS<S>;

constexpr std::size_t AES_PREFERRED_WIDTH = (1 << AES_PREFERRED_WIDTH_SHIFT);
constexpr std::size_t AES256_PREFERRED_WIDTH = (1 << AES256_PREFERRED_WIDTH_SHIFT);

template <secpar S>
constexpr std::size_t FIXED_KEY_PREFERRED_WIDTH_SHIFT = []
{
    if constexpr (S == secpar::s128)
        return AES_PREFERRED_WIDTH_SHIFT;
    else if constexpr (S == secpar::s192)
        return 1;
    else if constexpr (S == secpar::s256)
        return AES256_PREFERRED_WIDTH_SHIFT;
    else
        static_assert(false, "unsupported security parameter for fixed-key AES");
}();

template <secpar S>
constexpr std::size_t FIXED_KEY_PREFERRED_WIDTH = (1 << FIXED_KEY_PREFERRED_WIDTH_SHIFT<S>);

template <secpar S>
constexpr std::size_t RIJNDAEL_CTR_PREFERRED_WIDTH_SHIFT = []
{
    if constexpr (S == secpar::s128)
        return AES_PREFERRED_WIDTH_SHIFT;
    else if constexpr (S == secpar::s160)
        return AES_PREFERRED_WIDTH_SHIFT;
    else if constexpr (S == secpar::s192)
        return AES_PREFERRED_WIDTH_SHIFT;
    else if constexpr (S == secpar::s256)
        return AES_PREFERRED_WIDTH_SHIFT;
    else
        static_assert(false, "unsupported security parameter for Rijndael-CTR");
}();

constexpr std::size_t TRANSPOSE_BITS_ROWS = 1 << TRANSPOSE_BITS_ROWS_SHIFT;

template <secpar S, std::size_t WITNESS_BITS_IN, std::size_t QS_DEGREE_IN,
          std::size_t OWF_NUM_CONSTRAINTS_IN = 0, std::size_t OWF_MUL_C_PRE_IN = 0,
          std::size_t OWF_MUL_C_IP_IN = 0, std::size_t OWF_MUL_C_TOTAL_IN = 0>
struct SYDO_OWF_CONSTANTS
{
    constexpr static std::size_t WITNESS_BITS = WITNESS_BITS_IN;
    constexpr static std::size_t WITNESS_BYTES = (WITNESS_BITS + 7) / 8;
    constexpr static std::size_t OWF_KEY_SCHEDULE_SBOXES = 0;
    constexpr static std::size_t OWF_KEY_SCHEDULE_CONSTRAINTS = 0;
    constexpr static std::size_t OWF_KEY_WITNESS_BITS = WITNESS_BITS;
    constexpr static std::size_t OWF_BLOCK_SIZE = secpar_to_bytes(S);
    constexpr static std::size_t OWF_BLOCKS = 1;
    constexpr static std::size_t OWF_ROUNDS = 0;
    constexpr static std::size_t OWF_ENC_SBOXES = 0;
    constexpr static std::size_t OWF_ENC_CONSTRAINTS = 0;
    constexpr static std::size_t OWF_ENC_WITNESS_BITS_PER_BLOCK = 0;
    constexpr static std::size_t OWF_ENC_WITNESS_BITS = 0;
    constexpr static std::size_t OWF_NUM_CONSTRAINTS = OWF_NUM_CONSTRAINTS_IN;
    constexpr static std::size_t OWF_MUL_C_PRE = OWF_MUL_C_PRE_IN;
    constexpr static std::size_t OWF_MUL_C_IP = OWF_MUL_C_IP_IN;
    constexpr static std::size_t OWF_MUL_C_TOTAL = OWF_MUL_C_TOTAL_IN;
    constexpr static std::size_t QS_DEGREE = QS_DEGREE_IN;

    using block_t = block_secpar<S>;
};

template <secpar S, size_t max_deg, std::size_t EXTRA_LAMBDA_CHALLENGES = 0> struct QS_CONSTANTS
{
    constexpr static size_t CHALLENGE_BYTES =
        ((3 * secpar_to_bits(S) + 64 + EXTRA_LAMBDA_CHALLENGES * secpar_to_bits(S)) / 8);
    constexpr static size_t PROOF_BYTES = (max_deg - 1) * secpar_to_bytes(S);
    constexpr static size_t CHECK_BYTES = secpar_to_bytes(S);
};

template <secpar S> struct VOLE_CHECK_CONSTANTS
{
    constexpr static std::size_t HASH_BYTES = secpar_to_bytes(S) + 2;
    constexpr static std::size_t CHALLENGE_BYTES = (5 * secpar_to_bits(S) + 64) / 8;
    constexpr static std::size_t PROOF_BYTES = HASH_BYTES;
    constexpr static std::size_t CHECK_BYTES = 2 * secpar_to_bytes(S);
};

template <size_t TAU, size_t DELTA_BITS> struct VECTOR_COMMITMENT_CONSTANTS
{
    constexpr static size_t tau_v = TAU;
    constexpr static size_t delta_bits_v = DELTA_BITS;
    constexpr static std::size_t MIN_K = DELTA_BITS / TAU;
    constexpr static std::size_t MAX_K = (DELTA_BITS + TAU - 1) / TAU;
    constexpr static std::size_t NUM_MAX_K = DELTA_BITS % TAU;
    constexpr static std::size_t NUM_MIN_K = TAU - NUM_MAX_K;
};
} // namespace sydo

#include "vector_com.hpp"

namespace sydo
{

template <typename P> struct SYDO_CONSTANTS
{
    using QS = QS_CONSTANTS<P::secpar_v, P::OWF_CONSTS::QS_DEGREE, P::qs_extra_challenges>;
    using VOLE_CHECK = VOLE_CHECK_CONSTANTS<P::secpar_v>;
    using VEC_COM = VECTOR_COMMITMENT_CONSTANTS<P::tau_v, P::delta_bits_v>;

    constexpr static std::size_t VOLE_BLOCK = 1 << VOLE_BLOCK_SHIFT;
    constexpr static std::size_t WITNESS_BLOCKS =
        (P::OWF_CONSTS::WITNESS_BITS + 128 * VOLE_BLOCK - 1) / (128 * VOLE_BLOCK);
    constexpr static std::size_t WITNESS_BLOCKS_EXTENDED =
        (P::witness_extended_bits + 128 * VOLE_BLOCK - 1) / (128 * VOLE_BLOCK);

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
    constexpr static std::size_t VOLE_COMMIT_SIZE = (VOLE_ROWS / 8) * (P::tau_v - 1);

    constexpr static std::size_t QUICKSILVER_ROWS_EXTENDED =
        P::witness_extended_bits + (P::OWF_CONSTS::QS_DEGREE - 1) * P::secpar_bits;
    constexpr static std::size_t QUICKSILVER_ROWS_PADDED_EXTENDED =
        ((QUICKSILVER_ROWS_EXTENDED + QUICKSILVER_ROW_PAD_TO - 1) / QUICKSILVER_ROW_PAD_TO) *
        QUICKSILVER_ROW_PAD_TO;
    constexpr static std::size_t VOLE_ROWS_EXTENDED =
        QUICKSILVER_ROWS_EXTENDED + VOLE_CHECK::HASH_BYTES * 8;
    constexpr static std::size_t VOLE_COL_BLOCKS_EXTENDED =
        (VOLE_ROWS_EXTENDED + 128 * VOLE_BLOCK - 1) / (128 * VOLE_BLOCK);
    constexpr static std::size_t VOLE_COL_STRIDE_EXTENDED =
        VOLE_COL_BLOCKS_EXTENDED * 16 * VOLE_BLOCK;
    constexpr static std::size_t VOLE_ROWS_PADDED_EXTENDED =
        VOLE_COL_BLOCKS_EXTENDED * 128 * VOLE_BLOCK;
    constexpr static std::size_t VOLE_COMMIT_SIZE_EXTENDED =
        (VOLE_ROWS_EXTENDED / 8) * (P::tau_v - 1);

    constexpr static std::size_t VOLE_COMMIT_CHECK_SIZE = 2 * P::secpar_bytes;

    constexpr static std::size_t PRG_VOLE_BLOCK_SIZE_SHIFT = []
    {
        // if constexpr ((P::vole_prg_v == prg::aes_secpar_fixed_key_ctr ||
        //                P::vole_prg_v == prg::rijndael_fixed_key_ctr) &&
        //               P::secpar_v == secpar::s256)
        //     return 1;
        // else
        return 0;
    }();
    constexpr static std::size_t PRG_VOLE_BLOCK_SIZE = 1 << PRG_VOLE_BLOCK_SIZE_SHIFT;
    constexpr static std::size_t PRG_VOLE_BLOCKS_SHIFT =
        VOLE_BLOCK_SHIFT - PRG_VOLE_BLOCK_SIZE_SHIFT;
    constexpr static std::size_t PRG_VOLE_BLOCKS = 1 << PRG_VOLE_BLOCKS_SHIFT;
    constexpr static std::size_t VOLE_WIDTH_SHIFT =
        AES_PREFERRED_WIDTH_SHIFT - PRG_VOLE_BLOCKS_SHIFT;
    constexpr static std::size_t VOLE_WIDTH = 1 << VOLE_WIDTH_SHIFT;

    constexpr static std::size_t SALT_BYTES = P::secpar_bytes;
    constexpr static std::size_t DIGEST_BYTES = 2 * P::secpar_bytes;
    constexpr static std::size_t COMMITMENT_BYTES = 2 * P::secpar_bytes;
    constexpr static std::size_t RESPONSE_BYTES = 2 * P::secpar_bytes;

    constexpr static std::size_t SALT_OFFSET = 0;
    constexpr static std::size_t COMMITMENT_OFFSET = SALT_OFFSET + SALT_BYTES;
    constexpr static std::size_t RESPONSE_OFFSET = COMMITMENT_OFFSET + COMMITMENT_BYTES;
    constexpr static std::size_t SIGNATURE_TRANSCRIPT_BYTES =
        VOLE_COMMIT_SIZE + VOLE_CHECK::PROOF_BYTES + P::OWF_CONSTS::WITNESS_BYTES +
        QS::PROOF_BYTES + P::bavc_t::OPEN_SIZE + P::secpar_bytes +
        sizeof(typename P::vole_prg_t::iv_t) +
        P::grinding_counter_size;
    constexpr static std::size_t SIGNATURE_BYTES = SIGNATURE_TRANSCRIPT_BYTES;

    using check_witness_bits_ =
        std::enable_if_t<P::OWF_CONSTS::WITNESS_BYTES * 8 >= P::OWF_CONSTS::WITNESS_BITS>;
};

template <typename P> using CONSTANTS = SYDO_CONSTANTS<P>;

} // namespace sydo

#endif
