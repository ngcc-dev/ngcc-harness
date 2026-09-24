#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "polyvec.h"
#include "poly.h"
#include "ntt.h"
#include "consts.h"

void polyvec_matrix_expand_row0(polyvecl *rowa,
                                __attribute__((unused)) polyvecl *rowb,
                                const uint8_t rho[SEEDBYTES]) {
  poly_uniform_4x(&rowa->vec[0], &rowa->vec[1], &rowa->vec[2], &rowa->vec[3],
                  rho, 0, 1, 2, 3);
}

void polyvec_matrix_expand_row1(polyvecl *rowa,
                                __attribute__((unused)) polyvecl *rowb,
                                const uint8_t rho[SEEDBYTES]) {
  poly_uniform_4x(&rowa->vec[0], &rowa->vec[1], &rowa->vec[2], &rowa->vec[3],
                  rho, 256, 257, 258, 259);
}

void polyvec_matrix_expand_row2(polyvecl *rowa,
                                __attribute__((unused)) polyvecl *rowb,
                                const uint8_t rho[SEEDBYTES]) {
  poly_uniform_4x(&rowa->vec[0], &rowa->vec[1], &rowa->vec[2], &rowa->vec[3],
                  rho, 512, 513, 514, 515);
}

void polyvec_matrix_expand_row3(polyvecl *rowa,
                                __attribute__((unused)) polyvecl *rowb,
                                const uint8_t rho[SEEDBYTES]) {
  poly_uniform_4x(&rowa->vec[0], &rowa->vec[1], &rowa->vec[2], &rowa->vec[3],
                  rho, 768, 769, 770, 771);
}

void polyvec_matrix_expand(polyvecl mat[K], const uint8_t rho[SEEDBYTES]) {
  polyvec_matrix_expand_row0(&mat[0], NULL, rho);
  polyvec_matrix_expand_row1(&mat[1], NULL, rho);
  polyvec_matrix_expand_row2(&mat[2], NULL, rho);
  polyvec_matrix_expand_row3(&mat[3], NULL, rho);
}

static void polyvecl_pointwise_acc_montgomery_x4(poly *w0,
                                                 poly *w1,
                                                 poly *w2,
                                                 poly *w3,
                                                 const polyvecl *u0,
                                                 const polyvecl *u1,
                                                 const polyvecl *u2,
                                                 const polyvecl *u3,
                                                 const polyvecl *v) {
  poly t0, t1, t2, t3;
  unsigned int i, j;

  pointwise_avx(w0->vec, u0->vec[0].vec, v->vec[0].vec, qdata.vec);
  pointwise_avx(w1->vec, u1->vec[0].vec, v->vec[0].vec, qdata.vec);
  pointwise_avx(w2->vec, u2->vec[0].vec, v->vec[0].vec, qdata.vec);
  pointwise_avx(w3->vec, u3->vec[0].vec, v->vec[0].vec, qdata.vec);

  for(i = 1; i < L; ++i) {
    pointwise_avx(t0.vec, u0->vec[i].vec, v->vec[i].vec, qdata.vec);
    pointwise_avx(t1.vec, u1->vec[i].vec, v->vec[i].vec, qdata.vec);
    pointwise_avx(t2.vec, u2->vec[i].vec, v->vec[i].vec, qdata.vec);
    pointwise_avx(t3.vec, u3->vec[i].vec, v->vec[i].vec, qdata.vec);

    for(j = 0; j < N/8; ++j) {
      __m256i f0 = _mm256_load_si256(&w0->vec[j]);
      __m256i g0 = _mm256_load_si256(&t0.vec[j]);
      __m256i f1 = _mm256_load_si256(&w1->vec[j]);
      __m256i g1 = _mm256_load_si256(&t1.vec[j]);
      __m256i f2 = _mm256_load_si256(&w2->vec[j]);
      __m256i g2 = _mm256_load_si256(&t2.vec[j]);
      __m256i f3 = _mm256_load_si256(&w3->vec[j]);
      __m256i g3 = _mm256_load_si256(&t3.vec[j]);

      _mm256_store_si256(&w0->vec[j], _mm256_add_epi32(f0, g0));
      _mm256_store_si256(&w1->vec[j], _mm256_add_epi32(f1, g1));
      _mm256_store_si256(&w2->vec[j], _mm256_add_epi32(f2, g2));
      _mm256_store_si256(&w3->vec[j], _mm256_add_epi32(f3, g3));
    }
  }
}

void polyvec_matrix_pointwise_montgomery(polyveck *t, const polyvecl mat[K], const polyvecl *v) {
  polyvecl_pointwise_acc_montgomery_x4(&t->vec[0],
                                       &t->vec[1],
                                       &t->vec[2],
                                       &t->vec[3],
                                       &mat[0],
                                       &mat[1],
                                       &mat[2],
                                       &mat[3],
                                       v);
}

