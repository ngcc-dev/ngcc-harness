/* recover.h - rank-1 element of a space of symmetric matrices, via w w^T membership */
#ifndef RECOVER_H
#define RECOVER_H
#include "tensor.h"
#include "sign.h"
#include "xl.h"

/* Find a rank-1 element of span{A_0..A_{k-1}} (symmetric t x t).
   A rank-1 symmetric matrix is w w^T.  Membership in the space is
   t(t+1)/2 - k linear conditions, each becoming a quadratic form in w. */
static int find_rank1(const fe*A,int k,int t,fe*mu){
    int st=t*(t+1)/2;
    fe*S=mat_alloc(st,k);
    for(int a=0;a<k;a++){ const fe*Aa=&A[(size_t)a*t*t];
        for(int p=0;p<t;p++)for(int qq=p;qq<t;qq++)
            MAT(S,k,iq2(t,p,qq),a) = (p==qq)? MAT(Aa,t,p,p) : fadd(MAT(Aa,t,p,qq),MAT(Aa,t,qq,p)); }
    fe*ST=mat_alloc(k,st);
    for(int a=0;a<k;a++)for(int i=0;i<st;i++) MAT(ST,st,a,i)=MAT(S,k,i,a);
    fe*cond; int nc=mat_kernel(ST,k,st,&cond);
    if(nc==0){ for(int a=0;a<k;a++) mu[a]=0; mu[0]=1;
        free(S);free(ST);free(cond); return (t==1); }
    int ret=0;
    for(int norm=0; norm<t && !ret; norm++){
        int N=t-1;
        spoly*eqs=malloc(sizeof(spoly)*nc);
        for(int c=0;c<nc;c++){
            eqs[c].nterm=0; eqs[c].mo=malloc(sizeof(mono)*(st+1)); eqs[c].co=malloc(sizeof(fe)*(st+1));
            for(int p=0;p<t;p++)for(int qq=p;qq<t;qq++){
                fe cc=MAT(cond,nc,iq2(t,p,qq),c); if(!cc) continue;
                if(p!=qq) cc=fmul(2,cc);               /* monomial basis: off-diagonal counted twice */
                mono mo; memset(&mo,0,sizeof mo);
                int idx[2]={p,qq};
                for(int z=0;z<2;z++){ int v=idx[z]; if(v==norm) continue;
                    mo.e[(v<norm)? v : v-1]++; }
                int f=-1; for(int u=0;u<eqs[c].nterm;u++){ int same=1;
                    for(int i2=0;i2<N;i2++) if(eqs[c].mo[u].e[i2]!=mo.e[i2]){same=0;break;}
                    if(same){f=u;break;} }
                if(f>=0) eqs[c].co[f]=fadd(eqs[c].co[f],cc);
                else { eqs[c].mo[eqs[c].nterm]=mo; eqs[c].co[eqs[c].nterm]=cc; eqs[c].nterm++; } } }
        fe*sols=malloc(sizeof(fe)*(N?N:1)*32);
        int save=XL_EDEG; XL_EDEG=2;
        for(int d=2; d<=6 && !ret; d++){
            int kd2=0; double sc=0; int ns;
            if(N==0){ ns=1; }
            else ns=xl_rational_solutions(eqs,nc,N,d,sols,32,0,&kd2,&sc);
            if(ns<=0) continue;
            for(int z=0; z<ns && !ret; z++){
                fe*w=malloc(sizeof(fe)*t);
                for(int v=0,j=0;v<t;v++) w[v]=(v==norm)?1:sols[(size_t)z*N+(j++)];
                fe*rhs=malloc(sizeof(fe)*st);
                for(int p=0;p<t;p++)for(int qq=p;qq<t;qq++)
                    rhs[iq2(t,p,qq)] = (p==qq)? fmul(w[p],w[p]) : fmul(2,fmul(w[p],w[qq]));
                if(mat_solve(S,st,k,rhs,mu)){
                    fe*M=mat_alloc(t,t);
                    for(int a=0;a<k;a++){ if(!mu[a]) continue; const fe*Aa=&A[(size_t)a*t*t];
                        for(int i2=0;i2<t;i2++)for(int j2=0;j2<t;j2++)
                            MAT(M,t,i2,j2)=fadd(MAT(M,t,i2,j2),fmul(mu[a],MAT(Aa,t,i2,j2))); }
                    if(mat_rank(M,t,t)==1) ret=1;
                    free(M); }
                free(rhs); free(w); }
            if(ns>0 && !ret) break;                    /* solutions found but none rank 1 */
        }
        XL_EDEG=save;
        for(int c=0;c<nc;c++){ free(eqs[c].mo); free(eqs[c].co); } free(eqs); free(sols);
    }
    free(S);free(ST);free(cond); return ret;
}
/* Find u with rank[ A_0 u | A_1 u | ... | A_{k-1} u ] = 1  (A_a are k x k).
   In the triangular basis u is the top coordinate direction and the columns
   A_a u vanish for every a below the top layer, so the rank is 1. */
