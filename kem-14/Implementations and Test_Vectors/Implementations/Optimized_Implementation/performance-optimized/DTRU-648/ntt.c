#include <stdint.h>
#include <stdio.h>
#include <immintrin.h>
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

static inline __m256i montgomery_mul_avx2(__m256i a, __m256i b)
{
  const __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
  const __m256i qinv_vec = _mm256_set1_epi16(QINV);
  __m256i t0, t1;

  t1 = _mm256_mullo_epi16(qinv_vec, b);
  t0 = _mm256_mullo_epi16(a, t1);
  t1 = _mm256_mulhi_epi16(a, b);
  t0 = _mm256_mulhi_epi16(q_vec, t0);
  return _mm256_sub_epi16(t1, t0);
}

static inline __m256i sign_extend_low16_epi32(__m256i x)
{
  return _mm256_srai_epi32(_mm256_slli_epi32(x, 16), 16);
}

static inline __m256i montgomery_reduce_vec8x32(__m256i a)
{
  const __m256i mask16 = _mm256_set1_epi32(0xFFFF);
  const __m256i q_vec = _mm256_set1_epi32(DTRU_Q);
  const __m256i qinv_vec = _mm256_set1_epi32(QINV);

  __m256i a_lo = _mm256_and_si256(a, mask16);
  __m256i u = _mm256_mullo_epi32(a_lo, qinv_vec);
  __m256i u_signed;
  __m256i t;

  u = _mm256_and_si256(u, mask16);
  u_signed = sign_extend_low16_epi32(u);
  t = _mm256_sub_epi32(a, _mm256_mullo_epi32(u_signed, q_vec));
  return _mm256_srai_epi32(t, 16);
}

static inline __m128i montgomery_mul_epi16x8(__m128i a, __m128i b)
{
  const __m128i q_vec = _mm_set1_epi16(DTRU_Q);
  const __m128i qinv_vec = _mm_set1_epi16(QINV);
  __m128i t0, t1;

  t1 = _mm_mullo_epi16(qinv_vec, b);
  t0 = _mm_mullo_epi16(a, t1);
  t1 = _mm_mulhi_epi16(a, b);
  t0 = _mm_mulhi_epi16(q_vec, t0);
  return _mm_sub_epi16(t1, t0);
}

static inline void ct_bfly_broadcast_epi16x8(__m128i *a, __m128i *b, int16_t zeta)
{
  __m128i x0 = *a;
  __m128i x1 = *b;
  __m128i zeta_vec = _mm_set1_epi16(zeta);
  __m128i t = montgomery_mul_epi16x8(zeta_vec, x1);

  *a = _mm_add_epi16(x0, t);
  *b = _mm_sub_epi16(x0, t);
}

static inline void ct_scalar(int16_t *x, int16_t *y, int16_t zeta)
{
  int16_t a = *x;
  int16_t t = fqmul(zeta, *y);

  *x = a + t;
  *y = a - t;
}

static inline void transpose8x8_i16(
  __m128i *r0, __m128i *r1, __m128i *r2, __m128i *r3,
  __m128i *r4, __m128i *r5, __m128i *r6, __m128i *r7)
{
  __m128i t0, t1, t2, t3, t4, t5, t6, t7;
  __m128i u0, u1, u2, u3, u4, u5, u6, u7;
  __m128i c0, c1, c2, c3, c4, c5, c6, c7;

  t0 = _mm_unpacklo_epi16(*r0, *r1);
  t1 = _mm_unpackhi_epi16(*r0, *r1);
  t2 = _mm_unpacklo_epi16(*r2, *r3);
  t3 = _mm_unpackhi_epi16(*r2, *r3);
  t4 = _mm_unpacklo_epi16(*r4, *r5);
  t5 = _mm_unpackhi_epi16(*r4, *r5);
  t6 = _mm_unpacklo_epi16(*r6, *r7);
  t7 = _mm_unpackhi_epi16(*r6, *r7);

  u0 = _mm_unpacklo_epi32(t0, t2);
  u1 = _mm_unpackhi_epi32(t0, t2);
  u2 = _mm_unpacklo_epi32(t1, t3);
  u3 = _mm_unpackhi_epi32(t1, t3);
  u4 = _mm_unpacklo_epi32(t4, t6);
  u5 = _mm_unpackhi_epi32(t4, t6);
  u6 = _mm_unpacklo_epi32(t5, t7);
  u7 = _mm_unpackhi_epi32(t5, t7);

  c0 = _mm_unpacklo_epi64(u0, u4);
  c1 = _mm_unpackhi_epi64(u0, u4);
  c2 = _mm_unpacklo_epi64(u1, u5);
  c3 = _mm_unpackhi_epi64(u1, u5);
  c4 = _mm_unpacklo_epi64(u2, u6);
  c5 = _mm_unpackhi_epi64(u2, u6);
  c6 = _mm_unpacklo_epi64(u3, u7);
  c7 = _mm_unpackhi_epi64(u3, u7);

  *r0 = c0; *r1 = c1; *r2 = c2; *r3 = c3;
  *r4 = c4; *r5 = c5; *r6 = c6; *r7 = c7;
}

/* First-layer butterfly matching ntt(): (u, v) -> (u + zeta*v, u + v - zeta*v).
 * Standard ct_bfly gives (u + zeta*v, u - zeta*v); the difference is the extra +v
 * in the lower output, matching the non-standard len=N/2 step in ntt(). */
static inline void ntt_first_bfly_epi16x8(__m128i *lo, __m128i *hi, int16_t zeta)
{
  __m128i x0 = *lo, x1 = *hi;
  __m128i zv = _mm_set1_epi16(zeta);
  __m128i t  = montgomery_mul_epi16x8(zv, x1);
  *lo = _mm_add_epi16(x0, t);
  *hi = _mm_add_epi16(x0, _mm_sub_epi16(x1, t));   /* x0 + x1 - t */
}

