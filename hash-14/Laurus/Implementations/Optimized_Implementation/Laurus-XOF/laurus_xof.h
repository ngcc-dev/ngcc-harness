#include <stdint.h>

typedef struct {
	uint64_t A[25];
	uint64_t S[24];
	uint64_t sec;
	uint64_t pos;
	uint64_t counter;
} laurus_state;

void laurus_xof_init(laurus_state* state);
void laurus_xof_absorb(laurus_state* state, const uint8_t* in, uint64_t inlen);
void laurus_xof_finalize(laurus_state* state);
void laurus_xof_squeezeblocks(uint8_t* out, int nblocks, laurus_state* state);
void laurus_xof_squeeze(uint8_t* out, uint64_t outlen, laurus_state* state);
void laurus_xof(uint8_t* out, uint64_t outlen, const uint8_t* in, uint64_t inlen);