/*
 * QingLuan Digital Signature Scheme
 * restr.c - Restricted subgroup arithmetic over E = <g> subset F_q*
 */
#include "restr.h"
#include "fq_arith.h"

static fq_t RESTR_TAB[PARAM_Z];
static int  RESTR_READY = 0;

void restr_init(void)
{
    fq_t acc = 1;
    for (uint8_t i = 0; i < PARAM_Z; i++) {
        RESTR_TAB[i] = acc;
        acc = fq_mul(acc, (fq_t)PARAM_G);
    }
    RESTR_READY = 1;
}

fq_t restr_val(uint8_t exp)
{
    if (!RESTR_READY) restr_init();   /* idempotent lazy init; flag is not secret */
    fq_t v = 0;
    for (uint8_t i = 0; i < PARAM_Z; i++) {
        uint8_t d  = (uint8_t)(i ^ exp);
        uint8_t nz = (uint8_t)((d | (uint8_t)(-(int8_t)d)) >> 7); /* 1 if d!=0 else 0 */
        fq_t    m  = (fq_t)(nz - 1);                              /* all-ones if i==exp */
        v |= (fq_t)(RESTR_TAB[i] & m);
    }
    return v;
}

uint8_t restr_exp_add(uint8_t a, uint8_t b)
{
    /* a,b in [0,z), z<=127 => s<=2z-2<=252 fits in uint8 */
    uint8_t s     = (uint8_t)(a + b);
    uint8_t r     = (uint8_t)(s - PARAM_Z);          /* underflows (>=128) iff s<z */
    uint8_t under = (uint8_t)(r >> 7);               /* 1 if s<z, else 0 (r<=z-2<128) */
    uint8_t mask  = (uint8_t)(-(int8_t)under);       /* 0xFF if s<z else 0x00 */
    return (uint8_t)((s & mask) | (r & (uint8_t)~mask));
}

uint8_t restr_exp_sub(uint8_t a, uint8_t b)
{
    /* (a - b) mod z, constant-time. t = a + z - b in [1, 2z-1] (z<=127 => <=252). */
    uint8_t t     = (uint8_t)(a + PARAM_Z - b);
    uint8_t r     = (uint8_t)(t - PARAM_Z);          /* t-z; underflows (>=128) iff t<z */
    uint8_t under = (uint8_t)(r >> 7);               /* 1 if t<z (then result is t) */
    uint8_t mask  = (uint8_t)(-(int8_t)under);       /* 0xFF if t<z else 0x00 */
    return (uint8_t)((t & mask) | (r & (uint8_t)~mask));
}

void restr_vec_from_exp(fq_t *e, const uint8_t *eta, size_t n)
{
    for (size_t i = 0; i < n; i++)
        e[i] = restr_val(eta[i]);
}
