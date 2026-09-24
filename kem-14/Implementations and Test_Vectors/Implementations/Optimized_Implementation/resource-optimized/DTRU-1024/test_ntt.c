#include "poly.h"
#include "params.h"
// #include "reduce.h"
#include <stdint.h>
#include <stdio.h>
#include "randombytes.h"
#include "ntt.h"
#include "ntt_avx2.h"
#define NTESTS 1000

void print_poly(const char *name, const poly *p) {
    printf("%s: ", name);
    for (int i = 0; i < DTRU_N; i++) {
        printf("%d ", p->coeffs[i]);
    }
    printf("\n");
}

void poly_naivemul_q(poly *c, const poly *a, const poly *b, int16_t q) {
     int16_t r[2*DTRU_N] = {0};

     for(int i = 0; i < DTRU_N; i++)
     {
         for(int j = 0; j < DTRU_N; j++){
             r[i+j] = (r[i+j] + (int32_t)a->coeffs[i]*b->coeffs[j]) % q;
         }

     }
     for(int i = DTRU_N; i < 2*DTRU_N; i++){
         r[i-DTRU_N] = (r[i-DTRU_N] - r[i]);
         r[i-DTRU_N/2] = (r[i-DTRU_N/2] + r[i]);
         r[i] = 0;
     }
     for(int i = DTRU_N; i < 2*DTRU_N; i++){
         r[i-DTRU_N] = (r[i-DTRU_N] - r[i]);
         r[i-DTRU_N/2] = (r[i-DTRU_N/2] + r[i]);
         r[i] = 0;
     }


     for(int i = 0; i < DTRU_N; i++)
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

        for (int i = 0; i < DTRU_N / 16; ++i)
        {
            basemul(c + 16 * i,
                    a + 16 * i,
                    b + 16 * i,
                    zetas[64 + i]);
            basemul(c + 16 * i + 8,
                    a + 16 * i + 8,
                    b + 16 * i + 8,
                    -zetas[64 + i]);
        }

        // basemul_avx2
        for (int i = 0; i < DTRU_N / 16 / 8; ++i)
        {
            basemul_avx2(c_ntt_avx + i * 16 * 8,
                         a_ntt_avx + i * 16 * 8,
                         b_ntt_avx + i * 16 * 8,
                         zetas + 64 + 8 * i);
        }

        // test ntt result
        for (i = 0; i < DTRU_N; i++)
        {
            if ((a_ntt_avx[i] - a[i]) % DTRU_Q != 0)
            {
                cnt++;
                int delta = (a_ntt_avx[i] - a[i]) % DTRU_Q;
                if (delta < 0)
                    delta += DTRU_Q;
                printf("NTT wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, a[i], a_ntt_avx[i], delta);
                break;
                // return 1;
            }
        }
        for (i = 0; i < DTRU_N; i++)
        {
            if ((b_ntt_avx[i] - b[i]) % DTRU_Q != 0)
            {
                cnt++;
                int delta = (b_ntt_avx[i] - b[i]) % DTRU_Q;
                if (delta < 0)
                    delta += DTRU_Q;
                printf("NTT wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, b[i], b_ntt_avx[i], delta);
                break;
                // return 1;
            }
        }

        // test basemul result
        for (i = 0; i < DTRU_N; i++)
        {
            if ((c_ntt_avx[i] - c[i]) % DTRU_Q != 0)
            {
                cnt++;
                int delta = (c_ntt_avx[i] - c[i]) % DTRU_Q;
                if (delta < 0)
                    delta += DTRU_Q;
                printf("basemul wrong at i = %4d; ref = %6d avx = %6d; delta = %4d\n", i, c[i], c_ntt_avx[i], delta);
                break;
                // return 1;
            }
        }

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
    test_ntt();
    return 0;
}