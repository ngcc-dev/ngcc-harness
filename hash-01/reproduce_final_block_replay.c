#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Include the submitted mode in this translation unit so the witness can
 * inspect the state at the public g/h boundary without modifying it.
 */
#include "afs_tredm.c"

static void state_to_rate(unsigned char *out, const uint64_t A[25],
                          unsigned rate_lanes)
{
    unsigned i;
    for (i = 0; i < rate_lanes; i++)
        store_be64(out + 8U * i, A[i]);
}

static void bytes_to_block(uint64_t block[AFS_TREDM_MAX_RATE_LANES],
                           const unsigned char *in, unsigned rate_lanes)
{
    unsigned i;
    memset(block, 0, AFS_TREDM_MAX_RATE_LANES * sizeof(*block));
    for (i = 0; i < rate_lanes; i++)
        block[i] = load_be64(in + 8U * i);
}

static void make_frame(unsigned char *frame, unsigned rate_bytes,
                       const unsigned char *msg, unsigned msg_bytes,
                       uint64_t msg_bits, unsigned digest_bits,
                       unsigned capacity_bits)
{
    memset(frame, 0, rate_bytes);
    if (msg_bytes)
        memcpy(frame, msg, msg_bytes);
    frame[msg_bytes] = 0x80;
    store_be64(frame + rate_bytes - 16U, msg_bits);
    store_be16(frame + rate_bytes - 8U, digest_bits);
    store_be16(frame + rate_bytes - 6U, rate_bytes * 8U);
    store_be16(frame + rate_bytes - 4U, capacity_bits);
    store_be16(frame + rate_bytes - 2U, AFS_TREDM_VERSION);
}

int main(void)
{
    const unsigned digest_bits = DIGEST_BIT_LENGTH;
    const unsigned capacity_bits = digest_bits + 64U;
    const unsigned rate_bits = 1600U - capacity_bits;
    const unsigned rate_lanes = rate_bits / 64U;
    const unsigned capacity_lanes = capacity_bits / 64U;
    const unsigned rate_bytes = rate_bits / 8U;
    const unsigned digest_bytes = digest_bits / 8U;
    const unsigned char message[1] = {0x41};
    unsigned char *encoded = calloc(rate_bytes, 1U);
    unsigned char *extended = calloc(2U * rate_bytes, 1U);
    unsigned char *control = calloc(2U * rate_bytes, 1U);
    unsigned char *rate = calloc(rate_bytes, 1U);
    unsigned char *padding = calloc(rate_bytes, 1U);
    unsigned char *digest = calloc(digest_bytes, 1U);
    unsigned char *candidate = calloc(digest_bytes, 1U);
    unsigned char *control_digest = calloc(digest_bytes, 1U);
    uint64_t A[25], cancelled[25], capacity_only[25];
    uint64_t block[AFS_TREDM_MAX_RATE_LANES];
    uint64_t zero[AFS_TREDM_MAX_RATE_LANES] = {0};

    if (!encoded || !extended || !control || !rate || !padding ||
        !digest || !candidate || !control_digest)
        return 2;

    if (digest_bits == 512U)
        memcpy(A, IV_512, sizeof(A));
    else if (digest_bits == 768U)
        memcpy(A, IV_768, sizeof(A));
    else if (digest_bits == 1024U)
        memcpy(A, IV_1024, sizeof(A));
    else
        return 2;

    /* P(M) is exactly one rate block for this one-byte message. */
    make_frame(encoded, rate_bytes, message, sizeof(message), 8U,
               digest_bits, capacity_bits);
    bytes_to_block(block, encoded, rate_lanes);
    absorb_prepared_block(A, block, rate_lanes, capacity_lanes);

    if (afs_tredm_hash((int)digest_bits, message, 8U, digest) != 0)
        return 2;
    extract_digest(A, candidate, digest_bits, rate_lanes);
    if (memcmp(digest, candidate, digest_bytes) != 0) {
        fprintf(stderr, "internal framing does not match CryptHash\n");
        return 1;
    }

    /* M' = P(M) || R, where R is the exposed rate of the final state. */
    state_to_rate(rate, A, rate_lanes);
    memcpy(extended, encoded, rate_bytes);
    memcpy(extended + rate_bytes, rate, rate_bytes);

    memcpy(cancelled, A, sizeof(A));
    bytes_to_block(block, rate, rate_lanes);
    absorb_prepared_block(cancelled, block, rate_lanes, capacity_lanes);

    /* The same state follows from 0^r || capacity and a zero rate block. */
    memcpy(capacity_only, A, sizeof(A));
    memset(capacity_only, 0, rate_lanes * sizeof(*capacity_only));
    absorb_prepared_block(capacity_only, zero, rate_lanes, capacity_lanes);
    if (memcmp(cancelled, capacity_only, sizeof(cancelled)) != 0) {
        fprintf(stderr, "rate cancellation failed\n");
        return 1;
    }

    /* Hash framing adds one fresh block after the two data blocks in M'. */
    make_frame(padding, rate_bytes, NULL, 0U, 2ULL * rate_bits,
               digest_bits, capacity_bits);
    bytes_to_block(block, padding, rate_lanes);
    absorb_prepared_block(cancelled, block, rate_lanes, capacity_lanes);
    extract_digest(cancelled, candidate, digest_bits, rate_lanes);

    if (afs_tredm_hash((int)digest_bits, extended, 2ULL * rate_bits,
                       digest) != 0 ||
        memcmp(digest, candidate, digest_bytes) != 0) {
        fprintf(stderr, "extended-message candidate did not match\n");
        return 1;
    }

    memcpy(control, extended, 2U * rate_bytes);
    control[rate_bytes] ^= 0x80U;
    if (afs_tredm_hash((int)digest_bits, control, 2ULL * rate_bits,
                       control_digest) != 0)
        return 2;
    if (memcmp(control_digest, candidate, digest_bytes) == 0) {
        fprintf(stderr, "changed-rate control unexpectedly matched\n");
        return 1;
    }

    printf("ATTACK final-block-replay AFS-TrEDM-%u CONFIRMED "
           "hidden_capacity_bits=64 control=REJECTED\n",
           digest_bits);
    return 0;
}
