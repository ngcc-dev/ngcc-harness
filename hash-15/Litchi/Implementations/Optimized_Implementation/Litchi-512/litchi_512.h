#include <stdint.h>

typedef struct {
	uint64_t A[25];
	uint64_t S[25];
	uint64_t sec;
	uint64_t pos;
	uint64_t counter;
} litchi_state;


void litchi_512_init(litchi_state* state);
void litchi_512_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen);
void litchi_512_finalize(litchi_state* state);
void litchi_512_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state);
void litchi_512(uint8_t h[64], const uint8_t* in, uint64_t inlen);

