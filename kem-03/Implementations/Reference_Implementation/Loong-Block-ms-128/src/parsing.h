#ifndef LOONG_PARSING_H
#define LOONG_PARSING_H

#include "loong_parameters.h"

typedef struct {
	const unsigned char *seed;
	unsigned long long seed_len_bytes;
	const unsigned char *s;
	unsigned long long s_len_bytes;
} loong_public_key_view;

typedef struct {
	const unsigned char *seed;
	unsigned long long seed_len_bytes;
	const unsigned char *x;
	unsigned long long x_len_bytes;
	const unsigned char *pk;
	unsigned long long pk_len_bytes;
	const unsigned char *xi;
	unsigned long long xi_len_bytes;
} loong_secret_key_view;

typedef struct {
	const unsigned char *core;
	unsigned long long core_len_bytes;
	unsigned long long c1_bit_offset;
	unsigned long long c1_bit_len;
	unsigned long long c2_bit_offset;
	unsigned long long c2_bit_len;
	const unsigned char *salt;
	unsigned long long salt_len_bytes;
} loong_ciphertext_view;

#define LOONG_PK_SEED_OFFSET 0ULL
#define LOONG_PK_S_OFFSET ((unsigned long long)LOONG_PK_SEED_BYTES)

#define LOONG_SK_SEED_OFFSET 0ULL
#define LOONG_SK_X_OFFSET ((unsigned long long)LOONG_PK_SEED_BYTES)
#define LOONG_SK_PK_OFFSET (LOONG_SK_X_OFFSET + (unsigned long long)LOONG_X_OR_S_BYTES)
#define LOONG_SK_XI_OFFSET (LOONG_SK_PK_OFFSET + (unsigned long long)LOONG_PK_BYTES)

#define LOONG_CT_CORE_OFFSET 0ULL
#define LOONG_CT_SALT_OFFSET ((unsigned long long)LOONG_PKE_CT_BYTES)

#define LOONG_C1_BITS ((unsigned long long)LOONG_N * LOONG_N2 * LOONG_M)
#define LOONG_C2_BITS ((unsigned long long)LOONG_N1 * LOONG_N2 * LOONG_M)

int loong_public_key_parse(loong_public_key_view *view, const unsigned char *pk,
                           unsigned long long pk_len_bytes);
int loong_secret_key_parse(loong_secret_key_view *view, const unsigned char *sk,
                           unsigned long long sk_len_bytes);
int loong_ciphertext_parse(loong_ciphertext_view *view,
                           const unsigned char *ct,
                           unsigned long long ct_len_bytes);

int loong_public_key_encode(unsigned char *pk, unsigned long long pk_len_bytes,
                            const unsigned char *seed,
                            unsigned long long seed_len_bytes,
                            const unsigned char *s,
                            unsigned long long s_len_bytes);
int loong_secret_key_encode(unsigned char *sk, unsigned long long sk_len_bytes,
                            const unsigned char *seed,
                            unsigned long long seed_len_bytes,
                            const unsigned char *x,
                            unsigned long long x_len_bytes,
                            const unsigned char *pk,
                            unsigned long long pk_len_bytes,
                            const unsigned char *xi,
                            unsigned long long xi_len_bytes);
int loong_ciphertext_encode(unsigned char *ct, unsigned long long ct_len_bytes,
                            const unsigned char *core,
                            unsigned long long core_len_bytes,
                            const unsigned char *salt,
                            unsigned long long salt_len_bytes);

#endif
