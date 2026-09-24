#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "ntt.h"

int16_t zetas[128] = {
  3310, 886, 1520, 1571, 704, 624, 813, 1742, 594, 2255, 2260, 2946, 137, 200, 2500, 16, 2682, 963, 3395, 1045, 2987, 2569, 2728, 2418, 1548, 115, 3166, 1392, 1028, 1854, 2433, 978, 2585, 1427, 2281, 529, 341, 2895, 3346, 923, 975, 2357, 78, 3369, 2041, 2, 25, 415, 631, 795, 1295, 755, 3349, 3047, 1789, 1350, 2692, 1129, 2013, 920, 2547, 2179, 1310, 1004, 1987, 3254, 2648, 1090, 2454, 2018, 1026, 438, 1826, 3347, 2082, 1374, 2624, 1383, 1731, 1770, 2825, 1954, 226, 986, 152, 449, 427, 1557, 2229, 1740, 1008, 1522, 2177, 2951, 589, 2172, 2621, 2716, 2837, 79, 2214, 1491, 3081, 3438, 126, 2783, 1946, 1882, 1370, 2000, 801, 160, 2853, 1036, 2579, 636, 2377, 2814, 605, 3129, 2721, 919, 2845, 2286, 1271, 1048, 2729, 3126
};

int16_t zetas_inv[129] = {
  331, 728, 2409, 2186, 1171, 612, 2538, 736, 328, 2852, 643, 1080, 2821, 878, 2421, 604, 3297, 2656, 1457, 2087, 1575, 1511, 674, 3331, 19, 376, 1966, 1243, 3378, 620, 741, 836, 1285, 2868, 506, 1280, 1935, 2449, 1717, 1228, 1900, 3030, 3008, 3305, 2471, 3231, 1503, 632, 1687, 1726, 2074, 833, 2083, 1375, 110, 1631, 3019, 2431, 1439, 1003, 2367, 809, 203, 1470, 2453, 2147, 1278, 910, 2537, 1444, 2328, 765, 2107, 1668, 410, 108, 2702, 2162, 2662, 2826, 3042, 3432, 3455, 1416, 88, 3379, 1100, 2482, 2534, 111, 562, 3116, 2928, 1176, 2030, 872, 2479, 1024, 1603, 2429, 2065, 291, 3342, 1909, 1039, 729, 888, 470, 2412, 62, 2494, 775, 3441, 957, 3257, 3320, 511, 1197, 1202, 2863, 1715, 2644, 2833, 2753, 1886, 1937, 1665, 790, 1580
};

void ntt(int16_t *a)
{
  unsigned int len, start, j, k = 1;
  int16_t t, zeta;

  zeta = zetas[k++];
  for (j = 0; j < DTRU_N / 2; ++j)
  {
    t = fqmul(zeta, a[j + DTRU_N / 2]);
    a[j + DTRU_N / 2] = barrett_reduce(a[j] + a[j + DTRU_N / 2] - t);
    a[j] = barrett_reduce(a[j] + t);
  }

  for (len = DTRU_N / 4; len >= 128; len >>= 1)
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

  len = 64;
  for (start = 0; start < DTRU_N; start = j + len)
  {
    zeta = zetas[k++];
    for (j = start; j < start + len; ++j)
    {
      t = fqmul(zeta, a[j + len]);
      a[j + len] = barrett_reduce(a[j] - t);
      a[j] = barrett_reduce(a[j] + t);
    }
  }

  for (len = 32; len >= ROOT_DIMENSION; len >>= 1)
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
}

void invntt(int16_t *a)
{
  unsigned int start, len, j, k = 0;
  int16_t t, zeta;

  len = ROOT_DIMENSION;
  for (start = 0; start < DTRU_N; start = j + len)
  {
    zeta = zetas_inv[k++];
    for (j = start; j < start + len; ++j)
    {
      t = a[j];
      a[j] = barrett_reduce(t + a[j + len]);
      a[j + len] = barrett_reduce(t - a[j + len]);
      a[j + len] = fqmul(zeta, a[j + len]);
    }
  }

  for (len = 16; len <= 32; len <<= 1)
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

  len = 64;
  for (start = 0; start < DTRU_N; start = j + len)
  {
    zeta = zetas_inv[k++];
    for (j = start; j < start + len; ++j)
    {
      t = a[j];
      a[j] = barrett_reduce(t + a[j + len]);
      a[j + len] = fqmul(zeta, t - a[j + len]);
    }
  }

  for (len = 128; len <= DTRU_N / 4; len <<= 1)
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
    t = a[j] - a[j + DTRU_N / 2];
    t = fqmul(zeta, t);
    a[j] = a[j] + a[j + DTRU_N / 2] - t;
    a[j] = fqmul(zetas_inv[127], a[j]);
    a[j + DTRU_N / 2] = fqmul(zetas_inv[128], t);
  }
}

#define CALC_D(a, b, x, y, d) (fqmul((a[x] + a[y]), (b[x] + b[y])) - d[x] - d[y])

void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t d[8];
  for (int i = 0; i < 8; i++)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul((CALC_D(a, b, 1, 7, d) + CALC_D(a, b, 2, 6, d) + CALC_D(a, b, 3, 5, d) + d[4]), zeta));
  c[1] = barrett_reduce(CALC_D(a, b, 0, 1, d) + fqmul((CALC_D(a, b, 2, 7, d) + CALC_D(a, b, 3, 6, d) + CALC_D(a, b, 4, 5, d)), zeta));
  c[2] = barrett_reduce(CALC_D(a, b, 0, 2, d) + d[1] + fqmul((CALC_D(a, b, 3, 7, d) + CALC_D(a, b, 4, 6, d) + d[5]), zeta));
  c[3] = barrett_reduce(CALC_D(a, b, 0, 3, d) + CALC_D(a, b, 1, 2, d) + fqmul((CALC_D(a, b, 4, 7, d) + CALC_D(a, b, 5, 6, d)), zeta));
  c[4] = barrett_reduce(CALC_D(a, b, 0, 4, d) + CALC_D(a, b, 1, 3, d) + d[2] + fqmul((CALC_D(a, b, 5, 7, d) + d[6]), zeta));
  c[5] = barrett_reduce(CALC_D(a, b, 0, 5, d) + CALC_D(a, b, 1, 4, d) + CALC_D(a, b, 2, 3, d)) + fqmul(CALC_D(a, b, 6, 7, d), zeta);
  c[6] = barrett_reduce(CALC_D(a, b, 0, 6, d) + CALC_D(a, b, 1, 5, d) + CALC_D(a, b, 2, 4, d)) + d[3] + fqmul(d[7], zeta);
  c[7] = barrett_reduce(CALC_D(a, b, 0, 7, d) + CALC_D(a, b, 1, 6, d) + CALC_D(a, b, 2, 5, d) + CALC_D(a, b, 3, 4, d));
}