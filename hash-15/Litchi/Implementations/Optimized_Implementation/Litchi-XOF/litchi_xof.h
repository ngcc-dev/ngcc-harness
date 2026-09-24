#include <stdint.h>

typedef struct {
	uint64_t A[25];
	uint64_t S[25];
	uint64_t sec;
	uint64_t pos;
	uint64_t counter;
} litchi_state;


void litchi_xof_init(litchi_state* state);
void litchi_xof_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen);
void litchi_xof_finalize(litchi_state* state);
void litchi_xof_squeezeblocks(uint8_t* out, int nblocks, litchi_state* state);
void litchi_xof_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state);
void litchi_xof(uint8_t* out, uint64_t outlen, const uint8_t* in, uint64_t inlen);


void litchi_512_init(litchi_state* state);
void litchi_512_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen);
void litchi_512_finalize(litchi_state* state);
void litchi_512_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state);
void litchi_512(uint8_t h[64], const uint8_t* in, uint64_t inlen);


void litchi_768_init(litchi_state* state);
void litchi_768_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen);
void litchi_768_finalize(litchi_state* state);
void litchi_768_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state);
void litchi_768(uint8_t h[96], const uint8_t* in, uint64_t inlen);


void litchi_1024_init(litchi_state* state);
void litchi_1024_absorb(litchi_state* state, const uint8_t* in, uint64_t inlen);
void litchi_1024_finalize(litchi_state* state);
void litchi_1024_squeeze(uint8_t* out, uint64_t outlen, litchi_state* state);
void litchi_1024(uint8_t h[128], const uint8_t* in, uint64_t inlen);