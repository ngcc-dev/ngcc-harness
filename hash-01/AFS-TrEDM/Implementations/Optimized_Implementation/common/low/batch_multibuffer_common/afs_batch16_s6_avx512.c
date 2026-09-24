#include "CryptHash_AlgorithmInstance.h"
#include "afs_batch16_s6_avx512.h"
#include "afs_p1600.h"

#include <stdint.h>
#include <string.h>

#define AFS_BATCH16_AVX512_UNSUPPORTED 1

#if defined(__GNUC__) || defined(__clang__)
#define AFS_BATCH16_NOINLINE __attribute__((noinline))
#else
#define AFS_BATCH16_NOINLINE
#endif

#if AFS_TREDM_HAVE_BATCH16_AVX512_TRUE && \
    (DIGEST_BIT_LENGTH == 512 || DIGEST_BIT_LENGTH == 768 || DIGEST_BIT_LENGTH == 1024)

#if DIGEST_BIT_LENGTH == 512
#define AFS_BATCH16_DIGEST_BITS 512U
#define AFS_BATCH16_RATE_BITS 1024U
#define AFS_BATCH16_RATE_BYTES 128U
#define AFS_BATCH16_RATE_LANES 16U
#define AFS_BATCH16_CAPACITY_BITS 576U
#define AFS_BATCH16_CAPACITY_LANES 9U
#define AFS_BATCH16_DIGEST_LANES 8U
static const uint64_t AFS_BATCH16_IV[25] = {
    0x53994199EC2747FFULL, 0xBFD3CD9EBA914A21ULL, 0xE23244DDD302409BULL, 0xB82F2C0D67F11A56ULL, 0x2D61D2E164DC0C64ULL,
    0xF604258A090B4F87ULL, 0xE583B2DE7BCC0934ULL, 0x9C1960759D1FD367ULL, 0xCDE041B956651F19ULL, 0x32D52A1048F20B3FULL,
    0xDA10FA3BE09F1AEEULL, 0xC22C74696B68742DULL, 0x81A2E4EC00649D7FULL, 0x0057434538916AEAULL, 0x4FEA9520E57D8E11ULL,
    0x3121586C1336D4CBULL, 0xD652524E1FB3F916ULL, 0x27804CBD8F09D5F2ULL, 0x9F7DC67670C3DF84ULL, 0xEB9CDC94C6B7B81BULL,
    0x1388619624CA2963ULL, 0x09B71D9B01191FEDULL, 0x14CFDB61E20D4FDCULL, 0x3DFA768C46B23134ULL, 0xE50AEFF14CB6DA44ULL
};
#elif DIGEST_BIT_LENGTH == 768
#define AFS_BATCH16_DIGEST_BITS 768U
#define AFS_BATCH16_RATE_BITS 768U
#define AFS_BATCH16_RATE_BYTES 96U
#define AFS_BATCH16_RATE_LANES 12U
#define AFS_BATCH16_CAPACITY_BITS 832U
#define AFS_BATCH16_CAPACITY_LANES 13U
#define AFS_BATCH16_DIGEST_LANES 12U
static const uint64_t AFS_BATCH16_IV[25] = {
    0xB0F2BB7C26FDCB63ULL, 0x41D198EC6D5B2B7BULL, 0x751BCA414F8B3A3DULL, 0x9B4D5ECA37E5DCA2ULL, 0xD5451D0D87B13E70ULL,
    0x29BDA85FB12825C0ULL, 0xEFAE039D229BB856ULL, 0x5533F9678259C50FULL, 0x57F530B2442DEC6CULL, 0xED048C2F7D498654ULL,
    0x0A7306A3F12442A3ULL, 0xCCE25A5F87D775C1ULL, 0x079D4E0253B563AAULL, 0xAC33774D15DBC6FDULL, 0x81EC4A8254BFD641ULL,
    0xC3B964F0008423A0ULL, 0x06B3661AA4B14E44ULL, 0xC47AD03E50312E31ULL, 0x6AD795F957D10241ULL, 0xEF8D172A503F8F62ULL,
    0xEB3CA46C3DD3FB80ULL, 0xECEE8411633651CDULL, 0xD7139EC73D704F63ULL, 0xE307189922FE9F8EULL, 0x73DD2748F22D7F1FULL
};
#else
#define AFS_BATCH16_DIGEST_BITS 1024U
#define AFS_BATCH16_RATE_BITS 512U
#define AFS_BATCH16_RATE_BYTES 64U
#define AFS_BATCH16_RATE_LANES 8U
#define AFS_BATCH16_CAPACITY_BITS 1088U
#define AFS_BATCH16_CAPACITY_LANES 17U
#define AFS_BATCH16_DIGEST_LANES 16U
static const uint64_t AFS_BATCH16_IV[25] = {
    0xA4F69C995CEFCD69ULL, 0x06991C0A3E9FB307ULL, 0x6A07F13502426450ULL, 0x60F0251F0F6A8899ULL, 0x47F583E911141969ULL,
    0xC5B7C6E311543B3FULL, 0x2C0515877EC5BDAFULL, 0x14A57976513CCA55ULL, 0xEA295865D33E9D7BULL, 0x3F17E94697F82F64ULL,
    0xF0D601977DB63BA5ULL, 0x6C2026D3858676EEULL, 0x7D0C3960480FD237ULL, 0x2D8ED3ED88F475D7ULL, 0x9332B33412A596F6ULL,
    0x9DF09750C9D639B4ULL, 0x52D73E916617792CULL, 0x6493DF5CE7DF5A21ULL, 0x20384DA1E6C497FEULL, 0x61C456C0442053A2ULL,
    0xE9CDFD19AAEC53A1ULL, 0xEAEA95C4CEA0E06EULL, 0x19F1B0C69940D2BEULL, 0x4F4A68E0F0FB52F1ULL, 0xA59BEAD309B75F6BULL
};
#endif

