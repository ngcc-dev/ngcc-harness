/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
#include "drng.h"
#include "sm3x4.h"

#define OUTLEN (32)
#define DRNG_SUCCESS 0
#define DRNG_MEMORY_ALLOCATION_FAILED -2

#define MIN_INT(a, b) ((a) < (b) ? (a) : (b))
#define MAX_INT(a, b) ((a) > (b) ? (a) : (b))
#define DIVISION_ROUND_UP(dividend, divisor, result) \
    do                                               \
    {                                                \
        (result) = (dividend) / (divisor);           \
        if ((dividend) % (divisor))                  \
            (result)++;                              \
    } while (0)

#define HIGH_N_BIT_MASK(N) ((unsigned char)((~0U) << (8 - (N))))
#define FF1(x, y, z) ((x) ^ (y) ^ (z))
#define FF2(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG1(x, y, z) ((x) ^ (y) ^ (z))
#define GG2(x, y, z) ((((y) ^ (z)) & (x)) ^ (z))
#define L_SHIFT(a, n) ((a) << (n) | ((a) & 0xFFFFFFFF) >> (32 - (n)))
#define P0(x) ((x) ^ L_SHIFT((x), 9) ^ L_SHIFT((x), 17))
#define P1(x) ((x) ^ L_SHIFT((x), 15) ^ L_SHIFT((x), 23))
#define PUT32(a, b) ((a)[0] = (unsigned char)((b) >> 24), \
                     (a)[1] = (unsigned char)((b) >> 16), \
                     (a)[2] = (unsigned char)((b) >> 8),  \
                     (a)[3] = (unsigned char)(b))

static void sm3_bit_init(unsigned int *init_digest)
{
    init_digest[0] = 0x7380166F;
    init_digest[1] = 0x4914B2B9;
    init_digest[2] = 0x172442D7;
    init_digest[3] = 0xDA8A0600;
    init_digest[4] = 0xA96F30BC;
    init_digest[5] = 0x163138AA;
    init_digest[6] = 0xE38DEE4D;
    init_digest[7] = 0xB0FB0E4E;
}

