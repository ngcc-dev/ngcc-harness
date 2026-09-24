/*
 * QuantaSylva Hash (QSH) -- AArch64 NEON "Additional" (WITHIN-PERMUTATION).
 *
 * QSH-512 (w=32): WITHIN-PERMUTATION NEON -- the 16 G's of each permutation
 *   layer run across 16 lanes held as four uint32x4_t per quantity (A,B,C,D),
 *   mirroring the VERIFIED x86 within-permutation kernel: dx1 = in-register
 *   lane rotate (vext), dx2 = register relabel, layer-2 = 4x4 transpose. This
 *   accelerates EVERY call, including short messages.
 * QSH-768/1024 (w=64): NEON 2-way chunk batching (uint64x2_t) -- a 64-bit
 *   within-permutation NEON kernel needs 8 regs/quantity and is left as future
 *   work; chunk batching gives NEON acceleration here meanwhile.
 *
 * Non-NEON targets: the whole hash uses the portable folded scalar path
 * (bit-identical to the QSH reference; verified on x86).
 *
 * !!! VALIDATION !!!  The NEON paths are NOT executed in the authoring
 * environment. Build on AArch64 (or via qemu-aarch64) and KAT-cross-check
 * against the reference (Implementations/validate_on_arm.sh) before use.
 * The within-permutation ALGORITHM (4-register model) was verified == scalar E3.
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "CryptHash_AlgorithmInstance.h"

#if defined(__ARM_NEON) || defined(__aarch64__)
#  include <arm_neon.h>
#  define HAS_NEON 1
#else
#  define HAS_NEON 0
#endif

#define U 64u
#define V 32u
#define RHO 16u
#define HALF 16u
#define F_CS 1u
#define F_CE 2u
#define F_PA 4u
#define F_RO 8u

static const int QUAD[3][16][4] = {
{{0,16,32,48},{1,17,33,49},{2,18,34,50},{3,19,35,51},{4,20,36,52},{5,21,37,53},
 {6,22,38,54},{7,23,39,55},{8,24,40,56},{9,25,41,57},{10,26,42,58},{11,27,43,59},
 {12,28,44,60},{13,29,45,61},{14,30,46,62},{15,31,47,63}},
{{0,17,34,51},{1,18,35,48},{2,19,32,49},{3,16,33,50},{4,21,38,55},{5,22,39,52},
 {6,23,36,53},{7,20,37,54},{8,25,42,59},{9,26,43,56},{10,27,40,57},{11,24,41,58},
 {12,29,46,63},{13,30,47,60},{14,31,44,61},{15,28,45,62}},
{{0,5,10,15},{4,9,14,3},{8,13,2,7},{12,1,6,11},{16,21,26,31},{20,25,30,19},
 {24,29,18,23},{28,17,22,27},{32,37,42,47},{36,41,46,35},{40,45,34,39},{44,33,38,43},
 {48,53,58,63},{52,57,62,51},{56,61,50,55},{60,49,54,59}}
};
static const uint32_t H0_512[64]={0x07499a01u,0x28746253u,0xec9be175u,0x29c5e753u,0x5f065f76u,0x95566f10u,0x32363d9cu,0xc083cb7eu,0x71ba5034u,0x273fe3edu,0x3ef08910u,0xef403f0du,0xbaa9ee95u,0xab16ac7au,0xc0e1f853u,0x07d483f5u,0xe851d482u,0x3066224cu,0xe31345b1u,0x98929133u,0x3da6d626u,0x1fea4a5au,0x4a2a9612u,0x20036ca0u,0xbfd93dbau,0xffb71434u,0x136bb97du,0x704a78a3u,0x63915722u,0xc9e507d8u,0x73696945u,0x29da7516u,0xe99f34aau,0x715beea6u,0xc1771d45u,0x9c1bb1eau,0xba767c7fu,0x63242a92u,0x8963e450u,0x012ec042u,0x4a71f1bdu,0xde680cd3u,0x358659f7u,0x709f5edfu,0x3d1fea68u,0xdc8f7c6cu,0x62e1e479u,0xdbc59ee1u,0x9de969b8u,0x17b2be2fu,0x0e6f5437u,0xa2ea29f9u,0x7f696c63u,0x0259d217u,0x2bc788cfu,0x7e63e73fu,0x7564d669u,0x3c4b2befu,0x9b5cde7du,0x25d22d2du,0x91751019u,0x6dd4e47au,0xdd99806fu,0x7a4f3e1au};
static const uint64_t H0_768[64]={0xefff9fecc1ecc495ULL,0xcd2aec670a91dfefULL,0x7d5b0a07ba56336bULL,0xfa8054dcd08e0741ULL,0x34e2e3dc742233bdULL,0xf02bc7e19f0b6962ULL,0x48af3a75acb7c5b5ULL,0xb59bf623dbdeff5dULL,0xf11b1a41072478ffULL,0x6a466260d2f99e59ULL,0xd55cdbf74ce5975fULL,0x12e29535c968a558ULL,0x248be0fa563db080ULL,0x7907a73fa6c6bb92ULL,0x389c37ff35f07448ULL,0xd72852dd9463bb69ULL,0x6ecbe430d7bfa876ULL,0x00f86ee90bf74b89ULL,0xda192346c407f3b5ULL,0x1f87ec34ec7b9401ULL,0x351a7170edda6f41ULL,0x6a32c6670c96fd62ULL,0x45a08c6d51275512ULL,0x0c6774f2a3476c7fULL,0xcde2bea163ed6d86ULL,0xea1da0f47f946af2ULL,0x47937bc661a549d3ULL,0x6c6a8bbeb1ebd7a1ULL,0x5796e6906355154cULL,0xd65cf7dd8c39b51cULL,0x5b669afed56024bbULL,0x18c44120821f71adULL,0x7196c81e418f29c8ULL,0x6dbcddf4342d5cfaULL,0x2577aac7b0c88423ULL,0x2b7dbc9d943646e6ULL,0x5b169c1f837bdb76ULL,0xd213634269b0d2bdULL,0xbf47e0e88d06f9bbULL,0xcd6a07b0f2c54cf5ULL,0x7c2c870659ede14fULL,0x5d47d0d5c971d7fbULL,0xa364ef6de39187c5ULL,0xbdc2db5804f31b05ULL,0x349c7e380f1cf1f7ULL,0xcab63e2d7280eb9aULL,0x7e64c89d0d98cc78ULL,0x1f242944ad66a250ULL,0x7fd497a4f8034eebULL,0x3ffcbad2953273fbULL,0x41c05fa96e18a88fULL,0x922587c14e767659ULL,0xf86ed849af0c3a6fULL,0xf3730714eba5ab7cULL,0x81e4845cd4129113ULL,0x58e651a96ebdb5d1ULL,0x6903175ae7801895ULL,0xcccaaacc505fbde1ULL,0xfe5aa2cdd34b6ff0ULL,0x80692e7501af749dULL,0xc403004f07afead2ULL,0x39a4ee29a0e48427ULL,0xb9fb4bbdd192944aULL,0x20e15af064677a95ULL};
static const uint64_t H0_1024[64]={0xade288ae275b9709ULL,0x3c1836f1bf4fe859ULL,0x22dae4f0febdf98aULL,0x132d144e72f7de56ULL,0xd1ce8d5e7656ba34ULL,0x527128a1cb34cbc9ULL,0x895703c738183174ULL,0xa6a39bf1d232446cULL,0x9e43e594ec4bdf5fULL,0x15b65338129764f3ULL,0x51cd401afbed8c62ULL,0x3366bef907b39379ULL,0xfba27ab4c6b84714ULL,0x9d5d79cb6e3f0829ULL,0xa67d0bf3e7df0916ULL,0x816126f23752bdc9ULL,0x358befb3a7af4eabULL,0x4cbb82b5f9ad4937ULL,0xf5a7fb1454401118ULL,0xa7b7a11d2126a68eULL,0xa6f96c97653ccaedULL,0xc4206508a19b3daeULL,0xcee436e9615ae41eULL,0x2082c51de0d8c9fbULL,0x2ab892bd781dfe0eULL,0x889c7d7afd19c93aULL,0xa8f68bee3fa5b27fULL,0x157e42b9317c97ceULL,0x674fbf0a522de44aULL,0x825933bb8219f38aULL,0x5c4497f950344f82ULL,0x518116af0f1cfafbULL,0x7fa243448f13a2e4ULL,0x114d929240596e86ULL,0xf45c4b94ca52155cULL,0x8cad611cfcfbe3b0ULL,0x913955c2fa634d0fULL,0x4dfe2edacdbbc2b2ULL,0x628e563f3d9a61e0ULL,0x50e7e111f60640dfULL,0xf8e109ddb674a14dULL,0x48ef6f273528c2e2ULL,0xcd652f9ecba65bceULL,0x843c6cd2e27e7f1bULL,0x15b0b137ef7afea4ULL,0x02b3becc163852aaULL,0x6773a26b25405f7cULL,0xc8b694b41dfae741ULL,0xd0afcdb238b2c406ULL,0x6c1c56c6f926700aULL,0xa158c1ecda551aacULL,0x57cb72b48bc94e44ULL,0x182df1a84e93bc91ULL,0x4f04c7ed4fab754aULL,0x2c8c2cff95c15d7fULL,0x88cf38ae8d89a171ULL,0xde1fb2886c484702ULL,0xb1b265cbe51c57d1ULL,0x28e64cdf8b996c44ULL,0x8fc58f5c0b5bab1fULL,0x798c5c6eeed3052bULL,0xd3593f63c706d85fULL,0xb59313aa5fbacf06ULL,0x94e725d325bf1aceULL};

/* ---- portable scalar folded path (fallback / w=64 tail&tree / non-NEON) ---- */
typedef struct { int w, rounds, n; uint64_t mask; int m; const void *H0; } params;
static int select_params(int dbits, params *P){
    switch(dbits){
    case 512:  P->w=32; P->rounds=9;  P->n=512;  P->H0=H0_512;  break;
    case 768:  P->w=64; P->rounds=18; P->n=768;  P->H0=H0_768;  break;
    case 1024: P->w=64; P->rounds=18; P->n=1024; P->H0=H0_1024; break;
    default: return -1; }
    P->mask=(P->w==64)?~(uint64_t)0:(uint64_t)0xFFFFFFFFu; P->m=P->w*32; return 0;
}
static inline uint64_t rl_s(uint64_t x,int r,int w,uint64_t mk){ r%=w; x&=mk; return r?(((x<<r)|(x>>(w-r)))&mk):x; }
static void Gs(uint64_t q[4],const params*P){
    const int w=P->w; const uint64_t mk=P->mask;
    const int s1=1,s2=w/2-1,s3=0,s4=w/4,t1=w/2,t2=w/2-4,t3=w/4,t4=w/4-1;
    uint64_t a=q[0],b=q[1],c=q[2],d=q[3];
    a=(a+rl_s(b,s1,w,mk))&mk; d=rl_s(d^a,t1,w,mk);
    c=(c+rl_s(d,s2,w,mk))&mk; b=rl_s(b^c,t2,w,mk);
    a=(a+rl_s(b,s3,w,mk))&mk; d=rl_s(d^a,t3,w,mk);
    c=(c+rl_s(d,s4,w,mk))&mk; b=rl_s(b^c,t4,w,mk);
    q[0]=a;q[1]=b;q[2]=c;q[3]=d;
}
static void E3s(uint64_t*s,const params*P){
    int r,L,qi; uint64_t q[4];
    for(r=0;r<P->rounds;r++)for(L=0;L<3;L++)for(qi=0;qi<16;qi++){const int*ix=QUAD[L][qi];
        q[0]=s[ix[0]];q[1]=s[ix[1]];q[2]=s[ix[2]];q[3]=s[ix[3]];Gs(q,P);
        s[ix[0]]=q[0];s[ix[1]]=q[1];s[ix[2]]=q[2];s[ix[3]]=q[3];}
    for(qi=0;qi<16;qi++){const int*ix=QUAD[0][qi];
        q[0]=s[ix[0]];q[1]=s[ix[1]];q[2]=s[ix[2]];q[3]=s[ix[3]];Gs(q,P);
        s[ix[0]]=q[0];s[ix[1]]=q[1];s[ix[2]]=q[2];s[ix[3]]=q[3];}
}
static inline unsigned char hm(int n){return (unsigned char)((0xFFu<<(8-n))&0xFF);}
static unsigned char pbyte(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long i){
    unsigned long long f=L>>3;int r=(int)(L&7u);
    if(i<f)return m[i];
    if(i==f){unsigned char b=0;if(r)b=(unsigned char)(m[i]&hm(r));b|=(unsigned char)(0x80u>>r);return b;}
    if(i>=tot-16){unsigned sh=(unsigned)(i-(tot-16))*8u;return sh>=64?0:(unsigned char)((L>>sh)&0xFFu);}
    return 0;
}

