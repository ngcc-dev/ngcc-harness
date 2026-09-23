#include <stdint.h>
#include "params.h"
#include "poly.h"

#include<string.h>
#include "auxfunc.h"

#include <immintrin.h>

#define OVERFLOWING_MUL(X, Y) ((uint32_t)((uint32_t)(X) * (uint32_t)(Y)))


///* Sanity check — remove if Q > 4096 and switch r[] to uint64_t */
//#if defined(Q) && (Q > 4096)
//#warning "Q > 4096: uint32_t accumulators may overflow. Switch r[] to uint64_t."
//#endif
 
void schoolbook_8_wrap(poly * restrict       c,
                       const poly * restrict a,
                       const poly * restrict b)
{
    /*
     * r[0..2N-1]: linear convolution accumulator.
     * uint32_t wrapping arithmetic throughout — matches OVERFLOWING_MUL semantics.
     * Aligned 32 bytes for _mm256_load/store.
     */
    uint32_t r[2 * N] __attribute__((aligned(32)));
    memset(r, 0, sizeof(r));
 
    const uint32_t * restrict ac = a->coeffs;
    const uint32_t * restrict bc = b->coeffs;
 
    /* ------------------------------------------------------------------ */
    /* Inner loop — AVX2 broadcast multiply-accumulate                     */
    /*                                                                     */
    /* Scalar equivalent per iteration:                                    */
    /*   r[i+j] += OVERFLOWING_MUL(a[i], b[j])                           */
    /*                                                                     */
    /* AVX2: broadcast a[i] across 8 lanes, mullo with b[j..j+7],        */
    /*       add into r[i+j..i+j+7].                                      */
    /* _mm256_mullo_epi32 == low 32 bits of each product == OVERFLOWING_MUL */
    /* ------------------------------------------------------------------ */
    for (unsigned int i = 0; i < N; i++) {
        /* broadcast a[i] — scalar OVERFLOWING_MUL left operand */
        __m256i vai = _mm256_set1_epi32((int)ac[i]);
        uint32_t * restrict rp = r + i;
 
        for (unsigned int j = 0; j < N; j += 8) {
            /* vb = b[j..j+7] */
            __m256i vb = _mm256_loadu_si256((const __m256i *)(bc + j));
            /* vr = r[i+j..i+j+7] */
            __m256i vr = _mm256_loadu_si256((const __m256i *)(rp + j));
            /* vr += OVERFLOWING_MUL(a[i], b[j..j+7])
             * _mm256_mullo_epi32 gives low 32 bits == (uint32_t)(X*Y) */
            vr = _mm256_add_epi32(vr, _mm256_mullo_epi32(vai, vb));
            _mm256_storeu_si256((__m256i *)(rp + j), vr);
        }
    }
 
    /* ------------------------------------------------------------------ */
    /* Negacyclic fold + mod-Q reduction — AVX2 vectorised                 */
    /*                                                                     */
    /* Scalar equivalent:                                                  */
    /*   for i = N..2N-2: r[i-N] = (r[i-N] + Q - r[i]) & (Q-1)          */
    /*   for i = 0..N-1:  c[i]   = r[i]                                  */
    /*                                                                     */
    /* Fused into one pass:                                                */
    /*   c[k] = (r[k] + Q - r[k+N]) & (Q-1),  k = 0..N-2                */
    /*   c[N-1] = r[N-1] & (Q-1)                                          */
    /* ------------------------------------------------------------------ */
    __m256i vqmask = _mm256_set1_epi32((int)(Q - 1));
    __m256i vQ     = _mm256_set1_epi32((int)Q);
    uint32_t * restrict cc = c->coeffs;
 
    /* N-8 = 120: process k = 0..119 in 15 full AVX2 chunks of 8 */
    for (unsigned int k = 0; k < N - 8; k += 8) {
        __m256i vlo  = _mm256_loadu_si256((const __m256i *)(r + k));
        __m256i vhi  = _mm256_loadu_si256((const __m256i *)(r + k + N));
        /* (r[k] + Q - r[k+N]) & (Q-1) — vectorised OVERFLOWING_MUL-safe fold */
        __m256i vres = _mm256_and_si256(
                           _mm256_add_epi32(
                               _mm256_sub_epi32(vlo, vhi), vQ),
                           vqmask);
        _mm256_storeu_si256((__m256i *)(cc + k), vres);
    }
 
    /* Scalar tail: k = N-8..N-2 (7 terms with wrap), k = N-1 (no wrap) */
    for (unsigned int k = N - 8; k < N - 1; k++)
        cc[k] = (r[k] + Q - r[k + N]) & (Q - 1);
    cc[N - 1] = r[N - 1] & (Q - 1);
}
/*
static inline __attribute__((always_inline)) void schoolbook_8_wrap(
    const uint32_t * restrict a,
    const uint32_t * restrict b,
    uint32_t       * restrict res)
{
  __m256i f0, f1, f2;
  __m128i g0, g1;
  memset(res, 0, N8 * sizeof(uint32_t));

#if N8 == 8
  // res[0] = sum_{i+j=0 mod 8, sign} ...
  // Diagonal k: a[i]*b[k-i] for i=0..k  (add), then wrap with negation

  // --- res[0]: only a[0]*b[0] (no wrap terms for k=0) ---
  res[0] = OVERFLOWING_MUL(a[0], b[0]);

  // Wrap contributions to res[0]: a[i]*b[8-i] for i=1..7 => subtract
  // = -(a[1]*b[7] + a[2]*b[6] + a[3]*b[5] + a[4]*b[4] +
  //     a[5]*b[3] + a[6]*b[2] + a[7]*b[1])
  f0 = _mm256_loadu_si256((__m256i *)(a + 1));   // a[1..7] (pad 0)
  f1 = _mm256_loadu_si256((__m256i *)(b + 1));   // b[1..7] (pad 0)
  f1 = _mm256_permutevar8x32_epi32(f1,
         _mm256_setr_epi32(6, 5, 4, 3, 2, 1, 0, 7)); // reverse b[1..7]
  f2 = _mm256_mullo_epi32(f0, f1);
  g0 = _mm_add_epi32(_mm256_extracti128_si256(f2, 0),
                     _mm256_extracti128_si256(f2, 1));
  // horizontal sum of g0 (4 lanes, but lane3 is a[7]*b[pad] — we only want lanes 0..2 of upper + lower)
  // Actually we loaded 7 products; the 8th lane (index 7) is a[7+1]*b[...] which is out of bounds.
  // Use scalar for safety on the 7-term sum:
  res[0] -= OVERFLOWING_MUL(a[1], b[7]) + OVERFLOWING_MUL(a[2], b[6])
           + OVERFLOWING_MUL(a[3], b[5]) + OVERFLOWING_MUL(a[4], b[4])
           + OVERFLOWING_MUL(a[5], b[3]) + OVERFLOWING_MUL(a[6], b[2])
           + OVERFLOWING_MUL(a[7], b[1]);

  // --- res[1]: a[0]*b[1]+a[1]*b[0]  minus  a[2]*b[7]+...+a[7]*b[2] ---
  res[1]  = OVERFLOWING_MUL(a[0], b[1]) + OVERFLOWING_MUL(a[1], b[0]);
  res[1] -= OVERFLOWING_MUL(a[2], b[7]) + OVERFLOWING_MUL(a[3], b[6])
           + OVERFLOWING_MUL(a[4], b[5]) + OVERFLOWING_MUL(a[5], b[4])
           + OVERFLOWING_MUL(a[6], b[3]) + OVERFLOWING_MUL(a[7], b[2]);

  // --- res[2]: a[0]*b[2]+a[1]*b[1]+a[2]*b[0]  minus  a[3]*b[7]+...+a[7]*b[3] ---
  res[2]  = OVERFLOWING_MUL(a[0], b[2]) + OVERFLOWING_MUL(a[1], b[1])
           + OVERFLOWING_MUL(a[2], b[0]);
  res[2] -= OVERFLOWING_MUL(a[3], b[7]) + OVERFLOWING_MUL(a[4], b[6])
           + OVERFLOWING_MUL(a[5], b[5]) + OVERFLOWING_MUL(a[6], b[4])
           + OVERFLOWING_MUL(a[7], b[3]);

  // --- res[3]: 4 add terms (use SSE like original), minus 4 subtract terms ---
  // add: a[0..3] * b[3..0]
  g0 = _mm_loadu_si128((__m128i *)a);               // a[0..3]
  g1 = _mm_loadu_si128((__m128i *)b);               // b[0..3]
  g1 = _mm_shuffle_epi32(g1, _MM_SHUFFLE(0,1,2,3)); // b[3,2,1,0]
  g1 = _mm_mullo_epi32(g0, g1);
  res[3] = _mm_extract_epi32(g1, 0) + _mm_extract_epi32(g1, 1)
         + _mm_extract_epi32(g1, 2) + _mm_extract_epi32(g1, 3);
  // sub: a[4..7] * b[7..4]
  g0 = _mm_loadu_si128((__m128i *)(a + 4));          // a[4..7]
  g1 = _mm_loadu_si128((__m128i *)(b + 4));          // b[4..7]
  g1 = _mm_shuffle_epi32(g1, _MM_SHUFFLE(0,1,2,3)); // b[7,6,5,4]
  g1 = _mm_mullo_epi32(g0, g1);
  res[3] -= _mm_extract_epi32(g1, 0) + _mm_extract_epi32(g1, 1)
          + _mm_extract_epi32(g1, 2) + _mm_extract_epi32(g1, 3);

  // --- res[4]: 5 add terms, 3 subtract terms ---
  // add: a[0..4] * b[4..0]
  g0 = _mm_loadu_si128((__m128i *)a);               // a[0..3]
  g1 = _mm_loadu_si128((__m128i *)(b + 1));          // b[1..4]
  g1 = _mm_shuffle_epi32(g1, _MM_SHUFFLE(0,1,2,3)); // b[4,3,2,1]
  g1 = _mm_mullo_epi32(g0, g1);
  res[4] = OVERFLOWING_MUL(a[4], b[0])
         + _mm_extract_epi32(g1, 0) + _mm_extract_epi32(g1, 1)
         + _mm_extract_epi32(g1, 2) + _mm_extract_epi32(g1, 3);
  // sub: a[5]*b[7] + a[6]*b[6] + a[7]*b[5]  (only 3 terms — scalar is cleanest)
  res[4] -= OVERFLOWING_MUL(a[5], b[7]) + OVERFLOWING_MUL(a[6], b[6])
           + OVERFLOWING_MUL(a[7], b[5]);

  // --- res[5]: 6 add terms, 2 subtract terms ---
  g0 = _mm_loadu_si128((__m128i *)a);               // a[0..3]
  g1 = _mm_loadu_si128((__m128i *)(b + 2));          // b[2..5]
  g1 = _mm_shuffle_epi32(g1, _MM_SHUFFLE(0,1,2,3)); // b[5,4,3,2]
  g1 = _mm_mullo_epi32(g0, g1);
  res[5] = OVERFLOWING_MUL(a[5], b[0]) + OVERFLOWING_MUL(a[4], b[1])
         + _mm_extract_epi32(g1, 0) + _mm_extract_epi32(g1, 1)
         + _mm_extract_epi32(g1, 2) + _mm_extract_epi32(g1, 3);
  res[5] -= OVERFLOWING_MUL(a[6], b[7]) + OVERFLOWING_MUL(a[7], b[6]);

  // --- res[6]: 7 add terms, 1 subtract term ---
  g0 = _mm_loadu_si128((__m128i *)a);               // a[0..3]
  g1 = _mm_loadu_si128((__m128i *)(b + 3));          // b[3..6]
  g1 = _mm_shuffle_epi32(g1, _MM_SHUFFLE(0,1,2,3)); // b[6,5,4,3]
  g1 = _mm_mullo_epi32(g0, g1);
  res[6] = OVERFLOWING_MUL(a[6], b[0]) + OVERFLOWING_MUL(a[5], b[1])
         + OVERFLOWING_MUL(a[4], b[2])
         + _mm_extract_epi32(g1, 0) + _mm_extract_epi32(g1, 1)
         + _mm_extract_epi32(g1, 2) + _mm_extract_epi32(g1, 3);
  res[6] -= OVERFLOWING_MUL(a[7], b[7]);

  // --- res[7]: all 8 terms, no subtraction — full dot product a[0..7]*b[7..0] ---
  f0 = _mm256_loadu_si256((__m256i *)a);
  f1 = _mm256_loadu_si256((__m256i *)b);
  f1 = _mm256_permutevar8x32_epi32(f1,
         _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0)); // b[7..0]
  f2 = _mm256_mullo_epi32(f0, f1);
  g0 = _mm_add_epi32(_mm256_extracti128_si256(f2, 0),
                     _mm256_extracti128_si256(f2, 1));
  res[7] = _mm_extract_epi32(g0, 0) + _mm_extract_epi32(g0, 1)
         + _mm_extract_epi32(g0, 2) + _mm_extract_epi32(g0, 3);

#else
#error "Implementation assumes N8 = 8"
#endif
}

void schoolbook_8_wrap_old2(poly *c, const poly *a, const poly *b) {
  unsigned int i,j;
  uint32_t r[2*N];

  // printf("c->coeffs[0]=%d, ", c->coeffs[0]);

  for(i = 0; i < 2*N; i++)
    r[i] = 0;

  for(i = 0; i < N; i++)
    for(j = 0; j < N; j++) {
      r[i+j] = (r[i+j] + ((uint32_t)a->coeffs[i] * b->coeffs[j])) & (Q - 1);
    }

  for(i = N; i < 2*N-1; i++) {
    r[i-N] = (r[i-N] + Q - r[i]) & (Q - 1);
  }

  for(i = 0; i < N; i++)
    c->coeffs[i] = r[i];
  // printf("c->coeffs[0]=%d, ", c->coeffs[0]);
}
void schoolbook_128_wrap(uint32_t *a, uint32_t *b, uint32_t *res)
{   // --- Only for testing multiplication between two 128 degree polynomials modulo x^128+1 ---
    memset(res,0,N*sizeof(int32_t));
    uint32_t prod;

    for(unsigned int i=0;i<N;i++){

        for(unsigned int j=0;j<N;j++){

            prod =  OVERFLOWING_MUL(a[i], b[j]);

            if(i+j < N)
                res[i+j] += prod;
            else
                res[i+j-N] -= prod;
        }
    }

    for(unsigned int i=0; i<N;i++){
        res[i] = res[i] & (Q-1);
    }
}

void toom4_32(const uint32_t *a, const uint32_t *b, uint32_t *res)
{

    uint32_t aw1[N8], aw2[N8], aw3[N8], aw4[N8], aw5[N8], aw6[N8], aw7[N8];
    uint32_t bw1[N8], bw2[N8], bw3[N8], bw4[N8], bw5[N8], bw6[N8], bw7[N8];

    // ----- Striding Decomposition -----
    uint32_t w1[N8]={0}, w2[N8]={0}, w3[N8]={0},
             w4[N8]={0}, w5[N8]={0}, w6[N8]={0}, w7[N8]={0};

    memset(res, 0, N32*sizeof(uint32_t)); // striding outputs 32 degree polynomial as the product of two 32-degree polynomials

    uint32_t A0[N8], A1[N8], A2[N8], A3[N8];
    uint32_t B0[N8], B1[N8], B2[N8], B3[N8];

    uint32_t r0, r1, r2, r3, r4, r5, r6;

    for(int j=0;j<N8;j++){

        A0[j] = a[4*j];
        A1[j] = a[4*j + 1];
        A2[j] = a[4*j + 2];
        A3[j] = a[4*j + 3];

        B0[j] = b[4*j];
        B1[j] = b[4*j + 1];
        B2[j] = b[4*j + 2];
        B3[j] = b[4*j + 3];
    }
    // -----------------------------------

    // ---------- Evaluation -------------
    for (int j=0;j<N8;j++){
        r0=A0[j];
        r1=A1[j];
        r2=A2[j];
        r3=A3[j];
        uint32_t t0=r0+r2, t1=r1+r3;

        aw3[j]=t0+t1;
        aw4[j]=t0-t1;

        t0=((r0<<2)+r2)<<1;
        t1=(r1<<2)+r3;

        aw5[j]=t0+t1;
        aw6[j]=t0-t1;

        aw2[j]=(r3<<3)+(r2<<2)+(r1<<1)+r0;
        aw7[j]=r0;
        aw1[j]=r3;
    }

    for (int j=0;j<N8;j++){
        r0=B0[j];
        r1=B1[j];
        r2=B2[j];
        r3=B3[j];

        uint32_t t0=r0+r2, t1=r1+r3;

        bw3[j]=t0+t1;
        bw4[j]=t0-t1;

        t0=((r0<<2)+r2)<<1;
        t1=(r1<<2)+r3;

        bw5[j]=t0+t1;
        bw6[j]=t0-t1;

        bw2[j]=(r3<<3)+(r2<<2)+(r1<<1)+r0;
        bw7[j]=r0;
        bw1[j]=r3;
    }

    // ---- Multiplication (BASE = schoolbook_8_wrap) ----
    schoolbook_8_wrap(aw1, bw1, w1);
    schoolbook_8_wrap(aw2, bw2, w2);
    schoolbook_8_wrap(aw3, bw3, w3);
    schoolbook_8_wrap(aw4, bw4, w4);
    schoolbook_8_wrap(aw5, bw5, w5);
    schoolbook_8_wrap(aw6, bw6, w6);
    schoolbook_8_wrap(aw7, bw7, w7);


    // ---- Interpolation ----
    uint32_t inv3 = 2863311531, inv9 = 954437177, inv15 = 4008636143;//Inverse with respect to 2^32

    uint32_t prev0, prev1, prev2; // for striding

    for(int i=0;i<N8;i++){  // striding needs N8 and NOT 2*N8 - 1 interpolations

        prev0 = r0;
        prev1 = r1;
        prev2 = r2;

        r0=w1[i];
        r1=w2[i];
        r2=w3[i];
        r3=w4[i];
        r4=w5[i];
        r5=w6[i];
        r6=w7[i];

        r1+=r4;
        r5-=r4;
        r3=(r3-r2)>>1;
        r4=r4-r0-(r6<<6);
        r4=(r4<<1)+r5;
        r2+=r3;
        r1=r1-(r2<<6)-r2;
        r2=r2-r6-r0;
        r1+=45*r2;
        r4=((r4-(r2<<3))*inv3)>>3;
        r5+=r1;
        r1=((r1+(r3<<4))*inv9)>>1;
        r3=-(r3+r1);
        r5=((30*r1-r5)*inv15)>>2;
        r2-=r4;
        r1-=r5;

        // ----- Striding -----
        res[4*i + 3] = r3;

        if(i==0){
            res[4*i + 0] = r6;
            res[4*i + 1] = r5;
            res[4*i + 2] = r4;
        }
        else{
            res[4*i + 0] = r6 + prev2;
            res[4*i + 1] = r5 + prev1;
            res[4*i + 2] = r4 + prev0;
        }
        // --------------------
    }

    res[0] -= r2;
    res[1] -= r1;
    res[2] -= r0;
}

void toom_cook_4way (const uint32_t *a1, const uint32_t *b1, uint32_t *result) {
  uint32_t inv3 = 2863311531, inv9 = 954437177, inv15 = 4008636143;//Inverse with respect to 2^32

    uint32_t aw1[N32], aw2[N32], aw3[N32], aw4[N32], aw5[N32], aw6[N32], aw7[N32];
    uint32_t bw1[N32], bw2[N32], bw3[N32], bw4[N32], bw5[N32], bw6[N32], bw7[N32];

    // ----- Striding Implementation -----
    uint32_t w1[N32] = {0}, w2[N32] = {0}, w3[N32] = {0}, w4[N32] = {0},
                            w5[N32] = {0}, w6[N32] = {0}, w7[N32] = {0};

    uint32_t r0, r1, r2, r3, r4, r5, r6, r7;

    uint32_t A0[N32], A1[N32], A2[N32], A3[N32];
    uint32_t B0[N32], B1[N32], B2[N32], B3[N32];

    for(int j = 0; j < N32; j++) {

        A0[j] = a1[4*j];
        A1[j] = a1[4*j + 1];
        A2[j] = a1[4*j + 2];
        A3[j] = a1[4*j + 3];

        B0[j] = b1[4*j];
        B1[j] = b1[4*j + 1];
        B2[j] = b1[4*j + 2];
        B3[j] = b1[4*j + 3];
    }

    uint32_t *C;
    C = result;

    unsigned int j;

    // EVALUATION
    for (j = 0; j < N32; ++j) {
        r0 = A0[j];
        r1 = A1[j];
        r2 = A2[j];
        r3 = A3[j];
        r4 = r0 + r2;
        r5 = r1 + r3;
        r6 = r4 + r5;
        r7 = r4 - r5;
        aw3[j] = r6;
        aw4[j] = r7;
        r4 = ((r0 << 2) + r2) << 1;
        r5 = (r1 << 2) + r3;
        r6 = r4 + r5;
        r7 = r4 - r5;
        aw5[j] = r6;
        aw6[j] = r7;
        r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
        aw2[j] = r4;
        aw7[j] = r0;
        aw1[j] = r3;
    }
    for (j = 0; j < N32; ++j) {
        r0 = B0[j];
        r1 = B1[j];
        r2 = B2[j];
        r3 = B3[j];
        r4 = r0 + r2;
        r5 = r1 + r3;
        r6 = r4 + r5;
        r7 = r4 - r5;
        bw3[j] = r6;
        bw4[j] = r7;
        r4 = ((r0 << 2) + r2) << 1;
        r5 = (r1 << 2) + r3;
        r6 = r4 + r5;
        r7 = r4 - r5;
        bw5[j] = r6;
        bw6[j] = r7;
        r4 = (r3 << 3) + (r2 << 2) + (r1 << 1) + r0;
        bw2[j] = r4;
        bw7[j] = r0;
        bw1[j] = r3;
    }

  // MULTIPLICATION
    toom4_32(aw1, bw1, w1);
    toom4_32(aw2, bw2, w2);
    toom4_32(aw3, bw3, w3);
    toom4_32(aw4, bw4, w4);
    toom4_32(aw5, bw5, w5);
    toom4_32(aw6, bw6, w6);
    toom4_32(aw7, bw7, w7);


    // INTERPOLATION

    uint32_t prev0, prev1, prev2;

    for(int i=0;i<N32;i++){  // striding needs N32 and NOT 2*N32 - 1 interpolations

        prev0 = r0;
        prev1 = r1;
        prev2 = r2;

        r0 = w1[i];
        r1 = w2[i];
        r2 = w3[i];
        r3 = w4[i];
        r4 = w5[i];
        r5 = w6[i];
        r6 = w7[i];

        r1 = r1 + r4;
        r5 = r5 - r4;
        r3 = ((r3 - r2) >> 1);
        r4 = r4 - r0;
        r4 = r4 - (r6 << 6);
        r4 = (r4 << 1) + r5;
        r2 = r2 + r3;
        r1 = r1 - (r2 << 6) - r2;
        r2 = r2 - r6;
        r2 = r2 - r0;
        r1 = r1 + 45 * r2;
        r4 = (uint32_t)(((r4 - (r2 << 3)) * (uint32_t)inv3) >> 3);
        r5 = r5 + r1;
        r1 = (uint32_t)(((r1 + (r3 << 4)) * (uint32_t)inv9) >> 1);
        r3 = -(r3 + r1);
        r5 = (uint32_t)(((30 * r1 - r5) * (uint32_t)inv15) >> 2);
        r2 = r2 - r4;
        r1 = r1 - r5;

        // ----- striding ------
        C[4*i + 3] = r3;

        if(i == 0){

            C[4*i + 0] = r6;
            C[4*i + 1] = r5;
            C[4*i + 2] = r4;

        }
        else{

            C[4*i + 0] = r6 + prev2;
            C[4*i + 1] = r5 + prev1;
            C[4*i + 2] = r4 + prev0;

        }
        // --------------------

    }

        C[0] -= r2;
        C[1] -= r1;
        C[2] -= r0;
}

void pol_mul(poly *c, const poly *a, const poly *b) {

  uint32_t tmp[2 * N] = {0};
  unsigned int i;

  toom_cook_4way(a->coeffs, b->coeffs, tmp);

  for (i = 0; i < N; i++) {
    c->coeffs[i] = (tmp[i] + Q - tmp[i + N]) & (Q - 1);
  }
}*/