static inline void ntt_first_scalar_v2(int16_t *lo, int16_t *hi, int16_t zeta)
{
  int16_t x0 = *lo, x1 = *hi;
  int16_t t = fqmul(zeta, x1);
  *lo = x0 + t;
  *hi = x0 + x1 - t;
}

static void split8x81_ntt_avx2(__m256i vec81[81], const int16_t src[648])
{
  const int16_t z1 = zetas[1];
  const int16_t z2 = zetas[2];
  const int16_t z3 = zetas[3];
  const int16_t z4 = zetas[4];
  const int16_t z5 = zetas[5];
  const int16_t z6 = zetas[6];
  const int16_t z7 = zetas[7];

  for (int base = 0; base <= 72; base += 8) {
    __m128i r0 = _mm_loadu_si128((const __m128i *)(src + 0 * 81 + base));
    __m128i r1 = _mm_loadu_si128((const __m128i *)(src + 1 * 81 + base));
    __m128i r2 = _mm_loadu_si128((const __m128i *)(src + 2 * 81 + base));
    __m128i r3 = _mm_loadu_si128((const __m128i *)(src + 3 * 81 + base));
    __m128i r4 = _mm_loadu_si128((const __m128i *)(src + 4 * 81 + base));
    __m128i r5 = _mm_loadu_si128((const __m128i *)(src + 5 * 81 + base));
    __m128i r6 = _mm_loadu_si128((const __m128i *)(src + 6 * 81 + base));
    __m128i r7 = _mm_loadu_si128((const __m128i *)(src + 7 * 81 + base));

    /* Layer 1 (len=324): ntt() non-standard butterfly */
    ntt_first_bfly_epi16x8(&r0, &r4, z1);
    ntt_first_bfly_epi16x8(&r1, &r5, z1);
    ntt_first_bfly_epi16x8(&r2, &r6, z1);
    ntt_first_bfly_epi16x8(&r3, &r7, z1);

    /* Layers 2 and 3 (len=162, 81): standard CT, same as split8x81_avx2 */
    ct_bfly_broadcast_epi16x8(&r0, &r2, z2);
    ct_bfly_broadcast_epi16x8(&r1, &r3, z2);
    ct_bfly_broadcast_epi16x8(&r4, &r6, z3);
    ct_bfly_broadcast_epi16x8(&r5, &r7, z3);

    ct_bfly_broadcast_epi16x8(&r0, &r1, z4);
    ct_bfly_broadcast_epi16x8(&r2, &r3, z5);
    ct_bfly_broadcast_epi16x8(&r4, &r5, z6);
    ct_bfly_broadcast_epi16x8(&r6, &r7, z7);

    transpose8x8_i16(&r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7);

    vec81[base + 0] = _mm256_cvtepi16_epi32(r0);
    vec81[base + 1] = _mm256_cvtepi16_epi32(r1);
    vec81[base + 2] = _mm256_cvtepi16_epi32(r2);
    vec81[base + 3] = _mm256_cvtepi16_epi32(r3);
    vec81[base + 4] = _mm256_cvtepi16_epi32(r4);
    vec81[base + 5] = _mm256_cvtepi16_epi32(r5);
    vec81[base + 6] = _mm256_cvtepi16_epi32(r6);
    vec81[base + 7] = _mm256_cvtepi16_epi32(r7);
  }

  {
    int16_t s0 = src[0 * 81 + 80];
    int16_t s1 = src[1 * 81 + 80];
    int16_t s2 = src[2 * 81 + 80];
    int16_t s3 = src[3 * 81 + 80];
    int16_t s4 = src[4 * 81 + 80];
    int16_t s5 = src[5 * 81 + 80];
    int16_t s6 = src[6 * 81 + 80];
    int16_t s7 = src[7 * 81 + 80];

    ntt_first_scalar_v2(&s0, &s4, z1);
    ntt_first_scalar_v2(&s1, &s5, z1);
    ntt_first_scalar_v2(&s2, &s6, z1);
    ntt_first_scalar_v2(&s3, &s7, z1);

    ct_scalar(&s0, &s2, z2);
    ct_scalar(&s1, &s3, z2);
    ct_scalar(&s4, &s6, z3);
    ct_scalar(&s5, &s7, z3);

    ct_scalar(&s0, &s1, z4);
    ct_scalar(&s2, &s3, z5);
    ct_scalar(&s4, &s5, z6);
    ct_scalar(&s6, &s7, z7);

    vec81[80] = _mm256_setr_epi32(
      (int32_t)s0, (int32_t)s1, (int32_t)s2, (int32_t)s3,
      (int32_t)s4, (int32_t)s5, (int32_t)s6, (int32_t)s7);
  }
}

static inline __m256i fqmul_vec8x32_debug(__m256i a, __m256i b)
{
  return montgomery_reduce_vec8x32(_mm256_mullo_epi32(a, b));
}

static inline __m256i barrett_vec8x32_debug(__m256i x)
{
  const __m256i barrett_v = _mm256_set1_epi32(BARRETT_V);
  const __m256i q_vec = _mm256_set1_epi32(DTRU_Q);
  __m256i t = _mm256_mullo_epi32(x, barrett_v);
  t = _mm256_srai_epi32(t, 26);
  t = _mm256_mullo_epi32(t, q_vec);
  return _mm256_sub_epi32(x, t);
}

static inline void butterfly3_vec8x32_avx2(
  __m256i *a0, __m256i *a1, __m256i *a2,
  __m256i zeta, __m256i zeta1, __m256i rho)
{
  __m256i x0 = *a0;
  __m256i x1 = *a1;
  __m256i x2 = *a2;
  __m256i z0, z1, z2, d;
  __m256i out0, out1, out2;

  z0 = fqmul_vec8x32_debug(zeta, x1);
  z1 = fqmul_vec8x32_debug(zeta1, x2);
  d = _mm256_sub_epi32(z0, z1);
  z2 = fqmul_vec8x32_debug(rho, d);

  out1 = _mm256_sub_epi32(_mm256_add_epi32(x0, z2), z1);
  out2 = _mm256_sub_epi32(_mm256_sub_epi32(x0, z2), z0);
  out0 = _mm256_add_epi32(_mm256_add_epi32(x0, z0), z1);

  *a0 = barrett_vec8x32_debug(out0);
  *a1 = barrett_vec8x32_debug(out1);
  *a2 = barrett_vec8x32_debug(out2);
}

