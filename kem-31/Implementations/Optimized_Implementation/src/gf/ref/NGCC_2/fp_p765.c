/*
 * Field arithmetic for NGCC_2 p765.
 *
 * Internal representation: saturated 12x64-bit Montgomery form
 * (R = 2^768).  The public fp_t size remains NWORDS_FIELD=13 for
 * compatibility with the generated NGCC_2 headers; limb 12 is always cleared.
 *
 * The modulus has four low limbs equal to -1, so p = B^4 * Q - 1 for
 * B = 2^64.  The assembly Montgomery path uses this shape for reduction.
 */

#include <fp.h>
#include <stdint.h>
#include <string.h>

#define FIELD_LIMBS 12

extern void fp765_12_add_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                             const uint64_t b[NWORDS_FIELD]);
extern void fp765_12_sub_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD],
                             const uint64_t b[NWORDS_FIELD]);
extern void fp765_12_neg_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp765_12_half_asm(uint64_t c[NWORDS_FIELD], const uint64_t a[NWORDS_FIELD]);
extern void fp765_12_montmul_asm(uint64_t *c, const uint64_t *a, const uint64_t *b);
extern void fp765_12_montsqr_asm(uint64_t *c, const uint64_t *a);
extern void fp765_12_modpro_asm(uint64_t *z, const uint64_t *w);

static const uint64_t MODULUS[FIELD_LIMBS] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x30eb7aa16fc48c0fULL, 0xe7bb8e9933551de5ULL,
    0x4acf5820369b15b0ULL, 0x140243d6c8bdb274ULL,
    0xa1434ee88e42e1e6ULL, 0x301e3eeaebbf1ce8ULL,
    0xbf54fba3dbb47692ULL, 0x1bfa60de9c0419b7ULL
};

const uint64_t p[NWORDS_FIELD] = {
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0xffffffffffffffffULL, 0xffffffffffffffffULL,
    0x30eb7aa16fc48c0fULL, 0xe7bb8e9933551de5ULL,
    0x4acf5820369b15b0ULL, 0x140243d6c8bdb274ULL,
    0xa1434ee88e42e1e6ULL, 0x301e3eeaebbf1ce8ULL,
    0xbf54fba3dbb47692ULL, 0x1bfa60de9c0419b7ULL,
    0
};

const uint64_t ONE[NWORDS_FIELD] = {
    0x0000000000000009ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x47b8b05312171370ULL, 0xda67fc9d3201f2f1ULL,
    0x5eb5e6de148c3cc7ULL, 0x4beb9d72f154b9e9ULL,
    0x54a239d2ffa60ee9ULL, 0x4eefc9bdb647fbd2ULL,
    0x4603273d46a7d4dcULL, 0x0432982c83db188aULL,
    0
};

const uint64_t ZERO[NWORDS_FIELD] = {0};

static const uint64_t MONTGOMERY_R2[FIELD_LIMBS] = {
    0x660308c39fda41daULL, 0xba8ffa09423f8002ULL,
    0xda9e0cfd85a629d1ULL, 0x0cef0cd549ec0df0ULL,
    0x7bc41101a2d4bbe7ULL, 0xcb17373b0a56f15eULL,
    0x277e690620db8b14ULL, 0xabe87bb85f5a6728ULL,
    0x94e150b3662a6f29ULL, 0xb4ee28812f8002f2ULL,
    0xa66d1adfcda31b6dULL, 0x02ff77884557b204ULL
};