static void sm3_bit_compress(unsigned int dgst[8], const unsigned char *msg, unsigned long long blocks)
{
    unsigned int A, B, C, D, E, F, G, H;
    unsigned int W[68];
    unsigned int W_prime[64];
    unsigned int SS1, SS2, TT1, TT2;
    int i;
    while (blocks--)
    {
        /************************Round function************************/
        for (i = 0; i < 16; i++)
        {
            W[i] = ((unsigned int)(msg + i * 4)[0] << 24 | (unsigned int)(msg + i * 4)[1] << 16 | (unsigned int)(msg + i * 4)[2] << 8 | (unsigned int)(msg + i * 4)[3]);
        }
        for (; i < 68; i++)
        {
            W[i] = P1(W[i - 16] ^ W[i - 9] ^ L_SHIFT(W[i - 3], 15)) ^ L_SHIFT(W[i - 13], 7) ^ W[i - 6];
        }
        for (i = 0; i < 64; i++)
        {
            W_prime[i] = W[i] ^ W[i + 4];
        }
        A = dgst[0];
        B = dgst[1];
        C = dgst[2];
        D = dgst[3];
        E = dgst[4];
        F = dgst[5];
        G = dgst[6];
        H = dgst[7];
        for (i = 0; i < 64; i++)
        {
            if (i < 16)
            {
                SS1 = L_SHIFT(L_SHIFT(A, 12) + E + L_SHIFT(0x79cc4519U, i & 0x1F), 7);
            }
            else
            {
                SS1 = L_SHIFT(L_SHIFT(A, 12) + E + L_SHIFT(0x7a879d8aU, i & 0x1F), 7);
            }
            SS2 = SS1 ^ L_SHIFT(A, 12);
            if (i < 16)
            {
                TT1 = FF1(A, B, C) + D + SS2 + W_prime[i];
                TT2 = GG1(E, F, G) + H + SS1 + W[i];
            }
            else
            {
                TT1 = FF2(A, B, C) + D + SS2 + W_prime[i];
                TT2 = GG2(E, F, G) + H + SS1 + W[i];
            }
            D = C;
            C = L_SHIFT(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = L_SHIFT(F, 19);
            F = E;
            E = P0(TT2);
        }
        dgst[0] ^= A;
        dgst[1] ^= B;
        dgst[2] ^= C;
        dgst[3] ^= D;
        dgst[4] ^= E;
        dgst[5] ^= F;
        dgst[6] ^= G;
        dgst[7] ^= H;
        msg += 64;
    }
}

static void sm3_bit(const unsigned char *msg, unsigned long long msg_bitlen, unsigned char *dgst)
{
    unsigned long long block_num;
    unsigned long long remain;
    block_num = msg_bitlen / 512;
    remain = msg_bitlen & 0x1FF;
    /**********************Initializing value**********************/
    unsigned int digest[8];
    sm3_bit_init(digest);
    /************************Round function************************/
    if (block_num != 0)
    {
        sm3_bit_compress(digest, msg, block_num);
    }
    unsigned char block[64];
    memset(block, 0, 64);
    memcpy(block, msg + block_num * 64, (remain + 7) >> 3);
    block[remain >> 3] &= ((0xFF00 >> (remain & 0x7)) & 0xFF);
    block[remain >> 3] |= (1 << (7 - (remain & 0x7)));
    /***************************Padding****************************/
    if (remain <= 512 - 65)
    {
        memset(block + (remain >> 3) + 1, 0, (512 - remain - 65) >> 3);
    }
    else
    {
        memset(block + (remain >> 3) + 1, 0, (512 - remain - 1) >> 3);
        sm3_bit_compress(digest, block, 1);
        memset(block, 0, 64 - 8);
    }
    PUT32(block + 56, block_num >> 32 << 9);
    PUT32(block + 60, (block_num << 9) + (remain));
    /************************Round function************************/
    sm3_bit_compress(digest, block, 1);
    /**********************Output hash value***********************/
    for (int i = 0; i < 8; i++)
    {
        PUT32(dgst + i * 4, digest[i]);
    }
}

static void u32_to_u8_big_endian(unsigned int u32, unsigned char u8[4])
{
    u8[0] = (unsigned char)((u32 >> 24) & 0xFF);
    u8[1] = (unsigned char)((u32 >> 16) & 0xFF);
    u8[2] = (unsigned char)((u32 >> 8) & 0xFF);
    u8[3] = (unsigned char)((u32 >> 0) & 0xFF);
}

static void inc_Big_Number(unsigned char *BN, unsigned long long BN_len_bytes)
{
    for (; BN_len_bytes != 0; BN_len_bytes--)
    {
        BN[BN_len_bytes - 1] += 1;
        if (BN[BN_len_bytes - 1])
            break;
    }
}

static void plus_Big_Number(unsigned char *BN1, const unsigned char *BN2, const unsigned char *BN3, const unsigned char *BN4, unsigned long long BN_len_bytes)
{
    unsigned int sum;
    unsigned char carry = 0;

    for (; BN_len_bytes != 0; BN_len_bytes--)
    {
        sum = BN1[BN_len_bytes - 1] + BN2[BN_len_bytes - 1] + BN3[BN_len_bytes - 1] + BN4[BN_len_bytes - 1] + carry;
        carry = sum / (0xFFU + 1);
        BN1[BN_len_bytes - 1] = (unsigned char)(sum & 0xFFU);
    }
}

static void copy_bytes_4x_avx2(unsigned char *dst0,
                               unsigned char *dst1,
                               unsigned char *dst2,
                               unsigned char *dst3,
                               const unsigned char *src0,
                               const unsigned char *src1,
                               const unsigned char *src2,
                               const unsigned char *src3,
                               unsigned long long len)
{
    unsigned long long i = 0;

    for(; i + 32 <= len; i += 32)
    {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(src0 + i));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(src1 + i));
        __m256i v2 = _mm256_loadu_si256((const __m256i *)(src2 + i));
        __m256i v3 = _mm256_loadu_si256((const __m256i *)(src3 + i));
        _mm256_storeu_si256((__m256i *)(dst0 + i), v0);
        _mm256_storeu_si256((__m256i *)(dst1 + i), v1);
        _mm256_storeu_si256((__m256i *)(dst2 + i), v2);
        _mm256_storeu_si256((__m256i *)(dst3 + i), v3);
    }

    if(i + 16 <= len)
    {
        __m128i v0 = _mm_loadu_si128((const __m128i *)(src0 + i));
        __m128i v1 = _mm_loadu_si128((const __m128i *)(src1 + i));
        __m128i v2 = _mm_loadu_si128((const __m128i *)(src2 + i));
        __m128i v3 = _mm_loadu_si128((const __m128i *)(src3 + i));
        _mm_storeu_si128((__m128i *)(dst0 + i), v0);
        _mm_storeu_si128((__m128i *)(dst1 + i), v1);
        _mm_storeu_si128((__m128i *)(dst2 + i), v2);
        _mm_storeu_si128((__m128i *)(dst3 + i), v3);
        i += 16;
    }

    if(i < len)
    {
        memcpy(dst0 + i, src0 + i, len - i);
        memcpy(dst1 + i, src1 + i, len - i);
        memcpy(dst2 + i, src2 + i, len - i);
        memcpy(dst3 + i, src3 + i, len - i);
    }
}

