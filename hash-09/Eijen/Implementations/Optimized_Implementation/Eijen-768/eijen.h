#ifndef EIJEN_H
#define EIJEN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EIJEN_STATE_WORDS 32
#define EIJEN_MAX_RATE_BYTES 216
#define EIJEN_MAX_DIGEST_BYTES 128

typedef struct {
    int digest_bits;
    int rate_words;
    int capacity_words;
    int digest_words;
    int rate_bytes;
    int digest_bytes;
} eijen_config;

typedef struct {
    uint64_t state[EIJEN_STATE_WORDS];
    uint8_t buf[EIJEN_MAX_RATE_BYTES];
    size_t buf_len;
    const eijen_config *cfg;
} eijen_ctx;

const char *eijen_implementation_name(void);

/* Return the parameter record for a supported digest length. */
const eijen_config *eijen_get_config(int digest_bits);

/* Return the digest length in bytes for a supported digest length. */
size_t eijen_digest_size(int digest_bits);

/* Return the absorbing rate in bytes for a supported digest length. */
size_t eijen_rate_size(int digest_bits);

/* Initialize a streaming hash context for Eijen-digest_bits. */
int eijen_init(eijen_ctx *ctx, int digest_bits);

/* Absorb a byte-aligned message fragment into an initialized context. */
int eijen_update(eijen_ctx *ctx, const uint8_t *data, size_t len);

/* Apply padding, process the final block, and write the digest. */
int eijen_final(eijen_ctx *ctx, uint8_t *digest);

/* Hash a byte-aligned message in one call. */
int eijen_hash(int digest_bits, const uint8_t *msg, size_t len, uint8_t *digest);

/* Hash a bit-length message using MSB-first interpretation of a partial byte. */
int eijen_hash_bits(int digest_bits, const uint8_t *msg,
                    unsigned long long msg_len_bits, uint8_t *digest);

#ifdef __AVX2__
int eijen_hash4_same(int digest_bits,
                     const uint8_t *msg0, const uint8_t *msg1,
                     const uint8_t *msg2, const uint8_t *msg3,
                     size_t len,
                     uint8_t *out0, uint8_t *out1,
                     uint8_t *out2, uint8_t *out3);
#endif

#ifdef __cplusplus
}
#endif

#endif
