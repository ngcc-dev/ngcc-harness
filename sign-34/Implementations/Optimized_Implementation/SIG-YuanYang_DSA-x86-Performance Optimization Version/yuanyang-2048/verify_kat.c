/*
 * KAT verifier for YuanYang signature response files.  It parses each vector,
 * feeds PK, message and signature into sig_verify(), and fails on the first
 * malformed field or invalid signature.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"

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
parse_len_line(const char *line, const char *label, unsigned long long *len)
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

int
main(int argc, char **argv)
{
	FILE *f;
	char *line;
	size_t line_len;
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

	vector_count = 0;
	skip_blank_lines(f, line, line_len, &have_line);
	while (have_line) {
		unsigned long long pk_len, sk_len, m_len, sn_len, ignored_len;
		unsigned char *pk, *m, *sn;
		int rc;

		if (strncmp(line, "Count = ", 8) != 0) {
			fprintf(stderr, "expected Count line, got: %s\n", line);
			free(line);
			fclose(f);
			return 2;
		}

		if (!read_required_line(f, line, line_len)
			|| !parse_len_line(line, "Seed_Len", &ignored_len)
			|| !read_required_line(f, line, line_len))
		{
			fprintf(stderr, "invalid Seed field\n");
			free(line);
			fclose(f);
			return 2;
		}

		if (!read_required_line(f, line, line_len)
			|| !parse_len_line(line, "PK_Len", &pk_len))
		{
			fprintf(stderr, "invalid PK_Len field\n");
			free(line);
			fclose(f);
			return 2;
		}
		pk = (unsigned char *)malloc((size_t)pk_len);
		if (pk == NULL || !read_required_line(f, line, line_len)
			|| !parse_hex_line(line, "PK", pk, pk_len))
		{
			fprintf(stderr, "invalid PK field\n");
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}

		if (!read_required_line(f, line, line_len)
			|| !parse_len_line(line, "SK_Len", &sk_len)
			|| !read_required_line(f, line, line_len))
		{
			fprintf(stderr, "invalid SK field\n");
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}
		(void)sk_len;

		if (!read_required_line(f, line, line_len)
			|| !parse_len_line(line, "M_Len", &m_len))
		{
			fprintf(stderr, "invalid M_Len field\n");
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}
		m = (unsigned char *)malloc((size_t)m_len);
		if (m == NULL || !read_required_line(f, line, line_len)
			|| !parse_hex_line(line, "M", m, m_len))
		{
			fprintf(stderr, "invalid M field\n");
			free(m);
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}

		if (!read_required_line(f, line, line_len)
			|| !parse_len_line(line, "Sn_Len", &sn_len))
		{
			fprintf(stderr, "invalid Sn_Len field\n");
			free(m);
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}
		sn = (unsigned char *)malloc((size_t)sn_len);
		if (sn == NULL || !read_required_line(f, line, line_len)
			|| !parse_hex_line(line, "Sn", sn, sn_len))
		{
			fprintf(stderr, "invalid Sn field\n");
			free(sn);
			free(m);
			free(pk);
			free(line);
			fclose(f);
			return 2;
		}

		rc = sig_verify(pk, pk_len, sn, sn_len, m, m_len);
		free(sn);
		free(m);
		free(pk);
		if (rc != 0) {
			fprintf(stderr, "vector %u failed with rc=%d\n", vector_count, rc);
			free(line);
			fclose(f);
			return 1;
		}
		vector_count++;
		skip_blank_lines(f, line, line_len, &have_line);
	}

	printf("verified %u vectors\n", vector_count);
	free(line);
	fclose(f);
	return 0;
}
