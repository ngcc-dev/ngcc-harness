/*
 * Field arithmetic for NGCC_1 p479.
 *
 * Internal representation: saturated 8x64-bit Montgomery form
 * (R = 2^512).  The prime has p[0] = -1, so the assembly hot path uses
 * CIOS Montgomery multiplication with mu = 1.
 */

#include <fp.h>
#include <pike_profile.h>
#include <string.h>

#include "modarith_p479.c"

extern void fp479_add_asm(uint64_t c[8], const uint64_t a[8], const uint64_t b[8]);
extern void fp479_sub_asm(uint64_t c[8], const uint64_t a[8], const uint64_t b[8]);
extern void fp479_mul_asm(uint64_t c[8], const uint64_t a[8], const uint64_t b[8]);
extern void fp479_sqr_asm(uint64_t c[8], const uint64_t a[8]);
extern void fp479_redc_asm(uint64_t c[8], const uint64_t a[8]);
extern void fp479_half_asm(uint64_t c[8], const uint64_t a[8]);

static const uint64_t MODULUS[8] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x94cbe0b3ffffffffULL, 0xe0daea16974b919eULL,
    0x588643dadd9997fbULL, 0x86b19c0bf3bb9a90ULL,
    0xc23ef552f7bea022ULL, 0x00000000588ee91aULL
};

const uint64_t p[NWORDS_FIELD] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x94cbe0b3ffffffffULL, 0xe0daea16974b919eULL,
    0x588643dadd9997fbULL, 0x86b19c0bf3bb9a90ULL,
    0xc23ef552f7bea022ULL, 0x00000000588ee91aULL
};

const uint64_t ZERO[NWORDS_FIELD] = {0, 0, 0, 0, 0, 0, 0, 0};

const uint64_t ONE[NWORDS_FIELD] = {
    0x00000002e4086177ULL, 0x0000000000000000ULL,
    0x3f76585400000000ULL, 0xe97f74e116a03d00ULL,
    0x32799f2af52c1636ULL, 0xaa66067189d39505ULL,
    0x5b0cc1b6cd48de4aULL, 0x000000000cea71daULL
};

static const uint64_t MONTGOMERY_R2[8] = {
    0xf3af5062d6276bf0ULL, 0xd91c272fe92c75bcULL,
    0x047dfb3510978d4dULL, 0xbdc6b0726ec3fc8fULL,
    0x57356dd4406e0d73ULL, 0x284780b4720e0bb0ULL,
    0x01c8d6d8dd9f108bULL, 0x000000000103d17aULL
};