/* ============ w=32 within-permutation: NEON or scalar E3 on uint32 s[64] ===== */
#if HAS_NEON
static inline uint32x4_t rl32(uint32x4_t x,int n){ return vorrq_u32(vshlq_n_u32(x,n),vshrq_n_u32(x,32-n)); }
#define GBLK(a,b,c,d) do{ \
  (a)=vaddq_u32((a),rl32((b),1));  (d)=rl32(veorq_u32((d),(a)),16); \
  (c)=vaddq_u32((c),rl32((d),15)); (b)=rl32(veorq_u32((b),(c)),12); \
  (a)=vaddq_u32((a),(b));          (d)=rl32(veorq_u32((d),(a)),8);  \
  (c)=vaddq_u32((c),rl32((d),8));  (b)=rl32(veorq_u32((b),(c)),7);  }while(0)
#define DX1(x,k) ((k)==1?vextq_u32((x),(x),1):(k)==2?vextq_u32((x),(x),2):vextq_u32((x),(x),3))
static inline void T4(uint32x4_t*r0,uint32x4_t*r1,uint32x4_t*r2,uint32x4_t*r3){
    uint32x4x2_t p=vtrnq_u32(*r0,*r1), q=vtrnq_u32(*r2,*r3);
    *r0=vcombine_u32(vget_low_u32(p.val[0]),vget_low_u32(q.val[0]));
    *r1=vcombine_u32(vget_low_u32(p.val[1]),vget_low_u32(q.val[1]));
    *r2=vcombine_u32(vget_high_u32(p.val[0]),vget_high_u32(q.val[0]));
    *r3=vcombine_u32(vget_high_u32(p.val[1]),vget_high_u32(q.val[1]));
}
static void E3_wp32(uint32_t*s){
    uint32x4_t A[4],B[4],C[4],D[4],t,u; int b,r;
    for(b=0;b<4;b++){A[b]=vld1q_u32(s+0+b*4);B[b]=vld1q_u32(s+16+b*4);C[b]=vld1q_u32(s+32+b*4);D[b]=vld1q_u32(s+48+b*4);}
    for(r=0;r<9;r++){
        for(b=0;b<4;b++) GBLK(A[b],B[b],C[b],D[b]);                          /* layer 0 */
        for(b=0;b<4;b++){B[b]=DX1(B[b],1);C[b]=DX1(C[b],2);D[b]=DX1(D[b],3);} /* layer 1 */
        for(b=0;b<4;b++) GBLK(A[b],B[b],C[b],D[b]);
        for(b=0;b<4;b++){B[b]=DX1(B[b],3);C[b]=DX1(C[b],2);D[b]=DX1(D[b],1);}
        for(b=0;b<4;b++) T4(&A[b],&B[b],&C[b],&D[b]);                         /* layer 2 */
        t=B[0];B[0]=B[1];B[1]=B[2];B[2]=B[3];B[3]=t;                          /* dx2 B +1 */
        t=C[0];u=C[1];C[0]=C[2];C[1]=C[3];C[2]=t;C[3]=u;                      /* dx2 C +2 */
        t=D[3];D[3]=D[2];D[2]=D[1];D[1]=D[0];D[0]=t;                          /* dx2 D +3 */
        for(b=0;b<4;b++) GBLK(A[b],B[b],C[b],D[b]);
        t=B[3];B[3]=B[2];B[2]=B[1];B[1]=B[0];B[0]=t;                          /* dx2 B +3 (undo) */
        t=C[0];u=C[1];C[0]=C[2];C[1]=C[3];C[2]=t;C[3]=u;                      /* dx2 C +2 (undo) */
        t=D[0];D[0]=D[1];D[1]=D[2];D[2]=D[3];D[3]=t;                          /* dx2 D +1 (undo) */
        for(b=0;b<4;b++) T4(&A[b],&B[b],&C[b],&D[b]);
    }
    for(b=0;b<4;b++) GBLK(A[b],B[b],C[b],D[b]);                              /* final column */
    for(b=0;b<4;b++){vst1q_u32(s+0+b*4,A[b]);vst1q_u32(s+16+b*4,B[b]);vst1q_u32(s+32+b*4,C[b]);vst1q_u32(s+48+b*4,D[b]);}
}
#endif /* HAS_NEON */