static void inc_Big_Number_4x_avx2(unsigned char *BN0,
                                   unsigned char *BN1,
                                   unsigned char *BN2,
                                   unsigned char *BN3,
                                   unsigned long long BN_len_bytes)
{
    __m256i carry = _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1);
    const __m256i ff = _mm256_set1_epi32(0xFF);
    const __m256i one = _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1);
    uint32_t lanes[8];

    for(; BN_len_bytes != 0; BN_len_bytes--)
    {
        __m256i x = _mm256_set_epi32(0, 0, 0, 0,
                                     BN3[BN_len_bytes - 1], BN2[BN_len_bytes - 1],
                                     BN1[BN_len_bytes - 1], BN0[BN_len_bytes - 1]);
        __m256i sum = _mm256_add_epi32(x, carry);

        _mm256_storeu_si256((__m256i *)lanes, _mm256_and_si256(sum, ff));
        BN0[BN_len_bytes - 1] = (unsigned char)lanes[0];
        BN1[BN_len_bytes - 1] = (unsigned char)lanes[1];
        BN2[BN_len_bytes - 1] = (unsigned char)lanes[2];
        BN3[BN_len_bytes - 1] = (unsigned char)lanes[3];

        carry = _mm256_and_si256(_mm256_cmpgt_epi32(sum, ff), one);
        if(_mm256_testz_si256(carry, one))
            break;
    }
}

