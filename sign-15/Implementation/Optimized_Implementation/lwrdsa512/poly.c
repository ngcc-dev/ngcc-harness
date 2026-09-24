#include <string.h>
#include <stdint.h>
#include "params.h"
#include "poly.h"

#include "auxfunc.h"

#include <immintrin.h>

#define OVERFLOWING_MUL64(X, Y) ((int64_t)((int64_t)(X) * (int64_t)(Y)))
/*
 * Emulate _mm256_mullo_epi64: low 64 bits of each 64x64-bit product.
 * Valid when AVX-512DQ is not available.
 */
static inline __m256i mullo_epi64(__m256i a, __m256i b)
{
    /* a_lo * b_lo (unsigned 32x32->64, even lanes) */
    __m256i lo_lo = _mm256_mul_epu32(a, b);

    /* a_lo * b_hi */
    __m256i b_hi  = _mm256_srli_epi64(b, 32);
    __m256i lo_hi = _mm256_mul_epu32(a, b_hi);

    /* a_hi * b_lo */
    __m256i a_hi  = _mm256_srli_epi64(a, 32);
    __m256i hi_lo = _mm256_mul_epu32(a_hi, b);

    /* cross terms shifted up by 32 */
    __m256i cross = _mm256_add_epi64(lo_hi, hi_lo);
    cross         = _mm256_slli_epi64(cross, 32);

    return _mm256_add_epi64(lo_lo, cross);
}

/*
 * Horizontal sum of a 256-bit register holding 4 × uint64_t lanes.
 */
static inline uint64_t hsum_epi64(__m256i v)
{
    __m128i lo = _mm256_castsi256_si128(v);
    __m128i hi = _mm256_extracti128_si256(v, 1);
    __m128i s  = _mm_add_epi64(lo, hi);
    /* s = [s0+s2, s1+s3]; add the two 64-bit halves */
    __m128i s1 = _mm_unpackhi_epi64(s, s);   /* [s1+s3, s1+s3] */
    __m128i r  = _mm_add_epi64(s, s1);
    return (uint64_t)_mm_cvtsi128_si64(r);
}

