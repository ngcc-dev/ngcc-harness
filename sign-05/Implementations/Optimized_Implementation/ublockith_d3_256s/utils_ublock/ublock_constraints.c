#include "ublock_constraints.h"

#include "ublock.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

/* Byte permutation form of PL/PR on one 128-bit branch (16 bytes). */
static inline void ublock_perm_PL_bytes(uint8_t out[16], const uint8_t in[16])
{
    static const uint8_t p[16] = {
        2, 7, 8, 13, 3, 6, 9, 12, 1, 4, 15, 10, 14, 11, 5, 0
    };
    for (unsigned int i = 0; i < 16; ++i) {
        out[i] = in[p[i]];
    }
}

static inline void ublock_perm_PR_bytes(uint8_t out[16], const uint8_t in[16])
{
    static const uint8_t p[16] = {
        6, 11, 1, 12, 9, 4, 2, 15, 7, 0, 13, 10, 14, 3, 8, 5
    };
    for (unsigned int i = 0; i < 16; ++i) {
        out[i] = in[p[i]];
    }
}

/* Commitment-bit permutation form on one 128-bit branch (16 bytes). */
static inline void ublock_perm_PL_bf256(bf256_t out[128], const bf256_t in[128])
{
    static const uint8_t p[16] = {
        2, 7, 8, 13, 3, 6, 9, 12, 1, 4, 15, 10, 14, 11, 5, 0
    };
    for (unsigned int i = 0; i < 16; ++i) {
        const unsigned int src = 8u * (unsigned int)p[i];
        const unsigned int dst = 8u * i;
        for (unsigned int b = 0; b < 8; ++b) {
            out[dst + b] = in[src + b];
        }
    }
}

static inline void ublock_perm_PR_bf256(bf256_t out[128], const bf256_t in[128])
{
    static const uint8_t p[16] = {
        6, 11, 1, 12, 9, 4, 2, 15, 7, 0, 13, 10, 14, 3, 8, 5
    };
    for (unsigned int i = 0; i < 16; ++i) {
        const unsigned int src = 8u * (unsigned int)p[i];
        const unsigned int dst = 8u * i;
        for (unsigned int b = 0; b < 8; ++b) {
            out[dst + b] = in[src + b];
        }
    }
}

static inline void ublock_perm_PL_inv_bytes(uint8_t out[16], const uint8_t in[16])
{
    static const uint8_t p_inv[16] = {
        15, 8, 0, 4, 9, 14, 5, 1, 2, 6, 11, 13, 7, 3, 12, 10
    };
    for (unsigned int i = 0; i < 16; ++i) {
        out[i] = in[p_inv[i]];
    }
}

static inline void ublock_perm_PR_inv_bytes(uint8_t out[16], const uint8_t in[16])
{
    static const uint8_t p_inv[16] = {
        9, 2, 6, 13, 5, 15, 0, 8, 14, 4, 11, 1, 3, 10, 12, 7
    };
    for (unsigned int i = 0; i < 16; ++i) {
        out[i] = in[p_inv[i]];
    }
}

static inline void ublock_perm_PL_inv_bf256(bf256_t out[128], const bf256_t in[128])
{
    static const uint8_t p_inv[16] = {
        15, 8, 0, 4, 9, 14, 5, 1, 2, 6, 11, 13, 7, 3, 12, 10
    };
    for (unsigned int i = 0; i < 16; ++i) {
        const unsigned int src = 8u * (unsigned int)p_inv[i];
        const unsigned int dst = 8u * i;
        for (unsigned int b = 0; b < 8; ++b) {
            out[dst + b] = in[src + b];
        }
    }
}

static inline void ublock_perm_PR_inv_bf256(bf256_t out[128], const bf256_t in[128])
{
    static const uint8_t p_inv[16] = {
        9, 2, 6, 13, 5, 15, 0, 8, 14, 4, 11, 1, 3, 10, 12, 7
    };
    for (unsigned int i = 0; i < 16; ++i) {
        const unsigned int src = 8u * (unsigned int)p_inv[i];
        const unsigned int dst = 8u * i;
        for (unsigned int b = 0; b < 8; ++b) {
            out[dst + b] = in[src + b];
        }
    }
}

static inline void ublock_rotl32_bytes(uint8_t out[16], const uint8_t in[16], unsigned int r)
{
    for (unsigned int w = 0; w < 4; ++w) {
        const uint32_t x = ((uint32_t)in[4u * w + 0u] << 24) |
                           ((uint32_t)in[4u * w + 1u] << 16) |
                           ((uint32_t)in[4u * w + 2u] << 8) |
                           (uint32_t)in[4u * w + 3u];
        const uint32_t y = rotl32_u(x, (uint8_t)r);
        out[4u * w + 0u] = (uint8_t)(y >> 24);
        out[4u * w + 1u] = (uint8_t)(y >> 16);
        out[4u * w + 2u] = (uint8_t)(y >> 8);
        out[4u * w + 3u] = (uint8_t)y;
    }
}

static inline void ublock_rotl32_bf256(bf256_t out[128], const bf256_t in[128], unsigned int r)
{
    for (unsigned int w = 0; w < 4; ++w) {
        const unsigned int word_off = 32u * w;
        for (unsigned int out_byte = 0; out_byte < 4; ++out_byte) {
            for (unsigned int out_bl = 0; out_bl < 8; ++out_bl) {
                const unsigned int p_out = (3u - out_byte) * 8u + out_bl;
                const unsigned int p_in  = (p_out + 32u - r) & 31u;
                const unsigned int in_byte = 3u - (p_in >> 3);
                const unsigned int in_bl   = p_in & 7u;
                out[word_off + 8u * out_byte + out_bl] = in[word_off + 8u * in_byte + in_bl];
            }
        }
    }
}

static inline void ublock_xor_bf256_128(bf256_t dst[128], const bf256_t src[128])
{
    for (unsigned int i = 0; i < 128; ++i) {
        dst[i] = bf256_add(dst[i], src[i]);
    }
}

static inline void ublock_deg3_xor_inplace(uint8_t a_bits[16],
                                           bf256_t a_d0[128],
                                           bf256_t a_d1[128],
                                           bf256_t a_d2[128],
                                           const uint8_t b_bits[16],
                                           const bf256_t b_d0[128],
                                           const bf256_t b_d1[128],
                                           const bf256_t b_d2[128])
{
    xor_u8_array(a_bits, b_bits, a_bits, 16);
    for (unsigned int i = 0; i < 128; ++i) {
        a_d0[i] = bf256_add(a_d0[i], b_d0[i]);
        a_d1[i] = bf256_add(a_d1[i], b_d1[i]);
        a_d2[i] = bf256_add(a_d2[i], b_d2[i]);
    }
}

