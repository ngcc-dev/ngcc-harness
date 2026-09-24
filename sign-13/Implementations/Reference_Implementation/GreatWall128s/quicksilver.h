#ifndef QUICKSILVER_H
#define QUICKSILVER_H

#include "polynomials.h"
#include "universal_hash.h"
#include "util.h"

#define QUICKSILVER_CHALLENGE_BYTES ((3 * SECURITY_PARAM + 64) / 8)
#define QUICKSILVER_PROOF_BYTES (SECURITY_PARAM / 8)
#define QUICKSILVER_CHECK_BYTES (SECURITY_PARAM / 8)

typedef struct
{
	poly_secpar_vec mac;
	poly1_vec value;
} quicksilver_vec_gf2;

typedef struct {
    quicksilver_vec_gf2 bit[137];
} qs_gf137_bits;

typedef struct {
    quicksilver_vec_gf2 bit[197];
} qs_gf197_bits;

typedef struct {
    quicksilver_vec_gf2 bit[263];
} qs_gf263_bits;

typedef struct {
    quicksilver_vec_gf2 bit[521];
} qs_gf521_bits;

typedef struct
{
	poly_secpar_vec mac;
	poly_secpar_vec value;
} quicksilver_vec_gfsecpar;

typedef struct
{
	poly_2secpar_vec mac0;
	poly_2secpar_vec mac1;
} quicksilver_vec_deg2;

typedef struct
{
	bool verifier;
	poly_secpar_vec delta;
	poly_secpar_vec deltaSq;

	hasher_gfsecpar_key key_secpar;
	hasher_gfsecpar_state state_secpar_const;
	hasher_gfsecpar_state state_secpar_linear;

	hasher_gfsecpar_64_key key_64;
	hasher_gfsecpar_64_state state_64_const;
	hasher_gfsecpar_64_state state_64_linear;

	poly_secpar_vec hash_combination[2];

	const uint8_t* witness;
	const block_secpar* macs;
} quicksilver_state;

void quicksilver_init_prover(
	quicksilver_state* state, const uint8_t* witness, const block_secpar* macs,
	size_t num_constraints, const uint8_t* challenge);

void quicksilver_init_verifier(
	quicksilver_state* state, const block_secpar* macs, size_t num_constraints,
	block_secpar delta, const uint8_t* challenge);

inline quicksilver_vec_gf2 quicksilver_get_witness_vec(const quicksilver_state* state, size_t index)
{
	quicksilver_vec_gf2 out;
	if (!state->verifier)
	{
		uint16_t tmp;

		memcpy(&tmp, &state->witness[index / 8], sizeof(tmp));
		out.value = poly1_load(tmp, index % 8);
	}
	out.mac = poly_secpar_load(&state->macs[index]);
	return out;
}

inline poly_secpar_vec quicksilver_get_delta(const quicksilver_state* state) {
    assert(state->verifier);
    return state->delta;
}

