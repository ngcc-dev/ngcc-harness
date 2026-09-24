// The open-source C-ref implementation from https://github.com/jedisct1/vistrutah

#include "vistrutah.h"
#include "../compat.h"
#include "../fields.h"

#if defined(SBOX) && SBOX && !defined(SBOX_TABLE)
#define SBOX_TABLE
#endif

#ifdef SBOX_TABLE
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static uint8_t
compute_sbox_table(uint8_t in)
{
    return sbox[in];
}

static uint32_t aes_t0[256];
static uint32_t aes_t1[256];
static uint32_t aes_t2[256];
static uint32_t aes_t3[256];
static uint32_t aes_s0[256];
static uint32_t aes_s1[256];
static uint32_t aes_s2[256];
static uint32_t aes_s3[256];
static int      aes_ttables_ready = 0;

static inline uint32_t
load32_le(const uint8_t src[4])
{
    uint32_t v;
    memcpy(&v, src, sizeof(v));
    return le32toh(v);
}

static inline void
store32_le(uint8_t dst[4], uint32_t src)
{
    src = htole32(src);
    memcpy(dst, &src, sizeof(src));
}

static inline uint32_t rotl8_u32(uint32_t x) { return (x << 8) | (x >> 24); }
static inline uint32_t rotr8_u32(uint32_t x) { return (x >> 8) | (x << 24); }
static inline uint32_t rotr16_u32(uint32_t x) { return (x >> 16) | (x << 16); }

static inline uint8_t
xtime8(uint8_t x)
{
    return (uint8_t)((x << 1) ^ (((x >> 7) & 1) ? 0x1b : 0x00));
}

static void
aes_build_ttables(void)
{
    if (aes_ttables_ready) {
        return;
    }
    for (unsigned int i = 0; i < 256; ++i) {
        const uint8_t s  = compute_sbox_table((uint8_t)i);
        const uint8_t s2 = xtime8(s);
        const uint8_t s3 = s2 ^ s;
        const uint32_t t = ((uint32_t)s2) | ((uint32_t)s << 8) | ((uint32_t)s << 16) |
                           ((uint32_t)s3 << 24);

        aes_t0[i] = t;
        aes_t1[i] = rotl8_u32(t);
        aes_t2[i] = rotr16_u32(t);
        aes_t3[i] = rotr8_u32(t);
        aes_s0[i] = (uint32_t)s;
        aes_s1[i] = (uint32_t)s << 8;
        aes_s2[i] = (uint32_t)s << 16;
        aes_s3[i] = (uint32_t)s << 24;
    }
    aes_ttables_ready = 1;
}

static inline void
aes_round_ttable(uint8_t state[16], const uint8_t round_key[16])
{
    const uint32_t s0 = load32_le(state);
    const uint32_t s1 = load32_le(state + 4);
    const uint32_t s2 = load32_le(state + 8);
    const uint32_t s3 = load32_le(state + 12);

    const uint32_t n0 = aes_t0[s0 & 0xff] ^ aes_t1[(s1 >> 8) & 0xff] ^ aes_t2[(s2 >> 16) & 0xff] ^
                        aes_t3[(s3 >> 24) & 0xff] ^ load32_le(round_key);
    const uint32_t n1 = aes_t0[s1 & 0xff] ^ aes_t1[(s2 >> 8) & 0xff] ^ aes_t2[(s3 >> 16) & 0xff] ^
                        aes_t3[(s0 >> 24) & 0xff] ^ load32_le(round_key + 4);
    const uint32_t n2 = aes_t0[s2 & 0xff] ^ aes_t1[(s3 >> 8) & 0xff] ^ aes_t2[(s0 >> 16) & 0xff] ^
                        aes_t3[(s1 >> 24) & 0xff] ^ load32_le(round_key + 8);
    const uint32_t n3 = aes_t0[s3 & 0xff] ^ aes_t1[(s0 >> 8) & 0xff] ^ aes_t2[(s1 >> 16) & 0xff] ^
                        aes_t3[(s2 >> 24) & 0xff] ^ load32_le(round_key + 12);

    store32_le(state, n0);
    store32_le(state + 4, n1);
    store32_le(state + 8, n2);
    store32_le(state + 12, n3);
}

