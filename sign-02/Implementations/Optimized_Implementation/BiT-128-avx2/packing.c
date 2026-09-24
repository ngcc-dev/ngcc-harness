/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#include "packing.h"
#include "polyvec.h"
#include "endian.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>

#ifndef BIT_HINT_BITPACK
/* Base-3 hint encoding (same as ref) — kept for later size compression. */
#define H3_BITS        8
#define H3_DIV_M       171U
#define H3_DIV_SHIFT   9

static const uint8_t h3_pow[5] = {1, 3, 9, 27, 81};

static inline void h3_divmod(uint8_t *q, uint8_t *r, uint8_t b) {
    uint8_t quot = (uint8_t)(((uint16_t)b * H3_DIV_M) >> H3_DIV_SHIFT);
    *r = b - quot * 3;
    *q = quot;
}

static void pack_polyveck_h_base3(unsigned char *out, const polyveck *h,
                                   unsigned char ov_buf[BIT_POLYVECK_H_OVERFLOW_BYTES]) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;
    uint8_t overflow_count = 0;

    memset(ov_buf, 0, BIT_POLYVECK_H_OVERFLOW_BYTES);
    uint64_t ov_acc = 0;
    int ov_bits = 0, ov_wi = 1;

    for (int i = 0; i < BIT_K; i++) {
        for (int g = 0; g < BIT_POLY_H_GROUPS; g++) {
            uint16_t base3 = 0;
            for (int k = 0; k < BIT_H_PACK_GROUP; k++) {
                int idx = g * BIT_H_PACK_GROUP + k;
                if (idx >= BIT_N) break;
                int16_t v = h->vec[i].coeffs[idx];
                int av = v < 0 ? -v : v;
                if (av == 2) {
                    if (overflow_count < BIT_H_OVERFLOW_MAX) {
                        uint16_t entry = (uint16_t)((uint16_t)(i * BIT_N + idx)
                                          | ((v < 0) ? 0x400U : 0));
                        ov_acc |= (uint64_t)entry << ov_bits;
                        ov_bits += BIT_H_OVERFLOW_BITS;
                        while (ov_bits >= 8) {
                            ov_buf[ov_wi++] = (uint8_t)ov_acc;
                            ov_acc >>= 8;
                            ov_bits -= 8;
                        }
                        overflow_count++;
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

    while (ov_bits > 0) {
        ov_buf[ov_wi++] = (uint8_t)ov_acc;
        ov_acc >>= 8;
        ov_bits -= 8;
    }
    ov_buf[0] = overflow_count;
}

static int unpack_polyveck_h_base3(polyveck *h, const unsigned char *in,
                                     const unsigned char ov_buf[BIT_POLYVECK_H_OVERFLOW_BYTES]) {
    uint64_t acc = 0;
    int bits = 0, wi = 0;

    uint8_t overflow_count = ov_buf[0];
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
            uint8_t b = base3, q, rv;
            for (int k = 0; k < BIT_H_PACK_GROUP && idx_base + k < BIT_N; k++) {
                h3_divmod(&q, &rv, b);
                h->vec[i].coeffs[idx_base + k] = (int16_t)(rv - 1);
                b = q;
            }
        }
    }

    {
        uint64_t ov_acc = 0;
        int ov_bits = 0, ov_rd = 1;
        for (int e = 0; e < overflow_count; e++) {
            while (ov_bits < BIT_H_OVERFLOW_BITS && ov_rd < BIT_POLYVECK_H_OVERFLOW_BYTES) {
                ov_acc |= (uint64_t)ov_buf[ov_rd++] << ov_bits;
                ov_bits += 8;
            }
            uint16_t entry = (uint16_t)(ov_acc & ((1U << BIT_H_OVERFLOW_BITS) - 1));
            ov_acc >>= BIT_H_OVERFLOW_BITS;
            ov_bits -= BIT_H_OVERFLOW_BITS;
            uint16_t flat_idx = entry & 0x3FFU;
            h->vec[flat_idx >> 8].coeffs[flat_idx & 0xFF] =
                (int16_t)((entry & 0x400U) ? -2 : 2);
        }
        if (ov_bits > 0 && (ov_acc & ((1ULL << ov_bits) - 1))) return -1;
        for (int i = ov_rd; i < BIT_POLYVECK_H_OVERFLOW_BYTES; i++)
            if (ov_buf[i]) return -1;
    }
    return 0;
}

#else  /* BIT_HINT_BITPACK */

/* Fixed-width bit packing: store (h + BIT_H_INF) as BIT_POLY_H_BITS-bit codes,
   K*N coefficients laid out contiguously.  ||h||_inf = BIT_H_INF = 3 is enforced
   upstream by check_reject_norm, so every written code is in [0, 2*BIT_H_INF]. */
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
            int v = code - BIT_H_INF;        /* 3-bit code -> [-3, 4] */
            if (v > BIT_H_INF) return -1;     /* reject |h| > 3 (all-ones code) */
            h->vec[i].coeffs[j] = (int16_t)v;
        }
    }
    return 0;
}

