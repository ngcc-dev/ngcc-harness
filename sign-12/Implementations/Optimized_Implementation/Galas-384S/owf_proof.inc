#ifndef OWF_PROOF_INC
#define OWF_PROOF_INC

#include "aes.hpp"
#include "constants.hpp"
#include "faest_keys.hpp"
#include "owf_proof.hpp"
#include "owf_proof_tools.hpp"
#include "parameters.hpp"
#include "quicksilver.hpp"
#include "galas_qs_gadgets.hpp"

namespace faest
{

template <secpar S> constexpr static std::size_t N_WD = secpar_to_bits(S) / 32;
template <secpar S, owf O> constexpr static std::size_t S_ENC = OWF_CONSTANTS<S, O>::OWF_ENC_SBOXES;

template <secpar S, owf O> using owf_block = OWF_CONSTANTS<S, O>::block_t;

template <typename QS, typename OC>
using owf_round_key_bits =
    std::array<quicksilver_gf2<QS>, 8 * OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1)>;

template <typename QS, typename OC>
using owf_round_key_bytes =
    std::array<quicksilver_gfsecpar<QS>, OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1)>;

template <typename QS> static void key_sched_fwd(const QS* state, quicksilver_gf2<QS>* output)
{
    constexpr secpar S = QS::secpar_v;
    for (size_t bit_i = 0; bit_i < secpar_to_bits(S); ++bit_i)
    {
        output[bit_i] = state->get_witness_bit(bit_i);
    }
    // current index in the extended witness
    size_t i_wd = secpar_to_bits(S);
    for (size_t word_j = N_WD<S>; word_j < 4 * (AES_ROUNDS<S> + 1); ++word_j)
    {
        if (word_j % N_WD<S> == 0 || (N_WD<S> > 6 && word_j % N_WD<S> == 4))
        {
            for (size_t bit_i = 0; bit_i < 32; ++bit_i)
            {
                output[32 * word_j + bit_i] = state->get_witness_bit(i_wd + bit_i);
            }
            i_wd += 32;
        }
        else
        {
            for (size_t bit_i = 0; bit_i < 32; ++bit_i)
            {
                output[32 * word_j + bit_i] =
                    output[32 * (word_j - N_WD<S>)+bit_i] + output[32 * (word_j - 1) + bit_i];
            }
        }
    }
}

template <typename QS, size_t n>
static void lift_bits(const std::array<quicksilver_gf2<QS>, 8 * n>& bits,
                      std::array<quicksilver_gfsecpar<QS>, n>& bytes)
{
    for (size_t byte_i = 0; byte_i < n; ++byte_i)
        bytes[byte_i] = quicksilver_gfsecpar<QS>::combine_8_bits(&bits[8 * byte_i]);
}

template <typename QS, size_t n>
static void lift_bits_and_square(QS* state, const std::array<quicksilver_gf2<QS>, 8 * n>& bits,
                                 std::array<quicksilver_gfsecpar<QS>, n>& bytes,
                                 std::array<quicksilver_gfsecpar<QS>, n>& bytes_sq)
{
    auto bits_sq = state->template const_gf2_array<8>();
    for (size_t byte_i = 0; byte_i < n; ++byte_i)
    {
        square_8_bits(bits_sq.data(), &bits[8 * byte_i]);
        bytes[byte_i] = quicksilver_gfsecpar<QS>::combine_8_bits(&bits[8 * byte_i]);
        bytes_sq[byte_i] = quicksilver_gfsecpar<QS>::combine_8_bits(bits_sq.data());
        if constexpr (!QS::is_verifier)
        {
            FAEST_ASSERT((bytes[byte_i].value() * bytes[byte_i].value())
                       .template reduce_to<QS::secpar_bits>() == bytes_sq[byte_i].value());
        }
    }
}

template <typename QS, owf O>
static void key_sched_bkwd(
    const QS* state, const owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
    std::array<quicksilver_gf2<QS>, 8 * OWF_CONSTANTS<QS::secpar_v, O>::OWF_KEY_SCHEDULE_SBOXES>&
        inv_outputs_bits)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    size_t i_wd = 0;         // bit index to the round key word we are currently handling
    size_t i_rcon = 0;       // round constant index
    bool remove_rcon = true; // flag indicating if we need to remove the round constant from the
                             // next word
    for (size_t sbox_j = 0; sbox_j < OC::OWF_KEY_SCHEDULE_SBOXES; ++sbox_j)
    {
        // load the witness byte
        auto sbox_out = state->template const_gf2_array<8>();
        for (size_t bit_i = 0; bit_i < 8; ++bit_i)
        {
            sbox_out[bit_i] = state->get_witness_bit(secpar_to_bits(S) + sbox_j * 8 + bit_i);
            // remove the byte that was xored in
            sbox_out[bit_i] = sbox_out[bit_i] + round_key_bits[i_wd + 8 * (sbox_j % 4) + bit_i];
        }
        // (possibly) remove the round constant
        if (sbox_j % 4 == 0 && remove_rcon)
        {
            // remove the round constant from the first byte of every word coming through the sboxes
            for (size_t bit_i = 0; bit_i < 8; ++bit_i)
            {
                if ((aes_round_constants[i_rcon] >> bit_i) & 1)
                {
                    sbox_out[bit_i] = sbox_out[bit_i] + 1;
                }
            }
            ++i_rcon;
        }

        // pointer to the part of the array that corresponds to the current sbox
        auto inv_out = &inv_outputs_bits[8 * sbox_j];
        for (size_t i = 0; i < 8; ++i)
            inv_out[(i + 1) % 8] = sbox_out[i];
        for (size_t i = 0; i < 8; ++i)
            inv_out[(i + 3) % 8] = inv_out[(i + 3) % 8] + sbox_out[i];
        for (size_t i = 0; i < 8; ++i)
            inv_out[(i + 6) % 8] = inv_out[(i + 6) % 8] + sbox_out[i];
        inv_out[0] = inv_out[0] + 1;
        inv_out[2] = inv_out[2] + 1;

        if (sbox_j % 4 == 3)
        {
            // increase i_wd to point to the next word
            if (S == secpar::s192)
            {
                i_wd += 192;
            }
            else
            {
                i_wd += 128;
                if (S == secpar::s256)
                {
                    remove_rcon = !remove_rcon;
                }
            }
        }
    }
}