static void plus_Big_Number_4x_avx2(unsigned char *BN10,
                                    unsigned char *BN11,
                                    unsigned char *BN12,
                                    unsigned char *BN13,
                                    const unsigned char *BN20,
                                    const unsigned char *BN21,
                                    const unsigned char *BN22,
                                    const unsigned char *BN23,
                                    const unsigned char *BN30,
                                    const unsigned char *BN31,
                                    const unsigned char *BN32,
                                    const unsigned char *BN33,
                                    const unsigned char *BN40,
                                    const unsigned char *BN41,
                                    const unsigned char *BN42,
                                    const unsigned char *BN43,
                                    unsigned long long BN_len_bytes)
{
    __m256i carry = _mm256_setzero_si256();
    const __m256i ff = _mm256_set1_epi32(0xFF);
    uint32_t lanes[8];

    for(; BN_len_bytes != 0; BN_len_bytes--)
    {
        __m256i a = _mm256_set_epi32(0, 0, 0, 0,
                                     BN13[BN_len_bytes - 1], BN12[BN_len_bytes - 1],
                                     BN11[BN_len_bytes - 1], BN10[BN_len_bytes - 1]);
        __m256i b = _mm256_set_epi32(0, 0, 0, 0,
                                     BN23[BN_len_bytes - 1], BN22[BN_len_bytes - 1],
                                     BN21[BN_len_bytes - 1], BN20[BN_len_bytes - 1]);
        __m256i c = _mm256_set_epi32(0, 0, 0, 0,
                                     BN33[BN_len_bytes - 1], BN32[BN_len_bytes - 1],
                                     BN31[BN_len_bytes - 1], BN30[BN_len_bytes - 1]);
        __m256i d = _mm256_set_epi32(0, 0, 0, 0,
                                     BN43[BN_len_bytes - 1], BN42[BN_len_bytes - 1],
                                     BN41[BN_len_bytes - 1], BN40[BN_len_bytes - 1]);
        __m256i sum = _mm256_add_epi32(_mm256_add_epi32(a, b),
                                       _mm256_add_epi32(carry, _mm256_add_epi32(c, d)));

        _mm256_storeu_si256((__m256i *)lanes, _mm256_and_si256(sum, ff));
        BN10[BN_len_bytes - 1] = (unsigned char)lanes[0];
        BN11[BN_len_bytes - 1] = (unsigned char)lanes[1];
        BN12[BN_len_bytes - 1] = (unsigned char)lanes[2];
        BN13[BN_len_bytes - 1] = (unsigned char)lanes[3];

        carry = _mm256_srli_epi32(sum, 8);
    }
}

static void mask_output_4x(unsigned char *out0,
                           unsigned char *out1,
                           unsigned char *out2,
                           unsigned char *out3,
                           unsigned long long requested_no_of_bits)
{
    unsigned long long requested_no_of_Byte;

    DIVISION_ROUND_UP(requested_no_of_bits, 8, requested_no_of_Byte);
    if(requested_no_of_Byte >= 1)
    {
        unsigned char mask = HIGH_N_BIT_MASK(8 - (8 * requested_no_of_Byte - requested_no_of_bits));
        out0[requested_no_of_Byte - 1] &= mask;
        out1[requested_no_of_Byte - 1] &= mask;
        out2[requested_no_of_Byte - 1] &= mask;
        out3[requested_no_of_Byte - 1] &= mask;
    }
}

static void prepare_df_input_4x(unsigned char *dst0,
                                unsigned char *dst1,
                                unsigned char *dst2,
                                unsigned char *dst3,
                                const unsigned char *src0,
                                const unsigned char *src1,
                                const unsigned char *src2,
                                const unsigned char *src3,
                                unsigned long long input_string_len_bytes,
                                unsigned char counter,
                                const unsigned char number_of_bits_to_return_big_endian[4])
{
    dst0[0] = counter;
    dst1[0] = counter;
    dst2[0] = counter;
    dst3[0] = counter;

    memcpy(dst0 + 1, number_of_bits_to_return_big_endian, 4);
    memcpy(dst1 + 1, number_of_bits_to_return_big_endian, 4);
    memcpy(dst2 + 1, number_of_bits_to_return_big_endian, 4);
    memcpy(dst3 + 1, number_of_bits_to_return_big_endian, 4);

    copy_bytes_4x_avx2(dst0 + 5, dst1 + 5, dst2 + 5, dst3 + 5,
                       src0, src1, src2, src3, input_string_len_bytes);
}

