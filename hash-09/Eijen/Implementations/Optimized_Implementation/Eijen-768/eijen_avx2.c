#include "eijen.h"
#include "eijen_internal.h"

#include <immintrin.h>
#include <string.h>

const char *eijen_implementation_name(void) {
    return "Eijen x86 AVX2 performance";
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

/* Rotate four packed 64-bit words in AVX2 registers. */
static inline __m256i eijen_rotl64_x4(__m256i x, int n) {
    return _mm256_or_si256(_mm256_slli_epi64(x, n), _mm256_srli_epi64(x, 64 - n));
}

#define EIJEN_ARX_QR(a, b, c, d, r0, r1, r2, r3, r4, r5, rc) do { \
    (a) = _mm256_add_epi64((a), (b)); \
    (d) = _mm256_xor_si256((d), (a)); \
    (d) = eijen_rotl64_x4((d), (r0)); \
    (a) = _mm256_xor_si256(eijen_rotl64_x4((a), (r4)), (rc)); \
    (c) = _mm256_add_epi64((c), (d)); \
    (b) = _mm256_xor_si256((b), (c)); \
    (b) = eijen_rotl64_x4((b), (r1)); \
    (c) = eijen_rotl64_x4((c), (r5)); \
    (a) = _mm256_add_epi64((a), (b)); \
    (d) = _mm256_xor_si256((d), (a)); \
    (d) = eijen_rotl64_x4((d), (r2)); \
    (c) = _mm256_add_epi64((c), (d)); \
    (b) = _mm256_xor_si256((b), (c)); \
    (b) = eijen_rotl64_x4((b), (r3)); \
} while (0)

#define EIJEN_ARX_COLUMNS(s, rc0, rc1) do { \
    EIJEN_ARX_QR((s)[0], (s)[1], (s)[2], (s)[3], 36, 54, 45, 44, 18, 9, (rc0)); \
    EIJEN_ARX_QR((s)[4], (s)[5], (s)[6], (s)[7], 36, 54, 45, 44, 18, 9, (rc0)); \
    EIJEN_ARX_QR((s)[8], (s)[9], (s)[10], (s)[11], 36, 54, 45, 44, 18, 9, (rc0)); \
    EIJEN_ARX_QR((s)[12], (s)[13], (s)[14], (s)[15], 36, 54, 45, 44, 18, 9, (rc0)); \
    EIJEN_ARX_QR((s)[16], (s)[17], (s)[18], (s)[19], 56, 46, 54, 34, 19, 51, (rc1)); \
    EIJEN_ARX_QR((s)[20], (s)[21], (s)[22], (s)[23], 56, 46, 54, 34, 19, 51, (rc1)); \
    EIJEN_ARX_QR((s)[24], (s)[25], (s)[26], (s)[27], 56, 46, 54, 34, 19, 51, (rc1)); \
    EIJEN_ARX_QR((s)[28], (s)[29], (s)[30], (s)[31], 56, 46, 54, 34, 19, 51, (rc1)); \
} while (0)

#define EIJEN_SIGMA(s) do { \
    __m256i t_; \
    t_ = (s)[0]; (s)[0] = (s)[22]; (s)[22] = (s)[17]; (s)[17] = (s)[16]; (s)[16] = (s)[6]; (s)[6] = (s)[1]; (s)[1] = t_; \
    t_ = (s)[2]; (s)[2] = (s)[29]; (s)[29] = (s)[28]; (s)[28] = (s)[18]; (s)[18] = (s)[13]; (s)[13] = (s)[12]; (s)[12] = t_; \
    t_ = (s)[3]; (s)[3] = (s)[19]; (s)[19] = t_; \
    t_ = (s)[4]; (s)[4] = (s)[26]; (s)[26] = (s)[21]; (s)[21] = (s)[20]; (s)[20] = (s)[10]; (s)[10] = (s)[5]; (s)[5] = t_; \
    t_ = (s)[7]; (s)[7] = (s)[23]; (s)[23] = t_; \
    t_ = (s)[8]; (s)[8] = (s)[30]; (s)[30] = (s)[25]; (s)[25] = (s)[24]; (s)[24] = (s)[14]; (s)[14] = (s)[9]; (s)[9] = t_; \
    t_ = (s)[11]; (s)[11] = (s)[27]; (s)[27] = t_; \
    t_ = (s)[15]; (s)[15] = (s)[31]; (s)[31] = t_; \
} while (0)

#define EIJEN_LINEAR_LAYER(s) do { \
    int col_; \
    for (col_ = 0; col_ < 8; col_++) { \
        int b_ = col_ * 4; \
        __m256i x_ = _mm256_xor_si256(_mm256_xor_si256((s)[b_], (s)[b_ + 1]), \
                                      _mm256_xor_si256((s)[b_ + 2], (s)[b_ + 3])); \
        (s)[b_] = _mm256_xor_si256((s)[b_], x_); \
        (s)[b_ + 1] = _mm256_xor_si256((s)[b_ + 1], x_); \
        (s)[b_ + 2] = _mm256_xor_si256((s)[b_ + 2], x_); \
        (s)[b_ + 3] = _mm256_xor_si256((s)[b_ + 3], x_); \
    } \
} while (0)

/* Evaluate the full 16-round Eijen permutation on four states in parallel. */
static void eijen_permutation_simd(__m256i s[EIJEN_STATE_WORDS]) {
    int r;
    for (r = 0; r < EIJEN_ROUNDS; r++) {
        __m256i rc0 = _mm256_set1_epi64x((long long)EIJEN_RC0[r]);
        __m256i rc1 = _mm256_set1_epi64x((long long)EIJEN_RC1[r]);
        EIJEN_ARX_COLUMNS(s, rc0, rc1);
        EIJEN_SIGMA(s);
        EIJEN_LINEAR_LAYER(s);
        EIJEN_SIGMA(s);
    }
}

