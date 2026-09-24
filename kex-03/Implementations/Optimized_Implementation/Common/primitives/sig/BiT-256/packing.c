/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static void pack_bits(unsigned char *r, size_t rlen, const int32_t *coeffs,
                      size_t n, unsigned int bits) {
    /* Same LSB-first layout as before, but stream whole bytes out of a 64-bit
     * accumulator — no per-coefficient read-modify-write to r[] and no memset,
     * removing the store→load dependency chain that dominated this loop. */
    const uint32_t mask = (1U << bits) - 1U;
    uint64_t acc = 0;
    unsigned int accbits = 0;
    size_t ri = 0;

    for (size_t i = 0; i < n; i++) {
        acc |= (uint64_t)((uint32_t)coeffs[i] & mask) << accbits;
        accbits += bits;
        while (accbits >= 8) {
            r[ri++] = (unsigned char)acc;
            acc >>= 8;
            accbits -= 8;
        }
    }
    if (accbits > 0)
        r[ri++] = (unsigned char)acc;
    (void)rlen;
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

#ifndef BIT_HINT_BITPACK

#define H3_BITS        8
#define H3_DIV_M       171U
#define H3_DIV_SHIFT   9
#define H3_OV_SHIFT    (BIT_H_OVERFLOW_MAX * BIT_H_OVERFLOW_BITS)

static const uint8_t h3_pow[5] = {1, 3, 9, 27, 81};

static inline void h3_divmod(uint8_t *q, uint8_t *r, uint8_t b) {
    uint8_t quot = (uint8_t)(((uint16_t)b * H3_DIV_M) >> H3_DIV_SHIFT);
    *r = b - quot * 3;
    *q = quot;
}

static void pack_polyveck_h_base3(unsigned char *out, const polyveck *h,
                                   unsigned char overflow_buf[BIT_POLYVECK_H_OVERFLOW_BYTES]) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;
    uint8_t overflow_count = 0;
    uint16_t ov_entries[BIT_H_OVERFLOW_MAX] = {0};

    for (int i = 0; i < BIT_K; i++) {
        for (int g = 0; g < BIT_POLY_H_GROUPS; g++) {
            uint16_t base3 = 0;
            for (int k = 0; k < BIT_H_PACK_GROUP; k++) {
                int idx = g * BIT_H_PACK_GROUP + k;
                if (idx >= BIT_N) break;
                int32_t v = h->vec[i].coeffs[idx];
                int av = v < 0 ? -v : v;
                if (av == 2) {
                    if (overflow_count < BIT_H_OVERFLOW_MAX) {
                        ov_entries[overflow_count++] = (uint16_t)((uint16_t)(i * BIT_N + idx)
                                                   | ((v < 0) ? 0x800U : 0));
                    }
                    v = (v < 0) ? -1 : 1;
                }
                base3 += (uint16_t)(v + 1) * h3_pow[k];
            }
            acc |= (uint64_t)base3 << bits;
            bits += H3_BITS;
            if (bits >= 64) {
                store64_le(out + wi * 8, acc);
                wi++;
                acc = base3 >> (64 - (bits - H3_BITS));
                bits -= 64;
            }
        }
    }
    if (bits) store64_le_n(out + wi * 8, acc, (bits + 7) / 8);

    for (int e = 0; e < BIT_H_OVERFLOW_MAX; e++)
        store16_le(overflow_buf + 2 * e, ov_entries[e]);
    overflow_buf[2 * BIT_H_OVERFLOW_MAX] = overflow_count;
}

