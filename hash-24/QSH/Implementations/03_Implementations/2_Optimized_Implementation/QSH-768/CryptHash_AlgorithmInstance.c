/*
 * QuantaSylva Hash (QSH) -- x86-64 AVX2 optimized implementation.
 *
 * Implements CryptHash() for QSH-512 / QSH-768 / QSH-1024 (requires AVX2).
 *
 * Strategy: WITHIN-PERMUTATION SIMD.  The 16 independent G's of every layer
 * are evaluated together across 16 SIMD lanes, so one ChaCha-Bahru permutation
 * is computed with its whole 64-word state held in registers (no spilling):
 *   - state laid out as four 16-lane quantities A,B,C,D = the x0-planes
 *     (w=32: 2 YMM each = 8 YMM; w=64: 4 YMM each);
 *   - layer 0 = G(A,B,C,D);
 *   - layer 1 (diagonal in x1) = cheap in-lane rotations of B,C,D, then G,
 *     then the inverse rotations (the folded P16, like ChaCha's diagonal step);
 *   - layer 2 (mix along x1, diagonal in x2) = a 4x4 transpose to the x1-planes,
 *     block rotations, G, inverse rotations, transpose back.
 * 32/16/8-bit rotations use single vpshufb/vpshufd where possible.
 * The mode (padding, chunk loop, binary tree) is scalar; H^(0) is precomputed.
 *
 * Bit-identical to the reference implementation for all variants and sizes.
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <immintrin.h>
#include "CryptHash_AlgorithmInstance.h"

#define V 32u
#define RHO 16u
#define HALF 16u
#define F_CS 1u
#define F_CE 2u
#define F_PA 4u
#define F_RO 8u

static const uint32_t H0_512[64]={0x07499a01u,0x28746253u,0xec9be175u,0x29c5e753u,0x5f065f76u,0x95566f10u,0x32363d9cu,0xc083cb7eu,0x71ba5034u,0x273fe3edu,0x3ef08910u,0xef403f0du,0xbaa9ee95u,0xab16ac7au,0xc0e1f853u,0x07d483f5u,0xe851d482u,0x3066224cu,0xe31345b1u,0x98929133u,0x3da6d626u,0x1fea4a5au,0x4a2a9612u,0x20036ca0u,0xbfd93dbau,0xffb71434u,0x136bb97du,0x704a78a3u,0x63915722u,0xc9e507d8u,0x73696945u,0x29da7516u,0xe99f34aau,0x715beea6u,0xc1771d45u,0x9c1bb1eau,0xba767c7fu,0x63242a92u,0x8963e450u,0x012ec042u,0x4a71f1bdu,0xde680cd3u,0x358659f7u,0x709f5edfu,0x3d1fea68u,0xdc8f7c6cu,0x62e1e479u,0xdbc59ee1u,0x9de969b8u,0x17b2be2fu,0x0e6f5437u,0xa2ea29f9u,0x7f696c63u,0x0259d217u,0x2bc788cfu,0x7e63e73fu,0x7564d669u,0x3c4b2befu,0x9b5cde7du,0x25d22d2du,0x91751019u,0x6dd4e47au,0xdd99806fu,0x7a4f3e1au};
static const uint64_t H0_768[64]={0xefff9fecc1ecc495ULL,0xcd2aec670a91dfefULL,0x7d5b0a07ba56336bULL,0xfa8054dcd08e0741ULL,0x34e2e3dc742233bdULL,0xf02bc7e19f0b6962ULL,0x48af3a75acb7c5b5ULL,0xb59bf623dbdeff5dULL,0xf11b1a41072478ffULL,0x6a466260d2f99e59ULL,0xd55cdbf74ce5975fULL,0x12e29535c968a558ULL,0x248be0fa563db080ULL,0x7907a73fa6c6bb92ULL,0x389c37ff35f07448ULL,0xd72852dd9463bb69ULL,0x6ecbe430d7bfa876ULL,0x00f86ee90bf74b89ULL,0xda192346c407f3b5ULL,0x1f87ec34ec7b9401ULL,0x351a7170edda6f41ULL,0x6a32c6670c96fd62ULL,0x45a08c6d51275512ULL,0x0c6774f2a3476c7fULL,0xcde2bea163ed6d86ULL,0xea1da0f47f946af2ULL,0x47937bc661a549d3ULL,0x6c6a8bbeb1ebd7a1ULL,0x5796e6906355154cULL,0xd65cf7dd8c39b51cULL,0x5b669afed56024bbULL,0x18c44120821f71adULL,0x7196c81e418f29c8ULL,0x6dbcddf4342d5cfaULL,0x2577aac7b0c88423ULL,0x2b7dbc9d943646e6ULL,0x5b169c1f837bdb76ULL,0xd213634269b0d2bdULL,0xbf47e0e88d06f9bbULL,0xcd6a07b0f2c54cf5ULL,0x7c2c870659ede14fULL,0x5d47d0d5c971d7fbULL,0xa364ef6de39187c5ULL,0xbdc2db5804f31b05ULL,0x349c7e380f1cf1f7ULL,0xcab63e2d7280eb9aULL,0x7e64c89d0d98cc78ULL,0x1f242944ad66a250ULL,0x7fd497a4f8034eebULL,0x3ffcbad2953273fbULL,0x41c05fa96e18a88fULL,0x922587c14e767659ULL,0xf86ed849af0c3a6fULL,0xf3730714eba5ab7cULL,0x81e4845cd4129113ULL,0x58e651a96ebdb5d1ULL,0x6903175ae7801895ULL,0xcccaaacc505fbde1ULL,0xfe5aa2cdd34b6ff0ULL,0x80692e7501af749dULL,0xc403004f07afead2ULL,0x39a4ee29a0e48427ULL,0xb9fb4bbdd192944aULL,0x20e15af064677a95ULL};
static const uint64_t H0_1024[64]={0xade288ae275b9709ULL,0x3c1836f1bf4fe859ULL,0x22dae4f0febdf98aULL,0x132d144e72f7de56ULL,0xd1ce8d5e7656ba34ULL,0x527128a1cb34cbc9ULL,0x895703c738183174ULL,0xa6a39bf1d232446cULL,0x9e43e594ec4bdf5fULL,0x15b65338129764f3ULL,0x51cd401afbed8c62ULL,0x3366bef907b39379ULL,0xfba27ab4c6b84714ULL,0x9d5d79cb6e3f0829ULL,0xa67d0bf3e7df0916ULL,0x816126f23752bdc9ULL,0x358befb3a7af4eabULL,0x4cbb82b5f9ad4937ULL,0xf5a7fb1454401118ULL,0xa7b7a11d2126a68eULL,0xa6f96c97653ccaedULL,0xc4206508a19b3daeULL,0xcee436e9615ae41eULL,0x2082c51de0d8c9fbULL,0x2ab892bd781dfe0eULL,0x889c7d7afd19c93aULL,0xa8f68bee3fa5b27fULL,0x157e42b9317c97ceULL,0x674fbf0a522de44aULL,0x825933bb8219f38aULL,0x5c4497f950344f82ULL,0x518116af0f1cfafbULL,0x7fa243448f13a2e4ULL,0x114d929240596e86ULL,0xf45c4b94ca52155cULL,0x8cad611cfcfbe3b0ULL,0x913955c2fa634d0fULL,0x4dfe2edacdbbc2b2ULL,0x628e563f3d9a61e0ULL,0x50e7e111f60640dfULL,0xf8e109ddb674a14dULL,0x48ef6f273528c2e2ULL,0xcd652f9ecba65bceULL,0x843c6cd2e27e7f1bULL,0x15b0b137ef7afea4ULL,0x02b3becc163852aaULL,0x6773a26b25405f7cULL,0xc8b694b41dfae741ULL,0xd0afcdb238b2c406ULL,0x6c1c56c6f926700aULL,0xa158c1ecda551aacULL,0x57cb72b48bc94e44ULL,0x182df1a84e93bc91ULL,0x4f04c7ed4fab754aULL,0x2c8c2cff95c15d7fULL,0x88cf38ae8d89a171ULL,0xde1fb2886c484702ULL,0xb1b265cbe51c57d1ULL,0x28e64cdf8b996c44ULL,0x8fc58f5c0b5bab1fULL,0x798c5c6eeed3052bULL,0xd3593f63c706d85fULL,0xb59313aa5fbacf06ULL,0x94e725d325bf1aceULL};

#define XR _mm256_xor_si256

/* ===================== within-permutation, w=32 ========================= */
typedef struct { __m256i lo, hi; } v16;
static inline __m256i rl32(__m256i x,int r){ return _mm256_or_si256(_mm256_slli_epi32(x,r),_mm256_srli_epi32(x,32-r)); }
static inline __m256i s16_32(__m256i x){ const __m256i m=_mm256_setr_epi8(2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13,2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13); return _mm256_shuffle_epi8(x,m);}
static inline __m256i s8_32(__m256i x){ const __m256i m=_mm256_setr_epi8(3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14,3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14); return _mm256_shuffle_epi8(x,m);}
static inline v16 a32(v16 a,v16 b){v16 r;r.lo=_mm256_add_epi32(a.lo,b.lo);r.hi=_mm256_add_epi32(a.hi,b.hi);return r;}
static inline v16 x32(v16 a,v16 b){v16 r;r.lo=XR(a.lo,b.lo);r.hi=XR(a.hi,b.hi);return r;}
static inline v16 rr32(v16 a,int r){v16 x;x.lo=rl32(a.lo,r);x.hi=rl32(a.hi,r);return x;}
static inline v16 vs16(v16 a){v16 x;x.lo=s16_32(a.lo);x.hi=s16_32(a.hi);return x;}
static inline v16 vs8(v16 a){v16 x;x.lo=s8_32(a.lo);x.hi=s8_32(a.hi);return x;}
static inline void G32(v16*a,v16*b,v16*c,v16*d){
    *a=a32(*a,rr32(*b,1));  *d=vs16(x32(*d,*a));
    *c=a32(*c,rr32(*d,15)); *b=rr32(x32(*b,*c),12);
    *a=a32(*a,*b);          *d=vs8(x32(*d,*a));
    *c=a32(*c,vs8(*d));     *b=rr32(x32(*b,*c),7);
}
/* shuffle control must be a compile-time constant (clang is strict), so branch
 * on k and pass literal immediates instead of a runtime variable. */
