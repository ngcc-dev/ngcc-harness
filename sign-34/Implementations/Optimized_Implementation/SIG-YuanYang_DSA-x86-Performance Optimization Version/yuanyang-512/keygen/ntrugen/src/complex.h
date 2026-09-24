#ifndef COMPLEX_BIGINT_H
#define COMPLEX_BIGINT_H

#include "bigint.h"
#include <stdint.h>
#include <stddef.h>

/*
   Complex number:

      (sre * re + i * sim * im) * 2^(e * BIGINT_BASE)

   Canonical signs:
      sign ==  0  <=> component is zero
      sign == +1  positive
      sign == -1  negative

   re and im are non-negative bigint magnitudes.

   No function allocates.  Storage for re/im limbs is provided by the caller
   through complex_bind().  Workspace is supplied explicitly for mul/div.
*/

typedef struct {
    bigint re;
    bigint im;
    int64_t e;
} Complex;

/* ---------- binding / basic operations ---------- */

void complex_bind(Complex *z,
                  uint64_t *re_buf, size_t re_al,
                  uint64_t *im_buf, size_t im_al);

void complex_zero(Complex *z);
void complex_canonicalize(Complex *z);

void complex_copy(Complex *dst, const Complex *src);
void complex_neg(Complex *r, const Complex *a);
static inline void complex_neg1(Complex *r){
	r->re.s=-r->re.s;
	r->im.s=-r->im.s;
}
static inline void complex_conj(Complex *a){
	a->im.s=-a->im.s;
}

void complex_print(const Complex *r);

// shift the bigints
void complex_truncate(Complex *r,int64_t new_exponent);
void complex_precision_add(Complex *r,int64_t shift);

static inline void complex_round(Complex *a){
	if(a->e<0){
		complex_truncate(a,0);
	}
}

static inline void complex_trim(Complex *a){
	bigint_trim(&a->re);
	bigint_trim(&a->im);
}
/*
   Add/sub require equal exponents.

      assert(a->e == b->e)

   The result buffers must already be large enough.
*/
void complex_add(Complex *r, const Complex *a, const Complex *b);
void complex_sub(Complex *r, const Complex *a, const Complex *b);

/* ---------- multiplication ---------- */

/*
   Workspace size, in uint64_t limbs, required by complex_mul().
*/
size_t complex_mul_work_size(const Complex *a, const Complex *b);

/*
   Minimum useful result capacity for each output component of a*b.
   Both r->re.al and r->im.al should be at least this value.
*/
size_t complex_mul_result_cap(const Complex *a, const Complex *b);

/*
   Complex multiplication using 3 bigint multiplications:

      k1 = x*u
      k2 = y*v
      k3 = (x + y) * (u + v)

      real = k1 - k2
      imag = k3 - k1 - k2

   Multiplications use bigint_mul_karatsuba().
*/
void complex_mul(Complex *r,
                 const Complex *a,
                 const Complex *b,
                 uint64_t *work);


static inline void complex_mul_int(Complex *r,const bigint *a,uint64_t *work){
	r->re.n+=a->n;
	r->im.n+=a->n;
	assert(r->re.al>=r->re.n && r->im.al>=r->im.al);
	bigint res;
	size_t n=(r->re.n>r->im.n ? r->re.n : r->im.n)+a->n;
	res.d=work;res.al=n;res.n=n;work+=n;
	bigint_mul_karatsuba(&r->re,a,&res, work,3);
	bigint_copy(&r->re,&res);
	res.n=n;
	bigint_mul_karatsuba(&r->im,a,&res, work,3);
	bigint_copy(&r->im,&res);
}

/* ---------- division ---------- */

/*
   Workspace size, in uint64_t limbs, required by complex_div().
*/
size_t complex_div_work_size(const Complex *a,
                             const Complex *b,
                             size_t prec_limbs);

/*
   r = a / b, with prec_limbs base-2^BIGINT_BASE fractional limbs.

   Algorithm:

      numerator = a * conj(b)
      norm      = bre^2 + bim^2
      inv       = floor(2^(BIGINT_BASE * prec_limbs) / norm)
      r         = numerator * inv

   Result exponent:

      r->e = a->e - b->e - prec_limbs
*/
void complex_inv(Complex *r,
                 const Complex *a,
                 size_t prec,
                 uint64_t *work);

/*
   Divide both components by 2^k, in-place.

   Since the scaling exponent is in whole BIGINT_BASE limbs, this function
   first shifts by whole limbs through z->e, then divides the magnitudes by
   the remaining bit count.

   This is truncating division toward zero on each component.
*/
void complex_div_pow2(Complex *z, uint64_t k);

#endif /* COMPLEX_BIGINT_H */
