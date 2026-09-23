#include "loong_hash.h"
#include "loong_status.h"
#include "auxfunc.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOONG_TAG_ID "BAG-Loong/ID/v1"
#define LOONG_TAG_G "BAG-Loong/G/v1"
#define LOONG_TAG_K "BAG-Loong/K/v1"
#define LOONG_TAG_PUBLIC "BAG-Loong/PKE-public/v1"
#define LOONG_TAG_KEYGEN "BAG-Loong/PKE-keygen/v1"
#define LOONG_TAG_ENC "BAG-Loong/PKE-enc/v1"

static int bytes_to_bits(unsigned long long bytes, unsigned long long *bits)
{
	if (bits == 0) {
		return LOONG_ERR_NULL;
	}
	if (bytes > ULLONG_MAX / 8ULL) {
		return LOONG_ERR_OVERFLOW;
	}
	*bits = bytes * 8ULL;
	return LOONG_SUCCESS;
}

static void store_u64_be(unsigned char out[8], unsigned long long v)
{
	out[0] = (unsigned char)(v >> 56);
	out[1] = (unsigned char)(v >> 48);
	out[2] = (unsigned char)(v >> 40);
	out[3] = (unsigned char)(v >> 32);
	out[4] = (unsigned char)(v >> 24);
	out[5] = (unsigned char)(v >> 16);
	out[6] = (unsigned char)(v >> 8);
	out[7] = (unsigned char)v;
}

static int add_len(unsigned long long *total, unsigned long long add)
{
	if (*total > ULLONG_MAX - add) {
		return LOONG_ERR_OVERFLOW;
	}
	*total += add;
	return LOONG_SUCCESS;
}

static int build_tagged_message(unsigned char **out,
                                unsigned long long *out_len_bytes,
                                const char *tag,
                                const loong_hash_input *inputs,
                                size_t input_count)
{
	unsigned long long total;
	unsigned long long tag_len;
	unsigned long long offset;
	size_t i;
	unsigned char len_be[8];

	if (out == 0 || out_len_bytes == 0 || tag == 0) {
		return LOONG_ERR_NULL;
	}
	if (input_count != 0 && inputs == 0) {
		return LOONG_ERR_NULL;
	}

	tag_len = (unsigned long long)strlen(tag);
	total = tag_len;
	for (i = 0; i < input_count; i++) {
		if (inputs[i].len_bytes != 0 && inputs[i].data == 0) {
			return LOONG_ERR_NULL;
		}
		if (add_len(&total, 8ULL) != LOONG_SUCCESS ||
		    add_len(&total, inputs[i].len_bytes) != LOONG_SUCCESS) {
			return LOONG_ERR_OVERFLOW;
		}
	}
	if (total > (unsigned long long)SIZE_MAX) {
		return LOONG_ERR_OVERFLOW;
	}

	*out = (unsigned char *)malloc((size_t)(total == 0 ? 1 : total));
	if (*out == 0) {
		return LOONG_ERR_ALLOC;
	}

	memcpy(*out, tag, (size_t)tag_len);
	offset = tag_len;
	for (i = 0; i < input_count; i++) {
		unsigned long long bit_len;
		int rc = bytes_to_bits(inputs[i].len_bytes, &bit_len);
		if (rc != LOONG_SUCCESS) {
			free(*out);
			*out = 0;
			return rc;
		}
		store_u64_be(len_be, bit_len);
		memcpy(*out + offset, len_be, sizeof(len_be));
		offset += sizeof(len_be);
		if (inputs[i].len_bytes != 0) {
			memcpy(*out + offset, inputs[i].data, (size_t)inputs[i].len_bytes);
			offset += inputs[i].len_bytes;
		}
	}

	*out_len_bytes = total;
	return LOONG_SUCCESS;
}

int loong_hash_tagged_sm3_256(unsigned char out[32], const char *tag,
                              const loong_hash_input *inputs,
                              size_t input_count)
{
	unsigned char *msg = 0;
	unsigned long long msg_len_bytes = 0;
	unsigned long long msg_len_bits = 0;
	int rc;

	if (out == 0) {
		return LOONG_ERR_NULL;
	}
	rc = build_tagged_message(&msg, &msg_len_bytes, tag, inputs, input_count);
	if (rc != LOONG_SUCCESS) {
		return rc;
	}
	rc = bytes_to_bits(msg_len_bytes, &msg_len_bits);
	if (rc == LOONG_SUCCESS) {
		rc = sm3hash(256, msg, msg_len_bits, out);
		rc = rc == 0 ? LOONG_SUCCESS : LOONG_ERR_API_PKC;
	}
	free(msg);
	return rc;
}