static inline v16 dx1_32(v16 a,int k){ v16 r;
    if(k==1){ r.lo=_mm256_shuffle_epi32(a.lo,_MM_SHUFFLE(0,3,2,1)); r.hi=_mm256_shuffle_epi32(a.hi,_MM_SHUFFLE(0,3,2,1)); }
    else if(k==2){ r.lo=_mm256_shuffle_epi32(a.lo,_MM_SHUFFLE(1,0,3,2)); r.hi=_mm256_shuffle_epi32(a.hi,_MM_SHUFFLE(1,0,3,2)); }
    else { r.lo=_mm256_shuffle_epi32(a.lo,_MM_SHUFFLE(2,1,0,3)); r.hi=_mm256_shuffle_epi32(a.hi,_MM_SHUFFLE(2,1,0,3)); }
    return r;}
static inline v16 dx2_32(v16 a,int k){v16 r;
    if(k==1){r.lo=_mm256_permute2x128_si256(a.lo,a.hi,0x21);r.hi=_mm256_permute2x128_si256(a.hi,a.lo,0x21);}
    else if(k==2){r.lo=a.hi;r.hi=a.lo;}
    else{r.lo=_mm256_permute2x128_si256(a.hi,a.lo,0x21);r.hi=_mm256_permute2x128_si256(a.lo,a.hi,0x21);}return r;}