static inline __attribute__((always_inline)) void schoolbook_8_wrap(
    const uint64_t * restrict a,
    const uint64_t * restrict b,
    uint64_t       * restrict res)
{
    __m256i f0, f1, f2;

    memset(res, 0, N8 * sizeof(uint64_t));

#if N8 == 8

    /* ------------------------------------------------------------------ */
    /* res[0] = a[0]*b[0]
     *        - (a[1]*b[7] + a[2]*b[6] + a[3]*b[5] + a[4]*b[4]
     *           + a[5]*b[3] + a[6]*b[2] + a[7]*b[1])
     * ------------------------------------------------------------------ */
    res[0] = OVERFLOWING_MUL64(a[0], b[0]);
    res[0] -= OVERFLOWING_MUL64(a[1], b[7]) + OVERFLOWING_MUL64(a[2], b[6])
            + OVERFLOWING_MUL64(a[3], b[5]) + OVERFLOWING_MUL64(a[4], b[4])
            + OVERFLOWING_MUL64(a[5], b[3]) + OVERFLOWING_MUL64(a[6], b[2])
            + OVERFLOWING_MUL64(a[7], b[1]);

    /* ------------------------------------------------------------------ */
    /* res[1] = a[0]*b[1] + a[1]*b[0]
     *        - (a[2]*b[7] + a[3]*b[6] + a[4]*b[5] + a[5]*b[4]
     *           + a[6]*b[3] + a[7]*b[2])
     * ------------------------------------------------------------------ */
    res[1]  = OVERFLOWING_MUL64(a[0], b[1]) + OVERFLOWING_MUL64(a[1], b[0]);
    res[1] -= OVERFLOWING_MUL64(a[2], b[7]) + OVERFLOWING_MUL64(a[3], b[6])
            + OVERFLOWING_MUL64(a[4], b[5]) + OVERFLOWING_MUL64(a[5], b[4])
            + OVERFLOWING_MUL64(a[6], b[3]) + OVERFLOWING_MUL64(a[7], b[2]);

    /* ------------------------------------------------------------------ */
    /* res[2] = a[0]*b[2] + a[1]*b[1] + a[2]*b[0]
     *        - (a[3]*b[7] + a[4]*b[6] + a[5]*b[5] + a[6]*b[4] + a[7]*b[3])
     * ------------------------------------------------------------------ */
    res[2]  = OVERFLOWING_MUL64(a[0], b[2]) + OVERFLOWING_MUL64(a[1], b[1])
            + OVERFLOWING_MUL64(a[2], b[0]);
    res[2] -= OVERFLOWING_MUL64(a[3], b[7]) + OVERFLOWING_MUL64(a[4], b[6])
            + OVERFLOWING_MUL64(a[5], b[5]) + OVERFLOWING_MUL64(a[6], b[4])
            + OVERFLOWING_MUL64(a[7], b[3]);

    /* ------------------------------------------------------------------ */
    /* res[3] = a[0]*b[3] + a[1]*b[2] + a[2]*b[1] + a[3]*b[0]
     *        - (a[4]*b[7] + a[5]*b[6] + a[6]*b[5] + a[7]*b[4])
     *
     * add part: a[0..3] · b[3..0]  — 4 lanes fit exactly in one AVX2 reg
     * sub part: a[4..7] · b[7..4]  — 4 lanes fit exactly in one AVX2 reg
     * ------------------------------------------------------------------ */
    f0  = _mm256_loadu_si256((__m256i *)a);              /* a[0..3] */
    f1  = _mm256_loadu_si256((__m256i *)b);              /* b[0..3] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[3,2,1,0] */
    f2  = mullo_epi64(f0, f1);
    res[3] = hsum_epi64(f2);

    f0  = _mm256_loadu_si256((__m256i *)(a + 4));        /* a[4..7] */
    f1  = _mm256_loadu_si256((__m256i *)(b + 4));        /* b[4..7] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[7,6,5,4] */
    f2  = mullo_epi64(f0, f1);
    res[3] -= hsum_epi64(f2);

    /* ------------------------------------------------------------------ */
    /* res[4] = a[0]*b[4] + a[1]*b[3] + a[2]*b[2] + a[3]*b[1] + a[4]*b[0]
     *        - (a[5]*b[7] + a[6]*b[6] + a[7]*b[5])
     *
     * add: a[0..3] · b[4..1] (4 lanes) + scalar a[4]*b[0]
     * sub: 3 scalar terms
     * ------------------------------------------------------------------ */
    f0  = _mm256_loadu_si256((__m256i *)a);              /* a[0..3] */
    f1  = _mm256_loadu_si256((__m256i *)(b + 1));        /* b[1..4] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[4,3,2,1] */
    f2  = mullo_epi64(f0, f1);
    res[4] = hsum_epi64(f2) + OVERFLOWING_MUL64(a[4], b[0]);
    res[4] -= OVERFLOWING_MUL64(a[5], b[7]) + OVERFLOWING_MUL64(a[6], b[6])
            + OVERFLOWING_MUL64(a[7], b[5]);

    /* ------------------------------------------------------------------ */
    /* res[5] = a[0]*b[5]+a[1]*b[4]+a[2]*b[3]+a[3]*b[2]+a[4]*b[1]+a[5]*b[0]
     *        - (a[6]*b[7] + a[7]*b[6])
     *
     * add: a[0..3] · b[5..2] (4 lanes) + scalar a[4]*b[1] + scalar a[5]*b[0]
     * sub: 2 scalar terms
     * ------------------------------------------------------------------ */
    f0  = _mm256_loadu_si256((__m256i *)a);              /* a[0..3] */
    f1  = _mm256_loadu_si256((__m256i *)(b + 2));        /* b[2..5] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[5,4,3,2] */
    f2  = mullo_epi64(f0, f1);
    res[5] = hsum_epi64(f2)
           + OVERFLOWING_MUL64(a[4], b[1]) + OVERFLOWING_MUL64(a[5], b[0]);
    res[5] -= OVERFLOWING_MUL64(a[6], b[7]) + OVERFLOWING_MUL64(a[7], b[6]);

    /* ------------------------------------------------------------------ */
    /* res[6] = a[0]*b[6]+a[1]*b[5]+a[2]*b[4]+a[3]*b[3]
     *         +a[4]*b[2]+a[5]*b[1]+a[6]*b[0]
     *        - a[7]*b[7]
     *
     * add: a[0..3] · b[6..3] (4 lanes) + scalar a[4]*b[2]+a[5]*b[1]+a[6]*b[0]
     * sub: 1 scalar term
     * ------------------------------------------------------------------ */
    f0  = _mm256_loadu_si256((__m256i *)a);              /* a[0..3] */
    f1  = _mm256_loadu_si256((__m256i *)(b + 3));        /* b[3..6] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[6,5,4,3] */
    f2  = mullo_epi64(f0, f1);
    res[6] = hsum_epi64(f2)
           + OVERFLOWING_MUL64(a[4], b[2]) + OVERFLOWING_MUL64(a[5], b[1])
           + OVERFLOWING_MUL64(a[6], b[0]);
    res[6] -= OVERFLOWING_MUL64(a[7], b[7]);

    /* ------------------------------------------------------------------ */
    /* res[7] = a[0]*b[7]+a[1]*b[6]+...+a[7]*b[0]  (no subtraction)
     *
     * Full 8-term dot product: split into two AVX2 groups of 4 and sum.
     * ------------------------------------------------------------------ */
    /* lower half: a[0..3] · b[7..4] */
    f0  = _mm256_loadu_si256((__m256i *)a);              /* a[0..3] */
    f1  = _mm256_loadu_si256((__m256i *)(b + 4));        /* b[4..7] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[7,6,5,4] */
    f2  = mullo_epi64(f0, f1);
    res[7] = hsum_epi64(f2);

    /* upper half: a[4..7] · b[3..0] */
    f0  = _mm256_loadu_si256((__m256i *)(a + 4));        /* a[4..7] */
    f1  = _mm256_loadu_si256((__m256i *)b);              /* b[0..3] */
    f1  = _mm256_permute4x64_epi64(f1, 0x1B);           /* b[3,2,1,0] */
    f2  = mullo_epi64(f0, f1);
    res[7] += hsum_epi64(f2);

#else
#error "Implementation assumes N8 = 8"
#endif
}



