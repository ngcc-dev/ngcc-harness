
#include "inverse.h"
#include "poly.h"
#include <stdint.h>
#include "params.h"
#include "cpucycles.h"
#include "speed.h"
#include "ntt.h"
#include "ntt_avx2.h"

#define NTESTS 10000

uint64_t t[NTESTS];

int test_poly_baseinv_det(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 12; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 12 * i,
                        a->coeffs + 12 * i,
                        zetas[128 + 2 * i]);
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 12 * i + 4,
                        a->coeffs + 12 * i + 4,
                        zetas_base[i]);
    if(r != 0)   return r;
    r += rq_inverse_det(b->coeffs + 12 * i + 8,
                        a->coeffs + 12 * i + 8,
                        -zetas[128 + 2 * i]-zetas_base[i]);
  }
  return r;
}

int test_poly_baseinv_rec(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 12; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_recursive(b->coeffs + 12 * i,
                        a->coeffs + 12 * i,
                        zetas[128 + 2 * i]);
    if(r != 0)   return r;
    r += rq_inverse_recursive(b->coeffs + 12 * i + 4,
                        a->coeffs + 12 * i + 4,
                        zetas_base[i]);
    if(r != 0)   return r;
    r += rq_inverse_recursive(b->coeffs + 12 * i + 8,
                        a->coeffs + 12 * i + 8,
                        -zetas[128 + 2 * i]-zetas_base[i]);
  }
  return r;
}

int test_poly_baseinv_avx2(poly *b, const poly *a) {
  return poly_baseinv_avx2(b, a);
}
int main()
{
  poly a, out_det, out_rec, out_avx;
  unsigned int i, j;
  unsigned char buf[DTRU_CBD1_BYTES];

  poly_sample_keygen_f(&a, buf);

  test_poly_baseinv_det(&out_det, &a);
  test_poly_baseinv_rec(&out_rec, &a);
  test_poly_baseinv_avx2(&out_avx, &a);

  j = 0;

  // for(i = 0; i < DTRU_N; i++) {
  //   if ((out_det.coeffs[i] - out_rec.coeffs[i]) % DTRU_Q != 0) {
  //     printf("Wrong at index %u: det=%d, rec=%d\n", i, out_det.coeffs[i], out_rec.coeffs[i]);
  //     // j++;
  //     return -1;
  //   }
  // }

  for (i = 0; i < DTRU_N; i++) {
    if ((out_rec.coeffs[i] - out_avx.coeffs[i]) % DTRU_Q != 0) {
      printf("Mismatch at index %u: det = %d, rec=%d, avx=%d\n", i, out_det.coeffs[i], out_rec.coeffs[i], out_avx.coeffs[i]);
      j++;
      // return -1;
    }
  }
  if (j == 0) {
    printf("AVX2 implementation matches reference implementation.\n");
  } else {
    printf("Total mismatches: %u\n", j);
  }

  return 0;
}