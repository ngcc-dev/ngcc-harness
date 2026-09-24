/**
 * @file polarkem_ct.c
 * @brief Ciphertext codec for the portable optimized PolarKEM-512 profile.
 */
#include "polarkem_ct.h"

#include <stddef.h>
#include <string.h>

#include "auxfunc.h"
#include "polarkem_pack.h"
#include "polarkem_polar.h"

static const unsigned char polarkem_magic[8] = {
    (unsigned char)'P', (unsigned char)'O', (unsigned char)'L',
    (unsigned char)'K', (unsigned char)'E', (unsigned char)'M',
    (unsigned char)'1', 0x00U
};

static const unsigned char domain_perm[] = "PolarKEM-PERM-v1";
static const unsigned char domain_sign[] = "PolarKEM-SIGN-v1";
static const unsigned char domain_error[] = "PolarKEM-ERR-v1";
static const unsigned char domain_tag[] = "PolarKEM-TAG-v1";
static const unsigned char domain_pad[] = "PolarKEM-PAD-v1";

/** State for the canonical concatenated 4096-byte permutation word stream. */
typedef struct polarkem_perm_stream_s {
    unsigned char block[POLARKEM_PERM_BLOCK_BYTES];
    unsigned char input[(sizeof(domain_perm) - 1U) + POLARKEM_SEED_BYTES + 4U];
    uint32_t block_index;
    size_t offset;
} polarkem_perm_stream;

/**
 * Store a 32-bit integer in the profile's little-endian encoding.
 *
 * @param[out] out Four-byte destination.
 * @param value Value to serialize.
 * @return Nothing; all four destination bytes are written.
 */
static void store32_le(unsigned char out[4], uint32_t value)
{
    out[0] = (unsigned char)value;
    out[1] = (unsigned char)(value >> 8U);
    out[2] = (unsigned char)(value >> 16U);
    out[3] = (unsigned char)(value >> 24U);
}

/**
 * Load a 32-bit integer from the profile's little-endian encoding.
 *
 * @param[in] in Four readable encoded bytes.
 * @return The decoded unsigned 32-bit integer.
 */
static uint32_t load32_le(const unsigned char in[4])
{
    return (uint32_t)in[0]
           | ((uint32_t)in[1] << 8U)
           | ((uint32_t)in[2] << 16U)
           | ((uint32_t)in[3] << 24U);
}

/**
 * Initialize a permutation stream for one public transform seed.
 *
 * @param[out] stream Stream state whose first read will expand block zero.
 * @param[in] seed Exactly POLARKEM_SEED_BYTES transform-seed bytes.
 * @return Nothing; the complete state is initialized.
 */
static void perm_stream_init(polarkem_perm_stream *stream,
                             const unsigned char seed[POLARKEM_SEED_BYTES])
{
    const size_t domain_bytes = sizeof(domain_perm) - 1U;

    memset(stream, 0, sizeof(*stream));
    memcpy(stream->input, domain_perm, domain_bytes);
    memcpy(stream->input + domain_bytes, seed, POLARKEM_SEED_BYTES);
    stream->offset = POLARKEM_PERM_BLOCK_BYTES;
}

/**
 * Return the next little-endian word from the canonical block-XOF stream.
 *
 * @param[in,out] stream Initialized stream state and current byte offset.
 * @param[out] word Next decoded 32-bit word.
 * @return 0 on success or -4 when the official XOF fails.
 */
static int perm_stream_next(polarkem_perm_stream *stream, uint32_t *word)
{
    if (stream->offset == POLARKEM_PERM_BLOCK_BYTES) {
        const size_t counter_offset = (sizeof(domain_perm) - 1U)
                                      + POLARKEM_SEED_BYTES;
        store32_le(stream->input + counter_offset, stream->block_index);
        if (pseudoXOF((unsigned long long)POLARKEM_PERM_BLOCK_BYTES * 8ULL,
                      stream->input,
                      (unsigned long long)sizeof(stream->input) * 8ULL,
                      stream->block) != 0) {
            return -4;
        }
        stream->block_index += 1U;
        stream->offset = 0U;
    }

    *word = load32_le(stream->block + stream->offset);
    stream->offset += 4U;
    return 0;
}