#define AFS_BATCH16_SUFFIX_LANE ( \
    (((uint64_t)AFS_BATCH16_DIGEST_BITS) << 48) | \
    (((uint64_t)AFS_BATCH16_RATE_BITS) << 32) | \
    (((uint64_t)AFS_BATCH16_CAPACITY_BITS) << 16) | \
    ((uint64_t)2U) )

#include "generated/afs_batch16_rc.inc"
#include "generated/afs_s6_batch16_r0.inc"
#include "generated/afs_s6_batch16_r1.inc"
#include "generated/afs_s6_batch16_r2.inc"
#include "generated/afs_s6_batch16_r3.inc"
#include "generated/afs_s6_batch16_r4.inc"
#include "generated/afs_s6_batch16_r5.inc"

/* Function load_be64_batch16: loads sixteen big-endian 64-bit lanes for the AVX512 multi-buffer backend. */
static uint64_t load_be64_batch16(const uint8_t *p)
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

/* Function store_be64_batch16: stores sixteen big-endian 64-bit lanes for the AVX512 multi-buffer backend. */
static void store_be64_batch16(uint8_t *p, uint64_t x)
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

/* Function load_be64_partial_pad80_batch16: loads partial lanes and applies padding for sixteen batched messages. */
static uint64_t load_be64_partial_pad80_batch16(const uint8_t *p, unsigned n)
{
    uint64_t x = UINT64_C(0);
    unsigned i;

    for (i = 0U; i < n; i++) {
        x |= (uint64_t)p[i] << (56U - 8U * i);
    }
    x |= UINT64_C(0x80) << (56U - 8U * n);
    return x;
}

/* Function load_v64p_u64_lanes: loads the batch16 vector lane representation from scalar lane arrays. */
static v64p load_v64p_u64_lanes(const uint64_t words[16])
{
    uint32_t hi[16];
    uint32_t lo[16];
    unsigned i;
    v64p out;

    for (i = 0U; i < 16U; i++) {
        hi[i] = (uint32_t)(words[i] >> 32);
        lo[i] = (uint32_t)words[i];
    }
    out.hi = _mm512_loadu_si512((const void *)hi);
    out.lo = _mm512_loadu_si512((const void *)lo);
    return out;
}