static const uint64_t SMALL_MONTGOMERY[32][NWORDS_FIELD] = {
    {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL},
    {0x0000000000000009ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x47b8b05312171370ULL, 0xda67fc9d3201f2f1ULL, 0x5eb5e6de148c3cc7ULL, 0x4beb9d72f154b9e9ULL, 0x54a239d2ffa60ee9ULL, 0x4eefc9bdb647fbd2ULL, 0x4603273d46a7d4dcULL, 0x0432982c83db188aULL, 0x0000000000000000ULL},
    {0x0000000000000012ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x8f7160a6242e26e0ULL, 0xb4cff93a6403e5e2ULL, 0xbd6bcdbc2918798fULL, 0x97d73ae5e2a973d2ULL, 0xa94473a5ff4c1dd2ULL, 0x9ddf937b6c8ff7a4ULL, 0x8c064e7a8d4fa9b8ULL, 0x0865305907b63114ULL, 0x0000000000000000ULL},
    {0x000000000000001bULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xd72a10f936453a50ULL, 0x8f37f5d79605d8d3ULL, 0x1c21b49a3da4b657ULL, 0xe3c2d858d3fe2dbcULL, 0xfde6ad78fef22cbbULL, 0xeccf5d3922d7f376ULL, 0xd20975b7d3f77e94ULL, 0x0c97c8858b91499eULL, 0x0000000000000000ULL},
    {0x0000000000000024ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x1ee2c14c485c4dc0ULL, 0x699ff274c807cbc5ULL, 0x7ad79b785230f31fULL, 0x2fae75cbc552e7a5ULL, 0x5288e74bfe983ba5ULL, 0x3bbf26f6d91fef49ULL, 0x180c9cf51a9f5371ULL, 0x10ca60b20f6c6229ULL, 0x0000000000000000ULL},
    {0x000000000000002dULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x669b719f5a736130ULL, 0x4407ef11fa09beb6ULL, 0xd98d825666bd2fe7ULL, 0x7b9a133eb6a7a18eULL, 0xa72b211efe3e4a8eULL, 0x8aaef0b48f67eb1bULL, 0x5e0fc4326147284dULL, 0x14fcf8de93477ab3ULL, 0x0000000000000000ULL},
    {0x0000000000000036ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xae5421f26c8a74a0ULL, 0x1e6febaf2c0bb1a7ULL, 0x384369347b496cafULL, 0xc785b0b1a7fc5b78ULL, 0xfbcd5af1fde45977ULL, 0xd99eba7245afe6edULL, 0xa412eb6fa7eefd29ULL, 0x192f910b1722933dULL, 0x0000000000000000ULL},
    {0x0000000000000040ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xc52157a40edcfc00ULL, 0x111c59b32ab886b3ULL, 0x4c29f7f2593a93c6ULL, 0xff6f0a4dd09362edULL, 0xaf2c45dc6f47867aULL, 0xf87045451038c5d7ULL, 0x2ac1170912e25b73ULL, 0x0167c858fef99210ULL, 0x0000000000000000ULL},
    {0x0000000000000049ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0cda07f720f40f70ULL, 0xeb8456505cba79a5ULL, 0xaadfded06dc6d08dULL, 0x4b5aa7c0c1e81cd6ULL, 0x03ce7faf6eed9564ULL, 0x47600f02c680c1aaULL, 0x70c43e46598a3050ULL, 0x059a608582d4aa9aULL, 0x0000000000000000ULL},
    {0x0000000000000052ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x5492b84a330b22e0ULL, 0xc5ec52ed8ebc6c96ULL, 0x0995c5ae82530d55ULL, 0x97464533b33cd6c0ULL, 0x5870b9826e93a44dULL, 0x964fd8c07cc8bd7cULL, 0xb6c76583a032052cULL, 0x09ccf8b206afc324ULL, 0x0000000000000000ULL},
    {0x000000000000005bULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x9c4b689d45223650ULL, 0xa0544f8ac0be5f87ULL, 0x684bac8c96df4a1dULL, 0xe331e2a6a49190a9ULL, 0xad12f3556e39b336ULL, 0xe53fa27e3310b94eULL, 0xfcca8cc0e6d9da08ULL, 0x0dff90de8a8adbaeULL, 0x0000000000000000ULL},
    {0x0000000000000064ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xe40418f0573949c0ULL, 0x7abc4c27f2c05278ULL, 0xc701936aab6b86e5ULL, 0x2f1d801995e64a92ULL, 0x01b52d286ddfc220ULL, 0x342f6c3be958b521ULL, 0x42cdb3fe2d81aee5ULL, 0x1232290b0e65f439ULL, 0x0000000000000000ULL},
    {0x000000000000006dULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x2bbcc94369505d30ULL, 0x552448c524c2456aULL, 0x25b77a48bff7c3adULL, 0x7b091d8c873b047cULL, 0x565766fb6d85d109ULL, 0x831f35f99fa0b0f3ULL, 0x88d0db3b742983c1ULL, 0x1664c13792410cc3ULL, 0x0000000000000000ULL},
    {0x0000000000000076ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x737579967b6770a0ULL, 0x2f8c456256c4385bULL, 0x846d6126d4840075ULL, 0xc6f4baff788fbe65ULL, 0xaaf9a0ce6d2bdff2ULL, 0xd20effb755e8acc5ULL, 0xced40278bad1589dULL, 0x1a975964161c254dULL, 0x0000000000000000ULL},
    {0x0000000000000080ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x8a42af481db9f800ULL, 0x2238b36655710d67ULL, 0x9853efe4b275278cULL, 0xfede149ba126c5daULL, 0x5e588bb8de8f0cf5ULL, 0xf0e08a8a20718bafULL, 0x55822e1225c4b6e7ULL, 0x02cf90b1fdf32420ULL, 0x0000000000000000ULL},
    {0x0000000000000089ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xd1fb5f9b2fd10b70ULL, 0xfca0b00387730058ULL, 0xf709d6c2c7016453ULL, 0x4ac9b20e927b7fc3ULL, 0xb2fac58bde351bdfULL, 0x3fd05447d6b98781ULL, 0x9b85554f6c6c8bc4ULL, 0x070228de81ce3caaULL, 0x0000000000000000ULL},
    {0x0000000000000092ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x19b40fee41e81ee0ULL, 0xd708aca0b974f34aULL, 0x55bfbda0db8da11bULL, 0x96b54f8183d039adULL, 0x079cff5edddb2ac8ULL, 0x8ec01e058d018354ULL, 0xe1887c8cb31460a0ULL, 0x0b34c10b05a95534ULL, 0x0000000000000000ULL},
    {0x000000000000009bULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x616cc04153ff3250ULL, 0xb170a93deb76e63bULL, 0xb475a47ef019dde3ULL, 0xe2a0ecf47524f396ULL, 0x5c3f3931dd8139b1ULL, 0xddafe7c343497f26ULL, 0x278ba3c9f9bc357cULL, 0x0f67593789846dbfULL, 0x0000000000000000ULL},
    {0x00000000000000a4ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xa9257094661645c0ULL, 0x8bd8a5db1d78d92cULL, 0x132b8b5d04a61aabULL, 0x2e8c8a676679ad80ULL, 0xb0e17304dd27489bULL, 0x2c9fb180f9917af8ULL, 0x6d8ecb0740640a59ULL, 0x1399f1640d5f8649ULL, 0x0000000000000000ULL},
    {0x00000000000000adULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xf0de20e7782d5930ULL, 0x6640a2784f7acc1dULL, 0x71e1723b19325773ULL, 0x7a7827da57ce6769ULL, 0x0583acd7dccd5784ULL, 0x7b8f7b3eafd976cbULL, 0xb391f244870bdf35ULL, 0x17cc8990913a9ed3ULL, 0x0000000000000000ULL},
    {0x00000000000000b7ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x07ab56991a7fe090ULL, 0x58ed107c4e27a12aULL, 0x85c800f8f7237e8aULL, 0xb261817680656edeULL, 0xb8e297c24e308487ULL, 0x9a6106117a6255b4ULL, 0x3a401dddf1ff3d7fULL, 0x0004c0de79119da6ULL, 0x0000000000000000ULL},
    {0x00000000000000c0ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x4f6406ec2c96f400ULL, 0x33550d198029941bULL, 0xe47de7d70bafbb52ULL, 0xfe4d1ee971ba28c7ULL, 0x0d84d1954dd69370ULL, 0xe950cfcf30aa5187ULL, 0x8043451b38a7125bULL, 0x0437590afcecb630ULL, 0x0000000000000000ULL},
    {0x00000000000000c9ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x971cb73f3eae0770ULL, 0x0dbd09b6b22b870cULL, 0x4333ceb5203bf81aULL, 0x4a38bc5c630ee2b1ULL, 0x62270b684d7ca25aULL, 0x3840998ce6f24d59ULL, 0xc6466c587f4ee738ULL, 0x0869f13780c7cebaULL, 0x0000000000000000ULL},
    {0x00000000000000d2ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xded5679250c51ae0ULL, 0xe8250653e42d79fdULL, 0xa1e9b59334c834e1ULL, 0x962459cf54639c9aULL, 0xb6c9453b4d22b143ULL, 0x8730634a9d3a492bULL, 0x0c499395c5f6bc14ULL, 0x0c9c896404a2e745ULL, 0x0000000000000000ULL},
    {0x00000000000000dbULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x268e17e562dc2e50ULL, 0xc28d02f1162f6cefULL, 0x009f9c71495471a9ULL, 0xe20ff74245b85684ULL, 0x0b6b7f0e4cc8c02cULL, 0xd6202d08538244feULL, 0x524cbad30c9e90f0ULL, 0x10cf2190887dffcfULL, 0x0000000000000000ULL},
    {0x00000000000000e4ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x6e46c83874f341c0ULL, 0x9cf4ff8e48315fe0ULL, 0x5f55834f5de0ae71ULL, 0x2dfb94b5370d106dULL, 0x600db8e14c6ecf16ULL, 0x250ff6c609ca40d0ULL, 0x984fe210534665cdULL, 0x1501b9bd0c591859ULL, 0x0000000000000000ULL},
    {0x00000000000000edULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xb5ff788b870a5530ULL, 0x775cfc2b7a3352d1ULL, 0xbe0b6a2d726ceb39ULL, 0x79e732282861ca56ULL, 0xb4aff2b44c14ddffULL, 0x73ffc083c0123ca2ULL, 0xde53094d99ee3aa9ULL, 0x193451e9903430e3ULL, 0x0000000000000000ULL},
    {0x00000000000000f7ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xccccae3d295cdc90ULL, 0x6a096a2f78e027ddULL, 0xd1f1f8eb505e1250ULL, 0xb1d08bc450f8d1cbULL, 0x680edd9ebd780b02ULL, 0x92d14b568a9b1b8cULL, 0x650134e704e198f3ULL, 0x016c8937780b2fb6ULL, 0x0000000000000000ULL},
    {0x0000000000000100ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x14855e903b73f000ULL, 0x447166ccaae21acfULL, 0x30a7dfc964ea4f18ULL, 0xfdbc2937424d8bb5ULL, 0xbcb11771bd1e19ebULL, 0xe1c1151440e3175eULL, 0xab045c244b896dcfULL, 0x059f2163fbe64840ULL, 0x0000000000000000ULL},
    {0x0000000000000109ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x5c3e0ee34d8b0370ULL, 0x1ed96369dce40dc0ULL, 0x8f5dc6a779768be0ULL, 0x49a7c6aa33a2459eULL, 0x11535144bcc428d5ULL, 0x30b0ded1f72b1331ULL, 0xf1078361923142acULL, 0x09d1b9907fc160caULL, 0x0000000000000000ULL},
    {0x0000000000000112ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xa3f6bf365fa216e0ULL, 0xf94160070ee600b1ULL, 0xee13ad858e02c8a7ULL, 0x9593641d24f6ff87ULL, 0x65f58b17bc6a37beULL, 0x7fa0a88fad730f03ULL, 0x370aaa9ed8d91788ULL, 0x0e0451bd039c7955ULL, 0x0000000000000000ULL},
    {0x000000000000011bULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xebaf6f8971b92a50ULL, 0xd3a95ca440e7f3a2ULL, 0x4cc99463a28f056fULL, 0xe17f0190164bb971ULL, 0xba97c4eabc1046a7ULL, 0xce90724d63bb0ad5ULL, 0x7d0dd1dc1f80ec64ULL, 0x1236e9e9877791dfULL, 0x0000000000000000ULL}
};