typedef struct ntt_twiddles_avx2_cache {
  int inited;
  __m256i rho;
  __m256i zeta27;
  __m256i zeta27_1;
  __m256i zeta9[3];
  __m256i zeta9_1[3];
  __m256i basemul_zeta0[3];
  __m256i basemul_zeta1[3];
  __m256i basemul_zeta2[3];
  /* inverse radix-3 twiddles */
  __m256i inv_rho;
  __m256i inv_zeta27;
  __m256i inv_zeta27_1;
  __m256i inv_zeta9[3];
  __m256i inv_zeta9_1[3];
} ntt_twiddles_avx2_cache;

static ntt_twiddles_avx2_cache g_tw_avx2;

static inline void ntt_twiddles_avx2_init_once(void)
{
  if (g_tw_avx2.inited) return;

  g_tw_avx2.rho = _mm256_set1_epi32((int32_t)zetas[0]);

  g_tw_avx2.zeta27 = _mm256_setr_epi32(
    (int32_t)zetas[8], (int32_t)zetas[10], (int32_t)zetas[12], (int32_t)zetas[14],
    (int32_t)zetas[16], (int32_t)zetas[18], (int32_t)zetas[20], (int32_t)zetas[22]);
  g_tw_avx2.zeta27_1 = _mm256_setr_epi32(
    (int32_t)zetas[9], (int32_t)zetas[11], (int32_t)zetas[13], (int32_t)zetas[15],
    (int32_t)zetas[17], (int32_t)zetas[19], (int32_t)zetas[21], (int32_t)zetas[23]);

  for (int sub = 0; sub < 3; ++sub) {
    int32_t tmp_zeta[8];
    int32_t tmp_zeta1[8];
    int32_t tmp_bm0[8];
    int32_t tmp_bm1[8];
    int32_t tmp_bm2[8];

    for (int blk = 0; blk < 8; ++blk) {
      int idx = 24 + 6 * blk + 2 * sub;
      int i = 3 * blk + sub;
      tmp_zeta[blk] = (int32_t)zetas[idx];
      tmp_zeta1[blk] = (int32_t)zetas[idx + 1];
      tmp_bm0[blk] = (int32_t)zetas[24 + 2 * i];
      tmp_bm1[blk] = (int32_t)zetas_base[i];
      tmp_bm2[blk] = (int32_t)(-zetas[24 + 2 * i] - zetas_base[i]);
    }

    g_tw_avx2.zeta9[sub] = _mm256_setr_epi32(
      tmp_zeta[0], tmp_zeta[1], tmp_zeta[2], tmp_zeta[3],
      tmp_zeta[4], tmp_zeta[5], tmp_zeta[6], tmp_zeta[7]);
    g_tw_avx2.zeta9_1[sub] = _mm256_setr_epi32(
      tmp_zeta1[0], tmp_zeta1[1], tmp_zeta1[2], tmp_zeta1[3],
      tmp_zeta1[4], tmp_zeta1[5], tmp_zeta1[6], tmp_zeta1[7]);
    g_tw_avx2.basemul_zeta0[sub] = _mm256_setr_epi32(
      tmp_bm0[0], tmp_bm0[1], tmp_bm0[2], tmp_bm0[3],
      tmp_bm0[4], tmp_bm0[5], tmp_bm0[6], tmp_bm0[7]);
    g_tw_avx2.basemul_zeta1[sub] = _mm256_setr_epi32(
      tmp_bm1[0], tmp_bm1[1], tmp_bm1[2], tmp_bm1[3],
      tmp_bm1[4], tmp_bm1[5], tmp_bm1[6], tmp_bm1[7]);
    g_tw_avx2.basemul_zeta2[sub] = _mm256_setr_epi32(
      tmp_bm2[0], tmp_bm2[1], tmp_bm2[2], tmp_bm2[3],
      tmp_bm2[4], tmp_bm2[5], tmp_bm2[6], tmp_bm2[7]);
  }

  /* inverse radix-3 twiddles */
  g_tw_avx2.inv_rho = _mm256_set1_epi32((int32_t)zetas_inv[71]);

  /* inverse len=27: blk=0..7, zeta=zetas_inv[48+2*blk], zeta1=zetas_inv[49+2*blk] */
  g_tw_avx2.inv_zeta27 = _mm256_setr_epi32(
    (int32_t)zetas_inv[48], (int32_t)zetas_inv[50],
    (int32_t)zetas_inv[52], (int32_t)zetas_inv[54],
    (int32_t)zetas_inv[56], (int32_t)zetas_inv[58],
    (int32_t)zetas_inv[60], (int32_t)zetas_inv[62]);
  g_tw_avx2.inv_zeta27_1 = _mm256_setr_epi32(
    (int32_t)zetas_inv[49], (int32_t)zetas_inv[51],
    (int32_t)zetas_inv[53], (int32_t)zetas_inv[55],
    (int32_t)zetas_inv[57], (int32_t)zetas_inv[59],
    (int32_t)zetas_inv[61], (int32_t)zetas_inv[63]);

  /* inverse len=9: sub=0..2, blk=0..7
   * scalar group g = 3*blk + sub → zetas_inv idx = 6*blk + 2*sub */
  for (int inv_sub = 0; inv_sub < 3; ++inv_sub) {
    int32_t tmp_iz[8], tmp_iz1[8];
    for (int inv_blk = 0; inv_blk < 8; ++inv_blk) {
      int inv_idx = 6 * inv_blk + 2 * inv_sub;
      tmp_iz[inv_blk]  = (int32_t)zetas_inv[inv_idx];
      tmp_iz1[inv_blk] = (int32_t)zetas_inv[inv_idx + 1];
    }
    g_tw_avx2.inv_zeta9[inv_sub] = _mm256_setr_epi32(
      tmp_iz[0], tmp_iz[1], tmp_iz[2], tmp_iz[3],
      tmp_iz[4], tmp_iz[5], tmp_iz[6], tmp_iz[7]);
    g_tw_avx2.inv_zeta9_1[inv_sub] = _mm256_setr_epi32(
      tmp_iz1[0], tmp_iz1[1], tmp_iz1[2], tmp_iz1[3],
      tmp_iz1[4], tmp_iz1[5], tmp_iz1[6], tmp_iz1[7]);
  }

  g_tw_avx2.inited = 1;
}

