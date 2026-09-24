#include "nss_hqc_core.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint32_t hamming_weight(const uint8_t *a, uint32_t bits);

#include "auxfunc.h"
#include "code_layer.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

#define NSS_OK 0
#define NSS_ERR_ARG -2
#define NSS_ERR_HASH -3
#define NSS_ERR_RNG -4
#define NSS_ERR_ALLOC -5
#define NSS_ERR_DECAP -1

#define NSS_HQC_KAT_ENTROPY_BYTES 4096u
#define NSS_HQC_I_SEED_BYTES 32u

static uint8_t kat_entropy_buf[NSS_HQC_KAT_ENTROPY_BYTES];
static size_t kat_entropy_pos = 0u;
static int kat_entropy_enabled = 0;

#ifdef NSS_HQC_DEBUG_KAT
static int dbg_saved_first_encrypt = 0;
static uint8_t dbg_clean_cw[NSS_HQC_NC_BYTES];
static uint8_t dbg_v_before_dither[NSS_HQC_NC_BYTES];
static uint8_t dbg_v_after_dither[NSS_HQC_NC_BYTES];
static uint8_t dbg_u[NSS_HQC_N_BYTES];
static uint8_t dbg_r1[NSS_HQC_N_BYTES];
static uint8_t dbg_r2[NSS_HQC_N_BYTES];
static uint8_t dbg_e[NSS_HQC_N_BYTES];
#endif

static uint8_t bit_get(const uint8_t *a, uint32_t pos)
{
    return (uint8_t)((a[pos >> 3] >> (pos & 7u)) & 1u);
}

static void bit_xor(uint8_t *a, uint32_t pos, uint8_t v)
{
    a[pos >> 3] ^= (uint8_t)((v & 1u) << (pos & 7u));
}

static void bit_set(uint8_t *a, uint32_t pos, uint8_t v)
{
    uint8_t mask = (uint8_t)(1u << (pos & 7u));
    if (v) {
        a[pos >> 3] |= mask;
    } else {
        a[pos >> 3] &= (uint8_t)~mask;
    }
}

static void clear_unused_bits(uint8_t *a, uint32_t bits)
{
    uint32_t rem = bits & 7u;
    if (rem != 0u) {
        a[bits >> 3] &= (uint8_t)((1u << rem) - 1u);
    }
}

static int ct_verify(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t diff = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return diff == 0u ? 0 : -1;
}

static void ct_select(uint8_t *out, const uint8_t *a, const uint8_t *b,
                      size_t len, uint8_t use_a)
{
    uint8_t mask = (uint8_t)(0u - (uint8_t)(use_a != 0u));
    size_t i;
    for (i = 0; i < len; i++) {
        out[i] = (uint8_t)((a[i] & mask) | (b[i] & (uint8_t)~mask));
    }
}