void polyvecl_uniform_eta(polyvecl *v, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_uniform_eta(&v->vec[i], seed, nonce++);
}

void polyvecl_uniform_gamma1(polyvecl *v, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_uniform_gamma1(&v->vec[i], seed, (uint16_t)(L*nonce + i));
}

void polyvecl_reduce(polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_reduce(&v->vec[i]);
}

void polyvecl_add(polyvecl *w, const polyvecl *u, const polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_add(&w->vec[i], &u->vec[i], &v->vec[i]);
}

void polyvecl_ntt(polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_ntt(&v->vec[i]);
}

void polyvecl_invntt_tomont(polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_invntt_tomont(&v->vec[i]);
}

void polyvecl_pointwise_poly_montgomery(polyvecl *r, const poly *a, const polyvecl *v) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    poly_pointwise_montgomery(&r->vec[i], a, &v->vec[i]);
}

void polyvecl_pointwise_acc_montgomery(poly *w, const polyvecl *u, const polyvecl *v) {
  poly t;

  pointwise_avx(w->vec, u->vec[0].vec, v->vec[0].vec, qdata.vec);
  for(unsigned i = 1; i < L; ++i) {
    pointwise_avx(t.vec, u->vec[i].vec, v->vec[i].vec, qdata.vec);
    for(unsigned j = 0; j < N/8; ++j) {
      __m256i f = _mm256_load_si256(&w->vec[j]);
      __m256i g = _mm256_load_si256(&t.vec[j]);
      _mm256_store_si256(&w->vec[j], _mm256_add_epi32(f, g));
    }
  }
}

int polyvecl_chknorm(const polyvecl *v, int32_t B) {
  unsigned int i;

  for(i = 0; i < L; ++i)
    if(poly_chknorm(&v->vec[i], B))
      return 1;
  return 0;
}

/**************************************************************/
/************ Vectors of polynomials of length K **************/
/**************************************************************/

void polyveck_uniform_eta(polyveck *v, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_uniform_eta(&v->vec[i], seed, nonce++);
}

void polyveck_reduce(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_reduce(&v->vec[i]);
}

void polyveck_caddq(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_caddq(&v->vec[i]);
}

void polyveck_add(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_add(&w->vec[i], &u->vec[i], &v->vec[i]);
}

void polyveck_sub(polyveck *w, const polyveck *u, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_sub(&w->vec[i], &u->vec[i], &v->vec[i]);
}

void polyveck_shiftl(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_shiftl(&v->vec[i]);
}

void polyveck_ntt(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_ntt(&v->vec[i]);
}

void polyveck_invntt_tomont(polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_invntt_tomont(&v->vec[i]);
}

void polyveck_pointwise_poly_montgomery(polyveck *r, const poly *a, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_pointwise_montgomery(&r->vec[i], a, &v->vec[i]);
}

void polyveck_power2round(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_power2round(&v1->vec[i], &v0->vec[i], &v->vec[i]);
}

void polyveck_decompose(polyveck *v1, polyveck *v0, const polyveck *v) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_decompose(&v1->vec[i], &v0->vec[i], &v->vec[i]);
}

unsigned int polyveck_make_hint(polyveck *h, const polyveck *v0, const polyveck *v1) {
  unsigned int s = 0;
  unsigned int i;

  for(i = 0; i < K; ++i)
    s += poly_make_hint(&h->vec[i], &v0->vec[i], &v1->vec[i]);
  return s;
}

void polyveck_use_hint(polyveck *w, const polyveck *u, const polyveck *h) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    poly_use_hint(&w->vec[i], &u->vec[i], &h->vec[i]);
}

int polyveck_chknorm(const polyveck *v, int32_t B) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    if(poly_chknorm(&v->vec[i], B))
      return 1;
  return 0;
}

void polyveck_pack_w1(uint8_t *r, const polyveck *w1) {
  unsigned int i;

  for(i = 0; i < K; ++i)
    polyw1_pack(r + i * POLYW1_PACKEDBYTES, &w1->vec[i]);
}

void polyveck_verify_batch(polyveck *w, polyveck *t1, const poly *c, const polyveck *h) {
  unsigned int i;

  for(i = 0; i < K; ++i) {
    poly_shiftl(&t1->vec[i]);
    poly_ntt(&t1->vec[i]);
    poly_pointwise_montgomery(&t1->vec[i], c, &t1->vec[i]);

    poly_sub(&w->vec[i], &w->vec[i], &t1->vec[i]);
    poly_reduce(&w->vec[i]);
    poly_invntt_tomont(&w->vec[i]);
    poly_caddq(&w->vec[i]);
    poly_use_hint(&w->vec[i], &w->vec[i], &h->vec[i]);
  }
}