static inline void ntt_stage_len27_vec81_avx2(__m256i vec81[81])
{
  ntt_twiddles_avx2_init_once();

  for (int j = 0; j < 27; ++j) {
    butterfly3_vec8x32_avx2(
      &vec81[j],
      &vec81[j + 27],
      &vec81[j + 54],
      g_tw_avx2.zeta27,
      g_tw_avx2.zeta27_1,
      g_tw_avx2.rho);
  }
}

static inline void ntt_stage_len9_vec81_avx2(__m256i vec81[81])
{
  ntt_twiddles_avx2_init_once();

  for (int sub = 0; sub < 3; ++sub) {
    int base = 27 * sub;
    __m256i zeta = g_tw_avx2.zeta9[sub];
    __m256i zeta1 = g_tw_avx2.zeta9_1[sub];

    for (int j = 0; j < 9; ++j) {
      butterfly3_vec8x32_avx2(
        &vec81[base + j],
        &vec81[base + j + 9],
        &vec81[base + j + 18],
        zeta,
        zeta1,
        g_tw_avx2.rho);
    }
  }
}

static inline __m256i calc_d_vec8x32(
  const __m256i a[9], const __m256i b[9], const __m256i d[9], int x, int y)
{
  __m256i ab = _mm256_add_epi32(a[x], a[y]);
  __m256i bb = _mm256_add_epi32(b[x], b[y]);
  __m256i t = fqmul_vec8x32_debug(ab, bb);
  t = _mm256_sub_epi32(t, d[x]);
  t = _mm256_sub_epi32(t, d[y]);
  return t;
}

static inline void basemul9_vec8x32_avx2(
  __m256i c[9], const __m256i a[9], const __m256i b[9], __m256i zeta)
{
  __m256i d[9];

  for (int i = 0; i < 9; ++i) {
    d[i] = fqmul_vec8x32_debug(a[i], b[i]);
  }

  c[0] = barrett_vec8x32_debug(_mm256_add_epi32(
    d[0],
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 1, 8), calc_d_vec8x32(a, b, d, 2, 7)),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 3, 6), calc_d_vec8x32(a, b, d, 4, 5))))));

  c[1] = barrett_vec8x32_debug(_mm256_add_epi32(
    calc_d_vec8x32(a, b, d, 0, 1),
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 2, 8), calc_d_vec8x32(a, b, d, 3, 7)),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 4, 6), d[5])))));

  c[2] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 2), d[1]),
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 3, 8), calc_d_vec8x32(a, b, d, 4, 7)),
      calc_d_vec8x32(a, b, d, 5, 6)))));

  c[3] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 3), calc_d_vec8x32(a, b, d, 1, 2)),
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 4, 8), calc_d_vec8x32(a, b, d, 5, 7)),
      d[6]))));

  c[4] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 4), calc_d_vec8x32(a, b, d, 1, 3)),
      d[2]),
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      calc_d_vec8x32(a, b, d, 5, 8), calc_d_vec8x32(a, b, d, 6, 7)))));

  c[5] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(
      calc_d_vec8x32(a, b, d, 0, 5),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 1, 4), calc_d_vec8x32(a, b, d, 2, 3))),
    fqmul_vec8x32_debug(zeta, _mm256_add_epi32(
      calc_d_vec8x32(a, b, d, 6, 8), d[7]))));

  c[6] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 6), calc_d_vec8x32(a, b, d, 1, 5)),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 2, 4), d[3])),
    fqmul_vec8x32_debug(zeta, calc_d_vec8x32(a, b, d, 7, 8))));

  c[7] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 7), calc_d_vec8x32(a, b, d, 1, 6)),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 2, 5), calc_d_vec8x32(a, b, d, 3, 4))),
    fqmul_vec8x32_debug(zeta, d[8])));

  c[8] = barrett_vec8x32_debug(_mm256_add_epi32(
    _mm256_add_epi32(
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 0, 8), calc_d_vec8x32(a, b, d, 1, 7)),
      _mm256_add_epi32(calc_d_vec8x32(a, b, d, 2, 6), calc_d_vec8x32(a, b, d, 3, 5))),
    d[4]));
}

/* Centered reduction for rq_inverse() arithmetic.
 *
 * inverse.c uses fq_freeze() (true mod-q reduction) to the centered range
 * [-((q-1)/2), +((q-1)/2)].
 *
 * rq_inverse() multiplies NTT-domain coefficients; those can be as large as a
 * few *q* (radix-2 layers don't reduce), so intermediate products can reach
 * ~1e9. We therefore must avoid 32-bit overflow in the Barrett quotient step.
 *
 * We compute:
 *   t = (x * BARRETT_V) >> 26
 * using 64-bit products (via _mm256_mul_epi32 on even/odd lanes) and an
 * emulated arithmetic right shift on 64-bit lanes. */
