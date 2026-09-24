/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <string.h>

static void pack_polyveck_h_bits(unsigned char *out, const polyveck *h) {
    uint64_t acc = 0;
    int bits = 0;
    size_t wi = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            uint32_t code = (uint32_t)(h->vec[i].coeffs[j] + BIT_H_INF);
            acc |= (uint64_t)(code & ((1U << BIT_POLY_H_BITS) - 1)) << bits;
            bits += BIT_POLY_H_BITS;
            while (bits >= 8) { out[wi++] = (uint8_t)acc; acc >>= 8; bits -= 8; }
        }
    }
    if (bits) out[wi++] = (uint8_t)acc;
}

static int unpack_polyveck_h_bits(polyveck *h, const unsigned char *in) {
    uint64_t acc = 0;
    int bits = 0;
    size_t ri = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            while (bits < BIT_POLY_H_BITS) { acc |= (uint64_t)in[ri++] << bits; bits += 8; }
            int code = (int)(acc & ((1U << BIT_POLY_H_BITS) - 1));
            acc >>= BIT_POLY_H_BITS; bits -= BIT_POLY_H_BITS;
            int v = code - BIT_H_INF;
            if (v > BIT_H_INF) return -1;
            h->vec[i].coeffs[j] = (int16_t)v;
        }
    }
    return 0;
}

/* ---- b1: 4 coeffs in [0,1680] -> 43-bit base-1681*/
#define B1_PACK       1681U
#define B1_PACK_2     2825761ULL
#define B1_PACK_3     4750104241ULL
#define B1_PACK_BITS  43
#define B1_PACK_DIV_M 42865909600196ULL

/* Portable 64x64->128 multiply. Works on 32-bit platforms (no __uint128_t). */
static inline void mul64_128(uint64_t *lo, uint64_t *hi, uint64_t a, uint64_t b) {
    uint32_t al = (uint32_t)a, ah = (uint32_t)(a >> 32);
    uint32_t bl = (uint32_t)b, bh = (uint32_t)(b >> 32);
    uint64_t p0 = (uint64_t)al * bl;
    uint64_t p1 = (uint64_t)al * bh;
    uint64_t p2 = (uint64_t)ah * bl;
    uint64_t p3 = (uint64_t)ah * bh;
    uint64_t mid = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;
    *lo = (mid << 32) | (uint32_t)p0;
    *hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
}

/* Barrett remainder: x % B1_PACK  (x < 2^43) */
static inline uint64_t b1_barrett(uint64_t x) {
    uint64_t lo, hi;
    mul64_128(&lo, &hi, x, B1_PACK_DIV_M);
    uint64_t q = (hi << 8) | (lo >> 56);
    uint64_t r = x - q * B1_PACK;
    if (r >= B1_PACK) r -= B1_PACK;
    return r;
}

/* Barrett quotient: x / B1_PACK  (x < 2^43). No hardware division. */
static inline uint64_t b1_div(uint64_t x) {
    uint64_t lo, hi;
    mul64_128(&lo, &hi, x, B1_PACK_DIV_M);
    uint64_t q = (hi << 8) | (lo >> 56);
    uint64_t r = x - q * B1_PACK;
    if (r >= B1_PACK) { r -= B1_PACK; q++; }
    return q;
}

static void pack_poly_b1(unsigned char *r, const poly *a) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;

    for (int i = 0; i < BIT_N / 4; i++) {
        uint64_t val = (uint64_t)(uint16_t)a->coeffs[4*i]
                    + (uint64_t)(uint16_t)a->coeffs[4*i+1] * B1_PACK
                    + (uint64_t)(uint16_t)a->coeffs[4*i+2] * B1_PACK_2
                    + (uint64_t)(uint16_t)a->coeffs[4*i+3] * B1_PACK_3;

        acc |= val << bits;
        bits += B1_PACK_BITS;
        if (bits >= 64) {
            store64_le(r + wi * 8, acc);
            wi++;
            acc = val >> (64 - (bits - B1_PACK_BITS));
            bits -= 64;
        }
    }
    if (bits) store64_le_n(r + wi * 8, acc, (bits + 7) / 8);
}

