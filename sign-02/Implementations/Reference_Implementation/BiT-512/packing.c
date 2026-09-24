/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Base-3 hint encoding (no overflow, |h| ≤ 1) */
#define H3_BITS        8
#define H3_DIV_M       171U
#define H3_DIV_SHIFT   9
static const uint8_t h3_pow[5] = {1, 3, 9, 27, 81};
static inline void h3_divmod(uint8_t *q, uint8_t *r, uint8_t b) {
    uint8_t quot = (uint8_t)(((uint16_t)b * H3_DIV_M) >> H3_DIV_SHIFT);
    *r = b - quot * 3;
    *q = quot;
}

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

static void pack_poly_h(unsigned char *r, const poly *a) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;

    for (int g = 0; g < BIT_POLY_H_GROUPS; g++) {
        uint16_t base3 = 0;
        for (int k = 0; k < BIT_H_PACK_GROUP; k++) {
            int idx = g * BIT_H_PACK_GROUP + k;
            int32_t v = (idx < BIT_N) ? a->coeffs[idx] : 0;
            base3 += (uint16_t)(v + 1) * h3_pow[k];
        }
        acc |= (uint64_t)base3 << bits;
        bits += H3_BITS;
        if (bits >= 64) {
            store64_le(r + wi * 8, acc);
            wi++;
            acc = base3 >> (64 - (bits - H3_BITS));
            bits -= 64;
        }
    }
    if (bits) store64_le_n(r + wi * 8, acc, (bits + 7) / 8);
}

static int unpack_poly_h(poly *r, const unsigned char *a) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;

    for (int g = 0; g < BIT_POLY_H_GROUPS; g++) {
        if (bits < H3_BITS) {
            /* bounded tail read: load64_le always reads 8 bytes,
             * but the hint byte-count may not be a multiple of 8.
             * Read only the remaining valid bytes, zero the rest. */
            size_t offset = (size_t)wi * 8;
            size_t nvalid = BIT_POLY_H_BASE3_BYTES - offset;
            if (nvalid >= 8) {
                acc = load64_le(a + offset);
            } else {
                uint64_t tmp = 0;
                for (size_t b = 0; b < nvalid; b++)
                    tmp |= (uint64_t)a[offset + b] << (b * 8);
                acc = tmp;
            }
            wi++;
            bits = 64;
        }
        uint8_t base3 = (uint8_t)(acc & ((1U << H3_BITS) - 1));
        acc >>= H3_BITS;
        bits -= H3_BITS;

        uint8_t b = base3, q, rv;
        for (int k = 0; k < BIT_H_PACK_GROUP; k++) {
            int idx = g * BIT_H_PACK_GROUP + k;
            if (idx >= BIT_N) break;
            h3_divmod(&q, &rv, b);
            r->coeffs[idx] = (int32_t)rv - 1;
            b = q;
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

    for (int i = 0; i < BIT_K; i++) {
        pack_poly_h(sig, &h->vec[i]);
        sig += BIT_POLY_H_BASE3_BYTES;
    }
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

    for (int i = 0; i < BIT_K; i++) {
        if (unpack_poly_h(&h->vec[i], sig) != 0) return -1;
        sig += BIT_POLY_H_BASE3_BYTES;
    }
    return 0;
}
