#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include "faest.h"
#include "faest_details.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <malloc.h>
#endif
#include "hash.h"
#include "owf_proof.h"
#include "owf_alpha.h"
#include "small_vole.h"
#include "vole_commit.h"
#include "util.h"
#include "timing.h"
#include "greatwall.h"
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>

static void* faest_aligned_alloc(size_t alignment, size_t size)
{
#if defined(_WIN32)
	return _aligned_malloc(size, alignment);
#else
	void* ptr = NULL;
	return posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;
#endif
}

static void faest_aligned_free(void* ptr)
{
#if defined(_WIN32)
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}


void faest_free_public_key(public_key* pk)
{
	(void) pk;
}

void faest_free_secret_key(secret_key* sk)
{
	faest_free_public_key(&sk->pk);
}

bool faest_unpack_secret_key(secret_key* unpacked, const uint8_t* packed)
{

#if SECURITY_PARAM == 128
	block137_load_to_block(&unpacked->sk, packed);
	block137_load_to_block(&unpacked->pk.owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 192
	block197_load_to_block(&unpacked->sk, packed);
	block197_load_to_block(&unpacked->pk.owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 256
	block263_load_to_block(&unpacked->sk, packed);
	block263_load_to_block(&unpacked->pk.owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 512
	block521_load_to_block(&unpacked->sk, packed);
	block521_load_to_block(&unpacked->pk.owf_iv, packed + GREATWALL_FIELD_BYTES);
#endif
	if (!faest_compute_witness(unpacked))
	{
		faest_free_secret_key(unpacked);
		return false;
	}

	return true;
}

static void faest_pack_secret_key(uint8_t* packed, const secret_key* unpacked)
{
#if SECURITY_PARAM == 128
	block137_store_from_block(packed, &unpacked->sk);
	block137_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->pk.owf_iv);
#elif SECURITY_PARAM == 192
	block197_store_from_block(packed, &unpacked->sk);
	block197_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->pk.owf_iv);
#elif SECURITY_PARAM == 256
	block263_store_from_block(packed, &unpacked->sk);
	block263_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->pk.owf_iv);
#elif SECURITY_PARAM == 512
	block521_store_from_block(packed, &unpacked->sk);
	block521_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->pk.owf_iv);
#endif
}

void faest_pack_public_key(uint8_t* packed, const public_key* unpacked)
{
#if SECURITY_PARAM == 128
	block137_store_from_block(packed, &unpacked->owf_output);
	block137_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->owf_iv);
#elif SECURITY_PARAM == 192
	block197_store_from_block(packed, &unpacked->owf_output);
	block197_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->owf_iv);
#elif SECURITY_PARAM == 256
	block263_store_from_block(packed, &unpacked->owf_output);
	block263_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->owf_iv);
#elif SECURITY_PARAM == 512
	block521_store_from_block(packed, &unpacked->owf_output);
	block521_store_from_block(packed + GREATWALL_FIELD_BYTES, &unpacked->owf_iv);
#endif
}


