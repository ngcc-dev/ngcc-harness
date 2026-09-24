#ifndef RHYME_ZPNTT_H
#define RHYME_ZPNTT_H
#include <stdint.h>
#include "params.h"

#define zpntt_fwd RHYME_NAMESPACE(zpntt_fwd)
#define zpntt_inv_tomont RHYME_NAMESPACE(zpntt_inv_tomont)
#define zpntt_mont_mul RHYME_NAMESPACE(zpntt_mont_mul)
#define zpntt_prime RHYME_NAMESPACE(zpntt_prime)
#define zpntt_init RHYME_NAMESPACE(zpntt_init)

/* Exact negacyclic products over Z through one 31-bit prime (kg_prime(0)):
 * complete size-N NTT, Montgomery arithmetic, precomputed twiddles.
 * Convention mirrors the mod-q NTT: fwd keeps the standard domain,
 * pointwise zpntt_mont_mul carries an R^-1 deficit, inv_tomont restores it,
 * so fwd -> mont_mul -> inv_tomont yields the plain product mod p. */
void zpntt_init(void);                       /* idempotent, lazy-safe single thread */
uint32_t zpntt_prime(void);
void zpntt_fwd(uint32_t a[N]);
void zpntt_inv_tomont(uint32_t a[N]);
uint32_t zpntt_mont_mul(uint32_t a, uint32_t b);

#endif