static void unpack_poly_b1(poly *restrict r, const unsigned char *a) {
    uint64_t acc_lo = 0, acc_hi = 0;
    int bits = 0, wi = 0;

    for (int i = 0; i < BIT_N / 4; i++) {
        if (bits < B1_PACK_BITS) {
            uint64_t word = load64_le(a + wi * 8); wi++;
            if (bits == 0) {
                acc_lo |= word;
            } else {
                acc_lo |= word << bits;
                acc_hi |= word >> (64 - bits);
            }
            bits += 64;
        }
        uint64_t val = acc_lo & ((1ULL << B1_PACK_BITS) - 1);
        acc_lo = (acc_lo >> B1_PACK_BITS) | (acc_hi << (64 - B1_PACK_BITS));
        acc_hi >>= B1_PACK_BITS;
        bits -= B1_PACK_BITS;

        uint64_t x = val;
        r->coeffs[4*i  ] = (int16_t)b1_barrett(x); x = b1_div(x);
        r->coeffs[4*i+1] = (int16_t)b1_barrett(x); x = b1_div(x);
        r->coeffs[4*i+2] = (int16_t)b1_barrett(x); x = b1_div(x);
        r->coeffs[4*i+3] = (int16_t)b1_barrett(x);
    }
}

static void pack_poly_z1(unsigned char *r, const poly *a) {
    for (int i = 0; i < BIT_N / 8; i++) {
        uint16_t t[8];
        for (int j = 0; j < 8; j++) t[j] = (uint16_t)a->coeffs[8*i+j] & 0x07FF;
        r[ 0] = (uint8_t)( t[0] );                   r[ 1] = (uint8_t)((t[0]>>8)  | (t[1]<<3));
        r[ 2] = (uint8_t)((t[1]>>5)  | (t[2]<<6));   r[ 3] = (uint8_t)( t[2]>>2 );
        r[ 4] = (uint8_t)((t[2]>>10) | (t[3]<<1));   r[ 5] = (uint8_t)((t[3]>>7)  | (t[4]<<4));
        r[ 6] = (uint8_t)((t[4]>>4)  | (t[5]<<7));   r[ 7] = (uint8_t)( t[5]>>1 );
        r[ 8] = (uint8_t)((t[5]>>9)  | (t[6]<<2));   r[ 9] = (uint8_t)((t[6]>>6)  | (t[7]<<5));
        r[10] = (uint8_t)( t[7]>>3 );
        r += 11;
    }
}

static void pack_poly_b0(unsigned char *r, const poly *a) {
    for (int i = 0; i < BIT_N / 2; i++) {
        uint16_t t0 = (uint16_t)a->coeffs[2 * i + 0] & 0x000F;
        uint16_t t1 = (uint16_t)a->coeffs[2 * i + 1] & 0x000F;

        r[i] = (uint8_t)(t0 | (t1 << 4));
    }
}

static void unpack_poly_b0(poly *r, const unsigned char *a) {
    for (int i = 0; i < BIT_N / 2; i++) {
        uint16_t t0 = ((uint16_t)a[i] >> 0) & 0x000F;
        uint16_t t1 = ((uint16_t)a[i] >> 4) & 0x000F;

        r->coeffs[2 * i + 0] = (int16_t)(t0 << 12) >> 12;
        r->coeffs[2 * i + 1] = (int16_t)(t1 << 12) >> 12;
    }
}

