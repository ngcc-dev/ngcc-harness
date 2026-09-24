#ifndef UNIVERSAL_HASH_HPP
#define UNIVERSAL_HASH_HPP

#include <cassert>
#include <cstring>

#include "polynomials.hpp"

namespace faest
{

// Number of powers of the hash key to precompute
// For hasher_gfsecpar_64, there doesn't seem to be a good way to do more than 1.
constexpr std::size_t HASHER_GFSECPAR_KEY_POWS = 2;
constexpr std::size_t HASHER_GF64_KEY_POWS = 2;

template <secpar S> struct hasher_gfsecpar_key
{
    poly_secpar<S> key_pows[HASHER_GFSECPAR_KEY_POWS];

    inline void init(poly_secpar<S> key)
    {
        this->key_pows[0] = key;
        poly_secpar<S> key_pow = key;
        for (size_t i = 1; i < HASHER_GFSECPAR_KEY_POWS; ++i)
        {
            key_pow = (key_pow * key).template reduce_to<secpar_to_bits(S)>();
            this->key_pows[i] = key_pow;
        }
    }
};

struct hasher_gfsecpar_64_key
{
    poly64 key;

    inline void init(poly64 key) { this->key = key; }
};

struct hasher_gf64_key
{
    poly64 key_pows[HASHER_GF64_KEY_POWS];
    poly64 key_pow_times_a64;

    inline void init(poly64 key)
    {
        this->key_pows[0] = key;
        poly64 key_pow = key;
        for (size_t i = 1; i < HASHER_GF64_KEY_POWS; ++i)
        {
            key_pow = (key_pow * key).reduce_to<64>();
            this->key_pows[i] = key_pow;
        }

        this->key_pow_times_a64 = this->key_pows[HASHER_GF64_KEY_POWS - 1].mul_a64_reduce64();
    }
};

template <secpar S> struct hasher_gfsecpar_state
{
    poly_2secpar<S> state;
    int pow;

    inline void init(size_t num_coefficients)
    {
        memset(&this->state, 0, sizeof(this->state));
        this->pow = (num_coefficients + HASHER_GFSECPAR_KEY_POWS - 1) % HASHER_GFSECPAR_KEY_POWS;
    }

    // Update a vector of hashers on a vector of polynomials.
    // - Needs to get called `num_coefficients` (parameter to `init`) times before
    // calling `hasher_secpar_final`.
    inline void update(const hasher_gfsecpar_key<S>* key, poly_secpar<S> input)
    {
        if (this->pow == -1)
        {
            this->state = key->key_pows[HASHER_GFSECPAR_KEY_POWS - 1] *
                          this->state.template reduce_to<secpar_to_bits(S)>();
            this->pow = HASHER_GFSECPAR_KEY_POWS - 1;
        }

        poly_2secpar<S> summand;
        if (this->pow > 0)
            summand = key->key_pows[this->pow - 1] * input;
        else
            summand = poly_2secpar<S>::from(input);
        this->state = this->state + summand;
        --this->pow;
    }

    inline poly_secpar<S> finalize() const
    {
        FAEST_ASSERT(this->pow == -1);
        return this->state.template reduce_to<secpar_to_bits(S)>();
    }
};

template <secpar S> struct hasher_gfsecpar_64_state
{
    poly_secpar<S> state;

    inline void init(size_t num_coefficients)
    {
        (void)num_coefficients;
        memset(&this->state, 0, sizeof(this->state));
    }

    // Update a vector of hashers on a vector of polynomials.
    inline void update(const hasher_gfsecpar_64_key* key, poly_secpar<S> input)
    {
        this->state = (key->key * this->state).template reduce_to<secpar_to_bits(S)>();
        this->state = this->state + input;
    }

    inline poly_secpar<S> finalize() const { return this->state; }
};

struct hasher_gf64_state
{
    poly128 state;
    int pow;

    inline void init(size_t num_coefficients)
    {
        memset(&this->state, 0, sizeof(this->state));
        this->pow = (num_coefficients + HASHER_GF64_KEY_POWS - 1) % HASHER_GF64_KEY_POWS;
    }

    // Update a vector of hashers on a vector of polynomials.
    inline void update(const hasher_gf64_key* key, poly64 input)
    {
        if (this->pow == -1)
        {
            poly128 l = {clmul_block::clmul_ll(key->key_pows[HASHER_GF64_KEY_POWS - 1].data,
                                               this->state.data)};
            poly128 h = {clmul_block::clmul_lh(key->key_pow_times_a64.data, this->state.data)};
            this->state = l + h;
            this->pow = HASHER_GF64_KEY_POWS - 1;
        }

        poly128 summand;
        if (this->pow > 0)
            summand = key->key_pows[this->pow - 1] * input;
        else
            summand = poly128::from(input);
        this->state = this->state + summand;
        --this->pow;
    }

    inline poly64 finalize() const
    {
        FAEST_ASSERT(this->pow == -1);
        return this->state.reduce_to<64>();
    }
};

// Input key and output hash are in index 0 of their respective polynomial vectors. key_exp should
// be set to poly_secpar_exp(key, num_coefficients), where num_coefficients is the number of
// polynomial coefficients the vector elements are separated by.
template <secpar S>
inline poly_secpar<S> gfsecpar_combine_hashes(poly_secpar<S> key_exp, poly_secpar<S> hash)
{
    (void)key_exp;
    return hash.extract(0);
}

inline poly64 gf64_combine_hashes(poly64 key_exp, poly64 hash)
{
    (void)key_exp;
    poly64 output = hash.extract(0);
    return output;
}

} // namespace faest

#endif
