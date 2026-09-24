/**
 * @file compression.h
 * @brief HARE ciphertext-compression interface.
 *
 * The KR [51,41] covering-code compression backend is used in this package. PKE/KEM code calls only this backend-neutral interface for c = (u, m_tilde, v2).
 */

#ifndef HARE_COMPRESSION_H
#define HARE_COMPRESSION_H

#include <stdint.h>

/**
 * Compress the first PARAM_L1 bits of v into m_tilde and v2.
 */
void ciphertext_compress(uint64_t *m_tilde, uint64_t *v2, const uint64_t *v_vec);

/**
 * Reconstruct the PARAM_L1-bit compressed payload and clear the tail of v.
 */
void ciphertext_decompress(uint64_t *v_vec, const uint64_t *m_tilde, const uint64_t *v2);

#endif /* HARE_COMPRESSION_H */