template <typename QS, owf O>
static void
key_sched_constraints(QS* state,
                      owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
                      owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes,
                      owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes_sq)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    auto key_schedule_inv_outs_bits =
        state->template const_gf2_array<8 * OC::OWF_KEY_SCHEDULE_SBOXES>();
    key_sched_fwd(state, round_key_bits.data());
    key_sched_bkwd<QS, O>(state, round_key_bits, key_schedule_inv_outs_bits);
    auto key_schedule_inv_outs =
        state->template const_gfsecpar_array<OC::OWF_KEY_SCHEDULE_SBOXES>();
    auto key_schedule_inv_outs_sq =
        state->template const_gfsecpar_array<OC::OWF_KEY_SCHEDULE_SBOXES>();

    if constexpr (is_owf_with_norm_proof(O))
    {
        lift_bits(round_key_bits, round_key_bytes);
        lift_bits_and_square(state, key_schedule_inv_outs_bits, key_schedule_inv_outs,
                             key_schedule_inv_outs_sq);
    }
    else if constexpr (is_owf_with_zero_sboxes(O))
    {
        lift_bits_and_square(state, round_key_bits, round_key_bytes, round_key_bytes_sq);
        lift_bits_and_square(state, key_schedule_inv_outs_bits, key_schedule_inv_outs,
                             key_schedule_inv_outs_sq);
    }
    else
    {
        lift_bits(round_key_bits, round_key_bytes);
        lift_bits(key_schedule_inv_outs_bits, key_schedule_inv_outs);
    }

    // byte index of the current word to read from the round keys
    size_t i_wd = 4 * (N_WD<S> - 1);
    // for 256 bit we only rotate every second time
    bool rotate_word = true;
    auto lhss = state->template const_gfsecpar_array<4>();
    auto rhss = state->template const_gfsecpar_array<4>();
    auto lhss_sq = state->template const_gfsecpar_array<4>();
    auto rhss_sq = state->template const_gfsecpar_array<4>();
    auto bits_sq = state->template const_gf2_array<8>();
    for (size_t sboxwd_j = 0; sboxwd_j < OC::OWF_KEY_SCHEDULE_SBOXES / 4; ++sboxwd_j)
    {
        if (rotate_word)
        {
            for (size_t row_k = 0; row_k < 4; ++row_k)
            {
                lhss[(row_k + 3) % 4] = round_key_bytes[i_wd + row_k];
                rhss[row_k] = key_schedule_inv_outs[4 * sboxwd_j + row_k];
                if constexpr (is_owf_with_zero_sboxes(O))
                {
                    if constexpr (is_owf_with_norm_proof(O))
                    {
                        square_8_bits(bits_sq.data(), &round_key_bits[8 * (i_wd + row_k)]);
                        lhss_sq[(row_k + 3) % 4] =
                            quicksilver_gfsecpar<QS>::combine_8_bits(bits_sq.data());
                    }
                    else
                    {
                        lhss_sq[(row_k + 3) % 4] = round_key_bytes_sq[i_wd + row_k];
                    }
                    rhss_sq[row_k] = key_schedule_inv_outs_sq[4 * sboxwd_j + row_k];
                }
            }
        }
        else
        {
            for (size_t row_k = 0; row_k < 4; ++row_k)
            {
                lhss[row_k] = round_key_bytes[i_wd + row_k];
                rhss[row_k] = key_schedule_inv_outs[4 * sboxwd_j + row_k];
                if constexpr (is_owf_with_zero_sboxes(O))
                {
                    if constexpr (is_owf_with_norm_proof(O))
                    {
                        square_8_bits(bits_sq.data(), &round_key_bits[8 * (i_wd + row_k)]);
                        lhss_sq[row_k] = quicksilver_gfsecpar<QS>::combine_8_bits(bits_sq.data());
                    }
                    else
                    {
                        lhss_sq[row_k] = round_key_bytes_sq[i_wd + row_k];
                    }
                    rhss_sq[row_k] = key_schedule_inv_outs_sq[4 * sboxwd_j + row_k];
                }
            }
        }
        for (size_t row_k = 0; row_k < 4; ++row_k)
        {
            if constexpr (is_owf_with_zero_sboxes(O))
            {
                const auto x = lhss[row_k];
                const auto x_sq = lhss_sq[row_k];
                const auto y = rhss[row_k];
                const auto y_sq = rhss_sq[row_k];
                state->add_constraint(x_sq * y + x);
                state->add_constraint(x * y_sq + y);
            }
            else
            {
                state->add_inverse_constraints(lhss[row_k], rhss[row_k]);
            }
        }
        // increase i_wd to point to the next word
        if (S == secpar::s192)
        {
            i_wd += 24;
        }
        else
        {
            i_wd += 16;
            if (S == secpar::s256)
            {
                rotate_word = !rotate_word;
            }
        }
    }
}