#ifdef NSS_HQC_DEBUG_KAT
static void debug_print_bytes(const char *label, const uint8_t *buf, size_t len)
{
    size_t i;
    fprintf(stderr, "[debug] %s", label);
    for (i = 0; i < len; i++) {
        fprintf(stderr, "%02x", buf[i]);
        if (i + 1u != len) {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n");
}

static void debug_print_ct_diff(const uint8_t *ct, const uint8_t *ct_check)
{
    size_t i;
    size_t total_diff = 0u;
    size_t u_diff = 0u;
    size_t v_diff = 0u;
    size_t first = NSS_HQC_CT_BYTES;

    for (i = 0; i < NSS_HQC_CT_BYTES; i++) {
        if (ct[i] != ct_check[i]) {
            total_diff++;
            if (i < NSS_HQC_N_BYTES) {
                u_diff++;
            } else {
                v_diff++;
            }
            if (first == NSS_HQC_CT_BYTES) {
                first = i;
            }
        }
    }

    fprintf(stderr, "[debug] ct diff bytes total=%lu u=%lu v=%lu\n",
            (unsigned long)total_diff,
            (unsigned long)u_diff,
            (unsigned long)v_diff);
    if (first != NSS_HQC_CT_BYTES) {
        fprintf(stderr, "[debug] first ct diff index=%lu ct=%02x ct_check=%02x\n",
                (unsigned long)first, ct[first], ct_check[first]);
    }
}

static uint32_t debug_hamming_bits(const uint8_t *a, const uint8_t *b, uint32_t bits)
{
    uint32_t i;
    uint32_t d = 0u;
    for (i = 0u; i < bits; i++) {
        d += bit_get(a, i) ^ bit_get(b, i);
    }
    return d;
}

static uint32_t debug_weight_bits(const uint8_t *a, uint32_t bits)
{
    uint32_t i;
    uint32_t w = 0u;
    for (i = 0u; i < bits; i++) {
        w += bit_get(a, i);
    }
    return w;
}

static void debug_print_block_errors(const uint8_t *a, const uint8_t *b)
{
    uint32_t block;
    uint32_t min_err = 0xffffffffu;
    uint32_t max_err = 0u;
    uint64_t sum_err = 0u;
    uint32_t block_bits = 128u * NSS_HQC_MULT;
    uint32_t first16[16];
    memset(first16, 0, sizeof(first16));

    for (block = 0u; block < NSS_HQC_N1; block++) {
        uint32_t j;
        uint32_t errors = 0u;
        for (j = 0u; j < block_bits; j++) {
            uint32_t pos = block * block_bits + j;
            errors += bit_get(a, pos) ^ bit_get(b, pos);
        }
        if (errors < min_err) {
            min_err = errors;
        }
        if (errors > max_err) {
            max_err = errors;
        }
        if (block < 16u) {
            first16[block] = errors;
        }
        sum_err += errors;
    }
    fprintf(stderr, "[debug] final-vs-clean block errors min=%lu max=%lu avg=%lu\n",
            (unsigned long)min_err,
            (unsigned long)max_err,
            (unsigned long)(sum_err / NSS_HQC_N1));
    fprintf(stderr, "[debug] final-vs-clean block errors first16 =");
    for (block = 0u; block < NSS_HQC_N1 && block < 16u; block++) {
        fprintf(stderr, " %lu", (unsigned long)first16[block]);
    }
    fprintf(stderr, "\n");
}

static void debug_print_first16_diff(const char *label, const uint8_t *a, const uint8_t *b)
{
    uint32_t i;
    fprintf(stderr, "[debug] %s first16 xor =", label);
    for (i = 0u; i < 16u; i++) {
        fprintf(stderr, " %02x", (uint8_t)(a[i] ^ b[i]));
    }
    fprintf(stderr, "\n");
}
#endif

static uint64_t rotl64(uint64_t x, unsigned int n)
{
    if (n == 0u) {
        return x;
    }
    return (x << n) | (x >> (64u - n));
}

static uint64_t load64_le(const uint8_t *x)
{
    uint64_t r = 0u;
    unsigned int i;
    for (i = 0u; i < 8u; i++) {
        r |= ((uint64_t)x[i]) << (8u * i);
    }
    return r;
}

static void store64_le(uint8_t *x, uint64_t u)
{
    unsigned int i;
    for (i = 0u; i < 8u; i++) {
        x[i] = (uint8_t)(u >> (8u * i));
    }
}

static void keccakf1600(uint64_t s[25])
{
    static const uint64_t rc[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL,
        0x800000000000808aULL, 0x8000000080008000ULL,
        0x000000000000808bULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL,
        0x000000000000008aULL, 0x0000000000000088ULL,
        0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL,
        0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800aULL, 0x800000008000000aULL,
        0x8000000080008081ULL, 0x8000000000008080ULL,
        0x0000000080000001ULL, 0x8000000080008008ULL
    };
    static const unsigned int r[25] = {
        0u, 1u, 62u, 28u, 27u, 36u, 44u, 6u, 55u, 20u,
        3u, 10u, 43u, 25u, 39u, 41u, 45u, 15u, 21u, 8u,
        18u, 2u, 61u, 56u, 14u
    };
    static const unsigned int p[25] = {
        0u, 10u, 20u, 5u, 15u, 16u, 1u, 11u, 21u, 6u,
        7u, 17u, 2u, 12u, 22u, 23u, 8u, 18u, 3u, 13u,
        14u, 24u, 9u, 19u, 4u
    };
    unsigned int round;
    for (round = 0u; round < 24u; round++) {
        uint64_t c[5];
        uint64_t d[5];
        uint64_t b[25];
        unsigned int x;
        unsigned int y;
        for (x = 0u; x < 5u; x++) {
            c[x] = s[x] ^ s[x + 5u] ^ s[x + 10u] ^ s[x + 15u] ^ s[x + 20u];
        }
        for (x = 0u; x < 5u; x++) {
            d[x] = c[(x + 4u) % 5u] ^ rotl64(c[(x + 1u) % 5u], 1u);
        }
        for (x = 0u; x < 5u; x++) {
            for (y = 0u; y < 5u; y++) {
                s[x + 5u * y] ^= d[x];
            }
        }
        for (x = 0u; x < 25u; x++) {
            b[p[x]] = rotl64(s[x], r[x]);
        }
        for (y = 0u; y < 5u; y++) {
            for (x = 0u; x < 5u; x++) {
                s[x + 5u * y] = b[x + 5u * y] ^
                    ((uint64_t)~b[((x + 1u) % 5u) + 5u * y] &
                     b[((x + 2u) % 5u) + 5u * y]);
            }
        }
        s[0] ^= rc[round];
    }
}

static void shake256(uint8_t *out, size_t out_len,
                     const uint8_t *in, size_t in_len)
{
    enum { SHAKE256_RATE = 136 };
    uint64_t s[25];
    uint8_t block[SHAKE256_RATE];
    size_t i;
    memset(s, 0, sizeof(s));
    while (in_len >= SHAKE256_RATE) {
        for (i = 0u; i < SHAKE256_RATE / 8u; i++) {
            s[i] ^= load64_le(in + 8u * i);
        }
        keccakf1600(s);
        in += SHAKE256_RATE;
        in_len -= SHAKE256_RATE;
    }
    memset(block, 0, sizeof(block));
    if (in_len != 0u) {
        memcpy(block, in, in_len);
    }
    block[in_len] ^= 0x1fu;
    block[SHAKE256_RATE - 1u] ^= 0x80u;
    for (i = 0u; i < SHAKE256_RATE / 8u; i++) {
        s[i] ^= load64_le(block + 8u * i);
    }
    keccakf1600(s);
    while (out_len != 0u) {
        size_t take = out_len < SHAKE256_RATE ? out_len : SHAKE256_RATE;
        for (i = 0u; i < SHAKE256_RATE / 8u; i++) {
            store64_le(block + 8u * i, s[i]);
        }
        memcpy(out, block, take);
        out += take;
        out_len -= take;
        if (out_len != 0u) {
            keccakf1600(s);
        }
    }
}

static void keccak_hash(uint8_t *out, size_t out_len,
                        const uint8_t *in, size_t in_len, size_t rate)
{
    uint64_t s[25];
    uint8_t block[136];
    size_t i;
    memset(s, 0, sizeof(s));
    while (in_len >= rate) {
        for (i = 0u; i < rate / 8u; i++) {
            s[i] ^= load64_le(in + 8u * i);
        }
        keccakf1600(s);
        in += rate;
        in_len -= rate;
    }
    memset(block, 0, sizeof(block));
    if (in_len != 0u) {
        memcpy(block, in, in_len);
    }
    block[in_len] ^= 0x06u;
    block[rate - 1u] ^= 0x80u;
    for (i = 0u; i < rate / 8u; i++) {
        s[i] ^= load64_le(block + 8u * i);
    }
    keccakf1600(s);
    for (i = 0u; i < rate / 8u; i++) {
        store64_le(block + 8u * i, s[i]);
    }
    memcpy(out, block, out_len);
}

void nss_hqc_shake256_public(unsigned char *out, unsigned long long out_len,
                             const unsigned char *in, unsigned long long in_len)
{
    shake256(out, (size_t)out_len, in, (size_t)in_len);
}

static void sha3_512(uint8_t out[64], const uint8_t *in, size_t in_len)
{
    keccak_hash(out, 64u, in, in_len, 72u);
}

static int concat_tag(uint8_t **out, size_t *out_len,
                      const uint8_t *a, size_t alen,
                      const uint8_t *b, size_t blen,
                      const uint8_t *c, size_t clen,
                      uint8_t tag)
{
    uint8_t *msg;
    size_t len = alen + blen + clen + 1u;
    msg = (uint8_t *)calloc(len, 1u);
    if (msg == NULL) {
        return NSS_ERR_ALLOC;
    }
    if (alen != 0u) {
        memcpy(msg, a, alen);
    }
    if (blen != 0u) {
        memcpy(msg + alen, b, blen);
    }
    if (clen != 0u) {
        memcpy(msg + alen + blen, c, clen);
    }
    msg[len - 1u] = tag;
    *out = msg;
    *out_len = len;
    return NSS_OK;
}
static int hash_g(uint8_t *theta, const uint8_t *pk,
                  const uint8_t *m, const uint8_t *salt)
{
    size_t msg_len = NSS_HQC_MSG_BYTES + NSS_HQC_SALT_BYTES + NSS_HQC_PK_BYTES;
    uint8_t digest[64];
    uint8_t *msg = (uint8_t *)calloc(msg_len, 1u);
    if (msg == NULL) {
        return NSS_ERR_ALLOC;
    }
    memcpy(msg, m, NSS_HQC_MSG_BYTES);
    memcpy(msg + NSS_HQC_MSG_BYTES, salt, NSS_HQC_SALT_BYTES);
    memcpy(msg + NSS_HQC_MSG_BYTES + NSS_HQC_SALT_BYTES, pk, NSS_HQC_PK_BYTES);
    sha3_512(digest, msg, msg_len);
    memcpy(theta, digest, NSS_HQC_THETA_BYTES);
    memset(digest, 0, sizeof(digest));
    free(msg);
    return NSS_OK;
}

static int hash_kappa(uint8_t *out, const uint8_t *prefix, size_t prefix_len,
                      const uint8_t *ct)
{
    size_t msg_len = prefix_len + NSS_HQC_CT_FULL_BYTES;
    uint8_t *msg = (uint8_t *)calloc(msg_len, 1u);
    if (msg == NULL) {
        return NSS_ERR_ALLOC;
    }
    memcpy(msg, prefix, prefix_len);
    memcpy(msg + prefix_len, ct, NSS_HQC_CT_FULL_BYTES);
    shake256(out, NSS_HQC_SS_BYTES, msg, msg_len);
    free(msg);
    return NSS_OK;
}

static int derive_xy_seeds(uint8_t *seed_x, uint8_t *seed_y, const uint8_t *seed_sk)
{
    uint8_t digest[64];
    sha3_512(digest, seed_sk, NSS_HQC_SEED_SK_BYTES);
    memcpy(seed_x, digest, NSS_HQC_I_SEED_BYTES);
    memcpy(seed_y, digest + NSS_HQC_I_SEED_BYTES, NSS_HQC_I_SEED_BYTES);
    memset(digest, 0, sizeof(digest));
    return NSS_OK;
}

static int random_bytes(uint8_t *out, unsigned long long len)
{
    if (kat_entropy_enabled != 0) {
        if (len > (unsigned long long)(NSS_HQC_KAT_ENTROPY_BYTES - kat_entropy_pos)) {
            return NSS_ERR_RNG;
        }
        memcpy(out, kat_entropy_buf + kat_entropy_pos, (size_t)len);
        kat_entropy_pos += (size_t)len;
        return NSS_OK;
    }
    return get_random_number(&drng_algorithm, out, len * 8ull) == 0 ? NSS_OK : NSS_ERR_RNG;
}

int nss_hqc_kat_seed_entropy(const unsigned char *seed, unsigned long long seed_len)
{
    uint8_t *msg;
    if (seed == NULL || seed_len > (unsigned long long)(SIZE_MAX - 1u)) {
        return NSS_ERR_ARG;
    }
    msg = (uint8_t *)calloc((size_t)seed_len + 1u, 1u);
    if (msg == NULL) {
        return NSS_ERR_ALLOC;
    }
    memcpy(msg, seed, (size_t)seed_len);
    msg[seed_len] = 0x00u;
    shake256(kat_entropy_buf, NSS_HQC_KAT_ENTROPY_BYTES, msg, (size_t)seed_len + 1u);
    free(msg);
    kat_entropy_pos = 0u;
    kat_entropy_enabled = 1;
    return NSS_OK;
}

static int derive_h(uint8_t *h, const uint8_t *seed)
{
    uint8_t *msg = NULL;
    size_t msg_len = 0u;
    int ret = concat_tag(&msg, &msg_len, seed, NSS_HQC_PK_SEED_BYTES,
                         NULL, 0u, NULL, 0u, 0x01u);
    if (ret != NSS_OK) {
        return ret;
    }
    shake256(h, NSS_HQC_N_BYTES, msg, msg_len);
    clear_unused_bits(h, NSS_HQC_N);
    free(msg);
    return NSS_OK;
}

typedef struct {
    uint8_t *buf;
    size_t len;
    size_t pos;
} xof_stream_t;

static int xof_stream_init(xof_stream_t *st, const uint8_t *seed, size_t seed_len,
                           size_t out_len)
{
    uint8_t *msg = NULL;
    size_t msg_len = 0u;
    int ret = concat_tag(&msg, &msg_len, seed, seed_len, NULL, 0u, NULL, 0u, 0x01u);
    if (ret != NSS_OK) return ret;
    st->buf = (uint8_t *)calloc(out_len == 0u ? 1u : out_len, 1u);
    if (st->buf == NULL) {
        free(msg);
        return NSS_ERR_ALLOC;
    }
    st->len = out_len;
    st->pos = 0u;
    shake256(st->buf, out_len, msg, msg_len);
    free(msg);
    return NSS_OK;
}

static void xof_stream_clear(xof_stream_t *st)
{
    free(st->buf);
    st->buf = NULL;
    st->len = 0u;
    st->pos = 0u;
}

static int xof_stream_get(xof_stream_t *st, uint8_t *out, size_t len)
{
    if (st->pos + len > st->len) {
        return NSS_ERR_ARG;
    }
    memcpy(out, st->buf + st->pos, len);
    st->pos += len;
    return NSS_OK;
}

static int sample_fixed_weight1_stream(uint8_t *vec, xof_stream_t *st,
                                       uint32_t weight)
{
    uint32_t *perm;
    uint32_t i;
    if (weight > NSS_HQC_N) {
        return NSS_ERR_ARG;
    }
    perm = (uint32_t *)calloc(NSS_HQC_N, sizeof(uint32_t));
    if (perm == NULL) {
        return NSS_ERR_ALLOC;
    }
    for (i = 0u; i < NSS_HQC_N; i++) {
        perm[i] = i;
    }
    memset(vec, 0, NSS_HQC_N_BYTES);
    for (i = 0u; i < weight; i++) {
        const uint32_t range = NSS_HQC_N - i;
        const uint64_t bound64 = (UINT64_C(0x100000000) / (uint64_t)range) * (uint64_t)range;
        uint32_t u;
        uint8_t b[4];
        do {
            if (xof_stream_get(st, b, sizeof(b)) != NSS_OK) {
                clear_unused_bits(vec, NSS_HQC_N);
                free(perm);
                return NSS_ERR_ARG;
            }
            u = ((uint32_t)b[0] |
                ((uint32_t)b[1] << 8) |
                ((uint32_t)b[2] << 16) |
                ((uint32_t)b[3] << 24));
        } while ((uint64_t)u >= bound64);
        {
            uint32_t j = i + (uint32_t)((uint64_t)u % (uint64_t)range);
            uint32_t tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }
    }
    for (i = 0u; i < weight; i++) {
        bit_set(vec, perm[i], 1u);
    }
    clear_unused_bits(vec, NSS_HQC_N);
    free(perm);
    return NSS_OK;
}

static int sample_fixed_weight2_stream(uint8_t *vec, xof_stream_t *st,
                                       uint32_t weight)
{
    return sample_fixed_weight1_stream(vec, st, weight);
}

static void ring_mul_by_sparse(uint8_t *out, const uint8_t *dense, const uint8_t *sparse)
{
    uint32_t i;
    uint32_t j;
    memset(out, 0, NSS_HQC_N_BYTES);
    for (i = 0u; i < NSS_HQC_N; i++) {
        if (bit_get(sparse, i) != 0u) {
            for (j = 0u; j < NSS_HQC_N; j++) {
                if (bit_get(dense, j) != 0u) {
                    uint32_t pos = i + j;
                    if (pos >= NSS_HQC_N) {
                        pos -= NSS_HQC_N;
                    }
                    bit_xor(out, pos, 1u);
                }
            }
        }
    }
    clear_unused_bits(out, NSS_HQC_N);
}

static void xor_n(uint8_t *out, const uint8_t *a, const uint8_t *b)
{
    uint32_t i;
    for (i = 0u; i < NSS_HQC_N_BYTES; i++) {
        out[i] = (uint8_t)(a[i] ^ b[i]);
    }
    clear_unused_bits(out, NSS_HQC_N);
}

static void quant_compress(uint8_t *vhat, const uint8_t *bits)
{
    uint32_t block;
    memset(vhat, 0, NSS_HQC_VHAT_BYTES);
    for (block = 0u; block < NSS_HQC_NC / NSS_HQC_MULT; block++) {
        uint32_t j;
        uint32_t sum = 0u;
        uint32_t code;
        for (j = 0u; j < NSS_HQC_MULT; j++) {
            sum += bit_get(bits, block * NSS_HQC_MULT + j);
        }
        code = (sum * (NSS_HQC_Q - 1u) + NSS_HQC_MULT / 2u) / NSS_HQC_MULT;
        for (j = 0u; j < NSS_HQC_B; j++) {
            bit_set(vhat, block * NSS_HQC_B + j, (uint8_t)((code >> j) & 1u));
        }
    }
}

static void quant_decompress(uint8_t *bits, const uint8_t *vhat)
{
    uint32_t block;
    memset(bits, 0, NSS_HQC_NC_BYTES);
    for (block = 0u; block < NSS_HQC_NC / NSS_HQC_MULT; block++) {
        uint32_t j;
        uint32_t code = 0u;
        uint8_t fill;
        for (j = 0u; j < NSS_HQC_B; j++) {
            code |= ((uint32_t)bit_get(vhat, block * NSS_HQC_B + j)) << j;
        }
        fill = (uint8_t)(code >= (NSS_HQC_Q / 2u));
        for (j = 0u; j < NSS_HQC_MULT; j++) {
            bit_set(bits, block * NSS_HQC_MULT + j, fill);
        }
    }
    clear_unused_bits(bits, NSS_HQC_NC);
}

static int derive_dither_base(uint8_t *base, uint32_t base_bits,
                              const uint8_t *salt, const uint8_t *pk)
{
    static const uint8_t tag[] = { 'd', 'i', 't', 'h', 'e', 'r' };
    uint32_t base_bytes = (base_bits + 7u) / 8u;
    size_t msg_len = NSS_HQC_SALT_BYTES + NSS_HQC_PK_BYTES + sizeof(tag) + 1u;
    uint8_t *msg = (uint8_t *)calloc(msg_len, 1u);
    if (msg == NULL) {
        return NSS_ERR_ALLOC;
    }
    memcpy(msg, salt, NSS_HQC_SALT_BYTES);
    memcpy(msg + NSS_HQC_SALT_BYTES, pk, NSS_HQC_PK_BYTES);
    memcpy(msg + NSS_HQC_SALT_BYTES + NSS_HQC_PK_BYTES, tag, sizeof(tag));
    msg[msg_len - 1u] = 0x01u;
    shake256(base, base_bytes, msg, msg_len);
    clear_unused_bits(base, base_bits);
    free(msg);
    return NSS_OK;
}

static int derive_dither(uint8_t *d, const uint8_t *salt, const uint8_t *pk)
{
    uint32_t base_bytes = (NSS_HQC_NC / NSS_HQC_MULT + 7u) / 8u;
    uint8_t *base = (uint8_t *)calloc(base_bytes, 1u);
    uint32_t block;
    int ret;
    if (base == NULL) {
        return NSS_ERR_ALLOC;
    }
    memset(d, 0, NSS_HQC_NC_BYTES);
    ret = derive_dither_base(base, NSS_HQC_NC / NSS_HQC_MULT, salt, pk);
    if (ret == NSS_OK) {
        for (block = 0u; block < NSS_HQC_NC / NSS_HQC_MULT; block++) {
            uint32_t rep;
            uint8_t b = bit_get(base, block);
            for (rep = 0u; rep < NSS_HQC_MULT; rep++) {
                bit_set(d, block * NSS_HQC_MULT + rep, b);
            }
        }
        clear_unused_bits(d, NSS_HQC_NC);
    }
    free(base);
    return ret;
}

static int pke_encrypt(uint8_t *ct, const uint8_t *pk, const uint8_t *m,
                       const uint8_t *theta, const uint8_t *salt)
{
    const uint8_t *seed = pk;
    const uint8_t *s = pk + NSS_HQC_PK_SEED_BYTES;
    uint8_t *h = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *r1 = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *r2 = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *e = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *tmp = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *cw = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t *d = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    int ret = NSS_OK;
    uint32_t i;
    xof_stream_t theta_stream = { 0 };

    if (h == NULL || r1 == NULL || r2 == NULL || e == NULL ||
        tmp == NULL || cw == NULL || d == NULL) {
        ret = NSS_ERR_ALLOC;
        goto cleanup;
    }
    if ((ret = derive_h(h, seed)) != NSS_OK) goto cleanup;
    if ((ret = xof_stream_init(&theta_stream, theta, NSS_HQC_THETA_BYTES,
                               4u * (NSS_HQC_W_R2 + NSS_HQC_W_E + NSS_HQC_W_R1) + 4096u)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight2_stream(r2, &theta_stream, NSS_HQC_W_R2)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight2_stream(e, &theta_stream, NSS_HQC_W_E)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight2_stream(r1, &theta_stream, NSS_HQC_W_R1)) != NSS_OK) goto cleanup;

    ring_mul_by_sparse(tmp, h, r2);
    xor_n(ct, r1, tmp);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt == 0) {
        memcpy(dbg_u, ct, NSS_HQC_N_BYTES);
        memcpy(dbg_r1, r1, NSS_HQC_N_BYTES);
        memcpy(dbg_r2, r2, NSS_HQC_N_BYTES);
        memcpy(dbg_e, e, NSS_HQC_N_BYTES);
        fprintf(stderr,
                "[debug] first encrypt weights params W_SK=%lu W_R1=%lu W_R2=%lu W_E=%lu actual r1=%lu r2=%lu e=%lu u=%lu\n",
                (unsigned long)NSS_HQC_W_SK,
                (unsigned long)NSS_HQC_W_R1,
                (unsigned long)NSS_HQC_W_R2,
                (unsigned long)NSS_HQC_W_E,
                (unsigned long)debug_weight_bits(r1, NSS_HQC_N),
                (unsigned long)debug_weight_bits(r2, NSS_HQC_N),
                (unsigned long)debug_weight_bits(e, NSS_HQC_N),
                (unsigned long)debug_weight_bits(ct, NSS_HQC_N));
    }
#endif

    nss_hqc_code_encode(cw, m);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt == 0) {
        memcpy(dbg_clean_cw, cw, NSS_HQC_NC_BYTES);
    }
#endif
    ring_mul_by_sparse(tmp, s, r2);
    for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
        cw[i] ^= tmp[i];
    }
    for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
        cw[i] ^= e[i];
    }
    clear_unused_bits(cw, NSS_HQC_NC);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt == 0) {
        memcpy(dbg_v_before_dither, cw, NSS_HQC_NC_BYTES);
        debug_print_bytes("first encrypt v_before_dither[0..15] = ", cw, 16u);
    }