void pol_mul(poly *c, const poly *a, const poly *b) {

  //uint32_t tmp[2 * N] = {0};
  //unsigned int i;

  //toom_cook_4way(a->coeffs, b->coeffs, tmp);
  schoolbook_8_wrap(c, a, b);

  //for (i = 0; i < N; i++) {
  //  c->coeffs[i] = (tmp[i] + Q - tmp[i + N]) & (Q - 1);
 // }
}

/*************************************************
 * Name:        poly_freeze
 *
 * Description: Reduce all coefficients of polynomial to standard
 *              representatives. In-place.
 *
 * Arguments:   - poly *a: pointer to input/output polynomial
 **************************************************/
void poly_freeze_avx2(poly *a, int mod) {
  uint32_t mask = mod - 1;
  unsigned int i = 0;

  if (N >= 8)
  {
    __m256i vec_mask = _mm256_set1_epi32(mask);

    for(i = 0; i < N/8; i++)
    {
      __m256i vec = _mm256_loadu_si256(a->vec+i);
      vec = _mm256_and_si256(vec, vec_mask);
      _mm256_storeu_si256(a->vec+i, vec);
    }
  }

  for(; i < N; i++)
  {
    a->coeffs[i] &= mask;
  }
}

/*************************************************
 * Name:        poly_add
 *
 * Description: Add polynomials. No modular reduction is performed.
 *
 * Arguments:   - poly *c: pointer to output polynomial
 *              - poly *a: pointer to first summand
 *              - poly *b: pointer to second summand
 **************************************************/
