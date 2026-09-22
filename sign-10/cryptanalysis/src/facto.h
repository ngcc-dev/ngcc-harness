/* facto.h - Facto-DSA (sign-10) per spec Algorithms 1-11 */
#ifndef FACTO_H
#define FACTO_H
#include "fq.h"

/* ---------------- monomial indexing ---------------- */
static int NQ2(int r){ return r*(r+1)/2; }
static int NC3(int r){ return r*(r+1)*(r+2)/6; }
/* quadratic monomial index for a<=b  (lex on (a,b)) */
static inline int iq2(int r,int a,int b){ if(a>b){int t=a;a=b;b=t;} return a*r - a*(a-1)/2 + (b-a); }
/* cubic monomial index for a<=b<=c, lexicographic - built table */
static int *C3TAB=NULL; static int C3R=-1;
static void c3_init(int r){
    if(C3R==r) return; free(C3TAB); C3R=r;
    C3TAB=malloc(sizeof(int)*r*r*r);
    int idx=0;
    for(int a=0;a<r;a++)for(int b=a;b<r;b++)for(int c=b;c<r;c++) C3TAB[(a*r+b)*r+c]=idx++;
    /* fill permutations */
    for(int a=0;a<r;a++)for(int b=0;b<r;b++)for(int c=0;c<r;c++){
        int x=a,y=b,z=c,t;
        if(x>y){t=x;x=y;y=t;} if(y>z){t=y;y=z;z=t;} if(x>y){t=x;x=y;y=t;}
        C3TAB[(a*r+b)*r+c]=C3TAB[(x*r+y)*r+z];
    }
}
static inline int ic3(int r,int a,int b,int c){ return C3TAB[(a*r+b)*r+c]; }

/* ---------------- parameters ---------------- */
typedef struct { int n,m,D,r,s,Rmax; } param;
static param mkparam(int n,int m){ param p; p.n=n; p.D=2*n-1; p.r=2*n; p.m=m; p.s=p.D-m; p.Rmax=512; return p; }

typedef struct {
    param P;
    fe *C;            /* m x NC3(r) public cubic coefficients */
} pubkey;

typedef struct {
    param P;
    fe *S, *Sinv;     /* r x r */
    fe *L1, *L2;      /* n x r */
    fe *U, *Uinv;     /* D x D */
    fe *T;            /* m x D */
    fe *QA, *RA;      /* n symmetric n x n matrices each */
} seckey;

/* ---------------- polynomial factorisation over F_q ---------------- */
typedef struct { poly *f; int *mult; int cnt; } factz;

