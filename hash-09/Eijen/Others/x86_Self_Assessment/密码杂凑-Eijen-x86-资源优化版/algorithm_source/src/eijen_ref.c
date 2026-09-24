#include "eijen.h"
#include "eijen_internal.h"

#include <string.h>

const char *eijen_implementation_name(void) {
    return "Eijen x86 reference C99";
}

/* Look up the fixed parameter set for a supported digest length. */
const eijen_config *eijen_get_config(int digest_bits) {
    return eijen_config_by_bits(digest_bits);
}

/* Return the digest size in bytes, or zero for an unsupported instance. */
size_t eijen_digest_size(int digest_bits) {
    const eijen_config *cfg = eijen_get_config(digest_bits);
    return cfg == NULL ? 0U : (size_t)cfg->digest_bytes;
}

/* Return the absorbing rate in bytes, or zero for an unsupported instance. */
size_t eijen_rate_size(int digest_bits) {
    const eijen_config *cfg = eijen_get_config(digest_bits);
    return cfg == NULL ? 0U : (size_t)cfg->rate_bytes;
}

/* Apply the ARX MixQuad operation to one four-word column. */
static void eijen_mixquad(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d,
                          int r0, int r1, int r2, int r3, int r4, int r5,
                          uint64_t rc) {
    *a = *a + *b;
    *d ^= *a;
    *d = eijen_rotl64(*d, (unsigned int)r0);
    *a = eijen_rotl64(*a, (unsigned int)r4) ^ rc;
    *c = *c + *d;
    *b ^= *c;
    *b = eijen_rotl64(*b, (unsigned int)r1);
    *c = eijen_rotl64(*c, (unsigned int)r5);
    *a = *a + *b;
    *d ^= *a;
    *d = eijen_rotl64(*d, (unsigned int)r2);
    *c = *c + *d;
    *b ^= *c;
    *b = eijen_rotl64(*b, (unsigned int)r3);
}

/* Apply MixQuad to all eight columns with the two round constants. */
static void eijen_columns(uint64_t s[EIJEN_STATE_WORDS], uint64_t rc0,
                          uint64_t rc1) {
    int i;
    for (i = 0; i < 16; i += 4) {
        eijen_mixquad(&s[i], &s[i + 1], &s[i + 2], &s[i + 3],
                      36, 54, 45, 44, 18, 9, rc0);
    }
    for (i = 16; i < 32; i += 4) {
        eijen_mixquad(&s[i], &s[i + 1], &s[i + 2], &s[i + 3],
                      56, 46, 54, 34, 19, 51, rc1);
    }
}

/* Apply the word-position permutation by its cycle decomposition. */
static void eijen_sigma(uint64_t s[EIJEN_STATE_WORDS]) {
    uint64_t t;
    t = s[0]; s[0] = s[22]; s[22] = s[17]; s[17] = s[16]; s[16] = s[6]; s[6] = s[1]; s[1] = t;
    t = s[2]; s[2] = s[29]; s[29] = s[28]; s[28] = s[18]; s[18] = s[13]; s[13] = s[12]; s[12] = t;
    t = s[3]; s[3] = s[19]; s[19] = t;
    t = s[4]; s[4] = s[26]; s[26] = s[21]; s[21] = s[20]; s[20] = s[10]; s[10] = s[5]; s[5] = t;
    t = s[7]; s[7] = s[23]; s[23] = t;
    t = s[8]; s[8] = s[30]; s[30] = s[25]; s[25] = s[24]; s[24] = s[14]; s[14] = s[9]; s[9] = t;
    t = s[11]; s[11] = s[27]; s[27] = t;
    t = s[15]; s[15] = s[31]; s[31] = t;
}

/* Apply the four-word XOR mixing layer independently to each column. */
static void eijen_linear_layer(uint64_t s[EIJEN_STATE_WORDS]) {
    int col;
    for (col = 0; col < 8; col++) {
        int b = col * 4;
        uint64_t x = s[b] ^ s[b + 1] ^ s[b + 2] ^ s[b + 3];
        s[b] ^= x;
        s[b + 1] ^= x;
        s[b + 2] ^= x;
        s[b + 3] ^= x;
    }
}

/* Evaluate the full 16-round Eijen permutation. */
static void eijen_permutation(uint64_t state[EIJEN_STATE_WORDS]) {
    int r;
    for (r = 0; r < EIJEN_ROUNDS; r++) {
        eijen_columns(state, EIJEN_RC0[r], EIJEN_RC1[r]);
        eijen_sigma(state);
        eijen_linear_layer(state);
        eijen_sigma(state);
    }
}