void poly_add(poly *c, const poly *a, const poly *b)  {
  unsigned int i;

  for(i = 0; i < N/8; ++i)
    _mm256_store_si256(c->vec+i, _mm256_add_epi32(a->vec[i], b->vec[i]));

}

/*************************************************
 * Name:        poly_sub
 *
 * Description: Subtract polynomials. Assumes coefficients of input polynomials
 *              to be less than 2*Q. No modular reduction is performed.
 *
 * Arguments:   - poly *c: pointer to output polynomial
 *              - poly *a: pointer to first input polynomial
 *              - poly *b: pointer to second input polynomial to be subtraced
 *                         from first input polynomial
 **************************************************/
void poly_sub(poly *c, const poly *a, const poly *b) {
  unsigned int i;

  for(i = 0; i < N/8; ++i)
    _mm256_store_si256(c->vec+i, _mm256_sub_epi32(a->vec[i], b->vec[i]));
}

/*************************************************
 * Name:        poly_neg
 *
 * Description: Negate polynomial in-place.
 *              Assumes input coefficients to be less than 2*Q.
 *
 * Arguments:   - poly *a: pointer to input/output polynomial
 **************************************************/
void poly_neg(poly *a) {
  unsigned int i;

  for(i = 0; i < N; ++i)
    a->coeffs[i] = 2*Q - a->coeffs[i];
}

