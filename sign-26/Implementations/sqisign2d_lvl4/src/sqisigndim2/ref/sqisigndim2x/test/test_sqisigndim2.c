#include <rng.h>
#include <stdio.h>
#include <ec.h>
#include <inttypes.h>
#include <locale.h>
#include <time.h>

#include "test_sqisigndim2.h"
#include <tools.h>

bool curve_is_canonical(ec_curve_t const *E)
{
    ec_curve_t EE;
    ec_isom_t isom;
    ec_curve_normalize(&EE, &isom, E);

    fp2_t lhs, rhs;
    fp2_mul(&lhs, &E->A, &EE.C);
    fp2_mul(&rhs, &E->C, &EE.A);
    return fp2_is_equal(&lhs, &rhs);
}


int test_sqisign(int repeat)
{
    int res = 1;

    public_key_t pk;
    secret_key_t sk;
    signature_t sig;
    unsigned char msg[32] = { 0 };

    secret_key_init(&sk);
    secret_sig_init(&sig);

    clock_t t = tic();
   

    for (int i = 0; i < repeat; ++i)
    {
        protocols_keygen(&pk, &sk);
    }

    printf("*******************************************************\n");
    float ms = (1000. * (float) (clock() - t) / CLOCKS_PER_SEC);
    printf("average keygen time [%.2f ms]\n", (float) (ms/repeat));


    t = tic();
    for (int i = 0; i < repeat; ++i)
    {
        int val = protocols_sign(&sig, &pk, &sk, msg, 32, 0);
        //int res = protocols_verif(&sig, &pk, msg, 32);
    }
    // TOC(t, "protocols_sign");

    printf("*******************************************************\n");
   ms = (1000. * (float) (clock() - t) / CLOCKS_PER_SEC);
    printf("average signing time [%.2f ms]\n", (float) (ms/repeat));

      t = tic();
    for (int i = 0; i < repeat; ++i)
    {
        int res = protocols_verif(&sig, &pk, msg, 32);
    }
    printf("*******************************************************\n");

   ms = (1000. * (float) (clock() - t) / CLOCKS_PER_SEC);
    printf("average verification time [%.2f ms]\n", (float) (ms/repeat));

    secret_key_finalize(&sk);

    return res;
}

// run all tests in module
int main(){
    int res = 1;

    randombytes_init((unsigned char *) "some", (unsigned char *) "string", 128);


    res &= test_sqisign(10);

    if(!res){
        printf("\nSome tests failed!\n");
    } 
    else {
        printf("All tests passed!\n");
    }
    return(!res);
}
