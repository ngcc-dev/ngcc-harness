#ifndef DKE_RANDOM_SAMPLING_H
#define DKE_RANDOM_SAMPLING_H

#include "parameters.h"
#include "polyvec.h"
#include "poly.h"

void DKE_getsecretA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_geterrorA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_getsecretB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_geterrorB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);

/* DKE_HASH_SHAKE: defined when SHAKE-based operations are available (HASH=1 or HASH=2) */

#if defined(DKE_USE_OPT_C) && defined(DKE_HASH_SHAKE)
void DKE_getnoise_2x(poly *r0, poly *r1,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1);
#endif

#if (defined(DKE_USE_AVX2) || defined(DKE_USE_OPT_C)) && defined(DKE_HASH_SHAKE)
void DKE_getnoise_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3);

void DKE_getnoise_5x(poly *r0, poly *r1, poly *r2, poly *r3, poly *r4,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3, uint8_t nonce4);
#endif


#if defined(DKE_USE_AVX2) && defined(DKE_HASH_SHAKE)
#include "fips202.h"
#define DKE_NOISE_STATE_BYTES (sizeof(keccak_state))
void DKE_getnoise_absorb(keccak_state *state,
                          const uint8_t seed[DKE_SEEDBYTES],
                          uint8_t nonce);
void DKE_getnoise_squeeze(poly *r, keccak_state *state);
#endif

unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen);

#define gen_a(A,B)  DKE_gen_matrix(A,B,0)
#define gen_at(A,B) DKE_gen_matrix(A,B,1)
void DKE_gen_matrix(polyvec *res,
                    const uint8_t seed[DKE_SEEDBYTES],
                    const int transposed);

#endif
