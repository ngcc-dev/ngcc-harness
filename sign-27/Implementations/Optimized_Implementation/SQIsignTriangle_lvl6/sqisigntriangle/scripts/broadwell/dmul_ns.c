#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#define N 16
const uint64_t p[N]={~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,0x70FFFFFFFFFFFFFFULL};
const uint64_t p2[N]={0xFFFFFFFFFFFFFFFEULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,0xE1FFFFFFFFFFFFFFULL};
extern void fp_mul(uint64_t*,const uint64_t*,const uint64_t*);
extern void fp_sqr(uint64_t*,const uint64_t*);
static mpz_t P,R,Rinv;
static void tl(uint64_t x[N],const mpz_t A){memset(x,0,8*N);mpz_export(x,NULL,-1,8,-1,0,A);}
static void fl(mpz_t A,const uint64_t x[N]){mpz_import(A,N,-1,8,-1,0,x);}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):500000; unsigned seed=argc>2?atoi(argv[2]):1;
    mpz_inits(P,R,Rinv,NULL);mpz_ui_pow_ui(P,2,1016);mpz_mul_ui(P,P,113);mpz_sub_ui(P,P,1);
    mpz_ui_pow_ui(R,2,64*N);mpz_invert(Rinv,R,P);
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,seed);
    mpz_t A,B,E,G;mpz_inits(A,B,E,G,NULL); int fails=0,rng=0;
    uint64_t a[N],b[N],r[N];
    for(it=0;it<iters&&fails<6;it++){
        int m=it%3;
        if(m==0){mpz_urandomm(A,st,P);mpz_urandomm(B,st,P);}
        else if(m==1){mpz_urandomb(A,st,1023);mpz_urandomb(B,st,1023);}      /* partial reduced */
        else {mpz_sub_ui(A,P,1);mpz_set(B,A);}                                /* near p, squaring */
        tl(a,A);tl(b,B);
        fp_mul(r,a,b); fl(G,r);mpz_mod(G,G,P);
        mpz_mul(E,A,B);mpz_mul(E,E,Rinv);mpz_mod(E,E,P);
        if(mpz_cmp(G,E)){if(fails<6)gmp_fprintf(stderr,"MUL FAIL it=%ld\n g=%Zx\n e=%Zx\n",it,G,E);fails++;}
        if(r[N-1]>>63){if(rng<6)fprintf(stderr,"MUL RANGE it=%ld\n",it);rng++;}
        fp_sqr(r,a); fl(G,r);mpz_mod(G,G,P);
        mpz_mul(E,A,A);mpz_mul(E,E,Rinv);mpz_mod(E,E,P);
        if(mpz_cmp(G,E)){if(fails<6)gmp_fprintf(stderr,"SQR FAIL it=%ld\n",it);fails++;}
        if(r[N-1]>>63){if(rng<6)fprintf(stderr,"SQR RANGE it=%ld\n",it);rng++;}
    }
    printf("noshift fp_mul/sqr: seed=%u iters=%ld VAL_FAILS=%d RANGE_FAILS=%d -> %s\n",seed,it,fails,rng,(fails==0&&rng==0)?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return (fails||rng)?1:0;
}
