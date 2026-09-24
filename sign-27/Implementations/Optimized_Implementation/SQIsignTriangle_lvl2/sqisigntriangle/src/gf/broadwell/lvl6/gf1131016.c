#include "gf1131016.h"

extern void fp_mul(gf1131016 *out, const gf1131016 *a, const gf1131016 *b);
extern void fp_sqr(gf1131016 *out, const gf1131016 *a);

const gf1131016 gf1131016_ZERO = { 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };
const gf1131016 gf1131016_ONE = { 0x0000000000000002, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x1E00000000000000 };
const gf1131016 gf1131016_MINUS_ONE = { 0xFFFFFFFFFFFFFFFD, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x52FFFFFFFFFFFFFF };
static const gf1131016 R2 = { 0xF6F0243F6F0243FC, 0x43F6F0243F6F0243, 0x0243F6F0243F6F02, 0x6F0243F6F0243F6F, 0x3F6F0243F6F0243F, 0x243F6F0243F6F024, 0xF0243F6F0243F6F0, 0xF6F0243F6F0243F6, 0x43F6F0243F6F0243, 0x0243F6F0243F6F02, 0x6F0243F6F0243F6F, 0x3F6F0243F6F0243F, 0x243F6F0243F6F024, 0xF0243F6F0243F6F0, 0xF6F0243F6F0243F6, 0x07F6F0243F6F0243 };
static const gf1131016 MODULUS = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x70FFFFFFFFFFFFFF };
static const gf1131016 PM1O3 = { 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0x25AAAAAAAAAAAAAA };

static const uint64_t EXP_SQRT[16] = { 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x1C40000000000000 };

void gf1131016_pow(gf1131016 *out, const gf1131016 *a, const uint64_t *e, int nbits) {
    gf1131016 r = gf1131016_ONE, t;
    for (int i = nbits - 1; i >= 0; i--) {
        fp_sqr(&r, &r);
        fp_mul(&t, &r, a);
        uint64_t bit = (e[i >> 6] >> (i & 63)) & 1;
        gf1131016_select(&r, &r, &t, (uint32_t)(-(int64_t)bit));
    }
    *out = r;
}

uint32_t gf1131016_sqrt(gf1131016 *d, const gf1131016 *a) {
    gf1131016 y, yn, y2, tmp = *a;
    gf1131016_pow(&y, &tmp, EXP_SQRT, 1021);
    inner_gf1131016_montgomery_reduce(&yn, &y);
    uint32_t ctl = -((uint32_t)yn.v0 & 1);
    gf1131016_neg(&yn, &y);
    gf1131016_select(&y, &y, &yn, ctl);
    fp_sqr(&y2, &y);
    uint32_t r = gf1131016_equals(&y2, a);
    *d = y;
    return r;
}

static const gf1131016 INVT1020 = { 0x0000000000000010, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };

static inline uint64_t sgnw(uint64_t x){ return (uint64_t)(*(int64_t*)&x >> 63); }
#if defined __LZCNT__
#include <immintrin.h>
static inline uint64_t lzcnt(uint64_t x){ return _lzcnt_u64(x); }
#else
static inline uint64_t lzcnt(uint64_t x){
    uint64_t m,s;
    m=sgnw((x>>32)-1); s=m&32; x=(x>>32)^(m&(x^(x>>32)));
    m=sgnw((x>>16)-1); s|=m&16; x=(x>>16)^(m&(x^(x>>16)));
    m=sgnw((x>>8)-1);  s|=m&8;  x=(x>>8)^(m&(x^(x>>8)));
    m=sgnw((x>>4)-1);  s|=m&4;  x=(x>>4)^(m&(x^(x>>4)));
    m=sgnw((x>>2)-1);  s|=m&2;  x=(x>>2)^(m&(x^(x>>2)));
    s+=(2-x)&((x-3)>>2); return s;
}
#endif

