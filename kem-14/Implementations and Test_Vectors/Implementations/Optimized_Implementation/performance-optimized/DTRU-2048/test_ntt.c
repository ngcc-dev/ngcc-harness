#include "poly.h"
#include "params.h"
// #include "reduce.h"
#include "randombytes.h"
#include <stdint.h>
#include <stdio.h>

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

int main(){
    // for (int i = 0; i < DTRU_Q; i++) {
    //     int16_t t = fqinv(i);
    //     printf("%d, ", t);
    // }
    // printf("\n");
    poly f, g, h, k;
    unsigned char coins[DTRU_COINBYTES_KEYGEN];

    randombytes(coins, DTRU_COINBYTES_KEYGEN);
    poly_sample_keygen_f(&f, coins);
    poly_sample_keygen_g(&g, coins + DTRU_CBD2_BYTES);

    print_poly("Polynomial f\n", &f);
    print_poly("Polynomial g\n", &g);

    poly_naivemul_q(&k, &f, &g, DTRU_Q);
    poly_freeze(&k);

    poly_ntt(&f);
    print_poly("NTT Polynomial f\n", &f);
    poly_ntt(&g);
    print_poly("NTT Polynomial g\n", &g);

    poly_basemul(&h, &f, &g);
    print_poly("NTT Multiplication Result\n", &h);
    poly_invntt(&h);
    poly_freeze(&h);

    print_poly("Naive Multiplication Result\n", &k);
    print_poly("NTT Multiplication Result\n", &h);

return 0;
}