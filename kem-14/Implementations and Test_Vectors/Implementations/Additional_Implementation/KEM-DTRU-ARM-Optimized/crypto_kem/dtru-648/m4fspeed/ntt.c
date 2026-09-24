#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "ntt.h"

int16_t zetas_base[96] = {
  2674, 725, 353, 2858, 1835, 2014, 1628, 3358, 1244, 592, 3421, 2338, 268, 264, 1292, 726, 96, 2984, 445, 3173, 391, 2676, 211, 1085,

  3421, 1829, 1664, 2338, 99, 1020, 652, 2213, 592, 1244, 2865, 2805, 1793, 36, 1628, 2437, 1119, 3358, 353, 599, 2505, 2618, 1622, 2674, 1289, 1443, 725, 2014, 2732, 2168, 952, 3104, 2858, 783, 839, 1835, 211, 3012, 234, 1085, 284, 2088, 1172, 3066, 2676, 391, 781, 2285, 3223, 3246, 445, 1369, 2372, 3173, 1292, 2731, 2891, 3285, 3361, 268, 2720, 473, 264, 2984, 3193, 737, 566, 2165, 726, 3189, 172, 96
};

int16_t zetas[72] = {
  2571, 2424, 1886, 1937, 1715, 2644, 2833, 2753, 256, 1, 1846, 2734, 2195, 2735, 1973, 3456, 2293, 3200, 357, 2590, 1741, 2333, 1346, 257, 1622, 3080, 1443, 298, 2505, 1901, 3104, 93, 839, 2925, 2168, 2337, 36, 1120, 1119, 1994, 2805, 377, 2213, 2548, 1664, 2635, 1020, 3364, 3361, 2971, 473, 1888, 2891, 3065, 2165, 450, 172, 2221, 737, 491, 3246, 2966, 2372, 58, 2285, 486, 3066, 1735, 234, 2379, 2088, 3007
};

int16_t zetas_inv[74] = {
  284, 392, 3012, 1569, 1172, 486, 781, 1236, 1369, 3007, 3223, 2379, 3193, 1078, 3189, 1722, 566, 3065, 2731, 3399, 2720, 491, 3285, 2221, 99, 1556, 1829, 3159, 652, 377, 2865, 532, 2437, 3364, 1793, 2635, 2732, 822, 783, 909, 952, 1901, 599, 1463, 1289, 2337, 2618, 2925, 395, 867, 2111, 257, 1936, 1124, 3100, 2590, 222, 723, 1484, 3456, 1867, 722, 1611, 2734, 704, 624, 813, 1742, 1520, 1571, 1792, 1033, 3325, 3193
};

static const int16_t zetas_fwd_radix3_asm[64] = {
  /* len=27 */
  256, 1, 1846, 2734, 2195, 2735, 1973, 3456,
  2293, 3200, 357, 2590, 1741, 2333, 1346, 257,
  /* len=9 */
  1622, 3080, 1443, 298, 2505, 1901, 3104, 93,
  839, 2925, 2168, 2337, 36, 1120, 1119, 1994,
  2805, 377, 2213, 2548, 1664, 2635, 1020, 3364,
  3361, 2971, 473, 1888, 2891, 3065, 2165, 450,
  172, 2221, 737, 491, 3246, 2966, 2372, 58,
  2285, 486, 3066, 1735, 234, 2379, 2088, 3007
};

static const int16_t zetas_inv_radix3_asm[64] = {
  /* len=9 */
  284, 392, 3012, 1569, 1172, 486, 781, 1236,
  1369, 3007, 3223, 2379, 3193, 1078, 3189, 1722,
  566, 3065, 2731, 3399, 2720, 491, 3285, 2221,
  99, 1556, 1829, 3159, 652, 377, 2865, 532,
  2437, 3364, 1793, 2635, 2732, 822, 783, 909,
  952, 1901, 599, 1463, 1289, 2337, 2618, 2925,
  /* len=27 */
  395, 867, 2111, 257, 1936, 1124, 3100, 2590,
  222, 723, 1484, 3456, 1867, 722, 1611, 2734,
};

#define DUP16(x) (((uint32_t)(uint16_t)(x) << 16) | (uint16_t)(x))

const uint32_t zetas_fwd_radix2_dup[7] = {
  DUP16(2424),
  DUP16(1886), DUP16(1937),
  DUP16(1715), DUP16(2644), DUP16(2833), DUP16(2753)
};

const uint32_t zetas_inv_radix2_dup[7] = {
  DUP16(704), DUP16(624), DUP16(813), DUP16(1742),
  DUP16(1520), DUP16(1571), DUP16(1792)
};

const uint32_t zetas_inv_final_dup[2] = {
  DUP16(3325), DUP16(3193)
};

