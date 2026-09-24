#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "afs_tredm.h"
#include "afs_p1600.h"
#if defined(AFS_TREDM_OPT64_PORTABLE)
#include "CryptHash_AlgorithmInstance.h"
#endif
#if defined(AFS_TREDM_USE_AVX2) && defined(__AVX2__)
#include <immintrin.h>
#endif

#define AFS_TREDM_VERSION 2U
#define AFS_TREDM_STATE_BITS 1600U
#define AFS_TREDM_MAX_RATE_LANES 16U

#if !defined(AFS_TREDM_OPT64_PORTABLE) || DIGEST_BIT_LENGTH == 512
static const uint64_t IV_512[25] = {
    0x53994199EC2747FFULL, 0xBFD3CD9EBA914A21ULL, 0xE23244DDD302409BULL, 0xB82F2C0D67F11A56ULL, 0x2D61D2E164DC0C64ULL,
    0xF604258A090B4F87ULL, 0xE583B2DE7BCC0934ULL, 0x9C1960759D1FD367ULL, 0xCDE041B956651F19ULL, 0x32D52A1048F20B3FULL,
    0xDA10FA3BE09F1AEEULL, 0xC22C74696B68742DULL, 0x81A2E4EC00649D7FULL, 0x0057434538916AEAULL, 0x4FEA9520E57D8E11ULL,
    0x3121586C1336D4CBULL, 0xD652524E1FB3F916ULL, 0x27804CBD8F09D5F2ULL, 0x9F7DC67670C3DF84ULL, 0xEB9CDC94C6B7B81BULL,
    0x1388619624CA2963ULL, 0x09B71D9B01191FEDULL, 0x14CFDB61E20D4FDCULL, 0x3DFA768C46B23134ULL, 0xE50AEFF14CB6DA44ULL
};
#endif

#if !defined(AFS_TREDM_OPT64_PORTABLE) || DIGEST_BIT_LENGTH == 768
static const uint64_t IV_768[25] = {
    0xB0F2BB7C26FDCB63ULL, 0x41D198EC6D5B2B7BULL, 0x751BCA414F8B3A3DULL, 0x9B4D5ECA37E5DCA2ULL, 0xD5451D0D87B13E70ULL,
    0x29BDA85FB12825C0ULL, 0xEFAE039D229BB856ULL, 0x5533F9678259C50FULL, 0x57F530B2442DEC6CULL, 0xED048C2F7D498654ULL,
    0x0A7306A3F12442A3ULL, 0xCCE25A5F87D775C1ULL, 0x079D4E0253B563AAULL, 0xAC33774D15DBC6FDULL, 0x81EC4A8254BFD641ULL,
    0xC3B964F0008423A0ULL, 0x06B3661AA4B14E44ULL, 0xC47AD03E50312E31ULL, 0x6AD795F957D10241ULL, 0xEF8D172A503F8F62ULL,
    0xEB3CA46C3DD3FB80ULL, 0xECEE8411633651CDULL, 0xD7139EC73D704F63ULL, 0xE307189922FE9F8EULL, 0x73DD2748F22D7F1FULL
};
#endif

#if !defined(AFS_TREDM_OPT64_PORTABLE) || DIGEST_BIT_LENGTH == 1024
static const uint64_t IV_1024[25] = {
    0xA4F69C995CEFCD69ULL, 0x06991C0A3E9FB307ULL, 0x6A07F13502426450ULL, 0x60F0251F0F6A8899ULL, 0x47F583E911141969ULL,
    0xC5B7C6E311543B3FULL, 0x2C0515877EC5BDAFULL, 0x14A57976513CCA55ULL, 0xEA295865D33E9D7BULL, 0x3F17E94697F82F64ULL,
    0xF0D601977DB63BA5ULL, 0x6C2026D3858676EEULL, 0x7D0C3960480FD237ULL, 0x2D8ED3ED88F475D7ULL, 0x9332B33412A596F6ULL,
    0x9DF09750C9D639B4ULL, 0x52D73E916617792CULL, 0x6493DF5CE7DF5A21ULL, 0x20384DA1E6C497FEULL, 0x61C456C0442053A2ULL,
    0xE9CDFD19AAEC53A1ULL, 0xEAEA95C4CEA0E06EULL, 0x19F1B0C69940D2BEULL, 0x4F4A68E0F0FB52F1ULL, 0xA59BEAD309B75F6BULL
};
#endif