/*************************************************
 * Name:        poly_shiftl
 *
 * Description: Multiply polynomial by 2^k, in-place
 *
 * Arguments:   - poly *a: pointer to input/output polynomial
 *              - unsigned int k: exponent
 **************************************************/
void poly_shiftl(poly *a, unsigned int k) {
  unsigned int i;

  for(i = 0; i < N/8; ++i)
    _mm256_store_si256(a->vec+i, _mm256_slli_epi32(a->vec[i], k));
}

/*************************************************
 * Name:        poly_chknorm
 *
 * Description: Check infinity norm of polynomial against given bound.
 *              Assumes input coefficients to be standard representatives.
 *
 * Arguments:   - const poly *a: pointer to polynomial
 *              - uint32_t B: norm bound
 *
 * Returns 0 if norm is strictly smaller than B and 1 otherwise.
 **************************************************/
int poly_chknorm(const poly *a, uint32_t B, int mod) {
  unsigned int i;
  int32_t t;

  /* It is ok to leak which coefficient violates the bound since
     the probability for each coefficient is independent of secret
     data but we must not leak the sign of the centralized representative. */
  for(i = 0; i < N; ++i) {
    /* Absolute value of centralized representative */
    t = mod/2 - a->coeffs[i];
    t ^= (t >> 31);
    t = mod/2 - t;
    t = t & (mod-1);

    if((uint32_t)t >= B) {
      return 1;
    }
  }

  return 0;
}