void faest_unpack_public_key(public_key* unpacked, const uint8_t* packed)
{
#if SECURITY_PARAM == 128
	block137_load_to_block(&unpacked->owf_output, packed);
	block137_load_to_block(&unpacked->owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 192
	block197_load_to_block(&unpacked->owf_output, packed);
	block197_load_to_block(&unpacked->owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 256
	block263_load_to_block(&unpacked->owf_output, packed);
	block263_load_to_block(&unpacked->owf_iv, packed + GREATWALL_FIELD_BYTES);
#elif SECURITY_PARAM == 512
	block521_load_to_block(&unpacked->owf_output, packed);
	block521_load_to_block(&unpacked->owf_iv, packed + GREATWALL_FIELD_BYTES);
#endif
}

#include <inttypes.h>

void print_block137_hex(const block137* b) {
	(void)b;
}
void print_block197_hex(const block197* b) {
	(void)b;
}

void print_block263_hex(const block263* b) {
	(void)b;
}
bool faest_compute_witness(secret_key* sk)
{
	owf_block owf_alpha0;
	owf_block owf_alpha1;
	if (!derive_owf_alpha(&owf_alpha0, &sk->pk.owf_iv, 0) ||
	    !derive_owf_alpha(&owf_alpha1, &sk->pk.owf_iv, 1) ||
	    owf_alpha_is_zero(&owf_alpha0) ||
	    owf_alpha_is_zero(&owf_alpha1))
		return false;

#if SECURITY_PARAM == 128
	uint8_t* w_ptr = (uint8_t*) &sk->witness;

	block137_store_from_block(w_ptr, &sk->sk);
	w_ptr += GREATWALL_FIELD_BYTES;
	block137 state = sk->sk;
	print_block137_hex(&state);
	block137 rc0, rc1;
	block137_load_words(&rc0, greatwall_rc_137[0]);
	block137_load_words(&rc1, greatwall_rc_137[1]);
	
	block137_multiply_with_GF2_matrix(&state, greatwall_mat_137_0);
	print_block137_hex(&state);
	state = block137_xor(state, rc0);
	print_block137_hex(&state);
	block137_mersenne_inverse1(&state);
	if (!owf_alpha_dot_is_one(&owf_alpha0, &state))
		return false;
	
	print_block137_hex(&state);
	
	block137_store_from_block(w_ptr, &state);
	w_ptr += GREATWALL_FIELD_BYTES;

	state = block137_xor(state, sk->sk);
	block137_multiply_with_GF2_matrix(&state, greatwall_mat_137_1);
	state = block137_xor(state, sk->pk.owf_iv);
	block137 tmp = state;
	block137_mersenne_inverse2(&state);
	if (!owf_alpha_dot_is_one(&owf_alpha1, &state))
		return false;
	block137_multiply_with_GF2_matrix(&state, greatwall_mat_137_2);
	state = block137_xor(state, rc1);
	state = block137_xor(state, tmp);
	state = block137_xor(state, sk->sk);

	sk->pk.owf_output = state;

#elif SECURITY_PARAM == 192

    uint8_t* w_ptr = (uint8_t*) &sk->witness;

    block197_store_from_block(w_ptr, &sk->sk);
    w_ptr += GREATWALL_FIELD_BYTES;

    block197 state = sk->sk;
    print_block197_hex(&state);

    block197 rc0, rc1;
    block197_load_words(&rc0, greatwall_rc_197[0]);
    block197_load_words(&rc1, greatwall_rc_197[1]);

    block197_multiply_with_GF2_matrix(&state, greatwall_mat_197_0);
    print_block197_hex(&state);

    state = block197_xor(state, rc0);
    print_block197_hex(&state);

    block197_mersenne_inverse1(&state);
    if (!owf_alpha_dot_is_one(&owf_alpha0, &state))
        return false;
    print_block197_hex(&state);

    block197_store_from_block(w_ptr, &state);
    w_ptr += GREATWALL_FIELD_BYTES;

    state = block197_xor(state, sk->sk);

    block197_multiply_with_GF2_matrix(&state, greatwall_mat_197_1);
    state = block197_xor(state, sk->pk.owf_iv);

    block197 tmp = state;

    block197_mersenne_inverse2(&state);
    if (!owf_alpha_dot_is_one(&owf_alpha1, &state))
        return false;

    block197_multiply_with_GF2_matrix(&state, greatwall_mat_197_2);

    state = block197_xor(state, rc1);

    state = block197_xor(state, tmp);
    state = block197_xor(state, sk->sk);

    sk->pk.owf_output = state;

#elif SECURITY_PARAM == 256

    uint8_t* w_ptr = (uint8_t*) &sk->witness;

    block263_store_from_block(w_ptr, &sk->sk);
    w_ptr += GREATWALL_FIELD_BYTES;

    block263 state = sk->sk;
    print_block263_hex(&state);

    block263 rc0, rc1;
    block263_load_words(&rc0, greatwall_rc_263[0]);
    block263_load_words(&rc1, greatwall_rc_263[1]);

    block263_multiply_with_GF2_matrix(&state, greatwall_mat_263_0);
    print_block263_hex(&state);

    state = block263_xor(state, rc0);
    print_block263_hex(&state);

    block263_mersenne_inverse1(&state);
    if (!owf_alpha_dot_is_one(&owf_alpha0, &state))
        return false;
    print_block263_hex(&state);

    block263_store_from_block(w_ptr, &state);
    w_ptr += GREATWALL_FIELD_BYTES;

    state = block263_xor(state, sk->sk);

    block263_multiply_with_GF2_matrix(&state, greatwall_mat_263_1);
    state = block263_xor(state, sk->pk.owf_iv);

    block263 tmp = state;

    block263_mersenne_inverse2(&state);
    if (!owf_alpha_dot_is_one(&owf_alpha1, &state))
        return false;

    block263_multiply_with_GF2_matrix(&state, greatwall_mat_263_2);

    state = block263_xor(state, rc1);

    state = block263_xor(state, tmp);
    state = block263_xor(state, sk->sk);

    sk->pk.owf_output = state;

#elif SECURITY_PARAM == 512
	uint8_t* w_ptr = (uint8_t*)&sk->witness;
	block521_store_from_block(w_ptr, &sk->sk);
	w_ptr += GREATWALL_FIELD_BYTES;

	block521 state = sk->sk;
	block521 rc0, rc1;
	block521_load_words(&rc0, greatwall_rc_521[0]);
	block521_load_words(&rc1, greatwall_rc_521[1]);

	block521_multiply_with_GF2_matrix(&state, greatwall_mat_521_0);
	state = block521_xor(state, rc0);
	block521_mersenne_inverse1(&state);
	if (!owf_alpha_dot_is_one(&owf_alpha0, &state))
		return false;
	block521_store_from_block(w_ptr, &state);

	state = block521_xor(state, sk->sk);
	block521_multiply_with_GF2_matrix(&state, greatwall_mat_521_1);
	state = block521_xor(state, sk->pk.owf_iv);
	block521 tmp = state;
	block521_mersenne_inverse2(&state);
	if (!owf_alpha_dot_is_one(&owf_alpha1, &state))
		return false;
	block521_multiply_with_GF2_matrix(&state, greatwall_mat_521_2);
	state = block521_xor(state, rc1);
	state = block521_xor(state, tmp);
	state = block521_xor(state, sk->sk);
	sk->pk.owf_output = state;

#endif

	return true;
}
bool faest_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed, secret_key* sk)
{
	if (!faest_unpack_secret_key(sk, sk_packed))
		return false;

	faest_pack_public_key(pk_packed, &sk->pk);
	return true;
}
bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed)
{
	secret_key sk;
	if (!faest_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk))
		return false;

	faest_free_secret_key(&sk);
	return true;
}

