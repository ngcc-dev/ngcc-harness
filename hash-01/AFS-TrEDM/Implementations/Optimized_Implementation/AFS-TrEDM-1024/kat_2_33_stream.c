/*
 * Streaming KAT_2_33 generator for AFS-TrEDM.
 *
 * This helper avoids allocating the 2^33-bit test message.  It is not part of
 * the CryptHash API; it is used only to regenerate long known-answer vectors
 * from the same internal implementation and the ICCS DRNG.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "CryptHash_AlgorithmInstance.h"
#include "drng.c"
#include "afs_tredm.c"

#define SEED_LEN_BYTES_LOCAL 64
#define MSG_LEN_BITS_2_33_LOCAL 8589934592ULL

/* Function print_hex_file: writes a byte string to a file in hexadecimal. */
static void print_hex_file(FILE *fp, const unsigned char *p, unsigned n)
{
    unsigned i;
    for (i = 0U; i < n; i++) {
        fprintf(fp, "%02X", p[i]);
    }
}

/* Function init_state: initializes a deterministic state or stream-hash context. */
static void init_state(uint64_t A[25], unsigned *rate_bits, unsigned *capacity_bits,
                       unsigned *rate_lanes, unsigned *capacity_lanes)
{
#if DIGEST_BIT_LENGTH == 512
    *rate_bits = 1024U; *capacity_bits = 576U; memcpy(A, IV_512, sizeof(uint64_t) * 25U);
#elif DIGEST_BIT_LENGTH == 768
    *rate_bits = 768U; *capacity_bits = 832U; memcpy(A, IV_768, sizeof(uint64_t) * 25U);
#elif DIGEST_BIT_LENGTH == 1024
    *rate_bits = 512U; *capacity_bits = 1088U; memcpy(A, IV_1024, sizeof(uint64_t) * 25U);
#else
# error Unsupported DIGEST_BIT_LENGTH
#endif
    *rate_lanes = *rate_bits >> 6U;
    *capacity_lanes = *capacity_bits >> 6U;
}

/* Function absorb_byte_block: absorbs one streaming byte block for long-message KAT support. */
static void absorb_byte_block(uint64_t A[25], const unsigned char *block,
                              unsigned rate_lanes, unsigned capacity_lanes)
{
    uint64_t lanes[AFS_TREDM_MAX_RATE_LANES];
    unsigned i;
    for (i = 0U; i < rate_lanes; i++) {
        lanes[i] = load_be64(block + 8U * i);
    }
    absorb_prepared_block(A, lanes, rate_lanes, capacity_lanes);
}

/* Function hash_stream_constant: streams a long constant-input KAT message through the hash logic. */
static void hash_stream_constant(int ones, unsigned char *digest)
{
    uint64_t A[25];
    unsigned rate_bits, capacity_bits, rate_lanes, capacity_lanes;
    unsigned long long full_blocks;
    unsigned rem_bytes;
    unsigned char block[128]; /* max rate is 1024 bits */
    unsigned long long b;

    init_state(A, &rate_bits, &capacity_bits, &rate_lanes, &capacity_lanes);
    full_blocks = MSG_LEN_BITS_2_33_LOCAL / (unsigned long long)rate_bits;
    rem_bytes = (unsigned)((MSG_LEN_BITS_2_33_LOCAL % (unsigned long long)rate_bits) >> 3U);

    memset(block, ones ? 0xFF : 0x00, sizeof(block));
    for (b = 0ULL; b < full_blocks; b++) {
        absorb_byte_block(A, block, rate_lanes, capacity_lanes);
    }

    absorb_final_framed_blocks(A, block, MSG_LEN_BITS_2_33_LOCAL,
                               rem_bytes == 0U ? full_blocks : 0ULL,
                               rate_bits, rate_lanes, capacity_lanes,
                               (unsigned)DIGEST_BIT_LENGTH, capacity_bits);
    extract_digest(A, digest, (unsigned)DIGEST_BIT_LENGTH, rate_lanes);
}