static void gf1131016_lin(gf1131016 *d, const gf1131016 *u, const gf1131016 *v, uint64_t fc, uint64_t gc){
    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; gf1131016 tu; gf1131016_neg(&tu,u); gf1131016_select(&tu,u,&tu,(uint32_t)sf);
    uint64_t sg=sgnw(gc); gc=(gc^sg)-sg; gf1131016 tv; gf1131016_neg(&tv,v); gf1131016_select(&tv,v,&tv,(uint32_t)sg);
    uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, t;
    inner_gf1131016_umul_x2(d0, t, tu.v0, fc, tv.v0, gc);
    inner_gf1131016_umul_x2_add(d1, t, tu.v1, fc, tv.v1, gc, t);
    inner_gf1131016_umul_x2_add(d2, t, tu.v2, fc, tv.v2, gc, t);
    inner_gf1131016_umul_x2_add(d3, t, tu.v3, fc, tv.v3, gc, t);
    inner_gf1131016_umul_x2_add(d4, t, tu.v4, fc, tv.v4, gc, t);
    inner_gf1131016_umul_x2_add(d5, t, tu.v5, fc, tv.v5, gc, t);
    inner_gf1131016_umul_x2_add(d6, t, tu.v6, fc, tv.v6, gc, t);
    inner_gf1131016_umul_x2_add(d7, t, tu.v7, fc, tv.v7, gc, t);
    inner_gf1131016_umul_x2_add(d8, t, tu.v8, fc, tv.v8, gc, t);
    inner_gf1131016_umul_x2_add(d9, t, tu.v9, fc, tv.v9, gc, t);
    inner_gf1131016_umul_x2_add(d10, t, tu.v10, fc, tv.v10, gc, t);
    inner_gf1131016_umul_x2_add(d11, t, tu.v11, fc, tv.v11, gc, t);
    inner_gf1131016_umul_x2_add(d12, t, tu.v12, fc, tv.v12, gc, t);
    inner_gf1131016_umul_x2_add(d13, t, tu.v13, fc, tv.v13, gc, t);
    inner_gf1131016_umul_x2_add(d14, t, tu.v14, fc, tv.v14, gc, t);
    inner_gf1131016_umul_x2_add(d15, t, tu.v15, fc, tv.v15, gc, t);
    uint64_t h0=(d15 >> 56)|(t << 8);
    uint64_t h1=t >> 56;
    d15 &= 0x00FFFFFFFFFFFFFF;
    uint64_t z0,z1,quo0,rem0,quo1,rem1;
    inner_gf1131016_umul(z0,z1,h0,0x90FDBC090FDBC091); (void)z0;
    quo0=z1>>6; rem0=h0-(113*quo0);
    quo1=(0x91*h1)>>14; rem1=h1-(113*quo1);
    uint64_t ee,f0,f1; unsigned char cc;
    cc=inner_gf1131016_adc(0, rem0 + 0xFFFFFFFFFFFFFF8F, rem1, &ee);
    cc=inner_gf1131016_adc(cc, quo0, rem1 * 0x0243F6F0243F6F02, &f0);
    cc=inner_gf1131016_adc(cc, quo1, 0, &f1); assert(cc==0);
    ee -= 0xFFFFFFFFFFFFFF8F;
    cc=inner_gf1131016_adc(0, d0, f0, &d0);
    cc=inner_gf1131016_adc(cc, d1, f1, &d1);
    cc=inner_gf1131016_adc(cc, d2, 0, &d2);
    cc=inner_gf1131016_adc(cc, d3, 0, &d3);
    cc=inner_gf1131016_adc(cc, d4, 0, &d4);
    cc=inner_gf1131016_adc(cc, d5, 0, &d5);
    cc=inner_gf1131016_adc(cc, d6, 0, &d6);
    cc=inner_gf1131016_adc(cc, d7, 0, &d7);
    cc=inner_gf1131016_adc(cc, d8, 0, &d8);
    cc=inner_gf1131016_adc(cc, d9, 0, &d9);
    cc=inner_gf1131016_adc(cc, d10, 0, &d10);
    cc=inner_gf1131016_adc(cc, d11, 0, &d11);
    cc=inner_gf1131016_adc(cc, d12, 0, &d12);
    cc=inner_gf1131016_adc(cc, d13, 0, &d13);
    cc=inner_gf1131016_adc(cc, d14, 0, &d14);
    (void)inner_gf1131016_adc(cc, d15, ee << 56, &d15);
    d->v0=d0;
    d->v1=d1;
    d->v2=d2;
    d->v3=d3;
    d->v4=d4;
    d->v5=d5;
    d->v6=d6;
    d->v7=d7;
    d->v8=d8;
    d->v9=d9;
    d->v10=d10;
    d->v11=d11;
    d->v12=d12;
    d->v13=d13;
    d->v14=d14;
    d->v15=d15;
}