void polarkem_write_header(unsigned char object[POLARKEM_HEADER_BYTES])
{
    memset(object, 0, POLARKEM_HEADER_BYTES);
    memcpy(object, polarkem_magic, sizeof(polarkem_magic));
    object[8] = (unsigned char)POLARKEM_INSTANCE_ID;
}

int polarkem_header_is_valid(
    const unsigned char object[POLARKEM_HEADER_BYTES])
{
    unsigned int difference = 0U;
    size_t i;

    for (i = 0U; i < sizeof(polarkem_magic); ++i) {
        difference |= (unsigned int)(object[i] ^ polarkem_magic[i]);
    }
    difference |= (unsigned int)(object[8] ^ (unsigned char)POLARKEM_INSTANCE_ID);
    for (i = 9U; i < POLARKEM_HEADER_BYTES; ++i) {
        difference |= (unsigned int)object[i];
    }
    return difference == 0U ? 1 : 0;
}

int polarkem_hash_public_key(
    unsigned char digest[POLARKEM_HASH_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES])
{
    if (sm3hash(256, pk, (unsigned long long)POLARKEM_PK_BYTES * 8ULL,
                digest) != 0) {
        return -4;
    }
    return 0;
}

int polarkem_signed_permutation(
    uint16_t permutation[POLARKEM_N],
    int8_t signs[POLARKEM_N],
    const unsigned char seed[POLARKEM_SEED_BYTES])
{
    polarkem_perm_stream stream;
    unsigned char sign_bits[POLARKEM_N / 8U];
    unsigned char sign_input[(sizeof(domain_sign) - 1U) + POLARKEM_SEED_BYTES];
    size_t i;

    for (i = 0U; i < POLARKEM_N; ++i) {
        permutation[i] = (uint16_t)i;
    }
    perm_stream_init(&stream, seed);
    for (i = POLARKEM_N - 1U; i > 0U; --i) {
        const uint64_t bound = (uint64_t)i + UINT64_C(1);
        const uint64_t limit = (UINT64_C(0x100000000) / bound) * bound;
        uint32_t random_word;
        size_t j;
        uint16_t temporary;

        do {
            if (perm_stream_next(&stream, &random_word) != 0) {
                memset(&stream, 0, sizeof(stream));
                return -4;
            }
        } while ((uint64_t)random_word >= limit);
        j = (size_t)((uint64_t)random_word % bound);
        temporary = permutation[i];
        permutation[i] = permutation[j];
        permutation[j] = temporary;
    }

    memcpy(sign_input, domain_sign, sizeof(domain_sign) - 1U);
    memcpy(sign_input + sizeof(domain_sign) - 1U, seed, POLARKEM_SEED_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_N,
                  sign_input, (unsigned long long)sizeof(sign_input) * 8ULL,
                  sign_bits) != 0) {
        memset(&stream, 0, sizeof(stream));
        return -4;
    }
    for (i = 0U; i < POLARKEM_N; ++i) {
        const unsigned int sign_bit =
            (unsigned int)((sign_bits[i >> 3U] >> (i & 7U)) & 1U);
        signs[i] = sign_bit == 0U ? (int8_t)1 : (int8_t)-1;
    }

    memset(&stream, 0, sizeof(stream));
    memset(sign_bits, 0, sizeof(sign_bits));
    return 0;
}

/**
 * Build a deterministic ciphertext using an already-expanded permutation.
 *
 * @param[out] ct Exactly POLARKEM_CT_BYTES initialized ciphertext bytes.
 * @param[in] pk Exactly POLARKEM_PK_BYTES serialized public-key bytes.
 * @param[in] mu Exactly POLARKEM_MU_BYTES message bytes.
 * @param[in] permutation Destination-indexed source indices.
 * @param[in] signs Destination-indexed values, each +1 or -1.
 * @return 0 on success or -4 when an official helper fails.
 */
