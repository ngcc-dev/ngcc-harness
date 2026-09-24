#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "gf1131016.h"
#define NB 128
static mpz_t P;
static void tb(uint8_t b[NB],const mpz_t A){memset(b,0,NB);mpz_export(b,NULL,-1,1,-1,0,A);}
static void rep(gf1131016*r,const mpz_t A){uint8_t b[NB];tb(b,A);gf1131016_decode(r,b);}
static void val(mpz_t A,const gf1131016*r){uint8_t b[NB];gf1131016_encode(b,r);mpz_import(A,NB,-1,1,-1,0,b);}
static int fails=0;
static void test(const mpz_t A){
    gf1131016 ra,ro; rep(&ra,A);
    mpz_t E,G; mpz_inits(E,G,NULL);
    if(mpz_sgn(A)){ gf1131016_invert(&ro,&ra); val(G,&ro); mpz_invert(E,A,P);
        if(mpz_cmp(G,E)){ if(fails<10) gmp_printf("INV FAIL A=%Zx\n",A); fails++; } }
    int ls=gf1131016_legendre(&ra); int leg=mpz_legendre(A,P);
    if(ls!=leg){ if(fails<10) gmp_printf("LEG FAIL A=%Zx got=%d exp=%d\n",A,ls,leg); fails++; }
    mpz_clears(E,G,NULL);
}
int main(void){
    mpz_init(P); mpz_ui_pow_ui(P,2,1016); mpz_mul_ui(P,P,113); mpz_sub_ui(P,P,1);
    mpz_t A; mpz_init(A);
    // small values 0..5000
    for(unsigned k=0;k<=5000;k++){ mpz_set_ui(A,k); test(A); }
    // values near p: p-1 .. p-5000
    for(unsigned k=1;k<=5000;k++){ mpz_sub_ui(A,P,k); test(A); }
    // powers of two and 2^k-1 (high hamming weight) up to 320 bits
    for(int k=0;k<1024;k++){ mpz_ui_pow_ui(A,2,k); mpz_mod(A,A,P); test(A);
        mpz_ui_pow_ui(A,2,k); mpz_sub_ui(A,A,1); mpz_mod(A,A,P); test(A); }
    printf("edge tests done, FAILS=%d -> %s\n", fails, fails==0?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return fails?1:0;
}
