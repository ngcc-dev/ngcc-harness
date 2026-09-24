#ifndef FAEST_KEYS_INC
#define FAEST_KEYS_INC

#include "aes.hpp"
#include "constants.hpp"
#include "faest_keys.hpp"
#include "gfsmall.hpp"
#include "parameters.hpp"
#include "galas_qs_gadgets.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace faest
{

template <typename P> bool faest_unpack_secret_key(secret_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto sk_offset = FAEST_IV_BYTES<P>;
    if constexpr (is_owf_with_aes_ecb(P::owf_v) && is_owf_with_ctr_input(P::owf_v) &&
                  P::secpar_bits > 128)
    {
        // Load the first input block.
        memcpy(&unpacked->pk.owf_input[0], packed, sizeof(unpacked->pk.owf_input[0]));
        static_assert(sizeof(unpacked->pk.owf_input[0]) == sk_offset);
        // Compute the second block as the first XOR 1.
        unpacked->pk.owf_input[1] = unpacked->pk.owf_input[0];
        reinterpret_cast<uint8_t*>(&unpacked->pk.owf_input[1])[0] ^= 0x01;
    }
    else
    {
        // Load all input blocks.
        memcpy(&unpacked->pk.owf_input, packed, sizeof(unpacked->pk.owf_input));
        static_assert(sizeof(unpacked->pk.owf_input) == sk_offset);
    }
    // Load the secret key.
    memcpy(&unpacked->sk, packed + sk_offset, sizeof(unpacked->sk));

    if constexpr (is_owf_with_aes_ecb(P::owf_v))
        // Compute the key schedule of the secret AES key.
        aes_keygen<P::secpar_v>(&unpacked->round_keys, unpacked->sk);
    else if constexpr (is_owf_with_aes_em(P::owf_v))
        // Compute the key schedule of the public fixed key.
        rijndael_keygen<P::secpar_v>(&unpacked->pk.fixed_key, unpacked->pk.owf_input[0]);
    else if constexpr (is_owf_galas_owf(P::owf_v))
    {
        // Gala OWF: no AES key schedule needed here (unless you use AES inside Gala)
    }
    else
        static_assert(false, "unsupported OWF");

    return faest_compute_witness(unpacked);
}

template <typename P> void faest_pack_public_key(uint8_t* packed, const public_key<P>* unpacked)
{
    constexpr auto owf_output_offset = FAEST_IV_BYTES<P>;
    if constexpr (is_owf_with_aes_ecb(P::owf_v) && is_owf_with_ctr_input(P::owf_v) &&
                  P::secpar_bits > 128)
    {
        // Store only the first input block.
        memcpy(packed, &unpacked->owf_input[0], sizeof(unpacked->owf_input[0]));
        static_assert(sizeof(unpacked->owf_input[0]) == owf_output_offset);
    }
    else
    {
        // Store only all input blocks.
        memcpy(packed, &unpacked->owf_input, sizeof(unpacked->owf_input));
        static_assert(sizeof(unpacked->owf_input) == owf_output_offset);
    }
    // Store the OWF output block(s).
    memcpy(packed + owf_output_offset, &unpacked->owf_output, sizeof(unpacked->owf_output));
}

template <typename P> void faest_unpack_public_key(public_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto owf_output_offset = FAEST_IV_BYTES<P>;
    // Copy the OWF input.
    if constexpr (is_owf_with_aes_ecb(P::owf_v) && is_owf_with_ctr_input(P::owf_v) &&
                  P::secpar_bits > 128)
    {
        // Load the first block.
        memcpy(&unpacked->owf_input[0], packed, sizeof(unpacked->owf_input[0]));
        static_assert(sizeof(unpacked->owf_input[0]) == owf_output_offset);
        // Compute the second block as the first XOR 1.
        unpacked->owf_input[1] = unpacked->owf_input[0];
        reinterpret_cast<uint8_t*>(&unpacked->owf_input[1])[0] ^= 0x01;
    }
    else
    {
        // Load all input blocks from the packed representation.
        memcpy(&unpacked->owf_input, packed, sizeof(unpacked->owf_input));
        static_assert(sizeof(unpacked->owf_input) == owf_output_offset);
    }
    // Load the OWF output block(s).
    memcpy(&unpacked->owf_output, packed + owf_output_offset, sizeof(unpacked->owf_output));

    if constexpr (is_owf_with_aes_em(P::owf_v))
        // Compute the key schedule of the public fixed key.
        rijndael_keygen<P::secpar_v>(&unpacked->fixed_key, unpacked->owf_input[0]);
}

template <typename P> bool faest_compute_witness(secret_key<P>* sk)
{
    using OC = P::OWF_CONSTS;
    using owf_block = OC::block_t;

    uint8_t* w_ptr = (uint8_t*)&sk->witness;

    memcpy(w_ptr, &sk->sk, sizeof(sk->sk));

    if constexpr (is_owf_with_shrunk_keyspace(P::owf_v))
    {
        // Make sure that the first two bits of the key are not both 1.
        if ((w_ptr[0] & 1) && (w_ptr[0] & 2))
        {
            return false;
        }
    }
    if constexpr (is_owf_galas_owf(P::owf_v))
    {
        if ((w_ptr[0] & 0x03) != 0x03)
            return false;
    }

    w_ptr += sizeof(sk->sk);

    if constexpr (is_owf_galas_owf(P::owf_v))
    {
        // witness layout:
        // [0 .. secpar_bytes-1]               = k (already copied above)
        // [secpar_bytes .. secpar_bytes+3*(secpar_bytes/2)-1] = u0||u1||u2, each in F_{2^{λ/2}}
        //
        // where u_i = a_i^{-2^{λ/2}-1}   (your "x^{-2^{m}-1}" in subfield, with m=λ/2)

        constexpr size_t half_bytes = P::secpar_bytes / 2;
        static_assert(P::secpar_bytes % 2 == 0, "secpar_bytes must be even for Galas witness");

        // 1 derive a0,a1,a2:
        // a0 := x ⊕ k ⊕ c0
        // a1 := k ⊕ c1
        // a2 := k ⊕ c2
        //
        // NOTE: you said "generator = 2" for packing bits of k into the commitment polynomial;
        // that affects how you *commit* later, not the raw witness bytes here.
        //
        // We assume:
        //   - pk.owf_input[0] holds x (λ bits) for Galas
        //   - sk->sk holds k (λ bits)

        // New Galas structure:
        // rho0=x+k+c0, rho1=k+c1, rho2=k+c2;
        // a0=M0(rho0), a1=M1(rho1), a2=M2(rho2).
        std::array<uint8_t, P::secpar_bytes> x_bytes{};
        std::array<uint8_t, P::secpar_bytes> k_bytes{};
        memcpy(x_bytes.data(), &sk->pk.owf_input[0], P::secpar_bytes);
        memcpy(k_bytes.data(), &sk->sk, P::secpar_bytes);
        if (bytes_is_zero<P::secpar_v>(x_bytes.data()))
            return false;

        std::array<uint8_t, P::secpar_bytes> a0{}, a1{}, a2{};
        gala_wide_inputs<P::secpar_v>(a0, a1, a2, x_bytes.data(), k_bytes.data());

        if (!gala_eval_from_wide_inputs<P::secpar_v>(
                reinterpret_cast<uint8_t*>(&sk->pk.owf_output[0]), k_bytes.data(),
                a0, a1, a2, true))
        {
            return false;
        }

        // 2 compute u_i = a_i^{-2^{λ/2}-1} in F_{2^{λ/2}}
        //
        // You already fixed irreducible polynomials (P128/P192/P256).
        // Implement these helpers in gfsmall.hpp/cpp (or a new gala_field.hpp):
        //   - gala_invnorm<S>(out_half, in_full)
        //
        // out_half is λ/2 bits (half_bytes), in_full is λ bits (secpar_bytes).
        //
        // IMPORTANT: this is exactly the value you later commit as σ (in subfield).
        // Each u_i is the half-size invnorm witness for the corresponding a_i.
        std::array<uint8_t, half_bytes> u0{}, u1{}, u2{};

        gala_invnorm<P::secpar_v>(u0.data(), a0.data());
        gala_invnorm<P::secpar_v>(u1.data(), a1.data());
        gala_invnorm<P::secpar_v>(u2.data(), a2.data());

        // 3 write u0||u1||u2 into witness
        memcpy(w_ptr, u0.data(), half_bytes); w_ptr += half_bytes;
        memcpy(w_ptr, u1.data(), half_bytes); w_ptr += half_bytes;
        memcpy(w_ptr, u2.data(), half_bytes); w_ptr += half_bytes;

        // 4 pk.owf_output[0] was computed above by gala_eval_from_wide_inputs.
        // 4 compute OWF output y = Gala(x,k) and store to pk.owf_output[0]
        //
        // Implement gala_eval<S>(out, x, k) somewhere (e.g., gala.hpp).
        // It must follow:
        // b_i = S(a_i), z = L0(b0) ⊕ L1(b1) ⊕ L2(b2), t = S(z), y = t ⊕ L3(k)
        // 5 finalize witness padding exactly like other OWFs
        FAEST_ASSERT((size_t)(w_ptr - (uint8_t*)&sk->witness) == P::OWF_CONSTS::WITNESS_BITS / 8);
        memset(w_ptr, 0, sizeof(sk->witness) - P::OWF_CONSTS::WITNESS_BITS / 8);
        return true;
    }


    if constexpr (is_owf_with_aes_ecb(P::owf_v))
    {
        // Extract witness for key schedule.
        for (size_t i = P::secpar_bytes; i < OC::OWF_BLOCK_SIZE * (OC::OWF_ROUNDS + 1);
             i += OC::OWF_KEY_SCHEDULE_PERIOD, w_ptr += 4)
        {
            uint32_t prev_word, word;
            memcpy(&prev_word, ((uint8_t*)&sk->round_keys.keys[0]) + i - P::secpar_bytes, 4);
            memcpy(&word, ((uint8_t*)&sk->round_keys.keys[0]) + i, 4);
            memcpy(w_ptr, &word, 4);

            uint32_t sbox_output = word ^ prev_word;
            if (P::secpar_v != secpar::s256 || i % P::secpar_bytes == 0)
                sbox_output ^= aes_round_constants[i / P::secpar_bytes - 1];

            if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
            {
                // Check if there is an S-box with a zero
                // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
                sbox_output ^= 0x63636363; // AES SBox maps 0 to 0x63.
                if ((sbox_output - 0x01010101) & ~sbox_output & 0x80808080)
                    return false;
            }
        }
    }

    if constexpr (is_owf_with_aes_ecb(P::owf_v))
    {
        for (uint32_t i = 0; i < OC::OWF_BLOCKS; ++i)
            sk->pk.owf_output[i] = sk->round_keys.keys[0] ^ sk->pk.owf_input[i];
    }
    else if constexpr (is_owf_with_aes_em(P::owf_v))
    {
        static_assert(OC::OWF_BLOCKS == 1, "");
        sk->pk.owf_output[0] = sk->pk.fixed_key.keys[0] ^ sk->sk;
    }

    constexpr size_t witness_bytes_per_block = OC::OWF_ENC_WITNESS_BITS_PER_BLOCK / 8;

    // NB: Compared to the spec
    // - the order of the loops is switched
    // - the rounds are indexed by [1, R] instead of [0, R-2]
    for (size_t round_j = 1; round_j <= OC::OWF_ROUNDS; ++round_j)
    {
        for (size_t owf_block_i = 0; owf_block_i < OC::OWF_BLOCKS; ++owf_block_i)
        {
            if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
            {
                // The block is about to go into the SBox, so check for zeros.
                if (sk->pk.owf_output[owf_block_i].any_zeros())
                    return false;
            }

            if constexpr (is_owf_with_norm_proof(P::owf_v))
            {
                if (round_j < OC::OWF_ROUNDS && (round_j & 1) == 1)
                {
                    // "even" case in the spec
                    // compute the inverse norms and compress
                    std::array<uint8_t, OC::OWF_BLOCK_SIZE / 2> invnorms;
                    gf256_gf16_batch_invnorm<OC::OWF_BLOCK_SIZE>(
                        invnorms.data(),
                        reinterpret_cast<uint8_t*>(&sk->pk.owf_output[owf_block_i]));
                    // store the inverse norms into the witness
                    memcpy(w_ptr + owf_block_i * witness_bytes_per_block, invnorms.data(),
                           OC::OWF_BLOCK_SIZE / 2);
                }
            }

            owf_block after_sbox;
            if constexpr (is_owf_with_aes_ecb(P::owf_v))
            {
                aes_round_function<P::secpar_v>(&sk->round_keys, &sk->pk.owf_output[owf_block_i],
                                                &after_sbox, round_j);
            }
            else if constexpr (is_owf_with_aes_em(P::owf_v))
            {
                if constexpr (P::secpar_v == secpar::s128)
                    aes_round_function<secpar::s128>(
                        &sk->pk.fixed_key, &sk->pk.owf_output[owf_block_i], &after_sbox, round_j);
                else if constexpr (P::secpar_v == secpar::s192)
                    rijndael192_round_function(&sk->pk.fixed_key, &sk->pk.owf_output[owf_block_i],
                                               &after_sbox, round_j);
                else if constexpr (P::secpar_v == secpar::s256)
                    rijndael256_round_function(&sk->pk.fixed_key, &sk->pk.owf_output[owf_block_i],
                                               &after_sbox, round_j);
            }
            if constexpr (is_owf_with_norm_proof(P::owf_v))
            {
                if (round_j < OC::OWF_ROUNDS && ((round_j & 1) == 0))
                {
                    // "odd" case of the spec -> full-sized state
                    memcpy(w_ptr + owf_block_i * witness_bytes_per_block, &after_sbox,
                           sizeof(owf_block));
                }
            }
            else
            {
                if (round_j < OC::OWF_ROUNDS)
                    memcpy(w_ptr + owf_block_i * witness_bytes_per_block, &after_sbox,
                           sizeof(owf_block));
            }
        }

        if (round_j < OC::OWF_ROUNDS)
        {
            if constexpr (is_owf_with_norm_proof(P::owf_v))
            {
                if ((round_j & 1) == 0)
                {
                    // "odd" case of the spec -> full-sized state
                    w_ptr += sizeof(owf_block);
                }
                else
                {
                    // "even" case of the spec -> half-sized state
                    w_ptr += sizeof(owf_block) / 2;
                }
            }
            else
            {
                w_ptr += sizeof(owf_block);
            }
        }
    }

    w_ptr += (OC::OWF_BLOCKS - 1) * OC::OWF_ENC_WITNESS_BITS_PER_BLOCK / 8;
    FAEST_ASSERT(w_ptr - (uint8_t*)&sk->witness == OC::WITNESS_BITS / 8);
    memset(w_ptr, 0, sizeof(sk->witness) - OC::WITNESS_BITS / 8);

    if constexpr (is_owf_with_aes_em(P::owf_v))
    {
        for (uint32_t i = 0; i < OC::OWF_BLOCKS; ++i)
            sk->pk.owf_output[i] = sk->pk.owf_output[i] ^ sk->sk;
    }

    return true;
}

template <typename P>
bool faest_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed, secret_key<P>* sk)
{
    if (!faest_unpack_secret_key(sk, sk_packed))
        return false;

    faest_pack_public_key(pk_packed, &sk->pk);
    return true;
}

