/*
 * test_restr.c - unit tests for restricted subgroup arithmetic over E=<g>
 */
#include "restr.h"
#include "fq_arith.h"
#include <stdio.h>
#include <assert.h>

int main(void){
    restr_init();
    /* E=<2> mod 127 = {1,2,4,8,16,32,64} */
    fq_t expect[7] = {1,2,4,8,16,32,64};
    for (int i=0;i<PARAM_Z;i++) assert(restr_val((uint8_t)i)==expect[i]);

    /* 指数域加 = 群乘:g^a * g^b = g^((a+b) mod z) */
    for (int a=0;a<PARAM_Z;a++)
      for (int b=0;b<PARAM_Z;b++){
        uint8_t e = restr_exp_add((uint8_t)a,(uint8_t)b);
        assert(e < PARAM_Z);
        assert(restr_val(e) == fq_mul(restr_val((uint8_t)a), restr_val((uint8_t)b)));
      }

    /* 指数域减 = 群除:g^(a-b) * g^b = g^a (用于 v = eta - eta') */
    for (int a=0;a<PARAM_Z;a++)
      for (int b=0;b<PARAM_Z;b++){
        uint8_t d = restr_exp_sub((uint8_t)a,(uint8_t)b);
        assert(d < PARAM_Z);
        assert(fq_mul(restr_val(d), restr_val((uint8_t)b)) == restr_val((uint8_t)a));
      }

    /* 向量映射 */
    uint8_t eta[5]={0,1,6,3,5}; fq_t e[5];
    restr_vec_from_exp(e,eta,5);
    for (int i=0;i<5;i++) assert(e[i]==expect[eta[i]]);

    printf("test_restr OK\n");
    return 0;
}
