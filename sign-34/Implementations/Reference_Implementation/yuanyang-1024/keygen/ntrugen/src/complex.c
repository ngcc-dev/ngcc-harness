#include "complex.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t
max_size(size_t a, size_t b)
{
    return a > b ? a : b;
}

static void
tmp_bind(bigint *x, uint64_t **p, size_t al)
{
    x->d = *p;
    x->al = al;
    x->n = 0;
    x->s = 1;
    *p += al;
}

void
complex_bind(Complex *z,
             uint64_t *re_buf, size_t re_al,
             uint64_t *im_buf, size_t im_al)
{
    z->re.d = re_buf;
    z->re.al = re_al;
    z->re.n = 0;
    z->re.s = 0;

    z->im.d = im_buf;
    z->im.al = im_al;
    z->im.n = 0;
    z->im.s = 0;

    z->e = 0;
}

void
complex_zero(Complex *z)
{
    bigint_zero(&z->re);
    bigint_zero(&z->im);
    z->e = 0;
}

void
complex_canonicalize(Complex *z)
{
    bigint_trim(&z->re);
    bigint_trim(&z->im);

    if (z->re.n == 0)
        z->re.s = 0;
    if (z->im.n == 0)
        z->im.s = 0;

    if (z->re.n == 0 && z->im.n == 0)
        z->e = 0;
}

void complex_precision_add(Complex *r,int64_t shift){
    r->e-=shift;
//    printf("%zu %zu %ld\n",r->re.al,r->re.n,shift);
    assert((int64_t)r->re.al>=(int64_t)r->re.n+shift);
    assert((int64_t)r->im.al>=(int64_t)r->im.n+shift);
    if(shift>=0){
        size_t sh = (size_t)shift;
        memmove(r->im.d+sh,r->im.d,r->im.n*8);
        memset(r->im.d,0,8*sh);
        r->im.n+=sh;
        memmove(r->re.d+sh,r->re.d,r->re.n*8);
        r->re.n+=sh;
        memset(r->re.d,0,8*sh);
    }
    else{
        size_t sh = (size_t)-shift;
        if(sh<r->re.n)
            r->re.d[sh]+=r->re.d[sh-1]/(1ll<<BIGINT_BASE);
        if(sh<r->im.n)
            r->im.d[sh]+=r->im.d[sh-1]/(1ll<<BIGINT_BASE);
        bigint_shr(&r->re,(int)sh);
        bigint_shr(&r->im,(int)sh);
    }
}

void
complex_copy(Complex *dst, const Complex *src)
{
    assert(dst->re.al >= src->re.n);
    assert(dst->im.al >= src->im.n);

    bigint_copy(&dst->re, &src->re);
    bigint_copy(&dst->im, &src->im);
    dst->e = src->e;
}

void
complex_neg(Complex *r, const Complex *a)
{
    complex_copy(r, a);
    r->re.s = -r->re.s;
    r->im.s = -r->im.s;
}


void complex_print(const Complex *r){
    printf("%d.*(",r->re.s);
    for(size_t i=0;i<r->re.n;i++)
        printf("%lld*x^(%lld)%c",(long long)r->re.d[i],
            (long long)((int64_t)i+r->e),i==r->re.n-1 ? ')' : '+');
    printf("+I*%d.*(",r->im.s);
    for(size_t i=0;i<r->im.n;i++)
        printf("%lld*x^(%lld)%c",(long long)r->im.d[i],
            (long long)((int64_t)i+r->e),i==r->im.n-1 ? ')' : '+');
    puts("");
}

void
complex_truncate(Complex *r, int64_t new_exponent)
{
    if (new_exponent == r->e)
        return;

    if (new_exponent > r->e) {
        int64_t d = new_exponent - r->e;
        size_t sh;
        assert(d <= (int64_t)0x7fffffff);
        sh = (size_t)d;
        if(d>0){
            assert(sh<r->re.al && sh<r->im.al);
            if(r->re.n<=sh){
                if(r->re.n<sh)
                    r->re.d[sh-1]=0;
                r->re.d[sh]=0;
                r->re.n=sh+1;
            }
            if(r->im.n<=sh){
                if(r->re.n<sh)
                    r->re.d[sh-1]=0;
                r->im.n=sh+1;
                r->im.d[sh]=0;
            }
            r->re.d[sh]+=llround(r->re.d[sh-1]*1./(1ll<<BIGINT_BASE));
            r->im.d[sh]+=llround(r->im.d[sh-1]*1./(1ll<<BIGINT_BASE));
        }
        bigint_shr(&r->re, (int)d);
        bigint_shr(&r->im, (int)d);
    } else {
        int64_t d = r->e - new_exponent;
        assert(d <= (int64_t)0x7fffffff);
        bigint_shl(&r->re, (int)d);
        bigint_shl(&r->im, (int)d);
    }

    r->e = new_exponent;
}