static inline void
aes_round_ttable_zero(uint8_t state[16])
{
    const uint32_t s0 = load32_le(state);
    const uint32_t s1 = load32_le(state + 4);
    const uint32_t s2 = load32_le(state + 8);
    const uint32_t s3 = load32_le(state + 12);

    const uint32_t n0 = aes_t0[s0 & 0xff] ^ aes_t1[(s1 >> 8) & 0xff] ^ aes_t2[(s2 >> 16) & 0xff] ^
                        aes_t3[(s3 >> 24) & 0xff];
    const uint32_t n1 = aes_t0[s1 & 0xff] ^ aes_t1[(s2 >> 8) & 0xff] ^ aes_t2[(s3 >> 16) & 0xff] ^
                        aes_t3[(s0 >> 24) & 0xff];
    const uint32_t n2 = aes_t0[s2 & 0xff] ^ aes_t1[(s3 >> 8) & 0xff] ^ aes_t2[(s0 >> 16) & 0xff] ^
                        aes_t3[(s1 >> 24) & 0xff];
    const uint32_t n3 = aes_t0[s3 & 0xff] ^ aes_t1[(s0 >> 8) & 0xff] ^ aes_t2[(s1 >> 16) & 0xff] ^
                        aes_t3[(s2 >> 24) & 0xff];

    store32_le(state, n0);
    store32_le(state + 4, n1);
    store32_le(state + 8, n2);
    store32_le(state + 12, n3);
}

static inline void
aes_final_round_ttable(uint8_t state[16], const uint8_t round_key[16])
{
    const uint32_t s0 = load32_le(state);
    const uint32_t s1 = load32_le(state + 4);
    const uint32_t s2 = load32_le(state + 8);
    const uint32_t s3 = load32_le(state + 12);

    const uint32_t n0 = aes_s0[s0 & 0xff] ^ aes_s1[(s1 >> 8) & 0xff] ^
                        aes_s2[(s2 >> 16) & 0xff] ^ aes_s3[(s3 >> 24) & 0xff] ^
                        load32_le(round_key);
    const uint32_t n1 = aes_s0[s1 & 0xff] ^ aes_s1[(s2 >> 8) & 0xff] ^
                        aes_s2[(s3 >> 16) & 0xff] ^ aes_s3[(s0 >> 24) & 0xff] ^
                        load32_le(round_key + 4);
    const uint32_t n2 = aes_s0[s2 & 0xff] ^ aes_s1[(s3 >> 8) & 0xff] ^
                        aes_s2[(s0 >> 16) & 0xff] ^ aes_s3[(s1 >> 24) & 0xff] ^
                        load32_le(round_key + 8);
    const uint32_t n3 = aes_s0[s3 & 0xff] ^ aes_s1[(s0 >> 8) & 0xff] ^
                        aes_s2[(s1 >> 16) & 0xff] ^ aes_s3[(s2 >> 24) & 0xff] ^
                        load32_le(round_key + 12);

    store32_le(state, n0);
    store32_le(state + 4, n1);
    store32_le(state + 8, n2);
    store32_le(state + 12, n3);
}
#else
static uint8_t
compute_sbox_compute(uint8_t in)
{
    const uint8_t t = bf8_inv(in);
    uint8_t out = 0;
    out |= (uint8_t)(parity8(t & 0xf1) << 0);
    out |= (uint8_t)(parity8(t & 0xe3) << 1);
    out |= (uint8_t)(parity8(t & 0xc7) << 2);
    out |= (uint8_t)(parity8(t & 0x8f) << 3);
    out |= (uint8_t)(parity8(t & 0x1f) << 4);
    out |= (uint8_t)(parity8(t & 0x3e) << 5);
    out |= (uint8_t)(parity8(t & 0x7c) << 6);
    out |= (uint8_t)(parity8(t & 0xf8) << 7);
    return (uint8_t)(out ^ 0x63);
}
#endif

