#ifndef FF_ARITH_H

#define FF_ARITH_H

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/*
finite field F2[a] = F2[x]/(x^12+x^3+1);
*/
typedef uint16_t ff12b;

#define EXT_DEGREE 87

typedef ff12b fe[EXT_DEGREE];

void ff12b_add(ff12b a, ff12b b, ff12b *c);

void ff12b_or(ff12b a, ff12b b, ff12b *c);

// conditional add;  a[i] = a[i] ^ c if the ith bit of b is 1;
//   a[i] = a[i] + c * b[i]
void ff12b_cond_add(ff12b a[], unsigned char b[], int dim, ff12b c);

void ff12b_mul(ff12b a, ff12b b, ff12b *c);

void fe_add(fe a, fe b, fe c);

void fe_or(fe a, fe b, fe c);

void fe_mul(fe a, fe b, fe c);

int deg(fe a);

// compute r=a%b, with quotient q.  a = bq + r, where b!=0;
void rem(fe a, fe b, fe q, fe r);

// compute r=a%b, where a is fixed to be f= x^22 + (0x407)x^14 + (0x760)x^3 + (0x6b0)x^2 + (0x3b0);
void f_div(fe a, fe b, fe q, fe r);

void fe_inv(fe a, fe a_inv);

void fe_ff12b_mul(fe a, ff12b b, fe c);

// product = <a,,b> where a,b \in fe^{dim}
void fe_inner(fe a[], fe b[], int dim, fe product);

// product = <a,,b> where a\in fe^{dim}, b\in ff12b^{dim}
void fe_ff12b_inner(fe a[], ff12b b[], int dim, fe product);

// product = <a,,b> where a\in fe^{dim}, b\in f2^{dim}
void fe_f2_inner(fe a[], unsigned char b[], int dim, fe product);


//compress array v[] into nonce[], where sizeof(nonce) = dim * FE_BYTES
void  fe_compress(fe* v, int dim, unsigned char* nonce);

//decompress nonce[] into array v[], where sizeof(nonce) = dim * FE_BYTES
void  fe_decompress(unsigned char* nonce, fe* v, int dim);

#endif // FF_ARITH_H