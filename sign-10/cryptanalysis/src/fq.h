/* fq.h - F_q arithmetic, RNG, dense linear algebra, polynomials over F_q */
#ifndef FQ_H
#define FQ_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static uint32_t Q = 65519;           /* prime, q = 3 mod 4 */
typedef uint32_t fe;

static inline fe fadd(fe a, fe b){ uint32_t s=a+b; return s>=Q? s-Q : s; }
static inline fe fsub(fe a, fe b){ return a>=b? a-b : a+Q-b; }
static inline fe fneg(fe a){ return a? Q-a : 0; }
static inline fe fmul(fe a, fe b){ return (fe)(((uint64_t)a*b)%Q); }
static fe fpow(fe a, uint64_t e){ fe r=1; while(e){ if(e&1) r=fmul(r,a); a=fmul(a,a); e>>=1;} return r; }
static inline fe finv(fe a){ return fpow(a, Q-2); }
/* q = 3 mod 4 : sqrt by exponentiation, returns 0/1 for existence */
static int fsqrt(fe d, fe *out){
    if(d==0){ *out=0; return 1; }
    if(fpow(d,(Q-1)/2)!=1) return 0;
    *out = fpow(d,(Q+1)/4); return 1;
}

/* ---- rng (splitmix64) ---- */
static uint64_t RNG_STATE = 0x9E3779B97F4A7C15ull;
static uint64_t rng64(void){
    uint64_t z=(RNG_STATE+=0x9E3779B97F4A7C15ull);
    z=(z^(z>>30))*0xBF58476D1CE4E5B9ull; z=(z^(z>>27))*0x94D049BB133111EBull;
    return z^(z>>31);
}
static void rng_seed(uint64_t s){ RNG_STATE = s? s : 0x9E3779B97F4A7C15ull; }
static inline fe frand(void){ return (fe)(rng64()%Q); }
static inline fe frand_nz(void){ fe x; do{ x=frand(); }while(!x); return x; }

/* ---- dense matrices, row-major, fe ---- */
static fe* mat_alloc(int r,int c){ fe*m=calloc((size_t)r*c,sizeof(fe)); if(!m){fprintf(stderr,"oom\n");exit(1);} return m; }
#define MAT(m,cols,i,j) ((m)[(size_t)(i)*(cols)+(j)])

static void mat_rand(fe*m,int r,int c){ for(size_t i=0;i<(size_t)r*c;i++) m[i]=frand(); }

/* rref in place; returns rank; pivcol[] receives pivot column of each pivot row */
static int mat_rref(fe*m,int r,int c,int*pivcol){
    int row=0;
    for(int col=0; col<c && row<r; col++){
        int p=-1; for(int i=row;i<r;i++) if(MAT(m,c,i,col)){p=i;break;}
        if(p<0) continue;
        if(p!=row) for(int j=0;j<c;j++){ fe t=MAT(m,c,row,j); MAT(m,c,row,j)=MAT(m,c,p,j); MAT(m,c,p,j)=t; }
        fe iv=finv(MAT(m,c,row,col));
        for(int j=col;j<c;j++) MAT(m,c,row,j)=fmul(MAT(m,c,row,j),iv);
        for(int i=0;i<r;i++){ if(i==row) continue; fe f=MAT(m,c,i,col); if(!f) continue;
            for(int j=col;j<c;j++) MAT(m,c,i,j)=fsub(MAT(m,c,i,j), fmul(f,MAT(m,c,row,j))); }
        if(pivcol) pivcol[row]=col;
        row++;
    }
    return row;
}
static int mat_rank(const fe*m,int r,int c){ fe*t=mat_alloc(r,c); memcpy(t,m,(size_t)r*c*sizeof(fe)); int k=mat_rref(t,r,c,NULL); free(t); return k; }

