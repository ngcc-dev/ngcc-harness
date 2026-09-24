#ifndef RHYME_ENCODING_H
#define RHYME_ENCODING_H
#include <stdint.h>
#include <stddef.h>
#include "params.h"
#include "poly.h"

#define encode_z RHYME_NAMESPACE(encode_z)
#define decode_z RHYME_NAMESPACE(decode_z)
#define rhyme_encoding_init RHYME_NAMESPACE(encoding_init)

/* Compress (z1, z_rest) into buf (capacity cap).
 * Layout: [u16 rans_len][rans stream][raw low-bit stream].
 * Returns total bytes written, or 0 on overflow/error. */
size_t encode_z(uint8_t *buf, size_t cap, const poly *z1, const poly z_rest[D_REST]);

/* Decompress; len must be exact. Returns 0 ok, nonzero error. */
int decode_z(poly *z1, poly z_rest[D_REST], const uint8_t *buf, size_t len);

/* Build the rANS tables. One-time, idempotent. Called by keypair and verify so
 * the sign/verify measured path contains no table-construction cost. Safe to
 * call manually at startup. */
void rhyme_encoding_init(void);

#endif
