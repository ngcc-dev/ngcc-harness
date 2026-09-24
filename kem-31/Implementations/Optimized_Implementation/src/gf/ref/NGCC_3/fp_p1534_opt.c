/*
 * Field arithmetic for NGCC_3 p1534.
 *
 * Optimized saturated Montgomery branch.  The public fp_t size is kept at
 * NWORDS_FIELD=26 for compatibility with the current NGCC_3 headers; the first
 * 24 limbs hold the saturated value and the last two limbs are always cleared.
 *
 * The modulus has eight low limbs equal to -1, i.e. p = B^8 * Q - 1 for
 * B = 2^64.  Montgomery reduction uses this shape to skip the low eight
 * modulus limbs.
 */

#include <fp.h>
#include <string.h>

#include "modarith_p1534.c"

#define FIELD_LIMBS 24

extern void fp1534_24_add_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                              const uint64_t b[NWORDS_FIELD]);
extern void fp1534_24_sub_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                              const uint64_t b[NWORDS_FIELD]);
extern void fp1534_24_half_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp1534_24_montmul_pair_adx_asm(uint64_t *c, const uint64_t *a, const uint64_t *b);
extern void fp1534_24_montsqr_adx_asm(uint64_t *c, const uint64_t *a);

static const uint64_t MODULUS[FIELD_LIMBS] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x74f745f71f7b456bULL, 0x4e9c3615eab1df2aULL,
    0xe3cd124452453081ULL, 0xdae93fda6645a281ULL,
    0x3a5035c534b5f3afULL, 0x8cd6ed92b85bd0cbULL,
    0xb3e2fa38f5419bafULL, 0xef81c420b2dfbb3fULL,
    0x76f58123de8fab63ULL, 0xf4c3a493432f0854ULL,
    0x07a2821a8511c268ULL, 0xbdfbff3ede30e076ULL,
    0x14541bc051219e91ULL, 0xa886397f71da19e5ULL,
    0x5554f51ce0e783f6ULL, 0x22606deb050ed144ULL
};

const uint64_t p[NWORDS_FIELD] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x74f745f71f7b456bULL, 0x4e9c3615eab1df2aULL,
    0xe3cd124452453081ULL, 0xdae93fda6645a281ULL,
    0x3a5035c534b5f3afULL, 0x8cd6ed92b85bd0cbULL,
    0xb3e2fa38f5419bafULL, 0xef81c420b2dfbb3fULL,
    0x76f58123de8fab63ULL, 0xf4c3a493432f0854ULL,
    0x07a2821a8511c268ULL, 0xbdfbff3ede30e076ULL,
    0x14541bc051219e91ULL, 0xa886397f71da19e5ULL,
    0x5554f51ce0e783f6ULL, 0x22606deb050ed144ULL,
    0, 0
};

const uint64_t ONE[NWORDS_FIELD] = {
    0x0000000000000007ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0xcd3d163e23a11a0cULL, 0xd9ba85669522e5d6ULL,
    0xc5648021c01bac76ULL, 0x039f410734188e72ULL,
    0x67ce879b8f065631ULL, 0x261f80fcf57d4a71ULL,
    0x14cb28714b34be33ULL, 0x7373a31b1be1e142ULL,
    0xbf497804ea125044ULL, 0x4ea67ff929b6c5b0ULL,
    0xca8e71465c83af21ULL, 0xce1c0547eca9dcc5ULL,
    0x71b33dbdc814aa03ULL, 0x64546d83e3094abcULL,
    0xaaad4c35d9ab6441ULL, 0x0f5cfe92dc984721ULL,
    0, 0
};

const uint64_t ZERO[NWORDS_FIELD] = {0};

static const uint64_t MONTGOMERY_R2[FIELD_LIMBS] = {
    0xa1ca2cc47a0104f9ULL, 0x933b4e31e7b568acULL,
    0xa13e586c3e763763ULL, 0x583021f6332975ceULL,
    0x7ccade63497b282aULL, 0x5bfa5c8e6d872065ULL,
    0xe818d2d91b7ad942ULL, 0x76667ac0a1e86be4ULL,
    0xcf79f8962208deb5ULL, 0xf46e05ecbc004b65ULL,
    0x59d8737bb8693374ULL, 0x7adfcac00a9b02bcULL,
    0xf3286a9a2300c8c4ULL, 0x311c9817975e8c5eULL,
    0x010fa8a95ef4cae5ULL, 0x3a8df126a4f3b8f9ULL,
    0xb7644ce045bce36aULL, 0x91dd7a356e8ecb94ULL,
    0xd02eb07d635effd9ULL, 0xa4cba9e1dd14c463ULL,
    0xc6b4675f6bc30124ULL, 0xef525e4c97796b1cULL,
    0xbca01fd3c4a295b1ULL, 0x066e822000f9ccacULL
};

