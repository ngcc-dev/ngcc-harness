#include "cfft.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
bind_tmp_complex(Complex *z, uint64_t **wp, size_t cap)
{
    complex_bind(z, *wp, cap, *wp + cap, cap);
    *wp += (size_t)2 * cap;
}

size_t
complex_negacyclic_fft_work_size(const Complex *f,
                                 const Complex *gm)
{
    size_t tmp_cap;

    tmp_cap = f[0].re.al + gm[0].re.al;

    return (size_t)2 * tmp_cap;
}

void
negacyclic_FFT(Complex *f,
                       unsigned logn,
                       const Complex *gm,
                       int subfield,
                       int twiddleprecision,
                       uint64_t *work)
{
    size_t n, hn, t, m;
    unsigned u;
    size_t tmp_cap;
    uint64_t *wp;
    uint64_t *mul_work;
    Complex tmp;

    if (logn == 0) {
        return;
    }
    subfield=!!subfield;

    n = (size_t)1 << logn;
    hn = n >> 1;

    tmp_cap = f[0].re.al + gm[0].re.al;

    wp = work;
    bind_tmp_complex(&tmp, &wp, tmp_cap);
    mul_work = wp;

    t = hn;
    size_t twprec = twiddleprecision > 0 ? (size_t)twiddleprecision : 0;
    size_t subprec = gm[3].re.n > twprec ? gm[3].re.n - twprec : 0;
    Complex s=gm[3];
    s.e+=(int64_t)subprec;
    s.re.n-=subprec;
    s.im.n-=subprec;

    for (u = 1, m = 2; u < logn-subfield; u++, m <<= 1) {
        size_t ht, hm, i1, j1;

        ht = t >> 1;
        hm = m >> 1;

        for (i1 = 0, j1 = 0; i1 < hm; i1++, j1 += t) {
            size_t j, j2;

            j2 = j1 + ht;
            s.re.d = gm[m + i1].re.d+subprec;
            s.im.d = gm[m + i1].im.d+subprec;
            s.re.s = gm[m+i1].re.s;
            s.im.s = gm[m+i1].im.s;

  //          puts("twiddle");
//            complex_print(s);
            for (j = j1; j < j2; j+=1+subfield) {
                int64_t fe;

                fe = f[j].e;

                assert(f[j + ht].e == fe);

                complex_mul(&tmp, &f[j + ht], &s, mul_work);
//                puts("f");
//                complex_print(&f[j+ht]);
                complex_truncate(&tmp, (int)fe);
//                puts("tmp");
//                complex_print(&tmp);

                assert(tmp.e == fe);

                complex_sub(&f[j + ht], &f[j], &tmp);
                complex_add(&f[j], &f[j], &tmp);

                assert(f[j].e == fe);
                assert(f[j + ht].e == fe);
            }
        }

        t = ht;
    }
//    puts("end fft");
}

void
negacyclic_iFFT(Complex *f,
                        unsigned logn,
                        const Complex *gm,
                        int subfield,
                       int twiddleprecision,
                        uint64_t *work)
{
    size_t n, hn, t, m, u;
    size_t tmp_cap;
    uint64_t *wp;
    uint64_t *mul_work;
    Complex tmp;

    if (logn == 0) {
        return;
    }

    subfield=!!subfield;
    n = (size_t)1 << logn;
    hn = n >> 1;

    tmp_cap = f[0].re.al + gm[0].re.al;

    wp = work;
    bind_tmp_complex(&tmp, &wp, tmp_cap);
    mul_work = wp;

    t = 1;
    m = n;

    size_t twprec = twiddleprecision > 0 ? (size_t)twiddleprecision : 0;
    size_t subprec = gm[3].re.n > twprec ? gm[3].re.n - twprec : 0;
    Complex s=gm[3];
    s.e+=(int64_t)subprec;
    s.re.n-=subprec;
    s.im.n-=subprec;
//    printf("%zu %zu %ld\n",s.re.n,s.im.n,s.e);

    for (u = logn; u > 1; u--) {
        size_t hm, dt, i1, j1;

        hm = m >> 1;
        dt = t << 1;
        if(subfield && u==logn){
            t = dt;
            m = hm;
            continue;
        }
        for (i1 = 0, j1 = 0; j1 < hn; i1++, j1 += dt) {
            size_t j, j2;

            j2 = j1 + t;

            s.re.d = gm[hm + i1].re.d+subprec;
            s.im.d = gm[hm + i1].im.d+subprec;
            s.re.s = gm[hm+i1].re.s;
            s.im.s = -gm[hm+i1].im.s;
//            puts("twiddle");
//            complex_print(&s);

            for (j = j1; j < j2; j+=1+subfield) {
                int64_t fe;
                fe = f[j].e;
//                printf("%zu %zu\n",j,j+t);
                assert(f[j + t].e == fe);
                complex_sub(&tmp, &f[j], &f[j + t]);
                assert(tmp.e == fe);
/*                printf("ifft %zu %zu\n",j,j+t);
                printf("fft butter=");
                complex_print(&f[j]);
                complex_print(f+j+t);
                complex_print(&tmp);*/
                complex_add(&f[j], &f[j], &f[j + t]);
                assert(f[j].e == fe);
                complex_mul(&f[j + t], &tmp, &s, mul_work);
//                complex_print(&f[j+t]);
                complex_truncate(&f[j + t], (int)fe);
                assert(f[j + t].e == fe);
            }
        }
/*        puts("recap");
        for (i1 = 0; i1 < hn; i1++)
            complex_print(&f[i1]);*/
        t = dt;
        m = hm;
    }
//    puts("END IFFT");

    if (logn > 1) {
        size_t i;
        uint64_t shift;

        shift = (uint64_t)logn - 1-subfield;

        for (i = 0; i < hn; i++) {
//            complex_print(&f[i]);
            complex_div_pow2(&f[i], shift);
        }
    }
}

