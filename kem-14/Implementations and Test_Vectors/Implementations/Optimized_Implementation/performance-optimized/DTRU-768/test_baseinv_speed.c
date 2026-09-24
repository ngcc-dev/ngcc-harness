#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpucycles.h"
#include "params.h"
#include "poly.h"
#include "speed.h"

#define NTESTS 10000

static uint64_t t[NTESTS];

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

static int same_poly(const poly *a, const poly *b)
{
  for (int i = 0; i < DTRU_N; ++i)
    if (a->coeffs[i] != b->coeffs[i])
      return 0;
  return 1;
}

static int prepare_inputs(poly *c_ntt, poly *avx_ntt)
{
  poly input, c_inv, avx_inv, c_normal, avx_normal;
  int c_ret = 1;
  int avx_ret = 1;
  int seed;

  for (seed = 1; seed < 1000; ++seed)
  {
    make_input(&input, seed);
    memcpy(c_ntt, &input, sizeof(*c_ntt));
    memcpy(avx_ntt, &input, sizeof(*avx_ntt));

    poly_ntt(c_ntt);
    poly_ntt_avx(avx_ntt, avx_ntt);

    c_ret = poly_baseinv(&c_inv, c_ntt);
    avx_ret = poly_baseinv_avx(&avx_inv, avx_ntt);
    if (c_ret == 0 && avx_ret == 0)
      break;
  }

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

  if (!same_poly(&c_normal, &avx_normal))
  {
    printf("baseinv correctness: fail\n");
    return 1;
  }

  printf("baseinv correctness: pass (seed = %d)\n\n", seed);
  return 0;
}

int main(void)
{
  poly c_ntt, avx_ntt;
  poly out;
  volatile int ret_acc = 0;
  volatile int checksum = 0;

  if (prepare_inputs(&c_ntt, &avx_ntt) != 0)
    return 1;

  for (int i = 0; i < NTESTS; ++i)
  {
    t[i] = cpucycles();
    ret_acc += poly_baseinv(&out, &c_ntt);
  }
  checksum += out.coeffs[0];
  print_results("C poly_baseinv:", t, NTESTS);

  for (int i = 0; i < NTESTS; ++i)
  {
    t[i] = cpucycles();
    ret_acc += poly_baseinv_avx(&out, &avx_ntt);
  }
  checksum += out.coeffs[0];
  print_results("AVX2 poly_baseinv_avx:", t, NTESTS);

  printf("ret_acc = %d, checksum = %d\n", ret_acc, checksum);
  return ret_acc == 0 ? 0 : 1;
}