#endif

    if ((ret = derive_dither(d, salt, pk)) != NSS_OK) goto cleanup;
    for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
        cw[i] ^= d[i];
    }
    clear_unused_bits(cw, NSS_HQC_NC);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt == 0) {
        memcpy(dbg_v_after_dither, cw, NSS_HQC_NC_BYTES);
        debug_print_bytes("first encrypt v_after_dither[0..15] = ", cw, 16u);
        dbg_saved_first_encrypt = 1;
    }
#endif
    quant_compress(ct + NSS_HQC_N_BYTES, cw);

cleanup:
    xof_stream_clear(&theta_stream);
    free(d);
    free(cw);
    free(tmp);
    free(e);
    free(r2);
    free(r1);
    free(h);
    return ret;
}

static int pke_decrypt(uint8_t *m, const uint8_t *sk,
                       const uint8_t *ct, const uint8_t *salt)
{
    const uint8_t *seed_sk = sk;
    const uint8_t *pk = sk + NSS_HQC_SEED_SK_BYTES + NSS_HQC_SIGMA_BYTES;
    uint8_t *x = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *y = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *tmp = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *cw = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t *d = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t seed_x[NSS_HQC_I_SEED_BYTES];
    uint8_t seed_y[NSS_HQC_I_SEED_BYTES];
    xof_stream_t x_stream = { 0 };
    xof_stream_t y_stream = { 0 };
    int ret = NSS_OK;
    uint32_t i;

    if (x == NULL || y == NULL || tmp == NULL || cw == NULL || d == NULL) {
        ret = NSS_ERR_ALLOC;
        goto cleanup;
    }
    if ((ret = derive_xy_seeds(seed_x, seed_y, seed_sk)) != NSS_OK) goto cleanup;
    if ((ret = xof_stream_init(&x_stream, seed_x, NSS_HQC_I_SEED_BYTES,
                               3u * NSS_HQC_W_SK + 4096u)) != NSS_OK) goto cleanup;
    if ((ret = xof_stream_init(&y_stream, seed_y, NSS_HQC_I_SEED_BYTES,
                               3u * NSS_HQC_W_SK + 4096u)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight1_stream(x, &x_stream, NSS_HQC_W_SK)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight1_stream(y, &y_stream, NSS_HQC_W_SK)) != NSS_OK) goto cleanup;
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr,
            "[debug] decrypt weights params W_SK=%lu W_R1=%lu W_R2=%lu W_E=%lu actual x=%lu y=%lu saved_r1=%lu saved_r2=%lu saved_e=%lu\n",
            (unsigned long)NSS_HQC_W_SK,
            (unsigned long)NSS_HQC_W_R1,
            (unsigned long)NSS_HQC_W_R2,
            (unsigned long)NSS_HQC_W_E,
            (unsigned long)debug_weight_bits(x, NSS_HQC_N),
            (unsigned long)debug_weight_bits(y, NSS_HQC_N),
            (unsigned long)debug_weight_bits(dbg_r1, NSS_HQC_N),
            (unsigned long)debug_weight_bits(dbg_r2, NSS_HQC_N),
            (unsigned long)debug_weight_bits(dbg_e, NSS_HQC_N));
    if (dbg_saved_first_encrypt != 0) {
        fprintf(stderr, "[debug] decrypt u vs saved_u hd=%lu\n",
                (unsigned long)debug_hamming_bits(ct, dbg_u, NSS_HQC_N));
    }