const int16_t zetas_triplets[72] = {
  1622, 2674, -4296, 1443, 725, -2168, 2505, 353, -2858, 3104, 2858, -5962,
  839, 1835, -2674, 2168, 2014, -4182, 36, 1628, -1664, 1119, 3358, -4477,
  2805, 1244, -4049, 2213, 592, -2805, 1664, 3421, -5085, 1020, 2338, -3358,
  3361, 268, -3629, 473, 264, -737, 2891, 1292, -4183, 2165, 726, -2891,
  172, 96, -268, 737, 2984, -3721, 3246, 445, -3691, 2372, 3173, -5545,
  2285, 391, -2676, 3066, 2676, -5742, 234, 211, -445, 2088, 1085, -3173
};

extern void ntt_fast_648(int16_t *a, const int16_t *leaf_twiddles,
                         const uint32_t *radix2_twiddles);
extern void invntt_fast_648(int16_t *a, const int16_t *twiddles,
                            const uint32_t *radix2_twiddles,
                            const uint32_t *final_consts);

void ntt_asm(int16_t *a)
{
  ntt_fast_648(a, zetas_fwd_radix3_asm, zetas_fwd_radix2_dup);
}

void invntt_asm(int16_t *a)
{
  invntt_fast_648(a, zetas_inv_radix3_asm, zetas_inv_radix2_dup, zetas_inv_final_dup);
}

void ntt(int16_t *a)
{
  unsigned int len, start, j, k = 1;
  int16_t t, z[3], zeta, zeta1, rho = zetas[0];

  zeta = zetas[k++];
  for (j = 0; j < DTRU_N / 2; ++j)
  {
    t = fqmul(zeta, a[j + DTRU_N / 2]);
    a[j + DTRU_N / 2] = (a[j] + a[j + DTRU_N / 2] - t);
    a[j] = (a[j] + t);
  }

  for (len = DTRU_N / 4; len >= 81; len >>= 1)
  {
    for (start = 0; start < DTRU_N; start = j + len)
    {
      zeta = zetas[k++];
      for (j = start; j < start + len; ++j)
      {
        t = fqmul(zeta, a[j + len]);
        a[j + len] = (a[j] - t);
        a[j] = (a[j] + t);
      }
    }
  }

  len = 27;
  for (start = 0; start < DTRU_N; start =  j + 2 * len)
  {
    zeta = zetas[k++];
    zeta1 = zetas[k++];
    for (j = start; j < start + len; ++j)
    {
      z[0] = fqmul(zeta, a[j + len]);
      z[1] = fqmul(zeta1, a[j + 2 * len]);
      z[2] = fqmul(rho, z[0] - z[1]);
      a[j + len] = barrett_reduce(a[j] + z[2] - z[1]);
      a[j + 2 * len] = barrett_reduce(a[j] - z[2] - z[0]);
      a[j] = barrett_reduce(a[j] + z[0] + z[1]);
    }
  }

  len = 9;
  for (start = 0; start < DTRU_N; start =  j + 2 * len)
  {
    zeta = zetas[k++];
    zeta1 = zetas[k++];
    for (j = start; j < start + len; ++j)
    {
      z[0] = fqmul(zeta, a[j + len]);
      z[1] = fqmul(zeta1, a[j + 2 * len]);
      z[2] = fqmul(rho, z[0] - z[1]);
      a[j + len] = barrett_reduce(a[j] + z[2] - z[1]);
      a[j + 2 * len] = barrett_reduce(a[j] - z[2] - z[0]);
      a[j] = barrett_reduce(a[j] + z[0] + z[1]);
    }
  }
}

void invntt(int16_t *a)
{
  unsigned int start, len, j, k = 0;
  int16_t t, z[3], zeta, zeta1, rho = zetas_inv[71];

  len = 9;
  for (start = 0; start < DTRU_N; start = j + 2 * len)
  {
    zeta = zetas_inv[k++];
    zeta1 = zetas_inv[k++];
    for (j = start; j < start + len; ++j)
    {
      z[0] = a[j + len];
      z[1] = a[j + 2 * len];
      z[2] = fqmul(rho, z[0] - z[1]);
      a[j + len] = fqmul(zeta, a[j] + z[2] - z[1]);
      a[j + 2 * len] = fqmul(zeta1, a[j] - z[2] - z[0]);
      a[j] = barrett_reduce(a[j] + z[0] + z[1]);
    }
  }

  len = 27;
  for (start = 0; start < DTRU_N; start = j + 2 * len)
  {
    zeta = zetas_inv[k++];
    zeta1 = zetas_inv[k++];
    for (j = start; j < start + len; ++j)
    {
      z[0] = a[j + len];
      z[1] = a[j + 2 * len];
      z[2] = fqmul(rho, z[0] - z[1]);
      a[j + len] = fqmul(zeta, a[j] + z[2] - z[1]);
      a[j + 2 * len] = fqmul(zeta1, a[j] - z[2] - z[0]);
      a[j] = barrett_reduce(a[j] + z[0] + z[1]);
    }
  }

  for (len = 81; len < DTRU_N / 2; len <<= 1)
  {
    for (start = 0; start < DTRU_N; start = j + len)
    {
      zeta = zetas_inv[k++];
      for (j = start; j < start + len; ++j)
      {
        t = a[j];
        a[j] = (t + a[j + len]);
        a[j + len] = (t - a[j + len]);
        a[j + len] = fqmul(zeta, a[j + len]);
      }
    }
  }

  zeta = zetas_inv[k++];
  for (j = 0; j < DTRU_N / 2; ++j)
  {
    t = fqmul(zeta, a[j] - a[j + DTRU_N / 2]);
    a[j] = fqmul(zetas_inv[72], a[j] + a[j + DTRU_N / 2] - t);
    a[j + DTRU_N / 2] = fqmul(zetas_inv[73], t);
  }
}