#if defined(AFS_TREDM_OPT64_PORTABLE) && defined(DIGEST_BIT_LENGTH)
# if DIGEST_BIT_LENGTH == 512
#  define AFS_TREDM_FIXED_DIGEST_BITS 512U
#  define AFS_TREDM_FIXED_RATE_BITS 1024U
#  define AFS_TREDM_FIXED_CAPACITY_BITS 576U
#  define AFS_TREDM_FIXED_RATE_LANES 16U
#  define AFS_TREDM_FIXED_CAPACITY_LANES 9U
#  define AFS_TREDM_FIXED_DIGEST_LANES 8U
#  define AFS_TREDM_FIXED_IV IV_512
# elif DIGEST_BIT_LENGTH == 768
#  define AFS_TREDM_FIXED_DIGEST_BITS 768U
#  define AFS_TREDM_FIXED_RATE_BITS 768U
#  define AFS_TREDM_FIXED_CAPACITY_BITS 832U
#  define AFS_TREDM_FIXED_RATE_LANES 12U
#  define AFS_TREDM_FIXED_CAPACITY_LANES 13U
#  define AFS_TREDM_FIXED_DIGEST_LANES 12U
#  define AFS_TREDM_FIXED_IV IV_768
# elif DIGEST_BIT_LENGTH == 1024
#  define AFS_TREDM_FIXED_DIGEST_BITS 1024U
#  define AFS_TREDM_FIXED_RATE_BITS 512U
#  define AFS_TREDM_FIXED_CAPACITY_BITS 1088U
#  define AFS_TREDM_FIXED_RATE_LANES 8U
#  define AFS_TREDM_FIXED_CAPACITY_LANES 17U
#  define AFS_TREDM_FIXED_DIGEST_LANES 16U
#  define AFS_TREDM_FIXED_IV IV_1024
# endif
# define AFS_TREDM_FIXED_SUFFIX_LANE ( \
    (((uint64_t)AFS_TREDM_FIXED_DIGEST_BITS) << 48) | \
    (((uint64_t)AFS_TREDM_FIXED_RATE_BITS) << 32) | \
    (((uint64_t)AFS_TREDM_FIXED_CAPACITY_BITS) << 16) | \
    ((uint64_t)AFS_TREDM_VERSION) )
#endif

#if defined(__GNUC__) || defined(__clang__)
# define AFS_TREDM_ALIGN32 __attribute__((aligned(32)))
# define AFS_TREDM_MAYBE_UNUSED __attribute__((unused))
#else
# define AFS_TREDM_ALIGN32
# define AFS_TREDM_MAYBE_UNUSED
#endif

/* Function load_be64: loads one big-endian 64-bit lane from bytes. */
static uint64_t load_be64(const unsigned char *p)
{
    uint64_t x;
    memcpy(&x, p, sizeof(x));
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
# if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(x);
# else
    return ((x & UINT64_C(0x00000000000000FF)) << 56) |
           ((x & UINT64_C(0x000000000000FF00)) << 40) |
           ((x & UINT64_C(0x0000000000FF0000)) << 24) |
           ((x & UINT64_C(0x00000000FF000000)) << 8)  |
           ((x & UINT64_C(0x000000FF00000000)) >> 8)  |
           ((x & UINT64_C(0x0000FF0000000000)) >> 24) |
           ((x & UINT64_C(0x00FF000000000000)) >> 40) |
           ((x & UINT64_C(0xFF00000000000000)) >> 56);
# endif
#else
    return x;
#endif
}

/* Function store_be64: stores one 64-bit lane to bytes in big-endian order. */
static void store_be64(unsigned char *p, uint64_t x)
{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
# if defined(__GNUC__) || defined(__clang__)
    x = __builtin_bswap64(x);
# else
    x = ((x & UINT64_C(0x00000000000000FF)) << 56) |
        ((x & UINT64_C(0x000000000000FF00)) << 40) |
        ((x & UINT64_C(0x0000000000FF0000)) << 24) |
        ((x & UINT64_C(0x00000000FF000000)) << 8)  |
        ((x & UINT64_C(0x000000FF00000000)) >> 8)  |
        ((x & UINT64_C(0x0000FF0000000000)) >> 24) |
        ((x & UINT64_C(0x00FF000000000000)) >> 40) |
        ((x & UINT64_C(0xFF00000000000000)) >> 56);
# endif
#endif
    memcpy(p, &x, sizeof(x));
}

/* Function store_be16: stores a 16-bit metadata field in big-endian order. */
static void store_be16(unsigned char *p, unsigned x)
{
    p[0] = (unsigned char)(x >> 8);
    p[1] = (unsigned char)x;
}

/* Function get_msg_bit_msb: reads a message bit using the MSB-first partial-byte convention. */
static unsigned get_msg_bit_msb(const unsigned char *msg, unsigned long long bitpos)
{
    return (unsigned)((msg[bitpos >> 3] >> (7U - (unsigned)(bitpos & 7ULL))) & 1U);
}

/* Function set_lane_bit_msb: sets a bit in a rate-lane block using the MSB-first state convention. */
static void set_lane_bit_msb(uint64_t block[AFS_TREDM_MAX_RATE_LANES],
                             unsigned bitpos,
                             unsigned bit)
{
    unsigned lane = bitpos >> 6;
    unsigned off = bitpos & 63U;
    if (bit) {
        block[lane] |= (uint64_t)1 << (63U - off);
    }
}

/* Function set_frame_bit: sets a bit in one of the final framed rate blocks. */
static void set_frame_bit(uint64_t frame[2][AFS_TREDM_MAX_RATE_LANES],
                          unsigned rate_bits,
                          unsigned bitpos,
                          unsigned bit)
{
    unsigned blk = bitpos / rate_bits;
    unsigned off = bitpos % rate_bits;
    set_lane_bit_msb(frame[blk], off, bit);
}

