/* solve.h - extract F_q-rational solutions from a Macaulay kernel (eigenvalue / FGLM style) */
#ifndef SOLVE_H
#define SOLVE_H
#include "fq.h"
#include "facto.h"

/* charpoly of n x n matrix via Hessenberg reduction, returns poly of degree n */
static poly charpoly(const fe*Min,int n){
    fe*H=mat_alloc(n,n); memcpy(H,Min,(size_t)n*n*sizeof(fe));
    for(int k=1;k<n-1;k++){
        int piv=-1; for(int i=k;i<n;i++) if(MAT(H,n,i,k-1)){piv=i;break;}
        if(piv<0) continue;
        if(piv!=k){ for(int j=0;j<n;j++){fe t=MAT(H,n,k,j);MAT(H,n,k,j)=MAT(H,n,piv,j);MAT(H,n,piv,j)=t;}
                    for(int i=0;i<n;i++){fe t=MAT(H,n,i,k);MAT(H,n,i,k)=MAT(H,n,i,piv);MAT(H,n,i,piv)=t;} }
        fe inv=finv(MAT(H,n,k,k-1));
        for(int i=k+1;i<n;i++){ fe f=fmul(MAT(H,n,i,k-1),inv); if(!f) continue;
            for(int j=0;j<n;j++) MAT(H,n,i,j)=fsub(MAT(H,n,i,j),fmul(f,MAT(H,n,k,j)));
            for(int j=0;j<n;j++) MAT(H,n,j,k)=fadd(MAT(H,n,j,k),fmul(f,MAT(H,n,j,i))); }
    }
    poly*p=malloc(sizeof(poly)*(n+1));
    p[0]=pnew(0); p[0].c[0]=1;
    for(int k=1;k<=n;k++){
        poly xs=pnew(k);                                   /* (x - H[k-1][k-1]) * p[k-1] */
        for(int i=0;i<=p[k-1].d;i++){ xs.c[i+1]=fadd(xs.c[i+1],p[k-1].c[i]);
            xs.c[i]=fsub(xs.c[i],fmul(MAT(H,n,k-1,k-1),p[k-1].c[i])); }
        pnorm(&xs);
        fe t=1;
        for(int i=1;i<=k-1;i++){ t=fmul(t,MAT(H,n,k-i,k-i-1)); if(!t) break;
            fe f=fmul(t,MAT(H,n,k-1-i,k-1)); if(!f) continue;
            for(int j2=0;j2<=p[k-1-i].d;j2++) xs.c[j2]=fsub(xs.c[j2],fmul(f,p[k-1-i].c[j2])); }
        pnorm(&xs); p[k]=xs;
    }
    poly res=p[n]; for(int i=0;i<n;i++) pfree(&p[i]); free(p); free(H);
    return res;
}
/* all roots of f in F_q */
static int poly_roots(const poly*f,fe*roots,int maxr){
    poly x=pnew(1); x.c[1]=1;
    poly xq=ppowmod(&x,Q,f);
    { poly t=pnew(xq.d>1?xq.d:1);                 /* t <- xq - x, keeping all terms */
      for(int i=0;i<=xq.d;i++) t.c[i]=xq.c[i];
      t.c[1]=fsub(t.c[1],1); pnorm(&t); pfree(&xq); xq=t; }
    poly g=pgcd(f,&xq); pfree(&xq); pfree(&x);
    int cnt=0;
    if(g.d>=1){ factz F; F.f=NULL;F.mult=NULL;F.cnt=0; poly gc=pcopy(&g); edf(gc,1,&F,1);
        for(int i=0;i<F.cnt && cnt<maxr;i++) if(F.f[i].d==1) roots[cnt++]=fneg(fmul(F.f[i].c[0],finv(F.f[i].c[1])));
        factz_free(&F); }
    pfree(&g); return cnt;
}
#endif