static inline void clear_unused_limbs(uint64_t *a) {
    a[24] = 0;
    a[25] = 0;
}

static void copy_field(uint64_t *dst, const uint64_t *src) {
    memcpy(dst, src, sizeof(uint64_t) * FIELD_LIMBS);
    clear_unused_limbs(dst);
}

static inline uint64_t subborrow64(uint64_t x, uint64_t y, uint64_t borrow, uint64_t *out) {
    uint64_t diff = x - y;
    uint64_t next_borrow = x < y;
    uint64_t result = diff - borrow;

    next_borrow |= diff < borrow;
    *out = result;
    return next_borrow;
}

static uint64_t subtract_modulus_if_possible(uint64_t *a) {
    uint64_t reduced[FIELD_LIMBS];
    uint64_t borrow = 0;

    for (int i = 0; i < FIELD_LIMBS; i++) {
        borrow = subborrow64(a[i], MODULUS[i], borrow, &reduced[i]);
    }

    uint64_t use_reduced = (uint64_t)0 - (borrow ^ 1u);
    for (int i = 0; i < FIELD_LIMBS; i++) {
        a[i] = (a[i] & ~use_reduced) | (reduced[i] & use_reduced);
    }

    return borrow ^ 1u;
}

static void reduce_u1536(uint64_t *a) {
    for (int i = 0; i < 7; i++) {
        (void)subtract_modulus_if_possible(a);
    }
    clear_unused_limbs(a);
}

static void montgomery_multiply(uint64_t *c, const uint64_t *a, const uint64_t *b) {
    fp1534_24_montmul_pair_adx_asm(c, a, b);
}

static void to_montgomery(const uint64_t *m, uint64_t *n) {
    montgomery_multiply(n, m, MONTGOMERY_R2);
}

static void from_montgomery(const uint64_t *n, uint64_t *m) {
    static const uint64_t normal_one[FIELD_LIMBS] = {1};
    montgomery_multiply(m, n, normal_one);
}

static void normal_to_radix60(const uint64_t normal[FIELD_LIMBS], uint64_t radix60[NWORDS_FIELD]) {
    const uint64_t mask = (1ULL << 60) - 1;

    for (int i = 0; i < NWORDS_FIELD; i++) {
        unsigned int bit = (unsigned int)(60 * i);
        unsigned int word = bit >> 6;
        unsigned int shift = bit & 63;
        uint64_t limb = 0;

        if (word < FIELD_LIMBS) {
            limb = normal[word] >> shift;
        }
        if (shift != 0 && word + 1 < FIELD_LIMBS) {
            limb |= normal[word + 1] << (64 - shift);
        }

        radix60[i] = limb & mask;
    }
}

static void radix60_to_normal(const uint64_t radix60[NWORDS_FIELD], uint64_t normal[NWORDS_FIELD]) {
    memset(normal, 0, sizeof(uint64_t) * NWORDS_FIELD);

    for (int i = 0; i < NWORDS_FIELD; i++) {
        unsigned int bit = (unsigned int)(60 * i);
        unsigned int word = bit >> 6;
        unsigned int shift = bit & 63;

        if (word < FIELD_LIMBS) {
            normal[word] |= radix60[i] << shift;
        }
        if (shift != 0 && word + 1 < FIELD_LIMBS) {
            normal[word + 1] |= radix60[i] >> (64 - shift);
        }
    }

    clear_unused_limbs(normal);
}

static void montgomery_to_radix60_montgomery(const uint64_t montgomery[NWORDS_FIELD],
                                             uint64_t radix60[NWORDS_FIELD]) {
    uint64_t normal[NWORDS_FIELD];

    from_montgomery(montgomery, normal);
    normal_to_radix60(normal, radix60);
    nres(radix60, radix60);
}

static void radix60_montgomery_to_montgomery(uint64_t radix60[NWORDS_FIELD],
                                             uint64_t montgomery[NWORDS_FIELD]) {
    uint64_t normal_radix60[NWORDS_FIELD];
    uint64_t normal[NWORDS_FIELD];

    redc(radix60, normal_radix60);
    (void)modfsb(normal_radix60);
    radix60_to_normal(normal_radix60, normal);
    to_montgomery(normal, montgomery);
}

static int is_zero_montgomery(const uint64_t *a) {
    uint64_t nonzero = 0;

    for (int i = 0; i < FIELD_LIMBS; i++) {
        nonzero |= a[i];
    }

    return nonzero == 0;
}

static int sign_montgomery(const uint64_t *a) {
    uint64_t normal[NWORDS_FIELD];

    from_montgomery(a, normal);
    return (int)(normal[0] & 1u);
}

void fp_add(fp_t *out, const fp_t *a, const fp_t *b) {
    fp1534_24_add_asm(*out, *a, *b);
}