// load the round keys into quicksilver values and "bake" EM secret key into the first round key
template <typename QS, owf O>
static void
load_fixed_round_key(const QS* state,
                     owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
                     owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes,
                     owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes_sq,
                     const rijndael_round_keys<QS::secpar_v>* fixed_key)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    const uint8_t* rk_bytes = (const uint8_t*)fixed_key;
    auto bits_sq = state->template const_gf2_array<8>();

    for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
    {
        for (size_t bit_i = 0; bit_i < 8; ++bit_i)
        {
            round_key_bits[8 * byte_j + bit_i] =
                poly1::load(rk_bytes[byte_j], bit_i) + state->get_witness_bit(8 * byte_j + bit_i);
        }
        round_key_bytes[byte_j] =
            quicksilver_gfsecpar<QS>::combine_8_bits(&round_key_bits[8 * byte_j]);
        if constexpr (is_owf_with_zero_sboxes(O) && !is_owf_with_norm_proof(O))
        {
            square_8_bits(bits_sq.data(), &round_key_bits[8 * byte_j]);
            round_key_bytes_sq[byte_j] = quicksilver_gfsecpar<QS>::combine_8_bits(bits_sq.data());
        }
    }

    for (size_t byte_j = OC::OWF_BLOCK_SIZE; byte_j < OC::OWF_BLOCK_SIZE * (OC::OWF_ROUNDS + 1);
         ++byte_j)
    {
        for (size_t bit_i = 0; bit_i < 8; ++bit_i)
        {
            round_key_bits[8 * byte_j + bit_i] =
                quicksilver_gf2<QS>(poly1::load(rk_bytes[byte_j], bit_i), state);
        }
        round_key_bytes[byte_j] =
            quicksilver_gfsecpar<QS>::combine_8_bits(&round_key_bits[8 * byte_j]);
        if constexpr (is_owf_with_zero_sboxes(O) && !is_owf_with_norm_proof(O))
        {
            square_8_bits(bits_sq.data(), &round_key_bits[8 * byte_j]);
            round_key_bytes_sq[byte_j] = quicksilver_gfsecpar<QS>::combine_8_bits(bits_sq.data());
        }
    }
}

// load 8 consecutive bits from s into QS GF(2) values, then combine them into a GF(2^secpar)
// value in the GF(2^8) subfield
template <typename QS>
static inline quicksilver_gfsecpar<QS, 0> load_const_8_bits_and_combine(const QS* state,
                                                                        const void* s)
{
    auto input_bits = state->template const_gf2_array<8, 0>();
    for (size_t bit_j = 0; bit_j < 8; ++bit_j)
        input_bits[bit_j] = quicksilver_gf2<QS, 0>(poly1::load(*(uint8_t*)s, bit_j), state);
    return quicksilver_gfsecpar<QS, 0>::combine_8_bits(input_bits.data());
}

template <typename QS>
static inline std::pair<quicksilver_gfsecpar<QS, 0>, quicksilver_gfsecpar<QS, 0>>
load_const_8_bits_and_combine_with_square(const QS* state, const void* s)
{
    auto input_bits = state->template const_gf2_array<8, 0>();
    auto squared_bits = state->template const_gf2_array<8, 0>();
    for (size_t bit_j = 0; bit_j < 8; ++bit_j)
        input_bits[bit_j] = quicksilver_gf2<QS, 0>(poly1::load(*(uint8_t*)s, bit_j), state);
    square_8_bits(squared_bits.data(), input_bits.data());
    return std::make_pair(quicksilver_gfsecpar<QS, 0>::combine_8_bits(input_bits.data()),
                          quicksilver_gfsecpar<QS, 0>::combine_8_bits(squared_bits.data()));
}