#endif /* BIT_HINT_BITPACK */

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
    uint64_t vals[64] __attribute__((aligned(32)));
    __m256i v_madd = _mm256_set1_epi32((1681 << 16) | 1);
    __m256i v_m2   = _mm256_set1_epi64x(2825761ULL);
    __m256i v_mask = _mm256_set1_epi64x(0xFFFFFFFFULL);

    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i a_vec = _mm256_load_si256((__m256i*)&a->coeffs[16*i]);
        __m256i p01 = _mm256_madd_epi16(a_vec, v_madd);
        __m256i v1  = _mm256_srli_epi64(p01, 32);
        __m256i v0  = _mm256_and_si256(p01, v_mask);
        __m256i val64 = _mm256_add_epi64(v0, _mm256_mul_epu32(v1, v_m2));
        _mm256_store_si256((__m256i*)&vals[4*i], val64);
    }

    uint64_t acc = 0; int bits = 0, wi = 0;
    for (int i = 0; i < 64; i++) {
        acc |= vals[i] << bits;
        bits += B1_PACK_BITS;
        if (bits >= 64) {
            store64_le(r + wi * 8, acc); wi++;
            acc = vals[i] >> (64 - (bits - B1_PACK_BITS));
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
            if (bits == 0) { acc_lo |= word; }
            else { acc_lo |= word << bits; acc_hi |= word >> (64 - bits); }
            bits += 64;
        }
        uint64_t val = acc_lo & ((1ULL << B1_PACK_BITS) - 1);
        acc_lo = (acc_lo >> B1_PACK_BITS) | (acc_hi << (64 - B1_PACK_BITS));
        acc_hi >>= B1_PACK_BITS; bits -= B1_PACK_BITS;

        uint32_t v0 = val % 2825761ULL;
        uint32_t v1 = val / 2825761ULL;
        r->coeffs[4*i  ] = (int16_t)(v0 % 1681);
        r->coeffs[4*i+1] = (int16_t)(v0 / 1681);
        r->coeffs[4*i+2] = (int16_t)(v1 % 1681);
        r->coeffs[4*i+3] = (int16_t)(v1 / 1681);
    }
}

static void pack_poly_z1(unsigned char *r, const poly *a) {
    __m256i mask_lo  = _mm256_set1_epi64x(0x00000000FFFFFFFFULL);
    __m256i v_madd   = _mm256_set1_epi32(0x08000001);
    __m256i v_mask11 = _mm256_set1_epi16(0x07FF);

    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i a_vec = _mm256_load_si256((__m256i*)&a->coeffs[16*i]);
        a_vec = _mm256_and_si256(a_vec, v_mask11);
        __m256i p01 = _mm256_madd_epi16(a_vec, v_madd);
        __m256i P0  = _mm256_and_si256(p01, mask_lo);
        __m256i P1  = _mm256_srli_epi64(_mm256_andnot_si256(mask_lo, p01), 10);
        __m256i Q02 = _mm256_or_si256(P0, P1);
        __m256i Q2s = _mm256_slli_epi64(_mm256_srli_si256(Q02, 8), 44);
        __m256i L   = _mm256_or_si256(Q02, Q2s);
        __m256i H   = _mm256_srli_epi64(Q02, 20);
        __m256i Fin = _mm256_blend_epi32(L, H, 0xCC);
        __m128i h0 = _mm256_castsi256_si128(Fin);
        __m128i h1 = _mm256_extracti128_si256(Fin, 1);
        _mm_storel_epi64((__m128i*)(r + 22*i), h0);
        uint32_t v0h = _mm_extract_epi32(h0, 2);
        r[22*i+8]=v0h; r[22*i+9]=v0h>>8; r[22*i+10]=v0h>>16;
        _mm_storel_epi64((__m128i*)(r + 22*i + 11), h1);
        uint32_t v1h = _mm_extract_epi32(h1, 2);
        r[22*i+19]=v1h; r[22*i+20]=v1h>>8; r[22*i+21]=v1h>>16;
    }
}

static void unpack_poly_z1(poly *r, const unsigned char *a) {
    __m256i shuf = _mm256_set_epi8(
        12,11,10,9, 11,10,9,8, 9,8,7,6, 8,7,6,5,
        7,6,5,4,   5,4,3,2,  4,3,2,1, 3,2,1,0);
    __m256i shift = _mm256_set_epi32(5,2,7,4, 1,6,3,0);
    for (int i = 0; i < BIT_N / 16; i++) {
        __m128i b0 = _mm_loadu_si128((__m128i*)(a + 22*i));
        __m128i b1 = _mm_loadu_si128((__m128i*)(a + 22*i + 11));
        __m256i v0 = _mm256_broadcastsi128_si256(b0);
        v0 = _mm256_srlv_epi32(_mm256_shuffle_epi8(v0, shuf), shift);
        v0 = _mm256_srai_epi32(_mm256_slli_epi32(v0, 21), 21);
        __m256i v1 = _mm256_broadcastsi128_si256(b1);
        v1 = _mm256_srlv_epi32(_mm256_shuffle_epi8(v1, shuf), shift);
        v1 = _mm256_srai_epi32(_mm256_slli_epi32(v1, 21), 21);
        __m256i pk = _mm256_packs_epi32(v0, v1);
        pk = _mm256_permute4x64_epi64(pk, _MM_SHUFFLE(3,1,2,0));
        _mm256_store_si256((__m256i*)&r->coeffs[16*i], pk);
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

#ifdef BIT_HINT_BITPACK
    pack_polyveck_h_bits(sig, h);
#else
    pack_polyveck_h_base3(sig, h, sig + BIT_POLYVECK_H_BASE3_BYTES);
#endif
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