static int find_common_kernel_dir(const fe*A,int k,fe*uout){
    if(k==1){ uout[0]=1; return 1; }
    int neq=0; for(int i=0;i<k;i++)for(int j=i+1;j<k;j++)for(int a=0;a<k;a++)for(int b=a+1;b<k;b++) neq++;
    for(int norm=0;norm<k;norm++){
        int N=k-1;
        spoly*eqs=malloc(sizeof(spoly)*neq); int ne=0;
        for(int i=0;i<k;i++)for(int j=i+1;j<k;j++)for(int a=0;a<k;a++)for(int b=a+1;b<k;b++){
            spoly f; f.nterm=0; f.mo=malloc(sizeof(mono)*(k*k+2)); f.co=malloc(sizeof(fe)*(k*k+2));
            /* (A_a u)_i (A_b u)_j - (A_b u)_i (A_a u)_j */
            for(int p=0;p<k;p++)for(int q=0;q<k;q++){
                fe c1=fmul(MAT(&A[(size_t)a*k*k],k,i,p),MAT(&A[(size_t)b*k*k],k,j,q));
                fe c2=fmul(MAT(&A[(size_t)b*k*k],k,i,p),MAT(&A[(size_t)a*k*k],k,j,q));
                fe c=fsub(c1,c2); if(!c) continue;
                mono mo; memset(&mo,0,sizeof mo); int idx[2]={p,q};
                for(int z=0;z<2;z++){ int v=idx[z]; if(v==norm) continue; mo.e[(v<norm)?v:v-1]++; }
                int fo=-1; for(int t=0;t<f.nterm;t++){ int same=1;
                    for(int i2=0;i2<N;i2++) if(f.mo[t].e[i2]!=mo.e[i2]){same=0;break;}
                    if(same){fo=t;break;} }
                if(fo>=0) f.co[fo]=fadd(f.co[fo],c);
                else { f.mo[f.nterm]=mo; f.co[f.nterm]=c; f.nterm++; } }
            if(f.nterm) eqs[ne++]=f; else { free(f.mo); free(f.co); } }
        fe*sols=malloc(sizeof(fe)*(N?N:1)*32); int ret=0;
        int save=XL_EDEG; XL_EDEG=2;
        for(int d=2; d<=5 && !ret; d++){
            int kd2=0; double sc=0;
            int ns=xl_rational_solutions(eqs,ne,N,d,sols,32,0,&kd2,&sc);
            if(ns<=0) continue;
            for(int z=0; z<ns && !ret; z++){
                fe*u=malloc(sizeof(fe)*k);
                for(int v=0,j2=0;v<k;v++) u[v]=(v==norm)?1:sols[(size_t)z*N+(j2++)];
                fe*Phi=mat_alloc(k,k);
                for(int a=0;a<k;a++)for(int i=0;i<k;i++){ uint64_t acc=0;
                    for(int p=0;p<k;p++) acc+=(uint64_t)MAT(&A[(size_t)a*k*k],k,i,p)*u[p]%Q;
                    MAT(Phi,k,i,a)=(fe)(acc%Q); }
                if(mat_rank(Phi,k,k)==1){ memcpy(uout,u,sizeof(fe)*k); ret=1; }
                free(Phi); free(u); }
            if(ns>0 && !ret) break;
        }
        XL_EDEG=save;
        for(int c=0;c<ne;c++){ free(eqs[c].mo); free(eqs[c].co); } free(eqs); free(sols);
        if(ret) return 1;
    }
    return 0;
}
#endif