#define TR32(A,B,C,D,P,Q,R,S) do{__m256i _0=_mm256_unpacklo_epi32(A,B),_1=_mm256_unpackhi_epi32(A,B),_2=_mm256_unpacklo_epi32(C,D),_3=_mm256_unpackhi_epi32(C,D);P=_mm256_unpacklo_epi64(_0,_2);Q=_mm256_unpackhi_epi64(_0,_2);R=_mm256_unpacklo_epi64(_1,_3);S=_mm256_unpackhi_epi64(_1,_3);}while(0)
static inline void tp32(v16*A,v16*B,v16*C,v16*D){v16 P,Q,R,S;
    TR32(A->lo,B->lo,C->lo,D->lo,P.lo,Q.lo,R.lo,S.lo);TR32(A->hi,B->hi,C->hi,D->hi,P.hi,Q.hi,R.hi,S.hi);*A=P;*B=Q;*C=R;*D=S;}
static void E3_32(uint32_t*s){
    v16 A,B,C,D;
    A.lo=_mm256_loadu_si256((const __m256i*)(s+0));A.hi=_mm256_loadu_si256((const __m256i*)(s+8));
    B.lo=_mm256_loadu_si256((const __m256i*)(s+16));B.hi=_mm256_loadu_si256((const __m256i*)(s+24));
    C.lo=_mm256_loadu_si256((const __m256i*)(s+32));C.hi=_mm256_loadu_si256((const __m256i*)(s+40));
    D.lo=_mm256_loadu_si256((const __m256i*)(s+48));D.hi=_mm256_loadu_si256((const __m256i*)(s+56));
    for(int r=0;r<9;r++){
        G32(&A,&B,&C,&D);
        B=dx1_32(B,1);C=dx1_32(C,2);D=dx1_32(D,3);G32(&A,&B,&C,&D);B=dx1_32(B,3);C=dx1_32(C,2);D=dx1_32(D,1);
        tp32(&A,&B,&C,&D);B=dx2_32(B,1);C=dx2_32(C,2);D=dx2_32(D,3);G32(&A,&B,&C,&D);B=dx2_32(B,3);C=dx2_32(C,2);D=dx2_32(D,1);tp32(&A,&B,&C,&D);
    }
    G32(&A,&B,&C,&D);
    _mm256_storeu_si256((__m256i*)(s+0),A.lo);_mm256_storeu_si256((__m256i*)(s+8),A.hi);
    _mm256_storeu_si256((__m256i*)(s+16),B.lo);_mm256_storeu_si256((__m256i*)(s+24),B.hi);
    _mm256_storeu_si256((__m256i*)(s+32),C.lo);_mm256_storeu_si256((__m256i*)(s+40),C.hi);
    _mm256_storeu_si256((__m256i*)(s+48),D.lo);_mm256_storeu_si256((__m256i*)(s+56),D.hi);
}

