/* hash-30-2 / hash-31-3: the 8-column L-fixed trail in the ZC-1536 permutation.
   Built against the unmodified reference ZuD1536-plain.c (pass its directory with -I).
     zc_column_trail check                 L = rho.theta.rho fixes D, E and W at every z
     zc_column_trail rate Z LOG2N SEED     D->D survival per start round (chi conditions imposed)
     zc_column_trail MODE n R Z SEED LOG2MAX
                                           reduced-round (last R rounds) DM/DMC collision from the
                                           zero IV; prints the two messages in hex                 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ZuD1536-plain.c"
typedef uint64_t u64;
static inline void zc_round(u64 *A, int i) { roundStep((ZuD1536_plain64_state*)A, RC[i]); }
static inline u64 rnd64(u64 *s){ u64 x=*s; x^=x<<13; x^=x>>7; x^=x<<17; *s=x; return x; }
static int R, Z, RL, DMC; static long MAXS;
static u64 seed;
static void perm(u64 *A){ for(int i=12-R;i<12;i++) zc_round(A,i); }
static void absorb(u64 *S, const u64 *Mr){ /* Mr: 24 lanes, only rate lanes used */
  u64 X[24]; for(int k=0;k<24;k++) X[k]=S[k]^(k<RL?Mr[k]:0);
  u64 Y[24]; memcpy(Y,X,sizeof Y); perm(Y);
  if(!DMC){ for(int k=0;k<24;k++) S[k]=Y[k]^X[k]; }
  else { for(int k=0;k<24;k++) S[k]=Y[k]^(k>=RL?X[k]:0); }
}
static void pat(char c,u64*P){ memset(P,0,24*8); u64 m=1ULL<<Z;
  for(int x=0;x<8;x++){int p0=0,p2=0;
    if(c=='d')p0=1; if(c=='D'){p0=x&1;p2=!(x&1);} if(c=='W'){p0=1;p2=1;}
    if(p0)P[x]|=m; if(p2)P[16+x]|=m;}}
static int chi3(int v){int a0=v&1,a1=(v>>1)&1,a2=(v>>2)&1;
  return (a0^((!a1)&a2)) | ((a1^((!a2)&a0))<<1) | ((a2^((!a0)&a1))<<2);}
static int colval(const u64*A,int x){return ((A[x]>>Z)&1)|(((A[8+x]>>Z)&1)<<1)|(((A[16+x]>>Z)&1)<<2);}
/* allowed-set of column values for transition din->dout */
static int allowed(int din,int dout,int v){return (chi3(v)^chi3(v^din))==dout;}

typedef struct { u64 w[8]; int rhs; } row_t;
/* find message block (rate lanes) such that pair (S^M, S^M^Pin) follows trail T for R rounds.
   Pin = pattern T[0].  returns 1 on success, M written. */