/*************************************************
 * Name:        poly_uniform
 *
 * Description: Sample uniformly random polynomial using stream of random bytes.
 *              Assumes that enough random bytes are given (e.g.
 *              5*XOF_168 bytes).
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - unsigned char *buf: array of random bytes
 **************************************************/
void poly_uniform(poly *a, unsigned char *buf) {
  unsigned int ctr, pos;
  uint32_t t;

  ctr = pos = 0;
  while(ctr < N) {
    t  = buf[pos++];
    t |= (uint32_t)buf[pos++] << 8;
    t |= (uint32_t)buf[pos++] << 16;
    t &= 0x7FFFFF;

    if(t < Q)
      a->coeffs[ctr++] = t;
  }
}

/*************************************************
 * Name:        rej_eta
 *
 * Description: Sample uniformly random coefficients in [-ETA, ETA] by
 *              performing rejection sampling using array of random bytes
 *
 * Arguments:   - uint32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const unsigned char *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len if not enough
 * random bytes were given.
 **************************************************/
static unsigned int rej_eta(uint32_t *a,
    unsigned int len,
    const unsigned char *buf,
    unsigned int buflen)
{
//#if ETA > 7
//#error "rej_eta() assumes ETA <= 7"
//#endif
  unsigned int ctr, pos;
  unsigned char t0, t1;

  ctr = pos = 0;
  while(ctr < len) {
#if ETA <= 3
    t0 = buf[pos] & 0x07;
    t1 = buf[pos++] >> 5;
#else
    t0 = buf[pos] & 0x0F;
    t1 = buf[pos++] >> 4;
#endif

    if(t0 <= 2*ETA)
      a[ctr++] = Q + ETA - t0;
    if(t1 <= 2*ETA && ctr < N)
      a[ctr++] = Q + ETA - t1;

    if(pos >= buflen)
      break;
  }

  return ctr;
}

