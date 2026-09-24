#ifndef VOLE_CHECK_H
#define VOLE_CHECK_H

#include <inttypes.h>
#include "universal_hash.h"

#define VOLE_CHECK_HASH_BYTES (SECURITY_PARAM / 8 + 2)
#define VOLE_CHECK_CHALLENGE_BYTES ((5 * SECURITY_PARAM + 64) / 8)
#define VOLE_CHECK_PROOF_BYTES VOLE_CHECK_HASH_BYTES
#define VOLE_CHECK_CHECK_BYTES (2 * SECURITY_PARAM / 8)

void vole_check_sender(
	const vole_block* restrict u, const vole_block* restrict v,
	const uint8_t* restrict challenge, uint8_t* restrict proof, uint8_t* restrict check);

void vole_check_receiver(
	const vole_block* restrict q, const uint8_t* restrict delta_bytes,
	const uint8_t* restrict challenge, const uint8_t* restrict proof, uint8_t* restrict check);


#endif
