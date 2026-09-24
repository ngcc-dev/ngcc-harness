#ifndef SM4_H
#define SM4_H

#include <stdint.h>
#include <stdlib.h>
#include "params.h"
#include "utils.h"

#define SM4_KEY_SCHEDULE 32
#define SM4_ROUNDS 32
#define SM4_BLOCK_SIZE 16

typedef struct {
    uint32_t rk[SM4_KEY_SCHEDULE];
} SM4_KEY;

int SM4_set_key(const uint8_t *key, SM4_KEY *ks);
void SM4_encrypt(const uint8_t *in, uint8_t *out, const SM4_KEY *ks);
void SM4_encrypt_store_state(const uint8_t *in, uint8_t *out, const SM4_KEY *ks, uint32_t *states);

/* Practical acceleration mode switch:
 * 0: fully disabled (scalar only)
 * 1: x86 SM4NI-4 backend + avx2 ecb kernels if supported
 */

enum {
    SM4_ACCE_IMPL_SCALAR = 0,
    SM4_ACCE_IMPL_X86_SM4NI4 = 1,
    SM4_ACCE_IMPL_X86_YUCHEN_FAST = 2,
    SM4_ACCE_IMPL_X86_YUCHEN_FAST_ICE16 = 3,};

int SM4_set_key_acce(const uint8_t* key, SM4_KEY* ks, unsigned int* impl);
int SM4_encrypt_acce(const uint8_t* in, uint8_t* out, const SM4_KEY* ks, size_t blocks);

/* SM4 S-box computation (used internally by encryption) */
uint8_t sm4_compute_sbox(uint8_t in);

/* Optional PRG ECB wrapper type and functions (generic SM4-ECB)
 * These provide a uniform interface for SM4-ECB wrappers
 * helpers so the PRG can use SM4 as a drop-in replacement.
 */
typedef struct {
    SM4_KEY ks;
    //uint32_t rk_gf4[SM4_KEY_SCHEDULE];
    unsigned int seclvl;
    unsigned int acce_impl;
    unsigned int have_icelake16;
} generic_sm4_ecb_t;

int generic_sm4_ecb_new(generic_sm4_ecb_t* ctx, const uint8_t* key, unsigned int seclvl);
int generic_sm4_ecb_encrypt(generic_sm4_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks);
void generic_sm4_ecb_free(generic_sm4_ecb_t* ctx);

/* ================================================================== *
 *  Used for VOLE                                                     *
 * ================================================================== */

// 1 SM4 word is 4 bytes
typedef uint8_t sm4_word_t[4];

typedef struct {
  sm4_word_t expand_keys[36];  // k0-k35, each is 4 bytes, k[0][0] is the first byte of k0
} sm4_expand_keys_t;

typedef struct {
  sm4_word_t enc_state[36];  // X0-X35, each is 4 bytes, enc_state[0][0] is the first byte of X0
} sm4_enc_state_t;

void sm4_init_round_keys(uint8_t* expand_keys, const uint8_t* key);

void sm4_get_enc_state(uint8_t* enc_states, const uint8_t* in, const uint8_t* expand_keys);

void sm4_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key, const uint8_t* in);

/* ================================================================== *
 *  SM4 Transformation Functions (L and L_prime)                     *
 * ================================================================== */

static inline uint32_t sm4_load_be_u32(const uint8_t in[4])
{
    return ((uint32_t)in[0] << 24)
        | ((uint32_t)in[1] << 16)
        | ((uint32_t)in[2] <<  8)
        |  (uint32_t)in[3];
}

static inline void sm4_store_be_u32(uint32_t value, uint8_t out[4])
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >>  8);
    out[3] = (uint8_t)value;
}

/* SM4 linear transformation L used in key expansion */
static inline uint32_t SM4_L(uint32_t B)
{
    return B ^ rotl32_u(B, 2) ^ rotl32_u(B, 10) ^ rotl32_u(B, 18) ^ rotl32_u(B, 24);
}

/* SM4 inverse of linear transformation L */
static inline uint32_t SM4_L_inv(uint32_t C)
{
    return C
        ^ rotl32_u(C,  2) ^ rotl32_u(C,  4) ^ rotl32_u(C,  8) ^ rotl32_u(C, 12)
        ^ rotl32_u(C, 14) ^ rotl32_u(C, 16) ^ rotl32_u(C, 18) ^ rotl32_u(C, 22)
        ^ rotl32_u(C, 24) ^ rotl32_u(C, 30);
}

/* SM4 inverse of linear transformation L for 4 input/output bytes */
static inline void SM4_L_inv_bytes(const uint8_t in[4], uint8_t out[4])
{
    sm4_store_be_u32(SM4_L_inv(sm4_load_be_u32(in)), out);
}

/* SM4 linear transformation L_prime used in encryption rounds */
static inline uint32_t SM4_L_prime(uint32_t B)
{
    return B ^ rotl32_u(B, 13) ^ rotl32_u(B, 23);
}

/* SM4 inverse of linear transformation L_prime */
static inline uint32_t SM4_L_prime_inv(uint32_t C)
{
    return C
        ^ rotl32_u(C,  2) ^ rotl32_u(C,  4) ^ rotl32_u(C,  8) ^ rotl32_u(C, 11)
        ^ rotl32_u(C, 12) ^ rotl32_u(C, 14) ^ rotl32_u(C, 17) ^ rotl32_u(C, 22)
        ^ rotl32_u(C, 23) ^ rotl32_u(C, 24) ^ rotl32_u(C, 30) ^ rotl32_u(C, 31);
}

/* SM4 inverse of linear transformation L_prime for 4 input/output bytes */
static inline void SM4_L_prime_inv_bytes(const uint8_t in[4], uint8_t out[4])
{
    sm4_store_be_u32(SM4_L_prime_inv(sm4_load_be_u32(in)), out);
}

/* ================================================================== *
 *  SM4 Affine Transformation Functions and Constants                 *
 * ================================================================== */

#include "fields.h"

extern const uint8_t SM4_AFFINE_CONST;
extern const uint8_t SM4_INV_AFFINE_CONST;

uint8_t sm4_affine_byte(uint8_t x);
uint8_t sm4_inv_affine_byte(uint8_t y);

void sm4_affine_byte_tag(bf128_t *out_tag,
                         const bf128_t *input_tag,
                         const bf128_t *const_tag);

void sm4_inv_affine_byte_tag(bf128_t *out_tag,
                             const bf128_t *input_tag,
                             const bf128_t *const_tag);

unsigned int sm4_word_tag_index(unsigned int word_bit);

void SM4_L_prime_inv_bytes_tag(bf128_t *out_tag,
                               const bf128_t *in_tag);

void SM4_L_inv_bytes_tag(bf128_t *out_tag,
                         const bf128_t *in_tag);

/* Verifier-side aliases: same functions, _key suffix clarifies the operands
 * are VOLE keys (q = v + delta*w) rather than prover tags (v). */
#define sm4_affine_byte_key          sm4_affine_byte_tag
#define sm4_inv_affine_byte_key      sm4_inv_affine_byte_tag
#define SM4_L_inv_bytes_key          SM4_L_inv_bytes_tag
#define SM4_L_prime_inv_bytes_key    SM4_L_prime_inv_bytes_tag

/* ================================================================== *
 *  SM4 parameters, FK[4] and CK[32]                                 *
 * ================================================================== */

static const uint32_t FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

static const uint32_t CK[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};

#endif