static inline void clear_unused_limb(uint64_t *a) {
    a[12] = 0;
}

static void copy_field(uint64_t *dst, const uint64_t *src) {
    memcpy(dst, src, sizeof(uint64_t) * FIELD_LIMBS);
    clear_unused_limb(dst);
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
    clear_unused_limb(a);

    return borrow ^ 1u;
}

static void reduce_u768(uint64_t *a) {
    for (int i = 0; i < 9; i++) {
        (void)subtract_modulus_if_possible(a);
    }
    clear_unused_limb(a);
}

static void montgomery_multiply(uint64_t *c, const uint64_t *a, const uint64_t *b) {
    fp765_12_montmul_asm(c, a, b);
}

static void to_montgomery(const uint64_t *m, uint64_t *n) {
    montgomery_multiply(n, m, MONTGOMERY_R2);
}

static void from_montgomery(const uint64_t *n, uint64_t *m) {
    static const uint64_t normal_one[FIELD_LIMBS] = {1};
    montgomery_multiply(m, n, normal_one);
}

static int is_zero_montgomery(const uint64_t *a);

static void repeat_square(uint64_t *a, int n) {
    for (int i = 0; i < n; i++) {
        fp765_12_montsqr_asm(a, a);
    }
}

static int is_one_montgomery(const uint64_t *a) {
    uint64_t normal[NWORDS_FIELD];
    uint64_t diff = 0;

    from_montgomery(a, normal);
    diff |= normal[0] ^ 1u;
    for (int i = 1; i < FIELD_LIMBS; i++) {
        diff |= normal[i];
    }

    return diff == 0;
}

