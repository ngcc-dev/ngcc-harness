#ifndef gf9309_h__
#define gf9309_h__
#ifdef __cplusplus
extern "C" {
#endif
#include <sqisign_namespace.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

    typedef uint64_t digit_t;
    typedef union { struct {
        uint64_t v0;
        uint64_t v1;
        uint64_t v2;
        uint64_t v3;
        uint64_t v4;
    }; digit_t arr[5]; } gf9309;

    extern const gf9309 gf9309_ZERO;
    extern const gf9309 gf9309_ONE;
    extern const gf9309 gf9309_MINUS_ONE;

#if (defined _MSC_VER && defined _M_X64) || (defined __x86_64__ && (defined __GNUC__ || defined __clang__))
#include <immintrin.h>
#define inner_gf9309_adc(cc, a, b, d) _addcarry_u64(cc, a, b, (unsigned long long *)(void *)d)
#define inner_gf9309_sbb(cc, a, b, d) _subborrow_u64(cc, a, b, (unsigned long long *)(void *)d)
#else
static inline unsigned char inner_gf9309_adc(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a+(unsigned __int128)b+cc; *d=(uint64_t)t; return (unsigned char)(t>>64);}
static inline unsigned char inner_gf9309_sbb(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a-(unsigned __int128)b-cc; *d=(uint64_t)t; return (unsigned char)(-(uint64_t)(t>>64));}
#endif
#define inner_gf9309_umul(lo,hi,x,y) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf9309_umul_add(lo,hi,x,y,z) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf9309_umul_x2(lo,hi,x1,y1,x2,y2) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf9309_umul_x2_add(lo,hi,x1,y1,x2,y2,z) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)

    static inline void gf9309_add(gf9309 *d, const gf9309 *a, const gf9309 *b) {
        uint64_t d0, d1, d2, d3, d4, f; unsigned char cc;
        cc = inner_gf9309_adc(0, a->v0, b->v0, &d0);
        cc = inner_gf9309_adc(cc, a->v1, b->v1, &d1);
        cc = inner_gf9309_adc(cc, a->v2, b->v2, &d2);
        cc = inner_gf9309_adc(cc, a->v3, b->v3, &d3);
        cc = inner_gf9309_adc(cc, a->v4, b->v4, &d4);
        f = d4 >> 57;
        cc = inner_gf9309_adc(0, d0, f, &d0);
        cc = inner_gf9309_adc(cc, d1, 0, &d1);
        cc = inner_gf9309_adc(cc, d2, 0, &d2);
        cc = inner_gf9309_adc(cc, d3, 0, &d3);
        (void)inner_gf9309_adc(cc, d4, ((uint64_t)0x7F7 << 53) & -f, &d4);
        f = d4 >> 57;
        cc = inner_gf9309_adc(0, d0, f, &d0);
        cc = inner_gf9309_adc(cc, d1, 0, &d1);
        cc = inner_gf9309_adc(cc, d2, 0, &d2);
        cc = inner_gf9309_adc(cc, d3, 0, &d3);
        (void)inner_gf9309_adc(cc, d4, ((uint64_t)0x7F7 << 53) & -f, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void gf9309_sub(gf9309 *d, const gf9309 *a, const gf9309 *b) {
        uint64_t d0, d1, d2, d3, d4, m, f; unsigned char cc;
        cc = inner_gf9309_sbb(0, a->v0, b->v0, &d0);
        cc = inner_gf9309_sbb(cc, a->v1, b->v1, &d1);
        cc = inner_gf9309_sbb(cc, a->v2, b->v2, &d2);
        cc = inner_gf9309_sbb(cc, a->v3, b->v3, &d3);
        cc = inner_gf9309_sbb(cc, a->v4, b->v4, &d4);
        (void)inner_gf9309_sbb(cc, 0, 0, &m);
        cc = inner_gf9309_sbb(0, d0, m & 2, &d0);
        cc = inner_gf9309_sbb(cc, d1, 0, &d1);
        cc = inner_gf9309_sbb(cc, d2, 0, &d2);
        cc = inner_gf9309_sbb(cc, d3, 0, &d3);
        (void)inner_gf9309_sbb(cc, d4, ((uint64_t)0x7EE << 53) & m, &d4);
        f = d4 >> 57;
        cc = inner_gf9309_adc(0, d0, f, &d0);
        cc = inner_gf9309_adc(cc, d1, 0, &d1);
        cc = inner_gf9309_adc(cc, d2, 0, &d2);
        cc = inner_gf9309_adc(cc, d3, 0, &d3);
        (void)inner_gf9309_adc(cc, d4, ((uint64_t)0x7F7 << 53) & -f, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void gf9309_neg(gf9309 *d, const gf9309 *a) {
        uint64_t d0, d1, d2, d3, d4, f; unsigned char cc;
        cc = inner_gf9309_sbb(0, (uint64_t)0xFFFFFFFFFFFFFFFE, a->v0, &d0);
        cc = inner_gf9309_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v1, &d1);
        cc = inner_gf9309_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v2, &d2);
        cc = inner_gf9309_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v3, &d3);
        cc = inner_gf9309_sbb(cc, (uint64_t)0x023FFFFFFFFFFFFF, a->v4, &d4);
        f = d4 >> 57;
        cc = inner_gf9309_adc(0, d0, f, &d0);
        cc = inner_gf9309_adc(cc, d1, 0, &d1);
        cc = inner_gf9309_adc(cc, d2, 0, &d2);
        cc = inner_gf9309_adc(cc, d3, 0, &d3);
        (void)inner_gf9309_adc(cc, d4, ((uint64_t)0x7F7 << 53) & -f, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void gf9309_select(gf9309 *d, const gf9309 *a0, const gf9309 *a1, uint32_t ctl) {
        uint64_t cw = (uint64_t)*(int32_t *)&ctl;
        d->v0 = a0->v0 ^ (cw & (a0->v0 ^ a1->v0));
        d->v1 = a0->v1 ^ (cw & (a0->v1 ^ a1->v1));
        d->v2 = a0->v2 ^ (cw & (a0->v2 ^ a1->v2));
        d->v3 = a0->v3 ^ (cw & (a0->v3 ^ a1->v3));
        d->v4 = a0->v4 ^ (cw & (a0->v4 ^ a1->v4));
    }

    static inline void gf9309_cswap(gf9309 *a, gf9309 *b, uint32_t ctl) {
        uint64_t cw = (uint64_t)*(int32_t *)&ctl, t;
        t = cw & (a->v0 ^ b->v0); a->v0 ^= t; b->v0 ^= t;
        t = cw & (a->v1 ^ b->v1); a->v1 ^= t; b->v1 ^= t;
        t = cw & (a->v2 ^ b->v2); a->v2 ^= t; b->v2 ^= t;
        t = cw & (a->v3 ^ b->v3); a->v3 ^= t; b->v3 ^= t;
        t = cw & (a->v4 ^ b->v4); a->v4 ^= t; b->v4 ^= t;
    }

    static inline void gf9309_half(gf9309 *d, const gf9309 *a) {
        uint64_t d0, d1, d2, d3, d4;
        d0 = (a->v0 >> 1) | (a->v1 << 63);
        d1 = (a->v1 >> 1) | (a->v2 << 63);
        d2 = (a->v2 >> 1) | (a->v3 << 63);
        d3 = (a->v3 >> 1) | (a->v4 << 63);
        d4 = a->v4 >> 1;
        d4 += ((uint64_t)0x90000000000000) & -(a->v0 & 1);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void gf9309_mul2(gf9309 *d, const gf9309 *a) {
        gf9309_add(d, a, a);
    }

    static inline uint32_t gf9309_iszero(const gf9309 *a) {
        uint64_t t0, t1, r;
        t0 = a->v0 | a->v1 | a->v2 | a->v3 | a->v4;
        t1 = ~a->v0 | ~a->v1 | ~a->v2 | ~a->v3 | (a->v4 ^ 0x011FFFFFFFFFFFFF);
        r = (t0 | -t0) & (t1 | -t1);
        return (uint32_t)(r >> 63) - 1;
    }

    static inline uint32_t gf9309_equals(const gf9309 *a, const gf9309 *b) { gf9309 d; gf9309_sub(&d,a,b); return gf9309_iszero(&d); }

    static inline void inner_gf9309_partial_reduce(gf9309 *d, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
        uint64_t d0, d1, d2, d3, d4, h, quo, rem; unsigned char cc;
        h = a4 >> 53;
        a4 &= 0x001FFFFFFFFFFFFF;
        quo = (0x71D * h) >> 14;
        rem = h - (9 * quo);
        cc = inner_gf9309_adc(0, a0, quo, &d0);
        cc = inner_gf9309_adc(cc, a1, 0, &d1);
        cc = inner_gf9309_adc(cc, a2, 0, &d2);
        cc = inner_gf9309_adc(cc, a3, 0, &d3);
        (void)inner_gf9309_adc(cc, a4, rem << 53, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void inner_gf9309_normalize(gf9309 *d, const gf9309 *a) {
        uint64_t d0, d1, d2, d3, d4, m; unsigned char cc;
        cc = inner_gf9309_sbb(0, a->v0, 0xFFFFFFFFFFFFFFFF, &d0);
        cc = inner_gf9309_sbb(cc, a->v1, 0xFFFFFFFFFFFFFFFF, &d1);
        cc = inner_gf9309_sbb(cc, a->v2, 0xFFFFFFFFFFFFFFFF, &d2);
        cc = inner_gf9309_sbb(cc, a->v3, 0xFFFFFFFFFFFFFFFF, &d3);
        cc = inner_gf9309_sbb(cc, a->v4, 0x011FFFFFFFFFFFFF, &d4);
        (void)inner_gf9309_sbb(cc, 0, 0, &m);
        cc = inner_gf9309_adc(0, d0, m, &d0);
        cc = inner_gf9309_adc(cc, d1, m, &d1);
        cc = inner_gf9309_adc(cc, d2, m, &d2);
        cc = inner_gf9309_adc(cc, d3, m, &d3);
        (void)inner_gf9309_adc(cc, d4, m & 0x011FFFFFFFFFFFFF, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void gf9309_set_small(gf9309 *d, uint32_t x) {
        uint64_t h, lo, hi, quo, rem;
        h = (uint64_t)x << 11;
        inner_gf9309_umul(lo, hi, h, 0xE38E38E38E38E38F); (void)lo;
        quo = hi >> 3;
        rem = h - (9 * quo);
        d->v0 = quo;
        d->v1 = 0;
        d->v2 = 0;
        d->v3 = 0;
        d->v4 = rem << 53;
    }

    static inline void gf9309_mul_small(gf9309 *d, const gf9309 *a, uint32_t x) {
        uint64_t d0, d1, d2, d3, d4, d5, lo, hi, carry, b, h, quo, rem; unsigned char cc; (void)cc;
        b = (uint64_t)x; carry = 0;
        inner_gf9309_umul(lo, hi, a->v0, b); cc = inner_gf9309_adc(0, lo, carry, &d0); carry = hi + cc;
        inner_gf9309_umul(lo, hi, a->v1, b); cc = inner_gf9309_adc(0, lo, carry, &d1); carry = hi + cc;
        inner_gf9309_umul(lo, hi, a->v2, b); cc = inner_gf9309_adc(0, lo, carry, &d2); carry = hi + cc;
        inner_gf9309_umul(lo, hi, a->v3, b); cc = inner_gf9309_adc(0, lo, carry, &d3); carry = hi + cc;
        inner_gf9309_umul(lo, hi, a->v4, b); cc = inner_gf9309_adc(0, lo, carry, &d4); carry = hi + cc;
        d5 = carry;
        h = (d5 << 11) | (d4 >> 53);
        d4 &= 0x001FFFFFFFFFFFFF;
        inner_gf9309_umul(lo, hi, h, 0xE38E38E38E38E38F);
        quo = hi >> 3; rem = h - (9 * quo);
        cc = inner_gf9309_adc(0, d0, quo, &d0);
        cc = inner_gf9309_adc(cc, d1, 0, &d1);
        cc = inner_gf9309_adc(cc, d2, 0, &d2);
        cc = inner_gf9309_adc(cc, d3, 0, &d3);
        (void)inner_gf9309_adc(cc, d4, rem << 53, &d4);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
    }

    static inline void inner_gf9309_montgomery_reduce(gf9309 *d, const gf9309 *a) {
        uint64_t x0, x1, x2, x3, x4;
        uint64_t f0, f1, f2, f3, f4;
        uint64_t g0, g1, g2, g3, g4, g5, g6, g7, g8, g9;
        uint64_t d0, d1, d2, d3, d4;
        uint64_t hi, t, w; unsigned char cc;
        x0 = a->v0;
        x1 = a->v1;
        x2 = a->v2;
        x3 = a->v3;
        x4 = a->v4;
        f0 = x0;
        f1 = x1;
        f2 = x2;
        f3 = x3;
        f4 = x4 + ((x0 * 9) << 53);
        inner_gf9309_umul(g4, hi, f0, (uint64_t)9 << 53);
        inner_gf9309_umul_add(g5, hi, f1, (uint64_t)9 << 53, hi);
        inner_gf9309_umul_add(g6, hi, f2, (uint64_t)9 << 53, hi);
        inner_gf9309_umul_add(g7, hi, f3, (uint64_t)9 << 53, hi);
        inner_gf9309_umul_add(g8, g9, f4, (uint64_t)9 << 53, hi);
        cc = inner_gf9309_sbb(0, 0, f0, &g0);
        cc = inner_gf9309_sbb(cc, 0, f1, &g1);
        cc = inner_gf9309_sbb(cc, 0, f2, &g2);
        cc = inner_gf9309_sbb(cc, 0, f3, &g3);
        cc = inner_gf9309_sbb(cc, g4, f4, &g4);
        cc = inner_gf9309_sbb(cc, g5, 0, &g5);
        cc = inner_gf9309_sbb(cc, g6, 0, &g6);
        cc = inner_gf9309_sbb(cc, g7, 0, &g7);
        cc = inner_gf9309_sbb(cc, g8, 0, &g8);
        (void)inner_gf9309_sbb(cc, g9, 0, &g9);
        cc = inner_gf9309_adc(0, g0, x0, &x0);
        cc = inner_gf9309_adc(cc, g1, x1, &x1);
        cc = inner_gf9309_adc(cc, g2, x2, &x2);
        cc = inner_gf9309_adc(cc, g3, x3, &x3);
        cc = inner_gf9309_adc(cc, g4, x4, &x4);
        cc = inner_gf9309_adc(cc, g5, 0, &d0);
        cc = inner_gf9309_adc(cc, g6, 0, &d1);
        cc = inner_gf9309_adc(cc, g7, 0, &d2);
        cc = inner_gf9309_adc(cc, g8, 0, &d3);
        cc = inner_gf9309_adc(cc, g9, 0, &d4);
        (void)cc;
        t = d0 & d1 & d2 & d3 & (d4 ^ ~(uint64_t)0x011FFFFFFFFFFFFF);
        cc = inner_gf9309_adc(0, t, 1, &t);
        (void)inner_gf9309_sbb(cc, 0, 0, &w); w = ~w;
        d->v0 = d0 & w;
        d->v1 = d1 & w;
        d->v2 = d2 & w;
        d->v3 = d3 & w;
        d->v4 = d4 & w;
    }

    uint32_t gf9309_invert(gf9309 *d, const gf9309 *a);
    int32_t gf9309_legendre(const gf9309 *a);
    uint32_t gf9309_sqrt(gf9309 *d, const gf9309 *a);
    void gf9309_div3(gf9309 *d, const gf9309 *a);
    void gf9309_encode(void *dst, const gf9309 *a);
    uint32_t gf9309_decode(gf9309 *d, const void *src);
    void gf9309_decode_reduce(gf9309 *d, const void *src, size_t len);
    void gf9309_pow(gf9309 *out, const gf9309 *a, const uint64_t *e, int nbits);

#ifdef __cplusplus
}
#endif
#endif