template <typename QS, owf O>
static void
enc_fwd(const QS* state,
        const owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes,
        const owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes_sq,
        size_t witness_bit_offset, owf_block<QS::secpar_v, O> in,
        std::array<quicksilver_gfsecpar<QS>, S_ENC<QS::secpar_v, O>>& output,
        std::array<quicksilver_gfsecpar<QS>, S_ENC<QS::secpar_v, O>>& output_sq)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    const uint8_t* in_bytes = (uint8_t*)&in;

    // first round: only add the round key
    for (size_t byte_i = 0; byte_i < OC::OWF_BLOCK_SIZE; ++byte_i)
    {
        if constexpr (!is_owf_with_zero_sboxes(O))
        {
            const auto input_byte = load_const_8_bits_and_combine(state, &in_bytes[byte_i]);
            output[byte_i] = round_key_bytes[byte_i] + input_byte;
        }
        else
        {
            const auto [input_byte, input_byte_sq] =
                load_const_8_bits_and_combine_with_square(state, &in_bytes[byte_i]);
            output[byte_i] = round_key_bytes[byte_i] + input_byte;
            output_sq[byte_i] = round_key_bytes_sq[byte_i] + input_byte_sq;
        }
    }

    const poly_secpar<S> c_two = poly_secpar<S>::from_8_byte(0x02);
    const poly_secpar<S> c_three = poly_secpar<S>::from_8_byte(0x03);
    const poly_secpar<S> c_two_sq = (c_two * c_two).template reduce_to<secpar_to_bits(S)>();
    const poly_secpar<S> c_three_sq = (c_three * c_three).template reduce_to<secpar_to_bits(S)>();

    size_t round_key_byte_offset = OC::OWF_BLOCK_SIZE;
    size_t output_byte_offset = OC::OWF_BLOCK_SIZE;

    auto col_wit_bytes = state->template const_gfsecpar_array<4>();
    auto col_wit_bytes_sq = state->template const_gfsecpar_array<4>();
    for (size_t round_i = 1; round_i < OC::OWF_ROUNDS; ++round_i)
    {
        for (size_t col_j = 0; col_j < NUM_COLS<S, O>; ++col_j)
        {
            for (size_t row_k = 0; row_k < 4; ++row_k)
            {
                auto witness_bits = state->load_witness_8_bits(witness_bit_offset + row_k * 8);
                col_wit_bytes[row_k] =
                    quicksilver_gfsecpar<QS>::combine_8_bits(witness_bits.data());
                if constexpr (is_owf_with_zero_sboxes(O))
                {
                    auto squared_bits = state->template const_gf2_array<8>();
                    square_8_bits(squared_bits.data(), witness_bits.data());
                    col_wit_bytes_sq[row_k] =
                        quicksilver_gfsecpar<QS>::combine_8_bits(squared_bits.data());
                }
            }
            for (size_t row_k = 0; row_k < 4; ++row_k)
            {
                output[output_byte_offset + row_k] =
                    c_two * col_wit_bytes[row_k] + c_three * col_wit_bytes[(row_k + 1) % 4] +
                    col_wit_bytes[(row_k + 2) % 4] + col_wit_bytes[(row_k + 3) % 4] +
                    round_key_bytes[round_key_byte_offset + row_k];
                if constexpr (is_owf_with_zero_sboxes(O))
                {
                    output_sq[output_byte_offset + row_k] =
                        c_two_sq * col_wit_bytes_sq[row_k] +
                        c_three_sq * col_wit_bytes_sq[(row_k + 1) % 4] +
                        col_wit_bytes_sq[(row_k + 2) % 4] + col_wit_bytes_sq[(row_k + 3) % 4] +
                        round_key_bytes_sq[round_key_byte_offset + row_k];
                }
            }
            witness_bit_offset += 32;
            round_key_byte_offset += 4;
            output_byte_offset += 4;
        }
    }
}

template <typename QS, owf O>
static void enc_bkwd(const QS* state,
                     const owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
                     size_t witness_bit_offset, owf_block<QS::secpar_v, O> out,
                     std::array<quicksilver_gfsecpar<QS>, S_ENC<QS::secpar_v, O>>& output,
                     std::array<quicksilver_gfsecpar<QS>, S_ENC<QS::secpar_v, O>>& output_sq)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    const uint8_t* out_bytes = (uint8_t*)&out;
    const size_t last_round_key_bit_offset = 8 * OC::OWF_ROUNDS * OC::OWF_BLOCK_SIZE;

    for (size_t round_i = 0; round_i < OC::OWF_ROUNDS;
         ++round_i, witness_bit_offset += OC::OWF_BLOCK_SIZE * 8)
    {
        for (size_t col_j = 0; col_j < NUM_COLS<S, O>; ++col_j)
        {
            for (size_t row_k = 0; row_k < 4; ++row_k)
            {
                auto witness_bits = state->template const_gf2_array<8>();
                size_t inv_shifted_index = inverse_shift_rows_perm<S, O>(row_k, col_j);
                if (round_i < OC::OWF_ROUNDS - 1)
                {
                    // read witness bits directly
                    for (size_t bit_i = 0; bit_i < 8; ++bit_i)
                    {
                        witness_bits[bit_i] = state->get_witness_bit(witness_bit_offset +
                                                                     8 * inv_shifted_index + bit_i);
                    }
                }
                else
                {
                    // compute witness bits from the last round key and the output
                    for (size_t bit_i = 0; bit_i < 8; ++bit_i)
                    {
                        witness_bits[bit_i] = poly1::load(out_bytes[inv_shifted_index], bit_i) +
                                              round_key_bits[last_round_key_bit_offset +
                                                             8 * inv_shifted_index + bit_i];
                        if constexpr (is_owf_with_aes_em(O))
                        {
                            witness_bits[bit_i] =
                                witness_bits[bit_i] +
                                state->get_witness_bit(8 * inv_shifted_index + bit_i);
                        }
                    }
                }

                auto inv_out = state->template const_gf2_array<8>();
                for (size_t i = 0; i < 8; ++i)
                    inv_out[(i + 1) % 8] = witness_bits[i];
                for (size_t i = 0; i < 8; ++i)
                    inv_out[(i + 3) % 8] += witness_bits[i];
                for (size_t i = 0; i < 8; ++i)
                    inv_out[(i + 6) % 8] += witness_bits[i];
                inv_out[0] = inv_out[0] + 1;
                inv_out[2] = inv_out[2] + 1;

                // lift into a field element and store in the output buffer
                output[round_i * OC::OWF_BLOCK_SIZE + 4 * col_j + row_k] =
                    quicksilver_gfsecpar<QS>::combine_8_bits(inv_out.data());

                if constexpr (is_owf_with_zero_sboxes(O))
                {
                    // square and lift
                    auto squared_bits = state->template const_gf2_array<8>();
                    square_8_bits(squared_bits.data(), inv_out.data());
                    output_sq[round_i * OC::OWF_BLOCK_SIZE + 4 * col_j + row_k] =
                        quicksilver_gfsecpar<QS>::combine_8_bits(squared_bits.data());
                }
            }
        }
    }
}

