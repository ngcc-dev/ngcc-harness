#ifndef INDCPA_H
#define INDCPA_H

#include "params.h"
#include "polyvec.h"
#include <stdint.h>
#include <stdio.h>

#define gen_matrix KEM_NAMESPACE (gen_matrix)
void gen_matrix (polyvec *a, const uint8_t seed[KEM_SYMBYTES], int transposed);
#define indcpa_keypair KEM_NAMESPACE (indcpa_keypair)
void indcpa_keypair (uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
										 uint8_t sk[KEM_INDCPA_SECRETKEYBYTES]);

void indcpa_keypair_print (uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
													 uint8_t sk[KEM_INDCPA_SECRETKEYBYTES]);

void indcpa_keypair_file (FILE *file, uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
													uint8_t sk[KEM_INDCPA_SECRETKEYBYTES]);

#define indcpa_enc KEM_NAMESPACE (indcpa_enc)
void indcpa_enc (uint8_t c[KEM_INDCPA_BYTES],
								 const uint8_t m[KEM_INDCPA_MSGBYTES],
								 const uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
								 const uint8_t coins[KEM_SYMBYTES]);
void indcpa_enc_file (uint8_t c[KEM_INDCPA_BYTES],
											const uint8_t m[KEM_INDCPA_MSGBYTES],
											const uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
											const uint8_t coins[KEM_SYMBYTES]);
void indcpa_enc_input_print (uint8_t c[KEM_INDCPA_BYTES],
														 const uint8_t m[KEM_INDCPA_MSGBYTES],
														 const uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
														 const uint8_t coins[KEM_SYMBYTES]);

#define indcpa_dec KEM_NAMESPACE (indcpa_dec)
void indcpa_dec (uint8_t m[KEM_INDCPA_MSGBYTES],
								 const uint8_t c[KEM_INDCPA_BYTES],
								 const uint8_t sk[KEM_INDCPA_SECRETKEYBYTES]);

void indcpa_dec_file (uint8_t m[KEM_INDCPA_MSGBYTES],
											const uint8_t c[KEM_INDCPA_BYTES],
											const uint8_t sk[KEM_INDCPA_SECRETKEYBYTES]);

// Packing functions, public for testing
void pack_pk (uint8_t r[KEM_INDCPA_PUBLICKEYBYTES], polyvec *pk,
							const uint8_t seed[KEM_SYMBYTES]);
void unpack_pk (polyvec *pk, uint8_t seed[KEM_SYMBYTES],
								const uint8_t packedpk[KEM_INDCPA_PUBLICKEYBYTES]);
void pack_sk (uint8_t r[KEM_INDCPA_SECRETKEYBYTES], polyvec *sk);
void unpack_sk (polyvec *sk,
								const uint8_t packedsk[KEM_INDCPA_SECRETKEYBYTES]);
void unpack_ciphertext (polyvec *b, poly *v,
												const uint8_t c[KEM_INDCPA_BYTES]);
void pack_ciphertext (uint8_t r[KEM_INDCPA_BYTES], polyvec *b, poly *c);

#endif
