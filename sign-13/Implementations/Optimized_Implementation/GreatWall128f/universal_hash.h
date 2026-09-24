#ifndef UNIVERSAL_HASH_H
#define UNIVERSAL_HASH_H

#include <assert.h>
#include <string.h>

#include "polynomials.h"

#define HASHER_GFSECPAR_KEY_POWS 2
#define HASHER_GF64_KEY_POWS 2

typedef struct
{
	poly_secpar_vec key_pows[HASHER_GFSECPAR_KEY_POWS];
} hasher_gfsecpar_key;

typedef struct
{
	poly64_vec key;
} hasher_gfsecpar_64_key;

typedef struct
{
	poly64_vec key_pows[HASHER_GF64_KEY_POWS];
	poly64_vec key_pow_times_a64;
} hasher_gf64_key;

typedef struct
{
	poly_2secpar_vec state;
	int pow;
} hasher_gfsecpar_state;

typedef struct
{
	poly_secpar_vec state;
} hasher_gfsecpar_64_state;

typedef struct
{
	poly128_vec state;
	int pow;
} hasher_gf64_state;

inline void hasher_gfsecpar_init_key(hasher_gfsecpar_key* hash_key, poly_secpar_vec key)
{
	hash_key->key_pows[0] = key;
	poly_secpar_vec key_pow = key;
	for (size_t i = 1; i < HASHER_GFSECPAR_KEY_POWS; ++i)
	{
		key_pow = poly_2secpar_reduce_secpar(poly_secpar_mul(key_pow, key));
		hash_key->key_pows[i] = key_pow;
	}
}

inline void hasher_gfsecpar_init_state(hasher_gfsecpar_state* state, size_t num_coefficients)
{
	memset(&state->state, 0, sizeof(state->state));
	state->pow = (num_coefficients + HASHER_GFSECPAR_KEY_POWS - 1) % HASHER_GFSECPAR_KEY_POWS;
}

inline void hasher_gfsecpar_update(const hasher_gfsecpar_key* key, hasher_gfsecpar_state* state, poly_secpar_vec input)
{
	if (state->pow == -1)
	{
		state->state = poly_secpar_mul(key->key_pows[HASHER_GFSECPAR_KEY_POWS - 1], poly_2secpar_reduce_secpar(state->state));
		state->pow = HASHER_GFSECPAR_KEY_POWS - 1;
	}

	poly_2secpar_vec summand;
	if (state->pow > 0)
		summand = poly_secpar_mul(key->key_pows[state->pow - 1], input);
	else
		summand = poly_2secpar_from_secpar(input);
	state->state = poly_2secpar_add(state->state, summand);
	--state->pow;
}

inline poly_secpar_vec hasher_gfsecpar_final(const hasher_gfsecpar_state* state)
{
	assert(state->pow == -1);
	return poly_2secpar_reduce_secpar(state->state);
}

inline void hasher_gfsecpar_64_init_key(hasher_gfsecpar_64_key* hash_key, poly64_vec key)
{
	hash_key->key = key;
}

inline void hasher_gfsecpar_64_init_state(hasher_gfsecpar_64_state* state, size_t num_coefficients)
{
    (void) num_coefficients;
	memset(&state->state, 0, sizeof(state->state));
}

inline void hasher_gfsecpar_64_update(const hasher_gfsecpar_64_key* key, hasher_gfsecpar_64_state* state, poly_secpar_vec input)
{
	state->state = poly_secpar_plus_64_reduce_secpar(poly64xsecpar_mul(key->key, state->state));
    state->state = poly_secpar_add(state->state, input);
}

inline poly_secpar_vec hasher_gfsecpar_64_final(const hasher_gfsecpar_64_state* state)
{
	return state->state;
}

inline poly_secpar_vec gfsecpar_combine_hashes(poly_secpar_vec key_exp, poly_secpar_vec hash)
{
	poly_secpar_vec output = poly_secpar_extract(hash, 0);
	for (size_t i = 1; i < POLY_VEC_LEN; ++i)
	{
		output = poly_2secpar_reduce_secpar(poly_secpar_mul(key_exp, output));
		output = poly_secpar_add(output, poly_secpar_extract(hash, i));
	}
	return output;
}

inline void hasher_gf64_init_key(hasher_gf64_key* hash_key, poly64_vec key)
{
	hash_key->key_pows[0] = key;
	poly64_vec key_pow = key;
	for (size_t i = 1; i < HASHER_GF64_KEY_POWS; ++i)
	{
		key_pow = poly128_reduce64(poly64_mul(key_pow, key));
		hash_key->key_pows[i] = key_pow;
	}

	hash_key->key_pow_times_a64 = poly64_mul_a64_reduce64(hash_key->key_pows[HASHER_GF64_KEY_POWS - 1]);
}

inline void hasher_gf64_init_state(hasher_gf64_state* state, size_t num_coefficients)
{
	memset(&state->state, 0, sizeof(state->state));
	state->pow = (num_coefficients + HASHER_GF64_KEY_POWS - 1) % HASHER_GF64_KEY_POWS;
}

inline void hasher_gf64_update(const hasher_gf64_key* key, hasher_gf64_state* state, poly64_vec input)
{
	if (state->pow == -1)
	{
		poly128_vec l = clmul_block_clmul_ll(key->key_pows[HASHER_GF64_KEY_POWS - 1], state->state);
		poly128_vec h = clmul_block_clmul_lh(key->key_pow_times_a64, state->state);
		state->state = poly128_add(l, h);
		state->pow = HASHER_GF64_KEY_POWS - 1;
	}

	poly128_vec summand;
	if (state->pow > 0)
		summand = poly64_mul(key->key_pows[state->pow - 1], input);
	else
		summand = poly128_from_64(input);
	state->state = poly128_add(state->state, summand);
	--state->pow;
}

inline poly64_vec hasher_gf64_final(const hasher_gf64_state* state)
{
	assert(state->pow == -1);
	return poly128_reduce64(state->state);
}

inline poly64_vec gf64_combine_hashes(poly64_vec key_exp, poly64_vec hash)
{
	poly64_vec output = poly64_extract(hash, 0);
	for (size_t i = 1; i < POLY_VEC_LEN; ++i)
	{
		output = poly128_reduce64(poly64_mul(key_exp, output));
		output = poly64_add(output, poly64_extract(hash, i));
	}
	return output;
}

#endif
