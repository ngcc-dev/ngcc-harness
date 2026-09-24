#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "reduce.h"
#include "ntt.h"

int16_t zetas[128] = {
  171, 605, 688, 361, 583, 3, 250, 120, 640, 461, 223, 753, 626, 362, 432, 638, 694, 733, 76, 98, 203, 282, 430, 514, 178, 270, 199, 34, 400, 192, 620, 759, 689, 423, 645, 2, 114, 147, 715, 497, 600, 288, 161, 754, 683, 51, 405, 502, 170, 543, 648, 188, 719, 745, 307, 578, 263, 157, 523, 128, 375, 180, 389, 279, 428, 390, 202, 220, 236, 21, 212, 71, 635, 151, 23, 657, 537, 227, 717, 621, 244, 517, 532, 686, 652, 436, 703, 522, 477, 352, 624, 238, 493, 575, 495, 699, 209, 654, 670, 14, 29, 260, 391, 403, 355, 478, 358, 664, 167, 357, 528, 438, 421, 725, 691, 547, 419, 601, 611, 201, 303, 330, 585, 127, 318, 491, 416, 415
};

int16_t zetas_inv[128] = {
  354, 353, 278, 451, 642, 184, 439, 466, 568, 158, 168, 350, 222, 78, 44, 348, 331, 241, 412, 602, 105, 411, 291, 414, 366, 378, 509, 740, 755, 99, 115, 560, 70, 274, 194, 276, 531, 145, 417, 292, 247, 66, 333, 117, 83, 237, 252, 525, 148, 52, 542, 232, 112, 746, 618, 134, 698, 557, 748, 533, 549, 567, 379, 341, 490, 380, 589, 394, 641, 246, 612, 506, 191, 462, 24, 50, 581, 121, 226, 599, 267, 364, 718, 86, 15, 608, 481, 169, 272, 54, 622, 655, 767, 124, 346, 80, 10, 149, 577, 369, 735, 570, 499, 591, 255, 339, 487, 566, 671, 693, 36, 75, 131, 337, 407, 143, 16, 546, 308, 129, 649, 519, 766, 186, 408, 81, 164, 655
};

void ntt(int16_t *a)
{
  unsigned int len, start, j, k = 1;
  int16_t t, zeta;

  for (len = DTRU_N / 2; len >= ROOT_DIMENSION; len >>= 1)
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

  for (len = ROOT_DIMENSION; len <= 32; len <<= 1)
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

  for (len = 128; len <= DTRU_N / 2; len <<= 1)
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

  for (j = 0; j < DTRU_N; ++j)
  {
    a[j] = fqmul(a[j], zetas_inv[127]);
  }
}

#define CALC_D(a, b, x, y, d) (fqmul((a[x] + a[y]), (b[x] + b[y])) - d[x] - d[y])

void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t d[4];
  for (int i = 0; i < 4; i++)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul((CALC_D(a, b, 1, 3, d) + d[2]), zeta));
  c[1] = barrett_reduce(CALC_D(a, b, 0, 1, d) + fqmul(CALC_D(a, b, 2, 3, d), zeta));
  c[2] = barrett_reduce(CALC_D(a, b, 0, 2, d) + d[1] + fqmul(d[3], zeta));
  c[3] = barrett_reduce(CALC_D(a, b, 0, 3, d) + CALC_D(a, b, 1, 2, d));
}