static void sqfree_push(factz*F, poly p, int e){
    if(p.d<1){ pfree(&p); return; }
    F->f=realloc(F->f,sizeof(poly)*(F->cnt+1)); F->mult=realloc(F->mult,sizeof(int)*(F->cnt+1));
    F->f[F->cnt]=p; F->mult[F->cnt]=e; F->cnt++;
}
/* distinct-degree + equal-degree (Cantor-Zassenhaus) on squarefree monic f */
static void edf(poly f,int d, factz*out,int e){
    if(f.d==d){ pmonic(&f); sqfree_push(out,f,e); return; }
    if(f.d<=0){ pfree(&f); return; }
    /* exponent (q^d-1)/2 = ((1+q+..+q^{d-1})) * (q-1)/2 */
    for(;;){
        poly a=pnew(f.d-1); for(int i=0;i<f.d;i++) a.c[i]=frand(); pnorm(&a);
        if(a.d<1){ pfree(&a); continue; }
        /* aS = prod_{i<d} a^{q^i} */
        poly aS=pcopy(&a), cur=pcopy(&a);
        for(int i=1;i<d;i++){ poly nx=ppowmod(&cur,Q,&f); pfree(&cur); cur=nx;
            poly t=pmul(&aS,&cur); poly t2=pmod(&t,&f); pfree(&t); pfree(&aS); aS=t2; }
        pfree(&cur);
        poly b=ppowmod(&aS,(Q-1)/2,&f); pfree(&aS); pfree(&a);
        if(b.d>=0) b.c[0]=fsub(b.c[0],1); pnorm(&b);
        poly g=pgcd(&f,&b); pfree(&b);
        if(g.d>0 && g.d<f.d){
            poly h; pdivmod(&f,&g,&h,NULL);
            pfree(&f); edf(g,d,out,e); edf(h,d,out,e); return;
        }
        pfree(&g);
    }
}
static void ddf(poly f, factz*out,int e){      /* f squarefree monic */
    poly xq=pnew(1); xq.c[1]=1;                 /* x */
    poly cur=pmod(&xq,&f);                      /* x^{q^0} */
    poly X=pcopy(&xq); pfree(&xq);
    int d=1;
    poly work=pcopy(&f);
    while(work.d>=2*d){
        poly nx=ppowmod(&cur,Q,&work); pfree(&cur); cur=nx;   /* x^{q^d} mod work */
        poly t=pcopy(&cur); if(t.d>=1) t.c[1]=fsub(t.c[1],1); else { pfree(&t); t=pnew(1); t.c[1]=fneg(1);} pnorm(&t);
        poly g=pgcd(&work,&t); pfree(&t);
        if(g.d>0){ poly h; pdivmod(&work,&g,&h,NULL); pfree(&work); work=h;
                   poly gc=pcopy(&g); pfree(&g); edf(gc,d,out,e);
                   poly c2=pmod(&cur,&work); pfree(&cur); cur=c2; }
        else pfree(&g);
        d++;
    }
    if(work.d>0){ pmonic(&work); sqfree_push(out,work,e); } else pfree(&work);
    pfree(&X); pfree(&cur);
}
static factz factor_poly(const poly*fin){
    factz F; F.f=NULL; F.mult=NULL; F.cnt=0;
    poly f=pcopy(fin); pmonic(&f);
    /* squarefree decomposition (Yun) */
    int e=1;
    while(f.d>0){
        poly fp=pderiv(&f);
        poly g = (fp.d<0)? pcopy(&f) : pgcd(&f,&fp);
        pfree(&fp);
        poly w; pdivmod(&f,&g,&w,NULL);      /* w = squarefree part chain */
        if(w.d>0){ poly wc=pcopy(&w); ddf(wc,&F,e); }
        pfree(&w);
        pfree(&f); f=g; e++;
        if(e>64) break;
    }
    pfree(&f);
    return F;
}
static void factz_free(factz*F){ for(int i=0;i<F->cnt;i++) pfree(&F->f[i]); free(F->f); free(F->mult); F->cnt=0; }

