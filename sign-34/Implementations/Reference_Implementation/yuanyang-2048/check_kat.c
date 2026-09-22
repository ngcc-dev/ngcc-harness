#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "SIG_AlgorithmInstance.h"

#define SEED_LEN_BYTES 64u
#define VECTOR_COUNT   10u

extern DRNG_ctx drng_algorithm;

static int
hex_value(int c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

static const char *
payload_after_equals(const char *line)
{
	const char *p;

	p = strchr(line, '=');
	if (p == NULL) {
		return NULL;
	}
	p++;
	while (*p != '\0' && isspace((unsigned char)*p)) {
		p++;
	}
	return p;
}

static int
parse_count_line(const char *line, unsigned *count)
{
	const char *p;

	if (strncmp(line, "Count = ", 8) != 0) {
		return 0;
	}
	p = payload_after_equals(line);
	if (p == NULL) {
		return 0;
	}
	*count = (unsigned)strtoul(p, NULL, 10);
	return 1;
}

static int
parse_len_line(const char *line, const char *label, unsigned long long *len)
{
	const char *p;
	size_t label_len;

	label_len = strlen(label);
	if (strncmp(line, label, label_len) != 0) {
		return 0;
	}
	p = payload_after_equals(line);
	if (p == NULL || !isdigit((unsigned char)*p)) {
		return 0;
	}
	*len = strtoull(p, NULL, 10);
	return 1;
}

static int
parse_hex_line(const char *line, const char *label,
	unsigned char *out, unsigned long long out_len)
{
	const char *p;
	size_t label_len;

	label_len = strlen(label);
	if (strncmp(line, label, label_len) != 0) {
		return 0;
	}
	p = payload_after_equals(line);
	if (p == NULL) {
		return 0;
	}
	for (unsigned long long i = 0; i < out_len; i++) {
		int hi, lo;

		hi = hex_value((unsigned char)p[2u * i]);
		lo = hex_value((unsigned char)p[2u * i + 1u]);
		if (hi < 0 || lo < 0) {
			return 0;
		}
		out[i] = (unsigned char)((hi << 4) | lo);
	}
	if (isxdigit((unsigned char)p[2u * out_len])) {
		return 0;
	}
	return 1;
}

static int
read_required_line(FILE *f, char *line, size_t line_len)
{
	size_t len;

	if (fgets(line, (int)line_len, f) == NULL) {
		return 0;
	}
	len = strlen(line);
	while (len > 0 && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
		line[--len] = '\0';
	}
	return 1;
}

static void
skip_blank_lines(FILE *f, char *line, size_t line_len, int *have_line)
{
	while (read_required_line(f, line, line_len)) {
		if (line[0] != '\0') {
			*have_line = 1;
			return;
		}
	}
	*have_line = 0;
}

static int
read_len_and_hex(
	FILE *f, char *line, size_t line_len,
	const char *len_label, const char *hex_label,
	unsigned char **out, unsigned long long *out_len)
{
	if (!read_required_line(f, line, line_len)
		|| !parse_len_line(line, len_label, out_len))
	{
		return 0;
	}
	*out = (unsigned char *)malloc((size_t)*out_len);
	if (*out == NULL) {
		return 0;
	}
	if (!read_required_line(f, line, line_len)
		|| !parse_hex_line(line, hex_label, *out, *out_len))
	{
		free(*out);
		*out = NULL;
		return 0;
	}
	return 1;
}

static int
constant_time_equal(
	const unsigned char *a, const unsigned char *b, unsigned long long len)
{
	unsigned diff;

	diff = 0;
	for (unsigned long long i = 0; i < len; i++) {
		diff |= (unsigned)(a[i] ^ b[i]);
	}
	return diff == 0;
}

static int
check_equal_field(
	const char *label,
	unsigned vector_index,
	const unsigned char *got, unsigned long long got_len,
	const unsigned char *expected, unsigned long long expected_len)
{
	if (got_len != expected_len) {
		fprintf(stderr, "vector %u: %s length mismatch: got %llu expected %llu\n",
			vector_index, label, got_len, expected_len);
		return 0;
	}
	if (!constant_time_equal(got, expected, got_len)) {
		fprintf(stderr, "vector %u: %s bytes mismatch\n", vector_index, label);
		return 0;
	}
	return 1;
}

int
main(int argc, char **argv)
{
	FILE *f;
	char *line;
	size_t line_len;
	DRNG_ctx drng_seed;
	DRNG_ctx drng_msg;
	unsigned char nonce_seed[SEED_LEN_BYTES];
	unsigned char nonce_msg[SEED_LEN_BYTES];
	unsigned vector_count;
	int have_line;

	if (argc != 2) {
		fprintf(stderr, "usage: %s KAT_SIG_yuanyang-2048.txt\n", argv[0]);
		return 2;
	}
	f = fopen(argv[1], "rb");
	if (f == NULL) {
		perror(argv[1]);
		return 2;
	}
	line_len = 1048576u;
	line = (char *)malloc(line_len);
	if (line == NULL) {
		fclose(f);
		return 2;
	}

	for (unsigned i = 0; i < SEED_LEN_BYTES / 4u; i++) {
		memcpy(nonce_seed + 4u * i, "seed", 4u);
	}
	memset(nonce_msg, 0, sizeof nonce_msg);
	for (unsigned i = 0; i < SEED_LEN_BYTES / 3u; i++) {
		memcpy(nonce_msg + 3u * i, "msg", 3u);
	}
	nonce_msg[SEED_LEN_BYTES - 1u] = (unsigned char)'m';
	if (init_random_number(&drng_seed, nonce_seed, SEED_LEN_BYTES) != 0
		|| init_random_number(&drng_msg, nonce_msg, SEED_LEN_BYTES) != 0)
	{
		fprintf(stderr, "failed to initialize KAT replay DRNGs\n");
		free(line);
		fclose(f);
		return 2;
	}

	vector_count = 0;
	skip_blank_lines(f, line, line_len, &have_line);
	while (have_line) {
		unsigned count;
		unsigned long long seed_len, pk_len, sk_len, m_len, sn_len;
		unsigned long long pk_replay_len, sk_replay_len, sn_replay_len;
		unsigned char *seed, *pk, *sk, *m, *sn;
		unsigned char *pk_replay, *sk_replay, *sn_replay, *m_replay;
		unsigned char seed_replay[SEED_LEN_BYTES];
		int rc;

		seed = pk = sk = m = sn = NULL;
		pk_replay = sk_replay = sn_replay = m_replay = NULL;
		if (!parse_count_line(line, &count) || count != vector_count) {
			fprintf(stderr, "invalid Count field at vector %u\n", vector_count);
			goto fail;
		}
		if (!read_len_and_hex(f, line, line_len, "Seed_Len", "Seed",
			&seed, &seed_len))
		{
			fprintf(stderr, "vector %u: invalid Seed field\n", vector_count);
			goto fail;
		}
		if (seed_len != SEED_LEN_BYTES) {
			fprintf(stderr, "vector %u: Seed_Len must be %u, got %llu\n",
				vector_count, SEED_LEN_BYTES, seed_len);
			goto fail;
		}
		if (get_random_number(&drng_seed, seed_replay, 8u * SEED_LEN_BYTES) != 0
			|| !constant_time_equal(seed, seed_replay, SEED_LEN_BYTES))
		{
			fprintf(stderr, "vector %u: Seed does not match KAT_SIG stream\n",
				vector_count);
			goto fail;
		}

		if (!read_len_and_hex(f, line, line_len, "PK_Len", "PK", &pk, &pk_len)
			|| !read_len_and_hex(f, line, line_len, "SK_Len", "SK", &sk, &sk_len)
			|| !read_len_and_hex(f, line, line_len, "M_Len", "M", &m, &m_len)
			|| !read_len_and_hex(f, line, line_len, "Sn_Len", "Sn", &sn, &sn_len))
		{
			fprintf(stderr, "vector %u: invalid key/message/signature field\n",
				vector_count);
			goto fail;
		}
		if (m_len != 56u + 8u * vector_count) {
			fprintf(stderr, "vector %u: unexpected M_Len %llu\n",
				vector_count, m_len);
			goto fail;
		}
		m_replay = (unsigned char *)malloc((size_t)m_len);
		if (m_replay == NULL
			|| get_random_number(&drng_msg, m_replay, 8u * m_len) != 0
			|| !constant_time_equal(m, m_replay, m_len))
		{
			fprintf(stderr, "vector %u: M does not match KAT_SIG stream\n",
				vector_count);
			goto fail;
		}

		pk_replay_len = sig_get_pk_len_bytes();
		sk_replay_len = sig_get_sk_len_bytes();
		sn_replay_len = sig_get_sn_len_bytes();
		pk_replay = (unsigned char *)calloc((size_t)pk_replay_len, 1u);
		sk_replay = (unsigned char *)calloc((size_t)sk_replay_len, 1u);
		sn_replay = (unsigned char *)calloc((size_t)sn_replay_len, 1u);
		if (pk_replay == NULL || sk_replay == NULL || sn_replay == NULL) {
			fprintf(stderr, "vector %u: allocation failed\n", vector_count);
			goto fail;
		}

		if (init_random_number(&drng_algorithm, seed, seed_len) != 0) {
			fprintf(stderr, "vector %u: failed to initialize scheme DRNG\n",
				vector_count);
			goto fail;
		}
		rc = sig_keygen(pk_replay, &pk_replay_len, sk_replay, &sk_replay_len);
		if (rc != 0) {
			fprintf(stderr, "vector %u: sig_keygen returned %d\n", vector_count, rc);
			goto fail;
		}
		if (!check_equal_field("PK", vector_count, pk, pk_len,
				pk_replay, pk_replay_len)
			|| !check_equal_field("SK", vector_count, sk, sk_len,
				sk_replay, sk_replay_len))
		{
			goto fail;
		}
		rc = sig_sign(sk_replay, sk_replay_len, m, m_len,
			sn_replay, &sn_replay_len);
		if (rc != 0) {
			fprintf(stderr, "vector %u: sig_sign returned %d\n", vector_count, rc);
			goto fail;
		}
		if (!check_equal_field("Sn", vector_count, sn, sn_len,
				sn_replay, sn_replay_len))
		{
			goto fail;
		}
		rc = sig_verify(pk, pk_len, sn, sn_len, m, m_len);
		if (rc != 0) {
			fprintf(stderr, "vector %u: sig_verify returned %d\n", vector_count, rc);
			goto fail;
		}

		free(m_replay);
		free(sn_replay);
		free(sk_replay);
		free(pk_replay);
		free(sn);
		free(m);
		free(sk);
		free(pk);
		free(seed);
		vector_count++;
		skip_blank_lines(f, line, line_len, &have_line);
		continue;

fail:
		free(m_replay);
		free(sn_replay);
		free(sk_replay);
		free(pk_replay);
		free(sn);
		free(m);
		free(sk);
		free(pk);
		free(seed);
		free(line);
		fclose(f);
		return 1;
	}

	free(line);
	fclose(f);
	if (vector_count != VECTOR_COUNT) {
		fprintf(stderr, "expected %u vectors, got %u\n",
			VECTOR_COUNT, vector_count);
		return 1;
	}
	printf("checked %u deterministic KAT vectors\n", vector_count);
	return 0;
}
