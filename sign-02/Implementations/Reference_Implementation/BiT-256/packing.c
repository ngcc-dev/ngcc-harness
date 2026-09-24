/*
 * Copyright (c) 2026, Hang Zhang
 */

#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void pack_bits(unsigned char *r, size_t rlen, const int32_t *coeffs,
                      size_t n, unsigned int bits) {
    const uint32_t mask = (1U << bits) - 1U;
    size_t bitpos = 0;

    memset(r, 0, rlen);
    for (size_t i = 0; i < n; i++) {
        uint32_t v = (uint32_t)coeffs[i] & mask;
        size_t bytepos = bitpos >> 3;
        unsigned int offset = (unsigned int)(bitpos & 7);

        r[bytepos] |= (unsigned char)(v << offset);
        if (offset + bits > 8) {
            r[bytepos + 1] |= (unsigned char)(v >> (8 - offset));
            if (offset + bits > 16) {
                r[bytepos + 2] |= (unsigned char)(v >> (16 - offset));
            }
        }
        bitpos += bits;
    }
}

static void unpack_bits(int32_t *coeffs, const unsigned char *a, size_t n,
                        unsigned int bits, int sign_extend) {
    const uint32_t mask = (1U << bits) - 1U;
    size_t bitpos = 0;

    for (size_t i = 0; i < n; i++) {
        size_t bytepos = bitpos >> 3;
        unsigned int offset = (unsigned int)(bitpos & 7);
        uint32_t v = (uint32_t)a[bytepos] >> offset;

        if (offset + bits > 8) {
            v |= (uint32_t)a[bytepos + 1] << (8 - offset);
            if (offset + bits > 16) {
                v |= (uint32_t)a[bytepos + 2] << (16 - offset);
            }
        }
        v &= mask;
        if (sign_extend && (v & (1U << (bits - 1)))) {
            coeffs[i] = (int32_t)(v | ~mask);
        } else {
            coeffs[i] = (int32_t)v;
        }
        bitpos += bits;
    }
}

static void pack_poly_b1(unsigned char *r, const poly *a) {
    pack_bits(r, BIT_POLY_B1_BYTES, a->coeffs, BIT_N, BIT_POLY_B1_BITS);
}

static void unpack_poly_b1(poly *r, const unsigned char *a) {
    unpack_bits(r->coeffs, a, BIT_N, BIT_POLY_B1_BITS, 0);
}

static void pack_poly_b0(unsigned char *r, const poly *a) {
    pack_bits(r, BIT_POLY_B0_BYTES, a->coeffs, BIT_N, BIT_POLY_B0_BITS);
}

static void unpack_poly_b0(poly *r, const unsigned char *a) {
    unpack_bits(r->coeffs, a, BIT_N, BIT_POLY_B0_BITS, 1);
}

static void pack_poly_z0(unsigned char *r, const poly *a) {
    pack_bits(r, BIT_POLY_Z0_BYTES, a->coeffs, BIT_N, BIT_POLY_Z0_BITS);
}

static void unpack_poly_z0(poly *r, const unsigned char *a) {
    unpack_bits(r->coeffs, a, BIT_N, BIT_POLY_Z0_BITS, 1);
}

static void pack_poly_z1(unsigned char *r, const poly *a) {
    pack_bits(r, BIT_POLY_Z1_BYTES, a->coeffs, BIT_N, BIT_POLY_Z1_BITS);
}

static void unpack_poly_z1(poly *r, const unsigned char *a) {
    unpack_bits(r->coeffs, a, BIT_N, BIT_POLY_Z1_BITS, 1);
}

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
            h->vec[i].coeffs[j] = (int32_t)v;
        }
    }
    return 0;
}

void pack_pk(unsigned char *pk, const unsigned char *seed_A, const polyveck *b1) {
    memcpy(pk, seed_A, BIT_SEEDBYTES);
    pk += BIT_SEEDBYTES;
    for (int i = 0; i < BIT_K; i++) {
        pack_poly_b1(pk + i * BIT_POLY_B1_BYTES, &b1->vec[i]);
    }
}

