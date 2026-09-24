#include "linear_algebra.h"

#include <immintrin.h>
#include <string.h>

static uint8_t gf31_mul_lut[31][32];
static int gf31_lut_ready;

static void init_linear_algebra(void)
{
    if (gf31_lut_ready)
        return;
    for (int s = 0; s < TSUOV_q; s++) {
        for (int v = 0; v < 32; v++)
            gf31_mul_lut[s][v] = (v < TSUOV_q) ? Fq_mul((Fq)s, (Fq)v) : 0;
    }
    gf31_lut_ready = 1;
}

static inline __m128i gf31_mul_scalar_128(__m128i x, Fq scalar)
{
    const uint8_t *lut = gf31_mul_lut[scalar];
    __m128i v = _mm_and_si128(x, _mm_set1_epi8(0x1F));
    __m128i lo = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)lut), v);
    __m128i hi = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i *)(lut + 16)),
                                  _mm_sub_epi8(v, _mm_set1_epi8(16)));
    __m128i hi_sel = _mm_cmpgt_epi8(v, _mm_set1_epi8(15));
    return _mm_blendv_epi8(lo, hi, hi_sel);
}

static inline __m256i gf31_mul_scalar_256(__m256i x, Fq scalar)
{
    __m128i lo = _mm256_castsi256_si128(x);
    __m128i hi = _mm256_extracti128_si256(x, 1);
    return _mm256_set_m128i(gf31_mul_scalar_128(hi, scalar),
                             gf31_mul_scalar_128(lo, scalar));
}

static inline __m256i gf31_sub_256(__m256i a, __m256i b)
{
    __m256i diff = _mm256_sub_epi8(a, b);
    __m256i mask = _mm256_cmpgt_epi8(_mm256_setzero_si256(), diff);
    return _mm256_add_epi8(diff, _mm256_and_si256(mask, _mm256_set1_epi8(TSUOV_q)));
}

static void gf31_row_scale(Fq *row, int start, int end, Fq inv)
{
    int k = start;
    for (; k + 32 <= end; k += 32) {
        __m256i vx = _mm256_loadu_si256((__m256i *)&row[k]);
        _mm256_storeu_si256((__m256i *)&row[k], gf31_mul_scalar_256(vx, inv));
    }
    for (; k < end; k++)
        row[k] = Fq_mul(inv, row[k]);
}

static void gf31_row_axpy(Fq *dst, const Fq *src, int start, int end, Fq mul)
{
    int k = start;
    for (; k + 32 <= end; k += 32) {
        __m256i vd = _mm256_loadu_si256((__m256i *)&dst[k]);
        __m256i vs = _mm256_loadu_si256((__m256i *)&src[k]);
        _mm256_storeu_si256((__m256i *)&dst[k],
                             gf31_sub_256(vd, gf31_mul_scalar_256(vs, mul)));
    }
    for (; k < end; k++)
        dst[k] = Fq_sub(dst[k], Fq_mul(mul, src[k]));
}

static inline int hsum_epi16(__m256i x)
{
    const __m256i v_one = _mm256_set1_epi16(1);
    __m256i sum32_256 = _mm256_madd_epi16(x, v_one);
    __m128i sum32_lo = _mm256_castsi256_si128(sum32_256);
    __m128i sum32_hi = _mm256_extracti128_si256(sum32_256, 1);
    __m128i sum128 = _mm_add_epi32(sum32_lo, sum32_hi);
    __m128i hsum = _mm_hadd_epi32(sum128, sum128);
    hsum = _mm_hadd_epi32(hsum, hsum);
    return _mm_cvtsi128_si32(hsum);
}

static uint64_t gf31_dot_u64(const Fq *a, const Fq *b, int len)
{
    int i = 0;
    __m256i acc = _mm256_setzero_si256();

    for (; i + 32 <= len; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)&a[i]);
        __m256i vb = _mm256_loadu_si256((const __m256i *)&b[i]);
        acc = _mm256_add_epi16(acc, _mm256_maddubs_epi16(va, vb));
    }

    uint64_t t = (uint64_t)hsum_epi16(acc);
    for (; i < len; i++)
        t += (uint64_t)a[i] * (uint64_t)b[i];
    return t;
}

static void echelon_form_init(Fq mat[TSUOV_m2][TSUOV_m2], ECHELON_FORM echelon_form)
{
    for (int i = 0; i < TSUOV_m2; i++) {
        echelon_form->row[i].col = mat[i];
        echelon_form->row[i].original_row_id = i;
        echelon_form->eqn[i] = echelon_form->row + i;
    }
    echelon_form->rank = 0;
    memset(echelon_form->index, -1, sizeof(echelon_form->index));
}