#ifndef SBOX_TABLE
static uint8_t
compute_sbox(uint8_t in)
{
    return compute_sbox_compute(in);
}

static void
aes_sub_bytes(uint8_t state[16])
{
    for (unsigned int c = 0; c < 4; c++) {
        for (unsigned int r = 0; r < 4; r++) {
            state[c * 4 + r] = compute_sbox(state[c * 4 + r]);
        }
    }
}

static void
aes_shift_rows(uint8_t state[16])
{
    uint8_t new_state[16];

    for (unsigned int i = 0; i < 4; ++i) {
        new_state[i * 4 + 0] = state[i * 4 + 0];
        new_state[i * 4 + 1] = state[((i + 1) % 4) * 4 + 1];
        new_state[i * 4 + 2] = state[((i + 2) % 4) * 4 + 2];
        new_state[i * 4 + 3] = state[((i + 3) % 4) * 4 + 3];
    }

    memcpy(state, new_state, 16);
}

static void
aes_mix_columns(uint8_t state[16])
{
    for (unsigned int c = 0; c < 4; c++) {
        const uint8_t s0 = state[c * 4 + 0];
        const uint8_t s1 = state[c * 4 + 1];
        const uint8_t s2 = state[c * 4 + 2];
        const uint8_t s3 = state[c * 4 + 3];
        uint8_t       tmp[4];

        tmp[0] = bf8_mul(s0, 0x02) ^ bf8_mul(s1, 0x03) ^ s2 ^ s3;
        tmp[1] = s0 ^ bf8_mul(s1, 0x02) ^ bf8_mul(s2, 0x03) ^ s3;
        tmp[2] = s0 ^ s1 ^ bf8_mul(s2, 0x02) ^ bf8_mul(s3, 0x03);
        tmp[3] = bf8_mul(s0, 0x03) ^ s1 ^ s2 ^ bf8_mul(s3, 0x02);

        memcpy(&state[c * 4], tmp, sizeof(tmp));
    }
}
#endif

static void
aes_round(uint8_t state[16], const uint8_t round_key[16])
{
#ifdef SBOX_TABLE
    aes_build_ttables();
    aes_round_ttable(state, round_key);
#else
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_mix_columns(state);
    for (int i = 0; i < 16; i++) {
        state[i] ^= round_key[i];
    }
#endif
}

void
vistrutah_aes_round_128(uint8_t state[16], const uint8_t round_key[16])
{
    aes_round(state, round_key);
}

#ifndef SBOX_TABLE
static void
aes_round_zero(uint8_t state[16])
{
    aes_round(state, VISTRUTAH_ZERO);
}
#endif

static void
aes_final_round(uint8_t state[16], const uint8_t round_key[16])
{
#ifdef SBOX_TABLE
    aes_build_ttables();
    aes_final_round_ttable(state, round_key);
#else
    aes_sub_bytes(state);
    aes_shift_rows(state);
    for (int i = 0; i < 16; i++) {
        state[i] ^= round_key[i];
    }
#endif
}

void
vistrutah_aes_final_round_128(uint8_t state[16], const uint8_t round_key[16])
{
    aes_final_round(state, round_key);
}

#ifdef SBOX_TABLE
#define aes_round_encrypt aes_round_ttable
#define aes_round_zero_encrypt aes_round_ttable_zero
#define aes_final_round_encrypt aes_final_round_ttable
#else
#define aes_round_encrypt aes_round
#define aes_round_zero_encrypt aes_round_zero
#define aes_final_round_encrypt aes_final_round
#endif