/* Function suffix_bit: returns one bit of the 128-bit frame metadata suffix. */
static unsigned suffix_bit(unsigned idx,
                           unsigned long long msg_len_bits,
                           unsigned digest_len_bits,
                           unsigned rate_bits,
                           unsigned capacity_bits)
{
    if (idx < 64U) {
        return (unsigned)((msg_len_bits >> (63U - idx)) & 1ULL);
    }
    idx -= 64U;
    if (idx < 16U) {
        return (digest_len_bits >> (15U - idx)) & 1U;
    }
    idx -= 16U;
    if (idx < 16U) {
        return (rate_bits >> (15U - idx)) & 1U;
    }
    idx -= 16U;
    if (idx < 16U) {
        return (capacity_bits >> (15U - idx)) & 1U;
    }
    idx -= 16U;
    return (AFS_TREDM_VERSION >> (15U - idx)) & 1U;
}

/* Function absorb_prepared_block: absorbs one prepared rate block with the aligned TrEDM feed-forward step. */
static void absorb_prepared_block(uint64_t A[25],
                                  const uint64_t block[AFS_TREDM_MAX_RATE_LANES],
                                  unsigned rate_lanes,
                                  unsigned capacity_lanes)
{
    uint64_t saved_capacity[17];
    unsigned i;
    unsigned cap_start = rate_lanes;

    for (i = 0; i < rate_lanes; i++) {
        A[i] ^= block[i];
    }

    for (i = 0; i < capacity_lanes; i++) {
        saved_capacity[i] = A[cap_start + i];
    }

    afs_p1600_g(A);

    for (i = 0; i < capacity_lanes; i++) {
        A[cap_start + i] ^= saved_capacity[i];
    }

    afs_p1600_h(A);
}

/* Function absorb_message_full_blocks: absorbs all complete message rate blocks before final framing. */
static void AFS_TREDM_MAYBE_UNUSED absorb_message_full_blocks(uint64_t A[25],
                                       const unsigned char *msg,
                                       unsigned long long full_blocks,
                                       unsigned rate_bits,
                                       unsigned rate_lanes,
                                       unsigned capacity_lanes)
{
    unsigned long long b;
    unsigned i;
    unsigned block_bytes = rate_bits >> 3U;
    unsigned cap_start = rate_lanes;

    for (b = 0ULL; b < full_blocks; b++) {
        uint64_t saved_capacity[17];
        const unsigned char *p = msg + b * (unsigned long long)block_bytes;

        for (i = 0; i < capacity_lanes; i++) {
            saved_capacity[i] = A[cap_start + i];
        }

        /*
         * Full-block fast path.  All supported rates are multiples of 64 bits,
         * so the message block can be absorbed directly as big-endian lanes.
         */
        for (i = 0; i < rate_lanes; i++) {
            A[i] ^= load_be64(p + 8U * i);
        }

        afs_p1600_g(A);

        for (i = 0; i < capacity_lanes; i++) {
            A[cap_start + i] ^= saved_capacity[i];
        }

        afs_p1600_h(A);
    }
}


#if defined(AFS_TREDM_FIXED_RATE_LANES)

#define AFS_TREDM_SAVE_CAPACITY_FIXED()                                      \
    uint64_t c0 = A[AFS_TREDM_FIXED_RATE_LANES + 0U];                        \
    uint64_t c1 = A[AFS_TREDM_FIXED_RATE_LANES + 1U];                        \
    uint64_t c2 = A[AFS_TREDM_FIXED_RATE_LANES + 2U];                        \
    uint64_t c3 = A[AFS_TREDM_FIXED_RATE_LANES + 3U];                        \
    uint64_t c4 = A[AFS_TREDM_FIXED_RATE_LANES + 4U];                        \
    uint64_t c5 = A[AFS_TREDM_FIXED_RATE_LANES + 5U];                        \
    uint64_t c6 = A[AFS_TREDM_FIXED_RATE_LANES + 6U];                        \
    uint64_t c7 = A[AFS_TREDM_FIXED_RATE_LANES + 7U];                        \
    uint64_t c8 = A[AFS_TREDM_FIXED_RATE_LANES + 8U];                        \
    uint64_t c9 = UINT64_C(0);                                               \
    uint64_t c10 = UINT64_C(0);                                              \
    uint64_t c11 = UINT64_C(0);                                              \
    uint64_t c12 = UINT64_C(0);                                              \
    uint64_t c13 = UINT64_C(0);                                              \
    uint64_t c14 = UINT64_C(0);                                              \
    uint64_t c15 = UINT64_C(0);                                              \
    uint64_t c16 = UINT64_C(0);                                              \
    do {                                                                     \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 9U)  c9  = A[AFS_TREDM_FIXED_RATE_LANES + 9U];  \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 10U) c10 = A[AFS_TREDM_FIXED_RATE_LANES + 10U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 11U) c11 = A[AFS_TREDM_FIXED_RATE_LANES + 11U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 12U) c12 = A[AFS_TREDM_FIXED_RATE_LANES + 12U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 13U) c13 = A[AFS_TREDM_FIXED_RATE_LANES + 13U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 14U) c14 = A[AFS_TREDM_FIXED_RATE_LANES + 14U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 15U) c15 = A[AFS_TREDM_FIXED_RATE_LANES + 15U]; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 16U) c16 = A[AFS_TREDM_FIXED_RATE_LANES + 16U]; \
    } while (0)