static inline __m256i fq_freeze_vec8x32(__m256i x)
{
  const __m256i q = _mm256_set1_epi32(DTRU_Q);
  const __m256i half_q = _mm256_set1_epi32((DTRU_Q - 1) >> 1);
  const __m256i neg_half_q = _mm256_sub_epi32(_mm256_setzero_si256(), half_q);
  const __m256i v = _mm256_set1_epi32(BARRETT_V);
  const __m256i mask32_64 = _mm256_set1_epi64x(0xFFFFFFFFULL);

  /* 64-bit products for even and odd 32-bit lanes. */
  __m256i x_odd = _mm256_srli_epi64(x, 32);
  __m256i prod_even = _mm256_mul_epi32(x, v);
  __m256i prod_odd = _mm256_mul_epi32(x_odd, v);

  /* Build per-64-bit-lane sign masks from x's sign (BARRETT_V > 0). */
  __m256i sign32 = _mm256_srai_epi32(x, 31); /* 0 or -1 */
  __m256i idx_even_pair = _mm256_setr_epi32(0, 0, 2, 2, 4, 4, 6, 6);
  __m256i idx_odd_pair  = _mm256_setr_epi32(1, 1, 3, 3, 5, 5, 7, 7);
  __m256i sign_even64 = _mm256_permutevar8x32_epi32(sign32, idx_even_pair);
  __m256i sign_odd64  = _mm256_permutevar8x32_epi32(sign32, idx_odd_pair);

  /* Arithmetic right shift on 64-bit lanes: (prod >> 26) with sign fill. */
  __m256i t_even64 = _mm256_or_si256(
      _mm256_srli_epi64(prod_even, 26),
      _mm256_slli_epi64(sign_even64, 64 - 26));
  __m256i t_odd64 = _mm256_or_si256(
      _mm256_srli_epi64(prod_odd, 26),
      _mm256_slli_epi64(sign_odd64, 64 - 26));

  /* Pack t back into 8x32 lanes: even lanes from t_even64, odd lanes from t_odd64. */
  __m256i t_even32 = _mm256_and_si256(t_even64, mask32_64);
  __m256i t_odd32 = _mm256_slli_epi64(_mm256_and_si256(t_odd64, mask32_64), 32);
  __m256i t32 = _mm256_or_si256(t_even32, t_odd32);

  __m256i r = _mm256_sub_epi32(x, _mm256_mullo_epi32(t32, q));

  /* Clamp into centered range with at most two corrections. */
  __m256i gt = _mm256_cmpgt_epi32(r, half_q);
  r = _mm256_sub_epi32(r, _mm256_and_si256(gt, q));
  __m256i lt = _mm256_cmpgt_epi32(neg_half_q, r); /* r < -half_q */
  r = _mm256_add_epi32(r, _mm256_and_si256(lt, q));

  gt = _mm256_cmpgt_epi32(r, half_q);
  r = _mm256_sub_epi32(r, _mm256_and_si256(gt, q));
  lt = _mm256_cmpgt_epi32(neg_half_q, r);
  r = _mm256_add_epi32(r, _mm256_and_si256(lt, q));

  return r;
}

/* Vectorized fqinv(): returns a^{-1} mod q for 8 lanes in parallel.
 * Matches reduce.c:fqinv(), keeping `t` in normal domain and `a` in Montgomery. */
static inline __m256i fqinv_vec8x32(__m256i a)
{
  __m256i t = _mm256_set1_epi32(1);
  __m256i aR = fqmul_vec8x32_debug(a, _mm256_set1_epi32(867)); /* aR = a * R */

  for (int exp = DTRU_Q - 2; exp > 0; exp >>= 1) {
    if (exp & 1) t = fqmul_vec8x32_debug(t, aR);
    aR = fqmul_vec8x32_debug(aR, aR);
  }
  return t;
}

/* SIMD version of inverse.c:rq_inverse() for 8 independent polynomials in parallel.
 * Input/Output are 9-coefficient polynomials in Z_q[x]/(x^9 - zeta_lane).
 * Returns an 8-bit failure mask (bit i set => lane i failed). */
static int rq_inverse9_simd(__m256i finv[9], const __m256i f[9], __m256i zeta)
{
  __m256i Phi[ROOT_DIMENSION + 1];
  __m256i V[ROOT_DIMENSION + 1];
  __m256i S[ROOT_DIMENSION + 1];
  __m256i F[ROOT_DIMENSION + 1];
  __m256i Delta = _mm256_set1_epi32(1);
  const __m256i zero = _mm256_setzero_si256();
  const __m256i one = _mm256_set1_epi32(1);
  const __m256i all1 = _mm256_set1_epi32(-1);

  for (int i = 0; i < ROOT_DIMENSION + 1; ++i) {
    Phi[i] = zero;
    V[i] = zero;
    S[i] = zero;
    F[i] = zero;
  }

  /* Phi = x^9 - zeta (same layout as inverse.c) */
  Phi[0] = one;
  /* fqmul(-zeta, 1) converts Montgomery zeta to a normal-domain -zeta. */
  Phi[ROOT_DIMENSION] = fqmul_vec8x32_debug(_mm256_sub_epi32(zero, zeta), one);

  /* S = 1 */
  S[0] = one;

  /* F = reverse(f) */
  for (int i = 0; i < ROOT_DIMENSION; ++i)
    F[i] = f[ROOT_DIMENSION - 1 - i];
  F[ROOT_DIMENSION] = zero;

  for (int loop = 0; loop < 2 * ROOT_DIMENSION - 1; ++loop) {
    /* V = x*V */
    for (int i = ROOT_DIMENSION; i > 0; --i)
      V[i] = V[i - 1];
    V[0] = zero;

    /* swap = (Delta > 0) & (F[0] != 0) */
    __m256i delta_pos = _mm256_cmpgt_epi32(Delta, zero);
    __m256i f0_eq0 = _mm256_cmpeq_epi32(F[0], zero);
    __m256i f0_nz = _mm256_xor_si256(f0_eq0, all1);
    __m256i swap = _mm256_and_si256(delta_pos, f0_nz);

    for (int i = 0; i < ROOT_DIMENSION + 1; ++i) {
      __m256i t = _mm256_and_si256(swap, _mm256_xor_si256(Phi[i], F[i]));
      Phi[i] = _mm256_xor_si256(Phi[i], t);
      F[i] = _mm256_xor_si256(F[i], t);

      t = _mm256_and_si256(swap, _mm256_xor_si256(V[i], S[i]));
      V[i] = _mm256_xor_si256(V[i], t);
      S[i] = _mm256_xor_si256(S[i], t);
    }

    /* Delta ^= swap & (Delta ^ -Delta); Delta++ */
    __m256i negDelta = _mm256_sub_epi32(zero, Delta);
    Delta = _mm256_xor_si256(Delta, _mm256_and_si256(swap, _mm256_xor_si256(Delta, negDelta)));
    Delta = _mm256_add_epi32(Delta, one);

    __m256i Phi0 = Phi[0];
    __m256i F0 = F[0];

    for (int i = 0; i < ROOT_DIMENSION + 1; ++i) {
      __m256i term1 = _mm256_mullo_epi32(Phi0, F[i]);
      __m256i term2 = _mm256_mullo_epi32(F0, Phi[i]);
      F[i] = fq_freeze_vec8x32(_mm256_sub_epi32(term1, term2));
    }
    for (int i = 0; i < ROOT_DIMENSION; ++i)
      F[i] = F[i + 1];
    F[ROOT_DIMENSION] = zero;

    for (int i = 0; i < ROOT_DIMENSION + 1; ++i) {
      __m256i term1 = _mm256_mullo_epi32(Phi0, S[i]);
      __m256i term2 = _mm256_mullo_epi32(F0, V[i]);
      S[i] = fq_freeze_vec8x32(_mm256_sub_epi32(term1, term2));
    }
  }

  /* scale = fqinv(Phi[0]) */
  __m256i scale = Phi[0];
  /* Match inverse.c: scale += (scale >> 15) & q; (for int16 scale). Here the
   * centered range implies "if (scale < 0) scale += q". */
  __m256i scale_neg = _mm256_cmpgt_epi32(zero, scale);
  scale = _mm256_add_epi32(scale, _mm256_and_si256(scale_neg, _mm256_set1_epi32(DTRU_Q)));
  scale = fqinv_vec8x32(scale);

  for (int i = 0; i < ROOT_DIMENSION; ++i) {
    __m256i prod = _mm256_mullo_epi32(scale, V[ROOT_DIMENSION - 1 - i]);
    finv[i] = fq_freeze_vec8x32(prod);
  }

  /* Failure if Delta != 0. */
  __m256i delta_eq0 = _mm256_cmpeq_epi32(Delta, zero);
  __m256i fail = _mm256_xor_si256(delta_eq0, all1);
  return _mm256_movemask_ps(_mm256_castsi256_ps(fail));
}

