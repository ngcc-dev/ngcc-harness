#ifndef FFT_H
#define FFT_H

void double_negacyclic_FFT(double *f, unsigned logn,const double *twiddles);
void double_negacyclic_iFFT(double *f, unsigned logn,const double *twiddles);
void double_FFT_fold4(const double *f, unsigned logn,double *nf);
void make_twiddles(unsigned logn,double *tw);


#endif