static int SM3_df(unsigned char *input_string, unsigned long long input_string_len_bytes)
{
    unsigned char temp[(1 + SEEDLEN / OUTLEN) * OUTLEN];
    unsigned long long len;
    unsigned char counter;
    unsigned char *data_to_SM3 = NULL;
    unsigned long long data_to_SM3_len_bytes;
    unsigned char sm3_dgst[OUTLEN];
    unsigned char number_of_bits_to_return_big_endian[4];

    memset(temp, 0, sizeof(temp));
    DIVISION_ROUND_UP(SEEDLEN, OUTLEN, len);
    counter = 0x01;
    for (unsigned long long i = 0; i < len; i++)
    {
        data_to_SM3_len_bytes = sizeof(counter) + sizeof(number_of_bits_to_return_big_endian) + input_string_len_bytes;
        data_to_SM3 = (unsigned char *)malloc(data_to_SM3_len_bytes);
        if (NULL == data_to_SM3)
        {
            fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
            return DRNG_MEMORY_ALLOCATION_FAILED;
        }
        data_to_SM3[0] = counter;
        u32_to_u8_big_endian(8 * SEEDLEN, number_of_bits_to_return_big_endian);
        memcpy(data_to_SM3 + sizeof(counter), number_of_bits_to_return_big_endian, sizeof(number_of_bits_to_return_big_endian));
        memcpy(data_to_SM3 + sizeof(counter) + sizeof(number_of_bits_to_return_big_endian), input_string, input_string_len_bytes);
        sm3_bit(data_to_SM3, data_to_SM3_len_bytes * 8, sm3_dgst);
        memcpy(temp + i * OUTLEN, sm3_dgst, OUTLEN);
        free(data_to_SM3);
        data_to_SM3 = NULL;
        counter++;
    }
    memcpy(input_string, temp, SEEDLEN);

    return DRNG_SUCCESS;
}

static int SM3_DRNG_Instantiate(DRNG_ctx *drng, const unsigned char *nonce, unsigned long long nonce_len_bytes)
{
    unsigned char *seed_material = NULL;
    unsigned char seed[SEEDLEN];
    unsigned char padded_V[1 + sizeof(drng->V)];

    memset(drng, 0, sizeof(*drng));
    seed_material = (unsigned char *)malloc(MAX_INT(nonce_len_bytes, SEEDLEN));
    if (NULL == seed_material)
    {
        fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        return DRNG_MEMORY_ALLOCATION_FAILED;
    }
    memset(seed_material, 0, MAX_INT(nonce_len_bytes, SEEDLEN));
    memcpy(seed_material, nonce, nonce_len_bytes);
    SM3_df(seed_material, nonce_len_bytes);
    memcpy(seed, seed_material, sizeof(seed));
    memcpy(drng->V, seed, sizeof(drng->V));
    padded_V[0] = 0x00;
    memcpy(padded_V + 1, drng->V, sizeof(drng->V));
    SM3_df(padded_V, sizeof(padded_V));
    memcpy(drng->C, padded_V, sizeof(drng->C));
    inc_Big_Number(drng->reseed_counter, SEEDLEN);

    free(seed_material);
    return DRNG_SUCCESS;
}

static int SM3_DRNG_Generate(DRNG_ctx *drng, unsigned long long requested_no_of_bits, unsigned char *return_bits)
{
    unsigned long long m;
    unsigned char data[SEEDLEN];
    unsigned char w[OUTLEN];
    unsigned char padded_V[1 + sizeof(drng->V)];
    unsigned char H[SEEDLEN];
    unsigned long long requested_no_of_Byte;
    unsigned long long remainder_Byte;

    DIVISION_ROUND_UP(requested_no_of_bits, OUTLEN * 8, m);
    memcpy(data, drng->V, SEEDLEN);
    DIVISION_ROUND_UP(requested_no_of_bits, 8, requested_no_of_Byte);
    remainder_Byte = requested_no_of_Byte;
    for (unsigned long long i = 0; i < m; i++)
    {
        sm3_bit(data, SEEDLEN * 8, w);
        if (remainder_Byte >= sizeof(w))
        {
            memcpy(return_bits + OUTLEN * i, w, sizeof(w));
            remainder_Byte -= sizeof(w);
        }
        else
        {
            for (unsigned long long j = 0; j < sizeof(w); j++)
            {
                return_bits[OUTLEN * i + j] = w[j];
                remainder_Byte--;
                if (!remainder_Byte)
                {
                    break;
                }
            }
        }

        inc_Big_Number(data, SEEDLEN);
    }

    if (requested_no_of_Byte >= 1)
    {
        return_bits[requested_no_of_Byte - 1] &= HIGH_N_BIT_MASK(8 - (8 * requested_no_of_Byte - requested_no_of_bits));
    }
    memset(H, 0, sizeof(H));
    padded_V[0] = 0x03;
    memcpy(padded_V + 1, drng->V, sizeof(drng->V));
    sm3_bit(padded_V, sizeof(padded_V) * 8, H + (SEEDLEN - OUTLEN));
    plus_Big_Number(drng->V, H, drng->C, drng->reseed_counter, SEEDLEN);
    inc_Big_Number(drng->reseed_counter, SEEDLEN);

    return DRNG_SUCCESS;
}

