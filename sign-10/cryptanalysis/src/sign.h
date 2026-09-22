/* sign.h - Facto-DSA signing (Alg. 10), triangular inversion (Alg. 7), verify (Alg. 11) */
#ifndef SIGN_H
#define SIGN_H
#include "facto.h"

/* HashToField: random-oracle stand-in over (label || pkh || M) */
static void hash_to_field(const char*label,uint64_t pkh,const unsigned char*M,size_t len,fe*h,int m){
    uint64_t st=0xcbf29ce484222325ull ^ pkh;
    for(const char*p=label;*p;p++){ st^=(unsigned char)*p; st*=0x100000001b3ull; }
    for(size_t i=0;i<len;i++){ st^=M[i]; st*=0x100000001b3ull; }
    uint64_t save=RNG_STATE; RNG_STATE=st|1;
    for(int i=0;i<m;i++){ fe w; do{ w=(fe)(rng64()&0xffff); }while(w>=Q); h[i]=w; }
    RNG_STATE=save;
}
static uint64_t pk_digest(const pubkey*pk){
    int nc=NC3(pk->P.r); uint64_t st=0xcbf29ce484222325ull;
    for(int j=0;j<pk->P.m;j++)for(int i=0;i<nc;i++){ st^=MAT(pk->C,nc,j,i); st*=0x100000001b3ull; }
    return st;
}

/* evaluate homogeneous quadratic map R at Y: out_i = Y^T RA_i Y */
static void eval_quadmap(const fe*A,int n,const fe*Y,fe*out){
    for(int i=0;i<n;i++){ const fe*M=&A[(size_t)i*n*n]; uint64_t s=0;
        for(int a=0;a<n;a++){ if(!Y[a]) continue; uint64_t t=0;
            for(int b=0;b<n;b++) t+=(uint64_t)MAT(M,n,a,b)*Y[b]%Q;
            s+=(t%Q)*Y[a]%Q; }
        out[i]=(fe)(s%Q); }
}
/* Alg. 7 TriangularInvert, depth-first over both square roots */
static int tri_invert_rec(const fe*QA,int n,const fe*W,fe*X,int i){
    if(i==n) return 1;
    const fe*A=&QA[(size_t)i*n*n];
    fe a=MAT(A,n,i,i);                                   /* a_i */
    fe beta=0;                                           /* b_i(X_<i) */
    for(int j=0;j<i;j++) beta=fadd(beta,fmul(fadd(MAT(A,n,i,j),MAT(A,n,j,i)),X[j]));
    fe gamma=0;                                          /* c_i(X_<i) */
    for(int u=0;u<i;u++){ if(!X[u]) continue; uint64_t t=0;
        for(int v=0;v<i;v++) t+=(uint64_t)MAT(A,n,u,v)*X[v]%Q;
        gamma=fadd(gamma,fmul((fe)(t%Q),X[u])); }
    fe disc=fsub(fmul(beta,beta), fmul(fmul(4%Q,a), fsub(gamma,W[i])));
    fe v; if(!fsqrt(disc,&v)) return 0;
    fe i2a=finv(fmul(2,a));
    fe roots[2]; roots[0]=fmul(fsub(v,beta),i2a); roots[1]=fmul(fsub(fneg(v),beta),i2a);
    int nr = (roots[0]==roots[1])?1:2;
    for(int k=0;k<nr;k++){ X[i]=roots[k]; if(tri_invert_rec(QA,n,W,X,i+1)) return 1; }
    return 0;
}
static int tri_invert(const fe*QA,int n,const fe*W,fe*X){ memset(X,0,sizeof(fe)*n); return tri_invert_rec(QA,n,W,X,0); }

/* BoundedFactorSplits (Alg. 5) via subset-sum DP over irreducible factor copies */
typedef struct { poly A0, Y0; } split;
static int bounded_splits(const poly*f,int n,split*out,int maxout){
    int d=f->d; if(d<0) return 0;
    int lo = d-n+1; if(lo<0) lo=0; int hi=n-1; if(hi>d) hi=d;
    if(lo>hi) return 0;
    factz F=factor_poly(f);
    /* expand to a list of factor copies */
    int nc=0; for(int i=0;i<F.cnt;i++) nc+=F.mult[i];
    int *fid=malloc(sizeof(int)*(nc>0?nc:1)); int p=0;
    for(int i=0;i<F.cnt;i++) for(int e=0;e<F.mult[i];e++) fid[p++]=i;
    /* DP: reach[deg] = index of a copy used last, par[deg] = previous degree */
    int *use=malloc(sizeof(int)*(d+1)), *par=malloc(sizeof(int)*(d+1));
    char *ok=calloc(d+1,1); ok[0]=1; use[0]=-1; par[0]=-1;
    for(int c=0;c<nc;c++){ int dg=F.f[fid[c]].d;
        for(int t=d-dg;t>=0;t--) if(ok[t] && !ok[t+dg]){ ok[t+dg]=1; use[t+dg]=c; par[t+dg]=t; } }
    int cnt=0;
    for(int a=lo;a<=hi && cnt<maxout;a++){
        if(!ok[a]) continue;
        char*used=calloc(nc>0?nc:1,1);
        int t=a; while(t>0){ used[use[t]]=1; t=par[t]; }
        poly A0=pnew(0); A0.c[0]=f->c[f->d];             /* leading coeff on A0 */
        poly Y0=pnew(0); Y0.c[0]=1;
        for(int c=0;c<nc;c++){ poly*g=&F.f[fid[c]];
            if(used[c]){ poly t2=pmul(&A0,g); pfree(&A0); A0=t2; }
            else       { poly t2=pmul(&Y0,g); pfree(&Y0); Y0=t2; } }
        free(used);
        out[cnt].A0=A0; out[cnt].Y0=Y0; cnt++;
    }
    free(fid);free(use);free(par);free(ok); factz_free(&F);
    return cnt;
}