static uint64_t gf1131016_lindiv31abs(gf1131016 *d, const gf1131016 *a, const gf1131016 *b, uint64_t fc, uint64_t gc){
    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; uint64_t sg=sgnw(gc); gc=(gc^sg)-sg;
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16;
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16;
    unsigned char cc;
    cc=inner_gf1131016_sbb(0, a->v0 ^ sf, sf, &a0);
    cc=inner_gf1131016_sbb(cc, a->v1 ^ sf, sf, &a1);
    cc=inner_gf1131016_sbb(cc, a->v2 ^ sf, sf, &a2);
    cc=inner_gf1131016_sbb(cc, a->v3 ^ sf, sf, &a3);
    cc=inner_gf1131016_sbb(cc, a->v4 ^ sf, sf, &a4);
    cc=inner_gf1131016_sbb(cc, a->v5 ^ sf, sf, &a5);
    cc=inner_gf1131016_sbb(cc, a->v6 ^ sf, sf, &a6);
    cc=inner_gf1131016_sbb(cc, a->v7 ^ sf, sf, &a7);
    cc=inner_gf1131016_sbb(cc, a->v8 ^ sf, sf, &a8);
    cc=inner_gf1131016_sbb(cc, a->v9 ^ sf, sf, &a9);
    cc=inner_gf1131016_sbb(cc, a->v10 ^ sf, sf, &a10);
    cc=inner_gf1131016_sbb(cc, a->v11 ^ sf, sf, &a11);
    cc=inner_gf1131016_sbb(cc, a->v12 ^ sf, sf, &a12);
    cc=inner_gf1131016_sbb(cc, a->v13 ^ sf, sf, &a13);
    cc=inner_gf1131016_sbb(cc, a->v14 ^ sf, sf, &a14);
    cc=inner_gf1131016_sbb(cc, a->v15 ^ sf, sf, &a15);
    (void)inner_gf1131016_sbb(cc, 0, 0, &a16);
    cc=inner_gf1131016_sbb(0, b->v0 ^ sg, sg, &b0);
    cc=inner_gf1131016_sbb(cc, b->v1 ^ sg, sg, &b1);
    cc=inner_gf1131016_sbb(cc, b->v2 ^ sg, sg, &b2);
    cc=inner_gf1131016_sbb(cc, b->v3 ^ sg, sg, &b3);
    cc=inner_gf1131016_sbb(cc, b->v4 ^ sg, sg, &b4);
    cc=inner_gf1131016_sbb(cc, b->v5 ^ sg, sg, &b5);
    cc=inner_gf1131016_sbb(cc, b->v6 ^ sg, sg, &b6);
    cc=inner_gf1131016_sbb(cc, b->v7 ^ sg, sg, &b7);
    cc=inner_gf1131016_sbb(cc, b->v8 ^ sg, sg, &b8);
    cc=inner_gf1131016_sbb(cc, b->v9 ^ sg, sg, &b9);
    cc=inner_gf1131016_sbb(cc, b->v10 ^ sg, sg, &b10);
    cc=inner_gf1131016_sbb(cc, b->v11 ^ sg, sg, &b11);
    cc=inner_gf1131016_sbb(cc, b->v12 ^ sg, sg, &b12);
    cc=inner_gf1131016_sbb(cc, b->v13 ^ sg, sg, &b13);
    cc=inner_gf1131016_sbb(cc, b->v14 ^ sg, sg, &b14);
    cc=inner_gf1131016_sbb(cc, b->v15 ^ sg, sg, &b15);
    (void)inner_gf1131016_sbb(cc, 0, 0, &b16);
    uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16, t;
    inner_gf1131016_umul_x2(d0, t, a0, fc, b0, gc);
    inner_gf1131016_umul_x2_add(d1, t, a1, fc, b1, gc, t);
    inner_gf1131016_umul_x2_add(d2, t, a2, fc, b2, gc, t);
    inner_gf1131016_umul_x2_add(d3, t, a3, fc, b3, gc, t);
    inner_gf1131016_umul_x2_add(d4, t, a4, fc, b4, gc, t);
    inner_gf1131016_umul_x2_add(d5, t, a5, fc, b5, gc, t);
    inner_gf1131016_umul_x2_add(d6, t, a6, fc, b6, gc, t);
    inner_gf1131016_umul_x2_add(d7, t, a7, fc, b7, gc, t);
    inner_gf1131016_umul_x2_add(d8, t, a8, fc, b8, gc, t);
    inner_gf1131016_umul_x2_add(d9, t, a9, fc, b9, gc, t);
    inner_gf1131016_umul_x2_add(d10, t, a10, fc, b10, gc, t);
    inner_gf1131016_umul_x2_add(d11, t, a11, fc, b11, gc, t);
    inner_gf1131016_umul_x2_add(d12, t, a12, fc, b12, gc, t);
    inner_gf1131016_umul_x2_add(d13, t, a13, fc, b13, gc, t);
    inner_gf1131016_umul_x2_add(d14, t, a14, fc, b14, gc, t);
    inner_gf1131016_umul_x2_add(d15, t, a15, fc, b15, gc, t);
    d16 = t - (a16 & fc) - (b16 & gc);
    d0 = (d0 >> 31)|(d1 << 33);
    d1 = (d1 >> 31)|(d2 << 33);
    d2 = (d2 >> 31)|(d3 << 33);
    d3 = (d3 >> 31)|(d4 << 33);
    d4 = (d4 >> 31)|(d5 << 33);
    d5 = (d5 >> 31)|(d6 << 33);
    d6 = (d6 >> 31)|(d7 << 33);
    d7 = (d7 >> 31)|(d8 << 33);
    d8 = (d8 >> 31)|(d9 << 33);
    d9 = (d9 >> 31)|(d10 << 33);
    d10 = (d10 >> 31)|(d11 << 33);
    d11 = (d11 >> 31)|(d12 << 33);
    d12 = (d12 >> 31)|(d13 << 33);
    d13 = (d13 >> 31)|(d14 << 33);
    d14 = (d14 >> 31)|(d15 << 33);
    d15 = (d15 >> 31)|(d16 << 33);
    t = sgnw(d16);
    cc=inner_gf1131016_sbb(0, d0 ^ t, t, &d0);
    cc=inner_gf1131016_sbb(cc, d1 ^ t, t, &d1);
    cc=inner_gf1131016_sbb(cc, d2 ^ t, t, &d2);
    cc=inner_gf1131016_sbb(cc, d3 ^ t, t, &d3);
    cc=inner_gf1131016_sbb(cc, d4 ^ t, t, &d4);
    cc=inner_gf1131016_sbb(cc, d5 ^ t, t, &d5);
    cc=inner_gf1131016_sbb(cc, d6 ^ t, t, &d6);
    cc=inner_gf1131016_sbb(cc, d7 ^ t, t, &d7);
    cc=inner_gf1131016_sbb(cc, d8 ^ t, t, &d8);
    cc=inner_gf1131016_sbb(cc, d9 ^ t, t, &d9);
    cc=inner_gf1131016_sbb(cc, d10 ^ t, t, &d10);
    cc=inner_gf1131016_sbb(cc, d11 ^ t, t, &d11);
    cc=inner_gf1131016_sbb(cc, d12 ^ t, t, &d12);
    cc=inner_gf1131016_sbb(cc, d13 ^ t, t, &d13);
    cc=inner_gf1131016_sbb(cc, d14 ^ t, t, &d14);
    cc=inner_gf1131016_sbb(cc, d15 ^ t, t, &d15);
    (void)cc;
    d->v0=d0;
    d->v1=d1;
    d->v2=d2;
    d->v3=d3;
    d->v4=d4;
    d->v5=d5;
    d->v6=d6;
    d->v7=d7;
    d->v8=d8;
    d->v9=d9;
    d->v10=d10;
    d->v11=d11;
    d->v12=d12;
    d->v13=d13;
    d->v14=d14;
    d->v15=d15;
    return t;
}