void pack_pk(unsigned char *pk, const unsigned char *seed_A, const polyveck *b1) {
    memcpy(pk, seed_A, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;
    for (int i = 0; i < BIT_K; i++) {
        pack_poly_b1(pk + i * BIT_POLY_B1_BYTES, &b1->vec[i]);
    }
}

void pack_sk(unsigned char *sk, const unsigned char *seed_A, const polyveck *b1, const unsigned char *secret_seed, const unsigned char *tr, const polyvecl *s_0, const polyveck *e, const polyveck *b0) {
    memcpy(sk, seed_A, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    for (int i = 0; i < BIT_K; i++) {
        pack_poly_b1(sk + i * BIT_POLY_B1_BYTES, &b1->vec[i]);
    }
    sk += BIT_POLYVECK_B1_BYTES;
    memcpy(sk, secret_seed, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    memcpy(sk, tr, BIT_TRBYTES);
    sk += BIT_TRBYTES;
    for (int i = 0; i < BIT_L; ++i) {
        const int16_t *coeffs = s_0->vec[i].coeffs;
        for (int j =
            0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)(
                ( coeffs[j]   & 0x03)       |
                ((coeffs[j+1] & 0x03) << 2) |
                ((coeffs[j+2] & 0x03) << 4) |
                ((coeffs[j+3] & 0x03) << 6)
            );
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        const int16_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)(
                ( coeffs[j]   & 0x03)       |
                ((coeffs[j+1] & 0x03) << 2) |
                ((coeffs[j+2] & 0x03) << 4) |
                ((coeffs[j+3] & 0x03) << 6)
            );
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        pack_poly_b0(sk + i * BIT_POLY_B0_BYTES, &b0->vec[i]);
    }
}

void unpack_sk(unsigned char *seed_A, polyveck *b1, unsigned char *secret_seed, unsigned char *tr, polyvecl *s_0, polyveck *e, polyveck *b0, const unsigned char *sk) {
    memcpy(seed_A, sk, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    for (int i = 0; i < BIT_K; i++) {
        unpack_poly_b1(&b1->vec[i], sk + i * BIT_POLY_B1_BYTES);
    }
    sk += BIT_POLYVECK_B1_BYTES;
    memcpy(secret_seed, sk, BIT_SEEDBYTES);
    sk += BIT_SEEDBYTES;
    memcpy(tr, sk, BIT_TRBYTES);
    sk += BIT_TRBYTES;
    
    for (int i = 0; i < BIT_L; ++i) {
        int16_t *coeffs = s_0->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j]   = (int16_t)((uint16_t)( byte       & 0x03) << 14) >> 14;
            coeffs[j+1] = (int16_t)((uint16_t)((byte >> 2) & 0x03) << 14) >> 14;
            coeffs[j+2] = (int16_t)((uint16_t)((byte >> 4) & 0x03) << 14) >> 14;
            coeffs[j+3] = (int16_t)((uint16_t)((byte >> 6) & 0x03) << 14) >> 14;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        int16_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j]   = (int16_t)((uint16_t)( byte       & 0x03) << 14) >> 14;
            coeffs[j+1] = (int16_t)((uint16_t)((byte >> 2) & 0x03) << 14) >> 14;
            coeffs[j+2] = (int16_t)((uint16_t)((byte >> 4) & 0x03) << 14) >> 14;
            coeffs[j+3] = (int16_t)((uint16_t)((byte >> 6) & 0x03) << 14) >> 14;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        unpack_poly_b0(&b0->vec[i], sk + i * BIT_POLY_B0_BYTES);
    }
}

void unpack_pk(unsigned char *seed_A, polyveck *b1, const unsigned char *pk) {
    memcpy(seed_A, pk, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;
    
    for (int i = 0; i < BIT_K; i++) {
        unpack_poly_b1(&b1->vec[i], pk + i * BIT_POLY_B1_BYTES);
    }
}


void pack_sig(unsigned char *sig, const polyvecm1 *z1, const polyveck *h, const unsigned char *challenge) {

    for (int i = 0; i < BIT_CHALLENGEBYTES; i++) {
        sig[i] = challenge[i];
    }
    sig += BIT_CHALLENGEBYTES;

    for (int j = 0; j < BIT_N / 8; j++) {
        uint8_t c0,c1,c2,c3,c4,c5,c6,c7;
        c0 = (uint8_t)(z1->vec[0].coeffs[8*j+0] + 7);
        c1 = (uint8_t)(z1->vec[0].coeffs[8*j+1] + 7);
        c2 = (uint8_t)(z1->vec[0].coeffs[8*j+2] + 7);
        c3 = (uint8_t)(z1->vec[0].coeffs[8*j+3] + 7);
        c4 = (uint8_t)(z1->vec[0].coeffs[8*j+4] + 7);
        c5 = (uint8_t)(z1->vec[0].coeffs[8*j+5] + 7);
        c6 = (uint8_t)(z1->vec[0].coeffs[8*j+6] + 7);
        c7 = (uint8_t)(z1->vec[0].coeffs[8*j+7] + 7);
        sig[0] = c0       | (c1 << 4);
        sig[1] = c2       | (c3 << 4);
        sig[2] = c4       | (c5 << 4);
        sig[3] = c6       | (c7 << 4);
        sig += 4;
    }

    for (int i = 0; i < BIT_L; i++) {
        pack_poly_z1(sig + i * BIT_POLY_Z1_BYTES, &z1->vec[i + 1]);
    }
    sig += BIT_POLYVECL_Z1_BYTES;

    pack_polyveck_h_bits(sig, h);
    sig += BIT_POLYVECK_H_BYTES;
}


int unpack_sig(polyvecm1 *z1, polyveck *h, unsigned char *challenge, const unsigned char *sig) {

    for (int i = 0; i < BIT_CHALLENGEBYTES; i++) {
        challenge[i] = sig[i];
    }
    sig += BIT_CHALLENGEBYTES;

    for (int j = 0; j < BIT_N / 8; j++) {
        uint8_t s0=sig[0], s1=sig[1], s2=sig[2], s3=sig[3];
        z1->vec[0].coeffs[8*j+0] = (int16_t)( s0        & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+1] = (int16_t)((s0 >> 4)  & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+2] = (int16_t)( s1        & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+3] = (int16_t)((s1 >> 4)  & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+4] = (int16_t)( s2        & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+5] = (int16_t)((s2 >> 4)  & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+6] = (int16_t)( s3        & 0x0F) - 7;
        z1->vec[0].coeffs[8*j+7] = (int16_t)((s3 >> 4)  & 0x0F) - 7;
        sig += 4;
    }

    for (int i = 0; i < BIT_L; i++) {
        const unsigned char *a = sig + i * BIT_POLY_Z1_BYTES;

        for (int j = 0; j < BIT_N / 8; j++) {
            uint16_t t[8];

            t[0] = (uint16_t)a[0] | ((uint16_t)a[1] << 8);
            t[1] = (uint16_t)(a[1] >> 3) | ((uint16_t)a[2] << 5);
            t[2] = (uint16_t)(a[2] >> 6) | ((uint16_t)a[3] << 2) | ((uint16_t)a[4] << 10);
            t[3] = (uint16_t)(a[4] >> 1) | ((uint16_t)a[5] << 7);
            t[4] = (uint16_t)(a[5] >> 4) | ((uint16_t)a[6] << 4);
            t[5] = (uint16_t)(a[6] >> 7) | ((uint16_t)a[7] << 1) | ((uint16_t)a[8] << 9);
            t[6] = (uint16_t)(a[8] >> 2) | ((uint16_t)a[9] << 6);
            t[7] = (uint16_t)(a[9] >> 5) | ((uint16_t)a[10] << 3);
            a += 11;

            for (int k = 0; k < 8; k++) {
                z1->vec[i + 1].coeffs[8 * j + k] = (int16_t)((t[k] & 0x07FF) << 5) >> 5;
            }
        }
    }
    sig += BIT_POLYVECL_Z1_BYTES;

    if (unpack_polyveck_h_bits(h, sig) != 0) return -1;
    sig += BIT_POLYVECK_H_BYTES;
    return 0;
}