/* ---------------- key generation (Alg. 9) ---------------- */
static void keygen(param P, pubkey*pk, seckey*sk){
    int n=P.n,r=P.r,D=P.D,m=P.m;
    c3_init(r);
    sk->P=P; pk->P=P;
    sk->S=mat_alloc(r,r); sk->Sinv=mat_alloc(r,r);
    do{ mat_rand(sk->S,r,r); }while(!mat_inv(sk->S,r,sk->Sinv));
    sk->L1=mat_alloc(n,r); sk->L2=mat_alloc(n,r);
    for(int i=0;i<n;i++)for(int j=0;j<r;j++){ MAT(sk->L1,r,i,j)=MAT(sk->S,r,i,j); MAT(sk->L2,r,i,j)=MAT(sk->S,r,n+i,j); }
    sk->U=mat_alloc(D,D); sk->Uinv=mat_alloc(D,D);
    do{ mat_rand(sk->U,D,D); }while(!mat_inv(sk->U,D,sk->Uinv));
    sk->T=mat_alloc(m,D);
    for(int i=0;i<m;i++)for(int j=0;j<D;j++) MAT(sk->T,D,i,j)=MAT(sk->U,D,i,j);

    fe inv2=finv(2);
    sk->QA=mat_alloc(n,n*n); sk->RA=mat_alloc(n,n*n);
    for(int i=0;i<n;i++){
        fe*A=&sk->QA[(size_t)i*n*n];
        MAT(A,n,i,i)=frand_nz();                                  /* a_i != 0 */
        for(int j=0;j<i;j++){ fe b=frand(); MAT(A,n,i,j)=fmul(b,inv2); MAT(A,n,j,i)=MAT(A,n,i,j); } /* b_i(X_<i) X_i */
        for(int u=0;u<i;u++)for(int v=u;v<i;v++){ fe g=frand();    /* c_i(X_<i) */
            if(u==v) MAT(A,n,u,u)=fadd(MAT(A,n,u,u),g);
            else { fe hh=fmul(g,inv2); MAT(A,n,u,v)=fadd(MAT(A,n,u,v),hh); MAT(A,n,v,u)=fadd(MAT(A,n,v,u),hh); } }
        fe*B=&sk->RA[(size_t)i*n*n];
        for(int u=0;u<n;u++)for(int v=u;v<n;v++){ fe g=frand();
            if(u==v) MAT(B,n,u,u)=g; else { fe hh=fmul(g,inv2); MAT(B,n,u,v)=hh; MAT(B,n,v,u)=hh; } }
    }
    /* expand public key: A_i(z) = z^T (L1^T QA_i L1 + L2^T RA_i L2) z ; Y_k(z) = row_k(L2).z */
    int nc=NC3(r);
    fe *Az=mat_alloc(n,r*r), *tmp=mat_alloc(n,r), *tmp2=mat_alloc(r,r);
    for(int i=0;i<n;i++){
        fe*M=&Az[(size_t)i*r*r];
        /* L1^T QA_i L1 */
        mat_mul(&sk->QA[(size_t)i*n*n],n,n,sk->L1,r,tmp);        /* n x r */
        for(int a=0;a<r;a++)for(int b=0;b<r;b++){ uint64_t s=0; for(int k=0;k<n;k++) s+=(uint64_t)MAT(sk->L1,r,k,a)*MAT(tmp,r,k,b)%Q; MAT(M,r,a,b)=(fe)(s%Q); }
        mat_mul(&sk->RA[(size_t)i*n*n],n,n,sk->L2,r,tmp);
        for(int a=0;a<r;a++)for(int b=0;b<r;b++){ uint64_t s=0; for(int k=0;k<n;k++) s+=(uint64_t)MAT(sk->L2,r,k,a)*MAT(tmp,r,k,b)%Q; MAT(M,r,a,b)=fadd(MAT(M,r,a,b),(fe)(s%Q)); }
    }
    free(tmp); free(tmp2);
    /* f_l = sum_{i+k=l} A_i * Y_k  (cubic coefficient vectors) */
    fe *fl=mat_alloc(D,nc);
    for(int i=0;i<n;i++){ fe*M=&Az[(size_t)i*r*r];
        for(int k=0;k<n;k++){ int l=i+k; fe*dst=&fl[(size_t)l*nc]; const fe*lin=&MAT(sk->L2,r,k,0);
            for(int a=0;a<r;a++)for(int b=a;b<r;b++){
                fe qc = (a==b)? MAT(M,r,a,a) : fadd(MAT(M,r,a,b),MAT(M,r,b,a));
                if(!qc) continue;
                for(int c=0;c<r;c++){ if(!lin[c]) continue;
                    dst[ic3(r,a,b,c)] = fadd(dst[ic3(r,a,b,c)], fmul(qc,lin[c])); } } } }
    free(Az);
    pk->C=mat_alloc(m,nc);
    for(int j=0;j<m;j++)for(int l=0;l<D;l++){ fe t=MAT(sk->T,D,j,l); if(!t) continue;
        fe*dst=&MAT(pk->C,nc,j,0); const fe*src=&fl[(size_t)l*nc];
        for(int u=0;u<nc;u++) if(src[u]) dst[u]=fadd(dst[u],fmul(t,src[u])); }
    free(fl);
}
static void pk_eval(const pubkey*pk,const fe*z,fe*out){
    int r=pk->P.r,m=pk->P.m,nc=NC3(r);
    for(int j=0;j<m;j++) out[j]=0;
    for(int a=0;a<r;a++){ if(!z[a]) continue;
      for(int b=a;b<r;b++){ if(!z[b]) continue; fe zab=fmul(z[a],z[b]);
        for(int c=b;c<r;c++){ if(!z[c]) continue; fe mono=fmul(zab,z[c]); int id=ic3(r,a,b,c);
            for(int j=0;j<m;j++){ fe cc=MAT(pk->C,nc,j,id); if(cc) out[j]=fadd(out[j],fmul(cc,mono)); } } } }
}
#endif