#define AFS_TREDM_FEED_FORWARD_CAPACITY_FIXED()                              \
    do {                                                                     \
        A[AFS_TREDM_FIXED_RATE_LANES + 0U] ^= c0;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 1U] ^= c1;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 2U] ^= c2;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 3U] ^= c3;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 4U] ^= c4;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 5U] ^= c5;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 6U] ^= c6;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 7U] ^= c7;                            \
        A[AFS_TREDM_FIXED_RATE_LANES + 8U] ^= c8;                            \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 9U)  A[AFS_TREDM_FIXED_RATE_LANES + 9U]  ^= c9;  \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 10U) A[AFS_TREDM_FIXED_RATE_LANES + 10U] ^= c10; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 11U) A[AFS_TREDM_FIXED_RATE_LANES + 11U] ^= c11; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 12U) A[AFS_TREDM_FIXED_RATE_LANES + 12U] ^= c12; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 13U) A[AFS_TREDM_FIXED_RATE_LANES + 13U] ^= c13; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 14U) A[AFS_TREDM_FIXED_RATE_LANES + 14U] ^= c14; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 15U) A[AFS_TREDM_FIXED_RATE_LANES + 15U] ^= c15; \
        if (AFS_TREDM_FIXED_CAPACITY_LANES > 16U) A[AFS_TREDM_FIXED_RATE_LANES + 16U] ^= c16; \
    } while (0)

/* Function absorb_prepared_be_block_fixed: absorbs one already big-endian prepared block in a fixed profile. */
static void absorb_prepared_be_block_fixed(uint64_t A[25], const unsigned char *p)
{
    AFS_TREDM_SAVE_CAPACITY_FIXED();

    A[0] ^= load_be64(p + 0U);
    A[1] ^= load_be64(p + 8U);
    A[2] ^= load_be64(p + 16U);
    A[3] ^= load_be64(p + 24U);
    A[4] ^= load_be64(p + 32U);
    A[5] ^= load_be64(p + 40U);
    A[6] ^= load_be64(p + 48U);
    A[7] ^= load_be64(p + 56U);
#if AFS_TREDM_FIXED_RATE_LANES > 8U
    A[8] ^= load_be64(p + 64U);
    A[9] ^= load_be64(p + 72U);
    A[10] ^= load_be64(p + 80U);
    A[11] ^= load_be64(p + 88U);
#endif
#if AFS_TREDM_FIXED_RATE_LANES > 12U
    A[12] ^= load_be64(p + 96U);
    A[13] ^= load_be64(p + 104U);
    A[14] ^= load_be64(p + 112U);
    A[15] ^= load_be64(p + 120U);
#endif

    afs_p1600_g(A);
    AFS_TREDM_FEED_FORWARD_CAPACITY_FIXED();
    afs_p1600_h(A);
}

/* Function absorb_prepared_lanes_fixed: absorbs one prepared lane block in a fixed profile. */
static void absorb_prepared_lanes_fixed(uint64_t A[25],
                                        const uint64_t block[AFS_TREDM_FIXED_RATE_LANES])
{
    AFS_TREDM_SAVE_CAPACITY_FIXED();

    A[0] ^= block[0];
    A[1] ^= block[1];
    A[2] ^= block[2];
    A[3] ^= block[3];
    A[4] ^= block[4];
    A[5] ^= block[5];
    A[6] ^= block[6];
    A[7] ^= block[7];
#if AFS_TREDM_FIXED_RATE_LANES > 8U
    A[8] ^= block[8];
    A[9] ^= block[9];
    A[10] ^= block[10];
    A[11] ^= block[11];
#endif
#if AFS_TREDM_FIXED_RATE_LANES > 12U
    A[12] ^= block[12];
    A[13] ^= block[13];
    A[14] ^= block[14];
    A[15] ^= block[15];
#endif

    afs_p1600_g(A);
    AFS_TREDM_FEED_FORWARD_CAPACITY_FIXED();
    afs_p1600_h(A);
}

/* Function load_be64_partial_pad80: loads a partial big-endian lane and applies the frame 1-bit padding marker. */
static uint64_t load_be64_partial_pad80(const unsigned char *p, unsigned n)
{
    uint64_t x = UINT64_C(0);
    unsigned i;

    for (i = 0U; i < n; i++) {
        x |= (uint64_t)p[i] << (56U - 8U * i);
    }
    x |= UINT64_C(0x80) << (56U - 8U * n);
    return x;
}


/* Function absorb_final_zero_rem_block_fixed: absorbs the fixed-profile final block when no message tail remains. */
static void absorb_final_zero_rem_block_fixed(uint64_t A[25],
                                              unsigned long long msg_len_bits)
{
    /*
     * Byte-aligned rem=0 final block.  This covers empty messages and
     * rate-multiple messages, including the AFS-TrEDM-1024 64-byte
     * boundary case.  It avoids building a temporary byte frame and
     * absorbing all-zero rate lanes.
     */
    AFS_TREDM_SAVE_CAPACITY_FIXED();

    A[0] ^= UINT64_C(0x8000000000000000);
    A[AFS_TREDM_FIXED_RATE_LANES - 2U] ^= (uint64_t)msg_len_bits;
    A[AFS_TREDM_FIXED_RATE_LANES - 1U] ^= (uint64_t)AFS_TREDM_FIXED_SUFFIX_LANE;

    afs_p1600_g(A);
    AFS_TREDM_FEED_FORWARD_CAPACITY_FIXED();
    afs_p1600_h(A);
}