static int search_block(const u64 *S, const char *T, u64 *M){
  u64 P[16][24]; for(int r=0;r<=R;r++) pat(T[r],P[r]);
  int off=12-R;
  /* base X: plane-1 rate lanes random, then fix round-0 conditions on planes 1/2 in rate; check capacity */
  u64 X[24]; for(int k=0;k<24;k++) X[k]=S[k];
  for(int k=8;k<RL;k++) X[k]^=rnd64(&seed);
  u64 m=1ULL<<Z;
  /* round-0 chi input = X with A0[0]^=RC: planes 1,2 unaffected by iota */
  int fixa0[8]; /* -1 free, else required chi-input a0 value */
  for(int x=0;x<8;x++){
    int din=colval(P[0],x), dout=colval(P[1],x); fixa0[x]=-1;
    if(!din) continue;
    /* need values (a1,a2) such that some a0 works; choose a1,a2 among allowed */
    int ok=0;
    for(int v=0;v<8&&!ok;v++) if(allowed(din,dout,v)){
      int a1=(v>>1)&1,a2=(v>>2)&1;
      int cur1=(X[8+x]>>Z)&1, cur2=(X[16+x]>>Z)&1;
      int lane1rate=(8+x)<RL, lane2rate=(16+x)<RL;
      if(!lane1rate && cur1!=a1) continue;
      if(!lane2rate && cur2!=a2) continue;
      X[8+x]=(X[8+x]&~m)|((u64)a1<<Z); X[16+x]=(X[16+x]&~m)|((u64)a2<<Z);
      /* does a0 matter? */
      int both = allowed(din,dout,v^1);
      fixa0[x]= both? -1 : (v&1);
      ok=1;
    }
    if(!ok) return -1; /* capacity condition violated */
  }
  /* variables: plane-0 bits (all 8 lanes are rate for n=512,768) */
  u64 base[24]; memcpy(base,X,sizeof base); for(int x=0;x<8;x++) base[x]=0;
  /* F(v) = chi input of round 1 (after iota) */
  u64 F0[24]; { u64 T0[24]; memcpy(T0,base,sizeof T0); zc_round(T0,off); T0[0]^=RC[off+1]; memcpy(F0,T0,sizeof F0);}
  static u64 resp[512][24];
  for(int i=0;i<512;i++){ u64 T0[24]; memcpy(T0,base,sizeof T0); T0[i/64]^=1ULL<<(i%64); zc_round(T0,off); T0[0]^=RC[off+1];
    for(int k=0;k<24;k++) resp[i][k]=T0[k]^F0[k]; }
  /* equations */
  row_t eq[64]; int ne=0;
  for(int x=0;x<8;x++) if(fixa0[x]>=0){ /* chi input a0 = X0 ^ RC0(lane0) */
    int i=64*x+Z; row_t e; memset(&e,0,sizeof e); e.w[i/64]|=1ULL<<(i%64);
    int rc=(x==0)?(int)((RC[off]>>Z)&1):0; e.rhs=fixa0[x]^rc; eq[ne++]=e; }
  for(int x=0;x<8;x++){
    int din=colval(P[1],x), dout=colval(P[2],x); if(!din) continue;
    int vals[2],nv=0; for(int v=0;v<8;v++) if(allowed(din,dout,v)) vals[nv++]=v;
    /* linear masks constant on the set */
    int used=0;
    for(int mk=1;mk<8 && used<2;mk++){
      if(__builtin_parity(mk&vals[0])!=__builtin_parity(mk&vals[1])) continue;
      if(used==1 && mk==eq[ne-1].rhs+100) continue;
      row_t e; memset(&e,0,sizeof e); int c=0;
      for(int y=0;y<3;y++) if((mk>>y)&1){
        int b=64*(8*y+x)+Z; c^=(F0[b/64]>>(b%64))&1;
        for(int i=0;i<512;i++) if((resp[i][b/64]>>(b%64))&1) e.w[i/64]^=1ULL<<(i%64);
      }
      e.rhs=__builtin_parity(mk&vals[0])^c;
      /* independence: skip if mk equals xor of previous mask for this column (only 2 needed from 3) */
      static int prevmk; if(used==1 && (mk^prevmk)==0) continue;
      prevmk=mk; eq[ne++]=e; used++;
    }
  }
  /* gaussian elimination */
  int piv[64]; int rank=0;
  for(int c=0;c<512 && rank<ne;c++){
    int p=-1; for(int r=rank;r<ne;r++) if((eq[r].w[c/64]>>(c%64))&1){p=r;break;}
    if(p<0) continue; row_t t=eq[p]; eq[p]=eq[rank]; eq[rank]=t;
    for(int r=0;r<ne;r++) if(r!=rank && ((eq[r].w[c/64]>>(c%64))&1)){ for(int k=0;k<8;k++) eq[r].w[k]^=eq[rank].w[k]; eq[r].rhs^=eq[rank].rhs; }
    piv[rank++]=c;
  }
  for(int r=rank;r<ne;r++) if(eq[r].rhs) return -2;
  int ispiv[512]={0}; for(int r=0;r<rank;r++) ispiv[piv[r]]=1;
  /* kernel basis */
  static u64 ker[512][8]; int nk=0;
  for(int f=0;f<512;f++) if(!ispiv[f]){ memset(ker[nk],0,64); ker[nk][f/64]|=1ULL<<(f%64);
    for(int r=0;r<rank;r++) if((eq[r].w[f/64]>>(f%64))&1) ker[nk][piv[r]/64]|=1ULL<<(piv[r]%64); nk++; }
  u64 part[8]={0}; for(int r=0;r<rank;r++) if(eq[r].rhs) part[piv[r]/64]|=1ULL<<(piv[r]%64);
  u64 v[8]; memcpy(v,part,64);
  for(int k=0;k<nk;k++) if(rnd64(&seed)&1) for(int j=0;j<8;j++) v[j]^=ker[k][j];
  static u64 dense[48][8];
  for(int d=0;d<48;d++){ memset(dense[d],0,64); for(int k=0;k<nk;k++) if(rnd64(&seed)&1) for(int j=0;j<8;j++) dense[d][j]^=ker[k][j]; }
  for(long it=0; it<MAXS; it++){
    if(it){ int k=__builtin_ctzll(it)%48; for(int j=0;j<8;j++) v[j]^=dense[k][j]; }
    u64 A[24],B[24]; memcpy(A,X,sizeof A); for(int x=0;x<8;x++) A[x]=v[x];
    for(int k=0;k<24;k++) B[k]=A[k]^P[0][k];
    int ok=1;
    for(int r=0;r<R;r++){ zc_round(A,off+r); zc_round(B,off+r);
      for(int k=0;k<24;k++) if((A[k]^B[k])!=P[r+1][k]){ok=0;break;}
      if(!ok){ if(r<2) { fprintf(stderr,"MM failure at round %d\n",r); } break; } }
    if(ok){ for(int k=0;k<24;k++) M[k]=0; for(int x=0;x<8;x++) M[x]=v[x]^S[x]; for(int k=8;k<RL;k++) M[k]=X[k]^S[k];
      fprintf(stderr,"  block found after %ld samples (kernel dim %d, eqs %d)\n",it+1,nk,ne); return 1; }
  }
  return 0;
}
static void printblk(const u64*M){ for(int k=0;k<RL;k++) for(int b=0;b<8;b++) printf("%02x",(unsigned)((M[k]>>(8*b))&0xff)); }
static void linear(u64 *A){ ZuD1536_plain64_state *s=(ZuD1536_plain64_state*)A; rho(s); theta(s); rho(s); }
static int check_main(void){
  int bad=0; const char *names="DEW";
  for(int c=0;c<3;c++) for(Z=0;Z<64;Z++){ u64 P[24],L[24]; char t=names[c];
    if(t=='E'){ pat('D',P); u64 m=1ULL<<Z; for(int x=0;x<8;x++){ P[x]^=m; P[16+x]^=m; } } else pat(t,P);
    memcpy(L,P,sizeof L); linear(L); if(memcmp(L,P,sizeof L)) bad++; }
  printf("L-fixed check: D, E and W at all 64 z positions: %s\n", bad?"FAIL":"fixed");
  return bad?1:0;
}
static int cond(const u64 *A){ u64 m=1ULL<<Z;
  for(int x=1;x<8;x+=2){ if(A[8+x]&m) return 0; if(!(A[16+x]&m)) return 0; }
  for(int x=0;x<8;x+=2){ if(A[x]&m) return 0; if(!(A[8+x]&m)) return 0; }
  return 1; }
