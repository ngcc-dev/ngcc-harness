/*
 * ntt_ref.c -- portable SCALAR reference implementation of the complete n=256
 * negacyclic NTT / INTT over Z_q[x]/(x^256+1), q=15361, 16-bit Montgomery.
 *
 * COMPLETE NTT: q-1 = 2^10*3*5, so 2n=512 | q-1; x^256+1 splits into 256 linear
 * factors, hence 8 butterfly levels and POINTWISE (coefficient-wise) pointmul.
 * This is the correctness oracle for ntt.S and the scalar baseline in the speed
 * benchmark; it mirrors gen_ntt.py exactly.
 *
 * The oracle keeps coefficients in unsigned [0,q) for simplicity; the AVX
 * assembly uses a SIGNED centered representation, but the two agree mod q (the
 * harness reduces both before comparing).  The scalar transform leaves outputs
 * in standard bit-reversed order, NOT the AVX2 storage permutation; they agree
 * on the end-to-end results the harness checks (round-trip + convolution +
 * nttunpack).
 */
#include "ntt_ref.h"

/* SHUTTLE namespacing: the bare `R` macro is renamed per config so two
 * configs' ntt_ref.c never collide if ever pulled into one TU.  The public
 * functions/externs are aliased to s256_* by ntt_ref.h. */
#define S256_R 65536u
#define R S256_R

static uint32_t RINV;            /* (2^16)^-1 mod q */
static uint16_t ZMONT[NTT_N];    /* omega^{brv8(k)} * R mod q */
static uint16_t NINVTOMONT;      /* 256^-1 * R^2 mod q */
uint16_t ntt_ref_R2;             /* R^2 mod q (exported for pointwise) */

static uint32_t powmod(uint32_t b, uint32_t e){
    uint64_t r=1,x=b%NTT_Q; while(e){ if(e&1)r=r*x%NTT_Q; x=x*x%NTT_Q; e>>=1; } return (uint32_t)r;
}
/* SMALLEST primitive order-th root (same rule as gen_ntt.py), so this oracle
 * matches the avx assembly's (ref-aligned) zeta layout. */
static uint32_t mod_order(uint32_t g){
    uint32_t n=NTT_Q-1, m=NTT_Q-1;
    for(uint32_t p=2; (uint64_t)p*p<=m; p++) if(m%p==0){
        while(m%p==0) m/=p;
        while(n%p==0 && powmod(g,n/p)==1) n/=p;
    }
    if(m>1){ while(n%m==0 && powmod(g,n/m)==1) n/=m; }
    return n;
}
static uint32_t prim_root(uint32_t order){
    for(uint32_t g=2; g<NTT_Q; g++) if(mod_order(g)==order) return g;
    return 0;
}
static int brv8(int x){ int r=0; for(int i=0;i<8;i++) r|=((x>>i)&1)<<(7-i); return r; }

uint16_t ntt_ref_fqmul(uint16_t a, uint16_t b){
    return (uint16_t)((uint64_t)a*b%NTT_Q*RINV%NTT_Q);
}
static uint16_t addm(uint16_t a, uint16_t b){ uint32_t s=(uint32_t)a+b; return (uint16_t)(s>=NTT_Q?s-NTT_Q:s); }
static uint16_t subm(uint16_t a, uint16_t b){ return (uint16_t)(a>=b?a-b:a+NTT_Q-b); }

void ntt_ref_init(void){
    RINV = powmod(R%NTT_Q, NTT_Q-2);
    uint32_t w = prim_root(2*NTT_N);              /* smallest primitive 2N-th root (=98), w^N=-1 */
    for(int k=0;k<NTT_N;k++) ZMONT[k]=(uint16_t)((uint64_t)powmod(w,brv8(k))*(R%NTT_Q)%NTT_Q);
    ntt_ref_R2 = (uint16_t)((uint64_t)(R%NTT_Q)*(R%NTT_Q)%NTT_Q);
    NINVTOMONT = (uint16_t)((uint64_t)powmod(NTT_N,NTT_Q-2)*ntt_ref_R2%NTT_Q);
}

void ntt_ref(uint16_t r[NTT_N]){
    int k=1;
    for(int len=NTT_N/2; len>=1; len>>=1)
        for(int s=0; s<NTT_N; s+=2*len){ uint16_t z=ZMONT[k++];
            for(int j=s;j<s+len;j++){ uint16_t t=ntt_ref_fqmul(z,r[j+len]); r[j+len]=subm(r[j],t); r[j]=addm(r[j],t); } }
}
void invntt_tomont_ref(uint16_t r[NTT_N]){
    int k=NTT_N-1;
    for(int len=1; len<=NTT_N/2; len<<=1)
        for(int s=0; s<NTT_N; s+=2*len){ uint16_t z=ZMONT[k--];
            for(int j=s;j<s+len;j++){ uint16_t t=r[j]; r[j]=addm(t,r[j+len]); r[j+len]=ntt_ref_fqmul(z,subm(r[j+len],t)); } }
    for(int j=0;j<NTT_N;j++) r[j]=ntt_ref_fqmul(r[j],NINVTOMONT);
}
void pointwise_ref(const uint16_t a[NTT_N], const uint16_t b[NTT_N], uint16_t c[NTT_N]){
    for(int i=0;i<NTT_N;i++) c[i]=ntt_ref_fqmul(a[i], b[i]);
}
void polymul_schoolbook(const uint16_t a[NTT_N], const uint16_t b[NTT_N], uint16_t c[NTT_N]){
    for(int i=0;i<NTT_N;i++) c[i]=0;
    for(int i=0;i<NTT_N;i++) for(int j=0;j<NTT_N;j++){ uint16_t p=(uint16_t)((uint32_t)a[i]*b[j]%NTT_Q); int k=i+j;
        if(k<NTT_N) c[k]=addm(c[k],p); else c[k-NTT_N]=subm(c[k-NTT_N],p); }
}