static const uint64_t SMALL_MONTGOMERY[32][8] = {
    {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL},
    {0x00000002e4086177ULL, 0x0000000000000000ULL, 0x3f76585400000000ULL, 0xe97f74e116a03d00ULL, 0x32799f2af52c1636ULL, 0xaa66067189d39505ULL, 0x5b0cc1b6cd48de4aULL, 0x000000000cea71daULL},
    {0x00000005c810c2eeULL, 0x0000000000000000ULL, 0x7eecb0a800000000ULL, 0xd2fee9c22d407a00ULL, 0x64f33e55ea582c6dULL, 0x54cc0ce313a72a0aULL, 0xb619836d9a91bc95ULL, 0x0000000019d4e3b4ULL},
    {0x00000008ac192465ULL, 0x0000000000000000ULL, 0xbe6308fc00000000ULL, 0xbc7e5ea343e0b700ULL, 0x976cdd80df8442a4ULL, 0xff3213549d7abf0fULL, 0x1126452467da9adfULL, 0x0000000026bf558fULL},
    {0x0000000b902185dcULL, 0x0000000000000000ULL, 0xfdd9615000000000ULL, 0xa5fdd3845a80f400ULL, 0xc9e67cabd4b058dbULL, 0xa99819c6274e5414ULL, 0x6c3306db3523792aULL, 0x0000000033a9c769ULL},
    {0x0000000e7429e753ULL, 0x0000000000000000ULL, 0x3d4fb9a400000000ULL, 0x8f7d486571213101ULL, 0xfc601bd6c9dc6f12ULL, 0x53fe2037b121e919ULL, 0xc73fc892026c5775ULL, 0x0000000040943943ULL},
    {0x00000011583248caULL, 0x0000000000000000ULL, 0x7cc611f800000000ULL, 0x78fcbd4687c16e01ULL, 0x2ed9bb01bf088549ULL, 0xfe6426a93af57e1fULL, 0x224c8a48cfb535bfULL, 0x000000004d7eab1eULL},
    {0x000000143c3aaa42ULL, 0x0000000000000000ULL, 0x2770899800000000ULL, 0x81a1481107161963ULL, 0x08cd1651d69b0384ULL, 0x2218910ed10d7894ULL, 0xbb1a56aca53f73e8ULL, 0x0000000001da33ddULL},
    {0x0000001720430bb9ULL, 0x0000000000000000ULL, 0x66e6e1ec00000000ULL, 0x6b20bcf21db65663ULL, 0x3b46b57ccbc719bbULL, 0xcc7e97805ae10d99ULL, 0x1627186372885232ULL, 0x000000000ec4a5b8ULL},
    {0x0000001a044b6d30ULL, 0x0000000000000000ULL, 0xa65d3a4000000000ULL, 0x54a031d334569363ULL, 0x6dc054a7c0f32ff2ULL, 0x76e49df1e4b4a29eULL, 0x7133da1a3fd1307dULL, 0x000000001baf1792ULL},
    {0x0000001ce853cea7ULL, 0x0000000000000000ULL, 0xe5d3929400000000ULL, 0x3e1fa6b44af6d063ULL, 0xa039f3d2b61f4629ULL, 0x214aa4636e8837a3ULL, 0xcc409bd10d1a0ec8ULL, 0x000000002899896cULL},
    {0x0000001fcc5c301eULL, 0x0000000000000000ULL, 0x2549eae800000000ULL, 0x279f1b9561970d64ULL, 0xd2b392fdab4b5c60ULL, 0xcbb0aad4f85bcca8ULL, 0x274d5d87da62ed12ULL, 0x000000003583fb47ULL},
    {0x00000022b0649195ULL, 0x0000000000000000ULL, 0x64c0433c00000000ULL, 0x111e907678374a64ULL, 0x052d3228a0777297ULL, 0x7616b146822f61aeULL, 0x825a1f3ea7abcb5dULL, 0x00000000426e6d21ULL},
    {0x00000025946cf30cULL, 0x0000000000000000ULL, 0xa4369b9000000000ULL, 0xfa9e05578ed78764ULL, 0x37a6d15395a388cdULL, 0x207cb7b80c02f6b3ULL, 0xdd66e0f574f4a9a8ULL, 0x000000004f58defbULL},
    {0x0000002878755484ULL, 0x0000000000000000ULL, 0x4ee1133000000000ULL, 0x034290220e2c32c6ULL, 0x119a2ca3ad360709ULL, 0x4431221da21af128ULL, 0x7634ad594a7ee7d0ULL, 0x0000000003b467bbULL},
    {0x0000002b5c7db5fbULL, 0x0000000000000000ULL, 0x8e576b8400000000ULL, 0xecc2050324cc6fc6ULL, 0x4413cbcea2621d3fULL, 0xee97288f2bee862dULL, 0xd1416f1017c7c61aULL, 0x00000000109ed995ULL},
    {0x0000002e40861772ULL, 0x0000000000000000ULL, 0xcdcdc3d800000000ULL, 0xd64179e43b6cacc6ULL, 0x768d6af9978e3376ULL, 0x98fd2f00b5c21b32ULL, 0x2c4e30c6e510a465ULL, 0x000000001d894b70ULL},
    {0x00000031248e78e9ULL, 0x0000000000000000ULL, 0x0d441c2c00000000ULL, 0xbfc0eec5520ce9c7ULL, 0xa9070a248cba49adULL, 0x436335723f95b037ULL, 0x875af27db25982b0ULL, 0x000000002a73bd4aULL},
    {0x000000340896da60ULL, 0x0000000000000000ULL, 0x4cba748000000000ULL, 0xa94063a668ad26c7ULL, 0xdb80a94f81e65fe4ULL, 0xedc93be3c969453cULL, 0xe267b4347fa260faULL, 0x00000000375e2f24ULL},
    {0x00000036ec9f3bd7ULL, 0x0000000000000000ULL, 0x8c30ccd400000000ULL, 0x92bfd8877f4d63c7ULL, 0x0dfa487a7712761bULL, 0x982f4255533cda42ULL, 0x3d7475eb4ceb3f45ULL, 0x000000004448a0ffULL},
    {0x00000039d0a79d4eULL, 0x0000000000000000ULL, 0xcba7252800000000ULL, 0x7c3f4d6895eda0c7ULL, 0x4073e7a56c3e8c52ULL, 0x429548c6dd106f47ULL, 0x988137a21a341d90ULL, 0x00000000513312d9ULL},
    {0x0000003cb4affec6ULL, 0x0000000000000000ULL, 0x76519cc800000000ULL, 0x84e3d83315424c29ULL, 0x1a6742f583d10a8dULL, 0x6649b32c732869bcULL, 0x314f0405efbe5bb8ULL, 0x00000000058e9b99ULL},
    {0x0000003f98b8603dULL, 0x0000000000000000ULL, 0xb5c7f51c00000000ULL, 0x6e634d142be28929ULL, 0x4ce0e22078fd20c4ULL, 0x10afb99dfcfbfec1ULL, 0x8c5bc5bcbd073a03ULL, 0x0000000012790d73ULL},
    {0x000000427cc0c1b4ULL, 0x0000000000000000ULL, 0xf53e4d7000000000ULL, 0x57e2c1f54282c629ULL, 0x7f5a814b6e2936fbULL, 0xbb15c00f86cf93c6ULL, 0xe76887738a50184dULL, 0x000000001f637f4dULL},
    {0x0000004560c9232bULL, 0x0000000000000000ULL, 0x34b4a5c400000000ULL, 0x416236d65923032aULL, 0xb1d4207663554d32ULL, 0x657bc68110a328cbULL, 0x4275492a5798f698ULL, 0x000000002c4df128ULL},
    {0x0000004844d184a2ULL, 0x0000000000000000ULL, 0x742afe1800000000ULL, 0x2ae1abb76fc3402aULL, 0xe44dbfa158816369ULL, 0x0fe1ccf29a76bdd0ULL, 0x9d820ae124e1d4e3ULL, 0x0000000039386302ULL},
    {0x0000004b28d9e619ULL, 0x0000000000000000ULL, 0xb3a1566c00000000ULL, 0x1461209886637d2aULL, 0x16c75ecc4dad79a0ULL, 0xba47d364244a52d6ULL, 0xf88ecc97f22ab32dULL, 0x000000004622d4dcULL},
    {0x0000004e0ce24790ULL, 0x0000000000000000ULL, 0xf317aec000000000ULL, 0xfde095799d03ba2aULL, 0x4940fdf742d98fd6ULL, 0x64add9d5ae1de7dbULL, 0x539b8e4ebf739178ULL, 0x00000000530d46b7ULL},
    {0x00000050f0eaa908ULL, 0x0000000000000000ULL, 0x9dc2266000000000ULL, 0x068520441c58658cULL, 0x233459475a6c0e12ULL, 0x8862443b4435e250ULL, 0xec695ab294fdcfa0ULL, 0x000000000768cf76ULL},
    {0x00000053d4f30a7fULL, 0x0000000000000000ULL, 0xdd387eb400000000ULL, 0xf004952532f8a28cULL, 0x55adf8724f982448ULL, 0x32c84aacce097755ULL, 0x47761c696246adebULL, 0x0000000014534151ULL},
    {0x00000056b8fb6bf6ULL, 0x0000000000000000ULL, 0x1caed70800000000ULL, 0xd9840a064998df8dULL, 0x8827979d44c43a7fULL, 0xdd2e511e57dd0c5aULL, 0xa282de202f8f8c35ULL, 0x00000000213db32bULL},
    {0x000000599d03cd6dULL, 0x0000000000000000ULL, 0x5c252f5c00000000ULL, 0xc3037ee760391c8dULL, 0xbaa136c839f050b6ULL, 0x8794578fe1b0a15fULL, 0xfd8f9fd6fcd86a80ULL, 0x000000002e282505ULL},
};