/*************************************************
 * Name:        poly_uniform_eta
 *
 * Description: Sample polynomial with uniformly random coefficients
 *              in [-ETA,ETA] by performing rejection sampling using the
 *              output stream from XOF_136(seed|nonce)
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const unsigned char seed[]: byte array with seed of length
 *                                            SEEDBYTES
 *              - unsigned char nonce: nonce byte
 **************************************************/
void poly_uniform_eta(poly *a,
    const unsigned char seed[SEEDBYTES],
    unsigned char nonce)
{
  unsigned int i, ctr = 0;
  unsigned char inbuf[SEEDBYTES + 2];  // +1 for nonce, +1 for counter
  unsigned char outbuf[XOF_136];
  unsigned char counter = 0;
  unsigned int pos = 0;

  /* Prepare fixed part: seed || nonce */
  for(i = 0; i < SEEDBYTES; ++i)
    inbuf[i] = seed[i];
  inbuf[SEEDBYTES] = nonce;

  while(ctr < N) {
    /* Add counter to avoid repeating output */
    inbuf[SEEDBYTES + 1] = counter++;

    /* Generate fresh pseudorandom block */
    pseudoXOF((unsigned long long)(XOF_136 * 8),
        inbuf,
        (SEEDBYTES + 2) * 8,
        outbuf);

    /* Rejection sampling */
    pos = rej_eta(a->coeffs + ctr,
        N - ctr,
        outbuf,
        XOF_136);

    ctr += pos;
  }
}

/*************************************************
 * Name:        rej_gamma1m1
 *
 * Description: Sample uniformly random coefficients
 *              in [-(GAMMA1 - 1), GAMMA1 - 1] by performing rejection sampling
 *              using array of random bytes
 *
 * Arguments:   - uint32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const unsigned char *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len if not enough
 * random bytes were given.
 **************************************************/
static unsigned int rej_gamma1m1(uint32_t *a,
    unsigned int len,
    const unsigned char *buf,
    unsigned int buflen)
{
#if GAMMA1 > (1 << 19)
#error "rej_gamma1m1() assumes GAMMA1 - 1 fits in 19 bits"
#endif
  unsigned int ctr, pos;
  uint32_t t;

  ctr = pos = 0;
  while(ctr < len) {
    t  = buf[pos];
    t |= (uint32_t)buf[pos + 1] << 8;
    t |= (uint32_t)buf[pos + 2] << 16;
    t &= 0xFFFFF;

    t  = buf[pos + 2] >> 4;
    t |= (uint32_t)buf[pos + 3] << 4;
    t |= (uint32_t)buf[pos + 4] << 12;

    pos += 5;

    if(t <= 2*GAMMA1 - 2)
      a[ctr++] = Q + GAMMA1 - 1 - t;
    if(t <= 2*GAMMA1 - 2 && ctr < len)
      a[ctr++] = Q + GAMMA1 - 1 - t;

    if(pos > buflen - 5)
      break;
  }

  return ctr;
}

/*************************************************
 * Name:        poly_uniform_gamma1m1
 *
 * Description: Sample polynomial with uniformly random coefficients
 *              in [-(GAMMA1 - 1), GAMMA1 - 1] by performing rejection
 *              sampling on output stream of XOF_168(seed|nonce)
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const unsigned char seed[]: byte array with seed of length
 *                                            SEEDBYTES + CRHBYTES
 *              - uint16_t nonce: 16-bit nonce
 **************************************************/
void poly_uniform_gamma1m1(poly *a,
    const unsigned char seed[SEEDBYTES + CRHBYTES],
    uint16_t nonce)
{
  unsigned int i, ctr = 0;
  unsigned char inbuf[SEEDBYTES + CRHBYTES + 2];

  /* Conservative buffer size */
#define MAX_BLOCKS 6
#define OUTBUF_SIZE (MAX_BLOCKS * XOF_136)

  unsigned char outbuf[OUTBUF_SIZE];

  /* Prepare static part of input */
  for(i = 0; i < SEEDBYTES + CRHBYTES; ++i)
    inbuf[i] = seed[i];

  while(ctr < N) {
    /* Domain separation using nonce */
    inbuf[SEEDBYTES + CRHBYTES]     = nonce & 0xFF;
    inbuf[SEEDBYTES + CRHBYTES + 1] = nonce >> 8;

    /* Generate pseudorandom bytes */
    if(pseudoXOF(OUTBUF_SIZE * 8,
          inbuf,
          (SEEDBYTES + CRHBYTES + 2) * 8,
          outbuf) != 0) {
      /* Handle error */
      return;
    }

    /* Rejection sampling */
    ctr += rej_gamma1m1(a->coeffs + ctr,
        N - ctr,
        outbuf,
        OUTBUF_SIZE);

    nonce++;  /* Move to next domain */
  }
}