template <typename QS, owf O>
static void
enc_constraints_v2(QS* qs_state, size_t witness_bit_offset,
                   const owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
                   const owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes,
                   owf_block<QS::secpar_v, O> in, owf_block<QS::secpar_v, O> out)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    static_assert(OC::QS_DEGREE >= 3);

    auto state_bits = qs_state->template const_gf2_2d_array<8, OC::OWF_BLOCK_SIZE>();
    auto state_bits_2 = qs_state->template const_gf2_2d_array<8, OC::OWF_BLOCK_SIZE>();
    auto compressed_invnorm = qs_state->template const_gf2_array<4>();
    auto decompressed_invnorm = qs_state->template const_gf2_array<8>();
    auto invnorms_with_conjugates =
        qs_state->template const_gfsecpar_2d_array<4, OC::OWF_BLOCK_SIZE>();
    auto state_with_conjugates =
        qs_state->template const_gfsecpar_2d_array<8, OC::OWF_BLOCK_SIZE>();
    auto state_with_inverse_conjugates =
        qs_state->template const_gfsecpar_2d_array<8, OC::OWF_BLOCK_SIZE, 2>();
    auto state_with_sq = qs_state->template const_gfsecpar_2d_array<OC::OWF_BLOCK_SIZE, 2, 2>();

    // slighly unpretty solution due to last minute spec changes
    // TODO: make prettier
    auto round_key_bytes_sq =
        qs_state->template const_gfsecpar_array<OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1), 2>();
    for (size_t i = 0; i < round_key_bytes.size(); ++i)
    {
        round_key_bytes_sq[i] = round_key_bytes[i] * round_key_bytes[i];
    }

    // prepare the initial state: AddRoundKey to the input block
    {
        const uint8_t* in_bytes = reinterpret_cast<const uint8_t*>(&in);
        for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
        {
            for (size_t bit_i = 0; bit_i < 8; ++bit_i)
            {
                state_bits[byte_j][bit_i] = round_key_bits[8 * byte_j + bit_i] +
                                            poly1::load_offset8(&in_bytes[byte_j], bit_i);
            }
        }
    }

    for (size_t r = 0; r < OC::OWF_ROUNDS / 2; ++r)
    {
        for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
        {
            // - load inv_norm from witness and decompress
            for (size_t i = 0; i < 4; ++i)
            {
                compressed_invnorm[i] = qs_state->get_witness_bit(witness_bit_offset + i);
            }
            witness_bit_offset += 4;
            decompress_gf16_subfield(decompressed_invnorm, compressed_invnorm);
            // - compute the first four GF(256)/GF(2) conjugates of the invnorms
            gf256_gf2_conjugates<4>(qs_state, invnorms_with_conjugates[byte_j],
                                    decompressed_invnorm);
            // - compute all GF(256)/GF(2) conjugates of the state
            gf256_gf2_conjugates<8>(qs_state, state_with_conjugates[byte_j], state_bits[byte_j]);

            // - add InvNormConstraints: check that y * a^2 * a^16 = a for invnorm y and state a
            qs_state->add_constraint(invnorms_with_conjugates[byte_j][0] *
                                         state_with_conjugates[byte_j][1] *
                                         state_with_conjugates[byte_j][4] +
                                     state_with_conjugates[byte_j][0]);

            // - compute conjugates of the inversion output
            for (size_t i = 0; i < 8; ++i)
            {
                state_with_inverse_conjugates[byte_j][i] =
                    state_with_conjugates[byte_j][(i + 4) % 8] *
                    invnorms_with_conjugates[byte_j][i % 4];
            }
        }

        // - use conjugates to compute SBoxAffine on state and squared state
        // - compute ShiftRows (shuffle stuff around)
        for (size_t row_k = 0; row_k < 4; ++row_k)
        {
            for (size_t col_j = 0; col_j < NUM_COLS<S, O>; ++col_j)
            {
                const auto in_index = 4 * col_j + row_k;
                const auto out_index = inverse_shift_rows_perm<S, O>(row_k, col_j);
                state_with_sq[0][out_index] =
                    sbox_affine<false>(state_with_inverse_conjugates[in_index]);
                state_with_sq[1][out_index] =
                    sbox_affine<true>(state_with_inverse_conjugates[in_index]);
            }
        }

        // - compute MixColumns on state and squared state
        // - AddRoundKey
        // -> first inputs to inverse constraints
        {
            const auto this_round_key_bytes_index = (2 * r + 1) * OC::OWF_BLOCK_SIZE;
            mix_columns_and_add_roundkey_inplace<S, O, false>(
                qs_state, state_with_sq[0], &round_key_bytes[this_round_key_bytes_index]);
            mix_columns_and_add_roundkey_inplace<S, O, true>(
                qs_state, state_with_sq[1], &round_key_bytes_sq[this_round_key_bytes_index]);
        }

        // - load witness as bits
        if (r == OC::OWF_ROUNDS / 2 - 1)
        {
            // compute from the last round key and the output block
            const auto* last_round_key_bits =
                &round_key_bits[8 * OC::OWF_BLOCK_SIZE * OC::OWF_ROUNDS];
            const uint8_t* out_bytes = reinterpret_cast<const uint8_t*>(&out);
            for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
            {
                for (size_t bit_i = 0; bit_i < 8; ++bit_i)
                {
                    state_bits[byte_j][bit_i] = last_round_key_bits[8 * byte_j + bit_i] +
                                                poly1::load_offset8(&out_bytes[byte_j], bit_i);
                    if constexpr (is_owf_with_aes_em(O))
                    {
                        state_bits[byte_j][bit_i] += qs_state->get_witness_bit(8 * byte_j + bit_i);
                    }
                }
            }
        }
        else
        {
            // load directly from the witness
            for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
            {
                for (size_t bit_i = 0; bit_i < 8; ++bit_i)
                {
                    state_bits[byte_j][bit_i] =
                        qs_state->get_witness_bit(witness_bit_offset + bit_i);
                }
                witness_bit_offset += 8;
            }
        }

        // - compute InverseShiftRows and InverseAffine on bits
        bitwise_inverse_shift_rows_and_sbox_affine<S, O, true>(qs_state, state_bits_2, state_bits);

        for (size_t byte_j = 0; byte_j < OC::OWF_BLOCK_SIZE; ++byte_j)
        {
            // compute second inputs to constraints
            auto squared_bits = qs_state->template const_gf2_array<8>();
            square_8_bits(squared_bits.data(), state_bits_2[byte_j].data());
            const auto x = quicksilver_gfsecpar<QS>::combine_8_bits(state_bits_2[byte_j].data());
            const auto x_sq = quicksilver_gfsecpar<QS>::combine_8_bits(squared_bits.data());
            const auto y = state_with_sq[0][byte_j];
            const auto y_sq = state_with_sq[1][byte_j];
            // - add inversion or zero constraints
            qs_state->add_constraint(x_sq * y + x);
            qs_state->add_constraint(x * y_sq + y);
        }

        // - update state for the next round
        if (r != OC::OWF_ROUNDS / 2 - 1)
        {
            // compute MixColumns and AddRoundKey on bits
            const auto* this_round_key_bits = &round_key_bits[(2 * r + 2) * 8 * OC::OWF_BLOCK_SIZE];
            bitwise_mix_columns_and_add_roundkey_inplace<S, O>(qs_state, state_bits,
                                                               this_round_key_bits);
        }
    }
}

