/**
 * @file aes_armcrypto.c
 * @brief AArch64 ARM Crypto AES-128 CTR implementation for Optimized AES builds.
 *
 * This backend accelerates public-matrix byte generation with AArch64 AES
 * crypto instructions and exposes the same `aes.h` interface as the portable
 * and AES-NI backends.
 */

#include <stdint.h>
#include <string.h>
#include <arm_neon.h>
#include "aes.h"

#define AES_BLOCK_SIZE 16
#define AES128_KEY_SIZE 16
#define AES128_ROUNDS 10
#define AES128_KEY_SCHEDULE_WORDS (16 * (AES128_ROUNDS + 1))

// S-Box 和逆 S-Box
static const uint8_t sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};


// 轮常数
static const uint8_t Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

#define getSBoxValue(num) (sbox[(num)])
void AES128_load_schedule(const uint8_t *key, uint8_t *schedule)
{
    unsigned i, j, k;
    uint8_t tempa[4]; // Used for the column/row operations

    // The first round key is the key itself.
    for (i = 0; i < 4; ++i)
    {
        schedule[(i * 4) + 0] = key[(i * 4) + 0];
        schedule[(i * 4) + 1] = key[(i * 4) + 1];
        schedule[(i * 4) + 2] = key[(i * 4) + 2];
        schedule[(i * 4) + 3] = key[(i * 4) + 3];
    }

    // All other round keys are found from the previous round keys.
    for (i = 4; i < 4 * (10 + 1); ++i)
    {
        {
            k = (i - 1) * 4;
            tempa[0] = schedule[k + 0];
            tempa[1] = schedule[k + 1];
            tempa[2] = schedule[k + 2];
            tempa[3] = schedule[k + 3];
        }

        if (i % 4 == 0)
        {
            {
                const uint8_t u8tmp = tempa[0];
                tempa[0] = tempa[1];
                tempa[1] = tempa[2];
                tempa[2] = tempa[3];
                tempa[3] = u8tmp;
            }
            {
                tempa[0] = getSBoxValue(tempa[0]);
                tempa[1] = getSBoxValue(tempa[1]);
                tempa[2] = getSBoxValue(tempa[2]);
                tempa[3] = getSBoxValue(tempa[3]);
            }

            tempa[0] = tempa[0] ^ Rcon[i / 4];
        }

        j = i * 4;
        k = (i - 4) * 4;
        schedule[j + 0] = schedule[k + 0] ^ tempa[0];
        schedule[j + 1] = schedule[k + 1] ^ tempa[1];
        schedule[j + 2] = schedule[k + 2] ^ tempa[2];
        schedule[j + 3] = schedule[k + 3] ^ tempa[3];
    }
}

static inline uint8x16_t aes128_encrypt_block(uint8x16_t block,
                                              const uint8x16_t *subkeys)
{
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[0]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[1]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[2]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[3]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[4]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[5]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[6]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[7]));
    block = vaesmcq_u8(vaeseq_u8(block, subkeys[8]));
    block = vaeseq_u8(block, subkeys[9]);
    return veorq_u8(block, subkeys[10]);
}

void AES128_CTR_zero_sch(uint32_t counter_start, size_t block_count,
                         const uint8_t *schedule, uint8_t *ciphertext)
{
    uint8x16_t *subkeys = (uint8x16_t *)schedule;
    const uint32x4_t zero = vdupq_n_u32(0);

    while (block_count >= 4U)
    {
        uint8x16_t block0 =
            vreinterpretq_u8_u32(vsetq_lane_u32(counter_start + 0U, zero, 0));
        uint8x16_t block1 =
            vreinterpretq_u8_u32(vsetq_lane_u32(counter_start + 1U, zero, 0));
        uint8x16_t block2 =
            vreinterpretq_u8_u32(vsetq_lane_u32(counter_start + 2U, zero, 0));
        uint8x16_t block3 =
            vreinterpretq_u8_u32(vsetq_lane_u32(counter_start + 3U, zero, 0));

        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[0]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[0]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[0]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[0]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[1]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[1]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[1]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[1]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[2]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[2]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[2]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[2]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[3]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[3]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[3]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[3]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[4]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[4]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[4]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[4]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[5]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[5]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[5]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[5]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[6]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[6]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[6]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[6]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[7]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[7]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[7]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[7]));
        block0 = vaesmcq_u8(vaeseq_u8(block0, subkeys[8]));
        block1 = vaesmcq_u8(vaeseq_u8(block1, subkeys[8]));
        block2 = vaesmcq_u8(vaeseq_u8(block2, subkeys[8]));
        block3 = vaesmcq_u8(vaeseq_u8(block3, subkeys[8]));
        block0 = vaeseq_u8(block0, subkeys[9]);
        block1 = vaeseq_u8(block1, subkeys[9]);
        block2 = vaeseq_u8(block2, subkeys[9]);
        block3 = vaeseq_u8(block3, subkeys[9]);
        block0 = veorq_u8(block0, subkeys[10]);
        block1 = veorq_u8(block1, subkeys[10]);
        block2 = veorq_u8(block2, subkeys[10]);
        block3 = veorq_u8(block3, subkeys[10]);

        vst1q_u8(ciphertext + 0U * AES_BLOCK_SIZE, block0);
        vst1q_u8(ciphertext + 1U * AES_BLOCK_SIZE, block1);
        vst1q_u8(ciphertext + 2U * AES_BLOCK_SIZE, block2);
        vst1q_u8(ciphertext + 3U * AES_BLOCK_SIZE, block3);

        counter_start += 4U;
        ciphertext += 4U * AES_BLOCK_SIZE;
        block_count -= 4U;
    }

    while (block_count > 0U)
    {
        uint8x16_t block =
            vreinterpretq_u8_u32(vsetq_lane_u32(counter_start, zero, 0));
        block = aes128_encrypt_block(block, subkeys);
        vst1q_u8(ciphertext, block);

        counter_start++;
        ciphertext += AES_BLOCK_SIZE;
        block_count--;
    }
}

void AES128_free_schedule(uint8_t *schedule) { memset(schedule, 0, 16 * 11); }
