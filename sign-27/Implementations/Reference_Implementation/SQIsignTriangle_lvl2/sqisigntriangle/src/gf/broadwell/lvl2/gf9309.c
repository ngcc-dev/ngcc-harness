#include "gf9309.h"

extern void fp_mul(gf9309 *out, const gf9309 *a, const gf9309 *b);
extern void fp_sqr(gf9309 *out, const gf9309 *a);

const gf9309 gf9309_ZERO = { 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };
const gf9309 gf9309_ONE = { 0x00000000000000E3, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x00A0000000000000 };
const gf9309 gf9309_MINUS_ONE = { 0xFFFFFFFFFFFFFF1C, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF };
static const gf9309 R2 = { 0xE38E38E38E39ADD3, 0x8E38E38E38E38E38, 0x38E38E38E38E38E3, 0xE38E38E38E38E38E, 0x00D8E38E38E38E38 };
static const gf9309 MODULUS = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x011FFFFFFFFFFFFF };
static const gf9309 PM1O3 = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x00BFFFFFFFFFFFFF };

static const uint64_t EXP_SQRT[5] = { 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0048000000000000 };

void gf9309_pow(gf9309 *out, const gf9309 *a, const uint64_t *e, int nbits) {
    gf9309 r = gf9309_ONE, t;
    for (int i = nbits - 1; i >= 0; i--) {
        fp_sqr(&r, &r);
        fp_mul(&t, &r, a);
        uint64_t bit = (e[i >> 6] >> (i & 63)) & 1;
        gf9309_select(&r, &r, &t, (uint32_t)(-(int64_t)bit));
    }
    *out = r;
}

uint32_t gf9309_sqrt(gf9309 *d, const gf9309 *a) {
    gf9309 y, yn, y2, tmp = *a;
    gf9309_pow(&y, &tmp, EXP_SQRT, 311);
    inner_gf9309_montgomery_reduce(&yn, &y);
    uint32_t ctl = -((uint32_t)yn.v0 & 1);
    gf9309_neg(&yn, &y);
    gf9309_select(&y, &y, &yn, ctl);
    fp_sqr(&y2, &y);
    uint32_t r = gf9309_equals(&y2, a);
    *d = y;
    return r;
}

static const gf9309 INVT304 = { 0x0000000000010000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };

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

static void gf9309_lin(gf9309 *d, const gf9309 *u, const gf9309 *v, uint64_t fc, uint64_t gc){
    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; gf9309 tu; gf9309_neg(&tu,u); gf9309_select(&tu,u,&tu,(uint32_t)sf);
    uint64_t sg=sgnw(gc); gc=(gc^sg)-sg; gf9309 tv; gf9309_neg(&tv,v); gf9309_select(&tv,v,&tv,(uint32_t)sg);
    uint64_t d0, d1, d2, d3, d4, t;
    inner_gf9309_umul_x2(d0, t, tu.v0, fc, tv.v0, gc);
    inner_gf9309_umul_x2_add(d1, t, tu.v1, fc, tv.v1, gc, t);
    inner_gf9309_umul_x2_add(d2, t, tu.v2, fc, tv.v2, gc, t);
    inner_gf9309_umul_x2_add(d3, t, tu.v3, fc, tv.v3, gc, t);
    inner_gf9309_umul_x2_add(d4, t, tu.v4, fc, tv.v4, gc, t);
    uint64_t h0=(d4 >> 53)|(t << 11);
    uint64_t h1=t >> 53;
    d4 &= 0x001FFFFFFFFFFFFF;
    uint64_t z0,z1,quo0,rem0,quo1,rem1;
    inner_gf9309_umul(z0,z1,h0,0xE38E38E38E38E38F); (void)z0;
    quo0=z1>>3; rem0=h0-(9*quo0);
    quo1=(0x71D*h1)>>14; rem1=h1-(9*quo1);
    uint64_t ee,f0,f1; unsigned char cc;
    cc=inner_gf9309_adc(0, rem0 + 0xFFFFFFFFFFFFFFF7, rem1, &ee);
    cc=inner_gf9309_adc(cc, quo0, rem1 * 0x1C71C71C71C71C71, &f0);
    cc=inner_gf9309_adc(cc, quo1, 0, &f1); assert(cc==0);
    ee -= 0xFFFFFFFFFFFFFFF7;
    cc=inner_gf9309_adc(0, d0, f0, &d0);
    cc=inner_gf9309_adc(cc, d1, f1, &d1);
    cc=inner_gf9309_adc(cc, d2, 0, &d2);
    cc=inner_gf9309_adc(cc, d3, 0, &d3);
    (void)inner_gf9309_adc(cc, d4, ee << 53, &d4);
    d->v0=d0;
    d->v1=d1;
    d->v2=d2;
    d->v3=d3;
    d->v4=d4;
}

