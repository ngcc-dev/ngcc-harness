/* xl.h - Macaulay/XL solver: m polynomials of degree<=3 in N vars, unique solution */
#ifndef XL_H
#include <time.h>
#define XL_H
#include "fq.h"
#include "solve.h"

typedef struct { uint8_t e[16]; } mono;           /* exponent vector, N<=16 */
static int MN;                                     /* number of variables */
static int mdeg(const mono*a){ int s=0; for(int i=0;i<MN;i++) s+=a->e[i]; return s; }
static uint64_t mkey(const mono*a){ uint64_t k=1469598103934665603ull; for(int i=0;i<MN;i++){ k^=a->e[i]; k*=1099511628211ull; } return k; }
static int meq(const mono*a,const mono*b){ for(int i=0;i<MN;i++) if(a->e[i]!=b->e[i]) return 0; return 1; }

typedef struct { mono*list; int cnt,cap; int*ht; int htsize; } montab;
static void mt_init(montab*T,int cap){ T->cnt=0;T->cap=cap;T->list=malloc(sizeof(mono)*cap);
    T->htsize=1; while(T->htsize < 4*cap) T->htsize<<=1; T->ht=malloc(sizeof(int)*T->htsize);
    for(int i=0;i<T->htsize;i++) T->ht[i]=-1; }
static int mt_find(montab*T,const mono*a,int insert){
    uint64_t h=mkey(a)&(T->htsize-1);
    for(;;){ int id=T->ht[h]; if(id<0){ if(!insert) return -1;
            if(T->cnt>=T->cap){ T->cap*=2; T->list=realloc(T->list,sizeof(mono)*T->cap);}
            T->list[T->cnt]=*a; T->ht[h]=T->cnt; return T->cnt++; }
        if(meq(&T->list[id],a)) return id; h=(h+1)&(T->htsize-1); }
}
/* enumerate all monomials of degree <= d in N vars, graded order (deg asc, then lex) */
static void mt_fill(montab*T,int N,int d){
    mono z; memset(&z,0,sizeof z);
    for(int deg=0;deg<=d;deg++){
        /* iterate compositions of deg into N parts */
        int*e=calloc(N,sizeof(int)); e[0]=deg;
        for(;;){ mono a; memset(&a,0,sizeof a); for(int i=0;i<N;i++) a.e[i]=e[i];
            mt_find(T,&a,1);
            int i=N-1; while(i>0 && e[i]==0) i--;
            if(i==0) break;
            int v=e[i]; e[i]=0; e[i-1]--; e[N-1]=v+1;
            /* renormalise: standard next-composition */
            if(i-1==N-1) break;
        }
        free(e);
    }
}
/* simpler recursive enumeration */
static void mt_rec(montab*T,mono*cur,int pos,int rem,int N){
    if(pos==N-1){ cur->e[pos]=rem; mt_find(T,cur,1); return; }
    for(int k=rem;k>=0;k--){ cur->e[pos]=k; mt_rec(T,cur,pos+1,rem-k,N); }
    cur->e[pos]=0;
}
static void mt_build(montab*T,int N,int d){
    for(int deg=0;deg<=d;deg++){ mono c; memset(&c,0,sizeof c); mt_rec(T,&c,0,deg,N); }
}

typedef struct { int nterm; mono*mo; fe*co; } spoly;

