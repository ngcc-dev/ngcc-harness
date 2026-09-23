#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#define N 16
const uint64_t p[N]={~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,~0ULL,0x70FFFFFFFFFFFFFFULL};
extern void fp_mul(uint64_t*,const uint64_t*,const uint64_t*);
extern void fp_sqr(uint64_t*,const uint64_t*);
extern void fp_add(uint64_t*,const uint64_t*,const uint64_t*);
extern void fp_sub(uint64_t*,const uint64_t*,const uint64_t*);
static mpz_t P,R;
static void tl(uint64_t x[N],const mpz_t A){memset(x,0,8*N);mpz_export(x,NULL,-1,8,-1,0,A);}
static void fl(mpz_t A,const uint64_t x[N]){mpz_import(A,N,-1,8,-1,0,x);}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):50000;
    mpz_inits(P,R,NULL); mpz_ui_pow_ui(P,2,1016);mpz_mul_ui(P,P,113);mpz_sub_ui(P,P,1);
    mpz_ui_pow_ui(R,2,64*N);
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,5);
    mpz_t A,B,Am,Bm,E,G;mpz_inits(A,B,Am,Bm,E,G,NULL);
    int fails=0;
    for(it=0;it<iters&&fails<6;it++){
        if(it==0){mpz_set_ui(A,1);mpz_set_ui(B,1);}
        else if(it==1){mpz_sub_ui(A,P,1);mpz_set(B,A);}
        else {mpz_urandomm(A,st,P);mpz_urandomm(B,st,P);}
        mpz_mul(Am,A,R);mpz_mod(Am,Am,P); mpz_mul(Bm,B,R);mpz_mod(Bm,Bm,P);
        uint64_t la[N],lb[N],ld[N]; tl(la,Am);tl(lb,Bm);
        /* mul */ fp_mul(ld,la,lb); fl(G,ld); mpz_mod(G,G,P);
        mpz_mul(E,A,B);mpz_mul(E,E,R);mpz_mod(E,E,P);
        if(mpz_cmp(G,E)){if(fails<3)gmp_fprintf(stderr,"MUL FAIL it=%ld\n g=%Zx\n e=%Zx\n",it,G,E);fails++;}
        /* sqr */ fp_sqr(ld,la); fl(G,ld); mpz_mod(G,G,P);
        mpz_mul(E,A,A);mpz_mul(E,E,R);mpz_mod(E,E,P);
        if(mpz_cmp(G,E)){if(fails<3)gmp_fprintf(stderr,"SQR FAIL it=%ld\n",it);fails++;}
        /* add */ fp_add(ld,la,lb); fl(G,ld); mpz_mod(G,G,P);
        mpz_add(E,Am,Bm);mpz_mod(E,E,P); if(mpz_cmp(G,E)){if(fails<3)fprintf(stderr,"ADD FAIL it=%ld\n",it);fails++;}
        if(ld[N-1]>>63){if(fails<3)fprintf(stderr,"ADD RANGE it=%ld\n",it);fails++;}
        /* sub */ fp_sub(ld,la,lb); fl(G,ld); mpz_mod(G,G,P);
        mpz_sub(E,Am,Bm);mpz_mod(E,E,P); if(mpz_cmp(G,E)){if(fails<3)fprintf(stderr,"SUB FAIL it=%ld\n",it);fails++;}
        if(ld[N-1]>>63){if(fails<3)fprintf(stderr,"SUB RANGE it=%ld\n",it);fails++;}
        /* check output < 2^nbits (partial-reduced invariant): top limb bit 63 must be 0 */
        if(ld[N-1]>>63){if(fails<3)fprintf(stderr,"RANGE FAIL it=%ld top=%llx\n",it,(unsigned long long)ld[N-1]);fails++;}
    }
    printf("asm fp_mul/sqr N=16: iters=%ld FAILS=%d -> %s\n",it,fails,fails==0?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return fails?1:0;
}