static uint64_t gf9309_lindiv31abs(gf9309 *d, const gf9309 *a, const gf9309 *b, uint64_t fc, uint64_t gc){
    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; uint64_t sg=sgnw(gc); gc=(gc^sg)-sg;
    uint64_t a0, a1, a2, a3, a4, a5;
    uint64_t b0, b1, b2, b3, b4, b5;
    unsigned char cc;
    cc=inner_gf9309_sbb(0, a->v0 ^ sf, sf, &a0);
    cc=inner_gf9309_sbb(cc, a->v1 ^ sf, sf, &a1);
    cc=inner_gf9309_sbb(cc, a->v2 ^ sf, sf, &a2);
    cc=inner_gf9309_sbb(cc, a->v3 ^ sf, sf, &a3);
    cc=inner_gf9309_sbb(cc, a->v4 ^ sf, sf, &a4);
    (void)inner_gf9309_sbb(cc, 0, 0, &a5);
    cc=inner_gf9309_sbb(0, b->v0 ^ sg, sg, &b0);
    cc=inner_gf9309_sbb(cc, b->v1 ^ sg, sg, &b1);
    cc=inner_gf9309_sbb(cc, b->v2 ^ sg, sg, &b2);
    cc=inner_gf9309_sbb(cc, b->v3 ^ sg, sg, &b3);
    cc=inner_gf9309_sbb(cc, b->v4 ^ sg, sg, &b4);
    (void)inner_gf9309_sbb(cc, 0, 0, &b5);
    uint64_t d0, d1, d2, d3, d4, d5, t;
    inner_gf9309_umul_x2(d0, t, a0, fc, b0, gc);
    inner_gf9309_umul_x2_add(d1, t, a1, fc, b1, gc, t);
    inner_gf9309_umul_x2_add(d2, t, a2, fc, b2, gc, t);
    inner_gf9309_umul_x2_add(d3, t, a3, fc, b3, gc, t);
    inner_gf9309_umul_x2_add(d4, t, a4, fc, b4, gc, t);
    d5 = t - (a5 & fc) - (b5 & gc);
    d0 = (d0 >> 31)|(d1 << 33);
    d1 = (d1 >> 31)|(d2 << 33);
    d2 = (d2 >> 31)|(d3 << 33);
    d3 = (d3 >> 31)|(d4 << 33);
    d4 = (d4 >> 31)|(d5 << 33);
    t = sgnw(d5);
    cc=inner_gf9309_sbb(0, d0 ^ t, t, &d0);
    cc=inner_gf9309_sbb(cc, d1 ^ t, t, &d1);
    cc=inner_gf9309_sbb(cc, d2 ^ t, t, &d2);
    cc=inner_gf9309_sbb(cc, d3 ^ t, t, &d3);
    cc=inner_gf9309_sbb(cc, d4 ^ t, t, &d4);
    (void)cc;
    d->v0=d0;
    d->v1=d1;
    d->v2=d2;
    d->v3=d3;
    d->v4=d4;
    return t;
}

