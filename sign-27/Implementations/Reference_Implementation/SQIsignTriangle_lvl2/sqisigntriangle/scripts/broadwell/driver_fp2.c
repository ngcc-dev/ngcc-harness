#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "gf9309.h"
#define NB 40
typedef struct { gf9309 re, im; } fp2_t;
const digit_t p[5]  = {~0ULL,~0ULL,~0ULL,~0ULL,0x011FFFFFFFFFFFFFULL};
const digit_t p2[5] = {0xFFFFFFFFFFFFFFFEULL,~0ULL,~0ULL,~0ULL,0x023FFFFFFFFFFFFFULL};
extern void fp2_mul_c0(gf9309*,const fp2_t*,const fp2_t*);
extern void fp2_mul_c1(gf9309*,const fp2_t*,const fp2_t*);
extern void fp2_sq_c0(fp2_t*,const fp2_t*);
extern void fp2_sq_c1(gf9309*,const fp2_t*);
static mpz_t P;
static void tb(uint8_t b[NB],const mpz_t A){memset(b,0,NB);mpz_export(b,NULL,-1,1,-1,0,A);}
static void rep(gf9309*r,const mpz_t A){uint8_t b[NB];tb(b,A);gf9309_decode(r,b);}
static void val(mpz_t A,const gf9309*r){uint8_t b[NB];gf9309_encode(b,r);mpz_import(A,NB,-1,1,-1,0,b);}
static int fails=0;
static void chk(const char*n,const mpz_t g,const mpz_t e){if(mpz_cmp(g,e)){if(fails<8)gmp_fprintf(stderr,"FAIL %s\n g=%Zx\n e=%Zx\n",n,g,e);fails++;}}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):20000;
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,999);
    mpz_init(P);mpz_ui_pow_ui(P,2,309);mpz_mul_ui(P,P,9);mpz_sub_ui(P,P,1);
    mpz_t a0,a1,b0,b1,E,G,t,t2;mpz_inits(a0,a1,b0,b1,E,G,t,t2,NULL);
    for(it=0;it<iters && fails<=20;it++){
        mpz_urandomm(a0,st,P);mpz_urandomm(a1,st,P);mpz_urandomm(b0,st,P);mpz_urandomm(b1,st,P);
        fp2_t ra,rb; rep(&ra.re,a0);rep(&ra.im,a1);rep(&rb.re,b0);rep(&rb.im,b1);
        gf9309 o; fp2_t o2;
        /* mul_c0 = a0*b0 - a1*b1 */
        fp2_mul_c0(&o,&ra,&rb); val(G,&o);
        mpz_mul(t,a0,b0);mpz_mul(t2,a1,b1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("mul_c0",G,E);
        /* mul_c1 = a0*b1 + a1*b0 */
        fp2_mul_c1(&o,&ra,&rb); val(G,&o);
        mpz_mul(t,a0,b1);mpz_mul(t2,a1,b0);mpz_add(E,t,t2);mpz_mod(E,E,P);chk("mul_c1",G,E);
        /* sq_c0 = a0^2 - a1^2  (writes re) */
        fp2_sq_c0(&o2,&ra); val(G,&o2.re);
        mpz_mul(t,a0,a0);mpz_mul(t2,a1,a1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("sq_c0",G,E);
        /* sq_c1 = 2*a0*a1 */
        fp2_sq_c1(&o,&ra); val(G,&o);
        mpz_mul(E,a0,a1);mpz_mul_ui(E,E,2);mpz_mod(E,E,P);chk("sq_c1",G,E);
    }
    printf("fp2 iters=%ld FAILS=%d -> %s\n",it,fails,fails==0?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return fails?1:0;
}