#endif
    quant_decompress(cw, ct + NSS_HQC_N_BYTES);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt != 0) {
        fprintf(stderr, "[debug] hd decompress_vs_v_after_dither=%lu\n",
                (unsigned long)debug_hamming_bits(cw, dbg_v_after_dither, NSS_HQC_NC));
        debug_print_first16_diff("decompress_vs_v_after_dither", cw, dbg_v_after_dither);
    }
#endif
    if ((ret = derive_dither(d, salt, pk)) != NSS_OK) goto cleanup;
    for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
        cw[i] ^= d[i];
    }
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt != 0) {
        fprintf(stderr, "[debug] hd after_remove_dither_vs_v_before_dither=%lu\n",
                (unsigned long)debug_hamming_bits(cw, dbg_v_before_dither, NSS_HQC_NC));
        debug_print_first16_diff("after_remove_dither_vs_v_before_dither", cw, dbg_v_before_dither);
    }
#endif
    ring_mul_by_sparse(tmp, ct, y);
    for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
        cw[i] ^= tmp[i];
    }
    clear_unused_bits(cw, NSS_HQC_NC);
#ifdef NSS_HQC_DEBUG_KAT
    if (dbg_saved_first_encrypt != 0) {
        uint8_t *expected_noise = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
        uint8_t *expected_final = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
        if (expected_noise != NULL && expected_final != NULL) {
            ring_mul_by_sparse(tmp, x, dbg_r2);
            for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
                expected_noise[i] ^= tmp[i];
            }
            ring_mul_by_sparse(tmp, dbg_r1, y);
            for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
                expected_noise[i] ^= tmp[i];
            }
            for (i = 0u; i < NSS_HQC_NC_BYTES; i++) {
                expected_noise[i] ^= dbg_e[i];
                expected_final[i] = (uint8_t)(dbg_clean_cw[i] ^ expected_noise[i]);
            }
            clear_unused_bits(expected_noise, NSS_HQC_NC);
            clear_unused_bits(expected_final, NSS_HQC_NC);
            fprintf(stderr, "[debug] expected_noise weight=%lu\n",
                    (unsigned long)debug_weight_bits(expected_noise, NSS_HQC_NC));
            fprintf(stderr, "[debug] hd final_vs_expected=%lu\n",
                    (unsigned long)debug_hamming_bits(cw, expected_final, NSS_HQC_NC));
            fprintf(stderr, "[debug] hd final_vs_clean=%lu\n",
                    (unsigned long)debug_hamming_bits(cw, dbg_clean_cw, NSS_HQC_NC));
            fprintf(stderr, "[debug] hd expected_final_vs_clean=%lu\n",
                    (unsigned long)debug_hamming_bits(expected_final, dbg_clean_cw, NSS_HQC_NC));
            debug_print_first16_diff("final_vs_expected", cw, expected_final);
            debug_print_block_errors(cw, dbg_clean_cw);
        } else {
            fprintf(stderr, "[debug] expected buffer allocation failed\n");
        }
        free(expected_noise);
        free(expected_final);
    }