/*************************************************
 * Name:        polyeta_pack(Changed)
 *
 * Description: Bit-pack polynomial with coefficients in [-ETA,ETA].
 *              Input coefficients are assumed to be standard representatives.
 *
 * Arguments:   - unsigned char *r: pointer to output byte array with at least
 *                                  POLETA_SIZE_PACKED bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/

void polyeta_pack(unsigned char *r, const poly *a) {
  unsigned int i;
  unsigned char t[4];

  for(i = 0; i < N/4; ++i) {
    t[0] = Q + ETA - a->coeffs[4*i+0];
    t[1] = Q + ETA - a->coeffs[4*i+1];
    t[2] = Q + ETA - a->coeffs[4*i+2];
    t[3] = Q + ETA - a->coeffs[4*i+3];

    r[3*i+0] = (t[0] & 0x3F) | ((t[1] & 0x03) << 6);
    r[3*i+1] = ((t[1]>>2) & 0x0F) | ((t[2] & 0x0F) << 4);
    r[3*i+2] = ((t[2]>>4) & 0x03) | ((t[3] & 0x3F) << 2);

  }


}


/*************************************************
 * Name:        polyeta_unpack(Changed)
 *
 * Description: Unpack polynomial with coefficients in [-ETA,ETA].
 *              Output coefficients are not standard representatives but
 *              no greater than Q + ETA.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const unsigned char *a: byte array with bit-packed polynomial
 **************************************************/

void polyeta_unpack(poly *r, const unsigned char *a) {
  unsigned int i;

  for(i = 0; i < N/4; ++i) {
    r->coeffs[4*i+0] = (a[3*i+0] & 0x3F);
    r->coeffs[4*i+1] = (((a[3*i+0]>>6) & 0x03) | ((a[3*i+1] & 0x0F) << 2));
    r->coeffs[4*i+2] = (((a[3*i+1]>>4) & 0x0F) | ((a[3*i+2] & 0x03) << 4));
    r->coeffs[4*i+3] = ((a[3*i+2]>>2) & 0x3F);

    r->coeffs[4*i+0] = Q + ETA - r->coeffs[4*i+0];
    r->coeffs[4*i+1] = Q + ETA - r->coeffs[4*i+1];
    r->coeffs[4*i+2] = Q + ETA - r->coeffs[4*i+2];
    r->coeffs[4*i+3] = Q + ETA - r->coeffs[4*i+3];
  }
}

/*************************************************
 * Name:        polyt1_pack(Checked)
 *
 * Description: Bit-pack polynomial t1 with coefficients fitting in 9 bits.
 *              Input coefficients are assumed to be standard representatives.
 *
 * Arguments:   - unsigned char *r: pointer to output byte array with at least
 *                                  POLT1_SIZE_PACKED bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void polyt1_pack(unsigned char *r, const poly *a) {
  unsigned int i, j;

  for(i = 0, j = 0; i < N; i += 4, j += 5) {
    r[j] = a->coeffs[i] & 0xff;
    r[j+1] = (a->coeffs[i] >> 8) | ((a->coeffs[i+1] & 0x3f) << 2);
    r[j+2] = (a->coeffs[i+1] >> 6) | ((a->coeffs[i+2] & 0xf) << 4);
    r[j+3] = (a->coeffs[i+2] >> 4) | ((a->coeffs[i+3] & 0x3) << 6);
    r[j+4] = a->coeffs[i+3] >> 2;
  }
}

/*************************************************
 * Name:        polyt1_unpack(Checked)
 *
 * Description: Unpack polynomial t1 with 9-bit coefficients.
 *              Output coefficients are not standard representatives.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const unsigned char *a: byte array with bit-packed polynomial
 **************************************************/
void polyt1_unpack(poly *r, const unsigned char *a) {
  unsigned int i, j;

  for(i = 0, j = 0; i < N; i += 4, j += 5) {
    r->coeffs[i] = a[j] | ((uint32_t)(a[j+1] & 0x3) << 8);
    r->coeffs[i+1] = (a[j+1] >> 2) | ((uint32_t)(a[j+2] & 0xf) << 6);
    r->coeffs[i+2] = (a[j+2] >> 4) | ((uint32_t)(a[j+3] & 0x3f) << 4);
    r->coeffs[i+3] = (a[j+3] >> 6) | ((uint32_t)a[j+4] << 2);
  }
}

/*************************************************
 * Name:        polyt0_pack(Changed)
 *
 * Description: Bit-pack polynomial t0 with coefficients in ]-2^{D/2}, 2^{D/2}].
 *              Input coefficients are assumed to be standard representatives.
 *
 * Arguments:   - unsigned char *r: pointer to output byte array with at least
 *                                  POLT0_SIZE_PACKED bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/

/*Range of t_0--> -2^9 to 2^9 --> 10bits

Inputs:  8 * 10(bits)
Outputs: 10 * 8(1 Byte)
*/
void polyt0_pack(unsigned char *r, const poly *a) {
  unsigned int i;
  uint32_t t[8];

  for(i = 0; i < N/8; ++i) {
    t[0] = Q + (1 << (D-1)) - a->coeffs[8*i+0];
    t[1] = Q + (1 << (D-1)) - a->coeffs[8*i+1];
    t[2] = Q + (1 << (D-1)) - a->coeffs[8*i+2];
    t[3] = Q + (1 << (D-1)) - a->coeffs[8*i+3];
    t[4] = Q + (1 << (D-1)) - a->coeffs[8*i+4];
    t[5] = Q + (1 << (D-1)) - a->coeffs[8*i+5];
    t[6] = Q + (1 << (D-1)) - a->coeffs[8*i+6];
    t[7] = Q + (1 << (D-1)) - a->coeffs[8*i+7];

    r[10*i+0] = (t[0] & 0xFF);
    r[10*i+1] = ((t[0] & 0x300) >> 8)  | ((t[1] & 0x3F)<<2);

    r[10*i+2] = ((t[1] & 0x3C0) >> 6) | ((t[2] & 0x00F) << 4);
    r[10*i+3] = ((t[2]>>4) & 0x3F) | ((t[3] & 0x3)<<6);

    r[10*i+4] = ((t[3] >> 2) & 0xFF) ;
    r[10*i+5] = (t[4] & 0xFF);

    r[10*i+6] = ((t[4] & 0x300) >> 8)  | ((t[5] & 0x03F) << 2);
    r[10*i+7] = ((t[5] & 0x3C0) >> 6) | ((t[6] & 0x00F) << 4);
    r[10*i+8] = (((t[6]>>4) & 0x3F) | ((t[7] & 0x3) << 6));

    r[10*i+9] = ((t[7]>>2)& 0xFF)  ;
  }
}