/* Absorb one complete rate block and apply capacity feed-forward. */
static void eijen_absorb_block(eijen_ctx *ctx, const uint8_t *block, int is_last) {
    uint64_t saved_cap[17];
    int i;
    int cap_start = ctx->cfg->rate_words;
    int cap_words = ctx->cfg->capacity_words;

    for (i = 0; i < cap_words; i++) {
        saved_cap[i] = ctx->state[cap_start + i];
    }
    for (i = 0; i < ctx->cfg->rate_words; i++) {
        ctx->state[i] ^= eijen_load64_le(block + ((size_t)i * 8U));
    }
    if (is_last) {
        ctx->state[EIJEN_STATE_WORDS - 1] ^= (UINT64_C(1) << 63);
    }

    eijen_permutation(ctx->state);

    for (i = 0; i < cap_words; i++) {
        ctx->state[cap_start + i] ^= saved_cap[i];
    }
}

/* Initialize the byte-streaming hash context. */
int eijen_init(eijen_ctx *ctx, int digest_bits) {
    const eijen_config *cfg;
    if (ctx == NULL) {
        return -1;
    }
    cfg = eijen_get_config(digest_bits);
    if (cfg == NULL) {
        return -1;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = cfg;
    return 0;
}

/* Absorb a byte-aligned fragment into the streaming hash context. */
int eijen_update(eijen_ctx *ctx, const uint8_t *data, size_t len) {
    size_t rb;
    if (ctx == NULL || ctx->cfg == NULL || (data == NULL && len != 0U)) {
        return -1;
    }
    rb = (size_t)ctx->cfg->rate_bytes;
    while (len > 0U) {
        size_t space = rb - ctx->buf_len;
        size_t take = len < space ? len : space;
        if (ctx->buf_len == 0U && len > rb) {
            eijen_absorb_block(ctx, data, 0);
            data += rb;
            len -= rb;
            continue;
        }
        memcpy(ctx->buf + ctx->buf_len, data, take);
        ctx->buf_len += take;
        data += take;
        len -= take;
        if (ctx->buf_len == rb && len > 0U) {
            eijen_absorb_block(ctx, ctx->buf, 0);
            ctx->buf_len = 0U;
        }
    }
    return 0;
}

/* Finalize the context, optionally including a partial MSB-first final byte. */
static int eijen_final_partial(eijen_ctx *ctx, uint8_t partial_byte,
                               unsigned int partial_bits, uint8_t *digest) {
    int i;
    int ds;
    size_t rb;
    if (ctx == NULL || ctx->cfg == NULL || digest == NULL || partial_bits > 7U) {
        return -1;
    }
    rb = (size_t)ctx->cfg->rate_bytes;
    if (ctx->buf_len == rb) {
        eijen_absorb_block(ctx, ctx->buf, 0);
        ctx->buf_len = 0U;
    }
    if (partial_bits == 0U) {
        ctx->buf[ctx->buf_len] = 0x01U;
    } else {
        uint8_t keep = (uint8_t)(0xffU << (8U - partial_bits));
        uint8_t pad = (uint8_t)(0x80U >> partial_bits);
        ctx->buf[ctx->buf_len] = (uint8_t)((partial_byte & keep) | pad);
    }
    ctx->buf_len++;
    memset(ctx->buf + ctx->buf_len, 0, rb - ctx->buf_len);
    eijen_absorb_block(ctx, ctx->buf, 1);

    ds = EIJEN_STATE_WORDS - ctx->cfg->digest_words;
    for (i = 0; i < ctx->cfg->digest_words; i++) {
        eijen_store64_le(digest + ((size_t)i * 8U), ctx->state[ds + i]);
    }
    return 0;
}

/* Finalize a byte-aligned message. */
int eijen_final(eijen_ctx *ctx, uint8_t *digest) {
    return eijen_final_partial(ctx, 0U, 0U, digest);
}

/* Hash a byte-aligned message in one call. */
int eijen_hash(int digest_bits, const uint8_t *msg, size_t len, uint8_t *digest) {
    eijen_ctx ctx;
    if (eijen_init(&ctx, digest_bits) != 0) {
        return -1;
    }
    if (eijen_update(&ctx, msg, len) != 0) {
        return -1;
    }
    return eijen_final(&ctx, digest);
}

/* Hash a bit-length message using the API_CryptHash partial-byte convention. */
int eijen_hash_bits(int digest_bits, const uint8_t *msg,
                    unsigned long long msg_len_bits, uint8_t *digest) {
    eijen_ctx ctx;
    unsigned long long full_bytes = msg_len_bits / 8ULL;
    unsigned int partial_bits = (unsigned int)(msg_len_bits & 7ULL);
    uint8_t partial_byte = 0U;

    if ((msg == NULL && msg_len_bits != 0ULL) || digest == NULL) {
        return -1;
    }
    if (full_bytes > (unsigned long long)((size_t)-1)) {
        return -1;
    }
    if (eijen_init(&ctx, digest_bits) != 0) {
        return -1;
    }
    if (eijen_update(&ctx, msg, (size_t)full_bytes) != 0) {
        return -1;
    }
    if (partial_bits != 0U) {
        partial_byte = msg[full_bytes];
    }
    return eijen_final_partial(&ctx, partial_byte, partial_bits, digest);
}