static int unpack_polyveck_h_base3(polyveck *h, const unsigned char *in,
                                     const unsigned char overflow_buf[BIT_POLYVECK_H_OVERFLOW_BYTES]) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;
    uint8_t overflow_count = overflow_buf[2 * BIT_H_OVERFLOW_MAX];

    if (overflow_count > BIT_H_OVERFLOW_MAX) return -1;

    for (int i = 0; i < BIT_K; i++) {
        for (int g = 0; g < BIT_POLY_H_GROUPS; g++) {
            if (bits < H3_BITS) {
                size_t offset = (size_t)wi * 8;
                size_t nvalid = BIT_POLYVECK_H_BASE3_BYTES - offset;
                if (nvalid >= 8) {
                    acc = load64_le(in + offset);
                } else {
                    uint64_t tmp = 0;
                    for (size_t b = 0; b < nvalid; b++)
                        tmp |= (uint64_t)in[offset + b] << (b * 8);
                    acc = tmp;
                }
                wi++; bits = 64;
            }
            uint8_t base3 = (uint8_t)(acc & ((1U << H3_BITS) - 1));
            acc >>= H3_BITS;
            bits -= H3_BITS;

            int idx_base = g * BIT_H_PACK_GROUP;
            uint8_t b = base3, q, r;
            for (int k = 0; k < BIT_H_PACK_GROUP && idx_base + k < BIT_N; k++) {
                h3_divmod(&q, &r, b);
                h->vec[i].coeffs[idx_base + k] = (int32_t)(r - 1);
                b = q;
            }
        }
    }

    for (int e = 0; e < overflow_count && e < BIT_H_OVERFLOW_MAX; e++) {
        uint16_t entry = load16_le(overflow_buf + 2 * e);
        uint16_t flat_idx = entry & 0x7FFU;
        h->vec[flat_idx >> 9].coeffs[flat_idx & 0x1FF] =
            (entry & 0x800U) ? (int32_t)(-2) : (int32_t)2;
    }

    /* padding check: entries beyond overflow_count must be zero */
    for (int e = overflow_count; e < BIT_H_OVERFLOW_MAX; e++) {
        if (load16_le(overflow_buf + 2 * e) != 0) return -1;
    }

    return 0;
}

#endif /* !BIT_HINT_BITPACK */

#ifdef BIT_HINT_BITPACK
/* Fixed-width bit packing: store (h + BIT_H_INF) as BIT_POLY_H_BITS-bit codes. */
static void pack_polyveck_h_bits(unsigned char *out, const polyveck *h) {
    uint64_t acc = 0;
    int bits = 0;
    size_t wi = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            uint32_t code = (uint32_t)(h->vec[i].coeffs[j] + BIT_H_INF);
            acc |= (uint64_t)(code & ((1U << BIT_POLY_H_BITS) - 1)) << bits;
            bits += BIT_POLY_H_BITS;
            while (bits >= 8) {
                out[wi++] = (uint8_t)acc;
                acc >>= 8;
                bits -= 8;
            }
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
            while (bits < BIT_POLY_H_BITS) {
                acc |= (uint64_t)in[ri++] << bits;
                bits += 8;
            }
            int code = (int)(acc & ((1U << BIT_POLY_H_BITS) - 1));
            acc >>= BIT_POLY_H_BITS;
            bits -= BIT_POLY_H_BITS;
            int v = code - BIT_H_INF;
            if (v > BIT_H_INF) return -1;
            h->vec[i].coeffs[j] = (int32_t)v;
        }
    }
    return 0;
}
#endif /* BIT_HINT_BITPACK */

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

#ifdef BIT_HINT_BITPACK
    pack_polyveck_h_bits(sig, h);
#else
    {
        unsigned char overflow_buf[BIT_POLYVECK_H_OVERFLOW_BYTES];
        pack_polyveck_h_base3(sig, h, overflow_buf);
        sig += BIT_POLYVECK_H_BASE3_BYTES;
        memcpy(sig, overflow_buf, BIT_POLYVECK_H_OVERFLOW_BYTES);
        sig += BIT_POLYVECK_H_OVERFLOW_BYTES;
    }
#endif
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

#ifdef BIT_HINT_BITPACK
    if (unpack_polyveck_h_bits(h, sig) != 0) return -1;
#else
    if (unpack_polyveck_h_base3(h, sig, sig + BIT_POLYVECK_H_BASE3_BYTES) != 0) return -1;
#endif
    sig += BIT_POLYVECK_H_BYTES;
    return 0;

}