void
complex_add(Complex *r, const Complex *a, const Complex *b)
{
    assert(a->e == b->e);
    assert(r->re.al >= max_size(a->re.n, b->re.n) );
    assert(r->im.al >= max_size(a->im.n, b->im.n) );

    bigint_add(&r->re, &a->re, &b->re);
    bigint_add(&r->im, &a->im, &b->im);
    r->e = a->e;
}

void
complex_sub(Complex *r, const Complex *a, const Complex *b)
{
    assert(a->e == b->e);
    assert(r->re.al >= max_size(a->re.n, b->re.n) );
    assert(r->im.al >= max_size(a->im.n, b->im.n) );

    bigint_sub(&r->re, &a->re, &b->re);
    bigint_sub(&r->im, &a->im, &b->im);
    r->e = a->e;
}

/*
size_t
complex_mul_work_size(const Complex *a, const Complex *b)
{
    size_t t1_cap = max_size(a->re.n, a->im.n) + 2;
    size_t t2_cap = max_size(b->re.n, b->im.n) + 2;

    size_t k1_cap = prod_cap(a->re.n, b->re.n);
    size_t k2_cap = prod_cap(a->im.n, b->im.n);
    size_t k3_cap = prod_cap(t1_cap, t2_cap);

    size_t w1 = bigint_karatsuba_work_size(a->re.n, b->re.n);
    size_t w2 = bigint_karatsuba_work_size(a->im.n, b->im.n);
    size_t w3 = bigint_karatsuba_work_size(t1_cap, t2_cap);

    return t1_cap + t2_cap + k1_cap + k2_cap + k3_cap
         + max_size(max_size(w1, w2), w3);
}*/

void
complex_mul(Complex *r,
            const Complex *a,
            const Complex *b,
            uint64_t *work)
{
    size_t t1_cap = max_size(a->re.n, a->im.n) + 2;
    size_t t2_cap = max_size(b->re.n, b->im.n) + 2;

    size_t k1_cap = a->re.n+ b->re.n;
    size_t k2_cap = a->im.n+ b->im.n;
    size_t k3_cap = t1_cap+ t2_cap;

    uint64_t *p = work;

    bigint t1, t2, k1, k2, k3;

    tmp_bind(&t1, &p, t1_cap);
    tmp_bind(&t2, &p, t2_cap);
    tmp_bind(&k1, &p, k1_cap);
    tmp_bind(&k2, &p, k2_cap);
    tmp_bind(&k3, &p, k3_cap);

    uint64_t *kw = p;

    bigint_add(&t1, &a->re, &a->im);
    bigint_add(&t2, &b->re, &b->im);

    k1.n = a->re.n + b->re.n;
    bigint_mul_karatsuba(&a->re, &b->re, &k1, kw, 3);
    k2.n = a->im.n + b->im.n;
    bigint_mul_karatsuba(&a->im, &b->im, &k2, kw, 3);
    k3.n = t1.n + t2.n;
    bigint_mul_karatsuba(&t1, &t2, &k3, kw, 6);
    /*printf("cmul:%zu %zu al=%zu\n",k1.n,k2.n,r->re.al);
    puts("CMUL k1");
    bigint_printx(&a->re);
    bigint_printx(&b->re);
    bigint_printx(&k1);
    puts("CMUL k2");
    bigint_printx(&k2);
    puts("CMUL k3");
    bigint_printx(&k3);

    puts("CMUL re");*/
    bigint_trim(&k1);
    bigint_trim(&k2);
    bigint_trim(&k3);
    bigint_sub(&r->re, &k1, &k2);
//    puts("CMUL fin re");
//    bigint_printx(&r->re);
    bigint_sub(&k3, &k3, &k1);
    bigint_sub(&r->im, &k3, &k2);

    bigint_fast_normalize(&r->re);
//    bigint_printx(&r->re);
    bigint_fast_normalize(&r->im);
    assert(r->re.al>=r->re.n);
    r->e = a->e + b->e;
}
/*
size_t
complex_div_work_size(const Complex *a,
                      const Complex *b,
                      size_t prec_limbs)
{
    size_t num_cap = 0;

    size_t bre2_cap = b->re.n*2;
    size_t bim2_cap = b->im.n*2;
    size_t norm_cap = max_size(bre2_cap, bim2_cap) + 2;
    size_t inv_cap = prec_limbs + 2;

    size_t out_re_cap = prod_cap(num_cap, inv_cap);
    size_t out_im_cap = prod_cap(num_cap, inv_cap);

    size_t w_num = complex_mul_work_size(a, b);
    size_t w_bre2 = bigint_karatsuba_work_size(b->re.n, b->re.n);
    size_t w_bim2 = bigint_karatsuba_work_size(b->im.n, b->im.n);
    size_t w_recip =
        7 * prec_limbs + 14
        + bigint_karatsuba_work_size(prec_limbs + 2, prec_limbs + 2);
    size_t w_ore = bigint_karatsuba_work_size(num_cap, inv_cap);
    size_t w_oim = bigint_karatsuba_work_size(num_cap, inv_cap);

    size_t w = w_num;
    w = max_size(w, w_bre2);
    w = max_size(w, w_bim2);
    w = max_size(w, w_recip);
    w = max_size(w, w_ore);
    w = max_size(w, w_oim);

    return 2 * num_cap
         + bre2_cap
         + bim2_cap
         + norm_cap
         + inv_cap
         + out_re_cap
         + out_im_cap
         + w;
}*/

