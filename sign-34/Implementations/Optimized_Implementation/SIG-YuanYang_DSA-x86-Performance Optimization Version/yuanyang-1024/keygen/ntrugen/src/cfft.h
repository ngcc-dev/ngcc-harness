#ifndef COMPLEX_FFT_H
#define COMPLEX_FFT_H

#include "complex.h"
#include <stddef.h>
#include <stdint.h>

size_t complex_negacyclic_fft_work_size(const Complex *f,
                                        const Complex *gm);

// subfield indicates whether the input is a P(x^2); half of the input is required and half of the output is correct
// fold returns a subfield=1 element

void negacyclic_FFT(Complex *f,
                            unsigned logn,
                            const Complex *gm,
			    int subfield,
			    int twiddleprecision,
                            uint64_t *work);

void negacyclic_iFFT(Complex *f,
                             unsigned logn,
                             const Complex *gm,
			     int subfield,
			    int twiddleprecision,
                             uint64_t *work);

void complex_FFT_fold(const Complex *f, unsigned logn,Complex *nf,uint64_t *work);


/*
   tw_abs contains 2*n bigint magnitudes.

   n = 1 << logn

   For each k in [0, n):

       j = bit_reverse(k, logn)
       angle = pi * j / n

       tw_abs[(2*k + 0) * tw_al ... (2*k + 1) * tw_al - 1]
           = abs(cos(angle)) as a non-negative bigint

       tw_abs[(2*k + 1) * tw_al ... (2*k + 2) * tw_al - 1]
           = abs(sin(angle)) as a non-negative bigint

   All magnitudes are little-endian base-2^BIGINT_BASE limbs.

   The resulting Complex is:

       gm[k] = sign(cos(angle)) * abs(cos(angle))
             + i * sign(sin(angle)) * abs(sin(angle))

       gm[k] *= 2^(e * BIGINT_BASE)

   In the negacyclic FFT:

       w = exp(i*pi/n)
       gm[k] = w^bit_reverse(k, logn)
*/

void
complex_make_twiddles(Complex *gm,
                      unsigned logn,
                      const uint64_t *tw_abs,
                      int64_t e);


#endif