inline quicksilver_vec_gf2 quicksilver_add_gf2(const quicksilver_state* state, quicksilver_vec_gf2 x, quicksilver_vec_gf2 y)
{
	quicksilver_vec_gf2 out;
	if (!state->verifier)
		out.value = x.value ^ y.value;
	out.mac = poly_secpar_add(x.mac, y.mac);
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_add_gfsecpar(const quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y)
{
	quicksilver_vec_gfsecpar out;
	if (!state->verifier)
		out.value = poly_secpar_add(x.value, y.value);
	out.mac = poly_secpar_add(x.mac, y.mac);
	return out;
}

inline quicksilver_vec_deg2 quicksilver_add_deg2(const quicksilver_state* state, quicksilver_vec_deg2 x, quicksilver_vec_deg2 y)
{
	quicksilver_vec_deg2 out;
	out.mac0 = poly_2secpar_add(x.mac0, y.mac0);
	out.mac1 = poly_2secpar_set_zero();
	if (!state->verifier)
	{
		out.mac1 = poly_2secpar_add(x.mac1, y.mac1);
	}
	return out;
}

inline quicksilver_vec_gf2 quicksilver_zero_gf2()
{
	quicksilver_vec_gf2 out;
	out.value = 0;
	out.mac = poly_secpar_set_zero();
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_zero_gfsecpar()
{
	quicksilver_vec_gfsecpar out;
	out.value = poly_secpar_set_zero();
	out.mac = poly_secpar_set_zero();
	return out;
}

inline quicksilver_vec_deg2 quicksilver_zero_deg2()
{
	quicksilver_vec_deg2 out;
	out.mac0 = poly_2secpar_set_zero();
	out.mac1 = poly_2secpar_set_zero();
	return out;
}

inline quicksilver_vec_gf2 quicksilver_one_gf2(const quicksilver_state* state)
{
	quicksilver_vec_gf2 out;
	if (state->verifier)
		out.mac = state->delta;
	else
	{
		out.mac = poly_secpar_set_zero();
		out.value = poly1_set_all(0xff);
	}
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_one_gfsecpar(const quicksilver_state* state)
{
	quicksilver_vec_gfsecpar out;
	if (state->verifier)
		out.mac = state->delta;
	else
	{
		out.mac = poly_secpar_set_zero();
		out.value = poly_secpar_set_low32(1);
	}
	return out;
}

inline quicksilver_vec_deg2 quicksilver_one_deg2(const quicksilver_state* state)
{
	quicksilver_vec_deg2 out;
	out.mac1 = poly_2secpar_set_zero();
	if (state->verifier)
		out.mac0 = poly_2secpar_from_secpar(state->deltaSq);
	else
	{
		out.mac0 = poly_2secpar_set_zero();
		out.mac1 = poly_2secpar_from_secpar(poly_secpar_set_low32(1));
	}
	return out;
}

inline quicksilver_vec_gf2 quicksilver_const_gf2(const quicksilver_state* state, poly1_vec c)
{
	quicksilver_vec_gf2 out;
	if (state->verifier)
		out.mac = poly1xsecpar_mul(c, state->delta);
	else
	{
		out.mac = poly_secpar_set_zero();
		out.value = c;
	}
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_const_gfsecpar(const quicksilver_state* state, poly_secpar_vec c)
{
	quicksilver_vec_gfsecpar out;
	if (state->verifier)
		out.mac = poly_2secpar_reduce_secpar(poly_secpar_mul(state->delta, c));
	else
	{
		out.mac = poly_secpar_set_zero();
		out.value = c;
	}
	return out;
}

inline quicksilver_vec_deg2 quicksilver_const_deg2(const quicksilver_state* state, poly_secpar_vec c)
{
	quicksilver_vec_deg2 out;
	out.mac1 = poly_2secpar_set_zero();
	if (state->verifier)
		out.mac0 = poly_secpar_mul(state->deltaSq, c);
	else
	{
		out.mac0 = poly_2secpar_set_zero();
		out.mac1 = poly_2secpar_from_secpar(c);
	}
	return out;
}

inline quicksilver_vec_deg2 quicksilver_lift_gfsecpar_to_deg2(
    const quicksilver_state* state,
    quicksilver_vec_gfsecpar z)
{
    quicksilver_vec_deg2 out;
    out.mac1 = poly_2secpar_set_zero();
    if (state->verifier) {
        out.mac0 = poly_secpar_mul(state->deltaSq, z.mac);
    } else {
        out.mac0 = poly_2secpar_set_zero();
        out.mac1 = poly_2secpar_from_secpar(poly_secpar_add(z.value, z.mac));
    }
    return out;
}

inline quicksilver_vec_deg2 quicksilver_const_deg2_gf2(const quicksilver_state* state, poly1_vec c)
{
	quicksilver_vec_deg2 out;
	out.mac1 = poly_2secpar_set_zero();
	if (state->verifier)
		out.mac0 = poly_2secpar_from_secpar(poly1xsecpar_mul(c, state->deltaSq));
	else
	{
		out.mac0 = poly_2secpar_set_zero();
		out.mac1 = poly_2secpar_from_secpar(poly_secpar_from_1(c));
	}
	return out;
}

inline quicksilver_vec_gf2 quicksilver_mul_const_gf2(const quicksilver_state* state, quicksilver_vec_gf2 x, poly1_vec c)
{
	x.mac = poly1xsecpar_mul(c, x.mac);
	if (!state->verifier)
		x.value &= c;
	return x;
}

inline quicksilver_vec_gfsecpar quicksilver_mul_const_gf2_gfsecpar(const quicksilver_state* state, quicksilver_vec_gf2 x, poly_secpar_vec c)
{
	quicksilver_vec_gfsecpar out;
	out.mac = poly_2secpar_reduce_secpar(poly_secpar_mul(c, x.mac));
	if (!state->verifier)
		out.value = poly1xsecpar_mul(x.value, c);
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_mul_const(const quicksilver_state* state, quicksilver_vec_gfsecpar x, poly_secpar_vec c)
{
	x.mac = poly_2secpar_reduce_secpar(poly_secpar_mul(c, x.mac));
	if (!state->verifier)
		x.value = poly_2secpar_reduce_secpar(poly_secpar_mul(c, x.value));
	return x;
}

inline quicksilver_vec_deg2 quicksilver_mul_const_deg2_gf2(const quicksilver_state* state, quicksilver_vec_deg2 x, poly1_vec c)
{
	quicksilver_vec_deg2 out;
	out.mac0 = poly1x2secpar_mul(c, x.mac0);
	out.mac1 = poly_2secpar_set_zero();
	if (!state->verifier)
	{
		out.mac1 = poly1x2secpar_mul(c, x.mac1);
	}
	return out;
}

inline quicksilver_vec_deg2 quicksilver_mul_const_deg2(const quicksilver_state* state, quicksilver_vec_deg2 x, poly_secpar_vec c)
{
	quicksilver_vec_deg2 out;
	out.mac0 = poly_secpar_mul(c, poly_2secpar_reduce_secpar(x.mac0));
	out.mac1 = poly_2secpar_set_zero();
	if (!state->verifier)
	{
		out.mac1 = poly_secpar_mul(c, poly_2secpar_reduce_secpar(x.mac1));
	}
	return out;
}

inline quicksilver_vec_deg2 quicksilver_mul(const quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y)
{
	quicksilver_vec_deg2 out;
	out.mac0 = poly_secpar_mul(x.mac, y.mac);
	out.mac1 = poly_2secpar_set_zero();
	if (!state->verifier)
	{
		out.mac1 = poly_secpar_mul(poly_secpar_add(x.value, x.mac), poly_secpar_add(y.value, y.mac));
	}
	return out;
}



inline quicksilver_vec_gfsecpar quicksilver_combine_1_bit(const quicksilver_state* state, const quicksilver_vec_gf2 qs_bit)
{
	quicksilver_vec_gfsecpar out;
	out.mac = qs_bit.mac;
	if (!state->verifier)
		out.value = poly_secpar_from_1(qs_bit.value);
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_combine_8_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits)
{
	quicksilver_vec_gfsecpar out;

	poly_secpar_vec macs[8];
	for (size_t i = 0; i < 8; ++i)
		macs[i] = qs_bits[i].mac;
	out.mac = poly_secpar_from_8_poly_secpar(macs);

	if (!state->verifier)
	{
		poly1_vec bits[8];
		for (size_t i = 0; i < 8; ++i)
			bits[i] = qs_bits[i].value;
		out.value = poly_secpar_from_8_poly1(bits);
	}

	return out;
}



static inline quicksilver_vec_gfsecpar quicksilver_combine_secpar_bits_not_full(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits) {
	quicksilver_vec_gfsecpar out;
	#if SECURITY_PARAM == 128
	poly_secpar_vec macs[128];
	for (size_t i = 0; i < 128; ++i) {
        macs[i] = poly_secpar_set_zero();
    }
	for (size_t i = 0; i < 9; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[128];
		for (size_t i = 0; i < 128; ++i) {
			bits[i] = 0;
		}
		for (size_t i = 0; i < 9; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 192
	poly_secpar_vec macs[192];
	for (size_t i = 0; i < 192; ++i) {
		macs[i] = poly_secpar_set_zero();
	}
	for (size_t i = 0; i < 5; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[192];
		for (size_t i = 0; i < 192; ++i) {
			bits[i] = 0;
		}
		for (size_t i = 0; i < 5; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 256
	poly_secpar_vec macs[256];
	for (size_t i = 0; i < 256; ++i) {
		macs[i] = poly_secpar_set_zero();
	}
	for (size_t i = 0; i < 7; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[256];
		for (size_t i = 0; i < 256; ++i) {
			bits[i] = 0;
		}
		for (size_t i = 0; i < 7; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 512
	poly_secpar_vec macs[512];
	poly1_vec bits[512];
	for (size_t i = 0; i < 512; ++i) { macs[i] = poly_secpar_set_zero(); bits[i] = 0; }
	for (size_t i = 0; i < 9; ++i) { macs[i] = qs_bits[i].mac; bits[i] = qs_bits[i].value; }
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier) out.value = poly_secpar_from_secpar_poly1(bits);
	#endif
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_combine_secpar_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits) {
	quicksilver_vec_gfsecpar out;
	#if SECURITY_PARAM == 128
	poly_secpar_vec macs[128];
	for (size_t i = 0; i < 128; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[128];
		for (size_t i = 0; i < 128; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 192
	poly_secpar_vec macs[192];
	for (size_t i = 0; i < 192; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[192];
		for (size_t i = 0; i < 192; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 256
	poly_secpar_vec macs[256];
	for (size_t i = 0; i < 256; ++i) {
		macs[i] = qs_bits[i].mac;
	}
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier)
	{
		poly1_vec bits[256];
		for (size_t i = 0; i < 256; ++i) {
			bits[i] = qs_bits[i].value;
		}
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#elif SECURITY_PARAM == 512
	poly_secpar_vec macs[512];
	for (size_t i = 0; i < 512; ++i) macs[i] = qs_bits[i].mac;
	out.mac = poly_secpar_from_secpar_poly_secpar(macs);
	if (!state->verifier) {
		poly1_vec bits[512];
		for (size_t i = 0; i < 512; ++i) bits[i] = qs_bits[i].value;
		out.value = poly_secpar_from_secpar_poly1(bits);
	}
	#endif
	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_combine_16_bits(const quicksilver_state* state, const quicksilver_vec_gf2* qs_bits)
{
	quicksilver_vec_gfsecpar out;

	poly_secpar_vec macs[16];
	for (size_t i = 0; i < 16; ++i)
		macs[i] = qs_bits[i].mac;
	out.mac = poly_secpar_from_16_poly_secpar(macs);

	if (!state->verifier)
	{
		poly1_vec bits[16];
		for (size_t i = 0; i < 16; ++i)
			bits[i] = qs_bits[i].value;
		out.value = poly_secpar_from_16_poly1(bits);
	}

	return out;
}

inline quicksilver_vec_gfsecpar quicksilver_const_8_bits(const quicksilver_state* state, const void* s)
{
    quicksilver_vec_gf2 input_bits[8];
    for (size_t bit_j = 0; bit_j < 8; ++bit_j) {
        input_bits[bit_j] = quicksilver_const_gf2(state, poly1_load(*(uint8_t*)s, bit_j));
    }
    return quicksilver_combine_8_bits(state, input_bits);
}

inline quicksilver_vec_gfsecpar quicksilver_const_secpar_bits(const quicksilver_state* state, const void* s)
{
	#if SECURITY_PARAM == 128
	quicksilver_vec_gf2 input_bits[144];
    for (size_t bit_j = 0; bit_j < 144; ++bit_j) {
        input_bits[bit_j] = quicksilver_const_gf2(state, poly1_load(*((uint8_t*)s + (bit_j/8)), bit_j%8));
    }
	#elif SECURITY_PARAM == 192
	quicksilver_vec_gf2 input_bits[192];
    for (size_t bit_j = 0; bit_j < 192; ++bit_j) {
        input_bits[bit_j] = quicksilver_const_gf2(state, poly1_load(*((uint8_t*)s + (bit_j/8)), bit_j%8));
    }
	#elif SECURITY_PARAM == 256
	quicksilver_vec_gf2 input_bits[256];
    for (size_t bit_j = 0; bit_j < 256; ++bit_j) {
        input_bits[bit_j] = quicksilver_const_gf2(state, poly1_load(*((uint8_t*)s + (bit_j/8)), bit_j%8));
    }
	#elif SECURITY_PARAM == 512
	quicksilver_vec_gf2 input_bits[512];
    for (size_t bit_j = 0; bit_j < 512; ++bit_j)
        input_bits[bit_j] = quicksilver_const_gf2(state, poly1_load(*((const uint8_t*)s + bit_j/8), bit_j%8));
	#endif
    return quicksilver_combine_secpar_bits(state, input_bits);
}

inline quicksilver_vec_gfsecpar quicksilver_get_witness_8_bits(const quicksilver_state* state, size_t bit_index)
{
    quicksilver_vec_gf2 input_bits[8];
    for (size_t bit_j = 0; bit_j < 8; ++bit_j) {
        input_bits[bit_j] = quicksilver_get_witness_vec(state, bit_index + bit_j);
    }
    return quicksilver_combine_8_bits(state, input_bits);
}

inline quicksilver_vec_gfsecpar quicksilver_get_witness_secpar_bits(const quicksilver_state* state, size_t bit_index)
{
	#if SECURITY_PARAM == 128
	quicksilver_vec_gf2 input_bits[128];
    for (size_t bit_j = 0; bit_j < 128; ++bit_j) {
        input_bits[bit_j] = quicksilver_get_witness_vec(state, bit_index + bit_j);
    }
	#elif SECURITY_PARAM == 192
	quicksilver_vec_gf2 input_bits[192];
    for (size_t bit_j = 0; bit_j < 192; ++bit_j) {
        input_bits[bit_j] = quicksilver_get_witness_vec(state, bit_index + bit_j);
    }
	#elif SECURITY_PARAM == 256
	quicksilver_vec_gf2 input_bits[256];
    for (size_t bit_j = 0; bit_j < 256; ++bit_j) {
        input_bits[bit_j] = quicksilver_get_witness_vec(state, bit_index + bit_j);
    }
	#elif SECURITY_PARAM == 512
	quicksilver_vec_gf2 input_bits[512];
    for (size_t bit_j = 0; bit_j < 512; ++bit_j)
        input_bits[bit_j] = quicksilver_get_witness_vec(state, bit_index + bit_j);
	#endif
    return quicksilver_combine_secpar_bits(state, input_bits);
}

inline void quicksilver_constraint(quicksilver_state* state, quicksilver_vec_deg2 x)
{
	poly_secpar_vec const_term = poly_2secpar_reduce_secpar(x.mac0);

	if (state->verifier)
	{
		hasher_gfsecpar_update(&state->key_secpar, &state->state_secpar_const, const_term);
		hasher_gfsecpar_64_update(&state->key_64, &state->state_64_const, const_term);
	}
	else
	{
		poly_secpar_vec lin_term = poly_secpar_add(const_term, poly_2secpar_reduce_secpar(x.mac1));

		hasher_gfsecpar_update(&state->key_secpar, &state->state_secpar_const, const_term);
		hasher_gfsecpar_update(&state->key_secpar, &state->state_secpar_linear, lin_term);
		hasher_gfsecpar_64_update(&state->key_64, &state->state_64_const, const_term);
		hasher_gfsecpar_64_update(&state->key_64, &state->state_64_linear, lin_term);
	}
}

inline void quicksilver_inverse_constraint(quicksilver_state* state, quicksilver_vec_gfsecpar x, quicksilver_vec_gfsecpar y)
{

	quicksilver_vec_deg2 mul = quicksilver_mul(state, x, y);
	quicksilver_vec_deg2 constraint = quicksilver_add_deg2(state, mul, quicksilver_one_deg2(state));
	quicksilver_constraint(state, constraint);
}


void quicksilver_prove(const quicksilver_state* restrict state, size_t witness_bits,
                       uint8_t* restrict proof, uint8_t* restrict check);
void quicksilver_verify(const quicksilver_state* restrict state, size_t witness_bits,
                        const uint8_t* restrict proof, uint8_t* restrict check);

#endif