template <typename QS, owf O>
static void
enc_constraints(QS* qs_state,
                const owf_round_key_bits<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bits,
                const owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes,
                const owf_round_key_bytes<QS, OWF_CONSTANTS<QS::secpar_v, O>>& round_key_bytes_sq,
                size_t block_num, owf_block<QS::secpar_v, O> in, owf_block<QS::secpar_v, O> out)
{
    constexpr secpar S = QS::secpar_v;
    using OC = OWF_CONSTANTS<S, O>;
    // compute the starting index of the witness bits corresponding to the s-boxes in this round of
    // encryption
    FAEST_ASSERT(block_num < OC::OWF_BLOCKS);
    size_t witness_bit_offset =
        OC::OWF_KEY_WITNESS_BITS + block_num * OC::OWF_ENC_WITNESS_BITS_PER_BLOCK;

    if constexpr (!is_owf_with_norm_proof(O))
    {
        // FAEST v1 implementation
        auto inv_inputs = qs_state->template const_gfsecpar_array<S_ENC<S, O>>();
        auto inv_outputs = qs_state->template const_gfsecpar_array<S_ENC<S, O>>();
        auto inv_inputs_sq = qs_state->template const_gfsecpar_array<S_ENC<S, O>>();
        auto inv_outputs_sq = qs_state->template const_gfsecpar_array<S_ENC<S, O>>();
        enc_fwd<QS, O>(qs_state, round_key_bytes, round_key_bytes_sq, witness_bit_offset, in,
                       inv_inputs, inv_inputs_sq);
        enc_bkwd<QS, O>(qs_state, round_key_bits, witness_bit_offset, out, inv_outputs,
                        inv_outputs_sq);

        if constexpr (!is_owf_with_zero_sboxes(O))
        {
            for (size_t sbox_j = 0; sbox_j < S_ENC<S, O>; ++sbox_j)
            {
                qs_state->add_inverse_constraints(inv_inputs[sbox_j], inv_outputs[sbox_j]);
            }
        }
        else
        {
            for (size_t sbox_j = 0; sbox_j < S_ENC<S, O>; ++sbox_j)
            {
                qs_state->add_constraint(inv_inputs_sq[sbox_j] * inv_outputs[sbox_j] +
                                         inv_inputs[sbox_j]);
                qs_state->add_constraint(inv_inputs[sbox_j] * inv_outputs_sq[sbox_j] +
                                         inv_outputs[sbox_j]);
            }
        }
    }
    else
    {
        // FAEST v2 implementation
        enc_constraints_v2<QS, O>(qs_state, witness_bit_offset, round_key_bits, round_key_bytes, in,
                                  out);
    }
}