static inline void ublock_deg3_rotl32_inplace(uint8_t bits[16],
                                              bf256_t d0[128],
                                              bf256_t d1[128],
                                              bf256_t d2[128],
                                              unsigned int r)
{
    uint8_t bits_tmp[16];
    bf256_t d0_tmp[128], d1_tmp[128], d2_tmp[128];
    ublock_rotl32_bytes(bits_tmp, bits, r);
    ublock_rotl32_bf256(d0_tmp, d0, r);
    ublock_rotl32_bf256(d1_tmp, d1, r);
    ublock_rotl32_bf256(d2_tmp, d2, r);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(d0, d0_tmp, sizeof(d0_tmp));
    memcpy(d1, d1_tmp, sizeof(d1_tmp));
    memcpy(d2, d2_tmp, sizeof(d2_tmp));
}

static inline void ublock_deg3_perm_PL_inplace(uint8_t bits[16],
                                                bf256_t d0[128],
                                                bf256_t d1[128],
                                                bf256_t d2[128])
{
    uint8_t bits_tmp[16];
    bf256_t d0_tmp[128], d1_tmp[128], d2_tmp[128];
    ublock_perm_PL_bytes(bits_tmp, bits);
    ublock_perm_PL_bf256(d0_tmp, d0);
    ublock_perm_PL_bf256(d1_tmp, d1);
    ublock_perm_PL_bf256(d2_tmp, d2);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(d0, d0_tmp, sizeof(d0_tmp));
    memcpy(d1, d1_tmp, sizeof(d1_tmp));
    memcpy(d2, d2_tmp, sizeof(d2_tmp));
}

static inline void ublock_deg3_perm_PR_inplace(uint8_t bits[16],
                                                bf256_t d0[128],
                                                bf256_t d1[128],
                                                bf256_t d2[128])
{
    uint8_t bits_tmp[16];
    bf256_t d0_tmp[128], d1_tmp[128], d2_tmp[128];
    ublock_perm_PR_bytes(bits_tmp, bits);
    ublock_perm_PR_bf256(d0_tmp, d0);
    ublock_perm_PR_bf256(d1_tmp, d1);
    ublock_perm_PR_bf256(d2_tmp, d2);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(d0, d0_tmp, sizeof(d0_tmp));
    memcpy(d1, d1_tmp, sizeof(d1_tmp));
    memcpy(d2, d2_tmp, sizeof(d2_tmp));
}

static inline void ublock_deg1_rotl32_inplace(uint8_t bits[16], bf256_t tag[128], unsigned int r)
{
    uint8_t bits_tmp[16];
    bf256_t tag_tmp[128];
    ublock_rotl32_bytes(bits_tmp, bits, r);
    ublock_rotl32_bf256(tag_tmp, tag, r);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(tag, tag_tmp, sizeof(tag_tmp));
}

static inline void ublock_deg1_perm_PL_inv_inplace(uint8_t bits[16], bf256_t tag[128])
{
    uint8_t bits_tmp[16];
    bf256_t tag_tmp[128];
    ublock_perm_PL_inv_bytes(bits_tmp, bits);
    ublock_perm_PL_inv_bf256(tag_tmp, tag);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(tag, tag_tmp, sizeof(tag_tmp));
}

static inline void ublock_deg1_perm_PR_inv_inplace(uint8_t bits[16], bf256_t tag[128])
{
    uint8_t bits_tmp[16];
    bf256_t tag_tmp[128];
    ublock_perm_PR_inv_bytes(bits_tmp, bits);
    ublock_perm_PR_inv_bf256(tag_tmp, tag);
    memcpy(bits, bits_tmp, sizeof(bits_tmp));
    memcpy(tag, tag_tmp, sizeof(tag_tmp));
}

static inline void ublock_key_rotl32_inplace(bf256_t key[128], unsigned int r)
{
    bf256_t tmp[128];
    ublock_rotl32_bf256(tmp, key, r);
    memcpy(key, tmp, sizeof(tmp));
}

static inline void ublock_key_perm_PL_inplace(bf256_t key[128])
{
    bf256_t tmp[128];
    ublock_perm_PL_bf256(tmp, key);
    memcpy(key, tmp, sizeof(tmp));
}

static inline void ublock_key_perm_PR_inplace(bf256_t key[128])
{
    bf256_t tmp[128];
    ublock_perm_PR_bf256(tmp, key);
    memcpy(key, tmp, sizeof(tmp));
}

static inline void ublock_key_perm_PL_inv_inplace(bf256_t key[128])
{
    bf256_t tmp[128];
    ublock_perm_PL_inv_bf256(tmp, key);
    memcpy(key, tmp, sizeof(tmp));
}

static inline void ublock_key_perm_PR_inv_inplace(bf256_t key[128])
{
    bf256_t tmp[128];
    ublock_perm_PR_inv_bf256(tmp, key);
    memcpy(key, tmp, sizeof(tmp));
}

/* In-place PK permutation on K0||K1 (each 8 bytes). */
static void ublock_pk_permute_bytes(uint8_t k0[8], uint8_t k1[8])
{
    uint8_t in16[16], out16[16];
    uint8_t nib[32], out_nib[32];

    memcpy(in16, k0, 8);
    memcpy(in16 + 8, k1, 8);

    for (unsigned int i = 0; i < 16; ++i) {
        nib[2 * i]     = (uint8_t)((in16[i] >> 4) & 0x0Fu); /* high nibble */
        nib[2 * i + 1] = (uint8_t)(in16[i] & 0x0Fu);        /* low nibble */
    }
    for (unsigned int j = 0; j < 32; ++j) {
        out_nib[j] = nib[pk[j]];
    }
    for (unsigned int i = 0; i < 16; ++i) {
        out16[i] = (uint8_t)((out_nib[2 * i] << 4) | out_nib[2 * i + 1]);
    }

    memcpy(k0, out16, 8);
    memcpy(k1, out16 + 8, 8);
}

