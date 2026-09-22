/* attack1.c - Stage 1+2: recover K2 = ker L2 from the public key alone */
#include "sign.h"
#include "xl.h"
#include <time.h>

/* restrict public cubics to L = colspace(B), B is r x (n+1); dehomogenise u_N=1 */
static void restrict_system(const pubkey*pk,const fe*B,int NL,spoly*eqs){
    int r=pk->P.r,m=pk->P.m,nc=NC3(r); int N=NL-1;
    c3_init(r);
    int ncl=NC3(NL);
    fe*acc=mat_alloc(m,ncl);
    int *tab=malloc(sizeof(int)*NL*NL*NL);
    { int idx=0; for(int a=0;a<NL;a++)for(int b=a;b<NL;b++)for(int c=b;c<NL;c++) tab[(a*NL+b)*NL+c]=idx++;
      for(int a=0;a<NL;a++)for(int b=0;b<NL;b++)for(int c=0;c<NL;c++){ int x=a,y=b,z=c,t;
        if(x>y){t=x;x=y;y=t;} if(y>z){t=y;y=z;z=t;} if(x>y){t=x;x=y;y=t;}
        tab[(a*NL+b)*NL+c]=tab[(x*NL+y)*NL+z]; } }
    for(int a=0;a<r;a++)for(int b=a;b<r;b++)for(int c=b;c<r;c++){
        int id=ic3(r,a,b,c);
        int any=0; for(int j=0;j<m;j++) if(MAT(pk->C,nc,j,id)){any=1;break;}
        if(!any) continue;
        for(int i=0;i<NL;i++){ fe bi=MAT(B,NL,a,i); if(!bi) continue;
          for(int jj=0;jj<NL;jj++){ fe bj=MAT(B,NL,b,jj); if(!bj) continue; fe bij=fmul(bi,bj);
            for(int k=0;k<NL;k++){ fe bk=MAT(B,NL,c,k); if(!bk) continue;
                fe w=fmul(bij,bk); int tid=tab[(i*NL+jj)*NL+k];
                for(int j=0;j<m;j++){ fe cc=MAT(pk->C,nc,j,id); if(cc) MAT(acc,ncl,j,tid)=fadd(MAT(acc,ncl,j,tid),fmul(cc,w)); } } } }
    }
    /* dehomogenise: variable N is set to 1 */
    for(int j=0;j<m;j++){
        eqs[j].nterm=0; eqs[j].mo=malloc(sizeof(mono)*ncl); eqs[j].co=malloc(sizeof(fe)*ncl);
        for(int a=0;a<NL;a++)for(int b=a;b<NL;b++)for(int c=b;c<NL;c++){
            fe cc=MAT(acc,ncl,j,tab[(a*NL+b)*NL+c]); if(!cc) continue;
            mono mo; memset(&mo,0,sizeof mo);
            if(a<N) mo.e[a]++; if(b<N) mo.e[b]++; if(c<N) mo.e[c]++;
            /* merge duplicates */
            int found=-1; for(int t=0;t<eqs[j].nterm;t++){ int same=1; for(int i2=0;i2<N;i2++) if(eqs[j].mo[t].e[i2]!=mo.e[i2]){same=0;break;} if(same){found=t;break;} }
            if(found>=0) eqs[j].co[found]=fadd(eqs[j].co[found],cc);
            else { eqs[j].mo[eqs[j].nterm]=mo; eqs[j].co[eqs[j].nterm]=cc; eqs[j].nterm++; }
        }
    }
    free(acc); free(tab);
}
/* Jacobian of P at v : m x r */
static void jacobian(const pubkey*pk,const fe*v,fe*J){
    int r=pk->P.r,m=pk->P.m,nc=NC3(r);
    memset(J,0,sizeof(fe)*m*r);
    for(int a=0;a<r;a++)for(int b=a;b<r;b++)for(int c=b;c<r;c++){
        int id=ic3(r,a,b,c);
        fe vab=fmul(v[a],v[b]), vac=fmul(v[a],v[c]), vbc=fmul(v[b],v[c]);
        for(int j=0;j<m;j++){ fe C=MAT(pk->C,nc,j,id); if(!C) continue;
            MAT(J,r,j,a)=fadd(MAT(J,r,j,a),fmul(C,vbc));
            MAT(J,r,j,b)=fadd(MAT(J,r,j,b),fmul(C,vac));
            MAT(J,r,j,c)=fadd(MAT(J,r,j,c),fmul(C,vab)); }
    }
}