static inline void
vistrutah_rotate16_left_5(uint8_t data[16])
{
    const uint8_t d0  = data[0];
    const uint8_t d1  = data[1];
    const uint8_t d2  = data[2];
    const uint8_t d3  = data[3];
    const uint8_t d4  = data[4];
    const uint8_t d5  = data[5];
    const uint8_t d6  = data[6];
    const uint8_t d7  = data[7];
    const uint8_t d8  = data[8];
    const uint8_t d9  = data[9];
    const uint8_t d10 = data[10];
    const uint8_t d11 = data[11];
    const uint8_t d12 = data[12];
    const uint8_t d13 = data[13];
    const uint8_t d14 = data[14];
    const uint8_t d15 = data[15];

    data[0]  = d5;
    data[1]  = d6;
    data[2]  = d7;
    data[3]  = d8;
    data[4]  = d9;
    data[5]  = d10;
    data[6]  = d11;
    data[7]  = d12;
    data[8]  = d13;
    data[9]  = d14;
    data[10] = d15;
    data[11] = d0;
    data[12] = d1;
    data[13] = d2;
    data[14] = d3;
    data[15] = d4;
}

static inline void
vistrutah_rotate16_left_10(uint8_t data[16])
{
    const uint8_t d0  = data[0];
    const uint8_t d1  = data[1];
    const uint8_t d2  = data[2];
    const uint8_t d3  = data[3];
    const uint8_t d4  = data[4];
    const uint8_t d5  = data[5];
    const uint8_t d6  = data[6];
    const uint8_t d7  = data[7];
    const uint8_t d8  = data[8];
    const uint8_t d9  = data[9];
    const uint8_t d10 = data[10];
    const uint8_t d11 = data[11];
    const uint8_t d12 = data[12];
    const uint8_t d13 = data[13];
    const uint8_t d14 = data[14];
    const uint8_t d15 = data[15];

    data[0]  = d10;
    data[1]  = d11;
    data[2]  = d12;
    data[3]  = d13;
    data[4]  = d14;
    data[5]  = d15;
    data[6]  = d0;
    data[7]  = d1;
    data[8]  = d2;
    data[9]  = d3;
    data[10] = d4;
    data[11] = d5;
    data[12] = d6;
    data[13] = d7;
    data[14] = d8;
    data[15] = d9;
}

void
vistrutah_rotate_bytes(uint8_t* data, int shift, int len)
{
    if (len == 16) {
        if (shift == 5) {
            vistrutah_rotate16_left_5(data);
            return;
        }
        if (shift == 10) {
            vistrutah_rotate16_left_10(data);
            return;
        }
    }

    uint8_t temp[64];
    memcpy(temp, data, len);
    for (int i = 0; i < len; i++) {
        data[i] = temp[(i + shift) % len];
    }
}

static inline void
vistrutah_mix_vzip_64(uint8_t state[64])
{
    static const uint8_t row_order[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15 };
    uint8_t              temp[64];

    memcpy(temp, state, 64);
    for (int i = 0; i < 16; ++i) {
        const int src = row_order[i];
        const int dst = i << 2;
        state[dst + 0] = temp[src + 0];
        state[dst + 1] = temp[src + 16];
        state[dst + 2] = temp[src + 32];
        state[dst + 3] = temp[src + 48];
    }
}

void
vistrutah_mixing_layer_512(uint8_t state[64])
{
    vistrutah_mix_vzip_64(state);
}

void
vistrutah_shuffle_key(uint8_t fixed_key[64])
{
    uint8_t temp[32];
    memcpy(temp, fixed_key + 32, 32);
    for (int i = 0; i < 32; i++) {
        fixed_key[32 + i] = temp[VISTRUTAH_KEXP_SHUFFLE[i]];
    }
}