template <typename QS> static void shrunk_keyspace_constraint(QS* state)
{
    auto k0 = quicksilver_gfsecpar<QS>(state->get_witness_bit(0));
    auto k1 = quicksilver_gfsecpar<QS>(state->get_witness_bit(1));
    state->add_constraint(k0 * k1);
}

template <typename QS, typename PK>
inline void galas_owf_constraints(QS& st, const PK* pk)
{
    constexpr secpar S = QS::secpar_v;
    constexpr size_t n = secpar_to_bits(S);
    constexpr size_t m = n / 2;

    // witness layout for Galas:
    // [0..n-1]         = k bits
    // [n..n+m-1]       = sigma0 bits (packed after P-selection already in witness generation)
    // [n+m..n+2m-1]    = sigma1 bits
    // [n+2m..n+3m-1]   = sigma2 bits

    // 1 Load k bits
    std::array<quicksilver_gf2<QS,1>, n> k_bits{};
    faest::load_witness_bits(st, 0, k_bits.data(), n);
    auto k = faest::qs_rho_pow2i_from_k_bits_epow2<QS,1>(st, k_bits.data(), poly_secpar<S>::set_zero(), 0);
    auto k_frob = qs_precompute_k_frob_all<QS,1>(st, k_bits.data());
    (void)k;

    // 1b Restricted-keyspace constraint k0 * k1 = 1 (mirrors the FAEST v2
    // shrunk_keyspace_constraint).  Key generation fixes the two low-order
    // serialized key bits of k to one; this degree-two constraint enforces the
    // same condition inside the QuickSilver proof, so the rho <= 1/4
    // false-instance density factor used by the EUF-KO reduction holds on the
    // prover side as well.  Matches galas_constraints_internal in the NGCC C99
    // reference, which rejects w_bits[0]==0 || w_bits[1]==0.
    {
        auto k0 = quicksilver_gfsecpar<QS,1>(st.get_witness_bit(0));
        auto k1 = quicksilver_gfsecpar<QS,1>(st.get_witness_bit(1));
        // Constraint: k0 * k1 == 1, i.e. k0*k1 + 1 == 0
        auto c_key = k0 * k1 + 1;
        st.add_constraint(c_key);
    }

    // 2 Load sigma0/1/2 (embedded to GF(2^λ) via B'_i)
    auto sigma0 = faest::qs_embed_sigma_from_bprime<QS>(st, n);
    auto sigma1 = faest::qs_embed_sigma_from_bprime<QS>(st, n + m);
    auto sigma2 = faest::qs_embed_sigma_from_bprime<QS>(st, n + 2*m);

    // 3 Build rho0/rho1/rho2, then cache a_i=M_i(rho_i) and its Frobenius powers.
    // New structure: rho0=x+k+c0, rho1=k+c1, rho2=k+c2.
    // 3 Build rho0/rho1/rho2 (S-box inputs)
    // rho0 = x ⊕ k ⊕ c0
    // rho1 = k ⊕ c1
    // rho2 = k ⊕ c2
    // Here x is pk->owf_input[0], treated as constant.

    // Load constants c0/c1/c2 and x as poly_secpar<S>
    std::array<uint8_t, secpar_to_bytes(S)> x_bytes{};
    std::memcpy(x_bytes.data(), &pk->owf_input[0], secpar_to_bytes(S));
    poly_secpar<S> Xc = poly_secpar<S>::load(x_bytes.data());

    poly_secpar<S> C0 = poly_secpar<S>::load(gala_constants<S>::c0.data());
    poly_secpar<S> C1 = poly_secpar<S>::load(gala_constants<S>::c1.data());
    poly_secpar<S> C2 = poly_secpar<S>::load(gala_constants<S>::c2.data());

    auto c_frob_rho0 = qs_precompute_const_frob_all<QS>(st, (Xc + C0));
    auto c_frob_rho1 = qs_precompute_const_frob_all<QS>(st, C1);
    auto c_frob_rho2 = qs_precompute_const_frob_all<QS>(st, C2);

    auto a0_frob = qs_precompute_gala_input_frob_all<QS,0>(st, k_frob, c_frob_rho0);
    auto a1_frob = qs_precompute_gala_input_frob_all<QS,1>(st, k_frob, c_frob_rho1);
    auto a2_frob = qs_precompute_gala_input_frob_all<QS,2>(st, k_frob, c_frob_rho2);

    auto a0 = a0_frob[0];
    auto a1 = a1_frob[0];
    auto a2 = a2_frob[0];

    auto a0_frobm = a0_frob[m];
    auto a1_frobm = a1_frob[m];
    auto a2_frobm = a2_frob[m];
    
    // 4 Prove sigma * a^{2^m} * a = 1.
    {
        auto c0p = a0 * a0_frobm;   // deg2
        auto c_inv0 = sigma0 * c0p + 1;
        st.add_constraint(c_inv0);
    }
    {
        auto c1p = a1 * a1_frobm;
        auto c_inv1 = sigma1 * c1p + 1;
        st.add_constraint(c_inv1);
    }
    {
        auto c2p = a2 * a2_frobm;
        auto c_inv2 = sigma2 * c2p + 1;
        st.add_constraint(c_inv2);
    }

    // 5 sigma bits
    std::array<quicksilver_gf2<QS,1>, m> sigma0_bits{}, sigma1_bits{}, sigma2_bits{};
    load_witness_bits(st, n,       sigma0_bits.data(), m);
    load_witness_bits(st, n + m,   sigma1_bits.data(), m);
    load_witness_bits(st, n + 2*m, sigma2_bits.data(), m);

    // 6 Apply L0/L1/L2 to S(a0), S(a1), S(a2).
    auto l0 = galas_linear_layer_from_a_frob_ID(st, 0, sigma0_bits.data(), a0_frob);
    auto l1 = galas_linear_layer_from_a_frob_ID(st, 1, sigma1_bits.data(), a1_frob);
    auto l2 = galas_linear_layer_from_a_frob_ID(st, 2, sigma2_bits.data(), a2_frob);

    // 7 XOR the three to get last S-box input z (your step 7)
    auto z = l0 + l1 + l2;

    // 8 last S-box output t is derived from public output and L3(k): t = y_out ⊕ L3(k)
    // pk->owf_output[0] is constant y_out
    std::array<uint8_t, secpar_to_bytes(S)> y_bytes{};
    std::memcpy(y_bytes.data(), &pk->owf_output[0], secpar_to_bytes(S));
    poly_secpar<S> Yout = poly_secpar<S>::load(y_bytes.data());

    auto L3k = faest::galas_linear_layer_L3k(st, k_frob);
    auto t = quicksilver_gfsecpar<QS,1>(Yout, &st) + L3k;

    // 9 final inverse proof: z * t == 1  (your step 9)
    auto c_final = z * t + 1;
    st.add_constraint(c_final);
}


