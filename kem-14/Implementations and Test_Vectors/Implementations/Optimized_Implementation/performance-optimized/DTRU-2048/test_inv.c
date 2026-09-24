
#include "inverse.h"
#include "poly.h"
#include <stdint.h>
#include "params.h"
#include "cpucycles.h"
#include "speed.h"
#include "ntt.h"

#define NTESTS 10000

uint64_t t[NTESTS];

int test_poly_baseinv(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 16; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse(b->coeffs + 16 * i,
                    a->coeffs + 16 * i,
                    zetas_base_nega[i]);
  }
  return r;
}

int test_poly_baseinv_rec(poly *b, const poly *a)
{
  int r = 0;
  for (int i = 0; i < DTRU_N / 32; ++i)
  {
    if(r != 0)   return r;
    r += rq_inverse_recursive16(b->coeffs + 32 * i,
                    a->coeffs + 32 * i,
                    zetas[64 + i]);
    if(r != 0)   return r;
    r += rq_inverse_recursive16(b->coeffs + 32 * i + 16,
                    a->coeffs + 32 * i + 16,
                    -zetas[64 + i]);
  }
  return r;
}

int main()
{
    poly a, b, c;
    unsigned int i, j;
    unsigned char buf[DTRU_CBD1_BYTES];

    poly_sample_keygen_f(&a, buf);

    for (i = 0; i < NTESTS; i++)
    {
        t[i] = cpucycles();
        for (j = 0; j < 100; j++)
        {
            test_poly_baseinv(&c, &a);
        }
    }
    print_results("poly baseinv: ", t, NTESTS);

    for (i = 0; i < NTESTS; i++)
    {
        t[i] = cpucycles();
        for (j = 0; j < 100; j++)
        {
            test_poly_baseinv_rec(&b, &a);
        }
    }
    print_results("poly baseinv (rec): ", t, NTESTS);

    return 0;
}