#ifndef RHYME_KG_NTT_H
#define RHYME_KG_NTT_H
#include <stdint.h>
uint32_t kg_prime(unsigned idx);
unsigned kg_nprimes(void);
uint32_t kg_powmod(uint32_t a, uint32_t e, uint32_t p);
uint32_t kg_psi(unsigned pidx, unsigned n);
void kg_ntt_fwd(uint32_t *a, unsigned n, unsigned pidx);
void kg_ntt_inv(uint32_t *a, unsigned n, unsigned pidx);
#endif
