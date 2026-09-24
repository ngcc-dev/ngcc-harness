#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

void poly_ntt(int16_t *a) 
{
    small_ntt_asm_769(a, zetas_asm_769);
}

void poly_ntt_mq(int16_t *a) 
{
    small_ntt_mq_asm_769(a, zetas_asm_769);
}

void poly_intt(int16_t *a) 
{
    small_invntt_asm_769(a, zetas_inv_asm_769);
}

void poly_basemul_ntt(int16_t *r, int16_t *a, int16_t *b)
{
    basemul16_karatsuba_asm(r, a, b, zetas_769);
}

void poly_basemul_ntt_mq(int16_t *r, int16_t *a, int16_t *b)
{
    basemul16_karatsuba_mq_asm(r, a, b, zetas_769);
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    baseinv16_karatsuba_asm(r, a, zetas_769, qinv_asm_769);
}

void poly_cp(int16_t *cp, int16_t *tmp2)
{
    update_cp_asm(cp, tmp2);
}
