#include "poly.h"
#include "params.h"
// #include "reduce.h"
#include <stdint.h>
#include <stdio.h>
#include "ntt.h"
#include "ntt_avx2.h"
#include "randombytes.h"

#define NTESTS 10000

void print_poly(const char *name, const poly *p)
{
    printf("%s: ", name);
    for (int i = 0; i < DTRU_N; i++)
    {
        printf("%d ", p->coeffs[i]);
    }
    printf("\n");
}

void poly_naivemul_q(poly *c, const poly *a, const poly *b, int16_t q)
{
    int16_t r[2 * DTRU_N] = {0};

    for (int i = 0; i < DTRU_N; i++)
    {
        for (int j = 0; j < DTRU_N; j++)
        {
            r[i + j] = (r[i + j] + (int32_t)a->coeffs[i] * b->coeffs[j]) % q;
        }
    }
    for (int i = DTRU_N; i < 2 * DTRU_N; i++)
    {
        r[i - DTRU_N] = (r[i - DTRU_N] - r[i]);
        r[i - DTRU_N / 2] = (r[i - DTRU_N / 2] + r[i]);
        r[i] = 0;
    }
    for (int i = DTRU_N; i < 2 * DTRU_N; i++)
    {
        r[i - DTRU_N] = (r[i - DTRU_N] - r[i]);
        r[i - DTRU_N / 2] = (r[i - DTRU_N / 2] + r[i]);
        r[i] = 0;
    }

    for (int i = 0; i < DTRU_N; i++)
        c->coeffs[i] = r[i] % q;
}

static void test_ntt(void)
{
    int16_t a[DTRU_N] __attribute__((aligned(32)));
    int16_t a_ntt_avx[DTRU_N] __attribute__((aligned(32)));
    int16_t b[DTRU_N] __attribute__((aligned(32)));
    int16_t b_ntt_avx[DTRU_N] __attribute__((aligned(32)));
    int16_t c[DTRU_N] __attribute__((aligned(32)));
    int16_t c_ntt_avx[DTRU_N] __attribute__((aligned(32)));
    int16_t c_avx_out[DTRU_N] __attribute__((aligned(32)));

    int i, j, cnt = 0;
    for (j = 0; j < NTESTS; j++)
    {
        randombytes(a, DTRU_N);
        randombytes(b, DTRU_N);
        // 随机初始化输入数据
        for (i = 0; i < DTRU_N; i++)
        {
            a[i] = (a[i] % DTRU_Q + DTRU_Q) % DTRU_Q;
            b[i] = (b[i] % DTRU_Q + DTRU_Q) % DTRU_Q;
        }

        ntt_avx2(a_ntt_avx, a);
        ntt_avx2(b_ntt_avx, b);
        ntt(a);
        ntt(b);

        for (int i = 0; i < DTRU_N / 12; ++i)
        {
            basemul(c + 12 * i,
                    a + 12 * i,
                    b + 12 * i,
                    zetas[128 + 2 * i]);
            basemul(c + 12 * i + 4,
                    a + 12 * i + 4,
                    b + 12 * i + 4,
                    zetas_base[i]);
            basemul(c + 12 * i + 8,
                    a + 12 * i + 8,
                    b + 12 * i + 8,
                    -zetas[128 + 2 * i] - zetas_base[i]);
        }

        // for (int i = 0; i < DTRU_N / 12; ++i)
        // {
        //     basemul(c + 12 * i,
        //             a + 12 * i,
        //             b + 12 * i,
        //             1);
        //     basemul(c + 12 * i + 4,
        //             a + 12 * i + 4,
        //             b + 12 * i + 4,
        //             1);
        //     basemul(c + 12 * i + 8,
        //             a + 12 * i + 8,
        //             b + 12 * i + 8,
        //             1);
        // }

        int16_t ones[16] __attribute__((aligned(32)));
        for (i = 0; i < 16; i++)
        {
            ones[i] = 1;
        }
        // basemul_avx2
        for (i = 0; i < DTRU_N / 4 / 16; i++)
        {
            basemul_avx2(c_ntt_avx + i * 64, a_ntt_avx + i * 64, b_ntt_avx + i * 64, zetas_mul_inv_avx2 + i * 16);
            // basemul_avx2(c_ntt_avx + i * 64, a_ntt_avx + i * 64, b_ntt_avx + i * 64, ones);
        }

        // // test ntt result
        // for (i = 0; i < DTRU_N; i++)
        // {
        //     if ((a_ntt_avx[i] - a[i]) % DTRU_Q != 0)
        //     {
        //         cnt++;
        //         int delta = (a_ntt_avx[i] - a[i]) % DTRU_Q;
        //         if (delta < 0)
        //             delta += DTRU_Q;
        //         printf("NTT wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, a[i], a_ntt_avx[i], delta);
        //         break;
        //         // return 1;
        //     }
        // }
        // for (i = 0; i < DTRU_N; i++)
        // {
        //     if ((b_ntt_avx[i] - b[i]) % DTRU_Q != 0)
        //     {
        //         cnt++;
        //         int delta = (b_ntt_avx[i] - b[i]) % DTRU_Q;
        //         if (delta < 0)
        //             delta += DTRU_Q;
        //         printf("NTT wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, b[i], b_ntt_avx[i], delta);
        //         break;
        //         // return 1;
        //     }
        // }

        // // test basemul result
        // for (i = 0; i < DTRU_N; i++)
        // {
        //     if ((c_ntt_avx[i] - c[i]) % DTRU_Q != 0)
        //     {
        //         cnt++;
        //         int delta = (c_ntt_avx[i] - c[i]) % DTRU_Q;
        //         if (delta < 0)
        //             delta += DTRU_Q;
        //         printf("basemul wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, c[i], c_ntt_avx[i], delta);
        //         // break;
        //         // return 1;
        //     }
        // }

        invntt_avx2(c_avx_out, c_ntt_avx);
        invntt(c);
        
        // test invntt result
        for (i = 0; i < DTRU_N; i++)
        {
            if ((c_avx_out[i] - c[i]) % DTRU_Q != 0)
            {
                cnt++;
                int delta = (c_avx_out[i] - c[i]) % DTRU_Q;
                if (delta < 0)
                    delta += DTRU_Q;
                printf("INVNTT wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, c[i], c_avx_out[i], delta);
                break;
                // return 1;
            }
        }
    }
    if (cnt > 0)
        printf("%d errors in NTT!\n", cnt);
    else
        printf("ntt test pased!\n");
}

int main()
{
    // for (int i = 0; i < DTRU_N / 12; i++) {
    //     int zeta = zetas[128 + 2 * i];
    //     printf("%5d, ", zeta);
    //     zeta = zetas_base[i];
    //     printf("%5d, ", zeta);
    //     zeta = -zetas[128 + 2 * i]-zetas_base[i];
    //     printf("%5d, ", zeta);
    //     if (i % 4 == 3) {
    //         printf("\n    ");
    //     }
    // }
    test_ntt();
    return 0;
}