/* In-place PK permutation on tag/key bits for K0||K1 (each 64 bits). */
static void ublock_pk_permute_bf256(bf256_t k0[64], bf256_t k1[64])
{
    bf256_t nib[32][4], out_nib[32][4];
    bf256_t out0[64], out1[64];

    for (unsigned int byte_i = 0; byte_i < 16; ++byte_i) {
        const bf256_t* src = (byte_i < 8) ? k0 : k1;
        const unsigned int bit_off = (byte_i & 7u) * 8u;

        /* high nibble */
        for (unsigned int b = 0; b < 4; ++b) {
            nib[2 * byte_i][b] = src[bit_off + 4u + b];
        }
        /* low nibble */
        for (unsigned int b = 0; b < 4; ++b) {
            nib[2 * byte_i + 1][b] = src[bit_off + b];
        }
    }

    for (unsigned int j = 0; j < 32; ++j) {
        for (unsigned int b = 0; b < 4; ++b) {
            out_nib[j][b] = nib[pk[j]][b];
        }
    }

    for (unsigned int byte_i = 0; byte_i < 16; ++byte_i) {
        bf256_t* dst = (byte_i < 8) ? out0 : out1;
        const unsigned int bit_off = (byte_i & 7u) * 8u;

        for (unsigned int b = 0; b < 4; ++b) {
            dst[bit_off + 4u + b] = out_nib[2 * byte_i][b];
            dst[bit_off + b]      = out_nib[2 * byte_i + 1][b];
        }
    }

    memcpy(k0, out0, 64 * sizeof(bf256_t));
    memcpy(k1, out1, 64 * sizeof(bf256_t));
}

static void ublock_store_round_key_prover(uint8_t* k_out, bf256_t* k_out_tag, unsigned int round_idx,
                                          const uint8_t k0[8], const uint8_t k1[8],
                                          const uint8_t k2[8], const uint8_t k3[8],
                                          const bf256_t k0_tag[64], const bf256_t k1_tag[64],
                                          const bf256_t k2_tag[64], const bf256_t k3_tag[64])
{
    uint8_t* k_bytes = k_out + 32u * round_idx;
    bf256_t* k_tags  = k_out_tag + 256u * round_idx;

    memcpy(k_bytes,      k0, 8);
    memcpy(k_bytes + 8,  k1, 8);
    memcpy(k_bytes + 16, k2, 8);
    memcpy(k_bytes + 24, k3, 8);

    memcpy(k_tags,       k0_tag, 64 * sizeof(bf256_t));
    memcpy(k_tags + 64,  k1_tag, 64 * sizeof(bf256_t));
    memcpy(k_tags + 128, k2_tag, 64 * sizeof(bf256_t));
    memcpy(k_tags + 192, k3_tag, 64 * sizeof(bf256_t));
}

static void ublock_store_round_key_verifier(bf256_t* k_out_key, unsigned int round_idx,
                                            const bf256_t k0_key[64], const bf256_t k1_key[64],
                                            const bf256_t k2_key[64], const bf256_t k3_key[64])
{
    bf256_t* k_keys = k_out_key + 256u * round_idx;

    memcpy(k_keys,       k0_key, 64 * sizeof(bf256_t));
    memcpy(k_keys + 64,  k1_key, 64 * sizeof(bf256_t));
    memcpy(k_keys + 128, k2_key, 64 * sizeof(bf256_t));
    memcpy(k_keys + 192, k3_key, 64 * sizeof(bf256_t));
}

static inline uint8_t reverse_nibble_bits(uint8_t x)
{
    return (uint8_t)(((x & 0x1u) << 3) |
                     ((x & 0x2u) << 1) |
                     ((x & 0x4u) >> 1) |
                     ((x & 0x8u) >> 3));
}

/* Bit order follows ptr_get_bit/ptr_set_bit semantics: bit 0 is LSB of byte. */
static inline uint8_t get_nibble_le(const uint8_t* x, unsigned int nibble_idx)
{
    const uint8_t byte = x[nibble_idx >> 1];
    const unsigned int shift = (nibble_idx & 1u) ? 4u : 0u;
    return (uint8_t)((byte >> shift) & 0x0Fu);
}

static inline void set_nibble_le(uint8_t* x, unsigned int nibble_idx, uint8_t nibble)
{
    uint8_t* byte = &x[nibble_idx >> 1];
    const unsigned int shift = (nibble_idx & 1u) ? 4u : 0u;
    const uint8_t mask = (uint8_t)(0x0Fu << shift);
    *byte = (uint8_t)((*byte & (uint8_t)~mask) | (uint8_t)((nibble & 0x0Fu) << shift));
}

static void ublock_SSS_subword_prover(uint8_t* y,
                               bf256_t* y_tag_deg0,
                               bf256_t* y_tag_deg1,
                               bf256_t* y_tag_deg2,
                               const uint8_t* x,
                               const bf256_t* x_tag,
                               unsigned int n)
{
    assert(y);
    assert(y_tag_deg0);
    assert(y_tag_deg1);
    assert(y_tag_deg2);
    assert(x);
    assert(x_tag);

    for (unsigned int i = 0; i < n; ++i) {
        const unsigned int base = 4u * i;
        const uint8_t x_nibble = get_nibble_le(x, i);

        bf256_t x_tag_nibble[4];
        x_tag_nibble[0] = x_tag[base + 0];
        x_tag_nibble[1] = x_tag[base + 1];
        x_tag_nibble[2] = x_tag[base + 2];
        x_tag_nibble[3] = x_tag[base + 3];

        const uint8_t y_nibble = ublock_subword_nibble(x_nibble);

        bf256_t d0[4], d1[4], d2[4];
        ublock_subword_nibble_tag(d0, d1, d2, x_tag_nibble, x_nibble);

        set_nibble_le(y, i, y_nibble);
        y_tag_deg0[base + 0u] = d0[0];
        y_tag_deg0[base + 1u] = d0[1];
        y_tag_deg0[base + 2u] = d0[2];
        y_tag_deg0[base + 3u] = d0[3];
        y_tag_deg1[base + 0u] = d1[0];
        y_tag_deg1[base + 1u] = d1[1];
        y_tag_deg1[base + 2u] = d1[2];
        y_tag_deg1[base + 3u] = d1[3];
        y_tag_deg2[base + 0u] = d2[0];
        y_tag_deg2[base + 1u] = d2[1];
        y_tag_deg2[base + 2u] = d2[2];
        y_tag_deg2[base + 3u] = d2[3];
    }
}