static unsigned
bitrev_u(unsigned x, unsigned logn)
{
    unsigned r, u;

    r = 0;
    for (u = 0; u < logn; u++) {
        r = (r << 1) | (x & 1u);
        x >>= 1;
    }

    return r;
}

static int
cos_sign_from_rev(unsigned j, unsigned logn)
{
    size_t n;

    n = (size_t)1 << logn;

    if (j == (unsigned)(n >> 1)) {
        return 0;
    }

    return j < (unsigned)(n >> 1) ? +1 : -1;
}

static int
sin_sign_from_rev(unsigned j)
{
    if (j == 0) {
        return 0;
    }

    return +1;
}

/*
   tw_abs contains 2*n fixed-point bigint magnitudes.

   n = 1 << logn
   tw_al = -e
   e <= 0

   For each k in [0, n):

       j = bit_reverse(k, logn)
       angle = pi * j / n

       tw_abs[(2*k + 0) * tw_al ... (2*k + 1) * tw_al - 1]
           = abs(cos(angle)) * 2^((-e) * BIGINT_BASE)

       tw_abs[(2*k + 1) * tw_al ... (2*k + 2) * tw_al - 1]
           = abs(sin(angle)) * 2^((-e) * BIGINT_BASE)

   Each stored magnitude is a non-negative little-endian bigint with
   exactly tw_al allocated limbs in the source table.

   The resulting Complex is:

       gm[k] = (sre * re + i * sim * im) * 2^(e * BIGINT_BASE)

   Therefore re and im represent fixed-point magnitudes bounded by 1.

   Twiddle order:

       gm[k] = w^bit_reverse(k, logn)
       w = exp(i*pi/n)

   Thus:

       gm[1] = w^(n/2) = i
*/
void
complex_make_twiddles(Complex *gm,
                      unsigned logn,
                      const uint64_t *tw_abs,
                      int64_t e)
{
    size_t n, k;
    size_t tw_al;

    assert(e <= 0);

    n = (size_t)1 << logn;
    tw_al = (size_t)(-e);

    for (k = 0; k < n; k++) {
        unsigned j;

        j = bitrev_u((unsigned)k, logn);

        /*
         * Complex arithmetic never mutates twiddle limbs, but bigint storage is
         * shared with mutable temporaries, so the table binding is cast here.
         */
        gm[k].re.d= (uint64_t *)(tw_abs + ((size_t)2 * k + 0) * tw_al);
        gm[k].im.d = (uint64_t *)(tw_abs + ((size_t)2 * k + 1) * tw_al);
        gm[k].re.al=gm[k].im.al=-e;
        gm[k].re.n = gm[k].im.n = -e;

        gm[k].re.s = cos_sign_from_rev(j, logn);
        gm[k].im.s = sin_sign_from_rev(j);

        gm[k].e = e;

        complex_canonicalize(&gm[k]);
    }
}

void complex_FFT_fold(const Complex *f, unsigned logn,Complex *nf,uint64_t *work){
    int hn=1<<(logn-1),hhn=hn/2;
    for(int i=0;i<hhn;i++){
        complex_mul(&nf[2*i],&f[2*i],&f[2*i+1],work);
        complex_truncate(&nf[2*i],-1);
//        complex_copy(&nf[2*i+1],&nf[2*i]);
    }
}
