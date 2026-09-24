#include "quicksilver.h"

#include <assert.h>

static_assert(POLY_VEC_LEN == 1, "");

static void quicksilver_init_hash_keys(quicksilver_state* state, const uint8_t* challenge)
{
	for (size_t i = 0; i < 2; ++i, challenge += SECURITY_PARAM / 8)
		state->hash_combination[i] = poly_secpar_load_dup(challenge);
	poly_secpar_vec hash_key_secpar = poly_secpar_load_dup(challenge);
	poly64_vec hash_key_64 = poly64_load_dup(challenge + SECURITY_PARAM / 8);

	hasher_gfsecpar_init_key(&state->key_secpar, hash_key_secpar);
	hasher_gfsecpar_64_init_key(&state->key_64, hash_key_64);
}

void quicksilver_init_prover(
	quicksilver_state* state, const uint8_t* witness, const block_secpar* macs,
	size_t num_constraints, const uint8_t* challenge)
{
	state->verifier = false;

	quicksilver_init_hash_keys(state, challenge);
	hasher_gfsecpar_init_state(&state->state_secpar_const, num_constraints);
	hasher_gfsecpar_init_state(&state->state_secpar_linear, num_constraints);
	hasher_gfsecpar_64_init_state(&state->state_64_const, num_constraints);
	hasher_gfsecpar_64_init_state(&state->state_64_linear, num_constraints);

	state->witness = witness;
	state->macs = macs;
}

void quicksilver_init_verifier(
	quicksilver_state* state, const block_secpar* macs, size_t num_constraints,
	block_secpar delta, const uint8_t* challenge)
{
	state->verifier = true;
	state->delta = poly_secpar_load_dup(&delta);
	state->deltaSq = poly_2secpar_reduce_secpar(poly_secpar_mul(state->delta, state->delta));

	quicksilver_init_hash_keys(state, challenge);
	hasher_gfsecpar_init_state(&state->state_secpar_const, num_constraints);
	hasher_gfsecpar_64_init_state(&state->state_64_const, num_constraints);

	state->macs = macs;
}

static void quicksilver_final(
	const quicksilver_state* state,
	const hasher_gfsecpar_state* state_secpar, const hasher_gfsecpar_64_state* state_64,
	poly_2secpar_vec mask, uint8_t* output)
{
	poly_secpar_vec hashes[2];
	hashes[0] = hasher_gfsecpar_final(state_secpar);
	hashes[1] = hasher_gfsecpar_64_final(state_64);

	poly_2secpar_vec sum = mask;
	for (size_t i = 0; i < 2; ++i)
		sum = poly_2secpar_add(sum, poly_secpar_mul(state->hash_combination[i], hashes[i]));

	poly_secpar_store1(output, poly_2secpar_reduce_secpar(sum));
}

static poly_2secpar_vec combine_mask_macs(const quicksilver_state* state, size_t witness_bits)
{
	poly_2secpar_vec accumulators[8 / POLY_VEC_LEN];

	for (int j = 0; j < 8 / POLY_VEC_LEN; ++j)
	{
		poly_secpar_vec x =
			poly_secpar_load(&state->macs[witness_bits + SECURITY_PARAM - 8 + POLY_VEC_LEN * j]);
		accumulators[j] = poly_2secpar_from_secpar(x);
	}

	for (int i = SECURITY_PARAM - 16; i >= 0; i -= 8)
	{
		for (int j = 0; j < 8 / POLY_VEC_LEN; ++j)
		{
			poly_secpar_vec x = poly_secpar_load(&state->macs[witness_bits + i + POLY_VEC_LEN * j]);
			accumulators[j] = poly_2secpar_add(poly_2secpar_shift_left_8(accumulators[j]),
			                                   poly_2secpar_from_secpar(x));
		}
	}

	poly_2secpar_vec total;
	memset(&total, 0, sizeof(total));
	for (int j = 8 / POLY_VEC_LEN - 1; j >= 0; --j)
	{
		for (int k = POLY_VEC_LEN - 1; k >= 0; --k)
		{
			total = poly_2secpar_shift_left_1(total);
			total = poly_2secpar_add(total, poly_2secpar_extract(accumulators[j], k));
		}
	}

	return total;
}

