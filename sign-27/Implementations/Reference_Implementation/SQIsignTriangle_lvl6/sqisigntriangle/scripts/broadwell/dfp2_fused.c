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
static mpz_t P,R,Rinv;
static void tl(uint64_t x[N],const mpz_t A){memset(x,0,8*N);mpz_export(x,NULL,-1,8,-1,0,A);}
static void fl(mpz_t A,const uint64_t x[N]){mpz_import(A,N,-1,8,-1,0,x);}
static int fails=0;
static void chk(const char*n,long it,const mpz_t G,const mpz_t E){if(mpz_cmp(G,E)){if(fails<8)gmp_fprintf(stderr,"FAIL %s it=%ld\n g=%Zx\n e=%Zx\n",n,it,G,E);fails++;}}
/* expected montgomery: (x*y)*Rinv mod P (for raw limb values x,y) */
static void mm(mpz_t out,const mpz_t x,const mpz_t y){mpz_mul(out,x,y);mpz_mul(out,out,Rinv);mpz_mod(out,out,P);}
int main(int argc,char**argv){
    long it,iters=argc>1?atol(argv[1]):40000;
    mpz_inits(P,R,Rinv,NULL);mpz_ui_pow_ui(P,2,1016);mpz_mul_ui(P,P,113);mpz_sub_ui(P,P,1);
    mpz_ui_pow_ui(R,2,64*N);mpz_invert(Rinv,R,P);
    gmp_randstate_t st;gmp_randinit_default(st);gmp_randseed_ui(st,2026);
    mpz_t RA0,RA1,RB0,RB1,E,G,t,t2,NB; mpz_inits(RA0,RA1,RB0,RB1,E,G,t,t2,NB,NULL);
    mpz_ui_pow_ui(NB,2,1023);                  /* 2^nbits = partial-reduce bound */
    for(it=0;it<iters&&fails<8;it++){
        int mode=it%4;
        /* mode 0: all <P ; 1: all partial-reduced <2^1023 ; 2: =P or P+1 (force norm) ; 3: a0==a1 */
        if(mode==0){mpz_urandomm(RA0,st,P);mpz_urandomm(RA1,st,P);mpz_urandomm(RB0,st,P);mpz_urandomm(RB1,st,P);}
        else if(mode==1){mpz_urandomb(RA0,st,1023);mpz_urandomb(RA1,st,1023);mpz_urandomb(RB0,st,1023);mpz_urandomb(RB1,st,1023);}
        else if(mode==2){
            mpz_set(RA0,P);                                   /* exactly P -> must normalize to 0 */
            mpz_add_ui(RA1,P,1);                              /* P+1 */
            mpz_sub_ui(RB0,NB,1);                             /* 2^1023-1 (max partial) */
            mpz_urandomm(RB1,st,P);
        } else { mpz_urandomb(RA0,st,1023);mpz_set(RA1,RA0); mpz_urandomb(RB0,st,1023);mpz_set(RB1,RB0); }
        fp2_t ra,rb; tl(ra.re,RA0);tl(ra.im,RA1);tl(rb.re,RB0);tl(rb.im,RB1);
        uint64_t o[N]; fp2_t o2;
        /* c0 = a0*b0 - a1*b1 (montgomery) */
        fp2_mul_c0(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P);
        mm(t,RA0,RB0);mm(t2,RA1,RB1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("mul_c0",it,G,E);
        if(o[N-1]>>63){if(fails<8)fprintf(stderr,"RANGE c0 it=%ld mode=%d\n",it,mode);fails++;}
        /* c1 = a0*b1 + a1*b0 */
        fp2_mul_c1(o,&ra,&rb); fl(G,o);mpz_mod(G,G,P);
        mm(t,RA0,RB1);mm(t2,RA1,RB0);mpz_add(E,t,t2);mpz_mod(E,E,P);chk("mul_c1",it,G,E);
        if(o[N-1]>>63){if(fails<8)fprintf(stderr,"RANGE c1 it=%ld mode=%d\n",it,mode);fails++;}
        /* sq_c0 = a0^2 - a1^2 ; sq_c1 = 2 a0 a1 */
        fp2_sq_c0(&o2,&ra); fl(G,o2.re);mpz_mod(G,G,P);
        mm(t,RA0,RA0);mm(t2,RA1,RA1);mpz_sub(E,t,t2);mpz_mod(E,E,P);chk("sq_c0",it,G,E);
        if(o2.re[N-1]>>63){if(fails<8)fprintf(stderr,"RANGE sqc0 it=%ld\n",it);fails++;}
        fp2_sq_c1(o,&ra); fl(G,o);mpz_mod(G,G,P);
        mm(E,RA0,RA1);mpz_mul_ui(E,E,2);mpz_mod(E,E,P);chk("sq_c1",it,G,E);
        if(o[N-1]>>63){if(fails<8)fprintf(stderr,"RANGE sqc1 it=%ld\n",it);fails++;}
        /* aliasing: fp2_sqr in place — sq_c1 writes &x->im while reading x */
        if(mode==1){
            fp2_t x; tl(x.re,RA0);tl(x.im,RA1);
            fp2_sq_c1((uint64_t*)&x.im,&x); fl(G,x.im);mpz_mod(G,G,P);
            mm(E,RA0,RA1);mpz_mul_ui(E,E,2);mpz_mod(E,E,P);chk("sq_c1_alias",it,G,E);
        }
    }
    printf("FUSED fp2 N=16: iters=%ld FAILS=%d -> %s\n",it,fails,fails==0?"ALL PASS \xE2\x9C\x85":"*** FAIL ***");
    return fails?1:0;
}