/* kernel basis of m (r x c): returns k = dim, ker is c x k (columns are basis vectors) */
static int mat_kernel(const fe*m,int r,int c, fe**ker_out){
    fe*t=mat_alloc(r,c); memcpy(t,m,(size_t)r*c*sizeof(fe));
    int*piv=malloc(sizeof(int)*(r>c?r:c));
    int rk=mat_rref(t,r,c,piv);
    int k=c-rk; fe*ker=mat_alloc(c,k>0?k:1);
    char*ispiv=calloc(c,1); for(int i=0;i<rk;i++) ispiv[piv[i]]=1;
    int col=0;
    for(int f=0;f<c;f++){ if(ispiv[f]) continue;
        MAT(ker,k,f,col)=1;
        for(int i=0;i<rk;i++) MAT(ker,k,piv[i],col)=fneg(MAT(t,c,i,f));
        col++; }
    free(t);free(piv);free(ispiv); *ker_out=ker; return k;
}
/* inverse; 0 on singular */
static int mat_inv(const fe*a,int n,fe*out){
    fe*t=mat_alloc(n,2*n);
    for(int i=0;i<n;i++){ for(int j=0;j<n;j++) MAT(t,2*n,i,j)=MAT(a,n,i,j); MAT(t,2*n,i,n+i)=1; }
    int rk=mat_rref(t,n,2*n,NULL);
    if(rk<n){ free(t); return 0; }
    for(int i=0;i<n;i++) for(int j=0;j<n;j++) MAT(out,n,i,j)=MAT(t,2*n,i,n+j);
    free(t); return 1;
}
static void mat_rand_inv(fe*a,int n){ do{ mat_rand(a,n,n); } while(!({fe*tmp=mat_alloc(n,n); int ok=mat_inv(a,n,tmp); free(tmp); ok;})); }
static void mat_mul(const fe*a,int ar,int ac,const fe*b,int bc,fe*out){
    for(int i=0;i<ar;i++) for(int j=0;j<bc;j++){ uint64_t s=0;
        for(int k=0;k<ac;k++) s+=(uint64_t)MAT(a,ac,i,k)*MAT(b,bc,k,j)%Q;
        MAT(out,bc,i,j)=(fe)(s%Q); }
}
/* solve A x = b, A is r x c; returns 1 and one solution in x (c), or 0 */
static int mat_solve(const fe*A,int r,int c,const fe*b,fe*x){
    fe*t=mat_alloc(r,c+1);
    for(int i=0;i<r;i++){ for(int j=0;j<c;j++) MAT(t,c+1,i,j)=MAT(A,c,i,j); MAT(t,c+1,i,c)=b[i]; }
    int*piv=malloc(sizeof(int)*r); int rk=mat_rref(t,r,c+1,piv);
    for(int i=0;i<rk;i++) if(piv[i]==c){ free(t);free(piv); return 0; }
    memset(x,0,sizeof(fe)*c);
    for(int i=0;i<rk;i++) x[piv[i]]=MAT(t,c+1,i,c);
    free(t);free(piv); return 1;
}

/* ---- polynomials over F_q: fe* with explicit length (deg+1), normalised ---- */
typedef struct { fe*c; int d; } poly;   /* d = degree, -1 for zero */
static poly pnew(int d){ poly p; p.d=d; p.c=calloc(d<0?1:d+1,sizeof(fe)); return p; }
static void pfree(poly*p){ free(p->c); p->c=NULL; p->d=-1; }
static void pnorm(poly*p){ while(p->d>=0 && p->c[p->d]==0) p->d--; }
static poly pcopy(const poly*a){ poly r=pnew(a->d); if(a->d>=0) memcpy(r.c,a->c,(a->d+1)*sizeof(fe)); return r; }
static poly pmul(const poly*a,const poly*b){
    if(a->d<0||b->d<0) return pnew(-1);
    poly r=pnew(a->d+b->d);
    for(int i=0;i<=a->d;i++){ if(!a->c[i]) continue;
        for(int j=0;j<=b->d;j++) r.c[i+j]=fadd(r.c[i+j],fmul(a->c[i],b->c[j])); }
    pnorm(&r); return r;
}
static void pdivmod(const poly*a,const poly*b,poly*qo,poly*ro){
    assert(b->d>=0);
    poly r=pcopy(a);
    poly qq=pnew(a->d-b->d>=0? a->d-b->d : -1);
    fe ib=finv(b->c[b->d]);
    for(int i=a->d-b->d;i>=0;i--){
        if(r.d < i+b->d || !r.c[i+b->d]){ continue; }
        fe f=fmul(r.c[i+b->d],ib); qq.c[i]=f;
        for(int j=0;j<=b->d;j++) r.c[i+j]=fsub(r.c[i+j],fmul(f,b->c[j]));
    }
    pnorm(&r); pnorm(&qq);
    if(qo)*qo=qq; else pfree(&qq);
    if(ro)*ro=r; else pfree(&r);
}
static poly pmod(const poly*a,const poly*b){ poly r; pdivmod(a,b,NULL,&r); return r; }
static poly pgcd(const poly*a,const poly*b){
    poly x=pcopy(a), y=pcopy(b);
    while(y.d>=0){ poly r=pmod(&x,&y); pfree(&x); x=y; y=r; }
    if(x.d>=0){ fe iv=finv(x.c[x.d]); for(int i=0;i<=x.d;i++) x.c[i]=fmul(x.c[i],iv); }
    return x;
}
static poly pderiv(const poly*a){ if(a->d<1) return pnew(-1); poly r=pnew(a->d-1);
    for(int i=1;i<=a->d;i++) r.c[i-1]=fmul(a->c[i],(fe)(i%Q)); pnorm(&r); return r; }
/* a^e mod f */
static poly ppowmod(const poly*a,uint64_t e,const poly*f){
    poly base=pmod(a,f); poly r=pnew(0); r.c[0]=1;
    while(e){ if(e&1){ poly t=pmul(&r,&base); poly t2=pmod(&t,f); pfree(&t); pfree(&r); r=t2; }
        poly s=pmul(&base,&base); poly s2=pmod(&s,f); pfree(&s); pfree(&base); base=s2; e>>=1; }
    return r;
}
static void pmonic(poly*a){ if(a->d<0) return; fe iv=finv(a->c[a->d]); for(int i=0;i<=a->d;i++) a->c[i]=fmul(a->c[i],iv); }
#endif