/* Function store_v64p_u64_lanes: stores the batch16 vector lane representation to scalar lane arrays. */
static void store_v64p_u64_lanes(v64p v, uint64_t words[16])
{
    uint32_t hi[16];
    uint32_t lo[16];
    unsigned i;

    _mm512_storeu_si512((void *)hi, v.hi);
    _mm512_storeu_si512((void *)lo, v.lo);
    for (i = 0U; i < 16U; i++) {
        words[i] = ((uint64_t)hi[i] << 32) | (uint64_t)lo[i];
    }
}

/* Function afs64_t5_k2_batch16: computes the batch16 AVX512 AFS64_t5_k2 S-box over sixteen messages. */
static void afs64_t5_k2_batch16(v64p *lane, uint32_t rc)
{
    __m512i x = lane->hi;
    __m512i y = lane->lo;
    const __m512i c = _mm512_set1_epi32((int)rc);

    x = _mm512_add_epi32(x, vrot32(y, 17)); x = vxor32(x, c);
    y = _mm512_add_epi32(y, vrot32(x, 24)); y = vxor32(y, c);
    x = vxor32(x, vrot32(y, 1));
    y = vxor32(y, vrot32(x, 1));
    x = vxor32(x, vrot32(y, 16));
    y = vxor32(y, vrot32(x, 31));
    x = _mm512_add_epi32(x, vrot32(y, 24)); x = vxor32(x, c);
    y = _mm512_add_epi32(y, x); y = vxor32(y, c);

    lane->hi = x;
    lane->lo = y;
}

/* Function afs_batch16_s6_round: applies one batch16 AVX512 AFS-p-S6 round. */
static void afs_batch16_s6_round(v64p A[25], unsigned round)
{
    unsigned i;
    const uint32_t *rc = AFS_BATCH16_RC[round];

    for (i = 0U; i < 25U; i++) {
        afs64_t5_k2_batch16(&A[i], rc[i]);
    }

    switch (round % 6U) {
    case 0U:
        afs_s6_batch16_round_r0(A);
        break;
    case 1U:
        afs_s6_batch16_round_r1(A);
        break;
    case 2U:
        afs_s6_batch16_round_r2(A);
        break;
    case 3U:
        afs_s6_batch16_round_r3(A);
        break;
    case 4U:
        afs_s6_batch16_round_r4(A);
        break;
    default:
        afs_s6_batch16_round_r5(A);
        break;
    }
}

/* Function afs_batch16_g: applies the batch16 front half g of the split permutation. */
static void afs_batch16_g(v64p A[25])
{
    unsigned r;
    for (r = 0U; r < AFS_P1600_SPLIT_ROUNDS; r++) {
        afs_batch16_s6_round(A, r);
    }
}

/* Function afs_batch16_h: applies the batch16 back half h of the split permutation. */
static void afs_batch16_h(v64p A[25])
{
    unsigned r;
    for (r = AFS_P1600_SPLIT_ROUNDS; r < AFS_P1600_EFFECTIVE_ROUNDS; r++) {
        afs_batch16_s6_round(A, r);
    }
}

/* Function absorb_prepared_batch16: absorbs prepared rate blocks for the batch16 AVX512 backend. */
static void absorb_prepared_batch16(v64p A[25])
{
    v64p saved[AFS_BATCH16_CAPACITY_LANES];
    unsigned i;

    for (i = 0U; i < AFS_BATCH16_CAPACITY_LANES; i++) {
        saved[i] = A[AFS_BATCH16_RATE_LANES + i];
    }
    afs_batch16_g(A);
    for (i = 0U; i < AFS_BATCH16_CAPACITY_LANES; i++) {
        A[AFS_BATCH16_RATE_LANES + i] =
            v64p_xor(A[AFS_BATCH16_RATE_LANES + i], saved[i]);
    }
    afs_batch16_h(A);
}

