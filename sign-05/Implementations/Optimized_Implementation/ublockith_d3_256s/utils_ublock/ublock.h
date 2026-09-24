/*
 * uBlock-256/256 block cipher — portable C implementation
 *
 *   256-bit block, 256-bit key, r = 24 rounds
 *
 * Reference:
 *   "uBlock: A New Efficient Symmetric Encryption Primitive"
 *   (https://doi.org/10.13868/j.cnki.jcr.2019.04.001)
 */

#ifndef UBLOCK_H
#define UBLOCK_H

#include <stdint.h>
#include <stdlib.h>
#include "params.h"

/* ------------------------------------------------------------------ *
 *  Parameters                                                         *
 * ------------------------------------------------------------------ */
#define UBLOCK_ROUNDS   24          /* r = 24 for uBlock-256/256       */
#define UBLOCK_BLOCK    32          /* block size in bytes              */
#define UBLOCK_KEY      32          /* key size in bytes                */

/* ------------------------------------------------------------------ *
 *  Key-schedule context                                               *
 *  Stores RK^0 … RK^24, each 256 bits as 8 big-endian uint32_t.     *
 * ------------------------------------------------------------------ */
typedef struct {
    uint32_t rk[25][8];
} ublock256_key_t;

/* ------------------------------------------------------------------ *
 *  uBlock-256/256 API                                                 *
 * ------------------------------------------------------------------ */
int  ublock256_set_key(const uint8_t key[32], ublock256_key_t *ks);
void ublock256_256_encrypt(const uint8_t in[32],  uint8_t out[32],
                           const ublock256_key_t *ks);
void ublock_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key, const uint8_t* in);
/* ------------------------------------------------------------------ *
 *  ECB wrapper                                                        *
 * ------------------------------------------------------------------ */
int  ublock256_ecb_encrypt(const ublock256_key_t *ks,
                           uint8_t *out, const uint8_t *in,
                           size_t blocks);

/* ------------------------------------------------------------------ *
 *  VOLE-based algebraic variant (SubDWord)                           *
 *                                                                     *
 *  For UBLOCKITH/VOLE-committed uBlock implementations.                  *
 *  These functions operate on nibble-level S-box ANF over GF(2^256). *
 *                                                                     *
 *  Include fields.h before using these:                              *
 *    #include "fields.h"                                             *
 * ------------------------------------------------------------------ */
#ifdef FIELDS_H
uint8_t ublock_subword_nibble(uint8_t x);
void    ublock_subword_nibble_tag(bf256_t *y_tag_deg0, bf256_t *y_tag_deg1, bf256_t *y_tag_deg2,
                                  const bf256_t *x_tag, uint8_t x);
void    ublock_subword_nibble_key(bf256_t *y_key, const bf256_t *x_key, bf256_t delta);
void    ublock_subword_nibble_key_pows(bf256_t *y_key, const bf256_t *x_key,
                                       bf256_t delta, bf256_t delta2, bf256_t delta3);

uint8_t ublock_T_nibble(uint8_t y);
void    ublock_T_nibble_tag(bf256_t *z_tag, const bf256_t *y_tag);
void    ublock_T_nibble_key(bf256_t *z_key, const bf256_t *y_key, bf256_t delta);

uint8_t ublock_invsubword_nibble(uint8_t y);
void    ublock_invsubword_nibble_tag(bf256_t *z_tag_deg0, bf256_t *z_tag_deg1, bf256_t *z_tag_deg2,
                                     const bf256_t *y_tag, uint8_t y);
void    ublock_invsubword_nibble_key(bf256_t *z_key, const bf256_t *y_key, bf256_t delta);
void    ublock_invsubword_nibble_key_pows(bf256_t *z_key, const bf256_t *y_key,
                                          bf256_t delta, bf256_t delta2, bf256_t delta3);
#endif

/* ------------------------------------------------------------------ *
 *  Inline helpers                                                     *
 * ------------------------------------------------------------------ */

/* 4-bit s-box (kept for analysis / sbox test code) */
static const uint8_t UBLOCK_SBOX[16] = {
    0x7, 0x4, 0x9, 0xc, 0xb, 0xa, 0xd, 0x8,
    0xf, 0xe, 0x1, 0x6, 0x0, 0x3, 0x2, 0x5
};
#define UBLOCK_SBOX_NIBBLE UBLOCK_SBOX

/* 4-bit inverse s-box */
static const uint8_t UBLOCK_INV_SBOX_NIBBLE[16] = {
    0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0,
    0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8
};

