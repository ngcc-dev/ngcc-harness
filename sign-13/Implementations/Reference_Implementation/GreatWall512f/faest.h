#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "quicksilver.h"
#include "vole_check.h"
#include "vole_params.h"
#include "vector_com.h"
#include "vole_commit.h"


#if SECURITY_PARAM == 128
#define GREATWALL_SECRET_KEY_BITS 137
#elif SECURITY_PARAM == 192
#define GREATWALL_SECRET_KEY_BITS 197
#elif SECURITY_PARAM == 256
#define GREATWALL_SECRET_KEY_BITS 263
#elif SECURITY_PARAM == 512
#define GREATWALL_SECRET_KEY_BITS 521
#endif

#define GREATWALL_FIELD_BYTES ((GREATWALL_SECRET_KEY_BITS + 7) / 8)
#define FAEST_IV_BYTES GREATWALL_FIELD_BYTES

#define GREATWALL_SECRET_KEY_BYTES (GREATWALL_FIELD_BYTES + FAEST_IV_BYTES)
#define GREATWALL_PUBLIC_KEY_BYTES (GREATWALL_FIELD_BYTES + FAEST_IV_BYTES)

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0 && ZERO_BITS_IN_CHALLENGE_3 == 0
	#define COUNTER_BYTES 0
#else
	#define COUNTER_BYTES 4
#endif

#define FAEST_SIGNATURE_BYTES ( \
	VOLE_COMMIT_SIZE + \
	VOLE_CHECK_PROOF_BYTES + \
	WITNESS_BITS / 8 + \
	QUICKSILVER_PROOF_BYTES + \
	VECTOR_COM_OPEN_SIZE + \
	SECURITY_PARAM / 8 + \
	16 + COUNTER_BYTES)

bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed);

bool faest_sign(
	uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
	const uint8_t* random_seed, size_t random_seed_len);

bool faest_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed);