/* Build Macaulay matrix at degree d and return right-kernel dimension; solution in sol[N] */
static int xl_solve(spoly*eqs,int meq_n,int N,int d,fe*sol,int verbose,size_t*mem_out,double*secs){
    MN=N;
    montab COL; mt_init(&COL, 1<<12); mt_build(&COL,N,d);
    montab MUL; mt_init(&MUL, 1<<12); mt_build(&MUL,N,d-3);
    int nc=COL.cnt, nmul=MUL.cnt;
    long long nrow=(long long)meq_n*nmul;
    if(verbose) fprintf(stderr,"   [XL d=%d] rows=%lld cols=%d  mem=%.2f GB\n",d,nrow,nc,(double)nrow*nc*sizeof(fe)/1e9);
    if((double)nrow*nc*sizeof(fe) > 20e9){ if(verbose)fprintf(stderr,"   too big, skip\n"); free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht); return -1; }
    if(mem_out)*mem_out=(size_t)((double)nrow*nc*sizeof(fe));
    fe*M=mat_alloc((int)nrow,nc);
    for(int j=0;j<meq_n;j++) for(int u=0;u<nmul;u++){
        fe*row=&M[(size_t)(j*(long long)nmul+u)*nc];
        for(int t=0;t<eqs[j].nterm;t++){ mono p=eqs[j].mo[t];
            for(int i=0;i<N;i++) p.e[i]+=MUL.list[u].e[i];
            int id=mt_find(&COL,&p,0);
            row[id]=fadd(row[id],eqs[j].co[t]); } }
    clock_t t0=clock();
    fe*ker; int kd=mat_kernel(M,(int)nrow,nc,&ker);
    if(secs)*secs=(double)(clock()-t0)/CLOCKS_PER_SEC;
    int ret=kd;
    if(kd==1){
        /* column of constant monomial 1 */
        mono one; memset(&one,0,sizeof one); int i1=mt_find(&COL,&one,0);
        fe c0=MAT(ker,1,i1,0);
        if(!c0) ret=-2; else { fe ic=finv(c0);
            for(int v=0;v<N;v++){ mono x; memset(&x,0,sizeof x); x.e[v]=1; int ix=mt_find(&COL,&x,0);
                sol[v]=fmul(MAT(ker,1,ix,0),ic); } }
    }
    free(ker); free(M); free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht);
    return ret;
}