uint32_t gf1131016_div(gf1131016 *d, const gf1131016 *x, const gf1131016 *y){
    gf1131016 a,b,u,v; uint64_t xa,xb,f0,g0,f1,g1; uint32_t r;
    r=~gf1131016_iszero(y); inner_gf1131016_normalize(&a,y); b=MODULUS; u=*x; v=gf1131016_ZERO;
    for(int i=0;i<65;i++){
        uint64_t m15 = a.v15 | b.v15;
        uint64_t m14 = a.v14 | b.v14;
        uint64_t m13 = a.v13 | b.v13;
        uint64_t m12 = a.v12 | b.v12;
        uint64_t m11 = a.v11 | b.v11;
        uint64_t m10 = a.v10 | b.v10;
        uint64_t m9 = a.v9 | b.v9;
        uint64_t m8 = a.v8 | b.v8;
        uint64_t m7 = a.v7 | b.v7;
        uint64_t m6 = a.v6 | b.v6;
        uint64_t m5 = a.v5 | b.v5;
        uint64_t m4 = a.v4 | b.v4;
        uint64_t m3 = a.v3 | b.v3;
        uint64_t m2 = a.v2 | b.v2;
        uint64_t m1 = a.v1 | b.v1;
        uint64_t tnz15 = sgnw(m15 | -m15);
        uint64_t tnz14 = sgnw(m14 | -m14) & ~tnz15;
        uint64_t tnz13 = sgnw(m13 | -m13) & ~tnz15 & ~tnz14;
        uint64_t tnz12 = sgnw(m12 | -m12) & ~tnz15 & ~tnz14 & ~tnz13;
        uint64_t tnz11 = sgnw(m11 | -m11) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12;
        uint64_t tnz10 = sgnw(m10 | -m10) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11;
        uint64_t tnz9 = sgnw(m9 | -m9) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10;
        uint64_t tnz8 = sgnw(m8 | -m8) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9;
        uint64_t tnz7 = sgnw(m7 | -m7) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8;
        uint64_t tnz6 = sgnw(m6 | -m6) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7;
        uint64_t tnz5 = sgnw(m5 | -m5) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6;
        uint64_t tnz4 = sgnw(m4 | -m4) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5;
        uint64_t tnz3 = sgnw(m3 | -m3) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4;
        uint64_t tnz2 = sgnw(m2 | -m2) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4 & ~tnz3;
        uint64_t tnz1 = sgnw(m1 | -m1) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4 & ~tnz3 & ~tnz2;
        uint64_t tnzm = (m15 & tnz15) | (m14 & tnz14) | (m13 & tnz13) | (m12 & tnz12) | (m11 & tnz11) | (m10 & tnz10) | (m9 & tnz9) | (m8 & tnz8) | (m7 & tnz7) | (m6 & tnz6) | (m5 & tnz5) | (m4 & tnz4) | (m3 & tnz3) | (m2 & tnz2) | (m1 & tnz1);
        uint64_t tnza = (a.v15 & tnz15) | (a.v14 & tnz14) | (a.v13 & tnz13) | (a.v12 & tnz12) | (a.v11 & tnz11) | (a.v10 & tnz10) | (a.v9 & tnz9) | (a.v8 & tnz8) | (a.v7 & tnz7) | (a.v6 & tnz6) | (a.v5 & tnz5) | (a.v4 & tnz4) | (a.v3 & tnz3) | (a.v2 & tnz2) | (a.v1 & tnz1);
        uint64_t tnzb = (b.v15 & tnz15) | (b.v14 & tnz14) | (b.v13 & tnz13) | (b.v12 & tnz12) | (b.v11 & tnz11) | (b.v10 & tnz10) | (b.v9 & tnz9) | (b.v8 & tnz8) | (b.v7 & tnz7) | (b.v6 & tnz6) | (b.v5 & tnz5) | (b.v4 & tnz4) | (b.v3 & tnz3) | (b.v2 & tnz2) | (b.v1 & tnz1);
        uint64_t snza = (a.v14 & tnz15) | (a.v13 & tnz14) | (a.v12 & tnz13) | (a.v11 & tnz12) | (a.v10 & tnz11) | (a.v9 & tnz10) | (a.v8 & tnz9) | (a.v7 & tnz8) | (a.v6 & tnz7) | (a.v5 & tnz6) | (a.v4 & tnz5) | (a.v3 & tnz4) | (a.v2 & tnz3) | (a.v1 & tnz2) | (a.v0 & tnz1);
        uint64_t snzb = (b.v14 & tnz15) | (b.v13 & tnz14) | (b.v12 & tnz13) | (b.v11 & tnz12) | (b.v10 & tnz11) | (b.v9 & tnz10) | (b.v8 & tnz9) | (b.v7 & tnz8) | (b.v6 & tnz7) | (b.v5 & tnz6) | (b.v4 & tnz5) | (b.v3 & tnz4) | (b.v2 & tnz3) | (b.v1 & tnz2) | (b.v0 & tnz1);
        int64_t s = lzcnt(tnzm);
        uint64_t sm = (uint64_t)((31 - s) >> 63);
        tnza ^= sm & (tnza ^ ((tnza << 32) | (snza >> 32)));
        tnzb ^= sm & (tnzb ^ ((tnzb << 32) | (snzb >> 32)));
        s -= 32 & sm; tnza <<= s; tnzb <<= s;
        uint64_t tzx = ~(tnz1 | tnz2 | tnz3 | tnz4 | tnz5 | tnz6 | tnz7 | tnz8 | tnz9 | tnz10 | tnz11 | tnz12 | tnz13 | tnz14 | tnz15);
        tnza |= a.v0 & tzx; tnzb |= b.v0 & tzx;
        xa = (a.v0 & 0x7FFFFFFF) | (tnza & 0xFFFFFFFF80000000);
        xb = (b.v0 & 0x7FFFFFFF) | (tnzb & 0xFFFFFFFF80000000);
        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;
        for(int j=0;j<31;j++){
            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf1131016_sbb(0,xa,xb,&t0); (void)inner_gf1131016_sbb(cc,0,0,&swap); swap&=a_odd;
            t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; }
        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;
        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;
        gf1131016 na,nb,nu,nv;
        uint64_t nega=gf1131016_lindiv31abs(&na,&a,&b,f0,g0); uint64_t negb=gf1131016_lindiv31abs(&nb,&a,&b,f1,g1);
        f0=(f0^nega)-nega; g0=(g0^nega)-nega; f1=(f1^negb)-negb; g1=(g1^negb)-negb;
        gf1131016_lin(&nu,&u,&v,f0,g0); gf1131016_lin(&nv,&u,&v,f1,g1); a=na; b=nb; u=nu; v=nv;
    }
    xa=a.v0; xb=b.v0; f0=1; g0=0; f1=0; g1=1;
    for(int j=0;j<29;j++){
        uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;
        a_odd=-(xa&1); cc=inner_gf1131016_sbb(0,xa,xb,&t0); (void)inner_gf1131016_sbb(cc,0,0,&swap); swap&=a_odd;
        t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(f0^f1); f0^=t2; f1^=t2; t3=swap&(g0^g1); g0^=t3; g1^=t3;
        xa-=a_odd&xb; f0-=a_odd&f1; g0-=a_odd&g1; xa>>=1; f1<<=1; g1<<=1; }
    gf1131016_lin(d,&u,&v,f1,g1);
    fp_mul(d,d,&INVT1020);
    return r;
}

