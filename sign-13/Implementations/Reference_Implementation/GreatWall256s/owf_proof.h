#ifndef OWF_PROOF_H
#define OWF_PROOF_H

#define OWF_KEY_SCHEDULE_CONSTRAINTS 0
#define OWF_BLOCKS 1
#define OWF_ROUNDS 2

#if SECURITY_PARAM == 128
	typedef block137 owf_block;
	#define OWF_KEY_WITNESS_BITS 144
	#define OWF_BLOCK_SIZE 18
	#define OWF_CONSTRAINTS_PER_ROUND 2

#elif SECURITY_PARAM == 192
	typedef block197 owf_block;
	#define OWF_KEY_WITNESS_BITS 200
	#define OWF_BLOCK_SIZE 25
	#define OWF_CONSTRAINTS_PER_ROUND 2

#elif SECURITY_PARAM == 256
	typedef block263 owf_block;
	#define OWF_KEY_WITNESS_BITS 264
	#define OWF_BLOCK_SIZE 33
	#define OWF_CONSTRAINTS_PER_ROUND 2

#elif SECURITY_PARAM == 512
	typedef block521 owf_block;
	#define OWF_KEY_WITNESS_BITS 528
	#define OWF_BLOCK_SIZE 66
	#define OWF_CONSTRAINTS_PER_ROUND 2

#endif

#define OWF_ALPHA_CONSTRAINTS 2
#define OWF_NUM_CONSTRAINTS (OWF_BLOCKS * OWF_CONSTRAINTS_PER_ROUND * OWF_ROUNDS + OWF_KEY_SCHEDULE_CONSTRAINTS + OWF_ALPHA_CONSTRAINTS)
#define WITNESS_BITS (8 * OWF_BLOCKS * OWF_BLOCK_SIZE * (OWF_ROUNDS - 1) + OWF_KEY_WITNESS_BITS)

#include "aes.h"
#include "greatwall.h"
#include "quicksilver.h"

struct public_key;
typedef struct public_key public_key;

void owf_constraints_prover(quicksilver_state* state, const public_key* pk);
void owf_constraints_verifier(quicksilver_state* state, const public_key* pk);

#endif