/* ===================== within-permutation, w=64 ========================= */
typedef struct { __m256i y[4]; } w16;
static inline __m256i rl64(__m256i x,int r){ return _mm256_or_si256(_mm256_slli_epi64(x,r),_mm256_srli_epi64(x,64-r)); }
static inline __m256i s32_64(__m256i x){ return _mm256_shuffle_epi32(x,_MM_SHUFFLE(2,3,0,1)); }
static inline __m256i s16_64(__m256i x){ const __m256i m=_mm256_setr_epi8(6,7,0,1,2,3,4,5,14,15,8,9,10,11,12,13,6,7,0,1,2,3,4,5,14,15,8,9,10,11,12,13); return _mm256_shuffle_epi8(x,m);}
static inline void G64(w16*a,w16*b,w16*c,w16*d){
    for(int i=0;i<4;i++){
        __m256i A=a->y[i],B=b->y[i],C=c->y[i],D=d->y[i];
        A=_mm256_add_epi64(A,rl64(B,1));   D=s32_64(XR(D,A));
        C=_mm256_add_epi64(C,rl64(D,31));  B=rl64(XR(B,C),28);
        A=_mm256_add_epi64(A,B);           D=s16_64(XR(D,A));
        C=_mm256_add_epi64(C,s16_64(D));   B=rl64(XR(B,C),15);
        a->y[i]=A;b->y[i]=B;c->y[i]=C;d->y[i]=D;
    }
}
static inline w16 dx1_64(w16 a,int k){ w16 r; int i;
    if(k==1) for(i=0;i<4;i++) r.y[i]=_mm256_permute4x64_epi64(a.y[i],_MM_SHUFFLE(0,3,2,1));
    else if(k==2) for(i=0;i<4;i++) r.y[i]=_mm256_permute4x64_epi64(a.y[i],_MM_SHUFFLE(1,0,3,2));
    else for(i=0;i<4;i++) r.y[i]=_mm256_permute4x64_epi64(a.y[i],_MM_SHUFFLE(2,1,0,3));
    return r;}
static inline w16 dx2_64(w16 a,int k){w16 r;
    for(int i=0;i<4;i++){ r.y[i]=a.y[(i+k)&3]; }
    return r;}
static inline void tp64(w16*A,w16*B,w16*C,w16*D){w16 P,Q,R,S;
    for(int i=0;i<4;i++){
        __m256i t0=_mm256_unpacklo_epi64(A->y[i],B->y[i]),t1=_mm256_unpackhi_epi64(A->y[i],B->y[i]);
        __m256i t2=_mm256_unpacklo_epi64(C->y[i],D->y[i]),t3=_mm256_unpackhi_epi64(C->y[i],D->y[i]);
        P.y[i]=_mm256_permute2x128_si256(t0,t2,0x20);Q.y[i]=_mm256_permute2x128_si256(t1,t3,0x20);
        R.y[i]=_mm256_permute2x128_si256(t0,t2,0x31);S.y[i]=_mm256_permute2x128_si256(t1,t3,0x31);
    }*A=P;*B=Q;*C=R;*D=S;}