static bool faest_sign_attempt(
	uint8_t* signature, const uint8_t* msg, size_t msg_len,
	const secret_key* sk, const uint8_t* pk_packed,
	const uint8_t* random_seed, size_t random_seed_len, uint64_t attempt_num)
{
	FAEST_TIMING_DECLARE();
FAEST_TIMING_START();
	block_2secpar mu;
	hash_state hasher;
	hash_init(&hasher);
	hash_update(&hasher, pk_packed, GREATWALL_PUBLIC_KEY_BYTES);
	hash_update(&hasher, msg, msg_len);
	hash_update_byte(&hasher, 1);
	hash_final(&hasher, &mu, sizeof(mu));

	block_secpar seed;
	block128 iv;
	uint8_t seed_iv[sizeof(seed) + sizeof(iv)];

	hash_init(&hasher);
	uint8_t sk_packed[GREATWALL_SECRET_KEY_BYTES];
	faest_pack_secret_key(sk_packed, sk);
	hash_update(&hasher, sk_packed, GREATWALL_SECRET_KEY_BYTES);
	hash_update(&hasher, &mu, sizeof(mu));
	if (random_seed)
		hash_update(&hasher, random_seed, random_seed_len);

#if COUNTER_BYTES > 0
	uint8_t attempt_num_bytes[8];
	#ifdef __GNUC__
	#pragma GCC unroll (8)
	#endif
	for (int i = 0; i < 8; ++i)
		attempt_num_bytes[i] = attempt_num >> (8*i);
	hash_update(&hasher, &attempt_num_bytes[0], sizeof(attempt_num_bytes));
#else
	(void) attempt_num;
#endif
FAEST_TIMING_PRINT("first part");
FAEST_TIMING_START();
	hash_update_byte(&hasher, 3);
	hash_final(&hasher, seed_iv, sizeof(seed_iv));

	memcpy(&seed, seed_iv, sizeof(seed));
	memcpy(&iv, &seed_iv[sizeof(seed)], sizeof(iv));

	block_secpar* forest =
		faest_aligned_alloc(alignof(block_secpar), FOREST_SIZE * sizeof(block_secpar));
	block_2secpar* hashed_leaves =
		faest_aligned_alloc(alignof(block_2secpar), VECTOR_COMMIT_LEAVES * sizeof(block_2secpar));
	vole_block* u =
		faest_aligned_alloc(alignof(vole_block), VOLE_COL_BLOCKS * sizeof(vole_block));
	vole_block* v =
		faest_aligned_alloc(alignof(vole_block), SECURITY_PARAM * VOLE_COL_BLOCKS * sizeof(vole_block));
	uint8_t vole_commit_check[VOLE_COMMIT_CHECK_SIZE];

	vole_commit(seed, iv, forest, hashed_leaves, u, v, signature, vole_commit_check);

	uint8_t chal1[VOLE_CHECK_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &mu, sizeof(mu));
	hash_update(&hasher, vole_commit_check, VOLE_COMMIT_CHECK_SIZE);
	hash_update(&hasher, signature, VOLE_COMMIT_SIZE);
	hash_update(&hasher, &iv, sizeof(iv));
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal1[0], sizeof(chal1));

	uint8_t* vole_check_proof = signature + VOLE_COMMIT_SIZE;
	uint8_t vole_check_check[VOLE_CHECK_CHECK_BYTES];
	vole_check_sender(u, v, chal1, vole_check_proof, vole_check_check);

	uint8_t* correction = vole_check_proof + VOLE_CHECK_PROOF_BYTES;
	size_t remainder = (WITNESS_BITS / 8) % (16 * VOLE_BLOCK);
	for (size_t i = 0; i < WITNESS_BLOCKS - (remainder != 0); ++i)
	{
		vole_block correction_i = vole_block_xor(u[i], sk->witness[i]);
		memcpy(correction + i * sizeof(vole_block), &correction_i, sizeof(vole_block));
	}
	if (remainder)
	{
		vole_block correction_i = vole_block_xor(u[WITNESS_BLOCKS - 1], sk->witness[WITNESS_BLOCKS - 1]);
		memcpy(correction + (WITNESS_BLOCKS - 1) * sizeof(vole_block), &correction_i, remainder);
	}
