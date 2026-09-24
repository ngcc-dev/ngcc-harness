#include "toomcook.h"
#include <stdint.h>
#include <string.h>

#define OVERFLOWING_MUL(X, Y) ((int16_t)((int32_t)(X) * (int32_t)(Y)))
#define KARATSUBA_N N_SB
#define N_SB (LORE_N >> 2)
#define N_SB_RES (2 * N_SB - 1)

/*************************************************
* Name:        karatsuba_simple
*
* Description: A simple implementation of the Karatsuba multiplication algorithm.
* This function is used as a base case for the Toom-Cook algorithm.
*
* Arguments:   - const int16_t *a_1:          pointer to the first input polynomial
* - const int16_t *b_1:          pointer to the second input polynomial
* - int16_t *result_final: pointer to the output polynomial
**************************************************/
static void karatsuba_simple(const int16_t *a_1, const int16_t *b_1,
                             int16_t *result_final) {
    int16_t d01[KARATSUBA_N / 2 - 1];
    int16_t d0123[KARATSUBA_N / 2 - 1];
    int16_t d23[KARATSUBA_N / 2 - 1];
    int16_t result_d01[KARATSUBA_N - 1];
    int32_t i, j;

    memset(result_d01, 0, (KARATSUBA_N - 1) * sizeof(int16_t));
    memset(d01, 0, (KARATSUBA_N / 2 - 1) * sizeof(int16_t));
    memset(d0123, 0, (KARATSUBA_N / 2 - 1) * sizeof(int16_t));
    memset(d23, 0, (KARATSUBA_N / 2 - 1) * sizeof(int16_t));
    memset(result_final, 0, (2 * KARATSUBA_N - 1) * sizeof(int16_t));

    int16_t acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10;
    for (i = 0; i < KARATSUBA_N / 4; i++) {
        acc1 = a_1[i]; acc2 = a_1[i + KARATSUBA_N / 4];
        acc3 = a_1[i + 2 * KARATSUBA_N / 4]; acc4 = a_1[i + 3 * KARATSUBA_N / 4];
        for (j = 0; j < KARATSUBA_N / 4; j++) {
            acc5 = b_1[j]; acc6 = b_1[j + KARATSUBA_N / 4];
            result_final[i + j] += OVERFLOWING_MUL(acc1, acc5);
            result_final[i + j + 2 * KARATSUBA_N / 4] += OVERFLOWING_MUL(acc2, acc6);
            acc7 = acc5 + acc6; acc8 = acc1 + acc2;
            d01[i + j] += OVERFLOWING_MUL(acc7, acc8);
            
            acc7 = b_1[j + 2 * KARATSUBA_N / 4]; acc8 = b_1[j + 3 * KARATSUBA_N / 4];
            result_final[i + j + 4 * KARATSUBA_N / 4] += OVERFLOWING_MUL(acc7, acc3);
            result_final[i + j + 6 * KARATSUBA_N / 4] += OVERFLOWING_MUL(acc8, acc4);
            acc9 = acc3 + acc4; acc10 = acc7 + acc8;
            d23[i + j] += OVERFLOWING_MUL(acc9, acc10);
            
            acc5 += acc7; acc7 = acc1 + acc3;
            result_d01[i + j] += OVERFLOWING_MUL(acc5, acc7);
            acc6 += acc8; acc8 = acc2 + acc4;
            result_d01[i + j + 2 * KARATSUBA_N / 4] += OVERFLOWING_MUL(acc6, acc8);
            acc5 += acc6; acc7 += acc8;
            d0123[i + j] += OVERFLOWING_MUL(acc5, acc7);
        }
    }
    for (i = 0; i < KARATSUBA_N / 2 - 1; i++) {
        d0123[i] -= (int16_t)(result_d01[i] + result_d01[i + 2 * KARATSUBA_N / 4]);
        d01[i] -= (int16_t)(result_final[i] + result_final[i + 2 * KARATSUBA_N / 4]);
        d23[i] -= (int16_t)(result_final[i + 4 * KARATSUBA_N / 4] + result_final[i + 6 * KARATSUBA_N / 4]);
    }
    for (i = 0; i < KARATSUBA_N / 2 - 1; i++) {
        result_d01[i + KARATSUBA_N / 4] += d0123[i];
        result_final[i + KARATSUBA_N / 4] += d01[i];
        result_final[i + 5 * KARATSUBA_N / 4] += d23[i];
    }
    for (i = 0; i < KARATSUBA_N - 1; i++) {
       result_d01[i] -= (int16_t)(result_final[i] + result_final[i + KARATSUBA_N]);
    }
    for (i = 0; i < KARATSUBA_N - 1; i++) {
        result_final[i + KARATSUBA_N / 2] += result_d01[i];
    }
}