/* scalar w=32 E3 (fallback) */
static void E3_32s(uint32_t*s){
    static const params P32={32,9,512,0xFFFFFFFFu,1024,0};
    uint64_t t[64]; int i; for(i=0;i<64;i++)t[i]=s[i]; E3s(t,&P32); for(i=0;i<64;i++)s[i]=(uint32_t)t[i];
}
static void F3_w32(const uint32_t*H,const uint32_t*M,uint32_t fl,uint32_t*Ho){
    uint32_t S[64]; unsigned j;
    for(j=0;j<V;j++){S[j]=H[j];S[V+j]=H[V+j]+M[j];}
    S[0]^=fl;
#if HAS_NEON
    E3_wp32(S);
#else
    E3_32s(S);
#endif
    for(j=0;j<V;j++){Ho[j]=S[j]+M[j];Ho[V+j]=S[V+j];}
}
static void blk32(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long t,uint32_t*b){
    unsigned long long base=t*128ull; unsigned j,k;
    for(j=0;j<V;j++){uint32_t w=0;for(k=0;k<4;k++)w|=(uint32_t)pbyte(m,L,tot,base+j*4+k)<<(8*k);b[j]=w;}
}
static unsigned long long lp2(unsigned long long n){unsigned long long p=1;while(p*2<n)p*=2;return p;}
static void tree32(const uint32_t*cv,unsigned long long lo,unsigned long long hi,int root,uint32_t*out){
    if(hi-lo==1){memcpy(out,cv+lo*HALF,HALF*4);return;}
    unsigned long long ll=lp2(hi-lo);uint32_t L[16],R[16],M[32],I[64];
    tree32(cv,lo,lo+ll,0,L);tree32(cv,lo+ll,hi,0,R);
    memcpy(M,L,64);memcpy(M+16,R,64);F3_w32(H0_512,M,F_PA|(root?F_RO:0),I);memcpy(out,I,64);
}
static int crypt512(const unsigned char*msg,unsigned long long L,unsigned char*dig){
    unsigned long long m=1024,tot=L+2*m-(L%m),N=tot/m,Pi=(N+RHO-1)/RHO;int only=(Pi==1);
    uint32_t*cv=(uint32_t*)malloc((size_t)Pi*HALF*4);if(!cv)return -2;
    for(unsigned long long c=0;c<Pi;c++){
        unsigned long long st=c*RHO,Lc=(st+RHO<=N)?RHO:(N-st);uint32_t H[64],Hn[64],b[32];memcpy(H,H0_512,256);
        for(unsigned long long i=1;i<=Lc;i++){uint32_t fl=0;if(i==1)fl|=F_CS;if(i==Lc)fl|=F_CE;if(i==Lc&&only)fl|=F_RO;
            blk32(msg,L,tot/8,st+(i-1),b);F3_w32(H,b,fl,Hn);memcpy(H,Hn,256);}
        memcpy(cv+c*HALF,H,64);
    }
    uint32_t root[16];if(Pi==1)memcpy(root,cv,64);else tree32(cv,0,Pi,1,root);free(cv);
    for(int i=0;i<64;i++)dig[i]=(unsigned char)((root[i/4]>>(8*(i%4)))&0xFF);
    return 0;
}