static void invert_montgomery(const uint64_t *x, uint64_t *z) {
    fp_t progenitor;

    fp765_12_modpro_asm(progenitor, x);
    repeat_square(progenitor, 2);
    montgomery_multiply(z, x, progenitor);
}

static int is_square_montgomery(const uint64_t *x) {
    fp_t check;

    fp765_12_modpro_asm(check, x);
    fp765_12_montsqr_asm(check, check);
    montgomery_multiply(check, check, x);

    return is_one_montgomery(check) | is_zero_montgomery(x);
}

static void sqrt_montgomery(const uint64_t *x, uint64_t *r) {
    fp_t root_factor;

    fp765_12_modpro_asm(root_factor, x);
    montgomery_multiply(r, root_factor, x);
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
    fp765_12_add_asm(*out, *a, *b);
}

void fp_sub(fp_t *out, const fp_t *a, const fp_t *b) {
    fp765_12_sub_asm(*out, *a, *b);
}

void fp_sqr(fp_t *out, const fp_t *a) {
    fp765_12_montsqr_asm(*out, *a);
}

void fp_mul(fp_t *out, const fp_t *a, const fp_t *b) {
    montgomery_multiply(*out, *a, *b);
}

void fp_tomont(fp_t *out, const fp_t *a) {
    fp_t normal;

    copy_field(normal, *a);
    reduce_u768(normal);
    to_montgomery(normal, *out);
}