/* Function absorb_message_full_blocks_fixed: fixed-profile fast path for complete message rate blocks. */
static void absorb_message_full_blocks_fixed(uint64_t A[25],
                                             const unsigned char *msg,
                                             unsigned long long full_blocks)
{
    unsigned long long b;
    const unsigned block_bytes = AFS_TREDM_FIXED_RATE_BITS >> 3U;

    for (b = 0ULL; b < full_blocks; b++) {
        absorb_prepared_be_block_fixed(A, msg + b * (unsigned long long)block_bytes);
    }
}

#if AFS_TREDM_FIXED_DIGEST_BITS == 1024
/* Function absorb_1024_single_rate_block_then_final_fixed: handles the 1024-bit-rate one-full-block-plus-final fast path. */
static void AFS_TREDM_MAYBE_UNUSED
absorb_1024_single_rate_block_then_final_fixed(uint64_t A[25],
                                               const unsigned char *msg)
{
    absorb_prepared_be_block_fixed(A, msg);
    absorb_final_zero_rem_block_fixed(A, 512ULL);
}

# if defined(AFS_TREDM_ENABLE_STAGE19_1024_SHORT)
/* Function absorb_1024_short_byte_aligned_fixed: handles a short byte-aligned 1024-bit-rate message fast path. */
static void absorb_1024_short_byte_aligned_fixed(uint64_t A[25],
                                                 const unsigned char *msg,
                                                 unsigned long long msg_len_bits)
{
    const unsigned rem_bytes = (unsigned)(msg_len_bits >> 3U);
    const unsigned full_lanes = rem_bytes >> 3U;
    const unsigned tail_bytes = rem_bytes & 7U;
    uint64_t block[8] AFS_TREDM_ALIGN32;

    block[0] = UINT64_C(0);
    block[1] = UINT64_C(0);
    block[2] = UINT64_C(0);
    block[3] = UINT64_C(0);
    block[4] = UINT64_C(0);
    block[5] = UINT64_C(0);
    block[6] = UINT64_C(0);
    block[7] = UINT64_C(0);

    switch (full_lanes) {
    case 7U: block[6] = load_be64(msg + 48U); /* fall through */
    case 6U: block[5] = load_be64(msg + 40U); /* fall through */
    case 5U: block[4] = load_be64(msg + 32U); /* fall through */
    case 4U: block[3] = load_be64(msg + 24U); /* fall through */
    case 3U: block[2] = load_be64(msg + 16U); /* fall through */
    case 2U: block[1] = load_be64(msg + 8U);  /* fall through */
    case 1U: block[0] = load_be64(msg + 0U);  /* fall through */
    default: break;
    }
    block[full_lanes] = load_be64_partial_pad80(msg + 8U * full_lanes, tail_bytes);

    if (rem_bytes <= 47U) {
        block[6] = (uint64_t)msg_len_bits;
        block[7] = (uint64_t)AFS_TREDM_FIXED_SUFFIX_LANE;
        absorb_prepared_lanes_fixed(A, block);
    } else {
        absorb_prepared_lanes_fixed(A, block);
        block[0] = UINT64_C(0);
        block[1] = UINT64_C(0);
        block[2] = UINT64_C(0);
        block[3] = UINT64_C(0);
        block[4] = UINT64_C(0);
        block[5] = UINT64_C(0);
        block[6] = (uint64_t)msg_len_bits;
        block[7] = (uint64_t)AFS_TREDM_FIXED_SUFFIX_LANE;
        absorb_prepared_lanes_fixed(A, block);
    }
}
# endif
#endif

