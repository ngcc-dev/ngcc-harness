/*
 * QingLuan Digital Signature Scheme
 * utils.h - Utility functions
 */

#ifndef QINGLUAN_UTILS_H
#define QINGLUAN_UTILS_H

#include <stddef.h>
#include <stdint.h>

int randombytes(uint8_t *out, size_t len);

int drbg_init(void);

/*
 * Deterministically (re)initialize the DRBG from a caller-supplied seed.
 * Used for KAT reproducibility (e.g. the API_PKC 64-byte Seed). Subsequent
 * randombytes() output is a deterministic function of `seed`. len >= 32 advised.
 */
int drbg_seed(const uint8_t *seed, size_t len);

int ct_memcmp(const void *a, const void *b, size_t len);

void ct_cswap(uint8_t *a, uint8_t *b, size_t len, int condition);

void u32_to_be(uint8_t *out, uint32_t v);

uint32_t be_to_u32(const uint8_t *in);

void secure_zero(void *ptr, size_t len);

#endif /* QINGLUAN_UTILS_H */