static void normal_to_radix61(const uint64_t normal[8], uint64_t radix61[8]) {
    const uint64_t mask = (1ULL << 61) - 1;

    for (int i = 0; i < 8; i++) {
        unsigned int bit = (unsigned int)(61 * i);
        unsigned int word = bit >> 6;
        unsigned int shift = bit & 63;
        uint64_t limb = normal[word] >> shift;

        if (shift != 0 && word + 1 < 8) {
            limb |= normal[word + 1] << (64 - shift);
        }

        radix61[i] = limb & mask;
    }
}

static void radix61_to_normal(const uint64_t radix61[8], uint64_t normal[8]) {
    memset(normal, 0, sizeof(uint64_t) * 8);

    for (int i = 0; i < 8; i++) {
        unsigned int bit = (unsigned int)(61 * i);
        unsigned int word = bit >> 6;
        unsigned int shift = bit & 63;

        normal[word] |= radix61[i] << shift;
        if (shift != 0 && word + 1 < 8) {
            normal[word + 1] |= radix61[i] >> (64 - shift);
        }
    }
}

static inline void montgomery_multiply(uint64_t *c, const uint64_t *a, const uint64_t *b) {
    fp479_mul_asm(c, a, b);
}

static uint64_t subtract_modulus_if_possible(uint64_t *a) {
    uint64_t reduced[8];
    uint64_t borrow = 0;

    for (int i = 0; i < 8; i++) {
        __uint128_t diff = (__uint128_t)a[i] - MODULUS[i] - borrow;
        reduced[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 127);
    }

    uint64_t use_reduced = (uint64_t)0 - (borrow ^ 1u);
    for (int i = 0; i < 8; i++) {
        a[i] = (a[i] & ~use_reduced) | (reduced[i] & use_reduced);
    }

    return borrow ^ 1u;
}