static void row_swap(ROW_s *eqn[TSUOV_m2], int i, int j)
{
    ROW_s *tmp = eqn[i];
    eqn[i] = eqn[j];
    eqn[j] = tmp;
}

#define EQN(I, J) (eqn[I]->col[J])

void LU_decompose(Fq A[TSUOV_m2][TSUOV_m2], ECHELON_FORM echelon_form)
{
    init_linear_algebra();
    echelon_form_init(A, echelon_form);
    ROW_s **eqn = echelon_form->eqn;

    int c = -1;
    for (int i = 0; i < TSUOV_m2; i++) {
        c++;
        if (c >= TSUOV_m2)
            return;
        int j = i;
        while (EQN(j, c) == 0) {
            j++;
            if (j >= TSUOV_m2) {
                c++;
                if (c >= TSUOV_m2)
                    return;
                j = i;
            }
        }

        row_swap(eqn, i, j);
        echelon_form->index[echelon_form->rank++] = c;

        Fq pivot = EQN(i, c);
        Fq inv = Fq_inv(pivot);

        EQN(i, i) = pivot;
        gf31_row_scale(eqn[i]->col, c + 1, TSUOV_m2, inv);

        for (j = i + 1; j < TSUOV_m2; j++) {
            Fq mul = EQN(j, c);
            EQN(j, i) = mul;
            gf31_row_axpy(eqn[j]->col, eqn[i]->col, c + 1, TSUOV_m2, mul);
        }
    }
}

static void Fq_mxm_identity(Fq A[TSUOV_m2][TSUOV_m2])
{
    memset(A, 0, sizeof(Fq) * TSUOV_m2 * TSUOV_m2);
    for (int i = 0; i < TSUOV_m2; i++)
        A[i][i] = 1;
}

static void L_inverse(ECHELON_FORM echelon_form, Fq R[TSUOV_m2][TSUOV_m2])
{
    init_linear_algebra();
    ROW_s **eqn = echelon_form->eqn;
    int rank = echelon_form->rank;

    Fq_mxm_identity(R);

    for (int i = 0; i < rank; i++) {
        Fq inv = Fq_inv(EQN(i, i));
        gf31_row_scale(R[i], 0, i + 1, inv);
        for (int j = i + 1; j < TSUOV_m2; j++) {
            Fq mul = EQN(j, i);
            gf31_row_axpy(R[j], R[i], 0, i + 1, mul);
        }
    }
}

int consistent(ECHELON_FORM echelon_form, Fq b[TSUOV_m2], int *cacheR,
               Fq R[TSUOV_m2][TSUOV_m2])
{
    ROW_s **eqn = echelon_form->eqn;
    int rank = echelon_form->rank;
    if (rank == TSUOV_m2)
        return 1;

    if (*cacheR == 0) {
        L_inverse(echelon_form, R);
        *cacheR = 1;
    }

    for (int i = rank; i < TSUOV_m2; i++) {
        uint64_t t = 0;
        for (int j = 0; j < rank; j++) {
            int k = eqn[j]->original_row_id;
            t += (uint64_t)R[i][j] * (uint64_t)b[k];
        }
        int k = eqn[i]->original_row_id;
        t += (uint64_t)R[i][i] * (uint64_t)b[k];
        t %= TSUOV_q;
        if (t)
            return 0;
    }

    return 1;
}

void sample_a_solution(const TSUOV_SEED seed_sol, ECHELON_FORM echelon_form,
                       Fq b[TSUOV_m2], Fq x[TSUOV_m2], Fq b2[TSUOV_m2])
{
    int rank = echelon_form->rank;
    int *index = echelon_form->index;
    ROW_s **eqn = echelon_form->eqn;

    uint8_t random_buff[TSUOV_m2];
    uint8_t *random_element = random_buff;
    if (rank < TSUOV_m2)
        tsuov_expand_sol(seed_sol, random_buff);

    for (int i = 0; i < rank; i++) {
        uint64_t t = gf31_dot_u64(eqn[i]->col, b2, i);
        Fq tmp = (Fq)(t % TSUOV_q);
        int k = eqn[i]->original_row_id;
        tmp = Fq_sub(b[k], tmp);
        b2[i] = Fq_mul(tmp, Fq_inv(EQN(i, i)));
    }

    int i = TSUOV_m2 - 1;
    for (int j = rank - 1; j >= 0; j--) {
        int k = index[j];
        for (; i > k; i--)
            x[i] = *random_element++;
        uint64_t t = gf31_dot_u64(&eqn[j]->col[k + 1], &x[k + 1], TSUOV_m2 - (k + 1));
        x[i] = Fq_sub(b2[j], (Fq)(t % TSUOV_q));
        i--;
    }
    for (; i >= 0; i--)
        x[i] = *random_element++;
}