#endif
    if ((ret = nss_hqc_code_decode(m, cw)) != 0) goto cleanup;

cleanup:
    xof_stream_clear(&y_stream);
    xof_stream_clear(&x_stream);
    memset(seed_y, 0, sizeof(seed_y));
    memset(seed_x, 0, sizeof(seed_x));
    free(d);
    free(cw);
    free(tmp);
    free(y);
    free(x);
    return ret;
}

int nss_hqc_keygen(unsigned char *pk, unsigned long long *pk_len,
                   unsigned char *sk, unsigned long long *sk_len)
{
    uint8_t *h;
    uint8_t *x;
    uint8_t *y;
    uint8_t *hy;
    uint8_t seed_x[NSS_HQC_I_SEED_BYTES];
    uint8_t seed_y[NSS_HQC_I_SEED_BYTES];
    xof_stream_t x_stream = { 0 };
    xof_stream_t y_stream = { 0 };
    int ret;

    if (pk == NULL || sk == NULL || pk_len == NULL || sk_len == NULL) {
        return NSS_ERR_ARG;
    }
    h = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    x = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    y = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    hy = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    if (h == NULL || x == NULL || y == NULL || hy == NULL) {
        ret = NSS_ERR_ALLOC;
        goto cleanup;
    }
    if ((ret = random_bytes(pk, NSS_HQC_PK_SEED_BYTES)) != NSS_OK) goto cleanup;
    if ((ret = random_bytes(sk, NSS_HQC_SEED_SK_BYTES)) != NSS_OK) goto cleanup;
    if ((ret = random_bytes(sk + NSS_HQC_SEED_SK_BYTES, NSS_HQC_SIGMA_BYTES)) != NSS_OK) goto cleanup;
    if ((ret = derive_h(h, pk)) != NSS_OK) goto cleanup;
    if ((ret = derive_xy_seeds(seed_x, seed_y, sk)) != NSS_OK) goto cleanup;
    if ((ret = xof_stream_init(&x_stream, seed_x, NSS_HQC_I_SEED_BYTES,
                               3u * NSS_HQC_W_SK + 4096u)) != NSS_OK) goto cleanup;
    if ((ret = xof_stream_init(&y_stream, seed_y, NSS_HQC_I_SEED_BYTES,
                               3u * NSS_HQC_W_SK + 4096u)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight1_stream(x, &x_stream, NSS_HQC_W_SK)) != NSS_OK) goto cleanup;
    if ((ret = sample_fixed_weight1_stream(y, &y_stream, NSS_HQC_W_SK)) != NSS_OK) goto cleanup;
    ring_mul_by_sparse(hy, h, y);
    xor_n(pk + NSS_HQC_PK_SEED_BYTES, x, hy);
    memcpy(sk + NSS_HQC_SEED_SK_BYTES + NSS_HQC_SIGMA_BYTES, pk, NSS_HQC_PK_BYTES);
    *pk_len = NSS_HQC_PK_BYTES;
    *sk_len = NSS_HQC_SK_BYTES;
    ret = NSS_OK;

cleanup:
    xof_stream_clear(&y_stream);
    xof_stream_clear(&x_stream);
    memset(seed_y, 0, sizeof(seed_y));
    memset(seed_x, 0, sizeof(seed_x));
    free(hy);
    free(y);
    free(x);
    free(h);
    return ret;
}