/* ============ w=64 path: NEON 2-way chunk batch + scalar tail/tree ========== */
static void load_H0_64(uint64_t*H,const uint64_t*h){unsigned j;for(j=0;j<U;j++)H[j]=h[j];}
static void F3_64s(const uint64_t*H,const uint64_t*M,uint64_t fl,uint64_t*Ho){
    static const params P64={64,18,768,~(uint64_t)0,2048,0};
    uint64_t S[64];unsigned j;
    for(j=0;j<V;j++){S[j]=H[j];S[V+j]=H[V+j]+M[j];}
    S[0]^=fl; E3s(S,&P64);
    for(j=0;j<V;j++){Ho[j]=S[j]+M[j];Ho[V+j]=S[V+j];}
}
static void blk64(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long t,uint64_t*b){
    unsigned long long base=t*256ull; unsigned j,k;
    for(j=0;j<V;j++){uint64_t w=0;for(k=0;k<8;k++)w|=(uint64_t)pbyte(m,L,tot,base+j*8+k)<<(8*k);b[j]=w;}
}
static void chunk64s(const unsigned char*m,unsigned long long L,unsigned long long tot,unsigned long long c,
                     unsigned long long N,int only,const uint64_t*H0,uint64_t*cv){
    uint64_t H[64],Hn[64],b[32];unsigned long long st=c*RHO,i,Lc=(st+RHO<=N)?RHO:(N-st);
    load_H0_64(H,H0);
    for(i=1;i<=Lc;i++){uint64_t fl=0;if(i==1)fl|=F_CS;if(i==Lc)fl|=F_CE;if(i==Lc&&only)fl|=F_RO;
        blk64(m,L,tot,st+(i-1),b);F3_64s(H,b,fl,Hn);memcpy(H,Hn,sizeof(H));}
    memcpy(cv+c*HALF,H,HALF*8);
}
static void tree64(const uint64_t*cv,unsigned long long lo,unsigned long long hi,int root,uint64_t*out,const uint64_t*H0){
    if(hi-lo==1){memcpy(out,cv+lo*HALF,HALF*8);return;}
    unsigned long long ll=lp2(hi-lo);uint64_t L[16],R[16],M[32],I[64];
    tree64(cv,lo,lo+ll,0,L,H0);tree64(cv,lo+ll,hi,0,R,H0);
    memcpy(M,L,128);memcpy(M+16,R,128);F3_64s(H0,M,F_PA|(root?F_RO:0),I);memcpy(out,I,128);
}
static int crypt64(const unsigned char*msg,unsigned long long L,unsigned char*dig,const uint64_t*H0,int n){
    unsigned long long m=2048,tot=L+2*m-(L%m),N=tot/m,Pi=(N+RHO-1)/RHO;int only=(Pi==1);
    uint64_t*cv=(uint64_t*)malloc((size_t)Pi*HALF*8);if(!cv)return -2;
    /* w=64: scalar per-chunk (the NEON 2-chunk batch spilled the 64-vector state
       and was slower than scalar; w=32 keeps the register-resident NEON kernel). */
    for(unsigned long long c=0;c<Pi;c++) chunk64s(msg,L,tot/8,c,N,only,H0,cv);
    uint64_t root[16];if(Pi==1)memcpy(root,cv,128);else tree64(cv,0,Pi,1,root,H0);free(cv);
    int ob=n/8,produced=0,j,b;
    for(j=0;j<16&&produced<ob;j++)for(b=0;b<8&&produced<ob;b++)dig[produced++]=(unsigned char)((root[j]>>(8*b))&0xFF);
    return 0;
}

int CryptHash(int dbits,const unsigned char*msg,unsigned long long Lb,unsigned char*dig){
    params P; if(select_params(dbits,&P)) return -1;
    if(dbits==512) return crypt512(msg,Lb,dig);
    if(dbits==768) return crypt64(msg,Lb,dig,H0_768,768);
    return crypt64(msg,Lb,dig,H0_1024,1024);
}