static void E3_64(uint64_t*s){
    w16 A,B,C,D;
    for(int i=0;i<4;i++){A.y[i]=_mm256_loadu_si256((const __m256i*)(s+0+i*4));B.y[i]=_mm256_loadu_si256((const __m256i*)(s+16+i*4));
        C.y[i]=_mm256_loadu_si256((const __m256i*)(s+32+i*4));D.y[i]=_mm256_loadu_si256((const __m256i*)(s+48+i*4));}
    for(int r=0;r<18;r++){
        G64(&A,&B,&C,&D);
        B=dx1_64(B,1);C=dx1_64(C,2);D=dx1_64(D,3);G64(&A,&B,&C,&D);B=dx1_64(B,3);C=dx1_64(C,2);D=dx1_64(D,1);
        tp64(&A,&B,&C,&D);B=dx2_64(B,1);C=dx2_64(C,2);D=dx2_64(D,3);G64(&A,&B,&C,&D);B=dx2_64(B,3);C=dx2_64(C,2);D=dx2_64(D,1);tp64(&A,&B,&C,&D);
    }
    G64(&A,&B,&C,&D);
    for(int i=0;i<4;i++){_mm256_storeu_si256((__m256i*)(s+0+i*4),A.y[i]);_mm256_storeu_si256((__m256i*)(s+16+i*4),B.y[i]);
        _mm256_storeu_si256((__m256i*)(s+32+i*4),C.y[i]);_mm256_storeu_si256((__m256i*)(s+48+i*4),D.y[i]);}
}

/* ===================== padding (byte access) ============================ */
static inline unsigned char hm(int n){return (unsigned char)((0xFFu<<(8-n))&0xFF);}
static unsigned char pbyte(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long i){
    unsigned long long f=L>>3;int r=(int)(L&7);
    if(i<f)return m[i];
    if(i==f){unsigned char b=0;if(r)b=(unsigned char)(m[i]&hm(r));b|=(unsigned char)(0x80u>>r);return b;}
    if(i>=tot-16){unsigned sh=(unsigned)(i-(tot-16))*8u;return sh>=64?0:(unsigned char)((L>>sh)&0xFF);}
    return 0;
}

/* ===================== mode: w=32 (QSH-512) ============================= */
static void F3_32(const uint32_t*H,const uint32_t*M,uint32_t fl,uint32_t*Ho){
    uint32_t S[64];unsigned j;
    for(j=0;j<V;j++){S[j]=H[j];S[V+j]=H[V+j]+M[j];}
    S[0]^=fl; E3_32(S);
    for(j=0;j<V;j++){Ho[j]=S[j]+M[j];Ho[V+j]=S[V+j];}
}
static void blk32(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long t,uint32_t*b){
    unsigned long long base=t*128ull;
    for(unsigned j=0;j<V;j++){uint32_t w=0;for(unsigned k=0;k<4;k++)w|=(uint32_t)pbyte(m,L,tot,base+j*4+k)<<(8*k);b[j]=w;}
}
static unsigned long long lp2(unsigned long long n){unsigned long long p=1;while(p*2<n)p*=2;return p;}
static void tree32(const uint32_t*cv,unsigned long long lo,unsigned long long hi,int root,uint32_t*out){
    if(hi-lo==1){memcpy(out,cv+lo*HALF,HALF*4);return;}
    unsigned long long ll=lp2(hi-lo);uint32_t L[16],R[16],M[32],I[64];
    tree32(cv,lo,lo+ll,0,L);tree32(cv,lo+ll,hi,0,R);
    memcpy(M,L,64);memcpy(M+16,R,64);F3_32(H0_512,M,F_PA|(root?F_RO:0),I);memcpy(out,I,64);
}
static int crypt512(const unsigned char*msg,unsigned long long L,unsigned char*dig){
    unsigned long long m=1024,tot=L+2*m-(L%m),N=tot/m,Pi=(N+RHO-1)/RHO;int only=(Pi==1);
    uint32_t*cv=(uint32_t*)malloc((size_t)Pi*HALF*4);if(!cv)return -2;
    for(unsigned long long c=0;c<Pi;c++){
        unsigned long long st=c*RHO,Lc=(st+RHO<=N)?RHO:(N-st);uint32_t H[64],Hn[64],b[32];memcpy(H,H0_512,256);
        for(unsigned long long i=1;i<=Lc;i++){uint32_t fl=0;if(i==1)fl|=F_CS;if(i==Lc)fl|=F_CE;if(i==Lc&&only)fl|=F_RO;
            blk32(msg,L,tot/8,st+(i-1),b);F3_32(H,b,fl,Hn);memcpy(H,Hn,256);}
        memcpy(cv+c*HALF,H,64);
    }
    uint32_t root[16];if(Pi==1)memcpy(root,cv,64);else tree32(cv,0,Pi,1,root);free(cv);
    for(int i=0;i<64;i++)dig[i]=(unsigned char)((root[i/4]>>(8*(i%4)))&0xFF);
    return 0;
}

