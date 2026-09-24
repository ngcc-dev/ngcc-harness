/*
The software is provided by the Institute of Commercial Cryptography
Standards (ICCS), and is used for algorithm submissions in the
Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will
be uninterrupted or error-free in all cases. ICCS will take no
responsibility for the use of the software or the results thereof, if the
software is used for any other purposes.
*/

/*
 * 8-way AVX2 SM3 Hash-DRBG. The DRBG control logic (derivation function,
 * instantiate, generate, big-number add/increment) is a lane-parallel copy
 * of SHUTTLE/ref/drng.c; the only vectorised part is the SM3 kernel, which
 * becomes the 8-way sm3hash_avx2() because every SM3 call hashes per-lane
 * messages of identical length. The big-number arithmetic on the 55-byte
 * state is cheap relative to SM3 and kept scalar per lane.
 */

#include "drng_avx2.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc_avx2.h" /* sm3hash_avx2 */

#define NW DRNG_WAY_AVX2
#define OUTLEN 32
#define DRNG_SUCCESS 0
#define DRNG_MEMORY_ALLOCATION_FAILED -2

#define HIGH_N_BIT_MASK(N) ((unsigned char)((~0U) << (8 - (N))))

/* ---- scalar big-number helpers (byte-array, big-endian, BN[len-1]=LSB)
 * ---- */
static void u32_to_u8_big_endian(unsigned int u32, unsigned char u8[4])
{
    u8[0] = (unsigned char)((u32 >> 24) & 0xFF);
    u8[1] = (unsigned char)((u32 >> 16) & 0xFF);
    u8[2] = (unsigned char)((u32 >> 8) & 0xFF);
    u8[3] = (unsigned char)((u32 >> 0) & 0xFF);
}

static void inc_Big_Number(unsigned char *BN, unsigned long long len)
{
    for (; len != 0; len--) {
        BN[len - 1] += 1;
        if (BN[len - 1])
            break;
    }
}

static void plus_Big_Number(unsigned char *BN1, const unsigned char *BN2,
                            const unsigned char *BN3,
                            const unsigned char *BN4,
                            unsigned long long len)
{
    unsigned int sum;
    unsigned char carry = 0;
    for (; len != 0; len--) {
        sum = BN1[len - 1] + BN2[len - 1] + BN3[len - 1] + BN4[len - 1] +
              carry;
        carry = (unsigned char)(sum / (0xFFU + 1));
        BN1[len - 1] = (unsigned char)(sum & 0xFFU);
    }
}

/*
 * 8-way SM3 derivation function. out[k] receives SEEDLEN bytes derived
 * from in[k] (in_len bytes, shared). Mirrors SM3_df(): for counter
 * = 1..ceil(SEEDLEN/ OUTLEN), hash (counter || u32be(8*SEEDLEN) || in[k])
 * and concatenate.
 */
static int df_8way(unsigned char out[NW][SEEDLEN],
                   const unsigned char *const in[NW], size_t in_len)
{
    const size_t mlen = 1 + 4 + in_len; /* counter || len4 || input */
    const size_t blocks = (SEEDLEN + OUTLEN - 1) / OUTLEN;
    unsigned char len4[4];
    unsigned char temp[NW][((SEEDLEN + OUTLEN - 1) / OUTLEN) * OUTLEN];
    const unsigned char *mp[NW];
    unsigned char *dg[NW];

    u32_to_u8_big_endian(8 * SEEDLEN, len4);
    unsigned char *buf = (unsigned char *)malloc(NW * mlen);
    if (buf == NULL)
        return DRNG_MEMORY_ALLOCATION_FAILED;

    for (size_t c = 1; c <= blocks; c++) {
        for (int k = 0; k < NW; k++) {
            unsigned char *b = buf + (size_t)k * mlen;
            b[0] = (unsigned char)c;
            memcpy(b + 1, len4, 4);
            if (in_len)
                memcpy(b + 5, in[k], in_len);
            mp[k] = b;
            dg[k] = temp[k] + (c - 1) * OUTLEN;
        }
        sm3hash_avx2(mp, (unsigned long long)mlen * 8, dg);
    }
    for (int k = 0; k < NW; k++)
        memcpy(out[k], temp[k], SEEDLEN);

    free(buf);
    return DRNG_SUCCESS;
}