int nss_hqc_enc(const unsigned char *pk, unsigned long long pk_len,
                unsigned char *ss, unsigned long long *ss_len,
                unsigned char *ct, unsigned long long *ct_len)
{
    uint8_t m[NSS_HQC_MSG_BYTES];
    uint8_t theta[NSS_HQC_THETA_BYTES];
    uint8_t *ct_inner;
    uint8_t *salt;
    int ret;

    if (pk == NULL || ss == NULL || ss_len == NULL || ct == NULL || ct_len == NULL) {
        return NSS_ERR_ARG;
    }
    if (pk_len != NSS_HQC_PK_BYTES) {
        return NSS_ERR_ARG;
    }
    ct_inner = ct;
    salt = ct + NSS_HQC_CT_BYTES;
    if ((ret = random_bytes(m, NSS_HQC_MSG_BYTES)) != NSS_OK) return ret;
    if ((ret = random_bytes(salt, NSS_HQC_SALT_BYTES)) != NSS_OK) return ret;
    if ((ret = hash_g(theta, pk, m, salt)) != NSS_OK) return ret;
    if ((ret = pke_encrypt(ct_inner, pk, m, theta, salt)) != NSS_OK) return ret;
    if ((ret = hash_kappa(ss, m, NSS_HQC_MSG_BYTES, ct)) != NSS_OK) return ret;
    *ss_len = NSS_HQC_SS_BYTES;
    *ct_len = NSS_HQC_CT_FULL_BYTES;
    return NSS_OK;
}

int nss_hqc_dec(const unsigned char *sk, unsigned long long sk_len,
                const unsigned char *ct, unsigned long long ct_len,
                unsigned char *ss, unsigned long long *ss_len)
{
    const uint8_t *sigma;
    const uint8_t *pk;
    const uint8_t *salt;
    uint8_t m[NSS_HQC_MSG_BYTES];
    uint8_t theta[NSS_HQC_THETA_BYTES];
    uint8_t *ct_check;
    uint8_t accept[NSS_HQC_SS_BYTES];
    uint8_t reject[NSS_HQC_SS_BYTES];
    uint8_t ok;
    int ret;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len == NULL) {
        return NSS_ERR_ARG;
    }
    if (sk_len != NSS_HQC_SK_BYTES || ct_len != NSS_HQC_CT_FULL_BYTES) {
        return NSS_ERR_ARG;
    }
    sigma = sk + NSS_HQC_SEED_SK_BYTES;
    pk = sk + NSS_HQC_SEED_SK_BYTES + NSS_HQC_SIGMA_BYTES;
    salt = ct + NSS_HQC_CT_BYTES;

    ret = pke_decrypt(m, sk, ct, salt);
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] pke_decrypt ret = %d\n", ret);
#endif
    if (ret != NSS_OK) {
        memset(m, 0, sizeof(m));
    }
#ifdef NSS_HQC_DEBUG_KAT
    debug_print_bytes("m[0..15] = ", m, NSS_HQC_MSG_BYTES < 16u ? NSS_HQC_MSG_BYTES : 16u);
#endif
    if ((ret = hash_g(theta, pk, m, salt)) != NSS_OK) {
        return ret;
    }
#ifdef NSS_HQC_DEBUG_KAT
    debug_print_bytes("theta[0..15] = ", theta, 16u);
#endif
    ct_check = (uint8_t *)calloc(NSS_HQC_CT_BYTES, 1u);
    if (ct_check == NULL) {
        return NSS_ERR_ALLOC;
    }
    ret = pke_encrypt(ct_check, pk, m, theta, salt);
    ok = (uint8_t)(ret == NSS_OK && ct_verify(ct_check, ct, NSS_HQC_CT_BYTES) == 0);
#ifdef NSS_HQC_DEBUG_KAT
    fprintf(stderr, "[debug] pke_encrypt ret = %d\n", ret);
    debug_print_ct_diff(ct, ct_check);
    fprintf(stderr, "[debug] FO ok = %u\n", (unsigned)ok);
#endif
    free(ct_check);

    if ((ret = hash_kappa(accept, m, NSS_HQC_MSG_BYTES, ct)) != NSS_OK) {
        return ret;
    }
    if ((ret = hash_kappa(reject, sigma, NSS_HQC_SIGMA_BYTES, ct)) != NSS_OK) {
        return ret;
    }
    ct_select(ss, accept, reject, NSS_HQC_SS_BYTES, ok);
    *ss_len = NSS_HQC_SS_BYTES;
    return ok ? NSS_OK : NSS_ERR_DECAP;
}

