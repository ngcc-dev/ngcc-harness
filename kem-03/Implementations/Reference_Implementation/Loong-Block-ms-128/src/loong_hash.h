#ifndef LOONG_HASH_H
#define LOONG_HASH_H

#include <stddef.h>

typedef struct {
	const unsigned char *data;
	unsigned long long len_bytes;
} loong_hash_input;

int loong_hash_tagged_sm3_256(unsigned char out[32], const char *tag,
                              const loong_hash_input *inputs,
                              size_t input_count);
int loong_hash_tagged_pseudohash_512(unsigned char out[64], const char *tag,
                                     const loong_hash_input *inputs,
                                     size_t input_count);
int loong_hash_tagged_xof(unsigned char *out, unsigned long long out_len_bits,
                          const char *tag, const loong_hash_input *inputs,
                          size_t input_count);

int loong_hash_id(unsigned char id_pk[32], const unsigned char *pk,
                  unsigned long long pk_len_bytes);
int loong_hash_g(unsigned char out[64], const unsigned char *id_pk,
                 unsigned long long id_pk_len_bytes, const unsigned char *m,
                 unsigned long long m_len_bytes, const unsigned char *salt,
                 unsigned long long salt_len_bytes);
int loong_hash_kdf(unsigned char *ss, unsigned long long ss_len_bytes,
                   const unsigned char *secret,
                   unsigned long long secret_len_bytes,
                   const unsigned char *ct_core,
                   unsigned long long ct_core_len_bytes);
int loong_xof_expand_public_seed(unsigned char *out,
                                 unsigned long long out_len_bits,
                                 const unsigned char *seed,
                                 unsigned long long seed_len_bytes);
int loong_xof_expand_keygen_seed(unsigned char *out,
                                 unsigned long long out_len_bits,
                                 const unsigned char *seed,
                                 unsigned long long seed_len_bytes);
int loong_xof_expand_enc_seed(unsigned char *out,
                              unsigned long long out_len_bits,
                              const unsigned char *theta,
                              unsigned long long theta_len_bytes);

#endif
