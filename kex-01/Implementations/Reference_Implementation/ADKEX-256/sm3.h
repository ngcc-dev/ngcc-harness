/*
 * SM3 Hash Function (GB/T 32905-2016)
 *
 * Independent module extracted from auxfunc.c / drng.c.
 * Provides core SM3 primitives and DKE-specific convenience wrappers.
 */
#ifndef DKE_SM3_H
#define DKE_SM3_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SM3 digest size */
#define SM3_DIGEST_BYTES  32
#define SM3_DIGEST_BITS   256
#define SM3_BLOCK_BYTES   64
#define SM3_BLOCK_BITS    512

/* Return codes */
#define SM3_SUCCESS                0
#define SM3_MEMORY_ALLOCATION_FAILED -2
#define SM3_HASH_FAILED            -3
#define SM3_INVALID_DIGESTBITLEN   -4
#define SM3_XOF_SUCCESS            0

/* ---- Core SM3 API ---- */

/*
 * Initialize SM3 state (8 x uint32 IV).
 */
void dke_sm3_init(unsigned int digest[8]);

/*
 * Compress one or more 64-byte blocks into the SM3 state.
 * @param digest  8-word running state (modified in place)
 * @param msg     pointer to message blocks (64 bytes each)
 * @param blocks  number of 64-byte blocks to process
 */
void dke_sm3_compress(unsigned int digest[8],
                      const unsigned char *msg,
                      unsigned long long blocks);

/*
 * Compute SM3 hash of a bit-length message.
 * Handles padding internally. Output is 32 bytes.
 * @param msg         input message bytes
 * @param msg_bitlen  message length in bits
 * @param dgst        output: 32-byte digest
 */
void dke_sm3_hash(const unsigned char *msg,
                  unsigned long long msg_bitlen,
                  unsigned char *dgst);

/*
 * Compute SM3 hash of a byte-aligned message (convenience wrapper).
 * @param msg       input message bytes
 * @param msg_bytes message length in bytes
 * @param dgst      output: 32-byte digest
 */
void dke_sm3_hash_bytes(const unsigned char *msg,
                        unsigned long long msg_bytes,
                        unsigned char *dgst);

/* ---- DKE-specific wrappers (use DKE_MODE parameters) ---- */

/*
 * HMAC-SM3 (GB/T 15852.2-2024 Section 7).
 * Key block size L1=512 bits, output L2=256 bits.
 * @param msg           input message
 * @param msg_len_bits  message length in bits
 * @param key           HMAC key
 * @param key_len_bits  key length in bits
 * @param mac           output: 32-byte MAC
 * @return 0 on success, negative on error
 */
int dke_sm3_hmac(const unsigned char *msg,
                 unsigned long long msg_len_bits,
                 const unsigned char *key,
                 unsigned long long key_len_bits,
                 unsigned char *mac);

/*
 * Normalize: clear unused bits in the last byte of a bit-string.
 */
void dke_sm3_normalize(unsigned char *input, unsigned long long total_bits);

#ifdef __cplusplus
}
#endif

#endif /* DKE_SM3_H */