/* Function hash_stream_random: streams a long deterministic-random KAT message through the hash logic. */
static void hash_stream_random(const unsigned char seed[SEED_LEN_BYTES_LOCAL],
                               unsigned char first8[8], unsigned char last8[8],
                               unsigned char *digest)
{
    uint64_t A[25];
    unsigned rate_bits, capacity_bits, rate_lanes, capacity_lanes;
    unsigned block_bytes;
    unsigned rem_bytes;
    unsigned long long full_blocks;
    unsigned long long total_bytes = MSG_LEN_BITS_2_33_LOCAL / 8ULL;
    unsigned long long produced = 0ULL;
    unsigned long long b;
    unsigned char block[128];
    unsigned char data[SEEDLEN];
    unsigned char w[OUTLEN];
    unsigned wpos = OUTLEN;
    DRNG_ctx drng;

    init_state(A, &rate_bits, &capacity_bits, &rate_lanes, &capacity_lanes);
    block_bytes = rate_bits >> 3U;
    full_blocks = MSG_LEN_BITS_2_33_LOCAL / (unsigned long long)rate_bits;
    rem_bytes = (unsigned)((MSG_LEN_BITS_2_33_LOCAL % (unsigned long long)rate_bits) >> 3U);

    init_random_number(&drng, seed, SEED_LEN_BYTES_LOCAL);
    memcpy(data, drng.V, sizeof(data));
    memset(first8, 0, 8U);
    memset(last8, 0, 8U);

    for (b = 0ULL; b < full_blocks; b++) {
        unsigned i;
        for (i = 0U; i < block_bytes; i++) {
            unsigned char byte;
            if (wpos >= OUTLEN) {
                sm3_bit(data, SEEDLEN * 8ULL, w);
                inc_Big_Number(data, SEEDLEN);
                wpos = 0U;
            }
            byte = w[wpos++];
            block[i] = byte;
            if (produced < 8ULL) {
                first8[produced] = byte;
            }
            if (produced >= total_bytes - 8ULL) {
                last8[produced - (total_bytes - 8ULL)] = byte;
            }
            produced++;
        }
        absorb_byte_block(A, block, rate_lanes, capacity_lanes);
    }

    if (rem_bytes != 0U) {
        unsigned i;
        memset(block, 0, sizeof(block));
        for (i = 0U; i < rem_bytes; i++) {
            unsigned char byte;
            if (wpos >= OUTLEN) {
                sm3_bit(data, SEEDLEN * 8ULL, w);
                inc_Big_Number(data, SEEDLEN);
                wpos = 0U;
            }
            byte = w[wpos++];
            block[i] = byte;
            if (produced < 8ULL) {
                first8[produced] = byte;
            }
            if (produced >= total_bytes - 8ULL) {
                last8[produced - (total_bytes - 8ULL)] = byte;
            }
            produced++;
        }
    }

    absorb_final_framed_blocks(A, block, MSG_LEN_BITS_2_33_LOCAL,
                               rem_bytes == 0U ? full_blocks : 0ULL,
                               rate_bits, rate_lanes, capacity_lanes,
                               (unsigned)DIGEST_BIT_LENGTH, capacity_bits);
    extract_digest(A, digest, (unsigned)DIGEST_BIT_LENGTH, rate_lanes);
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    const char *dir_name = "output";
    char filename[96] = "KAT_2_33_";
    char file_path[160];
    unsigned char seed[SEED_LEN_BYTES_LOCAL];
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    unsigned char first8[8], last8[8];
    FILE *fp;
    unsigned i;

#if defined(_WIN32)
    _mkdir(dir_name);
#else
    mkdir(dir_name, 0777);
#endif

    strcat(filename, ALGORITHM_INSTANCE);
    strcat(filename, ".txt");
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, filename);

    for (i = 0U; i < SEED_LEN_BYTES_LOCAL / 8U; i++) {
        memcpy(seed + 8U * i, filename, 8U);
    }

    fp = fopen(file_path, "wb");
    if (fp == NULL) {
        perror(file_path);
        return 1;
    }

    /* All-zero message. */
    hash_stream_constant(0, digest);
    fprintf(fp, "Msg_Len = %llu\n", MSG_LEN_BITS_2_33_LOCAL);
    fprintf(fp, "Msg_Seed = \n");
    fprintf(fp, "Msg_Exp = 0000000000000000 .... 0000000000000000\n");
    fprintf(fp, "Dst_Len = %d\n", DIGEST_BIT_LENGTH);
    fprintf(fp, "Dst = "); print_hex_file(fp, digest, DIGEST_BIT_LENGTH / 8U); fprintf(fp, "\n\n");

    /* All-one message. */
    hash_stream_constant(1, digest);
    fprintf(fp, "Msg_Len = %llu\n", MSG_LEN_BITS_2_33_LOCAL);
    fprintf(fp, "Msg_Seed = \n");
    fprintf(fp, "Msg_Exp = FFFFFFFFFFFFFFFF .... FFFFFFFFFFFFFFFF\n");
    fprintf(fp, "Dst_Len = %d\n", DIGEST_BIT_LENGTH);
    fprintf(fp, "Dst = "); print_hex_file(fp, digest, DIGEST_BIT_LENGTH / 8U); fprintf(fp, "\n\n");

    /* ICCS DRNG random message. */
    hash_stream_random(seed, first8, last8, digest);
    fprintf(fp, "Msg_Len = %llu\n", MSG_LEN_BITS_2_33_LOCAL);
    fprintf(fp, "Msg_Seed = "); print_hex_file(fp, seed, SEED_LEN_BYTES_LOCAL); fprintf(fp, "\n");
    fprintf(fp, "Msg_Exp = "); print_hex_file(fp, first8, 8U); fprintf(fp, " .... "); print_hex_file(fp, last8, 8U); fprintf(fp, "\n");
    fprintf(fp, "Dst_Len = %d\n", DIGEST_BIT_LENGTH);
    fprintf(fp, "Dst = "); print_hex_file(fp, digest, DIGEST_BIT_LENGTH / 8U); fprintf(fp, "\n\n");

    fclose(fp);
    return 0;
}
