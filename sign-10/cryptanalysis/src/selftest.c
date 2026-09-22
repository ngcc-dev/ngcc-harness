#include "sign.h"
#include <time.h>
int main(int argc,char**argv){
    int n = argc>1? atoi(argv[1]) : 10;
    int m = argc>2? atoi(argv[2]) : 13;
    uint64_t seed = argc>3? strtoull(argv[3],0,10) : 1;
    rng_seed(seed);
    param P=mkparam(n,m);
    printf("Facto-DSA  q=%u n=%d r=%d D=%d m=%d s=%d   pk=%d coeffs\n",Q,P.n,P.r,P.D,P.m,P.s,P.m*NC3(P.r));
    pubkey pk; seckey sk;
    clock_t t0=clock(); keygen(P,&pk,&sk);
    printf("keygen: %.2fs\n",(double)(clock()-t0)/CLOCKS_PER_SEC);
    int ok=0,fail=0,tt=0;
    fe*z=malloc(sizeof(fe)*P.r);
    t0=clock();
    for(int i=0;i<20;i++){
        char msg[64]; int L=snprintf(msg,sizeof msg,"message number %d",i);
        int tr=0;
        if(facto_sign(&sk,&pk,(unsigned char*)msg,L,z,&tr)){ tt+=tr;
            if(facto_verify(&pk,(unsigned char*)msg,L,z)) ok++; else fail++;
        } else fail++;
    }
    printf("sign+verify: %d ok, %d fail, avg trials %.1f, %.2fs total\n",ok,fail,tt/(double)(ok?ok:1),(double)(clock()-t0)/CLOCKS_PER_SEC);
    /* negative control: tampered signature must be rejected */
    char msg[]="message number 0"; facto_sign(&sk,&pk,(unsigned char*)msg,16,z,NULL);
    z[0]=fadd(z[0],1);
    printf("tampered signature rejected: %s\n", facto_verify(&pk,(unsigned char*)msg,16,z)?"NO (BUG)":"yes");
    /* structural check: P vanishes identically on K2 = ker L2 */
    fe*ker; int k=mat_kernel(sk.L2,P.n,P.r,&ker);
    fe*v=malloc(sizeof(fe)*P.r), *out=malloc(sizeof(fe)*P.m);
    int bad=0;
    for(int t=0;t<200;t++){
        for(int i=0;i<P.r;i++) v[i]=0;
        for(int c=0;c<k;c++){ fe a=frand(); for(int i=0;i<P.r;i++) v[i]=fadd(v[i],fmul(a,MAT(ker,k,i,c))); }
        pk_eval(&pk,v,out); for(int j=0;j<P.m;j++) if(out[j]) bad++;
    }
    printf("dim ker L2 = %d (expect n=%d);  P vanishes on 200 random pts of K2: %s\n",k,P.n,bad?"NO (BUG)":"yes");
    return 0;
}