void
complex_inv(Complex *r,
            const Complex *a,
            size_t prec,
            uint64_t *work)
{
    uint64_t *p = work;

    bigint tmp,tmp2;
    tmp_bind(&tmp, &p, max_size(a->re.n,a->im.n)*2);
    tmp_bind(&tmp2, &p, max_size(prec+1,a->im.n*2));


    tmp.n = 2*a->re.n;
    bigint_mul_karatsuba(&a->re, &a->re, &tmp, p, 4);

    tmp2.n = a->im.n + a->im.n;
    bigint_mul_karatsuba(&a->im, &a->im, &tmp2, p, 4);

    bigint_add(&tmp, &tmp, &tmp2);
    bigint_fast_normalize(&tmp);

    assert(tmp.n != 0);

    bigint_trim(&tmp);
    int shift=tmp.n>prec+1 ? tmp.n-prec-1 : 0;
//    printf("normap=");bigint_printx(&tmp);
    bigint_shr(&tmp,shift);
//    printf("normap=");bigint_printx(&tmp);
    bigint_reciprocal(&tmp2, &tmp,tmp.n+prec,p);
//    printf("%zu invnormap=",tmp.n+prec);bigint_printx(&tmp2);
//    printf("sh=%d e=%ld, tmp=%zu prec=%zu\n",shift,a->e,tmp.n,prec);
    r->re.n = prec + a->re.n+1;
    assert(r->re.al>=r->re.n);
    bigint_mul_karatsuba(&a->re, &tmp2, &r->re, p, 4);

    r->im.n =prec + a->im.n+1;
    assert(r->im.al>=r->im.n);
    bigint_mul_karatsuba(&a->im, &tmp2, &r->im, p, 4);
    bigint_neg(&r->im);
/*    printf("sh=%d e=%ld, tmp=%zu prec=%zu\n",shift,a->e,tmp.n,prec);
    printf("%zu %zu\n",r->re.n,r->im.n);*/
    r->e=-shift-a->e-tmp.n-prec;
}

void
complex_div_pow2(Complex *z, uint64_t k)
{
    uint64_t limbs = k / BIGINT_BASE;
    uint64_t bits = k % BIGINT_BASE;

    z->e -= (int64_t)limbs;
//    printf("bits=%lu \n",bits);

    if (bits != 0) {
        bigint_div_pow2(&z->re, bits);
        bigint_div_pow2(&z->im, bits);
    }
}