#define CALC_D(a, b, x, y, d) (fqmul((a[x] + a[y]), (b[x] + b[y])) - d[x] - d[y])

void basemul3(int16_t c[3], const int16_t a[3], const int16_t b[3], const int16_t zeta)
{
  int16_t d[3];

  for (int i = 0; i < 3; ++i)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul(CALC_D(a, b, 1, 2, d), zeta));
  c[1] = barrett_reduce(CALC_D(a, b, 0, 1, d) + fqmul(d[2], zeta));
  c[2] = barrett_reduce(CALC_D(a, b, 0, 2, d) + d[1]);
}

void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t a0[3], a1[3], a2[3];
  int16_t b0[3], b1[3], b2[3];
  int16_t d0[3], d1[3], d2[3];
  int16_t a01[3], a02[3], a12[3];
  int16_t b01[3], b02[3], b12[3];
  int16_t ab01[3], ab02[3], ab12[3];

  a0[0] = a[0];
  a1[0] = a[1];
  a2[0] = a[2];
  a0[1] = a[3];
  a1[1] = a[4];
  a2[1] = a[5];
  a0[2] = a[6];
  a1[2] = a[7];
  a2[2] = a[8];
  b0[0] = b[0];
  b1[0] = b[1];
  b2[0] = b[2];
  b0[1] = b[3];
  b1[1] = b[4];
  b2[1] = b[5];
  b0[2] = b[6];
  b1[2] = b[7];
  b2[2] = b[8];

  a01[0] = a0[0] + a1[0];
  a02[0] = a0[0] + a2[0];
  a12[0] = a1[0] + a2[0];
  a01[1] = a0[1] + a1[1];
  a02[1] = a0[1] + a2[1];
  a12[1] = a1[1] + a2[1];
  a01[2] = a0[2] + a1[2];
  a02[2] = a0[2] + a2[2];
  a12[2] = a1[2] + a2[2];
  b01[0] = b0[0] + b1[0];
  b02[0] = b0[0] + b2[0];
  b12[0] = b1[0] + b2[0];
  b01[1] = b0[1] + b1[1];
  b02[1] = b0[1] + b2[1];
  b12[1] = b1[1] + b2[1];
  b01[2] = b0[2] + b1[2];
  b02[2] = b0[2] + b2[2];
  b12[2] = b1[2] + b2[2];

  basemul3(d0, a0, b0, zeta);
  basemul3(d1, a1, b1, zeta);
  basemul3(d2, a2, b2, zeta);
  basemul3(ab01, a01, b01, zeta);
  basemul3(ab02, a02, b02, zeta);
  basemul3(ab12, a12, b12, zeta);

  c[0] = barrett_reduce(d0[0] + fqmul(ab12[2] - d1[2] - d2[2], zeta));
  c[3] = barrett_reduce(d0[1] + ab12[0] - d1[0] - d2[0]);
  c[6] = barrett_reduce(d0[2] + ab12[1] - d1[1] - d2[1]);

  c[1] = barrett_reduce(ab01[0] - d0[0] - d1[0] + fqmul(zeta, d2[2]));
  c[4] = barrett_reduce(ab01[1] - d0[1] - d1[1] + d2[0]);
  c[7] = barrett_reduce(ab01[2] - d0[2] - d1[2] + d2[1]);

  c[2] = barrett_reduce(ab02[0] - d0[0] - d2[0] + d1[0]);
  c[5] = barrett_reduce(ab02[1] - d0[1] - d2[1] + d1[1]);
  c[8] = barrett_reduce(ab02[2] - d0[2] - d2[2] + d1[2]);
}
