#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "gf1131016.h"

#define NB 128   /* 8*N bytes */
/* modulus symbols referenced by the asm */






static mpz_t P;
static void to_bytes(uint8_t b[NB], const mpz_t A){ memset(b,0,NB); mpz_export(b,NULL,-1,1,-1,0,A); }
static void rep_of(gf1131016*r, const mpz_t A){ uint8_t b[NB]; to_bytes(b,A); if(!gf1131016_decode(r,b)){fprintf(stderr,"decode<p failed\n");exit(2);} }
static void val_of(mpz_t A, const gf1131016*r){ uint8_t b[NB]; gf1131016_encode(b,r); mpz_import(A,NB,-1,1,-1,0,b); }

static int fails=0;
static void chk(const char*nm, const mpz_t got, const mpz_t exp){
    if(mpz_cmp(got,exp)!=0){ if(fails<8){gmp_fprintf(stderr,"FAIL %s\n  got=%Zx\n  exp=%Zx\n",nm,got,exp);} fails++; }
}

int main(int argc,char**argv){
    long iters = argc>1?atol(argv[1]):20000;
    gmp_randstate_t st; gmp_randinit_default(st); gmp_randseed_ui(st,12345);
    mpz_init(P);
    /* P = 9*2^309 - 1 */
    mpz_ui_pow_ui(P,2,1016); mpz_mul_ui(P,P,113); mpz_sub_ui(P,P,1);
    mpz_t A,B,E,G,inv2,inv3; mpz_inits(A,B,E,G,inv2,inv3,NULL);
    mpz_set_ui(inv2,2); mpz_invert(inv2,inv2,P);
    mpz_set_ui(inv3,3); mpz_invert(inv3,inv3,P);

    for(long it=0; it<iters; it++){
        mpz_urandomm(A,st,P); mpz_urandomm(B,st,P);
        gf1131016 rA,rB,rC; rep_of(&rA,A); rep_of(&rB,B);
        /* add */ gf1131016_add(&rC,&rA,&rB); val_of(G,&rC); mpz_add(E,A,B); mpz_mod(E,E,P); chk("add",G,E);
        /* sub */ gf1131016_sub(&rC,&rA,&rB); val_of(G,&rC); mpz_sub(E,A,B); mpz_mod(E,E,P); chk("sub",G,E);
        /* mul */ gf1131016_mul(&rC,&rA,&rB); val_of(G,&rC); mpz_mul(E,A,B); mpz_mod(E,E,P); chk("mul",G,E);
        /* sqr */ gf1131016_square(&rC,&rA); val_of(G,&rC); mpz_mul(E,A,A); mpz_mod(E,E,P); chk("sqr",G,E);
        /* neg */ gf1131016_neg(&rC,&rA); val_of(G,&rC); mpz_sub(E,P,A); mpz_mod(E,E,P); chk("neg",G,E);
        /* half*/ gf1131016_half(&rC,&rA); val_of(G,&rC); mpz_mul(E,A,inv2); mpz_mod(E,E,P); chk("half",G,E);
        /* div3*/ gf1131016_div3(&rC,&rA); val_of(G,&rC); mpz_mul(E,A,inv3); mpz_mod(E,E,P); chk("div3",G,E);
        /* mul_small */ uint32_t n=(uint32_t)mpz_get_ui(B)&0xFFFFF; gf1131016_mul_small(&rC,&rA,n);
            val_of(G,&rC); mpz_mul_ui(E,A,n); mpz_mod(E,E,P); chk("mul_small",G,E);
        /* set_small */ gf1131016_set_small(&rC,n); val_of(G,&rC); mpz_set_ui(E,n); mpz_mod(E,E,P); chk("set_small",G,E);
        /* invert */ if(mpz_sgn(A)){ gf1131016_invert(&rC,&rA); val_of(G,&rC); mpz_invert(E,A,P);
        /* legendre */ { int32_t ls=gf1131016_legendre(&rA); int leg=mpz_legendre(A,P); if(ls!=leg){if(fails<8)gmp_fprintf(stderr,"FAIL legendre got=%d exp=%d A=%Zx\n",ls,leg,A);fails++;} }
        /* sqrt */ { gf1131016 rs; uint32_t ok=gf1131016_sqrt(&rs,&rA); int isq=(mpz_legendre(A,P)>=0);
            if((ok!=0)!=isq){if(fails<8)fprintf(stderr,"FAIL sqrt status it=%ld\n",it);fails++;}
            if(isq){ val_of(G,&rs); mpz_mul(G,G,G); mpz_mod(G,G,P); chk("sqrt^2",G,A); } }
        /* encode/decode roundtrip */ { gf1131016 rd; uint8_t b[NB]; gf1131016_encode(b,&rA); gf1131016_decode(&rd,b);
            val_of(G,&rd); chk("enc/dec",G,A); }
        if(fails>20){fprintf(stderr,"too many fails, abort\n");break;}
    }
    /* decode_reduce: random length */
    for(int it=0;it<2000 && fails<=20;it++){
        int len=(int)(gmp_urandomb_ui(st,8))%200;
        uint8_t buf[256]; for(int i=0;i<len;i++) buf[i]=(uint8_t)gmp_urandomb_ui(st,8);
        gf1131016 rd; gf1131016_decode_reduce(&rd,buf,len); val_of(G,&rd);
        mpz_import(E,len,-1,1,-1,0,buf); mpz_mod(E,E,P); chk("decode_reduce",G,E);
    }
    /* pow: random small-ish exponents */
    for(int it=0;it<500 && fails<=20;it++){
        mpz_urandomm(A,st,P); mpz_t ex; mpz_init(ex); mpz_urandomb(ex,st,313);
        uint64_t e[5]={0,0,0,0,0}; mpz_export(e,NULL,-1,8,-1,0,ex); int nb=(int)mpz_sizeinbase(ex,2);
        gf1131016 rA2,rR; rep_of(&rA2,A); gf1131016_pow(&rR,&rA2,e,nb); val_of(G,&rR);
        mpz_powm(E,A,ex,P); chk("pow",G,E); mpz_clear(ex);
    }
    printf("iters=%ld  FAILS=%d  -> %s\n", iters, fails, fails==0?"ALL PASS \xE2\x9C\x85":"*** FAILURES ***");
    return fails?1:0;
}