/* Inverse GS radix-3 butterfly in 81x8 SIMD domain.
 * Mirrors scalar invntt: zeta/zeta1 multiply the *outputs*, rho multiplies the
 * intermediate difference. */
static inline void ibutterfly3_vec8x32(
  __m256i *a0, __m256i *a1, __m256i *a2,
  __m256i zeta, __m256i zeta1, __m256i rho)
{
  __m256i x0 = *a0;
  __m256i z0 = *a1;
  __m256i z1 = *a2;
  __m256i z2 = fqmul_vec8x32_debug(rho, _mm256_sub_epi32(z0, z1));

  *a1 = fqmul_vec8x32_debug(zeta,
          _mm256_sub_epi32(_mm256_add_epi32(x0, z2), z1));
  *a2 = fqmul_vec8x32_debug(zeta1,
          _mm256_sub_epi32(_mm256_sub_epi32(x0, z2), z0));
  *a0 = barrett_vec8x32_debug(_mm256_add_epi32(_mm256_add_epi32(x0, z0), z1));
}

void invntt_stage_len9_vec81_avx2(__m256i vec81[81])
{
  ntt_twiddles_avx2_init_once();
  for (int sub = 0; sub < 3; ++sub) {
    int base = 27 * sub;
    __m256i zeta  = g_tw_avx2.inv_zeta9[sub];
    __m256i zeta1 = g_tw_avx2.inv_zeta9_1[sub];
    for (int j = 0; j < 9; ++j)
      ibutterfly3_vec8x32(
        &vec81[base + j], &vec81[base + j + 9], &vec81[base + j + 18],
        zeta, zeta1, g_tw_avx2.inv_rho);
  }
}

void invntt_stage_len27_vec81_avx2(__m256i vec81[81])
{
  ntt_twiddles_avx2_init_once();
  for (int j = 0; j < 27; ++j)
    ibutterfly3_vec8x32(
      &vec81[j], &vec81[j + 27], &vec81[j + 54],
      g_tw_avx2.inv_zeta27, g_tw_avx2.inv_zeta27_1, g_tw_avx2.inv_rho);
}

/* Narrow 8 int32 lanes to 8 int16 (values must be in [-32768, 32767]). */
static inline __m128i narrow_i32x8_to_i16x8(__m256i x)
{
  return _mm_packs_epi32(
    _mm256_castsi256_si128(x),
    _mm256_extracti128_si256(x, 1));
}

/* GS radix-2 inverse butterfly: a,b → a+b, fqmul(zeta, a-b). */
static inline void gs_bfly_broadcast_epi16x8(__m128i *a, __m128i *b, int16_t zeta)
{
  __m128i t = *a;
  __m128i zv = _mm_set1_epi16(zeta);
  *a = _mm_add_epi16(t, *b);
  *b = montgomery_mul_epi16x8(zv, _mm_sub_epi16(t, *b));
}

/* Final radix-2 merge butterfly matching scalar:
 *   t   = fqmul(z70, a - b)
 *   a   = fqmul(z72, a + b - t)
 *   b   = fqmul(z73, t)          */
static inline void final_merge_bfly_epi16x8(
  __m128i *a, __m128i *b,
  int16_t z70, int16_t z72, int16_t z73)
{
  __m128i z70v = _mm_set1_epi16(z70);
  __m128i z72v = _mm_set1_epi16(z72);
  __m128i z73v = _mm_set1_epi16(z73);
  __m128i sum  = _mm_add_epi16(*a, *b);
  __m128i t    = montgomery_mul_epi16x8(z70v, _mm_sub_epi16(*a, *b));
  *a = montgomery_mul_epi16x8(z72v, _mm_sub_epi16(sum, t));
  *b = montgomery_mul_epi16x8(z73v, t);
}