static int build_ciphertext_with_permutation(
    unsigned char ct[POLARKEM_CT_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char mu[POLARKEM_MU_BYTES],
    const uint16_t permutation[POLARKEM_N],
    const int8_t signs[POLARKEM_N])
{
    uint64_t codeword[POLARKEM_WORDS];
    uint16_t coefficients[POLARKEM_N];
    unsigned char pk_hash[POLARKEM_HASH_BYTES];
    unsigned char error_bits[(POLARKEM_N * 2U) / 8U];
    unsigned char error_input[(sizeof(domain_error) - 1U)
                              + POLARKEM_MU_BYTES + POLARKEM_HASH_BYTES];
    unsigned char tag_input[(sizeof(domain_tag) - 1U)
                            + POLARKEM_MU_BYTES + POLARKEM_HASH_BYTES
                            + POLARKEM_CT_PAYLOAD_BYTES];
    unsigned char pad_input[(sizeof(domain_pad) - 1U)
                            + POLARKEM_MU_BYTES + POLARKEM_TAG_BYTES];
    size_t offset;
    size_t i;
    int result = -4;

    memset(ct, 0, POLARKEM_CT_BYTES);
    if (polarkem_hash_public_key(pk_hash, pk) != 0) {
        goto cleanup;
    }
    offset = 0U;
    memcpy(error_input + offset, domain_error, sizeof(domain_error) - 1U);
    offset += sizeof(domain_error) - 1U;
    memcpy(error_input + offset, mu, POLARKEM_MU_BYTES);
    offset += POLARKEM_MU_BYTES;
    memcpy(error_input + offset, pk_hash, POLARKEM_HASH_BYTES);
    if (pseudoXOF((unsigned long long)sizeof(error_bits) * 8ULL,
                  error_input, (unsigned long long)sizeof(error_input) * 8ULL,
                  error_bits) != 0) {
        goto cleanup;
    }

    polarkem_polar_encode(codeword, mu);
    for (i = 0U; i < POLARKEM_N; ++i) {
        static const int8_t error_map[4] = { 0, 1, -1, 0 };
        const uint16_t source = permutation[i];
        const unsigned int bit = (unsigned int)(
            (codeword[source >> 6U] >> (source & 63U)) & UINT64_C(1));
        const unsigned int error_code = (unsigned int)(
            (error_bits[i >> 2U] >> (2U * (i & 3U))) & 3U);
        int32_t value = bit == 0U ? (int32_t)POLARKEM_PEAK
                                  : -(int32_t)POLARKEM_PEAK;

        value *= (int32_t)signs[i];
        value += (int32_t)error_map[error_code];
        if (value < 0) {
            value += (int32_t)POLARKEM_Q;
        }
        coefficients[i] = (uint16_t)value;
    }

    polarkem_write_header(ct);
    polarkem_pack_coefficients(ct + POLARKEM_CT_PAYLOAD_OFFSET, coefficients);

    offset = 0U;
    memcpy(tag_input + offset, domain_tag, sizeof(domain_tag) - 1U);
    offset += sizeof(domain_tag) - 1U;
    memcpy(tag_input + offset, mu, POLARKEM_MU_BYTES);
    offset += POLARKEM_MU_BYTES;
    memcpy(tag_input + offset, pk_hash, POLARKEM_HASH_BYTES);
    offset += POLARKEM_HASH_BYTES;
    memcpy(tag_input + offset, ct + POLARKEM_CT_PAYLOAD_OFFSET,
           POLARKEM_CT_PAYLOAD_BYTES);
    if (sm3hash(256, tag_input,
                (unsigned long long)sizeof(tag_input) * 8ULL,
                ct + POLARKEM_CT_TAG_OFFSET) != 0) {
        goto cleanup;
    }

    offset = 0U;
    memcpy(pad_input + offset, domain_pad, sizeof(domain_pad) - 1U);
    offset += sizeof(domain_pad) - 1U;
    memcpy(pad_input + offset, mu, POLARKEM_MU_BYTES);
    offset += POLARKEM_MU_BYTES;
    memcpy(pad_input + offset, ct + POLARKEM_CT_TAG_OFFSET,
           POLARKEM_TAG_BYTES);
    if (pseudoXOF((unsigned long long)POLARKEM_CT_PAD_BYTES * 8ULL,
                  pad_input, (unsigned long long)sizeof(pad_input) * 8ULL,
                  ct + POLARKEM_CT_PAD_OFFSET) != 0) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (result != 0) {
        memset(ct, 0, POLARKEM_CT_BYTES);
    }
    memset(codeword, 0, sizeof(codeword));
    memset(coefficients, 0, sizeof(coefficients));
    memset(error_bits, 0, sizeof(error_bits));
    memset(error_input, 0, sizeof(error_input));
    memset(tag_input, 0, sizeof(tag_input));
    memset(pad_input, 0, sizeof(pad_input));
    return result;
}

/**
 * Recover a message using an already-expanded inverse signed permutation.
 *
 * @param[out] mu Exactly POLARKEM_MU_BYTES recovered candidate bytes.
 * @param[in] ct Exactly POLARKEM_CT_BYTES received ciphertext bytes.
 * @param[in] permutation Destination-indexed source indices.
 * @param[in] signs Destination-indexed values, each +1 or -1.
 * @return Nothing; every message byte is initialized.
 */
static void recover_message_with_permutation(
    unsigned char mu[POLARKEM_MU_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES],
    const uint16_t permutation[POLARKEM_N],
    const int8_t signs[POLARKEM_N])
{
    uint16_t coefficients[POLARKEM_N];
    uint64_t codeword[POLARKEM_WORDS];
    size_t i;

    memset(mu, 0, POLARKEM_MU_BYTES);
    memset(codeword, 0, sizeof(codeword));
    polarkem_unpack_coefficients(coefficients,
                                 ct + POLARKEM_CT_PAYLOAD_OFFSET);
    for (i = 0U; i < POLARKEM_N; ++i) {
        int32_t centered = (int32_t)coefficients[i];
        const uint16_t destination = permutation[i];

        if (centered > (int32_t)(POLARKEM_Q / 2U)) {
            centered -= (int32_t)POLARKEM_Q;
        }
        centered *= (int32_t)signs[i];
        codeword[destination >> 6U] |=
            (uint64_t)(centered < 0 ? 1U : 0U) << (destination & 63U);
    }
    polarkem_polar_decode(mu, codeword);

    memset(coefficients, 0, sizeof(coefficients));
    memset(codeword, 0, sizeof(codeword));
}

int polarkem_build_ciphertext(
    unsigned char ct[POLARKEM_CT_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char mu[POLARKEM_MU_BYTES])
{
    uint16_t permutation[POLARKEM_N];
    int8_t signs[POLARKEM_N];

    if (polarkem_signed_permutation(permutation, signs,
                                    pk + POLARKEM_PK_SEED_OFFSET) != 0) {
        memset(ct, 0, POLARKEM_CT_BYTES);
        return -4;
    }
    return build_ciphertext_with_permutation(ct, pk, mu, permutation, signs);
}

int polarkem_recover_message(
    unsigned char mu[POLARKEM_MU_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char ct[POLARKEM_CT_BYTES])
{
    uint16_t permutation[POLARKEM_N];
    int8_t signs[POLARKEM_N];

    if (polarkem_signed_permutation(permutation, signs,
                                    pk + POLARKEM_PK_SEED_OFFSET) != 0) {
        memset(mu, 0, POLARKEM_MU_BYTES);
        return -4;
    }
    recover_message_with_permutation(mu, ct, permutation, signs);
    return 0;
}

int polarkem_recover_and_reencrypt(
    unsigned char mu[POLARKEM_MU_BYTES],
    unsigned char rebuilt[POLARKEM_CT_BYTES],
    const unsigned char pk[POLARKEM_PK_BYTES],
    const unsigned char received[POLARKEM_CT_BYTES])
{
    uint16_t permutation[POLARKEM_N];
    int8_t signs[POLARKEM_N];

    if (polarkem_signed_permutation(permutation, signs,
                                    pk + POLARKEM_PK_SEED_OFFSET) != 0) {
        memset(mu, 0, POLARKEM_MU_BYTES);
        memset(rebuilt, 0, POLARKEM_CT_BYTES);
        return -4;
    }
    recover_message_with_permutation(mu, received, permutation, signs);
    return build_ciphertext_with_permutation(rebuilt, pk, mu,
                                             permutation, signs);
}

int polarkem_constant_time_equal(const unsigned char *a,
                                 const unsigned char *b,
                                 size_t length)
{
    unsigned int difference = 0U;
    size_t i;

    for (i = 0U; i < length; ++i) {
        difference |= (unsigned int)(a[i] ^ b[i]);
    }
    return difference == 0U ? 1 : 0;
}

