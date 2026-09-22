#ifndef BIGINT_H
#define BIGINT_H

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* A bigint is an array of int64 limbs in little-endian order.
   The base used is 2^BIGINT_BASE
   add/sub acts limbwise
   output has to be normalized before there is an overflow
   use normalize_signed if the integer array may be negative
   nbadd is the max size divided by 2^BIGINT_BASE
   multiplication assumes the input array is non-negative, res->n most signficant limbs are in the output;
   for karatsuba allocate at least a->n+b->n

   */
typedef struct {
    size_t  n;      /* number of limbs */
    size_t al; // allocated
    int s; //sign
    uint64_t *d;     /* properly aligned limb data */
} bigint;

#define BIGINT_BASE 50

/* ---------- life-cycle ---------- */
int  bigint_init(bigint *a, size_t n);   /* allocate zeroed, aligned limbs */
void bigint_free(bigint *a);
void bigint_zero(bigint *a);
static inline void bigint_scalar(bigint *a,int64_t scalar){
    a->n=1;
    if(scalar>0){
        a->d[0]=scalar;
        a->s=1;
    }
    else{
        a->d[0]=-scalar;
        a->s=-1;
    }
}

void bigint_copy(bigint *dst, const bigint *src);

/* ---------- core arithmetic ----------
   res must already be allocated by the caller.
   Does not grow the array
   */
void  bigint_add(bigint *res, const bigint *a, const bigint *b);
void  bigint_sub(bigint *res, const bigint *a, const bigint *b);
static inline void bigint_neg(bigint *r){
    r->s=-r->s;
}

/* scalar must satisfy scalar < (1ULL << BASE).                              */
void bigint_mul_scalar(bigint *res, const bigint *a, int64_t scalar);

/* schoolbook O(n·m) multiplication.  res must have a->n + b->n limbs.       */
void bigint_mul_quadratic(bigint *res, const bigint *a, const bigint *b);
/* input bounded by nbadd*2^BASE, output:nbadd=1+epsilon       */
void bigint_mul_karatsuba(const bigint *a,const bigint *b,bigint *r, uint64_t *work,int nbadd);

/* nbadd=1       */
void bigint_normalize(bigint *a);
/* nbadd=1+epsilon       */
void bigint_fast_normalize(bigint *a);
/* nbadd=2+epsilon ; output=sign      */
void bigint_fast_normalize_signed(bigint *a);

static inline void bigint_shr(bigint *a,int l){
    size_t sl;
    if(l<=0)
        return;
    sl = (size_t)l;
    if(sl>=a->n){
        a->d[0]=0;
        a->n=0;
        return;
    }
    memmove(a->d,a->d+sl,(a->n-sl)*8);
    a->n-=sl;
}

static inline void bigint_shl(bigint *a,int l){
    size_t sl;
    if(l<=0)
        return;
    sl = (size_t)l;
    assert(sl+a->n<=a->al);
    memmove(a->d+sl,a->d,a->n*8);
    memset(a->d,0,sl*8);
    a->n+=sl;
}

static inline void
bigint_trim(bigint *a)
{
    while (a->n > 0 && a->d[a->n - 1] == 0)
        a->n--;
}

void bigint_print(const bigint *a);
void bigint_printx(const bigint *a);
void bigint_printx_s(const bigint *a);

size_t bigint_karatsuba_work_size(size_t na,size_t nb);

/*
 * bigint_reciprocal_newton
 *
 * Compute  res = floor( 2^(BIGINT_BASE*e) / a )  using Newton iteration.
 *
 * a       – non-zero, normalized bigint.
 * e       – exponent (>= 0).
 * work    – contiguous uint64_t scratch buffer.
 * Total need = 7*e + 14 + bigint_karatsuba_work_size(e+2, e+2) uint64_t.
 *
 * res must be allocated by the caller with at least e+2 limbs.
 */
void bigint_reciprocal(bigint *res, const bigint *a, int64_t e,uint64_t *work);


void bigint_div_pow2(bigint *a, uint64_t k);

// true iff gcd=1

bool bigint_ext_gcd(bigint *res,const bigint *a,const bigint *b,uint64_t *work);

#endif