/*************************************************
 * Name:        polyt0_unpack(Changed)
 *
 * Description: Unpack polynomial t0 with coefficients in ]-2^{D/2}, 2^{D/2}].
 *              Output coefficients are not standard representatives but at most
 *              Q + 2^{D/2}.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const unsigned char *a: byte array with bit-packed polynomial
 **************************************************/

void polyt0_unpack(poly *r, const unsigned char *a) {
  unsigned int i;

  for(i = 0; i < N/8; ++i) {

    r->coeffs[8*i+0] = ((a[10*i+0]) | ((a[10*i+1] & 0x3) << 8) );
    r->coeffs[8*i+1] = (((a[10*i+1] >>2) & 0x3F) | ((a[10*i+2] & 0x0F) << 6) );

    r->coeffs[8*i+2] = (((a[10*i+2] >> 4) & 0xF) | ((a[10*i+3] & 0x3F) <<4) );

    r->coeffs[8*i+3] = (((a[10*i+3]>>6) & 0x3) | ((a[10*i+4] & 0xFF) <<2));
    r->coeffs[8*i+4] = ((a[10*i+5]) | ((a[10*i+6] & 0x3) << 8));
    r->coeffs[8*i+5] = (((a[10*i+6] >>2) & 0x3F) | ((a[10*i+7] & 0xF) << 6));
    r->coeffs[8*i+6] = (((a[10*i+7] >> 4) & 0xF) | ((a[10*i+8] & 0x3F) <<4) );
    r->coeffs[8*i+7] = (((a[10*i+8]>>6) & 0x3) | ((a[10*i+9] & 0xFF) <<2));

    r->coeffs[8*i+0] = Q + (1 << (D-1)) - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = Q + (1 << (D-1)) - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = Q + (1 << (D-1)) - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = Q + (1 << (D-1)) - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = Q + (1 << (D-1)) - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = Q + (1 << (D-1)) - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = Q + (1 << (D-1)) - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = Q + (1 << (D-1)) - r->coeffs[8*i+7];

  }
}


/*************************************************
 * Name:        polyz_pack(Checked)
 *
 * Description: Bit-pack polynomial z with coefficients
 *              in [-(GAMMA1 - 1), GAMMA1 - 1].
 *              Input coefficients are assumed to be standard representatives.
 *
 * Arguments:   - unsigned char *r: pointer to output byte array with at least
 *                                  POLZ_SIZE_PACKED bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void polyz_pack(unsigned char *r, const poly *a) {
#if GAMMA1 > (1 << 19)
#error "polyz_pack() assumes GAMMA1 <= 2^{19}"
#endif
  unsigned int i;
  uint32_t t[2];

  for(i = 0; i < N/2; ++i) {
    /* Map to {0,...,2*GAMMA1 - 2} */
    t[0] = GAMMA1 - 1 - a->coeffs[2*i+0];
    t[0] += ((int32_t)t[0] >> 31) & Q;
    t[1] = GAMMA1 - 1 - a->coeffs[2*i+1];
    t[1] += ((int32_t)t[1] >> 31) & Q;

    r[5*i+0]  = t[0];
    r[5*i+1]  = t[0] >> 8;
    r[5*i+2]  = t[0] >> 16;
    r[5*i+2] |= t[1] << 4;
    r[5*i+3]  = t[1] >> 4;
    r[5*i+4]  = t[1] >> 12;
  }
}

/*************************************************
 * Name:        polyz_unpack(Checked)
 *
 * Description: Unpack polynomial z with coefficients
 *              in [-(GAMMA1 - 1), GAMMA1 - 1].
 *              Output coefficients are not standard representatives but at
 *              most Q + GAMMA1 - 1.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const unsigned char *a: byte array with bit-packed polynomial
 **************************************************/
void polyz_unpack(poly *r, const unsigned char *a) {
  unsigned int i;

  for(i = 0; i < N/2; ++i) {
    r->coeffs[2*i+0]  = a[5*i+0];
    r->coeffs[2*i+0] |= (uint32_t)a[5*i+1] << 8;
    r->coeffs[2*i+0] |= (uint32_t)(a[5*i+2] & 0x0F) << 16;

    r->coeffs[2*i+1]  = a[5*i+2] >> 4;
    r->coeffs[2*i+1] |= (uint32_t)a[5*i+3] << 4;
    r->coeffs[2*i+1] |= (uint32_t)a[5*i+4] << 12;

    r->coeffs[2*i+0] = GAMMA1 - 1 - r->coeffs[2*i+0];
    r->coeffs[2*i+0] += ((int32_t)r->coeffs[2*i+0] >> 31) & Q;
    r->coeffs[2*i+1] = GAMMA1 - 1 - r->coeffs[2*i+1];
    r->coeffs[2*i+1] += ((int32_t)r->coeffs[2*i+1] >> 31) & Q;
  }
}

/*************************************************
 * Name:        polyw1_pack(Checked)
 *
 * Description: Bit-pack polynomial w1 with coefficients in [0, 15].
 *              Input coefficients are assumed to be standard representatives.
 *
 * Arguments:   - unsigned char *r: pointer to output byte array with at least
 *                                  POLW1_SIZE_PACKED bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void polyw1_pack(unsigned char *r, const poly *a) {
  unsigned int i;

  for(i = 0; i < N/2; ++i)
    r[i] = a->coeffs[2*i+0] | (a->coeffs[2*i+1] << 4);
}