static void toom4_32(const uint64_t *a, const uint64_t *b, uint64_t *res)
{
    //#define N 32//64
    // //#define SB 8//16

    uint64_t aw1[N8], aw2[N8], aw3[N8], aw4[N8], aw5[N8], aw6[N8], aw7[N8];
    uint64_t bw1[N8], bw2[N8], bw3[N8], bw4[N8], bw5[N8], bw6[N8], bw7[N8];

        // ----- Striding Decomposition -----
    uint64_t w1[N8]={0}, w2[N8]={0}, w3[N8]={0},
             w4[N8]={0}, w5[N8]={0}, w6[N8]={0}, w7[N8]={0};

    memset(res, 0, N32*sizeof(uint64_t)); // striding outputs 32 degree polynomial as the product of two 32-degree polynomials

    uint64_t A0[N8], A1[N8], A2[N8], A3[N8];
    uint64_t B0[N8], B1[N8], B2[N8], B3[N8];

    uint64_t r0, r1, r2, r3, r4, r5, r6;

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
        uint64_t t0=r0+r2, t1=r1+r3;

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

        uint64_t t0=r0+r2, t1=r1+r3;

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
    uint64_t inv3 = 2863311531, inv9 = 954437177, inv15 = 4008636143;//Inverse with respect to 2^32

    uint64_t prev0, prev1, prev2; // for striding

    for(int i=0;i<N8;i++){  // striding

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

static void toom4_128(const uint64_t *a1, const uint64_t *b1, uint64_t *result)
{
    //#define N 128 //64
    //#define SB 32 //16

    uint64_t aw1[N32], aw2[N32], aw3[N32], aw4[N32], aw5[N32], aw6[N32], aw7[N32];
    uint64_t bw1[N32], bw2[N32], bw3[N32], bw4[N32], bw5[N32], bw6[N32], bw7[N32];

    memset(result, 0, (N128)*sizeof(uint64_t));

    // ----- Striding Implementation -----
    uint64_t w1[N32] = {0}, w2[N32] = {0}, w3[N32] = {0}, w4[N32] = {0},
                            w5[N32] = {0}, w6[N32] = {0}, w7[N32] = {0};
    // -----------------------------------

    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;

    uint64_t A0[N32], A1[N32], A2[N32], A3[N32];
    uint64_t B0[N32], B1[N32], B2[N32], B3[N32];

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

    uint64_t *C;
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
    uint64_t inv3 = 2863311531, inv9 = 954437177, inv15 = 4008636143;//Inverse with respect to 2^32

    uint64_t prev0, prev1, prev2;

    for(int i=0;i<N32;i++){  // striding

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
        r4 = (uint64_t)(((r4 - (r2 << 3)) * (uint64_t)inv3) >> 3);
        r5 = r5 + r1;
        r1 = (uint64_t)(((r1 + (r3 << 4)) * (uint64_t)inv9) >> 1);
        r3 = -(r3 + r1);
        r5 = (uint64_t)(((30 * r1 - r5) * (uint64_t)inv15) >> 2);
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

static void toom_cook_4way (const uint64_t *a1, const uint64_t *b1, uint64_t *result) {
  uint64_t inv3 = 2863311531, inv9 = 954437177, inv15 = 4008636143;//Inverse with respect to 2^32

    uint64_t aw1[N_SB], aw2[N_SB], aw3[N_SB], aw4[N_SB], aw5[N_SB], aw6[N_SB], aw7[N_SB];
    uint64_t bw1[N_SB], bw2[N_SB], bw3[N_SB], bw4[N_SB], bw5[N_SB], bw6[N_SB], bw7[N_SB];

    // ----- Striding Implementation -----
    uint64_t w1[N_SB] = {0}, w2[N_SB] = {0}, w3[N_SB] = {0}, w4[N_SB] = {0},
                            w5[N_SB] = {0}, w6[N_SB] = {0}, w7[N_SB] = {0};
    // -----------------------------------

    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;

    uint64_t A0[N_SB], A1[N_SB], A2[N_SB], A3[N_SB];
    uint64_t B0[N_SB], B1[N_SB], B2[N_SB], B3[N_SB];

    for(unsigned int j = 0; j < N_SB; j++) {

        A0[j] = a1[4*j];
        A1[j] = a1[4*j + 1];
        A2[j] = a1[4*j + 2];
        A3[j] = a1[4*j + 3];

        B0[j] = b1[4*j];
        B1[j] = b1[4*j + 1];
        B2[j] = b1[4*j + 2];
        B3[j] = b1[4*j + 3];
    }

    uint64_t *C;
    C = result;

    unsigned int j;

    // EVALUATION
    for (j = 0; j < N_SB; ++j) {
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
    for (j = 0; j < N_SB; ++j) {
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
  toom4_128(aw1, bw1, w1);
  toom4_128(aw2, bw2, w2);
  toom4_128(aw3, bw3, w3);
  toom4_128(aw4, bw4, w4);
  toom4_128(aw5, bw5, w5);
  toom4_128(aw6, bw6, w6);
  toom4_128(aw7, bw7, w7);

    // INTERPOLATION

    uint64_t prev0, prev1, prev2;

    for(unsigned int i=0;i<N_SB;i++){  // striding needs N_SB and NOT 2*N_SB - 1 interpolations

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
        r4 = (uint64_t)(((r4 - (r2 << 3)) * (uint64_t)inv3) >> 3);
        r5 = r5 + r1;
        r1 = (uint64_t)(((r1 + (r3 << 4)) * (uint64_t)inv9) >> 1);
        r3 = -(r3 + r1);
        r5 = (uint64_t)(((30 * r1 - r5) * (uint64_t)inv15) >> 2);
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
  unsigned int i;
  uint64_t r[2*N];
  uint64_t ai[N];
  uint64_t bi[N];

  for(i = 0; i < N; i++) {
    ai[i] = (uint64_t)a->coeffs[i] & (Q - 1);
    bi[i] = (uint64_t)b->coeffs[i] & (Q - 1);
  }

  for(i = 0; i < 2*N; i++)
    r[i] = 0;

  toom_cook_4way(ai, bi, r);

  for(i = N; i < 2*N-1; i++) {
    r[i-N] = (r[i-N] + Q - r[i]) & (Q - 1);
  }

  for(i = 0; i < N; i++)
    c->coeffs[i] = r[i];
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
*              Assumes input coefficients to be less than 2*Q (P).
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
*              5*SHAKE128_RATE bytes).
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
*              output stream from SHAKE256(seed|nonce)
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
  unsigned char inbuf[SEEDBYTES + 2];
  /* Probability that we need more than 2 blocks: < 2^{-84} */
  unsigned char outbuf[XOF_136];
  unsigned char counter = 0;
  unsigned int pos = 0;

  for(i= 0; i < SEEDBYTES; ++i)
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
#if GAMMA1 <= (1 << 19)
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
#elif GAMMA1 <= (1 << 23)
  unsigned int ctr, pos;
  uint32_t t;

  ctr = pos = 0;
  while (ctr < len) {
    t = buf[pos];
    t |= (uint32_t)buf[pos + 1] << 8;
    t |= (uint32_t)buf[pos + 2] << 16;

    pos += 3;

    if (t <= 2*GAMMA1 - 2)
      a[ctr++] = Q + GAMMA1 - 1 - t;

    if (pos > buflen - 3)
      break;
  }

  return ctr;
#else
#error "rej_gamma1m1() assumes GAMMA1 - 1 fits in 19 bits"
#endif
}

/*************************************************
* Name:        poly_uniform_gamma1m1
*
* Description: Sample polynomial with uniformly random coefficients
*              in [-(GAMMA1 - 1), GAMMA1 - 1] by performing rejection
*              sampling on output stream of SHAKE256(seed|nonce)
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

  for (i = 0, j = 0; i < N; i += 8, j += 13) {
    r[j] = a->coeffs[i] & 0xff;
    r[j+1] = (a->coeffs[i] >> 8) | ((a->coeffs[i+1] & 0x7) << 5);
    r[j+2] = (a->coeffs[i+1] >> 3) & 0xff;
    r[j+3] = (a->coeffs[i+1] >> 11) | ((a->coeffs[i+2] & 0x3f) << 2);
    r[j+4] = (a->coeffs[i+2] >> 6) | ((a->coeffs[i+3] & 0x1) << 7);
    r[j+5] = (a->coeffs[i+3] >> 1) & 0xff;
    r[j+6] = (a->coeffs[i+3] >> 9) | ((a->coeffs[i+4] & 0xf) << 4);
    r[j+7] = (a->coeffs[i+4] >> 4) & 0xff;
    r[j+8] = (a->coeffs[i+4] >> 12) | ((a->coeffs[i+5] & 0x7f) << 1);
    r[j+9] = (a->coeffs[i+5] >> 7) | ((a->coeffs[i+6] & 0x3) << 6);
    r[j+10] = (a->coeffs[i+6] >> 2) & 0xff;
    r[j+11] = (a->coeffs[i+6] >> 10) | ((a->coeffs[i+7] & 0x1f) << 3);
    r[j+12] = a->coeffs[i+7] >> 5;
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

  for (i = 0, j = 0; i < N; i += 8, j += 13) {
    r->coeffs[i] = a[j] | ((uint32_t)(a[j+1] & 0x1f) << 8);
    r->coeffs[i+1] = (a[j+1] >> 5) | ((uint32_t)a[j+2] << 3) | ((uint32_t)(a[j+3] & 0x3) << 11);
    r->coeffs[i+2] = (a[j+3] >> 2) | ((uint32_t)(a[j+4] & 0x7f) << 6);
    r->coeffs[i+3] = (a[j+4] >> 7) | ((uint32_t)a[j+5] << 1) | ((uint32_t)(a[j+6] & 0xf) << 9);
    r->coeffs[i+4] = (a[j+6] >> 4) | ((uint32_t)a[j+7] << 4) | ((uint32_t)(a[j+8] & 0x1) << 12);
    r->coeffs[i+5] = (a[j+8] >> 1) | ((uint32_t)(a[j+9] & 0x3f) << 7);
    r->coeffs[i+6] = (a[j+9] >> 6) | ((uint32_t)a[j+10] << 2) | ((uint32_t)(a[j+11] & 0x7) << 10);
    r->coeffs[i+7] = (a[j+11] >> 3) | ((uint32_t)a[j+12] << 5);
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
#if GAMMA1 <= (1 << 19)
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
#elif GAMMA1 <= (1 << 21)
  unsigned int i, j;
  uint32_t t[4];

  for (i = 0, j = 0; i < N; i += 4, j += 11) {
    t[0] = GAMMA1 - 1 - a->coeffs[i];
    t[0] += ((int32_t)t[0] >> 31) & Q;
    t[1] = GAMMA1 - 1 - a->coeffs[i+1];
    t[1] += ((int32_t)t[1] >> 31) & Q;
    t[2] = GAMMA1 - 1 - a->coeffs[i+2];
    t[2] += ((int32_t)t[2] >> 31) & Q;
    t[3] = GAMMA1 - 1 - a->coeffs[i+3];
    t[3] += ((int32_t)t[3] >> 31) & Q;

    r[j] = t[0] & 0xff;
    r[j+1] = (t[0] >> 8) & 0xff;
    r[j+2] = (t[0] >> 16) | ((t[1] & 0x3) << 6);
    r[j+3] = (t[1] >> 2) & 0xff;
    r[j+4] = (t[1] >> 10) & 0xff;
    r[j+5] = (t[1] >> 18) | ((t[2] & 0xf) << 4);
    r[j+6] = (t[2] >> 4) & 0xff;
    r[j+7] = (t[2] >> 12) & 0xff;
    r[j+8] = (t[2] >> 20) | ((t[3] & 0x3f) << 2);
    r[j+9] = (t[3] >> 6) & 0xff;
    r[j+10] = t[3] >> 14;
  }
#else
#error "polyz_pack() assumes GAMMA1 <= 2^{19}"
#endif
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
  unsigned int i, j;

  for(i = 0, j = 0; i < N; i += 4, j += 11) {
    r->coeffs[i] = a[j] | ((uint32_t)a[j+1] << 8) | ((uint32_t)(a[j+2] & 0x3f) << 16);
    r->coeffs[i+1] = (a[j+2] >> 6) | ((uint32_t)a[j+3] << 2) | ((uint32_t)a[j+4] << 10) | ((uint32_t)(a[j+5] & 0xf) << 18);
    r->coeffs[i+2] = (a[j+5] >> 4) | ((uint32_t)a[j+6] << 4) | ((uint32_t)a[j+7] << 12) | ((uint32_t)(a[j+8] & 0x3) << 20);
    r->coeffs[i+3] = (a[j+8] >> 2) | ((uint32_t)a[j+9] << 6) | ((uint32_t)a[j+10] << 14);

    r->coeffs[i+0] = GAMMA1 - 1 - r->coeffs[i];
    r->coeffs[i+0] += ((int32_t)r->coeffs[i] >> 31) & Q;
    r->coeffs[i+1] = GAMMA1 - 1 - r->coeffs[i+1];
    r->coeffs[i+1] += ((int32_t)r->coeffs[i+1] >> 31) & Q;
    r->coeffs[i+2] = GAMMA1 - 1 - r->coeffs[i+2];
    r->coeffs[i+2] += ((int32_t)r->coeffs[i+2] >> 31) & Q;
    r->coeffs[i+3] = GAMMA1 - 1 - r->coeffs[i+3];
    r->coeffs[i+3] += ((int32_t)r->coeffs[i+3] >> 31) & Q;
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