uint32_t gf9309_div(gf9309 *d, const gf9309 *x, const gf9309 *y){
    gf9309 a,b,u,v; uint64_t xa,xb,f0,g0,f1,g1; uint32_t r;
    r=~gf9309_iszero(y); inner_gf9309_normalize(&a,y); b=MODULUS; u=*x; v=gf9309_ZERO;
    for(int i=0;i<19;i++){
        uint64_t m4 = a.v4 | b.v4;
        uint64_t m3 = a.v3 | b.v3;
        uint64_t m2 = a.v2 | b.v2;
        uint64_t m1 = a.v1 | b.v1;
        uint64_t tnz4 = sgnw(m4 | -m4);
        uint64_t tnz3 = sgnw(m3 | -m3) & ~tnz4;
        uint64_t tnz2 = sgnw(m2 | -m2) & ~tnz4 & ~tnz3;
        uint64_t tnz1 = sgnw(m1 | -m1) & ~tnz4 & ~tnz3 & ~tnz2;
        uint64_t tnzm = (m4 & tnz4) | (m3 & tnz3) | (m2 & tnz2) | (m1 & tnz1);
        uint64_t tnza = (a.v4 & tnz4) | (a.v3 & tnz3) | (a.v2 & tnz2) | (a.v1 & tnz1);
        uint64_t tnzb = (b.v4 & tnz4) | (b.v3 & tnz3) | (b.v2 & tnz2) | (b.v1 & tnz1);
        uint64_t snza = (a.v3 & tnz4) | (a.v2 & tnz3) | (a.v1 & tnz2) | (a.v0 & tnz1);
        uint64_t snzb = (b.v3 & tnz4) | (b.v2 & tnz3) | (b.v1 & tnz2) | (b.v0 & tnz1);
        int64_t s = lzcnt(tnzm);
        uint64_t sm = (uint64_t)((31 - s) >> 63);
        tnza ^= sm & (tnza ^ ((tnza << 32) | (snza >> 32)));
        tnzb ^= sm & (tnzb ^ ((tnzb << 32) | (snzb >> 32)));
        s -= 32 & sm; tnza <<= s; tnzb <<= s;
        uint64_t tzx = ~(tnz1 | tnz2 | tnz3 | tnz4);
        tnza |= a.v0 & tzx; tnzb |= b.v0 & tzx;
        xa = (a.v0 & 0x7FFFFFFF) | (tnza & 0xFFFFFFFF80000000);
        xb = (b.v0 & 0x7FFFFFFF) | (tnzb & 0xFFFFFFFF80000000);
        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;
        for(int j=0;j<31;j++){
            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf9309_sbb(0,xa,xb,&t0); (void)inner_gf9309_sbb(cc,0,0,&swap); swap&=a_odd;
            t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; }
        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;
        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;
        gf9309 na,nb,nu,nv;
        uint64_t nega=gf9309_lindiv31abs(&na,&a,&b,f0,g0); uint64_t negb=gf9309_lindiv31abs(&nb,&a,&b,f1,g1);
        f0=(f0^nega)-nega; g0=(g0^nega)-nega; f1=(f1^negb)-negb; g1=(g1^negb)-negb;
        gf9309_lin(&nu,&u,&v,f0,g0); gf9309_lin(&nv,&u,&v,f1,g1); a=na; b=nb; u=nu; v=nv;
    }
    xa=a.v0; xb=b.v0; f0=1; g0=0; f1=0; g1=1;
    for(int j=0;j<35;j++){
        uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;
        a_odd=-(xa&1); cc=inner_gf9309_sbb(0,xa,xb,&t0); (void)inner_gf9309_sbb(cc,0,0,&swap); swap&=a_odd;
        t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(f0^f1); f0^=t2; f1^=t2; t3=swap&(g0^g1); g0^=t3; g1^=t3;
        xa-=a_odd&xb; f0-=a_odd&f1; g0-=a_odd&g1; xa>>=1; f1<<=1; g1<<=1; }
    gf9309_lin(d,&u,&v,f1,g1);
    fp_mul(d,d,&INVT304);
    return r;
}

uint32_t gf9309_invert(gf9309 *d, const gf9309 *a){ return gf9309_div(d, &gf9309_ONE, a); }

