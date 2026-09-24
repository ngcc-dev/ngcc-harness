#include "avx2_poly.h"
#include "reduce.h"
#include "inv_dtru_prime1087_avx2.h"
#include "avx2_cbd.h"

static inline __m256i fqcsubq_vec(__m256i x, __m256i q) {
    __m256i t = _mm256_srai_epi16(x, 15);
    t = _mm256_and_si256(t, q);
    x = _mm256_add_epi16(x, t);
    x = _mm256_sub_epi16(x, q);
    t = _mm256_srai_epi16(x, 15);
    t = _mm256_and_si256(t, q);
    x = _mm256_add_epi16(x, t);
    return x;
}

void poly_fqcsubq_avx2(poly *a) {
    int i = 0;
    int limit = DTRU_N & ~15;
    __m256i q = _mm256_set1_epi16(DTRU_Q);

    for (; i < limit; i += 16) {
        __m256i v = _mm256_loadu_si256((__m256i *)(a->coeffs + i));
        v = fqcsubq_vec(v, q);
        _mm256_storeu_si256((__m256i *)(a->coeffs + i), v);
    }
    for (; i < DTRU_N; ++i) {
        a->coeffs[i] = fqcsubq(a->coeffs[i]);
    }
}

void poly_add_avx2(poly *c, const poly *a, const poly *b) {
    int i = 0;
    int limit = DTRU_N & ~15;

    for (; i < limit; i += 16) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b->coeffs + i));
        __m256i vc = _mm256_add_epi16(va, vb);
        _mm256_storeu_si256((__m256i *)(c->coeffs + i), vc);
    }
    for (; i < DTRU_N; ++i) {
        c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
    }
}

void poly_multi_p_avx2(poly *b, const poly *a) {
    int i = 0;
    int limit = DTRU_N & ~15;

    for (; i < limit; i += 16) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a->coeffs + i));
        __m256i vb = _mm256_slli_epi16(va, 1);
        _mm256_storeu_si256((__m256i *)(b->coeffs + i), vb);
    }
    for (; i < DTRU_N; ++i) {
        b->coeffs[i] = 2 * a->coeffs[i];
    }
}

void poly_inverse_avx2(poly *b, const poly *a) {
    inv_dtru_prime1087_avx2(b->coeffs, a->coeffs);
}

void poly_sample_keygen_f_avx2(poly *a, const unsigned char *buf)
{
  cbd1_avx2_fast(a, buf);
}

void poly_sample_keygen_g_avx2(poly *a, const unsigned char *buf)
{
  cbd2_avx2_intrinsic(a, buf);
}

void poly_sample_enc_r_avx2(poly *a, const unsigned char *buf)
{
  cbd2_avx2_intrinsic(a, buf);
}

void poly_sample_enc_e_avx2(poly *a, const unsigned char *buf)
{
  cbd1_avx2_fast(a, buf);
}
