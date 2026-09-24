#ifndef DKE_RANDOM_SAMPLING_H
#define DKE_RANDOM_SAMPLING_H

#include "parameters.h"
#include "polyvec.h"
#include "poly.h"

void DKE_getsecretA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_geterrorA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_getsecretB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);
void DKE_geterrorB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce);

/* DKE_HASH_SHAKE: defined when the SHAKE/SHA3 (ML-KEM-suite, HASH=2) backend is selected */

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

/* ── DKE-512 Cortex-M4 matacc XOF bridge ───────────────────────────────────
 * Non-static wrappers called by plantard512_matacc_asm.S via `bl`.
 * The state is an opaque blob; callers must use DKE3_XOF_STATE_BYTES.      */
#if DKE_N == 512 && defined(DKE_USE_CORTEX_M4_PLANTARD)
#define DKE3_XOF_STATE_BYTES 128   /* generous bound for all XOF backends */
void dke3_xof_absorb(uint8_t state[DKE3_XOF_STATE_BYTES],
                     const uint8_t seed[DKE_SEEDBYTES],
                     uint8_t x, uint8_t y);
void dke3_xof_squeezeblocks(uint8_t *out, size_t nblocks,
                             uint8_t state[DKE3_XOF_STATE_BYTES]);
#endif

#define gen_a(A,B)  DKE_gen_matrix(A,B,0)
#define gen_at(A,B) DKE_gen_matrix(A,B,1)
void DKE_gen_matrix(polyvec *res,
                    const uint8_t seed[DKE_SEEDBYTES],
                    const int transposed);

#endif