uint32_t gf1131016_invert(gf1131016 *d, const gf1131016 *a){ return gf1131016_div(d, &gf1131016_ONE, a); }

int32_t gf1131016_legendre(const gf1131016 *x){
    gf1131016 a,b; uint64_t xa,xb,f0,g0,f1,g1,ls;
    inner_gf1131016_normalize(&a,x); b=MODULUS; ls=0;
    for(int i=0;i<65;i++){
        uint64_t m15 = a.v15 | b.v15;
        uint64_t m14 = a.v14 | b.v14;
        uint64_t m13 = a.v13 | b.v13;
        uint64_t m12 = a.v12 | b.v12;
        uint64_t m11 = a.v11 | b.v11;
        uint64_t m10 = a.v10 | b.v10;
        uint64_t m9 = a.v9 | b.v9;
        uint64_t m8 = a.v8 | b.v8;
        uint64_t m7 = a.v7 | b.v7;
        uint64_t m6 = a.v6 | b.v6;
        uint64_t m5 = a.v5 | b.v5;
        uint64_t m4 = a.v4 | b.v4;
        uint64_t m3 = a.v3 | b.v3;
        uint64_t m2 = a.v2 | b.v2;
        uint64_t m1 = a.v1 | b.v1;
        uint64_t tnz15 = sgnw(m15 | -m15);
        uint64_t tnz14 = sgnw(m14 | -m14) & ~tnz15;
        uint64_t tnz13 = sgnw(m13 | -m13) & ~tnz15 & ~tnz14;
        uint64_t tnz12 = sgnw(m12 | -m12) & ~tnz15 & ~tnz14 & ~tnz13;
        uint64_t tnz11 = sgnw(m11 | -m11) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12;
        uint64_t tnz10 = sgnw(m10 | -m10) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11;
        uint64_t tnz9 = sgnw(m9 | -m9) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10;
        uint64_t tnz8 = sgnw(m8 | -m8) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9;
        uint64_t tnz7 = sgnw(m7 | -m7) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8;
        uint64_t tnz6 = sgnw(m6 | -m6) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7;
        uint64_t tnz5 = sgnw(m5 | -m5) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6;
        uint64_t tnz4 = sgnw(m4 | -m4) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5;
        uint64_t tnz3 = sgnw(m3 | -m3) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4;
        uint64_t tnz2 = sgnw(m2 | -m2) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4 & ~tnz3;
        uint64_t tnz1 = sgnw(m1 | -m1) & ~tnz15 & ~tnz14 & ~tnz13 & ~tnz12 & ~tnz11 & ~tnz10 & ~tnz9 & ~tnz8 & ~tnz7 & ~tnz6 & ~tnz5 & ~tnz4 & ~tnz3 & ~tnz2;
        uint64_t tnzm = (m15 & tnz15) | (m14 & tnz14) | (m13 & tnz13) | (m12 & tnz12) | (m11 & tnz11) | (m10 & tnz10) | (m9 & tnz9) | (m8 & tnz8) | (m7 & tnz7) | (m6 & tnz6) | (m5 & tnz5) | (m4 & tnz4) | (m3 & tnz3) | (m2 & tnz2) | (m1 & tnz1);
        uint64_t tnza = (a.v15 & tnz15) | (a.v14 & tnz14) | (a.v13 & tnz13) | (a.v12 & tnz12) | (a.v11 & tnz11) | (a.v10 & tnz10) | (a.v9 & tnz9) | (a.v8 & tnz8) | (a.v7 & tnz7) | (a.v6 & tnz6) | (a.v5 & tnz5) | (a.v4 & tnz4) | (a.v3 & tnz3) | (a.v2 & tnz2) | (a.v1 & tnz1);
        uint64_t tnzb = (b.v15 & tnz15) | (b.v14 & tnz14) | (b.v13 & tnz13) | (b.v12 & tnz12) | (b.v11 & tnz11) | (b.v10 & tnz10) | (b.v9 & tnz9) | (b.v8 & tnz8) | (b.v7 & tnz7) | (b.v6 & tnz6) | (b.v5 & tnz5) | (b.v4 & tnz4) | (b.v3 & tnz3) | (b.v2 & tnz2) | (b.v1 & tnz1);
        uint64_t snza = (a.v14 & tnz15) | (a.v13 & tnz14) | (a.v12 & tnz13) | (a.v11 & tnz12) | (a.v10 & tnz11) | (a.v9 & tnz10) | (a.v8 & tnz9) | (a.v7 & tnz8) | (a.v6 & tnz7) | (a.v5 & tnz6) | (a.v4 & tnz5) | (a.v3 & tnz4) | (a.v2 & tnz3) | (a.v1 & tnz2) | (a.v0 & tnz1);
        uint64_t snzb = (b.v14 & tnz15) | (b.v13 & tnz14) | (b.v12 & tnz13) | (b.v11 & tnz12) | (b.v10 & tnz11) | (b.v9 & tnz10) | (b.v8 & tnz9) | (b.v7 & tnz8) | (b.v6 & tnz7) | (b.v5 & tnz6) | (b.v4 & tnz5) | (b.v3 & tnz4) | (b.v2 & tnz3) | (b.v1 & tnz2) | (b.v0 & tnz1);
        int64_t s = lzcnt(tnzm);
        uint64_t sm = (uint64_t)((31 - s) >> 63);
        tnza ^= sm & (tnza ^ ((tnza << 32) | (snza >> 32)));
        tnzb ^= sm & (tnzb ^ ((tnzb << 32) | (snzb >> 32)));
        s -= 32 & sm; tnza <<= s; tnzb <<= s;
        uint64_t tzx = ~(tnz1 | tnz2 | tnz3 | tnz4 | tnz5 | tnz6 | tnz7 | tnz8 | tnz9 | tnz10 | tnz11 | tnz12 | tnz13 | tnz14 | tnz15);
        tnza |= a.v0 & tzx; tnzb |= b.v0 & tzx;
        xa = (a.v0 & 0x7FFFFFFF) | (tnza & 0xFFFFFFFF80000000);
        xb = (b.v0 & 0x7FFFFFFF) | (tnzb & 0xFFFFFFFF80000000);
        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;
        for(int j=0;j<29;j++){
            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf1131016_sbb(0,xa,xb,&t0); (void)inner_gf1131016_sbb(cc,0,0,&swap); swap&=a_odd;
            ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; ls^=(xb+2)>>1; }
        uint64_t fg0z=fg0+0x7FFFFFFF7FFFFFFF, fg1z=fg1+0x7FFFFFFF7FFFFFFF;
        f0=(fg0z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0z>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1z>>32)-(uint64_t)0x7FFFFFFF;
        uint64_t a0=(a.v0*f0+b.v0*g0)>>29; uint64_t b0=(a.v0*f1+b.v0*g1)>>29;
        for(int j=0;j<2;j++){
            uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf1131016_sbb(0,xa,xb,&t0); (void)inner_gf1131016_sbb(cc,0,0,&swap); swap&=a_odd;
            ls^=swap&a0&b0; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            t3=swap&(a0^b0); a0^=t3; b0^=t3; xa-=a_odd&xb; fg0-=a_odd&fg1; a0-=a_odd&b0; xa>>=1; fg1<<=1; a0>>=1; ls^=(b0+2)>>1; }
        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;
        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;
        gf1131016 na,nb; uint64_t nega=gf1131016_lindiv31abs(&na,&a,&b,f0,g0); (void)gf1131016_lindiv31abs(&nb,&a,&b,f1,g1);
        ls^=nega&nb.v0; a=na; b=nb;
    }
    xa=a.v0; xb=b.v0;
    for(int j=0;j<29;j++){
        uint64_t a_odd,swap,t0,t1; unsigned char cc;
        a_odd=-(xa&1); cc=inner_gf1131016_sbb(0,xa,xb,&t0); (void)inner_gf1131016_sbb(cc,0,0,&swap); swap&=a_odd;
        ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; xa-=a_odd&xb; xa>>=1; ls^=(xb+2)>>1; }
    uint32_t rr = 1 - ((uint32_t)ls & 2); rr &= ~gf1131016_iszero(x); return *(int32_t*)&rr;
}