/* Function absorb_final_framed_blocks_byte_aligned_fixed: fixed-profile byte-aligned final framing and absorption. */
static void absorb_final_framed_blocks_byte_aligned_fixed(uint64_t A[25],
                                                          const unsigned char *msg,
                                                          unsigned long long msg_len_bits,
                                                          unsigned long long full_blocks)
{
    const unsigned block_bytes = AFS_TREDM_FIXED_RATE_BITS >> 3U;
    const unsigned rem_bytes = (unsigned)((msg_len_bits % (unsigned long long)AFS_TREDM_FIXED_RATE_BITS) >> 3U);
    const unsigned suffix_bits = 128U;
    const unsigned total_no_pad = (rem_bytes << 3U) + 1U + suffix_bits;
    const unsigned zero_pad = (AFS_TREDM_FIXED_RATE_BITS - (total_no_pad % AFS_TREDM_FIXED_RATE_BITS)) % AFS_TREDM_FIXED_RATE_BITS;
    const unsigned suffix_start = (rem_bytes << 3U) + 1U + zero_pad;
    const unsigned final_blocks = (total_no_pad + zero_pad) / AFS_TREDM_FIXED_RATE_BITS;
    const unsigned suffix_byte = suffix_start >> 3U;
    unsigned i;

    if (rem_bytes == 0U) {
        absorb_final_zero_rem_block_fixed(A, msg_len_bits);
        return;
    }

    (void)suffix_byte;

    {
        uint64_t block[AFS_TREDM_FIXED_RATE_LANES] AFS_TREDM_ALIGN32;
        const unsigned char *src = msg + full_blocks * (unsigned long long)block_bytes;
        const unsigned full_lanes = rem_bytes >> 3U;
        const unsigned tail_bytes = rem_bytes & 7U;

        memset(block, 0, sizeof(block));

        for (i = 0U; i < full_lanes; i++) {
            block[i] = load_be64(src + 8U * i);
        }
        block[full_lanes] = load_be64_partial_pad80(src + 8U * full_lanes, tail_bytes);

        if (final_blocks == 1U) {
            block[AFS_TREDM_FIXED_RATE_LANES - 2U] = (uint64_t)msg_len_bits;
            block[AFS_TREDM_FIXED_RATE_LANES - 1U] = (uint64_t)AFS_TREDM_FIXED_SUFFIX_LANE;
            absorb_prepared_lanes_fixed(A, block);
        } else {
            absorb_prepared_lanes_fixed(A, block);
            memset(block, 0, sizeof(block));
            block[AFS_TREDM_FIXED_RATE_LANES - 2U] = (uint64_t)msg_len_bits;
            block[AFS_TREDM_FIXED_RATE_LANES - 1U] = (uint64_t)AFS_TREDM_FIXED_SUFFIX_LANE;
            absorb_prepared_lanes_fixed(A, block);
        }
    }
}

/* Function extract_digest_fixed: extracts the digest from the fixed-profile capacity/output window. */
static void extract_digest_fixed(const uint64_t A[25], unsigned char *digest)
{
#if defined(AFS_TREDM_USE_AVX2) && defined(__AVX2__) && \
    defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && \
    (AFS_TREDM_FIXED_DIGEST_LANES == 16U)
    const __m256i bswap64 = _mm256_set_epi8(
        8, 9, 10, 11, 12, 13, 14, 15,
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        0, 1, 2, 3, 4, 5, 6, 7);
    __m256i v;

    v = _mm256_loadu_si256((const __m256i *)(const void *)(A + AFS_TREDM_FIXED_RATE_LANES + 0U));
    _mm256_storeu_si256((__m256i *)(void *)(digest + 0U), _mm256_shuffle_epi8(v, bswap64));
    v = _mm256_loadu_si256((const __m256i *)(const void *)(A + AFS_TREDM_FIXED_RATE_LANES + 4U));
    _mm256_storeu_si256((__m256i *)(void *)(digest + 32U), _mm256_shuffle_epi8(v, bswap64));
    v = _mm256_loadu_si256((const __m256i *)(const void *)(A + AFS_TREDM_FIXED_RATE_LANES + 8U));
    _mm256_storeu_si256((__m256i *)(void *)(digest + 64U), _mm256_shuffle_epi8(v, bswap64));
    v = _mm256_loadu_si256((const __m256i *)(const void *)(A + AFS_TREDM_FIXED_RATE_LANES + 12U));
    _mm256_storeu_si256((__m256i *)(void *)(digest + 96U), _mm256_shuffle_epi8(v, bswap64));
#else
    store_be64(digest + 0U, A[AFS_TREDM_FIXED_RATE_LANES + 0U]);
    store_be64(digest + 8U, A[AFS_TREDM_FIXED_RATE_LANES + 1U]);
    store_be64(digest + 16U, A[AFS_TREDM_FIXED_RATE_LANES + 2U]);
    store_be64(digest + 24U, A[AFS_TREDM_FIXED_RATE_LANES + 3U]);
    store_be64(digest + 32U, A[AFS_TREDM_FIXED_RATE_LANES + 4U]);
    store_be64(digest + 40U, A[AFS_TREDM_FIXED_RATE_LANES + 5U]);
    store_be64(digest + 48U, A[AFS_TREDM_FIXED_RATE_LANES + 6U]);
    store_be64(digest + 56U, A[AFS_TREDM_FIXED_RATE_LANES + 7U]);
#if AFS_TREDM_FIXED_DIGEST_LANES > 8U
    store_be64(digest + 64U, A[AFS_TREDM_FIXED_RATE_LANES + 8U]);
    store_be64(digest + 72U, A[AFS_TREDM_FIXED_RATE_LANES + 9U]);
    store_be64(digest + 80U, A[AFS_TREDM_FIXED_RATE_LANES + 10U]);
    store_be64(digest + 88U, A[AFS_TREDM_FIXED_RATE_LANES + 11U]);
#endif
#if AFS_TREDM_FIXED_DIGEST_LANES > 12U
    store_be64(digest + 96U, A[AFS_TREDM_FIXED_RATE_LANES + 12U]);
    store_be64(digest + 104U, A[AFS_TREDM_FIXED_RATE_LANES + 13U]);
    store_be64(digest + 112U, A[AFS_TREDM_FIXED_RATE_LANES + 14U]);
    store_be64(digest + 120U, A[AFS_TREDM_FIXED_RATE_LANES + 15U]);
#endif
#endif
}