static inline void gs_scalar(int16_t *a, int16_t *b, int16_t zeta)
{
  int16_t t = *a;
  *a = t + *b;
  *b = fqmul(zeta, t - *b);
}

static inline void final_merge_scalar(int16_t *a, int16_t *b)
{
  int16_t t   = fqmul(zetas_inv[70], *a - *b);
  int16_t sum = *a + *b;
  *a = fqmul(zetas_inv[72], sum - t);
  *b = fqmul(zetas_inv[73], t);
}

void invntt_full_81x8(int16_t dst[648], const __m256i src[81])
{
  __m256i vec[81];
  int p, base;

  for (p = 0; p < 81; ++p)
    vec[p] = src[p];

  /* inverse radix-3 stages in 81x8 SIMD domain */
  invntt_stage_len9_vec81_avx2(vec);
  invntt_stage_len27_vec81_avx2(vec);

  /* layout recovery: tiles of 8 coefficients */
  for (base = 0; base <= 72; base += 8) {
    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    /* narrow int32 → int16; stages already reduced, no extra Barrett needed */
    r0 = narrow_i32x8_to_i16x8(vec[base + 0]);
    r1 = narrow_i32x8_to_i16x8(vec[base + 1]);
    r2 = narrow_i32x8_to_i16x8(vec[base + 2]);
    r3 = narrow_i32x8_to_i16x8(vec[base + 3]);
    r4 = narrow_i32x8_to_i16x8(vec[base + 4]);
    r5 = narrow_i32x8_to_i16x8(vec[base + 5]);
    r6 = narrow_i32x8_to_i16x8(vec[base + 6]);
    r7 = narrow_i32x8_to_i16x8(vec[base + 7]);

    /* transpose: coefficient-major → block-major
     * r_blk[k] = value at position base+k in block blk */
    transpose8x8_i16(&r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7);

    /* GS radix-2, len=81: pairs (blk0,blk1),(blk2,blk3),(blk4,blk5),(blk6,blk7) */
    gs_bfly_broadcast_epi16x8(&r0, &r1, zetas_inv[64]);
    gs_bfly_broadcast_epi16x8(&r2, &r3, zetas_inv[65]);
    gs_bfly_broadcast_epi16x8(&r4, &r5, zetas_inv[66]);
    gs_bfly_broadcast_epi16x8(&r6, &r7, zetas_inv[67]);

    /* GS radix-2, len=162: (blk0,blk2) and (blk1,blk3) share one zeta;
     *                       (blk4,blk6) and (blk5,blk7) share another */
    gs_bfly_broadcast_epi16x8(&r0, &r2, zetas_inv[68]);
    gs_bfly_broadcast_epi16x8(&r1, &r3, zetas_inv[68]);
    gs_bfly_broadcast_epi16x8(&r4, &r6, zetas_inv[69]);
    gs_bfly_broadcast_epi16x8(&r5, &r7, zetas_inv[69]);

    /* final merge, len=324: (blk0,blk4),(blk1,blk5),(blk2,blk6),(blk3,blk7) */
    final_merge_bfly_epi16x8(&r0, &r4, zetas_inv[70], zetas_inv[72], zetas_inv[73]);
    final_merge_bfly_epi16x8(&r1, &r5, zetas_inv[70], zetas_inv[72], zetas_inv[73]);
    final_merge_bfly_epi16x8(&r2, &r6, zetas_inv[70], zetas_inv[72], zetas_inv[73]);
    final_merge_bfly_epi16x8(&r3, &r7, zetas_inv[70], zetas_inv[72], zetas_inv[73]);

    /* store: each r_blk holds 8 consecutive positions of block blk */
    _mm_storeu_si128((__m128i *)(dst + 0 * 81 + base), r0);
    _mm_storeu_si128((__m128i *)(dst + 1 * 81 + base), r1);
    _mm_storeu_si128((__m128i *)(dst + 2 * 81 + base), r2);
    _mm_storeu_si128((__m128i *)(dst + 3 * 81 + base), r3);
    _mm_storeu_si128((__m128i *)(dst + 4 * 81 + base), r4);
    _mm_storeu_si128((__m128i *)(dst + 5 * 81 + base), r5);
    _mm_storeu_si128((__m128i *)(dst + 6 * 81 + base), r6);
    _mm_storeu_si128((__m128i *)(dst + 7 * 81 + base), r7);
  }

  /* scalar tail: p=80 (one position per block) */
  {
    int32_t lanes[8];
    int16_t s[8];
    _mm256_storeu_si256((__m256i *)lanes, vec[80]);
    for (int blk = 0; blk < 8; ++blk)
      s[blk] = (int16_t)lanes[blk];

    gs_scalar(&s[0], &s[1], zetas_inv[64]);
    gs_scalar(&s[2], &s[3], zetas_inv[65]);
    gs_scalar(&s[4], &s[5], zetas_inv[66]);
    gs_scalar(&s[6], &s[7], zetas_inv[67]);

    gs_scalar(&s[0], &s[2], zetas_inv[68]);
    gs_scalar(&s[1], &s[3], zetas_inv[68]);
    gs_scalar(&s[4], &s[6], zetas_inv[69]);
    gs_scalar(&s[5], &s[7], zetas_inv[69]);

    final_merge_scalar(&s[0], &s[4]);
    final_merge_scalar(&s[1], &s[5]);
    final_merge_scalar(&s[2], &s[6]);
    final_merge_scalar(&s[3], &s[7]);

    for (int blk = 0; blk < 8; ++blk)
      dst[blk * 81 + 80] = s[blk];
  }
}

void ntt_forward_81x8_v2(__m256i vec81[81], const int16_t src[648])
{
  split8x81_ntt_avx2(vec81, src);
  ntt_stage_len27_vec81_avx2(vec81);
  ntt_stage_len9_vec81_avx2(vec81);
}

