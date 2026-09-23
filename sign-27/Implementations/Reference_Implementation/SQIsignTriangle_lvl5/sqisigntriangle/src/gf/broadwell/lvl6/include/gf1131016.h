#ifndef gf1131016_h__
#define gf1131016_h__
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
        uint64_t v5;
        uint64_t v6;
        uint64_t v7;
        uint64_t v8;
        uint64_t v9;
        uint64_t v10;
        uint64_t v11;
        uint64_t v12;
        uint64_t v13;
        uint64_t v14;
        uint64_t v15;
    }; digit_t arr[16]; } gf1131016;

    extern const gf1131016 gf1131016_ZERO;
    extern const gf1131016 gf1131016_ONE;
    extern const gf1131016 gf1131016_MINUS_ONE;

#if (defined _MSC_VER && defined _M_X64) || (defined __x86_64__ && (defined __GNUC__ || defined __clang__))
#include <immintrin.h>
#define inner_gf1131016_adc(cc, a, b, d) _addcarry_u64(cc, a, b, (unsigned long long *)(void *)d)
#define inner_gf1131016_sbb(cc, a, b, d) _subborrow_u64(cc, a, b, (unsigned long long *)(void *)d)
#else
static inline unsigned char inner_gf1131016_adc(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a+(unsigned __int128)b+cc; *d=(uint64_t)t; return (unsigned char)(t>>64);}
static inline unsigned char inner_gf1131016_sbb(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a-(unsigned __int128)b-cc; *d=(uint64_t)t; return (unsigned char)(-(uint64_t)(t>>64));}
#endif
#define inner_gf1131016_umul(lo,hi,x,y) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf1131016_umul_add(lo,hi,x,y,z) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf1131016_umul_x2(lo,hi,x1,y1,x2,y2) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_gf1131016_umul_x2_add(lo,hi,x1,y1,x2,y2,z) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)

    static inline void gf1131016_add(gf1131016 *d, const gf1131016 *a, const gf1131016 *b) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, f; unsigned char cc;
        cc = inner_gf1131016_adc(0, a->v0, b->v0, &d0);
        cc = inner_gf1131016_adc(cc, a->v1, b->v1, &d1);
        cc = inner_gf1131016_adc(cc, a->v2, b->v2, &d2);
        cc = inner_gf1131016_adc(cc, a->v3, b->v3, &d3);
        cc = inner_gf1131016_adc(cc, a->v4, b->v4, &d4);
        cc = inner_gf1131016_adc(cc, a->v5, b->v5, &d5);
        cc = inner_gf1131016_adc(cc, a->v6, b->v6, &d6);
        cc = inner_gf1131016_adc(cc, a->v7, b->v7, &d7);
        cc = inner_gf1131016_adc(cc, a->v8, b->v8, &d8);
        cc = inner_gf1131016_adc(cc, a->v9, b->v9, &d9);
        cc = inner_gf1131016_adc(cc, a->v10, b->v10, &d10);
        cc = inner_gf1131016_adc(cc, a->v11, b->v11, &d11);
        cc = inner_gf1131016_adc(cc, a->v12, b->v12, &d12);
        cc = inner_gf1131016_adc(cc, a->v13, b->v13, &d13);
        cc = inner_gf1131016_adc(cc, a->v14, b->v14, &d14);
        cc = inner_gf1131016_adc(cc, a->v15, b->v15, &d15);
        f = d15 >> 63;
        cc = inner_gf1131016_adc(0, d0, f, &d0);
        cc = inner_gf1131016_adc(cc, d1, 0, &d1);
        cc = inner_gf1131016_adc(cc, d2, 0, &d2);
        cc = inner_gf1131016_adc(cc, d3, 0, &d3);
        cc = inner_gf1131016_adc(cc, d4, 0, &d4);
        cc = inner_gf1131016_adc(cc, d5, 0, &d5);
        cc = inner_gf1131016_adc(cc, d6, 0, &d6);
        cc = inner_gf1131016_adc(cc, d7, 0, &d7);
        cc = inner_gf1131016_adc(cc, d8, 0, &d8);
        cc = inner_gf1131016_adc(cc, d9, 0, &d9);
        cc = inner_gf1131016_adc(cc, d10, 0, &d10);
        cc = inner_gf1131016_adc(cc, d11, 0, &d11);
        cc = inner_gf1131016_adc(cc, d12, 0, &d12);
        cc = inner_gf1131016_adc(cc, d13, 0, &d13);
        cc = inner_gf1131016_adc(cc, d14, 0, &d14);
        (void)inner_gf1131016_adc(cc, d15, ((uint64_t)0x8F << 56) & -f, &d15);
        f = d15 >> 63;
        cc = inner_gf1131016_adc(0, d0, f, &d0);
        cc = inner_gf1131016_adc(cc, d1, 0, &d1);
        cc = inner_gf1131016_adc(cc, d2, 0, &d2);
        cc = inner_gf1131016_adc(cc, d3, 0, &d3);
        cc = inner_gf1131016_adc(cc, d4, 0, &d4);
        cc = inner_gf1131016_adc(cc, d5, 0, &d5);
        cc = inner_gf1131016_adc(cc, d6, 0, &d6);
        cc = inner_gf1131016_adc(cc, d7, 0, &d7);
        cc = inner_gf1131016_adc(cc, d8, 0, &d8);
        cc = inner_gf1131016_adc(cc, d9, 0, &d9);
        cc = inner_gf1131016_adc(cc, d10, 0, &d10);
        cc = inner_gf1131016_adc(cc, d11, 0, &d11);
        cc = inner_gf1131016_adc(cc, d12, 0, &d12);
        cc = inner_gf1131016_adc(cc, d13, 0, &d13);
        cc = inner_gf1131016_adc(cc, d14, 0, &d14);
        (void)inner_gf1131016_adc(cc, d15, ((uint64_t)0x8F << 56) & -f, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void gf1131016_sub(gf1131016 *d, const gf1131016 *a, const gf1131016 *b) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, m, f; unsigned char cc;
        cc = inner_gf1131016_sbb(0, a->v0, b->v0, &d0);
        cc = inner_gf1131016_sbb(cc, a->v1, b->v1, &d1);
        cc = inner_gf1131016_sbb(cc, a->v2, b->v2, &d2);
        cc = inner_gf1131016_sbb(cc, a->v3, b->v3, &d3);
        cc = inner_gf1131016_sbb(cc, a->v4, b->v4, &d4);
        cc = inner_gf1131016_sbb(cc, a->v5, b->v5, &d5);
        cc = inner_gf1131016_sbb(cc, a->v6, b->v6, &d6);
        cc = inner_gf1131016_sbb(cc, a->v7, b->v7, &d7);
        cc = inner_gf1131016_sbb(cc, a->v8, b->v8, &d8);
        cc = inner_gf1131016_sbb(cc, a->v9, b->v9, &d9);
        cc = inner_gf1131016_sbb(cc, a->v10, b->v10, &d10);
        cc = inner_gf1131016_sbb(cc, a->v11, b->v11, &d11);
        cc = inner_gf1131016_sbb(cc, a->v12, b->v12, &d12);
        cc = inner_gf1131016_sbb(cc, a->v13, b->v13, &d13);
        cc = inner_gf1131016_sbb(cc, a->v14, b->v14, &d14);
        cc = inner_gf1131016_sbb(cc, a->v15, b->v15, &d15);
        (void)inner_gf1131016_sbb(cc, 0, 0, &m);
        cc = inner_gf1131016_sbb(0, d0, m & 2, &d0);
        cc = inner_gf1131016_sbb(cc, d1, 0, &d1);
        cc = inner_gf1131016_sbb(cc, d2, 0, &d2);
        cc = inner_gf1131016_sbb(cc, d3, 0, &d3);
        cc = inner_gf1131016_sbb(cc, d4, 0, &d4);
        cc = inner_gf1131016_sbb(cc, d5, 0, &d5);
        cc = inner_gf1131016_sbb(cc, d6, 0, &d6);
        cc = inner_gf1131016_sbb(cc, d7, 0, &d7);
        cc = inner_gf1131016_sbb(cc, d8, 0, &d8);
        cc = inner_gf1131016_sbb(cc, d9, 0, &d9);
        cc = inner_gf1131016_sbb(cc, d10, 0, &d10);
        cc = inner_gf1131016_sbb(cc, d11, 0, &d11);
        cc = inner_gf1131016_sbb(cc, d12, 0, &d12);
        cc = inner_gf1131016_sbb(cc, d13, 0, &d13);
        cc = inner_gf1131016_sbb(cc, d14, 0, &d14);
        (void)inner_gf1131016_sbb(cc, d15, ((uint64_t)0x1E << 56) & m, &d15);
        f = d15 >> 63;
        cc = inner_gf1131016_adc(0, d0, f, &d0);
        cc = inner_gf1131016_adc(cc, d1, 0, &d1);
        cc = inner_gf1131016_adc(cc, d2, 0, &d2);
        cc = inner_gf1131016_adc(cc, d3, 0, &d3);
        cc = inner_gf1131016_adc(cc, d4, 0, &d4);
        cc = inner_gf1131016_adc(cc, d5, 0, &d5);
        cc = inner_gf1131016_adc(cc, d6, 0, &d6);
        cc = inner_gf1131016_adc(cc, d7, 0, &d7);
        cc = inner_gf1131016_adc(cc, d8, 0, &d8);
        cc = inner_gf1131016_adc(cc, d9, 0, &d9);
        cc = inner_gf1131016_adc(cc, d10, 0, &d10);
        cc = inner_gf1131016_adc(cc, d11, 0, &d11);
        cc = inner_gf1131016_adc(cc, d12, 0, &d12);
        cc = inner_gf1131016_adc(cc, d13, 0, &d13);
        cc = inner_gf1131016_adc(cc, d14, 0, &d14);
        (void)inner_gf1131016_adc(cc, d15, ((uint64_t)0x8F << 56) & -f, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void gf1131016_neg(gf1131016 *d, const gf1131016 *a) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, f; unsigned char cc;
        cc = inner_gf1131016_sbb(0, (uint64_t)0xFFFFFFFFFFFFFFFE, a->v0, &d0);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v1, &d1);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v2, &d2);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v3, &d3);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v4, &d4);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v5, &d5);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v6, &d6);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v7, &d7);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v8, &d8);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v9, &d9);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v10, &d10);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v11, &d11);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v12, &d12);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v13, &d13);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xFFFFFFFFFFFFFFFF, a->v14, &d14);
        cc = inner_gf1131016_sbb(cc, (uint64_t)0xE1FFFFFFFFFFFFFF, a->v15, &d15);
        f = d15 >> 63;
        cc = inner_gf1131016_adc(0, d0, f, &d0);
        cc = inner_gf1131016_adc(cc, d1, 0, &d1);
        cc = inner_gf1131016_adc(cc, d2, 0, &d2);
        cc = inner_gf1131016_adc(cc, d3, 0, &d3);
        cc = inner_gf1131016_adc(cc, d4, 0, &d4);
        cc = inner_gf1131016_adc(cc, d5, 0, &d5);
        cc = inner_gf1131016_adc(cc, d6, 0, &d6);
        cc = inner_gf1131016_adc(cc, d7, 0, &d7);
        cc = inner_gf1131016_adc(cc, d8, 0, &d8);
        cc = inner_gf1131016_adc(cc, d9, 0, &d9);
        cc = inner_gf1131016_adc(cc, d10, 0, &d10);
        cc = inner_gf1131016_adc(cc, d11, 0, &d11);
        cc = inner_gf1131016_adc(cc, d12, 0, &d12);
        cc = inner_gf1131016_adc(cc, d13, 0, &d13);
        cc = inner_gf1131016_adc(cc, d14, 0, &d14);
        (void)inner_gf1131016_adc(cc, d15, ((uint64_t)0x8F << 56) & -f, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void gf1131016_select(gf1131016 *d, const gf1131016 *a0, const gf1131016 *a1, uint32_t ctl) {
        uint64_t cw = (uint64_t)*(int32_t *)&ctl;
        d->v0 = a0->v0 ^ (cw & (a0->v0 ^ a1->v0));
        d->v1 = a0->v1 ^ (cw & (a0->v1 ^ a1->v1));
        d->v2 = a0->v2 ^ (cw & (a0->v2 ^ a1->v2));
        d->v3 = a0->v3 ^ (cw & (a0->v3 ^ a1->v3));
        d->v4 = a0->v4 ^ (cw & (a0->v4 ^ a1->v4));
        d->v5 = a0->v5 ^ (cw & (a0->v5 ^ a1->v5));
        d->v6 = a0->v6 ^ (cw & (a0->v6 ^ a1->v6));
        d->v7 = a0->v7 ^ (cw & (a0->v7 ^ a1->v7));
        d->v8 = a0->v8 ^ (cw & (a0->v8 ^ a1->v8));
        d->v9 = a0->v9 ^ (cw & (a0->v9 ^ a1->v9));
        d->v10 = a0->v10 ^ (cw & (a0->v10 ^ a1->v10));
        d->v11 = a0->v11 ^ (cw & (a0->v11 ^ a1->v11));
        d->v12 = a0->v12 ^ (cw & (a0->v12 ^ a1->v12));
        d->v13 = a0->v13 ^ (cw & (a0->v13 ^ a1->v13));
        d->v14 = a0->v14 ^ (cw & (a0->v14 ^ a1->v14));
        d->v15 = a0->v15 ^ (cw & (a0->v15 ^ a1->v15));
    }

    static inline void gf1131016_cswap(gf1131016 *a, gf1131016 *b, uint32_t ctl) {
        uint64_t cw = (uint64_t)*(int32_t *)&ctl, t;
        t = cw & (a->v0 ^ b->v0); a->v0 ^= t; b->v0 ^= t;
        t = cw & (a->v1 ^ b->v1); a->v1 ^= t; b->v1 ^= t;
        t = cw & (a->v2 ^ b->v2); a->v2 ^= t; b->v2 ^= t;
        t = cw & (a->v3 ^ b->v3); a->v3 ^= t; b->v3 ^= t;
        t = cw & (a->v4 ^ b->v4); a->v4 ^= t; b->v4 ^= t;
        t = cw & (a->v5 ^ b->v5); a->v5 ^= t; b->v5 ^= t;
        t = cw & (a->v6 ^ b->v6); a->v6 ^= t; b->v6 ^= t;
        t = cw & (a->v7 ^ b->v7); a->v7 ^= t; b->v7 ^= t;
        t = cw & (a->v8 ^ b->v8); a->v8 ^= t; b->v8 ^= t;
        t = cw & (a->v9 ^ b->v9); a->v9 ^= t; b->v9 ^= t;
        t = cw & (a->v10 ^ b->v10); a->v10 ^= t; b->v10 ^= t;
        t = cw & (a->v11 ^ b->v11); a->v11 ^= t; b->v11 ^= t;
        t = cw & (a->v12 ^ b->v12); a->v12 ^= t; b->v12 ^= t;
        t = cw & (a->v13 ^ b->v13); a->v13 ^= t; b->v13 ^= t;
        t = cw & (a->v14 ^ b->v14); a->v14 ^= t; b->v14 ^= t;
        t = cw & (a->v15 ^ b->v15); a->v15 ^= t; b->v15 ^= t;
    }

    static inline void gf1131016_half(gf1131016 *d, const gf1131016 *a) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15;
        d0 = (a->v0 >> 1) | (a->v1 << 63);
        d1 = (a->v1 >> 1) | (a->v2 << 63);
        d2 = (a->v2 >> 1) | (a->v3 << 63);
        d3 = (a->v3 >> 1) | (a->v4 << 63);
        d4 = (a->v4 >> 1) | (a->v5 << 63);
        d5 = (a->v5 >> 1) | (a->v6 << 63);
        d6 = (a->v6 >> 1) | (a->v7 << 63);
        d7 = (a->v7 >> 1) | (a->v8 << 63);
        d8 = (a->v8 >> 1) | (a->v9 << 63);
        d9 = (a->v9 >> 1) | (a->v10 << 63);
        d10 = (a->v10 >> 1) | (a->v11 << 63);
        d11 = (a->v11 >> 1) | (a->v12 << 63);
        d12 = (a->v12 >> 1) | (a->v13 << 63);
        d13 = (a->v13 >> 1) | (a->v14 << 63);
        d14 = (a->v14 >> 1) | (a->v15 << 63);
        d15 = a->v15 >> 1;
        d15 += ((uint64_t)0x3880000000000000) & -(a->v0 & 1);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void gf1131016_mul2(gf1131016 *d, const gf1131016 *a) {
        gf1131016_add(d, a, a);
    }

    static inline uint32_t gf1131016_iszero(const gf1131016 *a) {
        uint64_t t0, t1, r;
        t0 = a->v0 | a->v1 | a->v2 | a->v3 | a->v4 | a->v5 | a->v6 | a->v7 | a->v8 | a->v9 | a->v10 | a->v11 | a->v12 | a->v13 | a->v14 | a->v15;
        t1 = ~a->v0 | ~a->v1 | ~a->v2 | ~a->v3 | ~a->v4 | ~a->v5 | ~a->v6 | ~a->v7 | ~a->v8 | ~a->v9 | ~a->v10 | ~a->v11 | ~a->v12 | ~a->v13 | ~a->v14 | (a->v15 ^ 0x70FFFFFFFFFFFFFF);
        r = (t0 | -t0) & (t1 | -t1);
        return (uint32_t)(r >> 63) - 1;
    }

    static inline uint32_t gf1131016_equals(const gf1131016 *a, const gf1131016 *b) { gf1131016 d; gf1131016_sub(&d,a,b); return gf1131016_iszero(&d); }

    static inline void inner_gf1131016_partial_reduce(gf1131016 *d, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7, uint64_t a8, uint64_t a9, uint64_t a10, uint64_t a11, uint64_t a12, uint64_t a13, uint64_t a14, uint64_t a15) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, h, quo, rem; unsigned char cc;
        h = a15 >> 56;
        a15 &= 0x00FFFFFFFFFFFFFF;
        quo = (0x91 * h) >> 14;
        rem = h - (113 * quo);
        cc = inner_gf1131016_adc(0, a0, quo, &d0);
        cc = inner_gf1131016_adc(cc, a1, 0, &d1);
        cc = inner_gf1131016_adc(cc, a2, 0, &d2);
        cc = inner_gf1131016_adc(cc, a3, 0, &d3);
        cc = inner_gf1131016_adc(cc, a4, 0, &d4);
        cc = inner_gf1131016_adc(cc, a5, 0, &d5);
        cc = inner_gf1131016_adc(cc, a6, 0, &d6);
        cc = inner_gf1131016_adc(cc, a7, 0, &d7);
        cc = inner_gf1131016_adc(cc, a8, 0, &d8);
        cc = inner_gf1131016_adc(cc, a9, 0, &d9);
        cc = inner_gf1131016_adc(cc, a10, 0, &d10);
        cc = inner_gf1131016_adc(cc, a11, 0, &d11);
        cc = inner_gf1131016_adc(cc, a12, 0, &d12);
        cc = inner_gf1131016_adc(cc, a13, 0, &d13);
        cc = inner_gf1131016_adc(cc, a14, 0, &d14);
        (void)inner_gf1131016_adc(cc, a15, rem << 56, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void inner_gf1131016_normalize(gf1131016 *d, const gf1131016 *a) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, m; unsigned char cc;
        cc = inner_gf1131016_sbb(0, a->v0, 0xFFFFFFFFFFFFFFFF, &d0);
        cc = inner_gf1131016_sbb(cc, a->v1, 0xFFFFFFFFFFFFFFFF, &d1);
        cc = inner_gf1131016_sbb(cc, a->v2, 0xFFFFFFFFFFFFFFFF, &d2);
        cc = inner_gf1131016_sbb(cc, a->v3, 0xFFFFFFFFFFFFFFFF, &d3);
        cc = inner_gf1131016_sbb(cc, a->v4, 0xFFFFFFFFFFFFFFFF, &d4);
        cc = inner_gf1131016_sbb(cc, a->v5, 0xFFFFFFFFFFFFFFFF, &d5);
        cc = inner_gf1131016_sbb(cc, a->v6, 0xFFFFFFFFFFFFFFFF, &d6);
        cc = inner_gf1131016_sbb(cc, a->v7, 0xFFFFFFFFFFFFFFFF, &d7);
        cc = inner_gf1131016_sbb(cc, a->v8, 0xFFFFFFFFFFFFFFFF, &d8);
        cc = inner_gf1131016_sbb(cc, a->v9, 0xFFFFFFFFFFFFFFFF, &d9);
        cc = inner_gf1131016_sbb(cc, a->v10, 0xFFFFFFFFFFFFFFFF, &d10);
        cc = inner_gf1131016_sbb(cc, a->v11, 0xFFFFFFFFFFFFFFFF, &d11);
        cc = inner_gf1131016_sbb(cc, a->v12, 0xFFFFFFFFFFFFFFFF, &d12);
        cc = inner_gf1131016_sbb(cc, a->v13, 0xFFFFFFFFFFFFFFFF, &d13);
        cc = inner_gf1131016_sbb(cc, a->v14, 0xFFFFFFFFFFFFFFFF, &d14);
        cc = inner_gf1131016_sbb(cc, a->v15, 0x70FFFFFFFFFFFFFF, &d15);
        (void)inner_gf1131016_sbb(cc, 0, 0, &m);
        cc = inner_gf1131016_adc(0, d0, m, &d0);
        cc = inner_gf1131016_adc(cc, d1, m, &d1);
        cc = inner_gf1131016_adc(cc, d2, m, &d2);
        cc = inner_gf1131016_adc(cc, d3, m, &d3);
        cc = inner_gf1131016_adc(cc, d4, m, &d4);
        cc = inner_gf1131016_adc(cc, d5, m, &d5);
        cc = inner_gf1131016_adc(cc, d6, m, &d6);
        cc = inner_gf1131016_adc(cc, d7, m, &d7);
        cc = inner_gf1131016_adc(cc, d8, m, &d8);
        cc = inner_gf1131016_adc(cc, d9, m, &d9);
        cc = inner_gf1131016_adc(cc, d10, m, &d10);
        cc = inner_gf1131016_adc(cc, d11, m, &d11);
        cc = inner_gf1131016_adc(cc, d12, m, &d12);
        cc = inner_gf1131016_adc(cc, d13, m, &d13);
        cc = inner_gf1131016_adc(cc, d14, m, &d14);
        (void)inner_gf1131016_adc(cc, d15, m & 0x70FFFFFFFFFFFFFF, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void gf1131016_set_small(gf1131016 *d, uint32_t x) {
        uint64_t h, lo, hi, quo, rem;
        h = (uint64_t)x << 8;
        inner_gf1131016_umul(lo, hi, h, 0x90FDBC090FDBC091); (void)lo;
        quo = hi >> 6;
        rem = h - (113 * quo);
        d->v0 = quo;
        d->v1 = 0;
        d->v2 = 0;
        d->v3 = 0;
        d->v4 = 0;
        d->v5 = 0;
        d->v6 = 0;
        d->v7 = 0;
        d->v8 = 0;
        d->v9 = 0;
        d->v10 = 0;
        d->v11 = 0;
        d->v12 = 0;
        d->v13 = 0;
        d->v14 = 0;
        d->v15 = rem << 56;
    }

    static inline void gf1131016_mul_small(gf1131016 *d, const gf1131016 *a, uint32_t x) {
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16, lo, hi, carry, b, h, quo, rem; unsigned char cc; (void)cc;
        b = (uint64_t)x; carry = 0;
        inner_gf1131016_umul(lo, hi, a->v0, b); cc = inner_gf1131016_adc(0, lo, carry, &d0); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v1, b); cc = inner_gf1131016_adc(0, lo, carry, &d1); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v2, b); cc = inner_gf1131016_adc(0, lo, carry, &d2); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v3, b); cc = inner_gf1131016_adc(0, lo, carry, &d3); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v4, b); cc = inner_gf1131016_adc(0, lo, carry, &d4); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v5, b); cc = inner_gf1131016_adc(0, lo, carry, &d5); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v6, b); cc = inner_gf1131016_adc(0, lo, carry, &d6); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v7, b); cc = inner_gf1131016_adc(0, lo, carry, &d7); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v8, b); cc = inner_gf1131016_adc(0, lo, carry, &d8); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v9, b); cc = inner_gf1131016_adc(0, lo, carry, &d9); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v10, b); cc = inner_gf1131016_adc(0, lo, carry, &d10); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v11, b); cc = inner_gf1131016_adc(0, lo, carry, &d11); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v12, b); cc = inner_gf1131016_adc(0, lo, carry, &d12); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v13, b); cc = inner_gf1131016_adc(0, lo, carry, &d13); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v14, b); cc = inner_gf1131016_adc(0, lo, carry, &d14); carry = hi + cc;
        inner_gf1131016_umul(lo, hi, a->v15, b); cc = inner_gf1131016_adc(0, lo, carry, &d15); carry = hi + cc;
        d16 = carry;
        h = (d16 << 8) | (d15 >> 56);
        d15 &= 0x00FFFFFFFFFFFFFF;
        inner_gf1131016_umul(lo, hi, h, 0x90FDBC090FDBC091);
        quo = hi >> 6; rem = h - (113 * quo);
        cc = inner_gf1131016_adc(0, d0, quo, &d0);
        cc = inner_gf1131016_adc(cc, d1, 0, &d1);
        cc = inner_gf1131016_adc(cc, d2, 0, &d2);
        cc = inner_gf1131016_adc(cc, d3, 0, &d3);
        cc = inner_gf1131016_adc(cc, d4, 0, &d4);
        cc = inner_gf1131016_adc(cc, d5, 0, &d5);
        cc = inner_gf1131016_adc(cc, d6, 0, &d6);
        cc = inner_gf1131016_adc(cc, d7, 0, &d7);
        cc = inner_gf1131016_adc(cc, d8, 0, &d8);
        cc = inner_gf1131016_adc(cc, d9, 0, &d9);
        cc = inner_gf1131016_adc(cc, d10, 0, &d10);
        cc = inner_gf1131016_adc(cc, d11, 0, &d11);
        cc = inner_gf1131016_adc(cc, d12, 0, &d12);
        cc = inner_gf1131016_adc(cc, d13, 0, &d13);
        cc = inner_gf1131016_adc(cc, d14, 0, &d14);
        (void)inner_gf1131016_adc(cc, d15, rem << 56, &d15);
        d->v0 = d0;
        d->v1 = d1;
        d->v2 = d2;
        d->v3 = d3;
        d->v4 = d4;
        d->v5 = d5;
        d->v6 = d6;
        d->v7 = d7;
        d->v8 = d8;
        d->v9 = d9;
        d->v10 = d10;
        d->v11 = d11;
        d->v12 = d12;
        d->v13 = d13;
        d->v14 = d14;
        d->v15 = d15;
    }

    static inline void inner_gf1131016_montgomery_reduce(gf1131016 *d, const gf1131016 *a) {
        uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
        uint64_t f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15;
        uint64_t g0, g1, g2, g3, g4, g5, g6, g7, g8, g9, g10, g11, g12, g13, g14, g15, g16, g17, g18, g19, g20, g21, g22, g23, g24, g25, g26, g27, g28, g29, g30, g31;
        uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15;
        uint64_t hi, t, w; unsigned char cc;
        x0 = a->v0;
        x1 = a->v1;
        x2 = a->v2;
        x3 = a->v3;
        x4 = a->v4;
        x5 = a->v5;
        x6 = a->v6;
        x7 = a->v7;
        x8 = a->v8;
        x9 = a->v9;
        x10 = a->v10;
        x11 = a->v11;
        x12 = a->v12;
        x13 = a->v13;
        x14 = a->v14;
        x15 = a->v15;
        f0 = x0;
        f1 = x1;
        f2 = x2;
        f3 = x3;
        f4 = x4;
        f5 = x5;
        f6 = x6;
        f7 = x7;
        f8 = x8;
        f9 = x9;
        f10 = x10;
        f11 = x11;
        f12 = x12;
        f13 = x13;
        f14 = x14;
        f15 = x15 + ((x0 * 113) << 56);
        inner_gf1131016_umul(g15, hi, f0, (uint64_t)113 << 56);
        inner_gf1131016_umul_add(g16, hi, f1, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g17, hi, f2, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g18, hi, f3, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g19, hi, f4, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g20, hi, f5, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g21, hi, f6, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g22, hi, f7, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g23, hi, f8, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g24, hi, f9, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g25, hi, f10, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g26, hi, f11, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g27, hi, f12, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g28, hi, f13, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g29, hi, f14, (uint64_t)113 << 56, hi);
        inner_gf1131016_umul_add(g30, g31, f15, (uint64_t)113 << 56, hi);
        cc = inner_gf1131016_sbb(0, 0, f0, &g0);
        cc = inner_gf1131016_sbb(cc, 0, f1, &g1);
        cc = inner_gf1131016_sbb(cc, 0, f2, &g2);
        cc = inner_gf1131016_sbb(cc, 0, f3, &g3);
        cc = inner_gf1131016_sbb(cc, 0, f4, &g4);
        cc = inner_gf1131016_sbb(cc, 0, f5, &g5);
        cc = inner_gf1131016_sbb(cc, 0, f6, &g6);
        cc = inner_gf1131016_sbb(cc, 0, f7, &g7);
        cc = inner_gf1131016_sbb(cc, 0, f8, &g8);
        cc = inner_gf1131016_sbb(cc, 0, f9, &g9);
        cc = inner_gf1131016_sbb(cc, 0, f10, &g10);
        cc = inner_gf1131016_sbb(cc, 0, f11, &g11);
        cc = inner_gf1131016_sbb(cc, 0, f12, &g12);
        cc = inner_gf1131016_sbb(cc, 0, f13, &g13);
        cc = inner_gf1131016_sbb(cc, 0, f14, &g14);
        cc = inner_gf1131016_sbb(cc, g15, f15, &g15);
        cc = inner_gf1131016_sbb(cc, g16, 0, &g16);
        cc = inner_gf1131016_sbb(cc, g17, 0, &g17);
        cc = inner_gf1131016_sbb(cc, g18, 0, &g18);
        cc = inner_gf1131016_sbb(cc, g19, 0, &g19);
        cc = inner_gf1131016_sbb(cc, g20, 0, &g20);
        cc = inner_gf1131016_sbb(cc, g21, 0, &g21);
        cc = inner_gf1131016_sbb(cc, g22, 0, &g22);
        cc = inner_gf1131016_sbb(cc, g23, 0, &g23);
        cc = inner_gf1131016_sbb(cc, g24, 0, &g24);
        cc = inner_gf1131016_sbb(cc, g25, 0, &g25);
        cc = inner_gf1131016_sbb(cc, g26, 0, &g26);
        cc = inner_gf1131016_sbb(cc, g27, 0, &g27);
        cc = inner_gf1131016_sbb(cc, g28, 0, &g28);
        cc = inner_gf1131016_sbb(cc, g29, 0, &g29);
        cc = inner_gf1131016_sbb(cc, g30, 0, &g30);
        (void)inner_gf1131016_sbb(cc, g31, 0, &g31);
        cc = inner_gf1131016_adc(0, g0, x0, &x0);
        cc = inner_gf1131016_adc(cc, g1, x1, &x1);
        cc = inner_gf1131016_adc(cc, g2, x2, &x2);
        cc = inner_gf1131016_adc(cc, g3, x3, &x3);
        cc = inner_gf1131016_adc(cc, g4, x4, &x4);
        cc = inner_gf1131016_adc(cc, g5, x5, &x5);
        cc = inner_gf1131016_adc(cc, g6, x6, &x6);
        cc = inner_gf1131016_adc(cc, g7, x7, &x7);
        cc = inner_gf1131016_adc(cc, g8, x8, &x8);
        cc = inner_gf1131016_adc(cc, g9, x9, &x9);
        cc = inner_gf1131016_adc(cc, g10, x10, &x10);
        cc = inner_gf1131016_adc(cc, g11, x11, &x11);
        cc = inner_gf1131016_adc(cc, g12, x12, &x12);
        cc = inner_gf1131016_adc(cc, g13, x13, &x13);
        cc = inner_gf1131016_adc(cc, g14, x14, &x14);
        cc = inner_gf1131016_adc(cc, g15, x15, &x15);
        cc = inner_gf1131016_adc(cc, g16, 0, &d0);
        cc = inner_gf1131016_adc(cc, g17, 0, &d1);
        cc = inner_gf1131016_adc(cc, g18, 0, &d2);
        cc = inner_gf1131016_adc(cc, g19, 0, &d3);
        cc = inner_gf1131016_adc(cc, g20, 0, &d4);
        cc = inner_gf1131016_adc(cc, g21, 0, &d5);
        cc = inner_gf1131016_adc(cc, g22, 0, &d6);
        cc = inner_gf1131016_adc(cc, g23, 0, &d7);
        cc = inner_gf1131016_adc(cc, g24, 0, &d8);
        cc = inner_gf1131016_adc(cc, g25, 0, &d9);
        cc = inner_gf1131016_adc(cc, g26, 0, &d10);
        cc = inner_gf1131016_adc(cc, g27, 0, &d11);
        cc = inner_gf1131016_adc(cc, g28, 0, &d12);
        cc = inner_gf1131016_adc(cc, g29, 0, &d13);
        cc = inner_gf1131016_adc(cc, g30, 0, &d14);
        cc = inner_gf1131016_adc(cc, g31, 0, &d15);
        (void)cc;
        t = d0 & d1 & d2 & d3 & d4 & d5 & d6 & d7 & d8 & d9 & d10 & d11 & d12 & d13 & d14 & (d15 ^ ~(uint64_t)0x70FFFFFFFFFFFFFF);
        cc = inner_gf1131016_adc(0, t, 1, &t);
        (void)inner_gf1131016_sbb(cc, 0, 0, &w); w = ~w;
        d->v0 = d0 & w;
        d->v1 = d1 & w;
        d->v2 = d2 & w;
        d->v3 = d3 & w;
        d->v4 = d4 & w;
        d->v5 = d5 & w;
        d->v6 = d6 & w;
        d->v7 = d7 & w;
        d->v8 = d8 & w;
        d->v9 = d9 & w;
        d->v10 = d10 & w;
        d->v11 = d11 & w;
        d->v12 = d12 & w;
        d->v13 = d13 & w;
        d->v14 = d14 & w;
        d->v15 = d15 & w;
    }

    uint32_t gf1131016_invert(gf1131016 *d, const gf1131016 *a);
    int32_t gf1131016_legendre(const gf1131016 *a);
    uint32_t gf1131016_sqrt(gf1131016 *d, const gf1131016 *a);
    void gf1131016_div3(gf1131016 *d, const gf1131016 *a);
    void gf1131016_encode(void *dst, const gf1131016 *a);
    uint32_t gf1131016_decode(gf1131016 *d, const void *src);
    void gf1131016_decode_reduce(gf1131016 *d, const void *src, size_t len);
    void gf1131016_pow(gf1131016 *out, const gf1131016 *a, const uint64_t *e, int nbits);

#ifdef __cplusplus
}
#endif
#endif
