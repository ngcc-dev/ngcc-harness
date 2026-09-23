#ifndef LOONG_PKE_H
#define LOONG_PKE_H

#include "loong_parameters.h"

unsigned long long loong_pke_get_message_len_bytes(void);

int loong_pke_keygen_derandomized(unsigned char *pk,
                                  unsigned long long pk_len_bytes,
                                  unsigned char *sk_prime,
                                  unsigned long long sk_prime_len_bytes,
                                  const unsigned char *public_seed,
                                  unsigned long long public_seed_len_bytes,
                                  const unsigned char *keygen_seed,
                                  unsigned long long keygen_seed_len_bytes);

int loong_pke_encrypt_derandomized(unsigned char *ct_core,
                                   unsigned long long ct_core_len_bytes,
                                   const unsigned char *pk,
                                   unsigned long long pk_len_bytes,
                                   const unsigned char *message,
                                   unsigned long long message_len_bytes,
                                   const unsigned char *theta,
                                   unsigned long long theta_len_bytes);

int loong_pke_decrypt(unsigned char *message,
                      unsigned long long message_len_bytes,
                      const unsigned char *sk_prime,
                      unsigned long long sk_prime_len_bytes,
                      const unsigned char *ct_core,
                      unsigned long long ct_core_len_bytes);

int loong_pke_decrypt_with_residual_rank(
	unsigned char *message, unsigned long long message_len_bytes,
	unsigned int *residual_rank, const unsigned char *sk_prime,
	unsigned long long sk_prime_len_bytes, const unsigned char *ct_core,
	unsigned long long ct_core_len_bytes);

#endif