/* Apply the scalar ARX MixQuad operation to one four-word column. */
static void eijen_mixquad_scalar(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d,
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

/* Apply scalar MixQuad to all eight columns. */
static void eijen_columns_scalar(uint64_t s[EIJEN_STATE_WORDS], uint64_t rc0,
                                 uint64_t rc1) {
    int i;
    for (i = 0; i < 16; i += 4) {
        eijen_mixquad_scalar(&s[i], &s[i + 1], &s[i + 2], &s[i + 3],
                             36, 54, 45, 44, 18, 9, rc0);
    }
    for (i = 16; i < 32; i += 4) {
        eijen_mixquad_scalar(&s[i], &s[i + 1], &s[i + 2], &s[i + 3],
                             56, 46, 54, 34, 19, 51, rc1);
    }
}

/* Apply the scalar word-position permutation. */
static void eijen_sigma_scalar(uint64_t s[EIJEN_STATE_WORDS]) {
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

/* Apply the scalar four-word XOR mixing layer to each column. */
static void eijen_linear_layer_scalar(uint64_t s[EIJEN_STATE_WORDS]) {
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

/* Evaluate the full scalar permutation used by the single-message path. */
static void eijen_permutation(uint64_t state[EIJEN_STATE_WORDS]) {
    int r;
    for (r = 0; r < EIJEN_ROUNDS; r++) {
        eijen_columns_scalar(state, EIJEN_RC0[r], EIJEN_RC1[r]);
        eijen_sigma_scalar(state);
        eijen_linear_layer_scalar(state);
        eijen_sigma_scalar(state);
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

/* Hash four independent single-block messages with identical byte length. */
static void eijen_hash4_oneblock(const eijen_config *cfg,
                                 const uint8_t *m0, const uint8_t *m1,
                                 const uint8_t *m2, const uint8_t *m3,
                                 size_t len,
                                 uint8_t *o0, uint8_t *o1,
                                 uint8_t *o2, uint8_t *o3) {
    __m256i s[EIJEN_STATE_WORDS];
    int full_words = (int)(len / 8U);
    int tail = (int)(len & 7U);
    int ds = EIJEN_STATE_WORDS - cfg->digest_words;
    int i;

    for (i = 0; i < EIJEN_STATE_WORDS; i++) {
        s[i] = _mm256_setzero_si256();
    }
    for (i = 0; i < full_words; i++) {
        s[i] = _mm256_set_epi64x((long long)eijen_load64_le(m3 + ((size_t)i * 8U)),
                                 (long long)eijen_load64_le(m2 + ((size_t)i * 8U)),
                                 (long long)eijen_load64_le(m1 + ((size_t)i * 8U)),
                                 (long long)eijen_load64_le(m0 + ((size_t)i * 8U)));
    }
    if (tail > 0) {
        const uint8_t *msgs[4] = {m0, m1, m2, m3};
        uint64_t v[4];
        int j;
        for (j = 0; j < 4; j++) {
            uint64_t w = 0;
            int k;
            const uint8_t *p = msgs[j] + ((size_t)full_words * 8U);
            for (k = 0; k < tail; k++) {
                w |= ((uint64_t)p[k]) << (8U * (unsigned int)k);
            }
            w |= UINT64_C(0x01) << (8U * (unsigned int)tail);
            v[j] = w;
        }
        s[full_words] = _mm256_loadu_si256((const __m256i *)v);
    } else {
        s[full_words] = _mm256_set1_epi64x((long long)UINT64_C(0x01));
    }
    s[EIJEN_STATE_WORDS - 1] = _mm256_xor_si256(
        s[EIJEN_STATE_WORDS - 1],
        _mm256_set1_epi64x((long long)(UINT64_C(1) << 63)));

    eijen_permutation_simd(s);

    for (i = 0; i < cfg->digest_words; i++) {
        uint64_t v[4];
        _mm256_storeu_si256((__m256i *)v, s[ds + i]);
        eijen_store64_le(o0 + ((size_t)i * 8U), v[0]);
        eijen_store64_le(o1 + ((size_t)i * 8U), v[1]);
        eijen_store64_le(o2 + ((size_t)i * 8U), v[2]);
        eijen_store64_le(o3 + ((size_t)i * 8U), v[3]);
    }
}

/* Public four-way AVX2 helper for equal-length byte-aligned short messages. */
int eijen_hash4_same(int digest_bits,
                     const uint8_t *msg0, const uint8_t *msg1,
                     const uint8_t *msg2, const uint8_t *msg3,
                     size_t len,
                     uint8_t *out0, uint8_t *out1,
                     uint8_t *out2, uint8_t *out3) {
    const eijen_config *cfg = eijen_get_config(digest_bits);
    if (cfg == NULL || (len != 0U && (msg0 == NULL || msg1 == NULL || msg2 == NULL || msg3 == NULL)) ||
        out0 == NULL || out1 == NULL || out2 == NULL || out3 == NULL) {
        return -1;
    }
    if (len < (size_t)cfg->rate_bytes) {
        eijen_hash4_oneblock(cfg, msg0, msg1, msg2, msg3, len, out0, out1, out2, out3);
        return 0;
    }
    if (eijen_hash(digest_bits, msg0, len, out0) != 0) return -1;
    if (eijen_hash(digest_bits, msg1, len, out1) != 0) return -1;
    if (eijen_hash(digest_bits, msg2, len, out2) != 0) return -1;
    if (eijen_hash(digest_bits, msg3, len, out3) != 0) return -1;
    return 0;
}