FAEST_TIMING_PRINT("c1 part");
FAEST_TIMING_START();
	uint8_t chal2[QUICKSILVER_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, chal1, sizeof(chal1));
    hash_update(&hasher, vole_check_proof, VOLE_CHECK_PROOF_BYTES);
    hash_update(&hasher, vole_check_check, VOLE_CHECK_CHECK_BYTES);
    hash_update(&hasher, correction, WITNESS_BITS / 8);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal2[0], sizeof(chal2));

	block_secpar* macs =
		faest_aligned_alloc(alignof(block_secpar), QUICKSILVER_ROWS_PADDED * sizeof(block_secpar));

	memcpy(&u[0], &sk->witness[0], WITNESS_BITS / 8);
	static_assert(QUICKSILVER_ROWS_PADDED % TRANSPOSE_BITS_ROWS == 0, "");
	transpose_secpar(v, macs, VOLE_COL_STRIDE, QUICKSILVER_ROWS_PADDED);
	faest_aligned_free(v);
FAEST_TIMING_PRINT("c2 part");
FAEST_TIMING_START();
	quicksilver_state qs;
	quicksilver_init_prover(&qs, (uint8_t*) &u[0], macs, OWF_NUM_CONSTRAINTS, chal2);
FAEST_TIMING_PRINT("quicksilver_init_prover part");
FAEST_TIMING_START();
	owf_constraints_prover(&qs, &sk->pk);
