#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "afs_tredm.h"
#include "afs_p1600.h"

#define AFS_TREDM_VERSION 2U
#define AFS_TREDM_STATE_BITS 1600U
#define AFS_TREDM_MAX_RATE_LANES 16U

static const uint64_t IV_512[25] = {
    0x53994199EC2747FFULL, 0xBFD3CD9EBA914A21ULL, 0xE23244DDD302409BULL, 0xB82F2C0D67F11A56ULL, 0x2D61D2E164DC0C64ULL,
    0xF604258A090B4F87ULL, 0xE583B2DE7BCC0934ULL, 0x9C1960759D1FD367ULL, 0xCDE041B956651F19ULL, 0x32D52A1048F20B3FULL,
    0xDA10FA3BE09F1AEEULL, 0xC22C74696B68742DULL, 0x81A2E4EC00649D7FULL, 0x0057434538916AEAULL, 0x4FEA9520E57D8E11ULL,
    0x3121586C1336D4CBULL, 0xD652524E1FB3F916ULL, 0x27804CBD8F09D5F2ULL, 0x9F7DC67670C3DF84ULL, 0xEB9CDC94C6B7B81BULL,
    0x1388619624CA2963ULL, 0x09B71D9B01191FEDULL, 0x14CFDB61E20D4FDCULL, 0x3DFA768C46B23134ULL, 0xE50AEFF14CB6DA44ULL
};

static const uint64_t IV_768[25] = {
    0xB0F2BB7C26FDCB63ULL, 0x41D198EC6D5B2B7BULL, 0x751BCA414F8B3A3DULL, 0x9B4D5ECA37E5DCA2ULL, 0xD5451D0D87B13E70ULL,
    0x29BDA85FB12825C0ULL, 0xEFAE039D229BB856ULL, 0x5533F9678259C50FULL, 0x57F530B2442DEC6CULL, 0xED048C2F7D498654ULL,
    0x0A7306A3F12442A3ULL, 0xCCE25A5F87D775C1ULL, 0x079D4E0253B563AAULL, 0xAC33774D15DBC6FDULL, 0x81EC4A8254BFD641ULL,
    0xC3B964F0008423A0ULL, 0x06B3661AA4B14E44ULL, 0xC47AD03E50312E31ULL, 0x6AD795F957D10241ULL, 0xEF8D172A503F8F62ULL,
    0xEB3CA46C3DD3FB80ULL, 0xECEE8411633651CDULL, 0xD7139EC73D704F63ULL, 0xE307189922FE9F8EULL, 0x73DD2748F22D7F1FULL
};

static const uint64_t IV_1024[25] = {
    0xA4F69C995CEFCD69ULL, 0x06991C0A3E9FB307ULL, 0x6A07F13502426450ULL, 0x60F0251F0F6A8899ULL, 0x47F583E911141969ULL,
    0xC5B7C6E311543B3FULL, 0x2C0515877EC5BDAFULL, 0x14A57976513CCA55ULL, 0xEA295865D33E9D7BULL, 0x3F17E94697F82F64ULL,
    0xF0D601977DB63BA5ULL, 0x6C2026D3858676EEULL, 0x7D0C3960480FD237ULL, 0x2D8ED3ED88F475D7ULL, 0x9332B33412A596F6ULL,
    0x9DF09750C9D639B4ULL, 0x52D73E916617792CULL, 0x6493DF5CE7DF5A21ULL, 0x20384DA1E6C497FEULL, 0x61C456C0442053A2ULL,
    0xE9CDFD19AAEC53A1ULL, 0xEAEA95C4CEA0E06EULL, 0x19F1B0C69940D2BEULL, 0x4F4A68E0F0FB52F1ULL, 0xA59BEAD309B75F6BULL
};

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
static void absorb_message_full_blocks(uint64_t A[25],
                                       const unsigned char *msg,
                                       unsigned long long full_blocks,
                                       unsigned rate_bits,
                                       unsigned rate_lanes,
                                       unsigned capacity_lanes)
{
    unsigned long long b;
    unsigned i;
    unsigned block_bytes = rate_bits >> 3;

    for (b = 0ULL; b < full_blocks; b++) {
        uint64_t block[AFS_TREDM_MAX_RATE_LANES];
        const unsigned char *p = msg + b * (unsigned long long)block_bytes;
        for (i = 0; i < rate_lanes; i++) {
            block[i] = load_be64(p + 8U * i);
        }
        absorb_prepared_block(A, block, rate_lanes, capacity_lanes);
    }
}

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
static void absorb_final_framed_blocks_byte_aligned(uint64_t A[25],
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
static void extract_digest(const uint64_t A[25],
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
    uint64_t A[25];
    unsigned rate_bits, capacity_bits, rate_lanes, capacity_lanes;
    unsigned long long full_blocks;

    if (digest == NULL) {
        return AFS_TREDM_NULL_POINTER;
    }
    if (msg == NULL && msg_len_bits != 0ULL) {
        return AFS_TREDM_NULL_POINTER;
    }

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

    rate_lanes = rate_bits >> 6;
    capacity_lanes = capacity_bits >> 6;
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
}