int init_random_number_avx2(DRNG_ctx_avx2 *drng,
                            const unsigned char *const seed[NW],
                            unsigned long long seed_len_bytes)
{
    int rc;
    unsigned char padded_V[NW][1 + SEEDLEN];
    const unsigned char *pp[NW];
    unsigned char C[NW][SEEDLEN];

    memset(drng, 0, sizeof(*drng));

    /* V = df(seed) */
    rc = df_8way(drng->V, seed, (size_t)seed_len_bytes);
    if (rc != DRNG_SUCCESS)
        return rc;

    /* C = df(0x00 || V) */
    for (int k = 0; k < NW; k++) {
        padded_V[k][0] = 0x00;
        memcpy(padded_V[k] + 1, drng->V[k], SEEDLEN);
        pp[k] = padded_V[k];
    }
    rc = df_8way(C, pp, 1 + SEEDLEN);
    if (rc != DRNG_SUCCESS)
        return rc;
    for (int k = 0; k < NW; k++)
        memcpy(drng->C[k], C[k], SEEDLEN);

    /* reseed_counter = 1 */
    for (int k = 0; k < NW; k++)
        inc_Big_Number(drng->reseed_counter[k], SEEDLEN);

    return DRNG_SUCCESS;
}

int get_random_number_avx2(DRNG_ctx_avx2 *drng,
                           unsigned char *const random_number[NW],
                           unsigned long long random_number_len_bits)
{
    unsigned long long m, req_bytes, remainder;
    unsigned char data[NW][SEEDLEN];
    unsigned char w[NW][OUTLEN];
    unsigned char padded_V[NW][1 + SEEDLEN];
    const unsigned char *mp[NW];
    unsigned char *dg[NW];

    m = random_number_len_bits / (OUTLEN * 8);
    if (random_number_len_bits % (OUTLEN * 8))
        m++;
    req_bytes = random_number_len_bits / 8;
    if (random_number_len_bits % 8)
        req_bytes++;
    remainder = req_bytes;

    for (int k = 0; k < NW; k++)
        memcpy(data[k], drng->V[k], SEEDLEN);

    for (unsigned long long i = 0; i < m; i++) {
        for (int k = 0; k < NW; k++) {
            mp[k] = data[k];
            dg[k] = w[k];
        }
        sm3hash_avx2(mp, (unsigned long long)SEEDLEN * 8, dg);

        size_t take = (remainder >= OUTLEN) ? OUTLEN : (size_t)remainder;
        for (int k = 0; k < NW; k++)
            memcpy(random_number[k] + OUTLEN * i, w[k], take);
        remainder -= take;

        for (int k = 0; k < NW; k++)
            inc_Big_Number(data[k], SEEDLEN);
    }

    if (req_bytes >= 1) {
        unsigned char mask = HIGH_N_BIT_MASK(
            8 - (unsigned)(8 * req_bytes - random_number_len_bits));
        for (int k = 0; k < NW; k++)
            random_number[k][req_bytes - 1] &= mask;
    }

    /* state update: V += SM3(0x03 || V) (right-aligned in SEEDLEN) + C +
     * ctr */
    for (int k = 0; k < NW; k++) {
        padded_V[k][0] = 0x03;
        memcpy(padded_V[k] + 1, drng->V[k], SEEDLEN);
        mp[k] = padded_V[k];
        dg[k] = w[k]; /* reuse w as the 32-byte digest scratch */
    }
    sm3hash_avx2(mp, (unsigned long long)(1 + SEEDLEN) * 8, dg);

    for (int k = 0; k < NW; k++) {
        unsigned char H[SEEDLEN];
        memset(H, 0, sizeof(H));
        memcpy(H + (SEEDLEN - OUTLEN), w[k], OUTLEN);
        plus_Big_Number(drng->V[k], H, drng->C[k], drng->reseed_counter[k],
                        SEEDLEN);
        inc_Big_Number(drng->reseed_counter[k], SEEDLEN);
    }

    return DRNG_SUCCESS;
}