/* Function absorb_full_block_batch16: absorbs full rate blocks for the batch16 AVX512 backend. */
static void absorb_full_block_batch16(v64p A[25],
                                      const uint8_t * const msg[16],
                                      uint64_t block_index)
{
    unsigned rate_lane;
    for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
        uint64_t words[16];
        unsigned lane;
        const uint64_t off = block_index * AFS_BATCH16_RATE_BYTES + (uint64_t)rate_lane * 8U;
        for (lane = 0U; lane < 16U; lane++) {
            words[lane] = load_be64_batch16(msg[lane] + off);
        }
        A[rate_lane] = v64p_xor(A[rate_lane], load_v64p_u64_lanes(words));
    }
    absorb_prepared_batch16(A);
}

/* Function absorb_final_zero_rem_batch16: absorbs the final zero-tail frame for the batch16 AVX512 backend. */
static void absorb_final_zero_rem_batch16(v64p A[25], uint64_t msg_len_bits)
{
    A[0] = v64p_xor(A[0], v64p_set1_u64(UINT64_C(0x8000000000000000)));
    A[AFS_BATCH16_RATE_LANES - 2U] =
        v64p_xor(A[AFS_BATCH16_RATE_LANES - 2U], v64p_set1_u64(msg_len_bits));
    A[AFS_BATCH16_RATE_LANES - 1U] =
        v64p_xor(A[AFS_BATCH16_RATE_LANES - 1U], v64p_set1_u64(AFS_BATCH16_SUFFIX_LANE));
    absorb_prepared_batch16(A);
}

/* Function absorb_final_framed_blocks_byte_aligned_batch16: handles byte-aligned final framing for batch16 messages. */
static void AFS_BATCH16_NOINLINE absorb_final_framed_blocks_byte_aligned_batch16(
    v64p A[25],
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint64_t full_blocks)
{
    const unsigned rem_bytes =
        (unsigned)((msg_len_bits % (uint64_t)AFS_BATCH16_RATE_BITS) >> 3U);
    const unsigned total_no_pad = (rem_bytes << 3U) + 1U + 128U;
    const unsigned zero_pad =
        (AFS_BATCH16_RATE_BITS - (total_no_pad % AFS_BATCH16_RATE_BITS)) %
        AFS_BATCH16_RATE_BITS;
    const unsigned final_blocks = (total_no_pad + zero_pad) / AFS_BATCH16_RATE_BITS;
    const unsigned full_lanes = rem_bytes >> 3U;
    const unsigned tail_bytes = rem_bytes & 7U;
    v64p block[AFS_BATCH16_RATE_LANES];
    unsigned rate_lane;

    for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
        uint64_t words[16];
        unsigned lane;
        for (lane = 0U; lane < 16U; lane++) {
            const uint8_t *src =
                msg[lane] + full_blocks * (uint64_t)AFS_BATCH16_RATE_BYTES;
            if (rate_lane < full_lanes) {
                words[lane] = load_be64_batch16(src + (uint64_t)rate_lane * 8U);
            } else if (rate_lane == full_lanes) {
                words[lane] = load_be64_partial_pad80_batch16(
                    src + (uint64_t)rate_lane * 8U, tail_bytes);
            } else {
                words[lane] = UINT64_C(0);
            }
        }
        block[rate_lane] = load_v64p_u64_lanes(words);
    }

    if (final_blocks == 1U) {
        block[AFS_BATCH16_RATE_LANES - 2U] =
            v64p_set1_u64((uint64_t)msg_len_bits);
        block[AFS_BATCH16_RATE_LANES - 1U] =
            v64p_set1_u64(AFS_BATCH16_SUFFIX_LANE);
        for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
            A[rate_lane] = v64p_xor(A[rate_lane], block[rate_lane]);
        }
        absorb_prepared_batch16(A);
    } else {
        for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
            A[rate_lane] = v64p_xor(A[rate_lane], block[rate_lane]);
        }
        absorb_prepared_batch16(A);

        for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
            block[rate_lane] = v64p_zero();
        }
        block[AFS_BATCH16_RATE_LANES - 2U] =
            v64p_set1_u64((uint64_t)msg_len_bits);
        block[AFS_BATCH16_RATE_LANES - 1U] =
            v64p_set1_u64(AFS_BATCH16_SUFFIX_LANE);
        for (rate_lane = 0U; rate_lane < AFS_BATCH16_RATE_LANES; rate_lane++) {
            A[rate_lane] = v64p_xor(A[rate_lane], block[rate_lane]);
        }
        absorb_prepared_batch16(A);
    }
}