int32_t gf9309_legendre(const gf9309 *x){
    gf9309 a,b; uint64_t xa,xb,f0,g0,f1,g1,ls;
    inner_gf9309_normalize(&a,x); b=MODULUS; ls=0;
    for(int i=0;i<19;i++){
        uint64_t m4 = a.v4 | b.v4;
        uint64_t m3 = a.v3 | b.v3;
        uint64_t m2 = a.v2 | b.v2;
        uint64_t m1 = a.v1 | b.v1;
        uint64_t tnz4 = sgnw(m4 | -m4);
        uint64_t tnz3 = sgnw(m3 | -m3) & ~tnz4;
        uint64_t tnz2 = sgnw(m2 | -m2) & ~tnz4 & ~tnz3;
        uint64_t tnz1 = sgnw(m1 | -m1) & ~tnz4 & ~tnz3 & ~tnz2;
        uint64_t tnzm = (m4 & tnz4) | (m3 & tnz3) | (m2 & tnz2) | (m1 & tnz1);
        uint64_t tnza = (a.v4 & tnz4) | (a.v3 & tnz3) | (a.v2 & tnz2) | (a.v1 & tnz1);
        uint64_t tnzb = (b.v4 & tnz4) | (b.v3 & tnz3) | (b.v2 & tnz2) | (b.v1 & tnz1);
        uint64_t snza = (a.v3 & tnz4) | (a.v2 & tnz3) | (a.v1 & tnz2) | (a.v0 & tnz1);
        uint64_t snzb = (b.v3 & tnz4) | (b.v2 & tnz3) | (b.v1 & tnz2) | (b.v0 & tnz1);
        int64_t s = lzcnt(tnzm);
        uint64_t sm = (uint64_t)((31 - s) >> 63);
        tnza ^= sm & (tnza ^ ((tnza << 32) | (snza >> 32)));
        tnzb ^= sm & (tnzb ^ ((tnzb << 32) | (snzb >> 32)));
        s -= 32 & sm; tnza <<= s; tnzb <<= s;
        uint64_t tzx = ~(tnz1 | tnz2 | tnz3 | tnz4);
        tnza |= a.v0 & tzx; tnzb |= b.v0 & tzx;
        xa = (a.v0 & 0x7FFFFFFF) | (tnza & 0xFFFFFFFF80000000);
        xb = (b.v0 & 0x7FFFFFFF) | (tnzb & 0xFFFFFFFF80000000);
        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;
        for(int j=0;j<29;j++){
            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf9309_sbb(0,xa,xb,&t0); (void)inner_gf9309_sbb(cc,0,0,&swap); swap&=a_odd;
            ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; ls^=(xb+2)>>1; }
        uint64_t fg0z=fg0+0x7FFFFFFF7FFFFFFF, fg1z=fg1+0x7FFFFFFF7FFFFFFF;
        f0=(fg0z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0z>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1z>>32)-(uint64_t)0x7FFFFFFF;
        uint64_t a0=(a.v0*f0+b.v0*g0)>>29; uint64_t b0=(a.v0*f1+b.v0*g1)>>29;
        for(int j=0;j<2;j++){
            uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;
            a_odd=-(xa&1); cc=inner_gf9309_sbb(0,xa,xb,&t0); (void)inner_gf9309_sbb(cc,0,0,&swap); swap&=a_odd;
            ls^=swap&a0&b0; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;
            t3=swap&(a0^b0); a0^=t3; b0^=t3; xa-=a_odd&xb; fg0-=a_odd&fg1; a0-=a_odd&b0; xa>>=1; fg1<<=1; a0>>=1; ls^=(b0+2)>>1; }
        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;
        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;
        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;
        gf9309 na,nb; uint64_t nega=gf9309_lindiv31abs(&na,&a,&b,f0,g0); (void)gf9309_lindiv31abs(&nb,&a,&b,f1,g1);
        ls^=nega&nb.v0; a=na; b=nb;
    }
    xa=a.v0; xb=b.v0;
    for(int j=0;j<35;j++){
        uint64_t a_odd,swap,t0,t1; unsigned char cc;
        a_odd=-(xa&1); cc=inner_gf9309_sbb(0,xa,xb,&t0); (void)inner_gf9309_sbb(cc,0,0,&swap); swap&=a_odd;
        ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; xa-=a_odd&xb; xa>>=1; ls^=(xb+2)>>1; }
    uint32_t rr = 1 - ((uint32_t)ls & 2); rr &= ~gf9309_iszero(x); return *(int32_t*)&rr;
}

