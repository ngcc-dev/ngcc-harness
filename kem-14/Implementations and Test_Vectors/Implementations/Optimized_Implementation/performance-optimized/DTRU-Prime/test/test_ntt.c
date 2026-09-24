#include <stdint.h>
#include <stdio.h>
#include "../poly.h"
#include "../params.h"
#include "../randombytes.h"
#include "../radix_ntt_n1087.h"
#include "../reduce.h"

void print_poly(const char *name, const poly *a)
{
    printf("%s: \n", name);
    for (int i = 0; i < DTRU_N; i++)
    {
        printf("%d ", a->coeffs[i]);
    }
    printf("\n");
}

void poly_mul_q1(poly *c, const poly *a, const poly *b)
{
  int32_t temp[2 * DTRU_N - 1] = {0};

  for (int i = 0; i < DTRU_N; i++) {
    for (int j = 0; j < DTRU_N; j++) {
      temp[i + j] += ((a->coeffs[i] * b->coeffs[j]) % DTRU_Q);
    }
  }
  for (int i = DTRU_N; i < 2 * DTRU_N - 1; i++) {
    temp[i-DTRU_N] += temp[i];
    temp[i-DTRU_N+1] += temp[i];
  }
  for (int i = 0; i < DTRU_N; i++) {
    c->coeffs[i] = temp[i] % DTRU_Q;
  }
}

int main()
{
    poly a, b, c, d;
    unsigned char buf[DTRU_CBD1_BYTES + DTRU_CBD2_BYTES];
    randombytes(buf, sizeof(buf));
    poly_sample_keygen_f(&a, buf);
    poly_sample_keygen_g(&b, buf + DTRU_CBD1_BYTES);

    poly_radix_ntt_n1087(&c, &a, &b);

    poly_mul_q1(&d, &a, &b);

    print_poly("a", &a);
    print_poly("b", &b);
    print_poly("c", &c);
    print_poly("d", &d);

    for (int16_t i = 0; i < DTRU_Q; i++)
    {
        int16_t inv = fqinv(i);
        printf("%d, ", inv);
    }
    printf("\n");
        

    return 0;
}