void quicksilver_prove(const quicksilver_state* restrict state, size_t witness_bits,
                       uint8_t* restrict proof, uint8_t* restrict check)
{
	assert(!state->verifier);
	assert(witness_bits % 8 == 0);

	poly_2secpar_vec value_mask =
		poly_2secpar_from_secpar(poly_secpar_load_dup(&state->witness[witness_bits / 8]));
	poly_2secpar_vec mac_mask = combine_mask_macs(state, witness_bits);

	quicksilver_final(state, &state->state_secpar_const, &state->state_64_const, mac_mask, check);
	quicksilver_final(state, &state->state_secpar_linear, &state->state_64_linear, value_mask, proof);
}

void quicksilver_verify(const quicksilver_state* restrict state, size_t witness_bits,
                        const uint8_t* restrict proof, uint8_t* restrict check)
{
	assert(state->verifier);

	poly_secpar_vec linear_term = poly_secpar_load_dup(proof);

	poly_2secpar_vec mac_mask = combine_mask_macs(state, witness_bits);
	mac_mask = poly_2secpar_add(mac_mask, poly_secpar_mul(linear_term, state->delta));
	quicksilver_final(state, &state->state_secpar_const, &state->state_64_const, mac_mask, check);
}

extern inline quicksilver_vec_gf2 quicksilver_get_witness_vec(const quicksilver_state* state, size_t index);
extern inline poly_secpar_vec quicksilver_get_delta(const quicksilver_state* state);
extern inline quicksilver_vec_gf2 quicksilver_add_gf2(const quicksilver_state* state, quicksilver_vec_gf2 x, quicksilver_vec_gf2 y);
extern inline quicksilver_vec_gfsecpar quicksilver_add_gfsecpar(const quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y);
extern inline quicksilver_vec_deg2 quicksilver_add_deg2(const quicksilver_state* state, quicksilver_vec_deg2 x, quicksilver_vec_deg2 y);
extern inline quicksilver_vec_gf2 quicksilver_zero_gf2();
extern inline quicksilver_vec_gfsecpar quicksilver_zero_gfsecpar();
extern inline quicksilver_vec_deg2 quicksilver_zero_deg2();
extern inline quicksilver_vec_gf2 quicksilver_one_gf2(const quicksilver_state* state);
extern inline quicksilver_vec_gfsecpar quicksilver_one_gfsecpar(const quicksilver_state* state);
extern inline quicksilver_vec_deg2 quicksilver_one_deg2(const quicksilver_state* state);
extern inline quicksilver_vec_gf2 quicksilver_const_gf2(const quicksilver_state* state, poly1_vec c);
extern inline quicksilver_vec_gfsecpar quicksilver_const_gfsecpar(const quicksilver_state* state, poly_secpar_vec c);
extern inline quicksilver_vec_deg2 quicksilver_const_deg2(const quicksilver_state* state, poly_secpar_vec c);
extern inline quicksilver_vec_deg2 quicksilver_const_deg2_gf2(const quicksilver_state* state, poly1_vec c);
extern inline quicksilver_vec_gf2 quicksilver_mul_const_gf2(const quicksilver_state* state, quicksilver_vec_gf2 x, poly1_vec c);
extern inline quicksilver_vec_gfsecpar quicksilver_mul_const(const quicksilver_state* state, quicksilver_vec_gfsecpar x, poly_secpar_vec c);
extern inline quicksilver_vec_deg2 quicksilver_mul_const_deg2_gf2(const quicksilver_state* state, quicksilver_vec_deg2 x, poly1_vec c);
extern inline quicksilver_vec_deg2 quicksilver_mul_const_deg2(const quicksilver_state* state, quicksilver_vec_deg2 x, poly_secpar_vec c);
extern inline quicksilver_vec_deg2 quicksilver_mul(const quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y);
extern inline quicksilver_vec_gfsecpar quicksilver_combine_1_bit(const quicksilver_state* state, const quicksilver_vec_gf2 qs_bit);
extern inline quicksilver_vec_gfsecpar quicksilver_combine_8_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits);
extern inline quicksilver_vec_gfsecpar quicksilver_combine_16_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits);
extern inline quicksilver_vec_gfsecpar quicksilver_combine_secpar_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits);
extern inline quicksilver_vec_gfsecpar quicksilver_const_8_bits(const quicksilver_state* state, const void* s);
extern inline quicksilver_vec_gfsecpar quicksilver_const_secpar_bits(const quicksilver_state* state, const void* s);
extern inline quicksilver_vec_gfsecpar quicksilver_get_witness_8_bits(const quicksilver_state* state, size_t bit_index);
extern inline void quicksilver_constraint(quicksilver_state* state, quicksilver_vec_deg2 x);
extern inline void quicksilver_inverse_constraint(quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y);