void fp_frommont(fp_t *out, const fp_t *a) {
    from_montgomery(*a, *out);
}

void fp_mont_setone(fp_t *out) {
    memcpy(*out, ONE, sizeof(ONE));
}

void fp_neg(fp_t *out, const fp_t *a) {
    fp765_12_neg_asm(*out, *a);
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
    if (val < 32) {
        memcpy(*x, SMALL_MONTGOMERY[val], sizeof(fp_t));
        return;
    }

    fp_t normal = {0};

    normal[0] = val;
    to_montgomery(normal, *x);
}

void fp_half(fp_t *out, const fp_t *a) {
    fp765_12_half_asm(*out, *a);
}

void fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl) {
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (unsigned int i = 0; i < FIELD_LIMBS; i++) {
        (*d)[i] = (*a0)[i] ^ (mask & ((*a0)[i] ^ (*a1)[i]));
    }
    clear_unused_limb(*d);
}

void fp_cswap(fp_t *a, fp_t *b, uint32_t ctl) {
    uint64_t mask = (uint64_t)(int32_t)ctl;

    for (unsigned int i = 0; i < FIELD_LIMBS; i++) {
        uint64_t swap = mask & ((*a)[i] ^ (*b)[i]);
        (*a)[i] ^= swap;
        (*b)[i] ^= swap;
    }
    clear_unused_limb(*a);
    clear_unused_limb(*b);
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
    reduce_u768(*d);
    to_montgomery(*d, *d);
}

void fp_decode(fp_t *d, const void *src) {
    memcpy(*d, src, FP_NBYTES);
    clear_unused_limb(*d);
    reduce_u768(*d);
    to_montgomery(*d, *d);
}

void fp_inv(fp_t *a) {
    if (is_zero_montgomery(*a)) {
        fp_set_zero(a);
        return;
    }

    invert_montgomery(*a, *a);
}

uint32_t fp_is_square(const fp_t *a) {
    return -(uint32_t)(is_square_montgomery(*a) == 1);
}

void fp_sqrt(fp_t *a) {
    fp_t negative_root;
    uint64_t mask;

    sqrt_montgomery(*a, *a);
    fp_neg(&negative_root, a);

    mask = (uint64_t)0 - (uint64_t)sign_montgomery(*a);
    for (int i = 0; i < NWORDS_FIELD; i++) {
        (*a)[i] = ((*a)[i] & ~mask) | (negative_root[i] & mask);
    }
}