/* 8-bit combined S-box: SBOX_BYTE[v] = (SBOX[v>>4]<<4) | SBOX[v&0xf] */
static const uint8_t UBLOCK_SBOX_BYTE[256] = {
    0x77, 0x74, 0x79, 0x7c, 0x7b, 0x7a, 0x7d, 0x78,
    0x7f, 0x7e, 0x71, 0x76, 0x70, 0x73, 0x72, 0x75,
    0x47, 0x44, 0x49, 0x4c, 0x4b, 0x4a, 0x4d, 0x48,
    0x4f, 0x4e, 0x41, 0x46, 0x40, 0x43, 0x42, 0x45,
    0x97, 0x94, 0x99, 0x9c, 0x9b, 0x9a, 0x9d, 0x98,
    0x9f, 0x9e, 0x91, 0x96, 0x90, 0x93, 0x92, 0x95,
    0xc7, 0xc4, 0xc9, 0xcc, 0xcb, 0xca, 0xcd, 0xc8,
    0xcf, 0xce, 0xc1, 0xc6, 0xc0, 0xc3, 0xc2, 0xc5,
    0xb7, 0xb4, 0xb9, 0xbc, 0xbb, 0xba, 0xbd, 0xb8,
    0xbf, 0xbe, 0xb1, 0xb6, 0xb0, 0xb3, 0xb2, 0xb5,
    0xa7, 0xa4, 0xa9, 0xac, 0xab, 0xaa, 0xad, 0xa8,
    0xaf, 0xae, 0xa1, 0xa6, 0xa0, 0xa3, 0xa2, 0xa5,
    0xd7, 0xd4, 0xd9, 0xdc, 0xdb, 0xda, 0xdd, 0xd8,
    0xdf, 0xde, 0xd1, 0xd6, 0xd0, 0xd3, 0xd2, 0xd5,
    0x87, 0x84, 0x89, 0x8c, 0x8b, 0x8a, 0x8d, 0x88,
    0x8f, 0x8e, 0x81, 0x86, 0x80, 0x83, 0x82, 0x85,
    0xf7, 0xf4, 0xf9, 0xfc, 0xfb, 0xfa, 0xfd, 0xf8,
    0xff, 0xfe, 0xf1, 0xf6, 0xf0, 0xf3, 0xf2, 0xf5,
    0xe7, 0xe4, 0xe9, 0xec, 0xeb, 0xea, 0xed, 0xe8,
    0xef, 0xee, 0xe1, 0xe6, 0xe0, 0xe3, 0xe2, 0xe5,
    0x17, 0x14, 0x19, 0x1c, 0x1b, 0x1a, 0x1d, 0x18,
    0x1f, 0x1e, 0x11, 0x16, 0x10, 0x13, 0x12, 0x15,
    0x67, 0x64, 0x69, 0x6c, 0x6b, 0x6a, 0x6d, 0x68,
    0x6f, 0x6e, 0x61, 0x66, 0x60, 0x63, 0x62, 0x65,
    0x07, 0x04, 0x09, 0x0c, 0x0b, 0x0a, 0x0d, 0x08,
    0x0f, 0x0e, 0x01, 0x06, 0x00, 0x03, 0x02, 0x05,
    0x37, 0x34, 0x39, 0x3c, 0x3b, 0x3a, 0x3d, 0x38,
    0x3f, 0x3e, 0x31, 0x36, 0x30, 0x33, 0x32, 0x35,
    0x27, 0x24, 0x29, 0x2c, 0x2b, 0x2a, 0x2d, 0x28,
    0x2f, 0x2e, 0x21, 0x26, 0x20, 0x23, 0x22, 0x25,
    0x57, 0x54, 0x59, 0x5c, 0x5b, 0x5a, 0x5d, 0x58,
    0x5f, 0x5e, 0x51, 0x56, 0x50, 0x53, 0x52, 0x55,
};

static inline void ublock_apply_sn(uint8_t *data, size_t nbytes)
{
    for (size_t i = 0; i < nbytes; i++)
        data[i] = UBLOCK_SBOX_BYTE[data[i]];
}

/* pk: nibble permutation on K0||K1 (32 nibbles = 16 bytes). */
static const uint8_t pk[32] = {
    10, 5, 15, 0, 2, 7, 8, 13, 1, 14, 4, 12, 9, 11, 3, 6,
    24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23
};

/* ------------------------------------------------------------------ *
 *  Round constants RC_i (i=1..24), big-endian 32-bit words.          *
 *  RC[0] = RC_1.                                                      *
 * ------------------------------------------------------------------ */
static const uint32_t UBLOCK_RC[24] = {
    0x988cc9dd, 0xf0e4a1b5, 0x21357064, 0x8397d2c6,
    0xc7d39682, 0x4f5b1e0a, 0x5e4a0f1b, 0x7c682d39,
    0x392d687c, 0xb3a7e2f6, 0xa7b3f6e2, 0x8e9adfcb,
    0xdcc88d99, 0x786c293d, 0x30246175, 0xa1b5f0e4,
    0x8296d3c7, 0xc5d19480, 0x4a5e1b0f, 0x55410410,
    0x6b7f3a2e, 0x17034652, 0xeffbbeaa, 0x1f0b4e5a
};

#endif /* UBLOCK_H */
