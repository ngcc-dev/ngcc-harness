/* tensor.h - cubic forms as full symmetric 3-tensors, substitution, restriction */
#ifndef TENSOR_H
#define TENSOR_H
#include "facto.h"
#define T3(T,r,a,b,c) ((T)[((size_t)(a)*(r)+(b))*(r)+(c)])

/* public key coefficient table -> m symmetric tensors (r^3 each) */
static fe* pk_tensors(const pubkey*pk){
    int r=pk->P.r,m=pk->P.m,nc=NC3(r); c3_init(r);
    fe*T=mat_alloc(m,r*r*r);
    fe i3=finv(3),i6=finv(6);
    for(int j=0;j<m;j++){ fe*Tj=&T[(size_t)j*r*r*r];
        for(int a=0;a<r;a++)for(int b=a;b<r;b++)for(int c=b;c<r;c++){
            fe C=MAT(pk->C,nc,j,ic3(r,a,b,c)); if(!C) continue;
            if(a==b&&b==c) T3(Tj,r,a,a,a)=C;
            else if(a==b||b==c){ fe v=fmul(C,i3);
                int p[3]={a,b,c};
                for(int x=0;x<3;x++)for(int y=0;y<3;y++)for(int z=0;z<3;z++){
                    if(x==y||y==z||x==z) continue; T3(Tj,r,p[x],p[y],p[z])=v; }
                /* the 3 distinct permutations of a multiset {a,a,c} */
                T3(Tj,r,a,b,c)=v; T3(Tj,r,a,c,b)=v; T3(Tj,r,c,a,b)=v;
                T3(Tj,r,b,a,c)=v; T3(Tj,r,b,c,a)=v; T3(Tj,r,c,b,a)=v;
            } else { fe v=fmul(C,i6);
                int p[3]={a,b,c};
                int perm[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
                for(int t=0;t<6;t++) T3(Tj,r,p[perm[t][0]],p[perm[t][1]],p[perm[t][2]])=v; }
        } }
    return T;
}
static fe tensor_eval(const fe*T,int r,const fe*z){
    uint64_t s=0;
    for(int a=0;a<r;a++){ if(!z[a]) continue;
        for(int b=0;b<r;b++){ if(!z[b]) continue; fe zab=fmul(z[a],z[b]);
            uint64_t t=0; for(int c=0;c<r;c++) t+=(uint64_t)T3(T,r,a,b,c)*z[c]%Q;
            s+=(uint64_t)zab*(t%Q)%Q; } }
    return (fe)(s%Q);
}
/* T'(w) = T(Vw) : V is r x rout */
static void tensor_subst(const fe*T,int r,const fe*V,int ro,fe*out){
    fe*U1=mat_alloc(ro,r*r), *U2=mat_alloc(ro*ro,r);
    for(int i=0;i<ro;i++)for(int b=0;b<r;b++)for(int c=0;c<r;c++){ uint64_t s=0;
        for(int a=0;a<r;a++) s+=(uint64_t)T3(T,r,a,b,c)*MAT(V,ro,a,i)%Q;
        U1[((size_t)i*r+b)*r+c]=(fe)(s%Q); }
    for(int i=0;i<ro;i++)for(int j=0;j<ro;j++)for(int c=0;c<r;c++){ uint64_t s=0;
        for(int b=0;b<r;b++) s+=(uint64_t)U1[((size_t)i*r+b)*r+c]*MAT(V,ro,b,j)%Q;
        U2[((size_t)i*ro+j)*r+c]=(fe)(s%Q); }
    for(int i=0;i<ro;i++)for(int j=0;j<ro;j++)for(int k=0;k<ro;k++){ uint64_t s=0;
        for(int c=0;c<r;c++) s+=(uint64_t)U2[((size_t)i*ro+j)*r+c]*MAT(V,ro,c,k)%Q;
        T3(out,ro,i,j,k)=(fe)(s%Q); }
    free(U1);free(U2);
}
#endif