static void ublock_SSS_subword_verifier(bf256_t* y_key,
                                 const bf256_t* x_key,
                                 bf256_t delta,
                                 unsigned int n)
{
    assert(y_key);
    assert(x_key);

    /* Degree alignment with prover output:
     * y_key = y * Delta^3 + deg2 * Delta^2 + deg1 * Delta + deg0,
     * where (deg2,deg1,deg0) are from ublock_SSS_subword_prover.
     */
    const bf256_t delta2 = bf256_mul(delta, delta);
    const bf256_t delta3 = bf256_mul(delta2, delta);
    for (unsigned int i = 0; i < n; ++i) {
        ublock_subword_nibble_key_pows(y_key + 4u * i, x_key + 4u * i, delta, delta2, delta3);
    }
}

static void ublock_SSS_inv_subword_prover(uint8_t* z,
                                   bf256_t* z_tag_deg0,
                                   bf256_t* z_tag_deg1,
                                   bf256_t* z_tag_deg2,
                                   const uint8_t* y,
                                   const bf256_t* y_tag,
                                   unsigned int n)
{
    assert(z);
    assert(z_tag_deg0);
    assert(z_tag_deg1);
    assert(z_tag_deg2);
    assert(y);
    assert(y_tag);

    for (unsigned int i = 0; i < n; ++i) {
        const unsigned int base = 4u * i;
        const uint8_t y_nibble = get_nibble_le(y, i);

        bf256_t y_tag_nibble[4];
        y_tag_nibble[0] = y_tag[base + 0];
        y_tag_nibble[1] = y_tag[base + 1];
        y_tag_nibble[2] = y_tag[base + 2];
        y_tag_nibble[3] = y_tag[base + 3];

        const uint8_t z_nibble = ublock_invsubword_nibble(y_nibble);

        bf256_t d0[4], d1[4], d2[4];
        ublock_invsubword_nibble_tag(d0, d1, d2, y_tag_nibble, y_nibble);

        set_nibble_le(z, i, z_nibble);
        z_tag_deg0[base + 0u] = d0[0];
        z_tag_deg0[base + 1u] = d0[1];
        z_tag_deg0[base + 2u] = d0[2];
        z_tag_deg0[base + 3u] = d0[3];
        z_tag_deg1[base + 0u] = d1[0];
        z_tag_deg1[base + 1u] = d1[1];
        z_tag_deg1[base + 2u] = d1[2];
        z_tag_deg1[base + 3u] = d1[3];
        z_tag_deg2[base + 0u] = d2[0];
        z_tag_deg2[base + 1u] = d2[1];
        z_tag_deg2[base + 2u] = d2[2];
        z_tag_deg2[base + 3u] = d2[3];
    }
}

static void ublock_SSS_inv_subword_verifier(bf256_t* z_key,
                                     const bf256_t* y_key,
                                     bf256_t delta,
                                     unsigned int n)
{
    assert(z_key);
    assert(y_key);

    const bf256_t delta2 = bf256_mul(delta, delta);
    const bf256_t delta3 = bf256_mul(delta2, delta);
    for (unsigned int i = 0; i < n; ++i) {
        ublock_invsubword_nibble_key_pows(z_key + 4u * i, y_key + 4u * i, delta, delta2, delta3);
    }
}

static void ublock_T_prover(uint8_t x[8], bf256_t x_tag[64])
{
    assert(x);
    assert(x_tag);

    for (unsigned int i = 0; i < 16; ++i) {
        const unsigned int base = 4u * i;

        const uint8_t x_nibble = get_nibble_le(x, i);

        /* ublock_T_nibble uses the opposite per-nibble bit order from
         * ublock_core.c's numeric gf24_mul2; convert by reverse-before/after. */
        bf256_t x_tag_rev[4], y_tag_rev[4];
        x_tag_rev[0] = x_tag[base + 3];
        x_tag_rev[1] = x_tag[base + 2];
        x_tag_rev[2] = x_tag[base + 1];
        x_tag_rev[3] = x_tag[base + 0];

        const uint8_t x_rev_nibble = reverse_nibble_bits(x_nibble);
        const uint8_t y_rev_nibble = ublock_T_nibble(x_rev_nibble);
        const uint8_t y_nibble     = reverse_nibble_bits(y_rev_nibble);
        ublock_T_nibble_tag(y_tag_rev, x_tag_rev);

        set_nibble_le(x, i, y_nibble);
        x_tag[base + 0u] = y_tag_rev[3u];
        x_tag[base + 1u] = y_tag_rev[2u];
        x_tag[base + 2u] = y_tag_rev[1u];
        x_tag[base + 3u] = y_tag_rev[0u];
    }
}

static void ublock_T_verifier(bf256_t x_key[64])
{
    assert(x_key);

    for (unsigned int i = 0; i < 16; ++i) {
        bf256_t x_key_rev[4], y_key_rev[4];
        x_key_rev[0] = x_key[4u * i + 3u];
        x_key_rev[1] = x_key[4u * i + 2u];
        x_key_rev[2] = x_key[4u * i + 1u];
        x_key_rev[3] = x_key[4u * i + 0u];
        ublock_T_nibble_key(y_key_rev, x_key_rev, bf256_zero());
        x_key[4u * i + 0u] = y_key_rev[3];
        x_key[4u * i + 1u] = y_key_rev[2];
        x_key[4u * i + 2u] = y_key_rev[1];
        x_key[4u * i + 3u] = y_key_rev[0];
    }
}