template <typename P, bool verifier>
void owf_constraints(quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE>* state,
                     const public_key<P>* pk)
{
    using QS = quicksilver_state<P::secpar_v, verifier, P::OWF_CONSTS::QS_DEGREE>;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;
    constexpr auto O = P::owf_v;
    if constexpr (is_owf_with_shrunk_keyspace(P::owf_v))
    {
        shrunk_keyspace_constraint(state);
    }

    auto round_key_bits =
        state->template const_gf2_array<8 * OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1)>();
    auto round_key_bytes =
        state->template const_gfsecpar_array<OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1)>();
    auto round_key_bytes_sq =
        state->template const_gfsecpar_array<OC::OWF_BLOCK_SIZE*(OC::OWF_ROUNDS + 1)>();
    if constexpr (is_owf_with_aes_ecb(O))
    {
        key_sched_constraints<QS, O>(state, round_key_bits, round_key_bytes, round_key_bytes_sq);
        for (size_t i = 0; i < OC::OWF_BLOCKS; ++i)
        {
            enc_constraints<QS, O>(state, round_key_bits, round_key_bytes, round_key_bytes_sq, i,
                                   pk->owf_input[i], pk->owf_output[i]);
        }
    }
    else if constexpr (is_owf_with_aes_em(O))
    {
        load_fixed_round_key<QS, O>(state, round_key_bits, round_key_bytes, round_key_bytes_sq,
                                    &pk->fixed_key);
        enc_constraints<QS, O>(state, round_key_bits, round_key_bytes, round_key_bytes_sq, 0,
                               owf_block<S, O>::set_low32(0), pk->owf_output[0]);
    }
    else if constexpr (is_owf_galas_owf(O)) {
        galas_owf_constraints<QS>( *state, pk );
    }
    else
    {
        static_assert(false, "unsupported OWF");
    }
}


} // namespace faest

#endif