int loong_hash_tagged_pseudohash_512(unsigned char out[64], const char *tag,
                                     const loong_hash_input *inputs,
                                     size_t input_count)
{
	unsigned char *msg = 0;
	unsigned long long msg_len_bytes = 0;
	unsigned long long msg_len_bits = 0;
	int rc;

	if (out == 0) {
		return LOONG_ERR_NULL;
	}
	rc = build_tagged_message(&msg, &msg_len_bytes, tag, inputs, input_count);
	if (rc != LOONG_SUCCESS) {
		return rc;
	}
	rc = bytes_to_bits(msg_len_bytes, &msg_len_bits);
	if (rc == LOONG_SUCCESS) {
		rc = pseudohash(512, msg, msg_len_bits, out);
		rc = rc == 0 ? LOONG_SUCCESS : LOONG_ERR_API_PKC;
	}
	free(msg);
	return rc;
}

int loong_hash_tagged_xof(unsigned char *out, unsigned long long out_len_bits,
                          const char *tag, const loong_hash_input *inputs,
                          size_t input_count)
{
	unsigned char *msg = 0;
	unsigned long long msg_len_bytes = 0;
	unsigned long long msg_len_bits = 0;
	int rc;

	if (out_len_bits != 0 && out == 0) {
		return LOONG_ERR_NULL;
	}
	rc = build_tagged_message(&msg, &msg_len_bytes, tag, inputs, input_count);
	if (rc != LOONG_SUCCESS) {
		return rc;
	}
	rc = bytes_to_bits(msg_len_bytes, &msg_len_bits);
	if (rc == LOONG_SUCCESS) {
		rc = pseudoXOF(out_len_bits, msg, msg_len_bits, out);
		rc = rc == 0 ? LOONG_SUCCESS : LOONG_ERR_API_PKC;
	}
	free(msg);
	return rc;
}

int loong_hash_id(unsigned char id_pk[32], const unsigned char *pk,
                  unsigned long long pk_len_bytes)
{
	loong_hash_input input;
	input.data = pk;
	input.len_bytes = pk_len_bytes;
	return loong_hash_tagged_sm3_256(id_pk, LOONG_TAG_ID, &input, 1);
}

int loong_hash_g(unsigned char out[64], const unsigned char *id_pk,
                 unsigned long long id_pk_len_bytes, const unsigned char *m,
                 unsigned long long m_len_bytes, const unsigned char *salt,
                 unsigned long long salt_len_bytes)
{
	loong_hash_input inputs[3];
	inputs[0].data = id_pk;
	inputs[0].len_bytes = id_pk_len_bytes;
	inputs[1].data = m;
	inputs[1].len_bytes = m_len_bytes;
	inputs[2].data = salt;
	inputs[2].len_bytes = salt_len_bytes;
	return loong_hash_tagged_pseudohash_512(out, LOONG_TAG_G, inputs, 3);
}

int loong_hash_kdf(unsigned char *ss, unsigned long long ss_len_bytes,
                   const unsigned char *secret,
                   unsigned long long secret_len_bytes,
                   const unsigned char *ct_core,
                   unsigned long long ct_core_len_bytes)
{
	loong_hash_input inputs[2];
	if (ss_len_bytes > ULLONG_MAX / 8ULL) {
		return LOONG_ERR_OVERFLOW;
	}
	inputs[0].data = secret;
	inputs[0].len_bytes = secret_len_bytes;
	inputs[1].data = ct_core;
	inputs[1].len_bytes = ct_core_len_bytes;
	return loong_hash_tagged_xof(ss, ss_len_bytes * 8ULL, LOONG_TAG_K, inputs, 2);
}

int loong_xof_expand_public_seed(unsigned char *out,
                                 unsigned long long out_len_bits,
                                 const unsigned char *seed,
                                 unsigned long long seed_len_bytes)
{
	loong_hash_input input;
	input.data = seed;
	input.len_bytes = seed_len_bytes;
	return loong_hash_tagged_xof(out, out_len_bits, LOONG_TAG_PUBLIC, &input, 1);
}

int loong_xof_expand_keygen_seed(unsigned char *out,
                                 unsigned long long out_len_bits,
                                 const unsigned char *seed,
                                 unsigned long long seed_len_bytes)
{
	loong_hash_input input;
	input.data = seed;
	input.len_bytes = seed_len_bytes;
	return loong_hash_tagged_xof(out, out_len_bits, LOONG_TAG_KEYGEN, &input, 1);
}

int loong_xof_expand_enc_seed(unsigned char *out,
                              unsigned long long out_len_bits,
                              const unsigned char *theta,
                              unsigned long long theta_len_bytes)
{
	loong_hash_input input;
	input.data = theta;
	input.len_bytes = theta_len_bytes;
	return loong_hash_tagged_xof(out, out_len_bits, LOONG_TAG_ENC, &input, 1);
}