/*************************************************
* Name:        toom_cook_4way
*
* Description: Toom-Cook 4-way multiplication algorithm.
*
* Arguments:   - const int16_t *a1:     pointer to the first input polynomial
* - const int16_t *b1:     pointer to the second input polynomial
* - int16_t *result: pointer to the output polynomial
**************************************************/
static void toom_cook_4way(const int16_t *a1, const int16_t *b1, int16_t *result) {
    //int16_t inv3 = 43691, inv9 = 36409, inv15 = 61167; 
    int16_t inv3 = -21845, inv9 = -29127, inv15 = -4369;
    int16_t aw1[N_SB], aw2[N_SB], aw3[N_SB], aw4[N_SB], aw5[N_SB], aw6[N_SB], aw7[N_SB];
    int16_t bw1[N_SB], bw2[N_SB], bw3[N_SB], bw4[N_SB], bw5[N_SB], bw6[N_SB], bw7[N_SB];
    int16_t w1[N_SB_RES] = {0}, w2[N_SB_RES] = {0}, w3[N_SB_RES] = {0},
            w4[N_SB_RES] = {0}, w5[N_SB_RES] = {0}, w6[N_SB_RES] = {0},
            w7[N_SB_RES] = {0};
    int16_t r0, r1, r2, r3, r4, r5, r6, r7;
    const int16_t *A0, *A1, *A2, *A3, *B0, *B1, *B2, *B3;
    
    A0 = a1; A1 = &a1[N_SB]; A2 = &a1[2 * N_SB]; A3 = &a1[3 * N_SB];
    B0 = b1; B1 = &b1[N_SB]; B2 = &b1[2 * N_SB]; B3 = &b1[3 * N_SB];

    int i, j;
    for (j = 0; j < N_SB; ++j) {
        r0 = A0[j]; r1 = A1[j]; r2 = A2[j]; r3 = A3[j];
        r4 = r0 + r2; r5 = r1 + r3; r6 = r4 + r5; r7 = r4 - r5;
        aw3[j] = r6; aw4[j] = r7;
        r4 = (int16_t)(((int32_t)r0 * 4 + r2) * 2); r5 = (int16_t)((int32_t)r1 * 4 + r3);
        r6 = r4 + r5; r7 = r4 - r5;
        aw5[j] = r6; aw6[j] = r7;
        r4 = (int16_t)((int32_t)r3 * 8 + (int32_t)r2 * 4 + (int32_t)r1 * 2 + r0);
        aw2[j] = r4; aw7[j] = r0; aw1[j] = r3;
    }
    for (j = 0; j < N_SB; ++j) {
        r0 = B0[j]; r1 = B1[j]; r2 = B2[j]; r3 = B3[j];
        r4 = r0 + r2; r5 = r1 + r3; r6 = r4 + r5; r7 = r4 - r5;
        bw3[j] = r6; bw4[j] = r7;
        r4 = (int16_t)(((int32_t)r0 * 4 + r2) * 2); r5 = (int16_t)((int32_t)r1 * 4 + r3);
        r6 = r4 + r5; r7 = r4 - r5;
        bw5[j] = r6; bw6[j] = r7;
       r4 = (int16_t)((int32_t)r3 * 8 + (int32_t)r2 * 4 + (int32_t)r1 * 2 + r0);
        bw2[j] = r4; bw7[j] = r0; bw1[j] = r3;
    }
    
    karatsuba_simple(aw1, bw1, w1); karatsuba_simple(aw2, bw2, w2);
    karatsuba_simple(aw3, bw3, w3); karatsuba_simple(aw4, bw4, w4);
    karatsuba_simple(aw5, bw5, w5); karatsuba_simple(aw6, bw6, w6);
    karatsuba_simple(aw7, bw7, w7);

    for (i = 0; i < N_SB_RES; ++i) {
        r0 = w1[i]; r1 = w2[i]; r2 = w3[i]; r3 = w4[i];
        r4 = w5[i]; r5 = w6[i]; r6 = w7[i];
        r1 += r4; r5 -= r4; r3 = (int16_t)((r3 - r2) >> 1);
        r4 -= r0; r4 -= (int16_t)((int32_t)r6 * 64); r4 = (int16_t)((int32_t)r4 * 2 + r5);
        r2 += r3; r1 -= (int16_t)((int32_t)r2 * 64 + r2);
        r2 -= r6; r2 -= r0; r1 += (int16_t)(45 * r2);
        r4 = (int16_t)(((int64_t)(r4 - (int32_t)r2 * 8) * inv3) >> 3);
        r5 += r1; r1 = (int16_t)(((int64_t)(r1 + (int32_t)r3 * 16) * inv9) >> 1);
        r3 = (int16_t)(-(r3 + r1)); r5 = (int16_t)(((int64_t)(30 * r1 - r5) * inv15) >> 2);
        r2 -= r4; r1 -= r5;
        // Accumulate results with N_SB offsets.
        result[i] += r6; 
        result[i + N_SB] += r5; 
        result[i + 2 * N_SB] += r4;
        result[i + 3 * N_SB] += r3; 
        result[i + 4 * N_SB] += r2; 
        result[i + 5 * N_SB] += r1;
        result[i + 6 * N_SB] += r0;
    }
}

/*************************************************
* Name:        poly_mul_acc
*
* Description: Polynomial multiplication with accumulation. This function
* uses Toom-Cook 4-way multiplication and reduces the result
* modulo (X^N + 1).
*
* Arguments:   - const int16_t a[LORE_N]:   the first input polynomial
* - const int16_t b[LORE_N]:   the second input polynomial
* - int16_t res[LORE_N]:       the output polynomial (accumulator)
**************************************************/
void poly_mul_acc(const int16_t a[LORE_N], const int16_t b[LORE_N], int16_t res[LORE_N]) {
    int16_t c[2 * LORE_N] = {0};
    toom_cook_4way(a, b, c);
    for (int i = 0; i < LORE_N; i++) {
        res[i] += (int16_t)(c[i] - c[i + LORE_N]);
    }
}