/* does P vanish identically on span(Kb) ? (Kb is r x k) */
static int vanishes_on(const pubkey*pk,const fe*Kb,int k){
    int r=pk->P.r,m=pk->P.m;
    fe*v=malloc(sizeof(fe)*r),*o=malloc(sizeof(fe)*m); int ok=1;
    for(int t=0;t<12 && ok;t++){
        for(int i=0;i<r;i++) v[i]=0;
        for(int c=0;c<k;c++){ fe a=frand(); for(int i=0;i<r;i++) v[i]=fadd(v[i],fmul(a,MAT(Kb,k,i,c))); }
        pk_eval(pk,v,o); for(int j=0;j<m;j++) if(o[j]) ok=0;
    }
    free(v);free(o); return ok;
}
int main(int argc,char**argv){
    int n=argc>1?atoi(argv[1]):6, m=argc>2?atoi(argv[2]):9;
    uint64_t seed=argc>3?strtoull(argv[3],0,10):1;
    int dmax=argc>4?atoi(argv[4]):12;
    rng_seed(seed);
    param P=mkparam(n,m);
    printf("=== Facto-DSA  n=%d r=%d D=%d m=%d s=%d  (seed %llu) ===\n",P.n,P.r,P.D,P.m,P.s,(unsigned long long)seed);
    pubkey pk; seckey sk; keygen(P,&pk,&sk);
    int NL=n+1,N=n;
    fe*B=mat_alloc(P.r,NL); mat_rand(B,P.r,NL);
    spoly*eqs=malloc(sizeof(spoly)*P.m);
    restrict_system(&pk,B,NL,eqs);
    printf("restricted to random L (dim %d): %d cubics in %d affine variables\n",NL,P.m,N);
    fe*sols=malloc(sizeof(fe)*N*64); int nsol=0,d,kd=0; double secs=0,tot=0;
    for(d=3;d<=dmax;d++){
        nsol=xl_rational_solutions(eqs,P.m,N,d,sols,64,1,&kd,&secs); tot+=secs;
        if(nsol>=1) break;
        if(nsol==-2){ printf("memory budget exceeded at d=%d\n",d); return 1; }
    }
    if(nsol<1){ printf("no rational solution found up to d=%d\n",dmax); return 1; }
    printf("*** solving degree = %d, quotient dim = %d, %d rational solutions, %.1fs ***\n",
           d,kd,nsol,tot);
    if (kd == (1 << n) + 1)
        printf("    quotient dimension matches 2^n+1 = %d\n", (1 << n) + 1);
    else
        printf("    quotient dimension differs from 2^n+1 = %d; recovery does not rely on that count\n",
               (1 << n) + 1);
    fe*J=mat_alloc(P.m,P.r); fe*v=malloc(sizeof(fe)*P.r); fe*u=malloc(sizeof(fe)*NL);
    int found=-1; fe*K2=NULL; int k2dim=0;
    for(int t=0;t<nsol;t++){
        for(int i=0;i<N;i++) u[i]=sols[(size_t)t*N+i]; u[N]=1;
        for(int i=0;i<P.r;i++){ uint64_t a=0; for(int k=0;k<NL;k++) a+=(uint64_t)MAT(B,NL,i,k)*u[k]%Q; v[i]=(fe)(a%Q); }
        jacobian(&pk,v,J);
        fe*Kb; int kdim=mat_kernel(J,P.m,P.r,&Kb);
        int van=(kdim>0)?vanishes_on(&pk,Kb,kdim):0;
        /* ground truth label */
        int inK2=1; for(int i=0;i<n;i++){ uint64_t a=0; for(int k=0;k<P.r;k++) a+=(uint64_t)MAT(sk.L2,P.r,i,k)*v[k]%Q; if(a%Q) inK2=0; }
        printf("  candidate %d: rank J(v)=%d  dim ker J(v)=%d  P|ker == 0 ? %-3s   [truth: %s]\n",
               t,P.m-0,kdim,van?"yes":"no", inK2?"in K2":"on W");
        if(van && found<0){ found=t; K2=Kb; k2dim=kdim; } else free(Kb);
    }
    if(found<0){ printf("no candidate passed the K2 test\n"); return 1; }
    int ok=(k2dim==n);
    for(int c=0;c<k2dim;c++) for(int i=0;i<n;i++){ uint64_t a=0;
        for(int k=0;k<P.r;k++) a+=(uint64_t)MAT(sk.L2,P.r,i,k)*MAT(K2,k2dim,k,c)%Q; if(a%Q) ok=0; }
    printf("*** K2 RECOVERED: dim=%d (n=%d), equals ker L2 : %s ***\n",k2dim,n,ok?"YES":"NO");
    return ok?0:1;
}