#endif /* AFS_TREDM_FIXED_RATE_LANES */


/* Function absorb_final_framed_blocks: constructs and absorbs the final framed block or blocks. */
static void absorb_final_framed_blocks(uint64_t A[25],
                                       const unsigned char *msg,
                                       unsigned long long msg_len_bits,
                                       unsigned long long full_blocks,
                                       unsigned rate_bits,
                                       unsigned rate_lanes,
                                       unsigned capacity_lanes,
                                       unsigned digest_len_bits,
                                       unsigned capacity_bits)
{
    uint64_t frame[2][AFS_TREDM_MAX_RATE_LANES];
    unsigned rem_bits = (unsigned)(msg_len_bits % (unsigned long long)rate_bits);
    unsigned suffix_bits = 128U;
    unsigned total_no_pad = rem_bits + 1U + suffix_bits;
    unsigned zero_pad = (rate_bits - (total_no_pad % rate_bits)) % rate_bits;
    unsigned suffix_start = rem_bits + 1U + zero_pad;
    unsigned total_bits = total_no_pad + zero_pad;
    unsigned final_blocks = total_bits / rate_bits;
    unsigned i;

    memset(frame, 0, sizeof(frame));

    for (i = 0; i < rem_bits; i++) {
        unsigned long long src_bit = full_blocks * (unsigned long long)rate_bits + (unsigned long long)i;
        set_frame_bit(frame, rate_bits, i, get_msg_bit_msb(msg, src_bit));
    }

    set_frame_bit(frame, rate_bits, rem_bits, 1U);

    for (i = 0; i < suffix_bits; i++) {
        set_frame_bit(frame, rate_bits, suffix_start + i,
                      suffix_bit(i, msg_len_bits, digest_len_bits, rate_bits, capacity_bits));
    }

    for (i = 0; i < final_blocks; i++) {
        absorb_prepared_block(A, frame[i], rate_lanes, capacity_lanes);
    }
}

/* Function absorb_final_framed_blocks_byte_aligned: fast-path final framing and absorption for byte-aligned messages. */
static void AFS_TREDM_MAYBE_UNUSED absorb_final_framed_blocks_byte_aligned(uint64_t A[25],
                                                    const unsigned char *msg,
                                                    unsigned long long msg_len_bits,
                                                    unsigned long long full_blocks,
                                                    unsigned rate_bits,
                                                    unsigned rate_lanes,
                                                    unsigned capacity_lanes,
                                                    unsigned digest_len_bits,
                                                    unsigned capacity_bits)
{
    unsigned char frame_bytes[2U * (AFS_TREDM_MAX_RATE_LANES * 8U)];
    uint64_t block[AFS_TREDM_MAX_RATE_LANES];
    unsigned block_bytes = rate_bits >> 3U;
    unsigned rem_bytes = (unsigned)((msg_len_bits % (unsigned long long)rate_bits) >> 3U);
    unsigned suffix_bits = 128U;
    unsigned total_no_pad = (rem_bytes << 3U) + 1U + suffix_bits;
    unsigned zero_pad = (rate_bits - (total_no_pad % rate_bits)) % rate_bits;
    unsigned suffix_start = (rem_bytes << 3U) + 1U + zero_pad;
    unsigned final_blocks = (total_no_pad + zero_pad) / rate_bits;
    unsigned suffix_byte = suffix_start >> 3U;
    unsigned i, j;

    memset(frame_bytes, 0, 2U * (size_t)block_bytes);

    if (rem_bytes != 0U) {
        const unsigned char *src = msg + full_blocks * (unsigned long long)block_bytes;
        memcpy(frame_bytes, src, rem_bytes);
    }

    /*
     * Byte-aligned fast path.  The next frame bit is the MSB of the next
     * byte, so it is a single 0x80 store; suffix_start is always byte-aligned
     * because the 128-bit suffix is placed at the end of the final rate block.
     */
    frame_bytes[rem_bytes] = 0x80U;

    store_be64(frame_bytes + suffix_byte, msg_len_bits);
    store_be16(frame_bytes + suffix_byte + 8U, digest_len_bits);
    store_be16(frame_bytes + suffix_byte + 10U, rate_bits);
    store_be16(frame_bytes + suffix_byte + 12U, capacity_bits);
    store_be16(frame_bytes + suffix_byte + 14U, AFS_TREDM_VERSION);

    for (i = 0U; i < final_blocks; i++) {
        const unsigned char *p = frame_bytes + i * block_bytes;
        for (j = 0U; j < rate_lanes; j++) {
            block[j] = load_be64(p + 8U * j);
        }
        absorb_prepared_block(A, block, rate_lanes, capacity_lanes);
    }
}

/* Function extract_digest: extracts the digest from the aligned capacity/output window. */
static void AFS_TREDM_MAYBE_UNUSED extract_digest(const uint64_t A[25],
                           unsigned char *digest,
                           unsigned digest_len_bits,
                           unsigned rate_lanes)
{
    unsigned out_lanes = digest_len_bits >> 6;
    unsigned i;
    for (i = 0; i < out_lanes; i++) {
        store_be64(digest + 8U * i, A[rate_lanes + i]);
    }
}