FAEST_TIMING_PRINT("owf_constraints_prover part");
FAEST_TIMING_START();
	uint8_t* qs_proof = correction + WITNESS_BITS / 8;
	uint8_t qs_check[QUICKSILVER_CHECK_BYTES];
	quicksilver_prove(&qs, WITNESS_BITS, qs_proof, qs_check);
	faest_aligned_free(macs);
	faest_aligned_free(u);
FAEST_TIMING_PRINT("quicksilver_prove part");
FAEST_TIMING_START();
	uint8_t* veccom_open_start = qs_proof + QUICKSILVER_PROOF_BYTES;
	uint8_t* delta = veccom_open_start + VECTOR_COM_OPEN_SIZE;

#if COUNTER_BYTES == 0
	hash_init(&hasher);
	hash_update(&hasher, &chal2, sizeof(chal2));
	hash_update(&hasher, qs_proof, QUICKSILVER_PROOF_BYTES);
	hash_update(&hasher, qs_check, QUICKSILVER_CHECK_BYTES);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, delta, sizeof(block_secpar));

	uint8_t delta_bytes[SECURITY_PARAM];
	for (size_t i = 0; i < SECURITY_PARAM; ++i)
		delta_bytes[i] = expand_bit_to_byte(delta[i / 8], i % 8);
	vector_open(forest, hashed_leaves, delta_bytes, veccom_open_start);
	bool open_success = true;

#else
	uint32_t counter = 0;
	unsigned char hash_prefix[sizeof(chal2) + QUICKSILVER_PROOF_BYTES + QUICKSILVER_CHECK_BYTES];
	memcpy(hash_prefix, &chal2, sizeof(chal2));
	memcpy(hash_prefix + sizeof(chal2), qs_proof, QUICKSILVER_PROOF_BYTES);
	memcpy(hash_prefix + sizeof(chal2) + QUICKSILVER_PROOF_BYTES, qs_check, QUICKSILVER_CHECK_BYTES);
	bool open_success = force_vector_open(forest, hashed_leaves, delta, veccom_open_start, hash_prefix, sizeof(chal2) + QUICKSILVER_PROOF_BYTES + QUICKSILVER_CHECK_BYTES, &counter);
#endif

	faest_aligned_free(forest);
	faest_aligned_free(hashed_leaves);
FAEST_TIMING_PRINT("open part");
FAEST_TIMING_START();
	if (!open_success)
		return false;

	uint8_t* iv_dst = delta + sizeof(block_secpar);
	memcpy(iv_dst, &iv, sizeof(iv));

	uint8_t* counter_dst = iv_dst + sizeof(iv);

#if COUNTER_BYTES > 0
	counter_dst[0] = counter;
	counter_dst[1] = counter>>8;
	counter_dst[2] = counter>>16;
	counter_dst[3] = counter>>24;
#else
	(void) counter_dst;
#endif

	assert(counter_dst + COUNTER_BYTES == signature + FAEST_SIGNATURE_BYTES);

FAEST_TIMING_PRINT("last part");
	return true;
}

bool faest_sign(
	uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
	const uint8_t* random_seed, size_t random_seed_len)
{
	secret_key sk;
	uint8_t pk_packed[GREATWALL_PUBLIC_KEY_BYTES];
	if (!faest_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk))
		return false;

	uint64_t attempt_num = 0;
	do
	{
		if (faest_sign_attempt(signature, msg, msg_len, &sk, &pk_packed[0],
			                   random_seed, random_seed_len, attempt_num))
		{
			faest_free_secret_key(&sk);
			return true;
		}
	} while (++attempt_num != 0);

	faest_free_secret_key(&sk);
	return false;
}