/* ===================== mode: w=64 (QSH-768/1024) ======================== */
static void F3_64(const uint64_t*H,const uint64_t*M,uint64_t fl,uint64_t*Ho){
    uint64_t S[64];unsigned j;
    for(j=0;j<V;j++){S[j]=H[j];S[V+j]=H[V+j]+M[j];}
    S[0]^=fl; E3_64(S);
    for(j=0;j<V;j++){Ho[j]=S[j]+M[j];Ho[V+j]=S[V+j];}
}
static void blk64(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long t,uint64_t*b){
    unsigned long long base=t*256ull;
    for(unsigned j=0;j<V;j++){uint64_t w=0;for(unsigned k=0;k<8;k++)w|=(uint64_t)pbyte(m,L,tot,base+j*8+k)<<(8*k);b[j]=w;}
}
static void tree64(const uint64_t*cv,unsigned long long lo,unsigned long long hi,int root,uint64_t*out,const uint64_t*H0){
    if(hi-lo==1){memcpy(out,cv+lo*HALF,HALF*8);return;}
    unsigned long long ll=lp2(hi-lo);uint64_t L[16],R[16],M[32],I[64];
    tree64(cv,lo,lo+ll,0,L,H0);tree64(cv,lo+ll,hi,0,R,H0);
    memcpy(M,L,128);memcpy(M+16,R,128);F3_64(H0,M,F_PA|(root?F_RO:0),I);memcpy(out,I,128);
}
static int crypt64(const unsigned char*msg,unsigned long long L,unsigned char*dig,const uint64_t*H0,int n){
    unsigned long long m=2048,tot=L+2*m-(L%m),N=tot/m,Pi=(N+RHO-1)/RHO;int only=(Pi==1);
    uint64_t*cv=(uint64_t*)malloc((size_t)Pi*HALF*8);if(!cv)return -2;
    for(unsigned long long c=0;c<Pi;c++){
        unsigned long long st=c*RHO,Lc=(st+RHO<=N)?RHO:(N-st);uint64_t H[64],Hn[64],b[32];memcpy(H,H0,512);
        for(unsigned long long i=1;i<=Lc;i++){uint64_t fl=0;if(i==1)fl|=F_CS;if(i==Lc)fl|=F_CE;if(i==Lc&&only)fl|=F_RO;
            blk64(msg,L,tot/8,st+(i-1),b);F3_64(H,b,fl,Hn);memcpy(H,Hn,512);}
        memcpy(cv+c*HALF,H,128);
    }
    uint64_t root[16];if(Pi==1)memcpy(root,cv,128);else tree64(cv,0,Pi,1,root,H0);free(cv);
    int ob=n/8,produced=0;
    for(int j=0;j<16&&produced<ob;j++)for(int k=0;k<8&&produced<ob;k++)dig[produced++]=(unsigned char)((root[j]>>(8*k))&0xFF);
    return 0;
}

int CryptHash(int dbits,const unsigned char*msg,unsigned long long Lb,unsigned char*dig){
    switch(dbits){
    case 512:  return crypt512(msg,Lb,dig);
    case 768:  return crypt64(msg,Lb,dig,H0_768,768);
    case 1024: return crypt64(msg,Lb,dig,H0_1024,1024);
    default:   return -1;
    }
}