int nss_hqc_selftest_pke(void)
{
    uint8_t *pk = (uint8_t *)calloc(NSS_HQC_PK_BYTES, 1u);
    uint8_t *sk = (uint8_t *)calloc(NSS_HQC_SK_BYTES, 1u);
    uint8_t *ct = (uint8_t *)calloc(NSS_HQC_CT_BYTES, 1u);
    uint8_t m[NSS_HQC_MSG_BYTES];
    uint8_t out[NSS_HQC_MSG_BYTES];
    uint8_t theta[NSS_HQC_THETA_BYTES];
    uint8_t salt[NSS_HQC_SALT_BYTES];
    unsigned long long pk_len = 0u;
    unsigned long long sk_len = 0u;
    uint32_t i;
    uint8_t diff = 0u;
    int key_ret;
    int enc_ret;
    int dec_ret;
    int ok = 1;

    if (pk == NULL || sk == NULL || ct == NULL) {
        free(pk);
        free(sk);
        free(ct);
        return 1;
    }
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        m[i] = (uint8_t)(0x42u + 13u * i);
    }
    for (i = 0u; i < sizeof(theta); i++) {
        theta[i] = (uint8_t)(0x90u + 7u * i);
    }
    for (i = 0u; i < NSS_HQC_SALT_BYTES; i++) {
        salt[i] = (uint8_t)(0x20u + 5u * i);
    }
    memset(out, 0, sizeof(out));
    key_ret = nss_hqc_keygen(pk, &pk_len, sk, &sk_len);
    ok &= key_ret == NSS_OK;
    ok &= pk_len == NSS_HQC_PK_BYTES;
    ok &= sk_len == NSS_HQC_SK_BYTES;
    enc_ret = pke_encrypt(ct, pk, m, theta, salt);
    dec_ret = pke_decrypt(out, sk, ct, salt);
    ok &= enc_ret == NSS_OK;
    ok &= dec_ret == NSS_OK;
    for (i = 0u; i < NSS_HQC_MSG_BYTES; i++) {
        diff |= (uint8_t)(m[i] ^ out[i]);
    }
    ok &= diff == 0u;
#ifdef NSS_HQC_DEBUG_KAT
    if (!ok) {
        fprintf(stderr,
                "[debug] pke selftest key_ret=%d enc_ret=%d dec_ret=%d pk_len=%llu sk_len=%llu msg_diff=%u\n",
                key_ret, enc_ret, dec_ret, pk_len, sk_len, (unsigned)diff);
        debug_print_bytes("pke selftest m = ", m, NSS_HQC_MSG_BYTES);
        debug_print_bytes("pke selftest out = ", out, NSS_HQC_MSG_BYTES);
    }
#endif
    free(pk);
    free(sk);
    free(ct);
    return ok ? 0 : 1;
}

int nss_hqc_selftest_kem(void)
{
    uint8_t *pk = (uint8_t *)calloc(NSS_HQC_PK_BYTES, 1u);
    uint8_t *sk = (uint8_t *)calloc(NSS_HQC_SK_BYTES, 1u);
    uint8_t *ct = (uint8_t *)calloc(NSS_HQC_CT_FULL_BYTES, 1u);
    uint8_t *tmp = (uint8_t *)calloc(NSS_HQC_CT_FULL_BYTES, 1u);
    uint8_t ss1[NSS_HQC_SS_BYTES];
    uint8_t ss2[NSS_HQC_SS_BYTES];
    unsigned long long pk_len = 0u, sk_len = 0u, ct_len = 0u, ss_len = 0u;
    uint32_t tamper[3] = {0u, NSS_HQC_N_BYTES, NSS_HQC_CT_BYTES};
    uint32_t i;
    int ok = 1;
    if (pk == NULL || sk == NULL || ct == NULL || tmp == NULL) {
        free(pk); free(sk); free(ct); free(tmp);
        return 1;
    }
    ok &= nss_hqc_keygen(pk, &pk_len, sk, &sk_len) == NSS_OK;
    ok &= nss_hqc_enc(pk, pk_len, ss1, &ss_len, ct, &ct_len) == NSS_OK;
    ok &= ss_len == NSS_HQC_SS_BYTES && ct_len == NSS_HQC_CT_FULL_BYTES;
    memset(ss2, 0, sizeof(ss2));
    ok &= nss_hqc_dec(sk, sk_len, ct, ct_len, ss2, &ss_len) == NSS_OK;
    ok &= ct_verify(ss1, ss2, NSS_HQC_SS_BYTES) == 0;
    for (i = 0u; i < 3u; i++) {
        memcpy(tmp, ct, NSS_HQC_CT_FULL_BYTES);
        tmp[tamper[i]] ^= 1u;
        memset(ss2, 0, sizeof(ss2));
        ok &= nss_hqc_dec(sk, sk_len, tmp, ct_len, ss2, &ss_len) == NSS_ERR_DECAP;
        ok &= ct_verify(ss1, ss2, NSS_HQC_SS_BYTES) != 0;
    }
    free(pk); free(sk); free(ct); free(tmp);
    return ok ? 0 : 1;
}

static uint32_t hamming_weight(const uint8_t *a, uint32_t bits)
{
    uint32_t i;
    uint32_t w = 0u;
    for (i = 0u; i < bits; i++) {
        w += bit_get(a, i);
    }
    return w;
}

int nss_hqc_selftest_ring(void)
{
    uint8_t *dense = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *sparse = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *out = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    int ok = 1;
    if (dense == NULL || sparse == NULL || out == NULL) {
        free(dense);
        free(sparse);
        free(out);
        return 1;
    }
    bit_set(dense, 0u, 1u);
    bit_set(dense, 2u, 1u);
    bit_set(dense, 5u, 1u);
    bit_set(sparse, 0u, 1u);
    bit_set(sparse, 1u, 1u);
    ring_mul_by_sparse(out, dense, sparse);
    ok &= bit_get(out, 0u) == 1u;
    ok &= bit_get(out, 1u) == 1u;
    ok &= bit_get(out, 2u) == 1u;
    ok &= bit_get(out, 3u) == 1u;
    ok &= bit_get(out, 5u) == 1u;
    ok &= bit_get(out, 6u) == 1u;
    ok &= hamming_weight(out, NSS_HQC_N) == 6u;
    free(dense);
    free(sparse);
    free(out);
    return ok ? 0 : 1;
}

int nss_hqc_selftest_sample(void)
{
    uint8_t seed[32];
    uint8_t *a = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    uint8_t *b = (uint8_t *)calloc(NSS_HQC_N_BYTES, 1u);
    xof_stream_t st_a = { 0 };
    xof_stream_t st_b = { 0 };
    uint32_t i;
    int ok;
    if (a == NULL || b == NULL) {
        free(a);
        free(b);
        return 1;
    }
    for (i = 0u; i < sizeof(seed); i++) {
        seed[i] = (uint8_t)i;
    }
    ok = xof_stream_init(&st_a, seed, sizeof(seed), 3u * NSS_HQC_W_SK + 4096u) == NSS_OK;
    ok &= xof_stream_init(&st_b, seed, sizeof(seed), 3u * NSS_HQC_W_SK + 4096u) == NSS_OK;
    ok &= sample_fixed_weight1_stream(a, &st_a, NSS_HQC_W_SK) == NSS_OK;
    ok &= sample_fixed_weight1_stream(b, &st_b, NSS_HQC_W_SK) == NSS_OK;
    ok &= ct_verify(a, b, NSS_HQC_N_BYTES) == 0;
    ok &= hamming_weight(a, NSS_HQC_N) == NSS_HQC_W_SK;
    xof_stream_clear(&st_a);
    xof_stream_clear(&st_b);
    free(a);
    free(b);
    return ok ? 0 : 1;
}