void gf1131016_div3(gf1131016 *d, const gf1131016 *a) {
    const digit_t MAGIC = 0xAAAAAAAAAAAAAAAB;
    uint64_t c0, c1, f0, f1;
    gf1131016 t;
    inner_gf1131016_umul(f0, f1, a->arr[15], MAGIC);
    t.arr[15] = f1 >> 1;
    c1 = a->arr[15] - 3 * t.arr[15];
    for (int32_t i = 14; i >= 0; i--) {
        c0 = c1;
        inner_gf1131016_umul(f0, f1, a->arr[i], MAGIC);
        t.arr[i] = f1 >> 1;
        c1 = c0 + a->arr[i] - 3 * t.arr[i];
        t.arr[i] += c0 * ((MAGIC - 1) >> 1);
        f0 = ((c1 >> 1) & c1);
        f1 = ((c1 >> 2) & !(c1 & 0x11));
        f0 |= f1;
        t.arr[i] += f0;
        c1 = c1 - 3 * f0;
    }
    *d = t;
    gf1131016_sub(&t, d, &PM1O3);
    gf1131016_select(d, d, &t, -((c1 & 1) | (c1 >> 1)));
    gf1131016_sub(&t, d, &PM1O3);
    gf1131016_select(d, d, &t, -(c1 == 2));
}

