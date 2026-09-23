#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#define N 16
typedef struct { uint64_t re[N], im[N]; } fp2_t;
const uint64_t p[N]={~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,0x70FFFFFFFFFFFFFFULL};
const uint64_t p2[N]={0xFFFFFFFFFFFFFFFEULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,0xE1FFFFFFFFFFFFFFULL};
extern void fp2_mul_c0(uint64_t*,const fp2_t*,const fp2_t*);
extern void fp2_mul_c1(uint64_t*,const fp2_t*,const fp2_t*);
extern void fp2_sq_c0(fp2_t*,const fp2_t*);
extern void fp2_sq_c1(uint64_t*,const fp2_t*);
static mpz_t P,R;
static void tl(uint64_t x[N],const mpz_t A){memset(x,0,8*N);mpz_export(x,NULL,-1,8,-1,0,A);}
static void fl(mpz_t A,const uint64_t x[N]){mpz_import(A,N,-1,8,-1,0,x);}
static void mont(uint64_t out[N], const mpz_t A){mpz_t t;mpz_init(t);mpz_mul(t,A,R);mpz_mod(t,t,P);tl(out,t);mpz_clear(t);}
static int fails=0;
static void chk(const char*n,const mpz_t g,const mpz_t e){if(mpz_cmp(g,e)){if(fails<6)gmp_fprintf(stderr,"FAIL %s\n g=%Zx\n e=%Zx\n",n,g,e);fails++;}}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):30000;
    mpz_inits(P,R,NULL);mpz_ui_pow_ui(P,2,1016);mpz_mul_ui(P,P,113);mpz_sub_ui(P,P,1);mpz_ui_pow_ui(R,2,64*N);
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,11);
    mpz_t a0,a1,b0,b1,E,G,t,t2;mpz_inits(a0,a1,b0,b1,E,G,t,t2,NULL);
    for(it=0;it<iters&&fails<6;it++){
        mpz_urandomm(a0,st,P);mpz_urandomm(a1,st,P);mpz_urandomm(b0,st,P);mpz_urandomm(b1,st,P);
        fp2_t ra,rb; mont(ra.re,a0);mont(ra.im,a1);mont(rb.re,b0);mont(rb.im,b1);
        uint64_t o[N]; fp2_t o2;
        /* c0 = a0b0 - a1b1 */ fp2_mul_c0(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P);
        mpz_mul(t,a0,b0);mpz_mul(t2,a1,b1);mpz_sub(E,t,t2);mpz_mul(E,E,R);mpz_mod(E,E,P);chk("mul_c0",G,E);
        if(o[N-1]>>63){if(fails<6)fprintf(stderr,"c0 range it=%ld\n",it);fails++;}
        /* c1 = a0b1 + a1b0 */ fp2_mul_c1(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P);
        mpz_mul(t,a0,b1);mpz_mul(t2,a1,b0);mpz_add(E,t,t2);mpz_mul(E,E,R);mpz_mod(E,E,P);chk("mul_c1",G,E);
        /* sq c0 = a0^2 - a1^2 */ fp2_sq_c0(&o2,&ra); fl(G,o2.re);mpz_mod(G,G,P);
        mpz_mul(t,a0,a0);mpz_mul(t2,a1,a1);mpz_sub(E,t,t2);mpz_mul(E,E,R);mpz_mod(E,E,P);chk("sq_c0",G,E);
        /* sq c1 = 2a0a1 */ fp2_sq_c1(o,&ra); fl(G,o);mpz_mod(G,G,P);
        mpz_mul(E,a0,a1);mpz_mul_ui(E,E,2);mpz_mul(E,E,R);mpz_mod(E,E,P);chk("sq_c1",G,E);
    }
    printf("asm fp2 N=16: iters=%ld FAILS=%d -> %s\n",it,fails,fails==0?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return fails?1:0;
}