int nss_hqc_selftest_quant(void)
{
    uint8_t *bits = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t *decoded = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t *vhat = (uint8_t *)calloc(NSS_HQC_VHAT_BYTES, 1u);
    uint32_t block;
    uint32_t constant_err = 0u;
    uint32_t quant_err = 0u;
    int ok = 1;
    if (bits == NULL || decoded == NULL || vhat == NULL) {
        free(bits);
        free(decoded);
        free(vhat);
        return 1;
    }
    for (block = 0u; block < NSS_HQC_NC / NSS_HQC_MULT; block++) {
        uint32_t j;
        uint8_t fill = (uint8_t)(block & 1u);
        for (j = 0u; j < NSS_HQC_MULT; j++) {
            bit_set(bits, block * NSS_HQC_MULT + j, fill);
        }
    }
    quant_compress(vhat, bits);
    quant_decompress(decoded, vhat);
    ok &= ct_verify(bits, decoded, NSS_HQC_NC_BYTES) == 0;
    for (block = 0u; block < NSS_HQC_NC; block++) {
        constant_err += bit_get(bits, block) ^ bit_get(decoded, block);
    }
    memset(bits, 0, NSS_HQC_NC_BYTES);
    memset(decoded, 0, NSS_HQC_NC_BYTES);
    memset(vhat, 0, NSS_HQC_VHAT_BYTES);
    for (block = 0u; block < NSS_HQC_NC / NSS_HQC_MULT; block++) {
        bit_set(bits, block * NSS_HQC_MULT, 1u);
    }
    quant_compress(vhat, bits);
    quant_decompress(decoded, vhat);
    for (block = 0u; block < NSS_HQC_NC; block++) {
        quant_err += bit_get(bits, block) ^ bit_get(decoded, block);
    }
    printf("[diag][quant] vhat_bits=%lu vhat_bytes=%lu nc_bits=%lu nc_bytes=%lu constant_block_roundtrip_error=%lu\n",
           (unsigned long)NSS_HQC_CT_V_BITS,
           (unsigned long)NSS_HQC_VHAT_BYTES,
           (unsigned long)NSS_HQC_NC,
           (unsigned long)NSS_HQC_NC_BYTES,
           (unsigned long)constant_err);
    printf("[diag][quant] one_hot_blocks=%lu e_quant_weight_one_hot=%lu\n",
           (unsigned long)(NSS_HQC_NC / NSS_HQC_MULT),
           (unsigned long)quant_err);
    memset(bits, 0, NSS_HQC_NC_BYTES);
    memset(vhat, 0, NSS_HQC_VHAT_BYTES);
    bit_set(bits, 1u * NSS_HQC_MULT + 0u, 1u);
    bit_set(bits, 1u * NSS_HQC_MULT + 1u, 1u);
    bit_set(bits, 2u * NSS_HQC_MULT + 0u, 1u);
    bit_set(bits, 2u * NSS_HQC_MULT + 1u, 1u);
    bit_set(bits, 2u * NSS_HQC_MULT + 2u, 1u);
    bit_set(bits, 3u * NSS_HQC_MULT + 0u, 1u);
    bit_set(bits, 3u * NSS_HQC_MULT + 1u, 1u);
    bit_set(bits, 3u * NSS_HQC_MULT + 2u, 1u);
    bit_set(bits, 3u * NSS_HQC_MULT + 3u, 1u);
    bit_set(bits, 3u * NSS_HQC_MULT + 4u, 1u);
    quant_compress(vhat, bits);
    ok &= vhat[0] == 0xe4u;
    printf("[diag][vhat] lsb_pack_q0123_byte0=%02x expected=e4 vhat_bytes=%lu\n",
           (unsigned int)vhat[0],
           (unsigned long)NSS_HQC_VHAT_BYTES);
    free(bits);
    free(decoded);
    free(vhat);
    return ok ? 0 : 1;
}

int nss_hqc_selftest_dither(void)
{
    uint8_t *pk = (uint8_t *)calloc(NSS_HQC_PK_BYTES, 1u);
    uint8_t *base = (uint8_t *)calloc((NSS_HQC_N1 * 128u + 7u) / 8u, 1u);
    uint8_t *a = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t *b = (uint8_t *)calloc(NSS_HQC_NC_BYTES, 1u);
    uint8_t salt[NSS_HQC_SALT_BYTES];
    uint32_t i;
    uint32_t first32 = 0u;
    int ok;
    if (pk == NULL || base == NULL || a == NULL || b == NULL) {
        free(pk);
        free(base);
        free(a);
        free(b);
        return 1;
    }
    for (i = 0u; i < NSS_HQC_PK_BYTES; i++) {
        pk[i] = (uint8_t)(i * 3u + 1u);
    }
    for (i = 0u; i < NSS_HQC_SALT_BYTES; i++) {
        salt[i] = (uint8_t)(i * 5u + 7u);
    }
    ok = derive_dither_base(base, NSS_HQC_N1 * 128u, salt, pk) == NSS_OK;
    for (i = 0u; i < 32u; i++) {
        first32 |= ((uint32_t)bit_get(base, i)) << i;
    }
    ok &= derive_dither(a, salt, pk) == NSS_OK;
    ok &= derive_dither(b, salt, pk) == NSS_OK;
    ok &= ct_verify(a, b, NSS_HQC_NC_BYTES) == 0;
    for (i = 0u; i < NSS_HQC_N1 * 128u && i < 64u; i++) {
        uint32_t rep;
        for (rep = 0u; rep < NSS_HQC_MULT; rep++) {
            ok &= bit_get(a, i * NSS_HQC_MULT + rep) == bit_get(base, i);
        }
    }
    salt[0] ^= 1u;
    ok &= derive_dither(b, salt, pk) == NSS_OK;
    ok &= ct_verify(a, b, NSS_HQC_NC_BYTES) != 0;
    printf("[diag][dither] base_bits=%lu expanded_bits=%lu base_bytes=%lu expanded_bytes=%lu\n",
           (unsigned long)(NSS_HQC_N1 * 128u),
           (unsigned long)NSS_HQC_NC,
           (unsigned long)((NSS_HQC_N1 * 128u + 7u) / 8u),
           (unsigned long)NSS_HQC_NC_BYTES);
    printf("[diag][dither] first32_base_bits_lsb=0x%08lx\n",
           (unsigned long)first32);
    free(pk);
    free(base);
    free(a);
    free(b);
    return ok ? 0 : 1;
}