#define MAX_SCALES 64
/* Alg. 10 Sign. returns 1 and z (r), else 0 */
static int facto_sign(const seckey*sk,const pubkey*pk,const unsigned char*M,size_t len,fe*z,int*trials_out){
    param P=sk->P; int n=P.n,m=P.m,D=P.D,r=P.r,s=P.s;
    fe*h=malloc(sizeof(fe)*m); hash_to_field("FDSA-H",pk_digest(pk),M,len,h,m);
    fe*hc=malloc(sizeof(fe)*D), *y=malloc(sizeof(fe)*D);
    fe*A=malloc(sizeof(fe)*n), *Y=malloc(sizeof(fe)*n), *W=malloc(sizeof(fe)*n);
    fe*RY=malloc(sizeof(fe)*n), *X=malloc(sizeof(fe)*n), *XY=malloc(sizeof(fe)*r), *chk=malloc(sizeof(fe)*m);
    split sp[64];
    for(int trial=0; trial<P.Rmax; trial++){
        for(int i=0;i<m;i++) hc[i]=h[i];
        for(int i=0;i<s;i++) hc[m+i]=frand();
        for(int i=0;i<D;i++){ uint64_t t=0; for(int k=0;k<D;k++) t+=(uint64_t)MAT(sk->Uinv,D,i,k)*hc[k]%Q; y[i]=(fe)(t%Q); }
        poly f=pnew(D-1); for(int i=0;i<D;i++) f.c[i]=y[i]; pnorm(&f);
        if(f.d<0){ pfree(&f); continue; }
        int ns=bounded_splits(&f,n,sp,64);
        for(int k=0;k<ns;k++){
            for(int sc=0; sc<MAX_SCALES; sc++){
                fe lam = sc? frand_nz() : 1, ilam=finv(lam);
                for(int i=0;i<n;i++){ A[i]= (i<=sp[k].A0.d)? fmul(lam,sp[k].A0.c[i]) : 0;
                                      Y[i]= (i<=sp[k].Y0.d)? fmul(ilam,sp[k].Y0.c[i]) : 0; }
                eval_quadmap(sk->RA,n,Y,RY);
                for(int i=0;i<n;i++) W[i]=fsub(A[i],RY[i]);
                if(!tri_invert(sk->QA,n,W,X)) continue;
                for(int i=0;i<n;i++){ XY[i]=X[i]; XY[n+i]=Y[i]; }
                for(int i=0;i<r;i++){ uint64_t t=0; for(int j=0;j<r;j++) t+=(uint64_t)MAT(sk->Sinv,r,i,j)*XY[j]%Q; z[i]=(fe)(t%Q); }
                pk_eval(pk,z,chk);
                int good=1; for(int i=0;i<m;i++) if(chk[i]!=h[i]) good=0;
                if(good){ for(int i=0;i<ns;i++){ pfree(&sp[i].A0); pfree(&sp[i].Y0);} pfree(&f);
                          free(h);free(hc);free(y);free(A);free(Y);free(W);free(RY);free(X);free(XY);free(chk);
                          if(trials_out)*trials_out=trial+1; return 1; }
            }
        }
        for(int i=0;i<ns;i++){ pfree(&sp[i].A0); pfree(&sp[i].Y0); }
        pfree(&f);
    }
    free(h);free(hc);free(y);free(A);free(Y);free(W);free(RY);free(X);free(XY);free(chk);
    return 0;
}
/* Alg. 11 Verify */
static int facto_verify(const pubkey*pk,const unsigned char*M,size_t len,const fe*z){
    int m=pk->P.m; fe*h=malloc(sizeof(fe)*m), *w=malloc(sizeof(fe)*m);
    hash_to_field("FDSA-H",pk_digest(pk),M,len,h,m);
    for(int i=0;i<pk->P.r;i++) if(z[i]>=Q){ free(h);free(w); return 0; }
    pk_eval(pk,z,w);
    int ok=1; for(int i=0;i<m;i++) if(w[i]!=h[i]) ok=0;
    free(h);free(w); return ok;
}
#endif
