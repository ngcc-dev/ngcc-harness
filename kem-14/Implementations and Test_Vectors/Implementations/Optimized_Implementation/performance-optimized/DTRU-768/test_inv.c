#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "poly.h"

static void fill_coins(uint8_t coins[DTRU_COINBYTES_KEYGEN], int seed)
{
  for (int i = 0; i < DTRU_COINBYTES_KEYGEN; ++i)
    coins[i] = (uint8_t)(seed * 17 + i * 73 + (i >> 1));
}

static void make_input(poly *f, int seed)
{
  uint8_t coins[DTRU_COINBYTES_KEYGEN];
  poly fp;

  fill_coins(coins, seed);
  poly_sample_keygen_f(&fp, coins);
  poly_multi_p(f, &fp);
  f->coeffs[0] += 1;
}

static void print_poly(const char *name, const poly *a)
{
  printf("%s = {\n", name);
  for (int i = 0; i < DTRU_N; ++i)
  {
    printf("%6d", a->coeffs[i]);
    if (i + 1 != DTRU_N)
      printf(",");
    if ((i & 15) == 15 || i + 1 == DTRU_N)
      printf("\n");
    else
      printf(" ");
  }
  printf("}\n");
}

static int same_poly(const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; ++i)
    if (a->coeffs[i] != b->coeffs[i])
      return 0;
  return 1;
}

static void print_mismatches(const poly *c, const poly *avx)
{
  int shown = 0;

  for (int i = 0; i < DTRU_N; ++i)
  {
    if (c->coeffs[i] != avx->coeffs[i])
    {
      printf("mismatch[%d]: C=%d AVX2=%d\n", i, c->coeffs[i], avx->coeffs[i]);
      if (++shown == 32)
      {
        printf("... more mismatches omitted\n");
        return;
      }
    }
  }
}

int main(void)
{
  poly input;
  poly c_ntt, avx_ntt;
  poly c_inv, avx_inv;
  poly c_normal, avx_normal;
  int c_ret = 1;
  int avx_ret = 1;
  int seed;

  for (seed = 1; seed < 1000; ++seed)
  {
    make_input(&input, seed);

    memcpy(&c_ntt, &input, sizeof(c_ntt));
    memcpy(&avx_ntt, &input, sizeof(avx_ntt));
    memset(&c_inv, 0, sizeof(c_inv));
    memset(&avx_inv, 0, sizeof(avx_inv));

    poly_ntt(&c_ntt);
    poly_ntt_avx(&avx_ntt, &avx_ntt);

    c_ret = poly_baseinv(&c_inv, &c_ntt);
    avx_ret = poly_baseinv_avx(&avx_inv, &avx_ntt);
    if (c_ret == 0 && avx_ret == 0)
      break;
  }

  printf("seed = %d\n", seed);
  printf("C baseinv return = %d\n", c_ret);
  printf("AVX2 baseinv return = %d\n", avx_ret);

  if (c_ret != 0 || avx_ret != 0)
  {
    printf("failed to find an invertible deterministic input\n");
    return 1;
  }

  memcpy(&c_normal, &c_inv, sizeof(c_normal));
  poly_invntt(&c_normal);
  poly_freeze(&c_normal);

  memcpy(&avx_normal, &avx_inv, sizeof(avx_normal));
  poly_invntt_avx(&avx_normal, &avx_normal);
  poly_freeze_avx(&avx_normal);

  print_poly("C inverse output", &c_inv);
  print_poly("AVX2 inverse output", &avx_inv);
  print_poly("C inverse output after invntt", &c_normal);
  print_poly("AVX2 inverse output after invntt", &avx_normal);

  if (same_poly(&c_normal, &avx_normal))
  {
    printf("C and AVX2 inverse outputs match after invntt\n");
    return 0;
  }

  printf("C and AVX2 inverse outputs differ after invntt\n");
  print_mismatches(&c_normal, &avx_normal);
  return 1;
}