static inline void enc64le(void *dst, uint64_t x){ uint8_t*b=dst; for(int i=0;i<8;i++) b[i]=(uint8_t)(x>>(8*i)); }
static inline uint64_t dec64le(const void *src){ const uint8_t*b=src; uint64_t r=0; for(int i=0;i<8;i++) r|=(uint64_t)b[i]<<(8*i); return r; }

void gf1131016_encode(void *dst, const gf1131016 *a) {
    gf1131016 x; inner_gf1131016_montgomery_reduce(&x, a); uint8_t *buf = dst;
    enc64le(buf + 0, x.v0);
    enc64le(buf + 8, x.v1);
    enc64le(buf + 16, x.v2);
    enc64le(buf + 24, x.v3);
    enc64le(buf + 32, x.v4);
    enc64le(buf + 40, x.v5);
    enc64le(buf + 48, x.v6);
    enc64le(buf + 56, x.v7);
    enc64le(buf + 64, x.v8);
    enc64le(buf + 72, x.v9);
    enc64le(buf + 80, x.v10);
    enc64le(buf + 88, x.v11);
    enc64le(buf + 96, x.v12);
    enc64le(buf + 104, x.v13);
    enc64le(buf + 112, x.v14);
    enc64le(buf + 120, x.v15);
}

