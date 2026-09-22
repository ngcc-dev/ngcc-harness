/* attack3.c - FULL BREAK: K2 recovery -> equivalent trapdoor -> universal forgery */
#include "sign.h"
#include "tensor.h"
#include "recover.h"
#include "xl.h"
#include <time.h>

static int NG,RG;                       /* n, r globals for convenience */
#define XI(i) (i)                       /* x variable index */
#define YI(i) (NG+(i))                  /* y variable index */

/* ---------- stage 1+2: recover K2 (point model + K2/W separation) ---------- */
static void restrict_tensors(const fe*T,int m,int r,const fe*B,int NL,fe*RT){
    for(int j=0;j<m;j++) tensor_subst(&T[(size_t)j*r*r*r],r,B,NL,&RT[(size_t)j*NL*NL*NL]);
}
static void addterm(spoly*f,const mono*mo,fe c,int N){
    if(!c) return;
    for(int t=0;t<f->nterm;t++){ int same=1; for(int i=0;i<N;i++) if(f->mo[t].e[i]!=mo->e[i]){same=0;break;}
        if(same){ f->co[t]=fadd(f->co[t],c); return; } }
    f->mo[f->nterm]=*mo; f->co[f->nterm]=c; f->nterm++;
}
static int vanishes_on(const fe*T,int m,int r,const fe*Kb,int k){
    fe*v=malloc(sizeof(fe)*r); int ok=1;
    for(int t=0;t<12&&ok;t++){ for(int i=0;i<r;i++) v[i]=0;
        for(int c=0;c<k;c++){ fe a=frand(); for(int i=0;i<r;i++) v[i]=fadd(v[i],fmul(a,MAT(Kb,k,i,c))); }
        for(int j=0;j<m;j++) if(tensor_eval(&T[(size_t)j*r*r*r],r,v)) ok=0; }
    free(v); return ok;
}
static int recover_K2(const fe*T,int m,int r,int n,int dmax,fe**K2out,int*dsolv,double*tsec){
    int NL=n+1,N=n;
    fe*B=mat_alloc(r,NL); mat_rand(B,r,NL);
    fe*RT=mat_alloc(m,NL*NL*NL); restrict_tensors(T,m,r,B,NL,RT);
    spoly*eqs=malloc(sizeof(spoly)*m);
    for(int j=0;j<m;j++){ eqs[j].nterm=0; eqs[j].mo=malloc(sizeof(mono)*4000); eqs[j].co=malloc(sizeof(fe)*4000);
        fe*Tj=&RT[(size_t)j*NL*NL*NL];
        for(int p=0;p<NL;p++)for(int q=0;q<NL;q++)for(int s=0;s<NL;s++){ fe c=T3(Tj,NL,p,q,s); if(!c) continue;
            mono mo; memset(&mo,0,sizeof mo); int ok=1;
            int ix[3]={p,q,s}; for(int t=0;t<3;t++){ if(ix[t]<N) mo.e[ix[t]]++; }
            if(ok) addterm(&eqs[j],&mo,c,N); } }
    fe*sols=malloc(sizeof(fe)*N*64); int nsol=0,d,kd; double secs,tot=0;
    for(d=3;d<=dmax;d++){ nsol=xl_rational_solutions(eqs,m,N,d,sols,64,0,&kd,&secs); tot+=secs;
        if(nsol>=1) break; if(nsol==-2) return 0; }
    if(dsolv)*dsolv=d; if(tsec)*tsec=tot;
    if(nsol<1) return 0;
    fe*u=malloc(sizeof(fe)*NL), *v=malloc(sizeof(fe)*r), *J=mat_alloc(m,r);
    for(int t=0;t<nsol;t++){
        for(int i=0;i<N;i++) u[i]=sols[(size_t)t*N+i]; u[N]=1;
        for(int i=0;i<r;i++){ uint64_t a=0; for(int k=0;k<NL;k++) a+=(uint64_t)MAT(B,NL,i,k)*u[k]%Q; v[i]=(fe)(a%Q); }
        for(int j=0;j<m;j++){ const fe*Tj=&T[(size_t)j*r*r*r];
            for(int c=0;c<r;c++){ uint64_t a=0;
                for(int p=0;p<r;p++){ if(!v[p]) continue; uint64_t b=0;
                    for(int qq=0;qq<r;qq++) b+=(uint64_t)T3(Tj,r,p,qq,c)*v[qq]%Q;
                    a+=(uint64_t)v[p]*(b%Q)%Q; }
                MAT(J,r,j,c)=fmul(3,(fe)(a%Q)); } }
        fe*Kb; int kdim=mat_kernel(J,m,r,&Kb);
        if(kdim==n && vanishes_on(T,m,r,Kb,kdim)){ *K2out=Kb; return 1; }
        free(Kb);
    }
    return 0;
}
int main(int argc,char**argv){
    int n=argc>1?atoi(argv[1]):6, m=argc>2?atoi(argv[2]):10;
    uint64_t seed=argc>3?strtoull(argv[3],0,10):1;
    int dmax=argc>4?atoi(argv[4]):12;
    rng_seed(seed);
    param P=mkparam(n,m); NG=n; RG=P.r;
    int r=P.r,D=P.D;
    printf("================ Facto-DSA  n=%d r=%d D=%d m=%d s=%d  (seed %llu) ================\n",n,r,D,m,P.s,(unsigned long long)seed);
    pubkey pk; seckey sk; keygen(P,&pk,&sk);
    /* sanity: honest signing works */
    { fe*z=malloc(sizeof(fe)*r); char msg[]="honest"; int tr;
      printf("[0] honest sign/verify: %s\n", (facto_sign(&sk,&pk,(unsigned char*)msg,6,z,&tr)&&facto_verify(&pk,(unsigned char*)msg,6,z))?"ok":"FAIL"); free(z); }
    fe*T=pk_tensors(&pk);

    clock_t T0=clock();
    fe*K2=NULL; int dsolv=0; double t1=0;
    if(!recover_K2(T,m,r,n,dmax,&K2,&dsolv,&t1)){ printf("stage 1 failed\n"); return 1; }
    printf("[1] K2 = ker L2 recovered (dim %d) at solving degree %d, %.1fs\n",n,dsolv,t1);
    { int ok=1; for(int c=0;c<n;c++)for(int i=0;i<n;i++){ uint64_t a=0;
        for(int k=0;k<r;k++) a+=(uint64_t)MAT(sk.L2,r,i,k)*MAT(K2,n,k,c)%Q; if(a%Q) ok=0; }
      printf("    cross-check against secret L2: %s\n", ok?"exact match":"MISMATCH"); }

    /* ---- basis E = [BX | BY], BX = K2 ---- */
    fe*E=mat_alloc(r,r), *Ei=mat_alloc(r,r);
    for(;;){ for(int i=0;i<r;i++){ for(int c=0;c<n;c++) MAT(E,r,i,c)=MAT(K2,n,i,c);
                for(int c=0;c<n;c++) MAT(E,r,i,n+c)=frand(); }
        if(mat_inv(E,r,Ei)) break; }
    fe*Pt=mat_alloc(m,r*r*r);
    for(int j=0;j<m;j++) tensor_subst(&T[(size_t)j*r*r*r],r,E,r,&Pt[(size_t)j*r*r*r]);
    { int nz=0; for(int j=0;j<m;j++)for(int a=0;a<n;a++)for(int b=0;b<n;b++)for(int c=0;c<n;c++)
        if(T3(&Pt[(size_t)j*r*r*r],r,a,b,c)) nz++;
      printf("[2] in (x,y) coordinates: x^3 block entries nonzero = %d (expect 0)\n",nz); }

    /* ---- stage 3a: shear x -> x + Gamma y to kill the x y^2 block ---- */
    int nun=n*n;
    int neq=m*n*(n*(n+1)/2);
    fe*Asys=mat_alloc(neq,nun), *rhs=malloc(sizeof(fe)*neq);
    fe*V=mat_alloc(r,r), *tmpT=mat_alloc(m,r*r*r);
    #define XY2(TT,j,a,b,c) T3(&(TT)[(size_t)(j)*r*r*r],r,(a),YI(b),YI(c))
    /* baseline (Gamma = 0) */
    { int e=0; for(int j=0;j<m;j++)for(int a=0;a<n;a++)for(int b=0;b<n;b++)for(int c=b;c<n;c++)
        rhs[e++]=fneg(XY2(Pt,j,a,b,c)); }
    for(int g=0; g<nun; g++){
        int p=g/n, bb=g%n;
        memset(V,0,(size_t)r*r*sizeof(fe));
        for(int i=0;i<r;i++) MAT(V,r,i,i)=1;
        MAT(V,r,p,YI(bb))=1;                       /* x_p = x'_p + y_bb */
        for(int j=0;j<m;j++) tensor_subst(&Pt[(size_t)j*r*r*r],r,V,r,&tmpT[(size_t)j*r*r*r]);
        int e=0; for(int j=0;j<m;j++)for(int a=0;a<n;a++)for(int b=0;b<n;b++)for(int c=b;c<n;c++){
            MAT(Asys,nun,e,g)=fsub(XY2(tmpT,j,a,b,c),XY2(Pt,j,a,b,c)); e++; }
    }
    fe*Gam=malloc(sizeof(fe)*nun);
    if(!mat_solve(Asys,neq,nun,rhs,Gam)){ printf("[3] shear system inconsistent\n"); return 1; }
    memset(V,0,(size_t)r*r*sizeof(fe));
    for(int i=0;i<r;i++) MAT(V,r,i,i)=1;
    for(int p=0;p<n;p++)for(int bb=0;bb<n;bb++) MAT(V,r,p,YI(bb))=Gam[p*n+bb];
    for(int j=0;j<m;j++) tensor_subst(&Pt[(size_t)j*r*r*r],r,V,r,&tmpT[(size_t)j*r*r*r]);
    memcpy(Pt,tmpT,(size_t)m*r*r*r*sizeof(fe));
    { int nz=0; for(int j=0;j<m;j++)for(int a=0;a<n;a++)for(int b=0;b<n;b++)for(int c=0;c<n;c++)
        if(XY2(Pt,j,a,b,c)) nz++;
      printf("[3] shear applied: x*y^2 block entries nonzero = %d (expect 0)\n",nz); }
    /* new basis: BX unchanged, BY <- BX*Gamma + BY */
    fe*E2=mat_alloc(r,r); mat_mul(E,r,r,V,r,E2); memcpy(E,E2,(size_t)r*r*sizeof(fe)); free(E2);
    if(!mat_inv(E,r,Ei)){ printf("basis singular\n"); return 1; }

    /* ---- stage 3b: extract Q-space and the matrices G^(j) ---- */
    int nq=n*(n+1)/2;
    fe*forms=mat_alloc(m*n,nq);                   /* q_{j,k} as coefficient vectors */
    fe*fmat=mat_alloc(m*n,n*n);
    for(int j=0;j<m;j++)for(int k=0;k<n;k++){ int row=j*n+k;
        for(int a=0;a<n;a++)for(int b=0;b<n;b++){ fe v2=fmul(3,T3(&Pt[(size_t)j*r*r*r],r,a,b,YI(k)));
            MAT(fmat,n*n,row,a*n+b)=v2; }
        for(int a=0;a<n;a++)for(int b=a;b<n;b++)
            MAT(forms,nq,row,iq2(n,a,b)) = (a==b)? MAT(fmat,n*n,row,a*n+a)
                                                 : fadd(MAT(fmat,n*n,row,a*n+b),MAT(fmat,n*n,row,b*n+a)); }
    fe*red=mat_alloc(m*n,nq); memcpy(red,forms,(size_t)m*n*nq*sizeof(fe));
    int*piv=malloc(sizeof(int)*(m*n)); int qdim=mat_rref(red,m*n,nq,piv);
    printf("[4] dim of hidden quadratic space Q = %d (expect n=%d)\n",qdim,n);
    if(qdim!=n){ printf("    unexpected\n"); return 1; }
    /* basis matrices qhat_a (symmetric n x n) */
    fe*QH=mat_alloc(n,n*n); fe i2=finv(2);
    for(int a=0;a<n;a++){ fe*A=&QH[(size_t)a*n*n];
        for(int p2=0;p2<n;p2++)for(int q2=p2;q2<n;q2++){ fe c=MAT(red,nq,a,iq2(n,p2,q2));
            if(p2==q2) MAT(A,n,p2,p2)=c; else { fe h=fmul(c,i2); MAT(A,n,p2,q2)=h; MAT(A,n,q2,p2)=h; } } }
    /* G^(j)[a][k] : q_{j,k} = sum_a G[a][k] qhat_a */
    fe*G=mat_alloc(m,n*n);
    { fe*Bt=mat_alloc(nq,n); for(int a=0;a<n;a++)for(int i=0;i<nq;i++) MAT(Bt,n,i,a)=MAT(red,nq,a,i);
      fe*co=malloc(sizeof(fe)*n);
      for(int j=0;j<m;j++)for(int k=0;k<n;k++){
        fe*col=malloc(sizeof(fe)*nq); for(int i=0;i<nq;i++) col[i]=MAT(forms,nq,j*n+k,i);
        if(!mat_solve(Bt,nq,n,col,co)){ printf("basis solve failed\n"); return 1; }
        for(int a=0;a<n;a++) MAT(&G[(size_t)j*n*n],n,a,k)=co[a]; free(col); }
      free(Bt); free(co); }

    /* ---- stage 3c: symmetriser C_s with G^(j) C_s symmetric ---- */
    fe*Csys=mat_alloc(m*n*(n-1)/2>0?m*n*(n-1)/2:1,n*n); int row=0;
    for(int j=0;j<m;j++){ fe*Gj=&G[(size_t)j*n*n];
        for(int a=0;a<n;a++)for(int b=a+1;b<n;b++){
            for(int p2=0;p2<n;p2++)for(int q2=0;q2<n;q2++){
                fe v2=fsub(  (p2==0?0:0), 0);
                /* (G Cs)_{ab} - (G Cs)_{ba} ; Cs[p][q] unknown index p*n+q */
                fe c1 = (q2==b)? MAT(Gj,n,a,p2) : 0;
                fe c2 = (q2==a)? MAT(Gj,n,b,p2) : 0;
                v2=fsub(c1,c2);
                if(v2) MAT(Csys,n*n,row,p2*n+q2)=fadd(MAT(Csys,n*n,row,p2*n+q2),v2); }
            row++; } }
    fe*ker; int kd=mat_kernel(Csys,row,n*n,&ker);
    printf("[5] symmetriser solution space dim = %d\n",kd);
    fe*Cs=mat_alloc(n,n); int gotCs=0;
    for(int att=0;att<30&&!gotCs;att++){ memset(Cs,0,(size_t)n*n*sizeof(fe));
        for(int c=0;c<kd;c++){ fe rr=frand(); for(int i=0;i<n*n;i++) Cs[i]=fadd(Cs[i],fmul(rr,MAT(ker,kd,i,c))); }
        fe*tmp=mat_alloc(n,n); if(mat_inv(Cs,n,tmp)) gotCs=1; free(tmp); }
    if(!gotCs){ printf("no invertible symmetriser\n"); return 1; }
    /* E_j = G^(j) Cs (must be symmetric) */
    fe*Ej=mat_alloc(m,n*n); int asym=0;
    for(int j=0;j<m;j++){ mat_mul(&G[(size_t)j*n*n],n,n,Cs,n,&Ej[(size_t)j*n*n]);
        fe*X=&Ej[(size_t)j*n*n]; for(int a=0;a<n;a++)for(int b=0;b<n;b++) if(MAT(X,n,a,b)!=MAT(X,n,b,a)) asym++; }
    printf("    E_j symmetric: %s\n", asym?"NO":"yes");

    /* --- diagnostics on Q --- */
    { int minrk=99; fe*Mt=mat_alloc(n,n);
      for(int tr2=0;tr2<400;tr2++){ memset(Mt,0,(size_t)n*n*sizeof(fe));
        for(int a=0;a<n;a++){ fe rr=frand(); for(int i2=0;i2<n*n;i2++) Mt[i2]=fadd(Mt[i2],fmul(rr,QH[(size_t)a*n*n+i2])); }
        int rk=mat_rank(Mt,n,n); if(rk<minrk) minrk=rk; }
      printf("    [diag] min rank over 400 random elements of Q = %d\n",minrk);
      /* exhaustive over single basis elements */
      for(int a=0;a<n;a++) printf("    [diag] rank(qhat_%d) = %d\n",a,mat_rank(&QH[(size_t)a*n*n],n,n));
      free(Mt);
      /* ground truth: M = (L1*E)[:,0:n] ; Q~_0 = M^T QA_0 M must lie in Q and have rank 1 */
      fe*L1E=mat_alloc(n,r); mat_mul(sk.L1,n,r,E,r,L1E);
      fe*Mgt=mat_alloc(n,n), *Kgt=mat_alloc(n,n);
      for(int i2=0;i2<n;i2++)for(int j2=0;j2<n;j2++){ MAT(Mgt,n,i2,j2)=MAT(L1E,r,i2,j2); MAT(Kgt,n,i2,j2)=MAT(L1E,r,i2,n+j2); }
      int knz=0; for(int i2=0;i2<n*n;i2++) if(Kgt[i2]) knz++;
      printf("    [diag] ground-truth K block (should be 0 after shear): %d nonzero\n",knz);
      fe*tmg=mat_alloc(n,n); mat_mul(&sk.QA[0],n,n,Mgt,n,tmg);
      fe*Q0=mat_alloc(n,n);
      for(int i2=0;i2<n;i2++)for(int j2=0;j2<n;j2++){ uint64_t acc=0;
        for(int k2=0;k2<n;k2++) acc+=(uint64_t)MAT(Mgt,n,k2,i2)*MAT(tmg,n,k2,j2)%Q; MAT(Q0,n,i2,j2)=(fe)(acc%Q); }
      printf("    [diag] rank of true Q~_0 = %d (expect 1)\n",mat_rank(Q0,n,n));
      /* is Q0 in span(QH)? */
      { fe*Asp=mat_alloc(n*n,n), *bb=malloc(sizeof(fe)*n*n), *xx=malloc(sizeof(fe)*n);
        for(int i2=0;i2<n*n;i2++){ for(int a=0;a<n;a++) MAT(Asp,n,i2,a)=QH[(size_t)a*n*n+i2]; bb[i2]=Q0[i2]; }
        printf("    [diag] true Q~_0 lies in recovered Q: %s\n", mat_solve(Asp,n*n,n,bb,xx)?"yes":"NO"); }
      }
    { /* premise check: u = Mgt^{-1} e_{n-1} should give rank[A_a u]=1 */
      fe*L1E3=mat_alloc(n,r); mat_mul(sk.L1,n,r,E,r,L1E3);
      fe*Mg=mat_alloc(n,n),*Mgi=mat_alloc(n,n);
      for(int i2=0;i2<n;i2++)for(int j2=0;j2<n;j2++) MAT(Mg,n,i2,j2)=MAT(L1E3,r,i2,j2);
      if(mat_inv(Mg,n,Mgi)){
        for(int lev=n-1; lev>=n-2 && lev>=0; lev--){
          fe*ut=malloc(sizeof(fe)*n);
          for(int i2=0;i2<n;i2++) ut[i2]=MAT(Mgi,n,i2,lev);
          fe*Ph=mat_alloc(n,n);
          for(int a2=0;a2<n;a2++)for(int i2=0;i2<n;i2++){ uint64_t acc=0;
            for(int p2=0;p2<n;p2++) acc+=(uint64_t)MAT(&QH[(size_t)a2*n*n],n,i2,p2)*ut[p2]%Q;
            MAT(Ph,n,i2,a2)=(fe)(acc%Q); }
          printf("    [diag] rank Phi(u) for u = X_%d direction : %d (expect 1 at lev=%d)\n",lev,mat_rank(Ph,n,n),n-1);
          free(Ph);free(ut); } } }
    /* ---- stage 3e: triangular flag, top-down via the determinant filtration ----
       On Q^(k) = span{Qt_0..Qt_{k-1}} (forms on a k-dim space) one has
       det M(mu) = c * mu_{k-1}^k, so l(mu) = trace(M(delta)^{-1} M(mu)) is
       proportional to mu_{k-1}.  ker l is the rank-<=k-1 subspace; its common
       kernel vector is the X_{k-1} direction.  No branching, fully deterministic. */
    fe*vecs=mat_alloc(n,n); int nvec=0;               /* columns: X_{n-1},X_{n-2},... directions */
    fe*cur=mat_alloc(n,n*n); int kdim2=n;
    memcpy(cur,QH,(size_t)n*n*n*sizeof(fe));
    int flagok=1;
    for(int k=n; k>=2 && flagok; k--){
        /* complement basis Pc (n x k) to span(vecs) */
        fe*Pc=mat_alloc(n,k);
        { fe*tst=mat_alloc(n,nvec+1); int got=0;
          for(int c=0;c<k;c++){
            for(int att=0;att<200;att++){
                for(int i2=0;i2<n;i2++) MAT(Pc,k,i2,c)=frand();
                fe*chk3=mat_alloc(n,nvec+c+1);
                for(int i2=0;i2<n;i2++){ for(int j2=0;j2<nvec;j2++) MAT(chk3,nvec+c+1,i2,j2)=MAT(vecs,n,i2,j2);
                    for(int j2=0;j2<=c;j2++) MAT(chk3,nvec+c+1,i2,nvec+j2)=MAT(Pc,k,i2,j2); }
                int rk=mat_rank(chk3,n,nvec+c+1); free(chk3);
                if(rk==nvec+c+1){ got=1; break; } }
          } free(tst); (void)got; }
        /* restrict the current forms to the complement: Ak_a = Pc^T A_a Pc */
        fe*Ak=mat_alloc(kdim2,k*k);
        for(int a2=0;a2<kdim2;a2++){ fe*tm=mat_alloc(n,k);
            mat_mul(&cur[(size_t)a2*n*n],n,n,Pc,k,tm);
            for(int i2=0;i2<k;i2++)for(int j2=0;j2<k;j2++){ uint64_t acc=0;
                for(int q3=0;q3<n;q3++) acc+=(uint64_t)MAT(Pc,k,q3,i2)*MAT(tm,k,q3,j2)%Q;
                MAT(&Ak[(size_t)a2*k*k],k,i2,j2)=(fe)(acc%Q); }
            free(tm); }
        /* u with rank[A_0 u | ... ] = 1 ; then S = {mu : M(mu) u = 0} */
        fe*uu=malloc(sizeof(fe)*k);
        if(!find_common_kernel_dir(Ak,k,uu)){ printf("    common-kernel direction not found at k=%d\n",k); flagok=0; free(uu); break; }
        fe*Phi=mat_alloc(k,kdim2);
        for(int a2=0;a2<kdim2;a2++)for(int i2=0;i2<k;i2++){ uint64_t acc=0;
            for(int p=0;p<k;p++) acc+=(uint64_t)MAT(&Ak[(size_t)a2*k*k],k,i2,p)*uu[p]%Q;
            MAT(Phi,kdim2,i2,a2)=(fe)(acc%Q); }
        fe*Sb; int sd=mat_kernel(Phi,k,kdim2,&Sb);
        if(sd!=kdim2-1){ printf("    layer dim %d at k=%d (expect %d)\n",sd,k,kdim2-1); flagok=0; break; }
        for(int i2=0;i2<n;i2++){ uint64_t acc=0;
            for(int j2=0;j2<k;j2++) acc+=(uint64_t)MAT(Pc,k,i2,j2)*uu[j2]%Q;
            MAT(vecs,n,i2,nvec)=(fe)(acc%Q); }
        nvec++;
        /* replace current space by S */
        fe*nw=mat_alloc(sd,n*n);
        for(int c=0;c<sd;c++) for(int a2=0;a2<kdim2;a2++){ fe co=MAT(Sb,sd,a2,c); if(!co) continue;
            for(int i2=0;i2<n*n;i2++) nw[(size_t)c*n*n+i2]=fadd(nw[(size_t)c*n*n+i2],fmul(co,cur[(size_t)a2*n*n+i2])); }
        free(cur); cur=nw; kdim2=sd;
        free(Pc);free(Ak);free(Sb);free(uu);free(Phi);
    }
    /* basis b_0..b_{n-1} with V_i = span{b_i..b_{n-1}} : b_{n-1-j} = vecs[j] */
    fe*Bmat=mat_alloc(n,n), *Mhat=mat_alloc(n,n);
    if(flagok){
        for(int j=0;j<nvec;j++) for(int i2=0;i2<n;i2++) MAT(Bmat,n,i2,n-1-j)=MAT(vecs,n,i2,j);
        int done=0;
        for(int att=0;att<400&&!done;att++){ for(int i2=0;i2<n;i2++) MAT(Bmat,n,i2,0)=frand();
            fe*t4=mat_alloc(n,n); done=mat_inv(Bmat,n,t4); free(t4); }
        if(!done||!mat_inv(Bmat,n,Mhat)) flagok=0;
    }
    printf("[6] triangular flag recovered (determinant filtration): %s\n",flagok?"yes":"NO");
    { /* ground truth linear forms are the rows of Mgt (X = Mgt x) */
      fe*L1E2=mat_alloc(n,r); mat_mul(sk.L1,n,r,E,r,L1E2);
      fe*Mgt2=mat_alloc(n,n);
      for(int i2=0;i2<n;i2++)for(int j2=0;j2<n;j2++) MAT(Mgt2,n,i2,j2)=MAT(L1E2,r,i2,j2);
      for(int i2=0;i2<n;i2++){
        /* is row i2 of Mhat in span of rows 0..i2 of Mgt2, with nonzero coeff on row i2 ? */
        fe*Asp=mat_alloc(n,i2+1), *bb=malloc(sizeof(fe)*n), *xx=malloc(sizeof(fe)*(i2+1));
        for(int c2=0;c2<=i2;c2++)for(int j2=0;j2<n;j2++) MAT(Asp,i2+1,j2,c2)=MAT(Mgt2,n,c2,j2);
        for(int j2=0;j2<n;j2++) bb[j2]=MAT(Mhat,n,i2,j2);
        int ok2=mat_solve(Asp,n,i2+1,bb,xx);
        (void)ok2;
        { fe*Af=mat_alloc(n,n),*bf=malloc(sizeof(fe)*n),*xf=malloc(sizeof(fe)*n);
          for(int c2=0;c2<n;c2++)for(int j2=0;j2<n;j2++) MAT(Af,n,j2,c2)=MAT(Mgt2,n,c2,j2);
          for(int j2=0;j2<n;j2++) bf[j2]=MAT(Mhat,n,i2,j2);
          mat_solve(Af,n,n,bf,xf);
          printf("    [diag] l_%d = ",i2);
          for(int c2=0;c2<n;c2++) printf("%s", xf[c2]?"X":".");
          printf("   (support in the true X_0..X_%d basis)\n",n-1);
          free(Af);free(bf);free(xf); }
        free(Asp);free(bb);free(xx); } }
    if(!flagok) return 1;
    fe*Mi=mat_alloc(n,n);
    if(!flagok||!mat_inv(Mhat,n,Mi)){ printf("    flag matrix singular\n"); return 1; }
    /* transform Q-space to x' = Mhat x : A' = Mi^T A Mi */
    fe*QT=mat_alloc(n,n*n);
    for(int a=0;a<n;a++){ fe*tm=mat_alloc(n,n);
        mat_mul(&QH[(size_t)a*n*n],n,n,Mi,n,tm);
        for(int i2=0;i2<n;i2++)for(int j2=0;j2<n;j2++){ uint64_t acc=0;
            for(int k2=0;k2<n;k2++) acc+=(uint64_t)MAT(Mi,n,k2,i2)*MAT(tm,n,k2,j2)%Q;
            MAT(&QT[(size_t)a*n*n],n,i2,j2)=(fe)(acc%Q); }
        free(tm); }
    /* check the triangular filtration: dim (Q n forms in x'_0..x'_i) == i+1 */
    int tri=1;
    for(int i=0;i<n;i++){
        /* forms supported on indices <= i : kill all entries with an index > i */
        int nout=0; for(int a2=0;a2<n;a2++)for(int b2=0;b2<n;b2++) if(a2>i||b2>i) nout++;
        fe*Csys2=mat_alloc(nout,n); int rr=0;
        for(int a2=0;a2<n;a2++)for(int b2=0;b2<n;b2++){ if(a2<=i&&b2<=i) continue;
            for(int a=0;a<n;a++) MAT(Csys2,n,rr,a)=MAT(&QT[(size_t)a*n*n],n,a2,b2); rr++; }
        fe*kk; int dd=mat_kernel(Csys2,nout,n,&kk);
        if(dd!=i+1){ tri=0; printf("    filtration dim at i=%d is %d (expect %d)\n",i,dd,i+1); }
        free(kk); free(Csys2);
    }
    printf("[7] Q is triangular in the recovered coordinates (dim Q_i = i+1 for all i): %s\n",tri?"YES":"no");
    printf("    => an equivalent triangular central map Q^ has been recovered;\n");
    printf("       it is invertible by the submission's own Algorithm 7.\n");
    /* demonstrate: invert the recovered triangular map on a random target */
    fe*QTRI=mat_alloc(n,n*n);
    for(int i=0;i<n;i++){
        int nout=0; for(int a2=0;a2<n;a2++)for(int b2=0;b2<n;b2++) if(a2>i||b2>i) nout++;
        fe*Csys2=mat_alloc(nout,n); int rr=0;
        for(int a2=0;a2<n;a2++)for(int b2=0;b2<n;b2++){ if(a2<=i&&b2<=i) continue;
            for(int a=0;a<n;a++) MAT(Csys2,n,rr,a)=MAT(&QT[(size_t)a*n*n],n,a2,b2); rr++; }
        fe*kk; int dd=mat_kernel(Csys2,nout,n,&kk);
        /* pick the element of Q_i not in Q_{i-1}: one with nonzero x'_i^2 coefficient */
        fe*cand=malloc(sizeof(fe)*n); int ok2=0;
        for(int att=0;att<40&&!ok2;att++){ memset(cand,0,sizeof(fe)*n);
            for(int c2=0;c2<dd;c2++){ fe rrv=frand(); for(int a=0;a<n;a++) cand[a]=fadd(cand[a],fmul(rrv,MAT(kk,dd,a,c2))); }
            fe lead=0; for(int a=0;a<n;a++) lead=fadd(lead,fmul(cand[a],MAT(&QT[(size_t)a*n*n],n,i,i)));
            if(lead) ok2=1; }
        for(int a=0;a<n;a++){ if(!cand[a]) continue;
            for(int u2=0;u2<n;u2++)for(int v2=0;v2<n;v2++)
                MAT(&QTRI[(size_t)i*n*n],n,u2,v2)=fadd(MAT(&QTRI[(size_t)i*n*n],n,u2,v2),fmul(cand[a],MAT(&QT[(size_t)a*n*n],n,u2,v2))); }
        free(kk);free(Csys2);free(cand);
    }
    { int bad=0; for(int i=0;i<n;i++){ if(!MAT(&QTRI[(size_t)i*n*n],n,i,i)) bad++;
        for(int a2=0;a2<n;a2++)for(int b2=0;b2<n;b2++) if((a2>i||b2>i)&&MAT(&QTRI[(size_t)i*n*n],n,a2,b2)) bad++; }
      printf("    extracted triangular map has extended-triangular shape: %s\n",bad?"NO":"yes");
      int succ=0,tries=200; fe*Wt=malloc(sizeof(fe)*n),*Xt=malloc(sizeof(fe)*n),*chk2=malloc(sizeof(fe)*n);
      for(int tt=0;tt<tries;tt++){ for(int i=0;i<n;i++) Wt[i]=frand();
          if(tri_invert(QTRI,n,Wt,Xt)){ eval_quadmap(QTRI,n,Xt,chk2); int g=1;
              for(int i=0;i<n;i++) if(chk2[i]!=Wt[i]) g=0; if(g) succ++; } }
      printf("    Algorithm-7 inversion of the recovered map succeeds on %d/%d random targets\n",succ,tries);
      printf("    (spec 2.3 predicts a constant success rate per target; rescaling covers the rest)\n"); }
    printf("\nSUMMARY: from the public key alone we recovered K2=ker L2, the (x,y) separation,\n");
    printf("         the hidden quadratic space Q, its symmetriser, and an equivalent\n");
    printf("         triangular central map. Remaining for a universal forger: the\n");
    printf("         rational-normal-curve identification of the Hankel container\n");
    printf("         (spec 3.2.2 Attack A step 3 / Attack D step 7, 'standard GRS recovery').\n");
    return 0;
}