void
vistrutah_512_prepare_key(vistrutah_512_prepared_key_t* prepared, const uint8_t* key)
{
#ifdef SBOX_TABLE
    aes_build_ttables();
#endif

    memcpy(prepared->fixed_key, key, 64);
    vistrutah_shuffle_key(prepared->fixed_key);

    memcpy(prepared->round_key, prepared->fixed_key + 16, 16);
    memcpy(prepared->round_key + 16, prepared->fixed_key, 16);
    memcpy(prepared->round_key + 32, prepared->fixed_key + 48, 16);
    memcpy(prepared->round_key + 48, prepared->fixed_key + 32, 16);
}

void
vistrutah_512_encrypt_prepared(const uint8_t* plaintext, uint8_t* ciphertext,
                               const vistrutah_512_prepared_key_t* prepared)
{
    uint8_t       state[64];
    uint8_t       round_key[64];
    const uint8_t* fixed_key = prepared->fixed_key;
    const int     steps     = VISTRUTAH_512_ROUNDS_LONG_512KEY / ROUNDS_PER_STEP;

    memcpy(state, plaintext, 64);
    memcpy(round_key, prepared->round_key, 64);

    for (int i = 0; i < 64; i++) {
        state[i] ^= round_key[i];
    }

    aes_round_encrypt(state, fixed_key);
    aes_round_encrypt(state + 16, fixed_key + 16);
    aes_round_encrypt(state + 32, fixed_key + 32);
    aes_round_encrypt(state + 48, fixed_key + 48);

    for (int i = 1; i < steps; i++) {
        aes_round_zero_encrypt(state);
        aes_round_zero_encrypt(state + 16);
        aes_round_zero_encrypt(state + 32);
        aes_round_zero_encrypt(state + 48);

        vistrutah_mixing_layer_512(state);

        vistrutah_rotate_bytes(round_key, 5, 16);
        vistrutah_rotate_bytes(round_key + 16, 10, 16);
        vistrutah_rotate_bytes(round_key + 32, 5, 16);
        vistrutah_rotate_bytes(round_key + 48, 10, 16);

        for (int j = 0; j < 64; j++) {
            state[j] ^= round_key[j];
        }
        for (int j = 0; j < 16; j++) {
            state[j] ^= ROUND_CONSTANTS[16 * (i - 1) + j];
        }

        aes_round_encrypt(state, fixed_key);
        aes_round_encrypt(state + 16, fixed_key + 16);
        aes_round_encrypt(state + 32, fixed_key + 32);
        aes_round_encrypt(state + 48, fixed_key + 48);
    }

    vistrutah_rotate_bytes(round_key, 5, 16);
    vistrutah_rotate_bytes(round_key + 16, 10, 16);
    vistrutah_rotate_bytes(round_key + 32, 5, 16);
    vistrutah_rotate_bytes(round_key + 48, 10, 16);

    aes_final_round_encrypt(state, round_key);
    aes_final_round_encrypt(state + 16, round_key + 16);
    aes_final_round_encrypt(state + 32, round_key + 32);
    aes_final_round_encrypt(state + 48, round_key + 48);

    memcpy(ciphertext, state, 64);
}

void
vistrutah_512_encrypt_prepared_batch_same_plaintext(
    const uint8_t* plaintext, const vistrutah_512_prepared_key_t* prepared, size_t lane_count,
    uint8_t* out_base, size_t out_stride)
{
    if (lane_count == 0) {
        return;
    }
    if (lane_count > 4u) {
        lane_count = 4u;
    }

    for (size_t lane = 0; lane < lane_count; ++lane) {
        vistrutah_512_encrypt_prepared(plaintext, out_base + lane * out_stride, &prepared[lane]);
    }
}

void
vistrutah_512_encrypt(const uint8_t* plaintext, uint8_t* ciphertext, const uint8_t* key)
{
    vistrutah_512_prepared_key_t prepared;

    vistrutah_512_prepare_key(&prepared, key);
    vistrutah_512_encrypt_prepared(plaintext, ciphertext, &prepared);
}