/* Function afs_tredm_hash_batch16_rate_multiple_validated: hashes rate-aligned batch16 messages after parameter validation. */
static int afs_tredm_hash_batch16_rate_multiple_validated(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16])
{
    v64p A[25];
    uint64_t full_blocks;
    uint64_t block;
    unsigned i;

    for (i = 0U; i < 25U; i++) {
        A[i] = v64p_set1_u64(AFS_BATCH16_IV[i]);
    }

    full_blocks = msg_len_bits / AFS_BATCH16_RATE_BITS;
    for (block = 0ULL; block < full_blocks; block++) {
        absorb_full_block_batch16(A, msg, block);
    }
    absorb_final_zero_rem_batch16(A, msg_len_bits);

    for (i = 0U; i < AFS_BATCH16_DIGEST_LANES; i++) {
        uint64_t words[16];
        unsigned lane;
        store_v64p_u64_lanes(A[AFS_BATCH16_RATE_LANES + i], words);
        for (lane = 0U; lane < 16U; lane++) {
            store_be64_batch16(digest[lane] + 8U * i, words[lane]);
        }
    }
    return 0;
}

/* Function afs_tredm_hash_batch16_avx512_same_len: hashes sixteen same-length messages using the AVX512 multi-buffer backend. */
int afs_tredm_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16])
{
    v64p A[25];
    uint64_t full_blocks;
    uint64_t block;
    unsigned i;

    if ((msg_len_bits & 7ULL) != 0ULL) {
        return AFS_BATCH16_AVX512_UNSUPPORTED;
    }
    for (i = 0U; i < 16U; i++) {
        if (digest[i] == 0 || (msg_len_bits != 0ULL && msg[i] == 0)) {
            return -1;
        }
    }

    if ((msg_len_bits % AFS_BATCH16_RATE_BITS) == 0ULL) {
        return afs_tredm_hash_batch16_rate_multiple_validated(
            msg, msg_len_bits, digest);
    }

    for (i = 0U; i < 25U; i++) {
        A[i] = v64p_set1_u64(AFS_BATCH16_IV[i]);
    }

    full_blocks = msg_len_bits / AFS_BATCH16_RATE_BITS;
    for (block = 0ULL; block < full_blocks; block++) {
        absorb_full_block_batch16(A, msg, block);
    }
    absorb_final_framed_blocks_byte_aligned_batch16(A, msg, msg_len_bits, full_blocks);

    for (i = 0U; i < AFS_BATCH16_DIGEST_LANES; i++) {
        uint64_t words[16];
        unsigned lane;
        store_v64p_u64_lanes(A[AFS_BATCH16_RATE_LANES + i], words);
        for (lane = 0U; lane < 16U; lane++) {
            store_be64_batch16(digest[lane] + 8U * i, words[lane]);
        }
    }
    return 0;
}

/* Function afs_tredm512_hash_batch16_avx512_same_len: hashes sixteen same-length AFS-TrEDM-512 messages using AVX512. */
int afs_tredm512_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16])
{
    return afs_tredm_hash_batch16_avx512_same_len(msg, msg_len_bits, digest);
}