int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes)
{
    return SM3_DRNG_Instantiate(drng, seed, seed_len_bytes);
}

int get_random_number(DRNG_ctx *drng, unsigned char *random_number, unsigned long long random_number_len_bits)
{
    return SM3_DRNG_Generate(drng, random_number_len_bits, random_number);
}

static int SM3_df_4x(unsigned char *input0,
                     unsigned char *input1,
                     unsigned char *input2,
                     unsigned char *input3,
                     unsigned long long input_string_len_bytes)
{
    unsigned char temp[4][(1 + SEEDLEN / OUTLEN) * OUTLEN];
    unsigned long long len;
    unsigned char counter;
    unsigned long long data_to_SM3_len_bytes;
    unsigned char number_of_bits_to_return_big_endian[4];
    unsigned char *data_to_SM3 = NULL;
    unsigned char *data0, *data1, *data2, *data3;
    memset(temp, 0, sizeof(temp));
    DIVISION_ROUND_UP(SEEDLEN, OUTLEN, len);
    data_to_SM3_len_bytes = 1 + sizeof(number_of_bits_to_return_big_endian) + input_string_len_bytes;
    u32_to_u8_big_endian(8 * SEEDLEN, number_of_bits_to_return_big_endian);

    data_to_SM3 = (unsigned char *)malloc(4 * data_to_SM3_len_bytes);
    if(NULL == data_to_SM3)
    {
        fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        return DRNG_MEMORY_ALLOCATION_FAILED;
    }

    data0 = data_to_SM3;
    data1 = data0 + data_to_SM3_len_bytes;
    data2 = data1 + data_to_SM3_len_bytes;
    data3 = data2 + data_to_SM3_len_bytes;

    counter = 0x01;
    for(unsigned long long i = 0; i < len; i++)
    {
        prepare_df_input_4x(data0, data1, data2, data3,
                            input0, input1, input2, input3,
                            input_string_len_bytes, counter,
                            number_of_bits_to_return_big_endian);

        sm3_bit_4x(data0, data1, data2, data3,
                   data_to_SM3_len_bytes * 8,
                   temp[0] + i * OUTLEN, temp[1] + i * OUTLEN,
                   temp[2] + i * OUTLEN, temp[3] + i * OUTLEN);
        counter++;
    }

    copy_bytes_4x_avx2(input0, input1, input2, input3,
                       temp[0], temp[1], temp[2], temp[3], SEEDLEN);

    free(data_to_SM3);

    return DRNG_SUCCESS;
}