template <typename P> bool faest_seckey(const uint8_t* sk_packed)
{
    secret_key<P> sk;
    if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
        return faest_unpack_secret_key<P>(&sk, sk_packed);
    else if constexpr (is_owf_with_shrunk_keyspace(P::owf_v))
        // check that the first two bits of the secret key are not both 1
        return (0x3 & sk_packed[FAEST_IV_BYTES<P>]) != 0x3;
    else
        return true;
}

template <typename P> bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed)
{
    secret_key<P> sk;
    if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
    {
        return faest_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk);
    }
    else
    {
        if (!faest_seckey<P>(sk_packed))
            return false;

        using OC = P::OWF_CONSTS;
        using owf_block = OC::block_t;
        constexpr auto S = P::secpar_v;
        constexpr auto sk_offset = FAEST_IV_BYTES<P>;
        constexpr auto owf_output_offset = FAEST_IV_BYTES<P>;

        std::array<owf_block, OC::OWF_BLOCKS> input;
        std::array<owf_block, OC::OWF_BLOCKS> output;
        memcpy(input.data(), sk_packed, sk_offset);

        if constexpr (is_owf_with_ctr_input(P::owf_v) && OC::OWF_BLOCKS == 2)
        {
            input[1] = input[0];
            reinterpret_cast<uint8_t*>(&input[1])[0] ^= 0x01;
        }

        if constexpr (is_owf_with_aes_em(P::owf_v))
        {
            block_secpar<S> em_key;
            block_secpar<S> fixed_key;
            memcpy(&em_key, sk_packed + sk_offset, P::secpar_bytes);
            memcpy(&fixed_key, sk_packed, P::secpar_bytes);
            rijndael_round_keys<S> round_keys;
            const auto zero = block_secpar<S>::set_zero();
            uint32_t tweak = 0;
            uint32_t counter = 0;
            rijndael_keygen<S>(&round_keys, fixed_key);
            rijndael_fixed_key_ctr<S, 1, 1>(&round_keys, &em_key, zero, &tweak, &counter, output.data());
        }
        else if constexpr (is_owf_galas_owf(P::owf_v))
        {
            // input[0] is x, aes_key is k in your old code naming
            std::array<uint8_t, P::secpar_bytes> x_bytes{}, k_bytes{}, y_bytes{};
            memcpy(x_bytes.data(), input.data(), P::secpar_bytes);
            memcpy(k_bytes.data(), sk_packed + sk_offset, P::secpar_bytes);

            gala_eval<S>(y_bytes.data(), x_bytes.data(), k_bytes.data());

            // write to output[0]
            memcpy(output.data(), y_bytes.data(), P::secpar_bytes);
        }
        else
        {
            block_secpar<S> aes_key;
            memcpy(&aes_key, sk_packed + sk_offset, P::secpar_bytes);
            aes_round_keys<S> round_keys;
            output = input;
            aes_keygen_ecb<S, 1, OC::OWF_BLOCKS>(&aes_key, &round_keys, output.data());
        }
        memcpy(pk_packed, sk_packed, owf_output_offset);
        memcpy(pk_packed + owf_output_offset, output.data(), output.size() * sizeof(output[0]));

        return true;
    }
}

} // namespace faest

#endif