/* Function rotr64_scalar_batch16: rotates a 64-bit scalar word for batch16 support code. */
static uint64_t rotr64_scalar_batch16(uint64_t x, unsigned r)
{
    r &= 63U;
    return (uint64_t)((x >> r) | (x << ((64U - r) & 63U)));
}

/* Function rotr32_scalar_batch16: rotates a 32-bit scalar word for batch16 support code. */
static uint32_t rotr32_scalar_batch16(uint32_t x, unsigned r)
{
    r &= 31U;
    return (uint32_t)((x >> r) | (x << ((32U - r) & 31U)));
}

/* Function afs64_scalar_batch16: computes the scalar AFS64_t5_k2 operation for batch16 fallback/self-test use. */
static uint64_t afs64_scalar_batch16(uint64_t in, uint32_t c)
{
    uint32_t x = (uint32_t)(in >> 32);
    uint32_t y = (uint32_t)in;

    x = (uint32_t)(x + rotr32_scalar_batch16(y, 17)); x ^= c;
    y = (uint32_t)(y + rotr32_scalar_batch16(x, 24)); y ^= c;
    x ^= rotr32_scalar_batch16(y, 1);
    y ^= rotr32_scalar_batch16(x, 1);
    x ^= rotr32_scalar_batch16(y, 16);
    y ^= rotr32_scalar_batch16(x, 31);
    x = (uint32_t)(x + rotr32_scalar_batch16(y, 24)); x ^= c;
    y = (uint32_t)(y + x); y ^= c;
    return ((uint64_t)x << 32) | (uint64_t)y;
}

/* Function afs_batch16_avx512_selftest_primitives: implements the indicated AFS-TrEDM helper routine. */
int afs_batch16_avx512_selftest_primitives(void)
{
    uint64_t input[16];
    uint64_t output[16];
    unsigned i;
    unsigned r;
    v64p v;

    for (i = 0U; i < 16U; i++) {
        input[i] = UINT64_C(0x9E3779B97F4A7C15) * (uint64_t)(i + 1U) ^
                   (UINT64_C(0xD1B54A32D192ED03) + (uint64_t)i);
    }
    v = load_v64p_u64_lanes(input);
    for (r = 0U; r < 64U; r++) {
        store_v64p_u64_lanes(v64p_rotr64(v, r), output);
        for (i = 0U; i < 16U; i++) {
            if (output[i] != rotr64_scalar_batch16(input[i], r)) {
                return -1;
            }
        }
    }

    v = load_v64p_u64_lanes(input);
    afs64_t5_k2_batch16(&v, UINT32_C(0xA5C39E17));
    store_v64p_u64_lanes(v, output);
    for (i = 0U; i < 16U; i++) {
        if (output[i] != afs64_scalar_batch16(input[i], UINT32_C(0xA5C39E17))) {
            return -2;
        }
    }
    return 0;
}

#else

/* Function afs_tredm_hash_batch16_avx512_same_len: hashes sixteen same-length messages using the AVX512 multi-buffer backend. */
int afs_tredm_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16])
{
    (void)msg;
    (void)msg_len_bits;
    (void)digest;
    return AFS_BATCH16_AVX512_UNSUPPORTED;
}

/* Function afs_tredm512_hash_batch16_avx512_same_len: hashes sixteen same-length AFS-TrEDM-512 messages using AVX512. */
int afs_tredm512_hash_batch16_avx512_same_len(
    const uint8_t * const msg[16],
    uint64_t msg_len_bits,
    uint8_t * const digest[16])
{
    return afs_tredm_hash_batch16_avx512_same_len(msg, msg_len_bits, digest);
}

/* Function afs_batch16_avx512_selftest_primitives: implements the indicated AFS-TrEDM helper routine. */
int afs_batch16_avx512_selftest_primitives(void)
{
    return AFS_BATCH16_AVX512_UNSUPPORTED;
}

#endif