void ublock_SSS_expkey_constraints_prover(uint8_t* k_out,
                                          bf256_t* k_out_tag,
                                          uint8_t* o,
                                          bf256_t* o_tag_deg0,
                                          bf256_t* o_tag_deg1,
                                          bf256_t* o_tag_deg2,
                                          const uint8_t* w,
                                          const bf256_t* w_tag)
{
    assert(k_out);
    assert(k_out_tag);
    assert(o);
    assert(o_tag_deg0);
    assert(o_tag_deg1);
    assert(o_tag_deg2);
    assert(w);
    assert(w_tag);

    uint8_t k0[8], k1[8], k2[8], k3[8];
    bf256_t k0_tag[64], k1_tag[64], k2_tag[64], k3_tag[64];

    memcpy(k0, w, 8);
    memcpy(k1, w + 8, 8);
    memcpy(k2, w + 16, 8);
    memcpy(k3, w + 24, 8);

    memcpy(k0_tag, w_tag, 64 * sizeof(bf256_t));
    memcpy(k1_tag, w_tag + 64, 64 * sizeof(bf256_t));
    memcpy(k2_tag, w_tag + 128, 64 * sizeof(bf256_t));
    memcpy(k3_tag, w_tag + 192, 64 * sizeof(bf256_t));

    // Step 2
    ublock_store_round_key_prover(k_out, k_out_tag, 0, k0, k1, k2, k3, k0_tag, k1_tag, k2_tag, k3_tag);

    // Step 3
    for (unsigned int i = 0; i < UBLOCK_ROUNDS; ++i) {
        uint8_t t0_rc[8];
        uint8_t* const o_round = o + 8u * i;
        bf256_t* const o_d0 = o_tag_deg0 + 64u * i;
        bf256_t* const o_d1 = o_tag_deg1 + 64u * i;
        bf256_t* const o_d2 = o_tag_deg2 + 64u * i;
        uint8_t old_t1[8];
        bf256_t old_t1_tag[64];

        // Step 4
        // k0 <- pk(K0), k1 <- pk(K1), they correspond to t0 and t1 in the paper
        ublock_pk_permute_bytes(k0, k1);
        ublock_pk_permute_bf256(k0_tag, k1_tag);

        // get t0_rc = pk(K0) XOR RC_i
        memcpy(t0_rc, k0, 8);
        {
            const uint32_t rc = UBLOCK_RC[i];
            t0_rc[0] ^= (uint8_t)(rc >> 24);
            t0_rc[1] ^= (uint8_t)(rc >> 16);
            t0_rc[2] ^= (uint8_t)(rc >> 8);
            t0_rc[3] ^= (uint8_t)(rc);
        }

        // Step 5
        memset(o_round, 0, 8);
        ublock_SSS_subword_prover(o_round, o_d0, o_d1, o_d2, t0_rc, k0_tag, 16);

        // Step 6
        // copy t1 to old_t1 for T evaluation, since t1 will be updated to t3 in Step 7
        memcpy(old_t1, k1, 8);
        memcpy(old_t1_tag, k1_tag, 64 * sizeof(bf256_t));
        ublock_T_prover(k1, k1_tag);

        /* t3 = old_k3 + T(t1) */
        for (unsigned int byte_i = 0; byte_i < 8; ++byte_i) {
            k1[byte_i] ^= k3[byte_i];
        }
        for (unsigned int b = 0; b < 64; ++b) {
            k1_tag[b] = bf256_add(k3_tag[b], k1_tag[b]);
        }

        // Step 7
        const uint8_t* k2_wit_bits = w + 32u + 8u * i;
        const bf256_t* k2_wit_tag  = w_tag + 256u + 64u * i;

        // Step 8
        for (unsigned int b = 0; b < 64; ++b) {
            ptr_set_bit(o_round, b,
                        (uint8_t)(ptr_get_bit(k2_wit_bits, b) ^
                                  ptr_get_bit(k2, b) ^
                                  ptr_get_bit(o_round, b)));

            o_d2[b] = bf256_add(k2_wit_tag[b], bf256_add(k2_tag[b], o_d2[b])); /* lift+add */
        }

        // Step 9
        /* Keep t1=tk-permuted K1 and t0=pk-permuted K0 for next state. */
        memcpy(k2, old_t1, 8);
        memcpy(k2_tag, old_t1_tag, 64 * sizeof(bf256_t));
        memcpy(k3, k0, 8);
        memcpy(k3_tag, k0_tag, 64 * sizeof(bf256_t));

        memcpy(k0, k2_wit_bits, 8);
        memcpy(k0_tag, k2_wit_tag, 64 * sizeof(bf256_t));

        ublock_store_round_key_prover(k_out, k_out_tag, i + 1u, k0, k1, k2, k3,
                                      k0_tag, k1_tag, k2_tag, k3_tag);
    }
}

void ublock_SSS_expkey_constraints_verifier(bf256_t* k_out_key,
                                            bf256_t* o_key,
                                            const bf256_t* w_key,
                                            bf256_t delta)
{
    assert(k_out_key);
    assert(o_key);
    assert(w_key);

    const bf256_t delta2 = bf256_mul(delta, delta);

    bf256_t k0_key[64], k1_key[64], k2_key[64], k3_key[64];
    memcpy(k0_key, w_key, 64 * sizeof(bf256_t));
    memcpy(k1_key, w_key + 64, 64 * sizeof(bf256_t));
    memcpy(k2_key, w_key + 128, 64 * sizeof(bf256_t));
    memcpy(k3_key, w_key + 192, 64 * sizeof(bf256_t));

    ublock_store_round_key_verifier(k_out_key, 0, k0_key, k1_key, k2_key, k3_key);

    for (unsigned int i = 0; i < UBLOCK_ROUNDS; ++i) {
        bf256_t x_key[64], old_t1_key[64];
        uint8_t rc_bits[8] = {0};
        bf256_t* const o_round_key = o_key + 64u * i;
        const bf256_t* k2_wit_key = w_key + 256u + 64u * i;

        {
            const uint32_t rc = UBLOCK_RC[i];
            rc_bits[0] = (uint8_t)(rc >> 24);
            rc_bits[1] = (uint8_t)(rc >> 16);
            rc_bits[2] = (uint8_t)(rc >> 8);
            rc_bits[3] = (uint8_t)(rc);
        }

        ublock_pk_permute_bf256(k0_key, k1_key);

        for (unsigned int b = 0; b < 64; ++b) {
            x_key[b] = bf256_add(k0_key[b], bf256_mul_bit(delta, ptr_get_bit(rc_bits, b)));
        }

        ublock_SSS_subword_verifier(o_round_key, x_key, delta, 16);

        memcpy(old_t1_key, k1_key, 64 * sizeof(bf256_t));
        ublock_T_verifier(k1_key);

        for (unsigned int b = 0; b < 64; ++b) {
            o_round_key[b] = bf256_add(bf256_mul(delta2, k2_wit_key[b]),
                                       bf256_add(bf256_mul(delta2, k2_key[b]), o_round_key[b])); /* lift+add */
            k1_key[b] = bf256_add(k3_key[b], k1_key[b]); /* t3 = old_k3 + T(t1) */
        }
        memcpy(k2_key, old_t1_key, 64 * sizeof(bf256_t));
        memcpy(k3_key, k0_key, 64 * sizeof(bf256_t));

        memcpy(k0_key, k2_wit_key, 64 * sizeof(bf256_t));

        ublock_store_round_key_verifier(k_out_key, i + 1u, k0_key, k1_key, k2_key, k3_key);
    }
}

