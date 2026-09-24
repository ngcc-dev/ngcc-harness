#include <stdint.h>
#include <stdio.h>

#include "BWcoding.c"

static uint64_t rng_state = 0x243f6a8885a308d3ULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static void ref_mul_phi_inv(int16_t *w, const int16_t *y, int n)
{
  int i;

  for(i = 0; i < n / 2; i++) {
    int16_t tmp = y[2 * i];
    w[2 * i] = (y[2 * i + 1] + tmp) >> 1;
    w[2 * i + 1] = (y[2 * i + 1] - tmp) >> 1;
  }
}

static void ref_mul_phi(int16_t *w, const int16_t *y, int n)
{
  int i;

  for(i = 0; i < n / 2; i++) {
    int16_t tmp = y[2 * i];
    w[2 * i] = tmp - y[2 * i + 1];
    w[2 * i + 1] = tmp + y[2 * i + 1];
  }
}

static uint32_t ref_vec_dis(const int16_t *y, const int16_t *w,
                            const int16_t *t1, const int16_t *t2,
                            int half_n)
{
  uint32_t sum = 0;
  int i;

  for(i = 0; i < half_n; i++) {
    int16_t d = y[i] - t1[i];
    sum += d * d;
  }
  for(i = 0; i < half_n; i++) {
    int16_t d = w[i] - t2[i];
    sum += d * d;
  }

  return sum;
}

static void ref_bdd(int16_t *w, const int16_t *t, int n)
{
  int half_n = n >> 1;
  int16_t y1[16], y2[16], z1[16], z2[16], w1[16], w2[16], tmp[16] = {0};
  uint32_t dis1;
  uint32_t dis2;
  uint16_t mask;
  int i;

  if(n == 2) {
    w[0] = ((t[0] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
    w[1] = ((t[1] + (lambda >> 1)) >> KYBER_EQ) << KYBER_EQ;
    return;
  }

  ref_bdd(y1, t, half_n);
  ref_bdd(y2, t + half_n, half_n);

  for(i = 0; i < half_n; i++) tmp[i] = t[half_n + i] - y1[i];
  ref_mul_phi_inv(tmp, tmp, half_n);
  ref_bdd(z1, tmp, half_n);

  for(i = 0; i < half_n; i++) tmp[i] = t[i] - y2[i];
  ref_mul_phi_inv(tmp, tmp, half_n);
  ref_bdd(z2, tmp, half_n);

  ref_mul_phi(w1, z1, half_n);
  for(i = 0; i < half_n; i++) w1[i] += y1[i];

  ref_mul_phi(w2, z2, half_n);
  for(i = 0; i < half_n; i++) w2[i] += y2[i];

  dis1 = ref_vec_dis(y1, w1, t, t + half_n, half_n);
  dis2 = ref_vec_dis(y2, w2, t + half_n, t, half_n);
  mask = -(((dis2 - dis1) >> 31) & 1);

  for(i = 0; i < half_n; i++) {
    w[i] = y1[i] ^ (mask & (y1[i] ^ w2[i]));
    w[i + half_n] = w1[i] ^ (mask & (w1[i] ^ y2[i]));
  }
}

static int compare_block(const int16_t *got, const int16_t *want, int n,
                         const char *name, unsigned int iter)
{
  int i;

  for(i = 0; i < n; i++) {
    if(got[i] != want[i]) {
      fprintf(stderr, "%s mismatch iter=%u coeff=%d got=%d want=%d\n",
              name, iter, i, (int)got[i], (int)want[i]);
      return 0;
    }
  }

  return 1;
}

static void fill_input(int16_t *t, int n, unsigned int iter)
{
  int i;

  for(i = 0; i < n; i++) {
    if(iter == 0) {
      t[i] = 0;
    } else if(iter == 1) {
      t[i] = (int16_t)(0x0fff << 2);
    } else if(iter == 2) {
      t[i] = (int16_t)((i & 1) ? (0x0fff << 2) : 0);
    } else {
      t[i] = (int16_t)(next_u32() & 0x3fff);
    }
  }
}

static int test_bdd4(unsigned int iter)
{
  int16_t t[16];
  int16_t got[16];
  int16_t want[4];
  int block;

  fill_input(t, 16, iter);
  _mm256_storeu_si256((__m256i *)got, bdd4_avx2_exact(_mm256_loadu_si256((const __m256i *)t)));

  for(block = 0; block < 4; block++) {
    ref_bdd(want, t + 4 * block, 4);
    if(!compare_block(got + 4 * block, want, 4, "BDD_4", iter)) return 0;
  }

  return 1;
}

static int test_bdd8(unsigned int iter)
{
  int16_t t[16];
  int16_t got[16];
  int16_t want[8];
  int block;

  fill_input(t, 16, iter);
  _mm256_storeu_si256((__m256i *)got, bdd8_avx2_exact(_mm256_loadu_si256((const __m256i *)t)));

  for(block = 0; block < 2; block++) {
    ref_bdd(want, t + 8 * block, 8);
    if(!compare_block(got + 8 * block, want, 8, "BDD_8", iter)) return 0;
  }

  return 1;
}

static int test_bdd16(unsigned int iter)
{
  int16_t t[16];
  int16_t got[16];
  int16_t want[16];

  fill_input(t, 16, iter);
  _mm256_storeu_si256((__m256i *)got, bdd16_avx2_exact(_mm256_loadu_si256((const __m256i *)t)));
  ref_bdd(want, t, 16);

  return compare_block(got, want, 16, "BDD_16", iter);
}

static int test_bdd32(unsigned int iter)
{
  int16_t t[32];
  int16_t got[32];
  int16_t want[32];

  fill_input(t, 32, iter);
  bdd32_avx2_exact(got,
                   _mm256_loadu_si256((const __m256i *)t),
                   _mm256_loadu_si256((const __m256i *)(t + 16)));
  ref_bdd(want, t, 32);

  return compare_block(got, want, 32, "BDD_32", iter);
}

int main(void)
{
  unsigned int i;

  for(i = 0; i < 10000u; i++) {
    if(!test_bdd4(i)) return 1;
    if(!test_bdd8(i)) return 1;
    if(!test_bdd16(i)) return 1;
    if(!test_bdd32(i)) return 1;
  }

  puts("BDD layer tests passed.");
  return 0;
}