void pack_sk(unsigned char *sk, const unsigned char *seed_A, const polyveck *b1,
             const unsigned char *secret_seed, const unsigned char *tr,
             const polyvecl *s_0, const polyveck *e, const polyveck *b0) {
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
        const int32_t *coeffs = s_0->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)((coeffs[j] & 0x03) |
                                    ((coeffs[j + 1] & 0x03) << 2) |
                                    ((coeffs[j + 2] & 0x03) << 4) |
                                    ((coeffs[j + 3] & 0x03) << 6));
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        const int32_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            *sk++ = (unsigned char)((coeffs[j] & 0x03) |
                                    ((coeffs[j + 1] & 0x03) << 2) |
                                    ((coeffs[j + 2] & 0x03) << 4) |
                                    ((coeffs[j + 3] & 0x03) << 6));
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        pack_poly_b0(sk + i * BIT_POLY_B0_BYTES, &b0->vec[i]);
    }
}

void unpack_sk(unsigned char *seed_A, polyveck *b1, unsigned char *secret_seed,
               unsigned char *tr, polyvecl *s_0, polyveck *e, polyveck *b0,
               const unsigned char *sk) {
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
        int32_t *coeffs = s_0->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j] = (int32_t)((uint32_t)(byte & 0x03) << 30) >> 30;
            coeffs[j + 1] = (int32_t)((uint32_t)((byte >> 2) & 0x03) << 30) >> 30;
            coeffs[j + 2] = (int32_t)((uint32_t)((byte >> 4) & 0x03) << 30) >> 30;
            coeffs[j + 3] = (int32_t)((uint32_t)((byte >> 6) & 0x03) << 30) >> 30;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        int32_t *coeffs = e->vec[i].coeffs;
        for (int j = 0; j < BIT_N; j += 4) {
            uint8_t byte = *sk++;
            coeffs[j] = (int32_t)((uint32_t)(byte & 0x03) << 30) >> 30;
            coeffs[j + 1] = (int32_t)((uint32_t)((byte >> 2) & 0x03) << 30) >> 30;
            coeffs[j + 2] = (int32_t)((uint32_t)((byte >> 4) & 0x03) << 30) >> 30;
            coeffs[j + 3] = (int32_t)((uint32_t)((byte >> 6) & 0x03) << 30) >> 30;
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

void pack_sig(unsigned char *sig, const polyvecm1 *z1, const polyveck *h,
              const unsigned char *challenge) {
    memcpy(sig, challenge, BIT_CHALLENGEBYTES);
    sig += BIT_CHALLENGEBYTES;

    pack_poly_z0(sig, &z1->vec[0]);
    sig += BIT_POLY_Z0_BYTES;

    for (int i = 0; i < BIT_L; i++) {
        pack_poly_z1(sig + i * BIT_POLY_Z1_BYTES, &z1->vec[i + 1]);
    }
    sig += BIT_POLYVECL_Z1_BYTES;

    pack_polyveck_h_bits(sig, h);
    sig += BIT_POLYVECK_H_BYTES;
}

int unpack_sig(polyvecm1 *z1, polyveck *h, unsigned char *challenge,
                const unsigned char *sig) {
    memcpy(challenge, sig, BIT_CHALLENGEBYTES);
    sig += BIT_CHALLENGEBYTES;

    unpack_poly_z0(&z1->vec[0], sig);
    sig += BIT_POLY_Z0_BYTES;

    for (int i = 0; i < BIT_L; i++) {
        unpack_poly_z1(&z1->vec[i + 1], sig + i * BIT_POLY_Z1_BYTES);
    }
    sig += BIT_POLYVECL_Z1_BYTES;

    if (unpack_polyveck_h_bits(h, sig) != 0) return -1;
    sig += BIT_POLYVECK_H_BYTES;
    return 0;
}
