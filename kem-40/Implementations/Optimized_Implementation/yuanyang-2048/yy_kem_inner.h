#ifndef YY_KEM_512_INNER_H
#define YY_KEM_512_INNER_H

#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include "params.h"

#define YY_SUCCESS             0
#define YY_INCORRECT           -1
#define YUANYANG_SUCCESS             0
#define YUANYANG_NOT_IMPLEMENTED    -2
#define YUANYANG_BADARG             -3
#define YUANYANG_MEMORY_ERROR       -4
#define YUANYANG_KEYGEN_FAILED      -5

typedef struct prng prng;

typedef struct {
	uint16_t h[YUANYANG_D];
	int8_t f[YUANYANG_D];
	uint8_t hash[YUANYANG_D/32]; // public key hash
} yy_kem_compact_sk;

typedef struct {
	yy_kem_compact_sk compact;
#ifdef FLOAT
	alignas(64) fpr finv[YUANYANG_D];
	alignas(64) uint8_t finvint[YUANYANG_D/2];
	alignas(64) uint32_t f[YUANYANG_D];
	alignas(64) fpr tmp[YUANYANG_D];
#else
	alignas(64) uint32_t finv[YUANYANG_D];
	alignas(64) uint8_t finvint[YUANYANG_D/2];
	alignas(64) uint32_t f[YUANYANG_D];
	alignas(64) uint32_t tmp[YUANYANG_D];
#endif
	alignas(64) int16_t fmbias[YUANYANG_D];
} yy_kem_expanded_sk;

size_t yy_get_ciphertext_bytes();
size_t yy_get_pk_len_bytes();
size_t yy_get_sk_len_bytes();
size_t yy_get_ss_len_bytes();

int yy_decapsulate(
	int8_t message[YUANYANG_D/32],
	const unsigned char *ct, yy_kem_expanded_sk *sk);

int yy_decapsulate_API(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes);

int yy_encapsulate_API(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes);

// from a yuan yang SIGNATURE private key
int yy_decode_private_key_kem(
	yy_kem_compact_sk *decoded,
	const unsigned char *sk, unsigned long long sk_len_bytes);

int yy_kem_expand_private_key(
	yy_kem_expanded_sk *expanded,
	const yy_kem_compact_sk *compact);

// generates KEM-ONLY pk/sk
int yy_kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,unsigned char *sk, unsigned long long *sk_len_bytes);

int gaussian_sample_sigma_0(int *out, prng *rng);
int gaussian_sample_sigma_sqrt2(int *out, prng *rng);



#endif