static int SM3_DRNG_Instantiate_4x(DRNG_x4_ctx *drng_x4,
                                   const unsigned char *seed0,
                                   const unsigned char *seed1,
                                   const unsigned char *seed2,
                                   const unsigned char *seed3,
                                   unsigned long long seed_len_bytes)
{
    unsigned char *seed_material = NULL;
    unsigned char *seed_material0, *seed_material1, *seed_material2, *seed_material3;
    unsigned char padded_V[4][1 + SEEDLEN];
    unsigned long long material_len = MAX_INT(seed_len_bytes, SEEDLEN);
    int ret;

    memset(drng_x4, 0, sizeof(*drng_x4));
    memset(padded_V, 0, sizeof(padded_V));

    seed_material = (unsigned char *)malloc(4 * material_len);
    if(NULL == seed_material)
    {
        fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
        return DRNG_MEMORY_ALLOCATION_FAILED;
    }

    seed_material0 = seed_material;
    seed_material1 = seed_material0 + material_len;
    seed_material2 = seed_material1 + material_len;
    seed_material3 = seed_material2 + material_len;

    memset(seed_material, 0, 4 * material_len);
    copy_bytes_4x_avx2(seed_material0, seed_material1, seed_material2, seed_material3,
                       seed0, seed1, seed2, seed3, seed_len_bytes);

    ret = SM3_df_4x(seed_material0, seed_material1, seed_material2, seed_material3, seed_len_bytes);
    if(ret != DRNG_SUCCESS)
        goto cleanup;

    copy_bytes_4x_avx2(drng_x4->ctx[0].V, drng_x4->ctx[1].V, drng_x4->ctx[2].V, drng_x4->ctx[3].V,
                       seed_material0, seed_material1, seed_material2, seed_material3, SEEDLEN);
    padded_V[0][0] = 0x00;
    padded_V[1][0] = 0x00;
    padded_V[2][0] = 0x00;
    padded_V[3][0] = 0x00;
    copy_bytes_4x_avx2(padded_V[0] + 1, padded_V[1] + 1, padded_V[2] + 1, padded_V[3] + 1,
                       drng_x4->ctx[0].V, drng_x4->ctx[1].V, drng_x4->ctx[2].V, drng_x4->ctx[3].V, SEEDLEN);

    ret = SM3_df_4x(padded_V[0], padded_V[1], padded_V[2], padded_V[3], sizeof(padded_V[0]));
    if(ret != DRNG_SUCCESS)
        goto cleanup;

    copy_bytes_4x_avx2(drng_x4->ctx[0].C, drng_x4->ctx[1].C, drng_x4->ctx[2].C, drng_x4->ctx[3].C,
                       padded_V[0], padded_V[1], padded_V[2], padded_V[3], SEEDLEN);
    inc_Big_Number_4x_avx2(drng_x4->ctx[0].reseed_counter, drng_x4->ctx[1].reseed_counter,
                           drng_x4->ctx[2].reseed_counter, drng_x4->ctx[3].reseed_counter, SEEDLEN);

cleanup:
    free(seed_material);

    return ret;
}