uint32_t gf1131016_decode(gf1131016 *d, const void *src) {
    const uint8_t *buf = src; unsigned char cc; uint64_t t;
    uint64_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15;
    d0 = dec64le(buf + 0);
    d1 = dec64le(buf + 8);
    d2 = dec64le(buf + 16);
    d3 = dec64le(buf + 24);
    d4 = dec64le(buf + 32);
    d5 = dec64le(buf + 40);
    d6 = dec64le(buf + 48);
    d7 = dec64le(buf + 56);
    d8 = dec64le(buf + 64);
    d9 = dec64le(buf + 72);
    d10 = dec64le(buf + 80);
    d11 = dec64le(buf + 88);
    d12 = dec64le(buf + 96);
    d13 = dec64le(buf + 104);
    d14 = dec64le(buf + 112);
    d15 = dec64le(buf + 120);
    cc = inner_gf1131016_sbb(0, d0, MODULUS.v0, &t);
    cc = inner_gf1131016_sbb(cc, d1, MODULUS.v1, &t);
    cc = inner_gf1131016_sbb(cc, d2, MODULUS.v2, &t);
    cc = inner_gf1131016_sbb(cc, d3, MODULUS.v3, &t);
    cc = inner_gf1131016_sbb(cc, d4, MODULUS.v4, &t);
    cc = inner_gf1131016_sbb(cc, d5, MODULUS.v5, &t);
    cc = inner_gf1131016_sbb(cc, d6, MODULUS.v6, &t);
    cc = inner_gf1131016_sbb(cc, d7, MODULUS.v7, &t);
    cc = inner_gf1131016_sbb(cc, d8, MODULUS.v8, &t);
    cc = inner_gf1131016_sbb(cc, d9, MODULUS.v9, &t);
    cc = inner_gf1131016_sbb(cc, d10, MODULUS.v10, &t);
    cc = inner_gf1131016_sbb(cc, d11, MODULUS.v11, &t);
    cc = inner_gf1131016_sbb(cc, d12, MODULUS.v12, &t);
    cc = inner_gf1131016_sbb(cc, d13, MODULUS.v13, &t);
    cc = inner_gf1131016_sbb(cc, d14, MODULUS.v14, &t);
    cc = inner_gf1131016_sbb(cc, d15, MODULUS.v15, &t);
    (void)inner_gf1131016_sbb(cc, 0, 0, &t);
    d->v0 = d0 & t;
    d->v1 = d1 & t;
    d->v2 = d2 & t;
    d->v3 = d3 & t;
    d->v4 = d4 & t;
    d->v5 = d5 & t;
    d->v6 = d6 & t;
    d->v7 = d7 & t;
    d->v8 = d8 & t;
    d->v9 = d9 & t;
    d->v10 = d10 & t;
    d->v11 = d11 & t;
    d->v12 = d12 & t;
    d->v13 = d13 & t;
    d->v14 = d14 & t;
    d->v15 = d15 & t;
    fp_mul(d, d, &R2);
    return (uint32_t)t;
}

void gf1131016_decode_reduce(gf1131016 *d, const void *src, size_t len) {
    const uint8_t *buf = src;
    *d = gf1131016_ZERO;
    if (len == 0) return;
    size_t rem = len % 128;
    if (rem != 0) {
        uint8_t tmp[128]; size_t k = len - rem;
        memcpy(tmp, buf + k, len - k); memset(tmp + len - k, 0, sizeof(tmp) - (len - k));
        d->v0 = dec64le(&tmp[0]);
        d->v1 = dec64le(&tmp[8]);
        d->v2 = dec64le(&tmp[16]);
        d->v3 = dec64le(&tmp[24]);
        d->v4 = dec64le(&tmp[32]);
        d->v5 = dec64le(&tmp[40]);
        d->v6 = dec64le(&tmp[48]);
        d->v7 = dec64le(&tmp[56]);
        d->v8 = dec64le(&tmp[64]);
        d->v9 = dec64le(&tmp[72]);
        d->v10 = dec64le(&tmp[80]);
        d->v11 = dec64le(&tmp[88]);
        d->v12 = dec64le(&tmp[96]);
        d->v13 = dec64le(&tmp[104]);
        d->v14 = dec64le(&tmp[112]);
        d->v15 = dec64le(&tmp[120]);
        len = k;
    } else {
        len -= 128;
        uint64_t b0 = dec64le(buf + len + 0);
        uint64_t b1 = dec64le(buf + len + 8);
        uint64_t b2 = dec64le(buf + len + 16);
        uint64_t b3 = dec64le(buf + len + 24);
        uint64_t b4 = dec64le(buf + len + 32);
        uint64_t b5 = dec64le(buf + len + 40);
        uint64_t b6 = dec64le(buf + len + 48);
        uint64_t b7 = dec64le(buf + len + 56);
        uint64_t b8 = dec64le(buf + len + 64);
        uint64_t b9 = dec64le(buf + len + 72);
        uint64_t b10 = dec64le(buf + len + 80);
        uint64_t b11 = dec64le(buf + len + 88);
        uint64_t b12 = dec64le(buf + len + 96);
        uint64_t b13 = dec64le(buf + len + 104);
        uint64_t b14 = dec64le(buf + len + 112);
        uint64_t b15 = dec64le(buf + len + 120);
        inner_gf1131016_partial_reduce(d, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15);
    }
    while (len > 0) {
        fp_mul(d, d, &R2); len -= 128;
        uint64_t t0 = dec64le(buf + len + 0);
        uint64_t t1 = dec64le(buf + len + 8);
        uint64_t t2 = dec64le(buf + len + 16);
        uint64_t t3 = dec64le(buf + len + 24);
        uint64_t t4 = dec64le(buf + len + 32);
        uint64_t t5 = dec64le(buf + len + 40);
        uint64_t t6 = dec64le(buf + len + 48);
        uint64_t t7 = dec64le(buf + len + 56);
        uint64_t t8 = dec64le(buf + len + 64);
        uint64_t t9 = dec64le(buf + len + 72);
        uint64_t t10 = dec64le(buf + len + 80);
        uint64_t t11 = dec64le(buf + len + 88);
        uint64_t t12 = dec64le(buf + len + 96);
        uint64_t t13 = dec64le(buf + len + 104);
        uint64_t t14 = dec64le(buf + len + 112);
        uint64_t t15 = dec64le(buf + len + 120);
        gf1131016 t; inner_gf1131016_partial_reduce(&t, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15);
        gf1131016_add(d, d, &t);
    }
    fp_mul(d, d, &R2);
}