void basemul_81x8(__m256i c[81], const __m256i a[81], const __m256i b[81])
{
  ntt_twiddles_avx2_init_once();

  for (int sub = 0; sub < 3; ++sub) {
    int base = 27 * sub;
    basemul9_vec8x32_avx2(c + base, a + base, b + base, g_tw_avx2.basemul_zeta0[sub]);
    basemul9_vec8x32_avx2(c + base + 9, a + base + 9, b + base + 9, g_tw_avx2.basemul_zeta1[sub]);
    basemul9_vec8x32_avx2(c + base + 18, a + base + 18, b + base + 18, g_tw_avx2.basemul_zeta2[sub]);
  }
}

int baseinv_81x8(__m256i b[81], const __m256i a[81])
{
  ntt_twiddles_avx2_init_once();

  int final_mask = 0;

  for (int sub = 0; sub < 3; ++sub) {
    int base = 27 * sub;
    final_mask |= rq_inverse9_simd(b + base + 0,  a + base + 0,  g_tw_avx2.basemul_zeta0[sub]);
    final_mask |= rq_inverse9_simd(b + base + 9,  a + base + 9,  g_tw_avx2.basemul_zeta1[sub]);
    final_mask |= rq_inverse9_simd(b + base + 18, a + base + 18, g_tw_avx2.basemul_zeta2[sub]);
  }

  return final_mask;
}

static inline void ntt_radix2_layer_scalar(int16_t *a, unsigned int len, unsigned int *k)
{
  unsigned int start, j;
  int16_t t, zeta;

  for (start = 0; start < DTRU_N; start = j + len)
  {
    zeta = zetas[(*k)++];
    for (j = start; j < start + len; ++j)
    {
      t = fqmul(zeta, a[j + len]);
      a[j + len] = (a[j] - t);
      a[j] = (a[j] + t);
    }
  }
}

static inline void ntt_radix2_layer_avx2_impl(int16_t *a, unsigned int len, unsigned int *k)
{
  unsigned int start, j;
  int16_t zeta;

  for (start = 0; start < DTRU_N; start = j + len)
  {
    zeta = zetas[(*k)++];
    __m256i zeta_vec = _mm256_set1_epi16(zeta);

    for (j = start; j + 15 < start + len; j += 16)
    {
      __m256i left = _mm256_loadu_si256((const __m256i *)(a + j));
      __m256i right = _mm256_loadu_si256((const __m256i *)(a + j + len));
      __m256i t = montgomery_mul_avx2(zeta_vec, right);

      _mm256_storeu_si256((__m256i *)(a + j), _mm256_add_epi16(left, t));
      _mm256_storeu_si256((__m256i *)(a + j + len), _mm256_sub_epi16(left, t));
    }

    for (; j < start + len; ++j)
    {
      int16_t t = fqmul(zeta, a[j + len]);
      a[j + len] = (a[j] - t);
      a[j] = (a[j] + t);
    }
  }
}

static inline void ntt_radix3_tail(int16_t *a, unsigned int *k, int16_t rho)
{
  unsigned int start, j;
  int16_t z[3], zeta, zeta1;
  unsigned int len = 27;

  for (start = 0; start < DTRU_N; start = j + 2 * len)
  {
    zeta = zetas[(*k)++];
    zeta1 = zetas[(*k)++];
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
  for (start = 0; start < DTRU_N; start = j + 2 * len)
  {
    zeta = zetas[(*k)++];
    zeta1 = zetas[(*k)++];
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

void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta)
{
  int16_t d[9];
  for (int i = 0; i < 9; i++)
    d[i] = fqmul(a[i], b[i]);

  c[0] = barrett_reduce(d[0] + fqmul(zeta, (CALC_D(a, b, 1, 8, d) + CALC_D(a, b, 2, 7, d) + CALC_D(a, b, 3, 6, d) + CALC_D(a, b, 4, 5, d))));
  c[1] = barrett_reduce(CALC_D(a, b, 0, 1, d) + fqmul(zeta, (CALC_D(a, b, 2, 8, d) + CALC_D(a, b, 3, 7, d) + CALC_D(a, b, 4, 6, d) + d[5])));
  c[2] = barrett_reduce(CALC_D(a, b, 0, 2, d) + d[1] + fqmul(zeta, (CALC_D(a, b, 3, 8, d) + CALC_D(a, b, 4, 7, d) + CALC_D(a, b, 5, 6, d))));
  c[3] = barrett_reduce(CALC_D(a, b, 0, 3, d) + CALC_D(a, b, 1, 2, d) + fqmul(zeta, (CALC_D(a, b, 4, 8, d) + CALC_D(a, b, 5, 7, d) + d[6])));
  c[4] = barrett_reduce(CALC_D(a, b, 0, 4, d) + CALC_D(a, b, 1, 3, d) + d[2] + fqmul(zeta, (CALC_D(a, b, 5, 8, d) + CALC_D(a, b, 6, 7, d))));
  c[5] = barrett_reduce(CALC_D(a, b, 0, 5, d) + CALC_D(a, b, 1, 4, d) + CALC_D(a, b, 2, 3, d) + fqmul(zeta, (CALC_D(a, b, 6, 8, d) + d[7])));
  c[6] = barrett_reduce(CALC_D(a, b, 0, 6, d) + CALC_D(a, b, 1, 5, d) + CALC_D(a, b, 2, 4, d) + d[3] + fqmul(zeta, CALC_D(a, b, 7, 8, d)));
  c[7] = barrett_reduce(CALC_D(a, b, 0, 7, d) + CALC_D(a, b, 1, 6, d) + CALC_D(a, b, 2, 5, d) + CALC_D(a, b, 3, 4, d) + fqmul(zeta, d[8]));
  c[8] = barrett_reduce(CALC_D(a, b, 0, 8, d) + CALC_D(a, b, 1, 7, d) + CALC_D(a, b, 2, 6, d) + CALC_D(a, b, 3, 5, d) + d[4]);
}
