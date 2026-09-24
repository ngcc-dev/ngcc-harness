#include "inverse.h"
#include "poly.h"
#include <stdint.h>
#include "params.h"
#include "cpucycles.h"
#include "speed.h"
#include "ntt.h"
#include "inverse_avx2.h"
#define NTESTS 10000

uint64_t t[NTESTS];

int test_poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 8; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse(b->coeffs + 8 * i,
                    a->coeffs + 8 * i,
                    zetas_base_nega[i]);
  }
  return r;
}

int test_poly_baseinv_rec(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 16; ++i)
  {
    // if(r != 0)   return r;
    r += rq_inverse_recursive8(b->coeffs + 16 * i,
                    a->coeffs + 16 * i,
                    zetas[64 + i]);
    // if(r != 0)   return r;
    r += rq_inverse_recursive8(b->coeffs + 16 * i + 8,
                    a->coeffs + 16 * i + 8,
                    -zetas[64 + i]);
    // if(i%8==0)printf("\n");
    // printf("%d,%d,",zetas[64 + i],-zetas[64 + i]);
  }
  return r;
}

int test_poly_baseinv_avx2(poly *b, const poly *a)
{
    int r = baseinv_avx2(b->coeffs, a->coeffs);
    return r;
}

int main()
{
    poly a, b, c;
    unsigned int i, j;
    unsigned char buf[DTRU_CBD2_BYTES];

    poly_sample_keygen_f(&a, buf);

    // for (i = 0; i < NTESTS; i++)
    // {
    //     t[i] = cpucycles();
    //     for (j = 0; j < 100; j++)
    //     {
    //         test_poly_baseinv(&c, &a);
    //     }
    // }
    // print_results("poly baseinv: ", t, NTESTS);

    // for (i = 0; i < NTESTS; i++)
    // {
    //     t[i] = cpucycles();
    //     for (j = 0; j < 100; j++)
    //     {
    //         test_poly_baseinv_rec(&b, &a);
    //     }
    // }
    // print_results("poly baseinv (rec): ", t, NTESTS);
    for(int i=0;i<DTRU_N;i++)a.coeffs[i] = (int16_t)i;
     poly b1, b2;
     test_poly_baseinv_rec(&b1, &a);
     test_poly_baseinv_avx2(&b2, &a);
     int err = 0;
     for(i=0;i<DTRU_N;i++)
     {
        printf("%d, %d, %d\n",a.coeffs[i],b1.coeffs[i],b2.coeffs[i]);
        if(b1.coeffs[i] != b2.coeffs[i])
        {
          err++;
          printf("avx2 error at %d\n",i);
        }
     }
     printf("avx2 error %d\n",err);
    return 0;
}