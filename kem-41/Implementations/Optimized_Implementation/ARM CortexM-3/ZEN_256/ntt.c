#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

extern void poly_ntt_asm(int16_t *a, const int16_t f[128]);
void poly_ntt(int16_t *a) 
{
    poly_ntt_asm(a, f);
}

extern void poly_ntt_mq_asm(int16_t *a, const int16_t f[128]);
void poly_ntt_mq(int16_t *a) 
{
    poly_ntt_mq_asm(a, f);
}

extern void poly_intt_asm(int16_t *a, const int16_t fn[128]);
void poly_intt(int16_t *a) 
{
    poly_intt_asm(a, fn);
}

extern void poly_basemul_asm(int16_t *r,  int16_t *a,  int16_t *b, int16_t *f);
void poly_basemul_ntt(int16_t *r,  int16_t *a,  int16_t *b)
{
    poly_basemul_asm(r, a, b, f);
}

extern void poly_basemul_mq_asm(int16_t *r,  int16_t *a,  int16_t *b, int16_t *f);
void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b)
{
    poly_basemul_mq_asm(r, a, b, f);
}

extern void poly_baseinv_asm(int16_t *r,  int16_t *a, int16_t *f, int16_t *qinv);
void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    poly_baseinv_asm(r, a, f, qinv);
}