void ublock_SSS_enc_constraints_prover(uint8_t* o,
                                       bf256_t* o_tag_deg0,
                                       bf256_t* o_tag_deg1,
                                       bf256_t* o_tag_deg2,
                                       const uint8_t* in,
                                       const bf256_t* in_tag,
                                       const uint8_t* out,
                                       const bf256_t* out_tag,
                                       const uint8_t* w,
                                       const bf256_t* w_tag,
                                       const uint8_t* k_bar,
                                       const bf256_t* k_bar_tag)
{
    enum {
        HALF_BYTES = 16,
        STATE_BYTES = 32,
        HALF_BITS = 128,
        STATE_BITS = 256,
        ENC_W_STATES = (UBLOCK_ROUNDS / 2) - 1,
        AUG_STATES = ENC_W_STATES + 2,
        ENC_O_BLOCKS = UBLOCK_ROUNDS / 2
    };

    assert(o);
    assert(o_tag_deg0);
    assert(o_tag_deg1);
    assert(o_tag_deg2);
    assert(in);
    assert(in_tag);
    assert(out);
    assert(out_tag);
    assert(w);
    assert(w_tag);
    assert(k_bar);
    assert(k_bar_tag);

    // Step 6
    uint8_t w_aug[AUG_STATES * STATE_BYTES];
    bf256_t w_aug_tag[AUG_STATES * STATE_BITS];

    memcpy(w_aug, in, STATE_BYTES);
    memcpy(w_aug + STATE_BYTES, w, ENC_W_STATES * STATE_BYTES);
    memcpy(w_aug + (AUG_STATES - 1u) * STATE_BYTES, out, STATE_BYTES);

    memcpy(w_aug_tag, in_tag, STATE_BITS * sizeof(bf256_t));
    memcpy(w_aug_tag + STATE_BITS, w_tag, ENC_W_STATES * STATE_BITS * sizeof(bf256_t));
    memcpy(w_aug_tag + (AUG_STATES - 1u) * STATE_BITS, out_tag, STATE_BITS * sizeof(bf256_t));

    // Step 7
    for (unsigned int i = 0; i < UBLOCK_ROUNDS; i += 2) {
        const unsigned int j = i / 2u;
        uint8_t x0_bits[HALF_BYTES], x1_bits[HALF_BYTES];
        uint8_t y0_bits[HALF_BYTES], y1_bits[HALF_BYTES];
        uint8_t tmp_0_bits[HALF_BYTES], tmp_1_bits[HALF_BYTES];
        bf256_t x0_d0[HALF_BITS], x0_d1[HALF_BITS], x0_d2[HALF_BITS];
        bf256_t x1_d0[HALF_BITS], x1_d1[HALF_BITS], x1_d2[HALF_BITS];
        bf256_t y0_d0[HALF_BITS], y0_d1[HALF_BITS], y0_d2[HALF_BITS];
        bf256_t y1_d0[HALF_BITS], y1_d1[HALF_BITS], y1_d2[HALF_BITS];
        bf256_t tmp_0_tag[HALF_BITS], tmp_1_tag[HALF_BITS];
        uint8_t t_bits[HALF_BYTES];
        bf256_t t_d0[HALF_BITS], t_d1[HALF_BITS], t_d2[HALF_BITS];

        // Step 8
        memcpy(x0_bits, w_aug + j * STATE_BYTES, HALF_BYTES);
        memcpy(x1_bits, w_aug + j * STATE_BYTES + HALF_BYTES, HALF_BYTES);

        // Step 9
        // k_bar + i * 32, k[i]
        const uint8_t* k_i = k_bar + i * STATE_BYTES;
        // k_bar_tag + i * 256
        const bf256_t* k_i_tag = k_bar_tag + i * STATE_BITS;

        // Step 10-11
        /* for variable minimization, we use d2 to represent tag of x0 + k0 and x1+k1 here */
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            x0_d2[b] = bf256_add(w_aug_tag[j * STATE_BITS + b], k_i_tag[b]);
            x1_d2[b] = bf256_add(w_aug_tag[j * STATE_BITS + HALF_BITS + b], k_i_tag[HALF_BITS + b]);
        }
        xor_u8_array(x0_bits, k_i, x0_bits, 16);
        xor_u8_array(x1_bits, k_i + HALF_BYTES, x1_bits, 16);
        ublock_SSS_subword_prover(x0_bits, x0_d0, x0_d1, x0_d2, x0_bits, x0_d2, 32);
        ublock_SSS_subword_prover(x1_bits, x1_d0, x1_d1, x1_d2, x1_bits, x1_d2, 32);

        // Step 12
        // x1^3 = x1^3 + x0^3
        ublock_deg3_xor_inplace(x1_bits, x1_d0, x1_d1, x1_d2, x0_bits, x0_d0, x0_d1, x0_d2);

        // Step 13: x0 ^= rotl(x1, 4)
        memcpy(t_bits, x1_bits, sizeof(t_bits));
        memcpy(t_d0, x1_d0, sizeof(t_d0));
        memcpy(t_d1, x1_d1, sizeof(t_d1));
        memcpy(t_d2, x1_d2, sizeof(t_d2));
        ublock_deg3_rotl32_inplace(t_bits, t_d0, t_d1, t_d2, 4);
        ublock_deg3_xor_inplace(x0_bits, x0_d0, x0_d1, x0_d2, t_bits, t_d0, t_d1, t_d2);
        // Step 14: x1 ^= rotl(x0, 8)
        memcpy(t_bits, x0_bits, sizeof(t_bits));
        memcpy(t_d0, x0_d0, sizeof(t_d0));
        memcpy(t_d1, x0_d1, sizeof(t_d1));
        memcpy(t_d2, x0_d2, sizeof(t_d2));
        ublock_deg3_rotl32_inplace(t_bits, t_d0, t_d1, t_d2, 8);
        ublock_deg3_xor_inplace(x1_bits, x1_d0, x1_d1, x1_d2, t_bits, t_d0, t_d1, t_d2);
        // Step 15: x0 ^= rotl(x1, 8)
        memcpy(t_bits, x1_bits, sizeof(t_bits));
        memcpy(t_d0, x1_d0, sizeof(t_d0));
        memcpy(t_d1, x1_d1, sizeof(t_d1));
        memcpy(t_d2, x1_d2, sizeof(t_d2));
        ublock_deg3_rotl32_inplace(t_bits, t_d0, t_d1, t_d2, 8);
        ublock_deg3_xor_inplace(x0_bits, x0_d0, x0_d1, x0_d2, t_bits, t_d0, t_d1, t_d2);
        // Step 16: x1 ^= rotl(x0, 20)
        memcpy(t_bits, x0_bits, sizeof(t_bits));
        memcpy(t_d0, x0_d0, sizeof(t_d0));
        memcpy(t_d1, x0_d1, sizeof(t_d1));
        memcpy(t_d2, x0_d2, sizeof(t_d2));
        ublock_deg3_rotl32_inplace(t_bits, t_d0, t_d1, t_d2, 20);
        ublock_deg3_xor_inplace(x1_bits, x1_d0, x1_d1, x1_d2, t_bits, t_d0, t_d1, t_d2);
        // Step 17
        ublock_deg3_xor_inplace(x0_bits, x0_d0, x0_d1, x0_d2, x1_bits, x1_d0, x1_d1, x1_d2);
        // Step 18
        ublock_deg3_perm_PL_inplace(x0_bits, x0_d0, x0_d1, x0_d2);
        // Step 19
        ublock_deg3_perm_PR_inplace(x1_bits, x1_d0, x1_d1, x1_d2);

        // Step 20
        // update to k_i+1
        k_i += STATE_BYTES;
        k_i_tag += STATE_BITS;
        
        // Step 21-22
        xor_u8_array(x0_bits, k_i, x0_bits, 16);
        xor_u8_array(x1_bits, k_i + HALF_BYTES, x1_bits, 16);
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            x0_d2[b] = bf256_add(x0_d2[b], k_i_tag[b]);
            x1_d2[b] = bf256_add(x1_d2[b], k_i_tag[HALF_BITS + b]);
        }

        // Step 23
        if (i == (UBLOCK_ROUNDS - 2u)) {

            // Step 24
            k_i += STATE_BYTES;
            k_i_tag += STATE_BITS;

            // Step 25-26
            memcpy(tmp_0_bits, out, HALF_BYTES);
            memcpy(tmp_1_bits, out + HALF_BYTES, HALF_BYTES);
            memcpy(tmp_0_tag, out_tag, HALF_BITS * sizeof(bf256_t));
            memcpy(tmp_1_tag, out_tag + HALF_BITS, HALF_BITS * sizeof(bf256_t));

            xor_u8_array(tmp_0_bits, k_i, tmp_0_bits, HALF_BYTES);
            xor_u8_array(tmp_1_bits, k_i + HALF_BYTES, tmp_1_bits, HALF_BYTES);
            ublock_xor_bf256_128(tmp_0_tag, k_i_tag);
            ublock_xor_bf256_128(tmp_1_tag, k_i_tag + HALF_BITS);
        } else {
            // Step 27-28
            memcpy(tmp_0_bits, w_aug + (j + 1u) * STATE_BYTES, HALF_BYTES);
            memcpy(tmp_1_bits, w_aug + (j + 1u) * STATE_BYTES + HALF_BYTES, HALF_BYTES);
            memcpy(tmp_0_tag, w_aug_tag + (j + 1u) * STATE_BITS, HALF_BITS * sizeof(bf256_t));
            memcpy(tmp_1_tag, w_aug_tag + (j + 1u) * STATE_BITS + HALF_BITS, HALF_BITS * sizeof(bf256_t));
        }

        // Step 30-31
        ublock_deg1_perm_PR_inv_inplace(tmp_1_bits, tmp_1_tag);
        ublock_deg1_perm_PL_inv_inplace(tmp_0_bits, tmp_0_tag);

        // Step 32
        xor_u8_array(tmp_0_bits, tmp_1_bits, tmp_0_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_0_tag, tmp_1_tag);

        // Step 33
        memcpy(y0_bits, tmp_0_bits, sizeof(y0_bits));
        memcpy(y0_d2, tmp_0_tag, sizeof(y0_d2));
        ublock_deg1_rotl32_inplace(y0_bits, y0_d2, 20);
        xor_u8_array(tmp_1_bits, y0_bits, tmp_1_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_1_tag, y0_d2);

        // Step 34
        memcpy(y0_bits, tmp_1_bits, sizeof(y0_bits));
        memcpy(y0_d2, tmp_1_tag, sizeof(y0_d2));
        ublock_deg1_rotl32_inplace(y0_bits, y0_d2, 8);
        xor_u8_array(tmp_0_bits, y0_bits, tmp_0_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_0_tag, y0_d2);

        // Step 35
        memcpy(y0_bits, tmp_0_bits, sizeof(y0_bits));
        memcpy(y0_d2, tmp_0_tag, sizeof(y0_d2));
        ublock_deg1_rotl32_inplace(y0_bits, y0_d2, 8);
        xor_u8_array(tmp_1_bits, y0_bits, tmp_1_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_1_tag, y0_d2);

        // Step 36
        memcpy(y0_bits, tmp_1_bits, sizeof(y0_bits));
        memcpy(y0_d2, tmp_1_tag, sizeof(y0_d2));
        ublock_deg1_rotl32_inplace(y0_bits, y0_d2, 4);
        xor_u8_array(tmp_0_bits, y0_bits, tmp_0_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_0_tag, y0_d2);

        // Step 37
        xor_u8_array(tmp_1_bits, tmp_0_bits, tmp_1_bits, HALF_BYTES);
        ublock_xor_bf256_128(tmp_1_tag, tmp_0_tag);

        // Step 38-39
        memset(y0_bits, 0, sizeof(y0_bits));
        memset(y1_bits, 0, sizeof(y1_bits));
        ublock_SSS_inv_subword_prover(y0_bits, y0_d0, y0_d1, y0_d2, tmp_0_bits, tmp_0_tag, 32);
        ublock_SSS_inv_subword_prover(y1_bits, y1_d0, y1_d1, y1_d2, tmp_1_bits, tmp_1_tag, 32);
        
        // Step 40
        uint8_t* o_round = o + j * STATE_BYTES;
        bf256_t* o0_d0 = o_tag_deg0 + j * STATE_BITS;
        bf256_t* o0_d1 = o_tag_deg1 + j * STATE_BITS;
        bf256_t* o0_d2 = o_tag_deg2 + j * STATE_BITS;
        for (unsigned int b = 0; b < HALF_BYTES; ++b) {
            o_round[b] = (uint8_t)(x0_bits[b] ^ y0_bits[b]);
            o_round[HALF_BYTES + b] = (uint8_t)(x1_bits[b] ^ y1_bits[b]);
        }
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            o0_d0[b] = bf256_add(x0_d0[b], y0_d0[b]);
            o0_d1[b] = bf256_add(x0_d1[b], y0_d1[b]);
            o0_d2[b] = bf256_add(x0_d2[b], y0_d2[b]);
            o0_d0[HALF_BITS + b] = bf256_add(x1_d0[b], y1_d0[b]);
            o0_d1[HALF_BITS + b] = bf256_add(x1_d1[b], y1_d1[b]);
            o0_d2[HALF_BITS + b] = bf256_add(x1_d2[b], y1_d2[b]);
        }
    }
}