void fp_sub(fp_t *out, const fp_t *a, const fp_t *b) {
    fp1534_24_sub_asm(*out, *a, *b);
}

void fp_sqr(fp_t *out, const fp_t *a) {
    fp1534_24_montsqr_adx_asm(*out, *a);
}

void fp_mul(fp_t *out, const fp_t *a, const fp_t *b) {
    montgomery_multiply(*out, *a, *b);
}

void fp_tomont(fp_t *out, const fp_t *a) {
    fp_t normal;

    copy_field(normal, *a);
    reduce_u1536(normal);
    to_montgomery(normal, *out);
}

void fp_frommont(fp_t *out, const fp_t *a) {
    from_montgomery(*a, *out);
}

void fp_mont_setone(fp_t *out) {
    memcpy(*out, ONE, sizeof(ONE));
}

void fp_neg(fp_t *out, const fp_t *a) {
    fp_sub(out, (const fp_t *)&ZERO, a);
}

void fp_copy(fp_t *out, const fp_t *a) {
    copy_field(*out, *a);
}

void fp_set_zero(fp_t *a) {
    memset(*a, 0, sizeof(fp_t));
}

void fp_set_one(fp_t *a) {
    memcpy(*a, ONE, sizeof(ONE));
}

void fp_set_small(fp_t *x, const digit_t val) {
    fp_t normal = {0};

    if (val == 0) {
        fp_set_zero(x);
        return;
    }
    if (val == 1) {
        fp_set_one(x);
        return;
    }
    if (val < (digit_t)(1u << 16)) {
        fp_t acc;
        fp_set_zero(x);
        fp_set_one(&acc);

        digit_t v = val;
        while (v != 0) {
            if ((v & 1u) != 0) {
                fp_add(x, x, &acc);
            }
            v >>= 1;
            if (v != 0) {
                fp_add(&acc, &acc, &acc);
            }
        }
        return;
    }

    normal[0] = val;
    to_montgomery(normal, *x);
}

void fp_half(fp_t *out, const fp_t *a) {
    fp1534_24_half_asm(*out, *a);
}

void fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl) {
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        (*d)[i] = (*a0)[i] ^ (mask & ((*a0)[i] ^ (*a1)[i]));
    }
}

void fp_cswap(fp_t *a, fp_t *b, uint32_t ctl) {
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        uint64_t swap = mask & ((*a)[i] ^ (*b)[i]);
        (*a)[i] ^= swap;
        (*b)[i] ^= swap;
    }
}

uint32_t fp_is_equal(const fp_t *a, const fp_t *b) {
    uint64_t diff = 0;

    for (int i = 0; i < FIELD_LIMBS; i++) {
        diff |= (*a)[i] ^ (*b)[i];
    }

    return -(uint32_t)(diff == 0);
}

uint32_t fp_is_zero(const fp_t *a) {
    return -(uint32_t)is_zero_montgomery(*a);
}

void fp_encode(void *dst, const fp_t *a) {
    uint64_t normal[NWORDS_FIELD];

    from_montgomery(*a, normal);
    memcpy(dst, normal, FP_NBYTES);
}

void fp_decode_reduce(fp_t *d, const void *src, size_t len) {
    size_t n = len < FP_NBYTES ? len : FP_NBYTES;

    memset(*d, 0, sizeof(fp_t));
    memcpy(*d, src, n);
    reduce_u1536(*d);
    to_montgomery(*d, *d);
}

void fp_decode(fp_t *d, const void *src) {
    memset(*d, 0, sizeof(fp_t));
    memcpy(*d, src, FP_NBYTES);
    reduce_u1536(*d);
    to_montgomery(*d, *d);
}

void fp_inv(fp_t *a) {
    uint64_t radix60[NWORDS_FIELD];

    if (is_zero_montgomery(*a)) {
        fp_set_zero(a);
        return;
    }

    montgomery_to_radix60_montgomery(*a, radix60);
    modinv(radix60, NULL, radix60);
    radix60_montgomery_to_montgomery(radix60, *a);
}

uint32_t fp_is_square(const fp_t *a) {
    uint64_t radix60[NWORDS_FIELD];

    montgomery_to_radix60_montgomery(*a, radix60);
    return -(uint32_t)(modqr(NULL, radix60) == 1);
}

void fp_sqrt(fp_t *a) {
    uint64_t radix60[NWORDS_FIELD];
    fp_t root, negative_root;
    uint64_t mask;

    montgomery_to_radix60_montgomery(*a, radix60);
    modsqrt(radix60, NULL, radix60);
    radix60_montgomery_to_montgomery(radix60, root);
    fp_neg(&negative_root, &root);

    mask = (uint64_t)0 - (uint64_t)sign_montgomery(root);
    for (int i = 0; i < NWORDS_FIELD; i++) {
        (*a)[i] = (root[i] & ~mask) | (negative_root[i] & mask);
    }
}