/* evaluate a spoly at x */
static fe sp_eval(const spoly*f,const fe*x,int N){
    fe acc=0;
    for(int t=0;t<f->nterm;t++){ fe mv=f->co[t];
        for(int i=0;i<N;i++) for(int e=0;e<f->mo[t].e[i];e++) mv=fmul(mv,x[i]);
        acc=fadd(acc,mv); }
    return acc;
}
/* Macaulay at degree d -> all F_q-rational solutions. returns count, or -1 if degree too low */
static int XL_EDEG=3;
static int xl_rational_solutions(spoly*eqs,int meq_n,int N,int d,fe*sols,int maxsols,
                                 int verbose,int*kdim_out,double*secs){
    MN=N;
    montab COL; mt_init(&COL,1<<12); mt_build(&COL,N,d);
    montab MUL; mt_init(&MUL,1<<12); mt_build(&MUL,N,d-XL_EDEG);
    int nc=COL.cnt,nmul=MUL.cnt; long long nrow=(long long)meq_n*nmul;
    if((double)nrow*nc*sizeof(fe) > 20e9){ if(verbose)fprintf(stderr,"   [d=%d] %.1f GB - over budget\n",d,(double)nrow*nc*sizeof(fe)/1e9);
        free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht); return -2; }
    fe*M=mat_alloc((int)nrow,nc);
    for(int j=0;j<meq_n;j++) for(int u=0;u<nmul;u++){
        fe*row=&M[(size_t)(j*(long long)nmul+u)*nc];
        for(int t=0;t<eqs[j].nterm;t++){ mono pm=eqs[j].mo[t];
            for(int i=0;i<N;i++) pm.e[i]+=MUL.list[u].e[i];
            int id=mt_find(&COL,&pm,0); row[id]=fadd(row[id],eqs[j].co[t]); } }
    clock_t t0=clock();
    fe*K; int kd=mat_kernel(M,(int)nrow,nc,&K);
    free(M);
    if(secs)*secs=(double)(clock()-t0)/CLOCKS_PER_SEC;
    if(kdim_out)*kdim_out=kd;
    if(verbose) fprintf(stderr,"   [d=%d] rows=%lld cols=%d kernel=%d (%.1fs)\n",d,nrow,nc,kd,secs?*secs:0.0);
    if(kd<1){ free(K);free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht); return 0; }
    /* S0 = monomials of degree <= d-1 */
    int *S0=malloc(sizeof(int)*nc), ns0=0;
    for(int i=0;i<nc;i++) if(mdeg(&COL.list[i])<=d-1) S0[ns0++]=i;
    /* A0 = K rows at S0 ; pick kd independent rows */
    fe*A0=mat_alloc(ns0,kd);
    for(int i=0;i<ns0;i++) for(int j=0;j<kd;j++) MAT(A0,kd,i,j)=MAT(K,kd,S0[i],j);
    fe*tmp=mat_alloc(ns0,kd); memcpy(tmp,A0,(size_t)ns0*kd*sizeof(fe));
    int*sel=malloc(sizeof(int)*kd), nsel=0;
    { fe*work=mat_alloc(kd,kd);
      for(int i=0;i<ns0 && nsel<kd;i++){
        for(int j=0;j<kd;j++) MAT(work,kd,nsel,j)=MAT(A0,kd,i,j);
        if(mat_rank(work,nsel+1,kd)==nsel+1){ sel[nsel++]=i; }
      } free(work); }
    if(nsel<kd){ free(K);free(A0);free(tmp);free(S0);free(sel);
        free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht); return -1; }
    fe*A0P=mat_alloc(kd,kd), *A0Pi=mat_alloc(kd,kd);
    for(int i=0;i<kd;i++) for(int j=0;j<kd;j++) MAT(A0P,kd,i,j)=MAT(A0,kd,sel[i],j);
    if(!mat_inv(A0P,kd,A0Pi)){ free(K);free(A0);free(tmp);free(S0);free(sel);free(A0P);free(A0Pi);
        free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht); return -1; }
    /* Mr = sum_i r_i * (A0P^-1 * A_iP) */
    fe*Mr=mat_alloc(kd,kd), *AiP=mat_alloc(kd,kd), *prod=mat_alloc(kd,kd);
    for(int vi=0;vi<N;vi++){
        fe r=frand();
        if(!r) continue;
        int bad=0;
        for(int i=0;i<kd;i++){ mono pm=COL.list[S0[sel[i]]]; pm.e[vi]++;
            int id=mt_find(&COL,&pm,0); if(id<0){bad=1;break;}
            for(int j=0;j<kd;j++) MAT(AiP,kd,i,j)=MAT(K,kd,id,j); }
        if(bad) continue;
        mat_mul(A0Pi,kd,kd,AiP,kd,prod);
        for(int i=0;i<kd;i++)for(int j=0;j<kd;j++) MAT(Mr,kd,i,j)=fadd(MAT(Mr,kd,i,j),fmul(r,MAT(prod,kd,i,j)));
    }
    poly cp=charpoly(Mr,kd);
    fe*roots=malloc(sizeof(fe)*(kd+1));
    int nr=poly_roots(&cp,roots,kd);
    pfree(&cp);
    if(verbose) fprintf(stderr,"   quotient dim=%d, F_q-rational eigenvalues: %d\n",kd,nr);
    mono one; memset(&one,0,sizeof one); int i1=mt_find(&COL,&one,0);
    int cnt=0;
    fe*shift=mat_alloc(kd,kd);
    for(int t=0;t<nr && cnt<maxsols;t++){
        memcpy(shift,Mr,(size_t)kd*kd*sizeof(fe));
        for(int i=0;i<kd;i++) MAT(shift,kd,i,i)=fsub(MAT(shift,kd,i,i),roots[t]);
        fe*ev; int ed=mat_kernel(shift,kd,kd,&ev);
        for(int e=0;e<ed && cnt<maxsols;e++){
            fe*c=malloc(sizeof(fe)*nc);
            for(int i=0;i<nc;i++){ uint64_t a=0; for(int j=0;j<kd;j++) a+=(uint64_t)MAT(K,kd,i,j)*MAT(ev,ed,j,e)%Q; c[i]=(fe)(a%Q); }
            if(!c[i1]){ free(c); continue; }
            fe ic=finv(c[i1]); fe*cand=malloc(sizeof(fe)*N);
            for(int v2=0;v2<N;v2++){ mono x; memset(&x,0,sizeof x); x.e[v2]=1; cand[v2]=fmul(c[mt_find(&COL,&x,0)],ic); }
            int ok=1; for(int j=0;j<meq_n;j++) if(sp_eval(&eqs[j],cand,N)){ok=0;break;}
            if(ok){ memcpy(&sols[(size_t)cnt*N],cand,sizeof(fe)*N); cnt++; }
            free(cand); free(c);
        }
        free(ev);
    }
    free(shift);free(roots);free(Mr);free(AiP);free(prod);free(A0P);free(A0Pi);
    free(A0);free(tmp);free(S0);free(sel);free(K);
    free(COL.list);free(COL.ht);free(MUL.list);free(MUL.ht);
    return cnt;
}

#endif