void ublock_SSS_enc_constraints_verifier(bf256_t* o_key,
                                         const bf256_t* in_key,
                                         const bf256_t* out_key,
                                         const bf256_t* w_key,
                                         const bf256_t* k_bar_key,
                                         bf256_t delta)
{
    enum {
        HALF_BITS = 128,
        STATE_BITS = 256,
        ENC_W_STATES = (UBLOCK_ROUNDS / 2) - 1,
        AUG_STATES = ENC_W_STATES + 2
    };

    assert(o_key);
    assert(in_key);
    assert(out_key);
    assert(w_key);
    assert(k_bar_key);

    const bf256_t delta2 = bf256_mul(delta, delta);

    // Step 6
    bf256_t w_aug_key[AUG_STATES * STATE_BITS];
    memcpy(w_aug_key, in_key, STATE_BITS * sizeof(bf256_t));
    memcpy(w_aug_key + STATE_BITS, w_key, ENC_W_STATES * STATE_BITS * sizeof(bf256_t));
    memcpy(w_aug_key + (AUG_STATES - 1u) * STATE_BITS, out_key, STATE_BITS * sizeof(bf256_t));

    // Step 7
    for (unsigned int i = 0; i < UBLOCK_ROUNDS; i += 2) {

        // Step 8
        const unsigned int j = i / 2u;
        bf256_t x0_key[HALF_BITS], x1_key[HALF_BITS];
        bf256_t y0_key[HALF_BITS], y1_key[HALF_BITS];
        bf256_t tmp_0_key[HALF_BITS], tmp_1_key[HALF_BITS];

        // Step 9
        const bf256_t* k_i_key = k_bar_key + i * STATE_BITS;

        // Step 10-11
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            x0_key[b] = bf256_add(w_aug_key[j * STATE_BITS + b], k_i_key[b]);
            x1_key[b] = bf256_add(w_aug_key[j * STATE_BITS + HALF_BITS + b], k_i_key[HALF_BITS + b]);
        }
        ublock_SSS_subword_verifier(x0_key, x0_key, delta, 32);
        ublock_SSS_subword_verifier(x1_key, x1_key, delta, 32);

        // Step 12
        ublock_xor_bf256_128(x1_key, x0_key);

        // Step 13
        memcpy(tmp_0_key, x1_key, sizeof(tmp_0_key));
        ublock_key_rotl32_inplace(tmp_0_key, 4);
        ublock_xor_bf256_128(x0_key, tmp_0_key);

        // Step 14
        memcpy(tmp_0_key, x0_key, sizeof(tmp_0_key));
        ublock_key_rotl32_inplace(tmp_0_key, 8);
        ublock_xor_bf256_128(x1_key, tmp_0_key);

        // Step 15
        memcpy(tmp_0_key, x1_key, sizeof(tmp_0_key));
        ublock_key_rotl32_inplace(tmp_0_key, 8);
        ublock_xor_bf256_128(x0_key, tmp_0_key);

        // Step 16       
        memcpy(tmp_0_key, x0_key, sizeof(tmp_0_key));
        ublock_key_rotl32_inplace(tmp_0_key, 20);
        ublock_xor_bf256_128(x1_key, tmp_0_key);

        // Step 17
        ublock_xor_bf256_128(x0_key, x1_key);

        // Step 18-19
        ublock_key_perm_PL_inplace(x0_key);
        ublock_key_perm_PR_inplace(x1_key);

        // Step 20
        k_i_key += STATE_BITS;

        // Step 21-22
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            x0_key[b] = bf256_add(x0_key[b], bf256_mul(delta2, k_i_key[b]));
            x1_key[b] = bf256_add(x1_key[b], bf256_mul(delta2, k_i_key[HALF_BITS + b]));
        }

        // Step 23
        if (i == (UBLOCK_ROUNDS - 2u)) {

            // Step 24
            k_i_key += STATE_BITS;
            // Step 25-26
            for (unsigned int b = 0; b < HALF_BITS; ++b) {
                tmp_0_key[b] = bf256_add(out_key[b], k_i_key[b]);
                tmp_1_key[b] = bf256_add(out_key[HALF_BITS + b], k_i_key[HALF_BITS + b]);
            }
        } else {
            // Step 27-29
            memcpy(tmp_0_key, w_aug_key + (j + 1u) * STATE_BITS, sizeof(tmp_0_key));
            memcpy(tmp_1_key, w_aug_key + (j + 1u) * STATE_BITS + HALF_BITS, sizeof(tmp_1_key));
        }

        // Step 30-31
        ublock_key_perm_PR_inv_inplace(tmp_1_key);
        ublock_key_perm_PL_inv_inplace(tmp_0_key);

        // Step 32
        ublock_xor_bf256_128(tmp_0_key, tmp_1_key);

        // Step 33
        memcpy(y0_key, tmp_0_key, sizeof(y0_key));
        ublock_key_rotl32_inplace(y0_key, 20);
        ublock_xor_bf256_128(tmp_1_key, y0_key);

        // Step 34
        memcpy(y0_key, tmp_1_key, sizeof(y0_key));
        ublock_key_rotl32_inplace(y0_key, 8);
        ublock_xor_bf256_128(tmp_0_key, y0_key);

        // Step 35
        memcpy(y0_key, tmp_0_key, sizeof(y0_key));
        ublock_key_rotl32_inplace(y0_key, 8);
        ublock_xor_bf256_128(tmp_1_key, y0_key);

        // Step 36
        memcpy(y0_key, tmp_1_key, sizeof(y0_key));
        ublock_key_rotl32_inplace(y0_key, 4);
        ublock_xor_bf256_128(tmp_0_key, y0_key);
        // Step 37
        ublock_xor_bf256_128(tmp_1_key, tmp_0_key);

        // Step 38-39
        ublock_SSS_inv_subword_verifier(y0_key, tmp_0_key, delta, 32);
        ublock_SSS_inv_subword_verifier(y1_key, tmp_1_key, delta, 32);

        
        bf256_t* o_round_key = o_key + j * STATE_BITS;
        for (unsigned int b = 0; b < HALF_BITS; ++b) {
            o_round_key[b] = bf256_add(x0_key[b], y0_key[b]);
            o_round_key[HALF_BITS + b] = bf256_add(x1_key[b], y1_key[b]);
        }
        
    }
}