bool faest_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed)
{
	block128 iv;
	block_2secpar mu;
	hash_state hasher;
	hash_init(&hasher);
	hash_update(&hasher, pk_packed, GREATWALL_PUBLIC_KEY_BYTES);
	hash_update(&hasher, msg, msg_len);
	hash_update_byte(&hasher, 1);
	hash_final(&hasher, &mu, sizeof(mu));

	const uint8_t* vole_check_proof = signature + VOLE_COMMIT_SIZE;
	const uint8_t* correction = vole_check_proof + VOLE_CHECK_PROOF_BYTES;
	const uint8_t* qs_proof = correction + WITNESS_BITS / 8;
	const uint8_t* veccom_open_start = qs_proof + QUICKSILVER_PROOF_BYTES;
	const uint8_t* delta = veccom_open_start + VECTOR_COM_OPEN_SIZE;
	const uint8_t* iv_ptr = delta + sizeof(block_secpar);
#if COUNTER_BYTES > 0
	const uint8_t* counter = iv_ptr + sizeof(iv);
#endif

	uint8_t delta_bytes[SECURITY_PARAM];
	for (size_t i = 0; i < SECURITY_PARAM; ++i)
		delta_bytes[i] = expand_bit_to_byte(delta[i / 8], i % 8);

	vole_block* q =
		faest_aligned_alloc(alignof(vole_block), SECURITY_PARAM * VOLE_COL_BLOCKS * sizeof(vole_block));
	uint8_t vole_commit_check[VOLE_COMMIT_CHECK_SIZE];

	memcpy(&iv, iv_ptr, sizeof(iv));
	bool reconstruct_success =  vole_reconstruct(iv, q, delta_bytes, signature, veccom_open_start, vole_commit_check);
	if (reconstruct_success == 0){
		faest_aligned_free(q);
		return 0;
	}

	uint8_t chal1[VOLE_CHECK_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &mu, sizeof(mu));
	hash_update(&hasher, vole_commit_check, VOLE_COMMIT_CHECK_SIZE);
	hash_update(&hasher, signature, VOLE_COMMIT_SIZE);
	hash_update(&hasher, &iv, sizeof(iv));
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal1[0], sizeof(chal1));

	uint8_t vole_check_check[VOLE_CHECK_CHECK_BYTES];
	vole_check_receiver(q, delta_bytes, chal1, vole_check_proof, vole_check_check);

	uint8_t chal2[QUICKSILVER_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &chal1, sizeof(chal1));
	hash_update(&hasher, vole_check_proof, VOLE_CHECK_PROOF_BYTES);
	hash_update(&hasher, vole_check_check, VOLE_CHECK_CHECK_BYTES);
	hash_update(&hasher, correction, WITNESS_BITS / 8);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal2[0], sizeof(chal2));

	vole_block correction_blocks[WITNESS_BLOCKS];
	memcpy(&correction_blocks, correction, WITNESS_BITS / 8);
	memset(((uint8_t*) &correction_blocks) + WITNESS_BITS / 8, 0,
	       sizeof(correction_blocks) - WITNESS_BITS / 8);
	vole_receiver_apply_correction(WITNESS_BLOCKS, NONZERO_BITS_IN_CHALLENGE_3, correction_blocks, q, delta_bytes);

	block_secpar* macs =
		faest_aligned_alloc(alignof(block_secpar), VOLE_ROWS_PADDED * sizeof(block_secpar));
	transpose_secpar(q, macs, VOLE_COL_STRIDE, QUICKSILVER_ROWS_PADDED);
	faest_aligned_free(q);

	block_secpar delta_block;
	memcpy(&delta_block, delta, sizeof(delta_block));

	public_key pk;
	faest_unpack_public_key(&pk, pk_packed);

	quicksilver_state qs;
	quicksilver_init_verifier(&qs, macs, OWF_NUM_CONSTRAINTS, delta_block, chal2);
	owf_constraints_verifier(&qs, &pk);

	faest_free_public_key(&pk);
	uint8_t qs_check[QUICKSILVER_CHECK_BYTES];
	quicksilver_verify(&qs, WITNESS_BITS, qs_proof, qs_check);
	faest_aligned_free(macs);

	block_secpar delta_check;
	hash_init(&hasher);
	hash_update(&hasher, &chal2, sizeof(chal2));
	hash_update(&hasher, qs_proof, QUICKSILVER_PROOF_BYTES);
	hash_update(&hasher, qs_check, QUICKSILVER_CHECK_BYTES);
#if COUNTER_BYTES > 0
	hash_update(&hasher, counter, COUNTER_BYTES);
#endif
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &delta_check, sizeof(delta_check));

	return memcmp(delta, &delta_check, sizeof(delta_check)) == 0;
}