/* Function afs_tredm_hash: computes an AFS-TrEDM digest for the supplied bit string. */
int afs_tredm_hash(int digest_len_bits,
                   const unsigned char *msg,
                   unsigned long long msg_len_bits,
                   unsigned char *digest)
{
    uint64_t A[25] AFS_TREDM_ALIGN32;
    unsigned long long full_blocks;

    if (digest == NULL) {
        return AFS_TREDM_NULL_POINTER;
    }
    if (msg == NULL && msg_len_bits != 0ULL) {
        return AFS_TREDM_NULL_POINTER;
    }

#if defined(AFS_TREDM_FIXED_RATE_LANES)
    if (digest_len_bits != (int)AFS_TREDM_FIXED_DIGEST_BITS) {
        return AFS_TREDM_BAD_DIGEST_LENGTH;
    }

    memcpy(A, AFS_TREDM_FIXED_IV, sizeof(A));
    full_blocks = msg_len_bits / (unsigned long long)AFS_TREDM_FIXED_RATE_BITS;

# if AFS_TREDM_FIXED_DIGEST_BITS == 1024
#  if defined(AFS_TREDM_ENABLE_STAGE19_1024_SHORT)
    if ((msg_len_bits & 7ULL) == 0ULL && msg_len_bits > 0ULL && msg_len_bits < 512ULL) {
        absorb_1024_short_byte_aligned_fixed(A, msg, msg_len_bits);
        extract_digest_fixed(A, digest);
        return AFS_TREDM_SUCCESS;
    }
#  endif
#  if !defined(AFS_TREDM_DISABLE_1024_EXACT_512_FAST)
    if (msg_len_bits == 512ULL) {
        absorb_1024_single_rate_block_then_final_fixed(A, msg);
        extract_digest_fixed(A, digest);
        return AFS_TREDM_SUCCESS;
    }
#  endif
# endif

    absorb_message_full_blocks_fixed(A, msg, full_blocks);
    if ((msg_len_bits & 7ULL) == 0ULL) {
        absorb_final_framed_blocks_byte_aligned_fixed(A, msg, msg_len_bits, full_blocks);
    } else {
        absorb_final_framed_blocks(A, msg, msg_len_bits, full_blocks,
                                   AFS_TREDM_FIXED_RATE_BITS,
                                   AFS_TREDM_FIXED_RATE_LANES,
                                   AFS_TREDM_FIXED_CAPACITY_LANES,
                                   AFS_TREDM_FIXED_DIGEST_BITS,
                                   AFS_TREDM_FIXED_CAPACITY_BITS);
    }
    extract_digest_fixed(A, digest);

    return AFS_TREDM_SUCCESS;

#else
    unsigned rate_bits, capacity_bits, rate_lanes, capacity_lanes;

# if defined(AFS_TREDM_OPT64_PORTABLE) && defined(DIGEST_BIT_LENGTH)
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return AFS_TREDM_BAD_DIGEST_LENGTH;
    }
#  if DIGEST_BIT_LENGTH == 512
    rate_bits = 1024U;
    capacity_bits = 576U;
    memcpy(A, IV_512, sizeof(A));
#  elif DIGEST_BIT_LENGTH == 768
    rate_bits = 768U;
    capacity_bits = 832U;
    memcpy(A, IV_768, sizeof(A));
#  elif DIGEST_BIT_LENGTH == 1024
    rate_bits = 512U;
    capacity_bits = 1088U;
    memcpy(A, IV_1024, sizeof(A));
#  else
#   error Unsupported AFS-TrEDM DIGEST_BIT_LENGTH
#  endif
# else
    if (digest_len_bits == 512) {
        rate_bits = 1024U;
        capacity_bits = 576U;
        memcpy(A, IV_512, sizeof(A));
    } else if (digest_len_bits == 768) {
        rate_bits = 768U;
        capacity_bits = 832U;
        memcpy(A, IV_768, sizeof(A));
    } else if (digest_len_bits == 1024) {
        rate_bits = 512U;
        capacity_bits = 1088U;
        memcpy(A, IV_1024, sizeof(A));
    } else {
        return AFS_TREDM_BAD_DIGEST_LENGTH;
    }
# endif

    rate_lanes = rate_bits >> 6U;
    capacity_lanes = capacity_bits >> 6U;
    full_blocks = msg_len_bits / (unsigned long long)rate_bits;

    absorb_message_full_blocks(A, msg, full_blocks, rate_bits, rate_lanes, capacity_lanes);
    if ((msg_len_bits & 7ULL) == 0ULL) {
        absorb_final_framed_blocks_byte_aligned(A, msg, msg_len_bits, full_blocks, rate_bits,
                                                rate_lanes, capacity_lanes,
                                                (unsigned)digest_len_bits, capacity_bits);
    } else {
        absorb_final_framed_blocks(A, msg, msg_len_bits, full_blocks, rate_bits, rate_lanes,
                                   capacity_lanes, (unsigned)digest_len_bits, capacity_bits);
    }
    extract_digest(A, digest, (unsigned)digest_len_bits, rate_lanes);

    return AFS_TREDM_SUCCESS;
#endif
}