void gf9309_div3(gf9309 *d, const gf9309 *a) {
    const digit_t MAGIC = 0xAAAAAAAAAAAAAAAB;
    uint64_t c0, c1, f0, f1;
    gf9309 t;
    inner_gf9309_umul(f0, f1, a->arr[4], MAGIC);
    t.arr[4] = f1 >> 1;
    c1 = a->arr[4] - 3 * t.arr[4];
    for (int32_t i = 3; i >= 0; i--) {
        c0 = c1;
        inner_gf9309_umul(f0, f1, a->arr[i], MAGIC);
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
    gf9309_sub(&t, d, &PM1O3);
    gf9309_select(d, d, &t, -((c1 & 1) | (c1 >> 1)));
    gf9309_sub(&t, d, &PM1O3);
    gf9309_select(d, d, &t, -(c1 == 2));
}

static inline void enc64le(void *dst, uint64_t x){ uint8_t*b=dst; for(int i=0;i<8;i++) b[i]=(uint8_t)(x>>(8*i)); }
static inline uint64_t dec64le(const void *src){ const uint8_t*b=src; uint64_t r=0; for(int i=0;i<8;i++) r|=(uint64_t)b[i]<<(8*i); return r; }

void gf9309_encode(void *dst, const gf9309 *a) {
    gf9309 x; inner_gf9309_montgomery_reduce(&x, a); uint8_t *buf = dst;
    enc64le(buf + 0, x.v0);
    enc64le(buf + 8, x.v1);
    enc64le(buf + 16, x.v2);
    enc64le(buf + 24, x.v3);
    enc64le(buf + 32, x.v4);
}

uint32_t gf9309_decode(gf9309 *d, const void *src) {
    const uint8_t *buf = src; unsigned char cc; uint64_t t;
    uint64_t d0, d1, d2, d3, d4;
    d0 = dec64le(buf + 0);
    d1 = dec64le(buf + 8);
    d2 = dec64le(buf + 16);
    d3 = dec64le(buf + 24);
    d4 = dec64le(buf + 32);
    cc = inner_gf9309_sbb(0, d0, MODULUS.v0, &t);
    cc = inner_gf9309_sbb(cc, d1, MODULUS.v1, &t);
    cc = inner_gf9309_sbb(cc, d2, MODULUS.v2, &t);
    cc = inner_gf9309_sbb(cc, d3, MODULUS.v3, &t);
    cc = inner_gf9309_sbb(cc, d4, MODULUS.v4, &t);
    (void)inner_gf9309_sbb(cc, 0, 0, &t);
    d->v0 = d0 & t;
    d->v1 = d1 & t;
    d->v2 = d2 & t;
    d->v3 = d3 & t;
    d->v4 = d4 & t;
    fp_mul(d, d, &R2);
    return (uint32_t)t;
}

void gf9309_decode_reduce(gf9309 *d, const void *src, size_t len) {
    const uint8_t *buf = src;
    *d = gf9309_ZERO;
    if (len == 0) return;
    size_t rem = len % 40;
    if (rem != 0) {
        uint8_t tmp[40]; size_t k = len - rem;
        memcpy(tmp, buf + k, len - k); memset(tmp + len - k, 0, sizeof(tmp) - (len - k));
        d->v0 = dec64le(&tmp[0]);
        d->v1 = dec64le(&tmp[8]);
        d->v2 = dec64le(&tmp[16]);
        d->v3 = dec64le(&tmp[24]);
        d->v4 = dec64le(&tmp[32]);
        len = k;
    } else {
        len -= 40;
        uint64_t b0 = dec64le(buf + len + 0);
        uint64_t b1 = dec64le(buf + len + 8);
        uint64_t b2 = dec64le(buf + len + 16);
        uint64_t b3 = dec64le(buf + len + 24);
        uint64_t b4 = dec64le(buf + len + 32);
        inner_gf9309_partial_reduce(d, b0, b1, b2, b3, b4);
    }
    while (len > 0) {
        fp_mul(d, d, &R2); len -= 40;
        uint64_t t0 = dec64le(buf + len + 0);
        uint64_t t1 = dec64le(buf + len + 8);
        uint64_t t2 = dec64le(buf + len + 16);
        uint64_t t3 = dec64le(buf + len + 24);
        uint64_t t4 = dec64le(buf + len + 32);
        gf9309 t; inner_gf9309_partial_reduce(&t, t0, t1, t2, t3, t4);
        gf9309_add(d, d, &t);
    }
    fp_mul(d, d, &R2);
}
