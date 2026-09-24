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
static mpz_t P,R,Rinv,NB;
static void tl(uint64_t x[N],const mpz_t A){memset(x,0,8*N);mpz_export(x,NULL,-1,8,-1,0,A);}
static void fl(mpz_t A,const uint64_t x[N]){mpz_import(A,N,-1,8,-1,0,x);}
static int fails=0;
static void chk(const char*n,long it,const mpz_t G,const mpz_t E){if(mpz_cmp(G,E)){if(fails<6)gmp_fprintf(stderr,"FAIL %s it=%ld\n",n,it);fails++;}}
static void mm(mpz_t o,const mpz_t x,const mpz_t y){mpz_mul(o,x,y);mpz_mul(o,o,Rinv);mpz_mod(o,o,P);}
static long rng_fail=0;
static void rngchk(const uint64_t o[N],const char*n,long it){if(o[N-1]>>63){if(rng_fail<6)fprintf(stderr,"RANGE %s it=%ld\n",n,it);rng_fail++;}}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):1000000; unsigned seed=argc>2?atoi(argv[2]):1;
    mpz_inits(P,R,Rinv,NB,NULL);mpz_ui_pow_ui(P,2,1016);mpz_mul_ui(P,P,113);mpz_sub_ui(P,P,1);
    mpz_ui_pow_ui(R,2,64*N);mpz_invert(Rinv,R,P);mpz_ui_pow_ui(NB,2,1023);
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,seed);
    mpz_t A0,A1,B0,B1,E,G,t,t2;mpz_inits(A0,A1,B0,B1,E,G,t,t2,NULL);
    for(it=0;it<iters&&fails<6;it++){
        int mode=it%5;
        if(mode==0){mpz_urandomm(A0,st,P);mpz_urandomm(A1,st,P);mpz_urandomm(B0,st,P);mpz_urandomm(B1,st,P);}
        else if(mode==1){mpz_urandomb(A0,st,1023);mpz_urandomb(A1,st,1023);mpz_urandomb(B0,st,1023);mpz_urandomb(B1,st,1023);}
        else if(mode==2){ /* c0 worst case: bb=2p (b1 small), b0 max, a near p */
            mpz_sub_ui(A0,P,1+ (mpz_get_ui(P)&0xfff)); mpz_sub_ui(A1,P,3);
            mpz_sub_ui(B0,NB,1); mpz_set_ui(B1, it & 0x7); }
        else if(mode==3){ mpz_set(A0,P);mpz_add_ui(A1,P,2);mpz_sub_ui(B0,NB,2);mpz_set_ui(B1,0); } /* exactly P, bb=2p */
        else { mpz_urandomb(A0,st,1023);mpz_set(A1,A0);mpz_urandomb(B0,st,1023);mpz_urandomb(B1,st,1023); }
        fp2_t ra,rb; tl(ra.re,A0);tl(ra.im,A1);tl(rb.re,B0);tl(rb.im,B1);
        uint64_t o[N]; fp2_t o2;
        fp2_mul_c0(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P); mm(t,A0,B0);mm(t2,A1,B1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("c0",it,G,E);rngchk(o,"c0",it);
        fp2_mul_c1(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P); mm(t,A0,B1);mm(t2,A1,B0);mpz_add(E,t,t2);mpz_mod(E,E,P);chk("c1",it,G,E);rngchk(o,"c1",it);
        fp2_sq_c0(&o2,&ra); fl(G,o2.re);mpz_mod(G,G,P); mm(t,A0,A0);mm(t2,A1,A1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("sqc0",it,G,E);rngchk(o2.re,"sqc0",it);
        fp2_sq_c1(o,&ra); fl(G,o);mpz_mod(G,G,P); mm(E,A0,A1);mpz_mul_ui(E,E,2);mpz_mod(E,E,P);chk("sqc1",it,G,E);rngchk(o,"sqc1",it);
    }
    printf("seed=%u iters=%ld VALUE_FAILS=%d RANGE_FAILS=%ld -> %s\n",seed,it,fails,rng_fail,(fails==0&&rng_fail==0)?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return (fails||rng_fail)?1:0;
}
