#include <stdint.h>

typedef struct {
	uint64_t A[25];
	uint64_t S[24];
	uint64_t sec;
	uint64_t pos;
	uint64_t counter;
} laurus_state;

void laurus_768_init(laurus_state* state);
void laurus_768_absorb(laurus_state* state, const uint8_t* in, uint64_t inlen);
void laurus_768_finalize(laurus_state* state);
void laurus_768_squeeze(uint8_t* out, uint64_t outlen, laurus_state* state);
void laurus_768(uint8_t h[96], const uint8_t* in, uint64_t inlen);