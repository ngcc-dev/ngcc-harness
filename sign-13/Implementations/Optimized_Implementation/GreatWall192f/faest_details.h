#ifndef FAEST_DETAILS_H
#define FAEST_DETAILS_H

#include "aes.h"
#include "block.h"
#include "owf_proof.h"
#include "vole_params.h"

typedef struct public_key
{
	owf_block owf_output;
	owf_block owf_iv;
} public_key;

typedef struct
{
	public_key pk;
	owf_block sk;
	vole_block witness[WITNESS_BLOCKS];
} secret_key;

void faest_free_public_key(public_key* pk);
void faest_free_secret_key(secret_key* sk);

bool faest_unpack_secret_key(secret_key* unpacked, const uint8_t* packed);
void faest_pack_public_key(uint8_t* packed, const public_key* unpacked);
void faest_unpack_public_key(public_key* unpacked, const uint8_t* packed);
bool faest_compute_witness(secret_key* sk);
bool faest_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed, secret_key* sk);

#endif