static void setcond(u64 *A){ u64 m=1ULL<<Z;
  for(int x=1;x<8;x+=2){ A[8+x]&=~m; A[16+x]|=m; }
  for(int x=0;x<8;x+=2){ A[x]&=~m; A[8+x]|=m; } }
static int rate_main(int argc,char**argv){
  Z=atoi(argv[2]); long n=1L<<atoi(argv[3]); u64 sd=0x9E3779B97F4A7C15ULL*(atoi(argv[4])+1);
  for(int I=0;I<11;I++){ long one=0,two=0;
    for(long t=0;t<n;t++){ u64 A[24]; for(int k=0;k<24;k++) A[k]=rnd64(&sd); setcond(A);
      ZuD1536_plain64_state *s=(ZuD1536_plain64_state*)A;
      chi(s); linear(A); A[0]^=RC[I+1]; if(!cond(A)) continue; one++;
      if(I+1<11){ chi(s); linear(A); A[0]^=RC[I+2]; if(cond(A)) two++; } }
    printf("start round %2d: D->D held %ld / %ld (2^%.2f)", I, one, n, one?__builtin_log2((double)one/n):-99.0);
    if(I+1<11) printf("; two further rounds %ld\n", two); else printf("\n"); }
  return 0;
}
static int collide_main(int argc,char**argv){
  DMC=!strcmp(argv[1],"DMC"); int n=atoi(argv[2]); R=atoi(argv[3]); Z=atoi(argv[4]); seed=0x9E3779B97F4A7C15ULL*(atoi(argv[5])+3); MAXS=1L<<atoi(argv[6]);
  RL=(1536-n-64)/64;
  char TA[17],TB[17]; TA[0]='d'; TA[1]='W'; for(int r=2;r<=R;r++) TA[r]='D'; TA[R+1]=0; for(int r=0;r<=R;r++) TB[r]='D'; TB[R+1]=0;
  u64 Dp[24],dp[24]; pat('D',Dp); pat('d',dp);
  long tries=0;
  for(;;){
    /* prefix block */
    u64 Pm[24]={0},S1[24]={0}; for(int k=0;k<RL;k++) Pm[k]=rnd64(&seed);
    absorb(S1,Pm); tries++;
    u64 MA[24]; int rA=search_block(S1,TA,MA); if(rA<0) continue;
    if(rA==0){fprintf(stderr,"block A not found\n"); continue;}
    u64 MA2[24]; for(int k=0;k<24;k++) MA2[k]=MA[k]^dp[k];
    u64 S2[24],S2b[24]; memcpy(S2,S1,sizeof S2); memcpy(S2b,S1,sizeof S2b); absorb(S2,MA); absorb(S2b,MA2);
    u64 dS[24]; for(int k=0;k<24;k++) dS[k]=S2[k]^S2b[k];
    fprintf(stderr,"prefix tries %ld; block A done; dS capacity matches D: %d\n",tries,
      ({int e=1; for(int k=RL;k<24;k++) if(dS[k]!=Dp[k]) e=0; e;}));
    u64 MB[24]; int rB=search_block(S2,TB,MB);
    if(rB<0){ fprintf(stderr,"block B capacity conditions fail; restart\n"); continue; }
    if(rB==0){ fprintf(stderr,"block B not found; restart\n"); continue; }
    /* partner block: X' = S2b ^ MB' must equal X ^ D  -> MB' = MB ^ (S2^S2b) ^ D on rate */
    u64 MB2[24]; for(int k=0;k<24;k++) MB2[k]=MB[k]^dS[k]^Dp[k];
    u64 S3[24],S3b[24]; memcpy(S3,S2,sizeof S3); memcpy(S3b,S2b,sizeof S3b); absorb(S3,MB); absorb(S3b,MB2);
    u64 MC[24]={0},MC2[24]={0}; int nblk=3;
    if(DMC){ for(int k=0;k<RL;k++){ MC[k]=rnd64(&seed); MC2[k]=MC[k]^S3[k]^S3b[k]; } nblk=4;
      absorb(S3,MC); absorb(S3b,MC2); }
    int eq=1; for(int k=0;k<24;k++) if(S3[k]!=S3b[k]) eq=0;
    fprintf(stderr,"internal state collision after %d blocks: %d\n",nblk,eq);
    printf("m1 "); printblk(Pm); printblk(MA); printblk(MB); if(DMC) printblk(MC); printf("\n");
    printf("m2 "); printblk(Pm); printblk(MA2); printblk(MB2); if(DMC) printblk(MC2); printf("\n");
    return 0;
  }
}
int main(int argc,char**argv){
  if(argc>1 && !strcmp(argv[1],"check")) return check_main();
  if(argc==5 && !strcmp(argv[1],"rate")) return rate_main(argc,argv);
  if(argc==7) return collide_main(argc,argv);
  fprintf(stderr,"usage: see header\n"); return 2;
}