static void reduce_u480(uint64_t *a) {
    for (int i = 0; i < 3; i++) {
        (void)subtract_modulus_if_possible(a);
    }
}

static void to_montgomery(const uint64_t *m, uint64_t *n) {
    montgomery_multiply(n, m, MONTGOMERY_R2);
}

static void from_montgomery(const uint64_t *n, uint64_t *m) {
    fp479_redc_asm(m, n);
}

static void montgomery_to_radix61_montgomery(const uint64_t montgomery[8], uint64_t radix61[8]) {
    uint64_t normal[8];

    from_montgomery(montgomery, normal);
    normal_to_radix61(normal, radix61);
    nres(radix61, radix61);
}

static void radix61_montgomery_to_montgomery(const uint64_t radix61[8], uint64_t montgomery[8]) {
    uint64_t normal_radix61[8], normal[8];

    redc(radix61, normal_radix61);
    radix61_to_normal(normal_radix61, normal);
    to_montgomery(normal, montgomery);
}

static void negate_montgomery(uint64_t *c, const uint64_t *a) {
    fp479_sub_asm(c, ZERO, a);
}

static void invert_montgomery(const uint64_t *x, uint64_t *z) {
    uint64_t radix61[8];

    montgomery_to_radix61_montgomery(x, radix61);
    modinv(radix61, NULL, radix61);
    radix61_montgomery_to_montgomery(radix61, z);
}

static void sqrt_montgomery(const uint64_t *x, uint64_t *r) {
    uint64_t radix61[8];

    montgomery_to_radix61_montgomery(x, radix61);
    modsqrt(radix61, NULL, radix61);
    radix61_montgomery_to_montgomery(radix61, r);
}

static int is_zero_montgomery(const uint64_t *a) {
    uint64_t nonzero = 0;

    for (int i = 0; i < 8; i++) {
        nonzero |= a[i];
    }

    return nonzero == 0;
}

static int is_square_montgomery(const uint64_t *x) {
    uint64_t radix61[8];

    montgomery_to_radix61_montgomery(x, radix61);
    return modqr(NULL, radix61);
}