static int SM3_DRNG_Generate_4x(DRNG_x4_ctx *drng_x4,
                                unsigned long long requested_no_of_bits,
                                unsigned char *return_bits0,
                                unsigned char *return_bits1,
                                unsigned char *return_bits2,
                                unsigned char *return_bits3)
{
    unsigned long long m;
    unsigned char data[4][SEEDLEN];
    unsigned char w[4][OUTLEN];
    unsigned char padded_V[4][1 + SEEDLEN];
    unsigned char H[4][SEEDLEN];
    unsigned long long requested_no_of_Byte;
    unsigned long long remainder_Byte[4];

    DIVISION_ROUND_UP(requested_no_of_bits, OUTLEN * 8, m);
    DIVISION_ROUND_UP(requested_no_of_bits, 8, requested_no_of_Byte);

    memset(H, 0, sizeof(H));
    memset(padded_V, 0, sizeof(padded_V));
    remainder_Byte[0] = requested_no_of_Byte;
    remainder_Byte[1] = requested_no_of_Byte;
    remainder_Byte[2] = requested_no_of_Byte;
    remainder_Byte[3] = requested_no_of_Byte;
    copy_bytes_4x_avx2(data[0], data[1], data[2], data[3],
                       drng_x4->ctx[0].V, drng_x4->ctx[1].V, drng_x4->ctx[2].V, drng_x4->ctx[3].V, SEEDLEN);

    for(unsigned long long i = 0; i < m; i++)
    {
        sm3_bit_4x(data[0], data[1], data[2], data[3], SEEDLEN * 8,
                   w[0], w[1], w[2], w[3]);

        if(remainder_Byte[0] >= sizeof(w[0]) &&
           remainder_Byte[1] >= sizeof(w[1]) &&
           remainder_Byte[2] >= sizeof(w[2]) &&
           remainder_Byte[3] >= sizeof(w[3]))
        {
            copy_bytes_4x_avx2(return_bits0 + OUTLEN * i, return_bits1 + OUTLEN * i,
                               return_bits2 + OUTLEN * i, return_bits3 + OUTLEN * i,
                               w[0], w[1], w[2], w[3], sizeof(w[0]));
            remainder_Byte[0] -= sizeof(w[0]);
            remainder_Byte[1] -= sizeof(w[1]);
            remainder_Byte[2] -= sizeof(w[2]);
            remainder_Byte[3] -= sizeof(w[3]);
        }
        else
        {
            if(remainder_Byte[0] != 0)
            {
                memcpy(return_bits0 + OUTLEN * i, w[0], remainder_Byte[0]);
                remainder_Byte[0] = 0;
            }
            if(remainder_Byte[1] != 0)
            {
                memcpy(return_bits1 + OUTLEN * i, w[1], remainder_Byte[1]);
                remainder_Byte[1] = 0;
            }
            if(remainder_Byte[2] != 0)
            {
                memcpy(return_bits2 + OUTLEN * i, w[2], remainder_Byte[2]);
                remainder_Byte[2] = 0;
            }
            if(remainder_Byte[3] != 0)
            {
                memcpy(return_bits3 + OUTLEN * i, w[3], remainder_Byte[3]);
                remainder_Byte[3] = 0;
            }
        }

        inc_Big_Number_4x_avx2(data[0], data[1], data[2], data[3], SEEDLEN);
    }

    mask_output_4x(return_bits0, return_bits1, return_bits2, return_bits3, requested_no_of_bits);

    padded_V[0][0] = 0x03;
    padded_V[1][0] = 0x03;
    padded_V[2][0] = 0x03;
    padded_V[3][0] = 0x03;
    copy_bytes_4x_avx2(padded_V[0] + 1, padded_V[1] + 1, padded_V[2] + 1, padded_V[3] + 1,
                       drng_x4->ctx[0].V, drng_x4->ctx[1].V, drng_x4->ctx[2].V, drng_x4->ctx[3].V, SEEDLEN);

    sm3_bit_4x(padded_V[0], padded_V[1], padded_V[2], padded_V[3],
               sizeof(padded_V[0]) * 8,
               H[0] + (SEEDLEN - OUTLEN), H[1] + (SEEDLEN - OUTLEN),
               H[2] + (SEEDLEN - OUTLEN), H[3] + (SEEDLEN - OUTLEN));

    plus_Big_Number_4x_avx2(drng_x4->ctx[0].V, drng_x4->ctx[1].V, drng_x4->ctx[2].V, drng_x4->ctx[3].V,
                            H[0], H[1], H[2], H[3],
                            drng_x4->ctx[0].C, drng_x4->ctx[1].C, drng_x4->ctx[2].C, drng_x4->ctx[3].C,
                            drng_x4->ctx[0].reseed_counter, drng_x4->ctx[1].reseed_counter,
                            drng_x4->ctx[2].reseed_counter, drng_x4->ctx[3].reseed_counter,
                            SEEDLEN);
    inc_Big_Number_4x_avx2(drng_x4->ctx[0].reseed_counter, drng_x4->ctx[1].reseed_counter,
                           drng_x4->ctx[2].reseed_counter, drng_x4->ctx[3].reseed_counter, SEEDLEN);

    return DRNG_SUCCESS;
}

int init_random_number_4x(DRNG_x4_ctx *drng_x4,
                          const unsigned char *seed0,
                          const unsigned char *seed1,
                          const unsigned char *seed2,
                          const unsigned char *seed3,
                          unsigned long long seed_len_bytes)
{
    return SM3_DRNG_Instantiate_4x(drng_x4, seed0, seed1, seed2, seed3, seed_len_bytes);
}

int get_random_number_4x(DRNG_x4_ctx *drng_x4,
                         unsigned char *random_number0,
                         unsigned char *random_number1,
                         unsigned char *random_number2,
                         unsigned char *random_number3,
                         unsigned long long random_number_len_bits)
{
    return SM3_DRNG_Generate_4x(drng_x4, random_number_len_bits,
                                random_number0, random_number1,
                                random_number2, random_number3);
}
