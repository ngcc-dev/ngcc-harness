#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "bigint.h"
#define BASE BIGINT_BASE
#define LAT_LIMIT (1ll<<33)

typedef struct {
    int64_t u00, u01;
    int64_t u10, u11;
} mat2_i64;

#if defined(__GNUC__) || defined(__clang__)
    /* GCC and Clang provide __builtin_expect */
    #define unlikely(x) __builtin_expect(!!(x), 0)
#elif defined(__INTEL_COMPILER) || defined(__ICC)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    /* MSVC does not have a direct equivalent; __assume is the closest,
       but it is semantically different. We leave it as a no-op. */
    #define unlikely(x) (x)
#else
    #define unlikely(x) (x)
#endif

mat2_i64
latred(long double a, long double b,int end)
{
    long double x=a,y=b,z,w,q,quo=a/b;
    int64_t xu=1,yu=0,tu1,tu2;
    int64_t limit=fabsl(quo)>1 ? LAT_LIMIT/fabsl(quo) : LAT_LIMIT;
    mat2_i64 U ;
    if(b==0)
        return (mat2_i64){1,0,0,1};
    while(1){
        q=roundl(x/y);
        tu1=xu-q*yu;
        z=x-y*q;
        if(unlikely(labs(tu1)>limit) || unlikely(z==0)){
//        printf("%Lg %Lg %Lg\n",x,y,z);
            U.u00=xu;
            U.u01=yu;
            U.u10=roundl((x-xu*a)/b);
            U.u11=roundl((y-yu*a)/b);
            if(z==0 && end){
                U.u00=tu1;
                U.u10=roundl(-tu1*a/b);
            }
            return U;
        }
        q=roundl(y/z);
        tu2=yu-q*tu1;
        w=y-z*q;
//        printf("%Lg %Lg %Lg %Lg %d\n",x,y,z,w,w==0);
        if(unlikely(labs(tu2)>limit) || unlikely(w==0)){
            U.u00=yu;
            U.u01=tu1;
            U.u10=roundl((y-yu*a)/b);
            U.u11=roundl((z-tu1*a)/b);
            if(w==0 && end){
                U.u00=tu2;
                U.u10=roundl(-tu2*a/b);
            }
            return U;
        }
        y=w;
        x=z;
        xu=tu1;
        yu=tu2;

    }
}


void
bigint_mul_i64_shift(bigint *r, const bigint *a, int64_t c, size_t sh)
{
    if (c == 0 || a->n == 0){
        r->n=0;
        return;
    }
    bigint_mul_scalar(r, a, c);
    if(sh)
	    bigint_shl(r, (int)sh);
}

void
bigint_addmul_i64_shift(bigint *r, const bigint *a, int64_t c, size_t sh,
                        bigint *t)
{
    bigint_mul_i64_shift(t, a, c, sh);
    bigint_add(r, r, t);
    bigint_trim(r);
}

long double bigint_top(bigint *a){
    bigint_trim(a);
    long double r=0;
    int i=a->n-1;
    for(;i>=0 && i>=(int)a->n-3;i--){
        r+=a->d[i]*pow(2.,(i-(int)a->n+1)*BASE*1.);
//        printf("%d %Lg %lg\n",i,r,a->d[i]*pow(2.,(i-(int)a->n+1)*BASE*1.));
    }
    return r*a->s;
}

static void
bigint_lincomb_into(bigint *r, const bigint *a, int64_t ca,
                    const bigint *b, int64_t cb, bigint *t)
{
    bigint_mul_i64_shift(r, a, ca, 0);
    bigint_addmul_i64_shift(r, b, cb, 0, t);
    bigint_trim(r);
}

static void
bigint_apply_matrix_pair(bigint *x, bigint *y, mat2_i64 M,
                         bigint *t0, bigint *t1,bigint *t2)
{
    bigint_lincomb_into(t0, x, M.u00, y, M.u10, t2);
    bigint_lincomb_into(t1, x, M.u01, y, M.u11, t2);
    bigint T = *x; *x = *t0; *t0 = T;
    T = *y; *y = *t1; *t1 = T;
}


static void
bigint_view(bigint *a, uint64_t **work, size_t al)
{
    a->n = 0;
    a->al = al;
    a->s = 1;
    a->d = *work;
    memset(a->d, 0, al * sizeof(uint64_t));
    *work += al;
}


bool
bigint_ext_gcd(bigint *res, const bigint *a, const bigint *b, uint64_t *work)
{
    size_t N = a->n > b->n ? a->n : b->n;
    size_t L = 2 * N + 8;
    bigint A, B, uA, uB, t0, t1,t2;
    bigint_view(&A, &work, L);
    bigint_view(&B, &work, L);
    bigint_view(&uA, &work, L);
    bigint_view(&uB, &work, L);
    bigint_view(&t0, &work, L);
    bigint_view(&t1, &work, L);
    bigint_view(&t2, &work, L);
    bigint_copy(&A, a);
    bigint_copy(&B, b);
    bigint_scalar(&uA, 1);
    bigint_scalar(&uB, 0);
/*    printf("%zu %zu\n",A.n,B.n);
    puts("A=");
    bigint_printx(&A);
    puts("B=");
    bigint_printx(&B);*/
    while (B.n != 0) {
 //       printf("%zu %zu | %zu %zu\n",A.n,B.n,uA.n,uB.n);
        long double ah = bigint_top(&A);
        long double bh = bigint_top(&B);
        if (A.n<B.n || (A.n==B.n && fabsl(ah)<fabsl(bh))) {
            bigint T = A; A = B; B = T;
            T = uA; uA = uB; uB = T;
            long double t=ah;
            ah =bh; bh=t;
        }
        if(B.n==0)
            break;
        int64_t dn = A.n - B.n;
        bh*=pow(2.,-BASE*dn);
        long double qd = ah / bh;
        if (dn>2 || fabsl(qd) > LAT_LIMIT) {
            int sn=log(fabsl(qd))/log(2.)/BASE;
            int64_t q = llround(qd*pow(2.,-BASE*sn*1.));
//            printf("Desequilibre %d %ld\n",sn,q);
            bigint_addmul_i64_shift(&A, &B, -q, sn, &t0);
            bigint_addmul_i64_shift(&uA, &uB, -q, sn, &t0);
            continue;
        }
//        printf("%Lf %Lf\n",ah,bh);
        if(bh==0)
            break;
/*        puts("A and uA");
        bigint_printx(&A);
        bigint_printx(&uA);*/
        mat2_i64 M = latred(ah,bh,A.n==1);
//        printf("[%ld,%ld;%ld,%ld]\n",M.u00,M.u01,M.u10,M.u11);
        bigint_apply_matrix_pair(&A, &B, M, &t0, &t1,&t2);
        bigint_apply_matrix_pair(&uA, &uB, M, &t0, &t1,&t2);
/*        puts("uA:");
        bigint_printx(&uA);*/
    }
/*    puts("end");
        bigint_printx(&A);
        bigint_printx(&uA);*/
    bigint_copy(res, &uA);
    if((int)A.d[0]*A.s<0)
        res->s=-res->s;
 /*   puts("RES=");
    bigint_printx(res);*/
    bigint_trim(res);
    return A.n==1 && A.d[0]==1;
}