static int sign_montgomery(const uint64_t *a) {
    uint64_t normal[8];

    from_montgomery(a, normal);
    return (int)(normal[0] & 1u);
}

void fp_add(fp_t *out, const fp_t *a, const fp_t *b) {
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP_ADD);
    fp479_add_asm(*out, *a, *b);
}

void fp_sub(fp_t *out, const fp_t *a, const fp_t *b) {
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP_SUB);
    fp479_sub_asm(*out, *a, *b);
}

void fp_sqr(fp_t *out, const fp_t *a) {
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP_SQR);
    fp479_sqr_asm(*out, *a);
}

void fp_mul(fp_t *out, const fp_t *a, const fp_t *b) {
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP_MUL);
    fp479_mul_asm(*out, *a, *b);
}

void fp_tomont(fp_t *out, const fp_t *a) {
    to_montgomery(*a, *out);
}

void fp_frommont(fp_t *out, const fp_t *a) {
    from_montgomery(*a, *out);
}

void fp_mont_setone(fp_t *out) {
    memcpy(*out, ONE, sizeof(ONE));
}

void fp_neg(fp_t *out, const fp_t *a) {
    negate_montgomery(*out, *a);
}

void fp_copy(fp_t *out, const fp_t *a) {
    memcpy(*out, *a, sizeof(fp_t));
}

void fp_set_zero(fp_t *a) {
    memset(*a, 0, sizeof(fp_t));
}

void fp_set_one(fp_t *a) {
    memcpy(*a, ONE, sizeof(ONE));
}

void fp_set_small(fp_t *x, const digit_t val) {
    if (val < 32) {
        memcpy(*x, SMALL_MONTGOMERY[val], sizeof(fp_t));
        return;
    }

    fp_set_zero(x);
    (*x)[0] = val;
    fp_tomont(x, x);
}

void fp_half(fp_t *out, const fp_t *a) {
    PIKE_PROFILE_COUNT(PIKE_PROFILE_FP_HALF);
    fp479_half_asm(*out, *a);
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

    for (int i = 0; i < 8; i++) {
        diff |= (*a)[i] ^ (*b)[i];
    }

    return -(uint32_t)(diff == 0);
}

uint32_t fp_is_zero(const fp_t *a) {
    return -(uint32_t)is_zero_montgomery(*a);
}

void fp_encode(void *dst, const fp_t *a) {
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP_ENCODE);
    uint64_t normal[8];

    from_montgomery(*a, normal);
    memcpy(dst, normal, FP_NBYTES);
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP_ENCODE);
}

void fp_decode_reduce(fp_t *d, const void *src, size_t len) {
    size_t n = len < FP_NBYTES ? len : FP_NBYTES;

    memset(*d, 0, sizeof(fp_t));
    memcpy(*d, src, n);
    reduce_u480(*d);
    to_montgomery(*d, *d);
}

void fp_decode(fp_t *d, const void *src) {
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP_DECODE);
    memset(*d, 0, sizeof(fp_t));
    memcpy(*d, src, FP_NBYTES);
    reduce_u480(*d);
    to_montgomery(*d, *d);
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP_DECODE);
}

void fp_inv(fp_t *a) {
    PIKE_PROFILE_START(prof_start, PIKE_PROFILE_FP_INV);
    uint64_t z[8];

    if (is_zero_montgomery(*a)) {
        fp_set_zero(a);
        PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP_INV);
        return;
    }

    invert_montgomery(*a, z);
    memcpy(*a, z, sizeof(fp_t));
    PIKE_PROFILE_STOP(prof_start, PIKE_PROFILE_FP_INV);
}

uint32_t fp_is_square(const fp_t *a) {
    return -(uint32_t)is_square_montgomery(*a);
}

void fp_sqrt(fp_t *a) {
    uint64_t root[8], negative_root[8];
    uint64_t mask;

    sqrt_montgomery(*a, root);
    negate_montgomery(negative_root, root);
    mask = (uint64_t)0 - (uint64_t)sign_montgomery(root);

    for (int i = 0; i < 8; i++) {
        root[i] = (root[i] & ~mask) | (negative_root[i] & mask);
    }

    memcpy(*a, root, sizeof(fp_t));
}
