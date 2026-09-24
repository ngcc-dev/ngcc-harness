#!/usr/bin/env python3
# Generate the safegcd (binary-GCD) inversion + Legendre for the broadwell gf,
# ported from Pornin's lvl1/3/5 structure, parameterized by (N,c,f,tag).
# Emits a C fragment appended to gf<tag>.c (replaces the Fermat invert/legendre).
import sys
N=int(sys.argv[1]); c=int(sys.argv[2]); f=int(sys.argv[3]); tag=sys.argv[4]
BITS=64*N; p=c*(1<<f)-1; nbits=p.bit_length(); tsh=f%64
outer=-(-(2*nbits-55)//31); inner_div=31; Total=2*nbits-2; final=Total-inner_div*outer
if len(sys.argv)>5: outer=int(sys.argv[5])
if len(sys.argv)>6: final=int(sys.argv[6])
Total=inner_div*outer+final
invt_exp=Total-BITS
e2=2*BITS-Total
invt_val=pow(2,e2,p) if e2>=0 else pow(pow(2,-e2,p),-1,p)
def lim(x): return [(x>>(64*i))&((1<<64)-1) for i in range(N)]
# magics for gf_lin reduction
recipM=(((1<<(64+3))+c-1)//c) if c==9 else None
# generic recip64
def recip64(c):
    for SH in range(16):
        M=(((1<<(64+SH))+c-1)//c); e=M*c-(1<<(64+SH))
        if e<=(1<<SH): return M,SH
    raise RuntimeError
def small_magic(c,L):
    for S in range(L,L+40):
        M=(((1<<S)+c-1)//c); e=M*c-(1<<S)
        if e*(1<<L)<=(1<<S): return M,S
    raise RuntimeError
recipM,recipSH=recip64(c)
smallM,smallS=small_magic(c,64-tsh)
M64=((1<<64)-1)//c           # floor((2^64-1)/c)
negc=(-c)&((1<<64)-1)
tshmask=(1<<tsh)-1
L=[]; e=L.append
def J(fmt,a,b,step=1,sep='\n'):  # join fmt over range
    return sep.join(fmt%i for i in range(a,b,step))

# ---- sgnw, lzcnt ----
e(r'''
static inline uint64_t sgnw(uint64_t x){ return (uint64_t)(*(int64_t*)&x >> 63); }
#if defined __LZCNT__
#include <immintrin.h>
static inline uint64_t lzcnt(uint64_t x){ return _lzcnt_u64(x); }
#else
static inline uint64_t lzcnt(uint64_t x){
    uint64_t m,s;
    m=sgnw((x>>32)-1); s=m&32; x=(x>>32)^(m&(x^(x>>32)));
    m=sgnw((x>>16)-1); s|=m&16; x=(x>>16)^(m&(x^(x>>16)));
    m=sgnw((x>>8)-1);  s|=m&8;  x=(x>>8)^(m&(x^(x>>8)));
    m=sgnw((x>>4)-1);  s|=m&4;  x=(x>>4)^(m&(x^(x>>4)));
    m=sgnw((x>>2)-1);  s|=m&2;  x=(x>>2)^(m&(x^(x>>2)));
    s+=(2-x)&((x-3)>>2); return s;
}
#endif''')

# ---- gf_lin ----
e('\nstatic void %s_lin(%s *d, const %s *u, const %s *v, uint64_t fc, uint64_t gc){'%(tag,tag,tag,tag))
e('    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; %s tu; %s_neg(&tu,u); %s_select(&tu,u,&tu,(uint32_t)sf);'%(tag,tag,tag))
e('    uint64_t sg=sgnw(gc); gc=(gc^sg)-sg; %s tv; %s_neg(&tv,v); %s_select(&tv,v,&tv,(uint32_t)sg);'%(tag,tag,tag))
e('    uint64_t %s, t;'%(', '.join('d%d'%i for i in range(N))))
e('    inner_%s_umul_x2(d0, t, tu.v0, fc, tv.v0, gc);'%tag)
for i in range(1,N):
    e('    inner_%s_umul_x2_add(d%d, t, tu.v%d, fc, tv.v%d, gc, t);'%(tag,i,i,i))
e('    uint64_t h0=(d%d >> %d)|(t << %d);'%(N-1,tsh,64-tsh))
e('    uint64_t h1=t >> %d;'%tsh)
e('    d%d &= 0x%016X;'%(N-1,tshmask))
e('    uint64_t z0,z1,quo0,rem0,quo1,rem1;')
e('    inner_%s_umul(z0,z1,h0,0x%016X); (void)z0;'%(tag,recipM))
e('    quo0=z1>>%d; rem0=h0-(%d*quo0);'%(recipSH,c))
e('    quo1=(0x%X*h1)>>%d; rem1=h1-(%d*quo1);'%(smallM,smallS,c))
e('    uint64_t ee,f0,f1; unsigned char cc;')
e('    cc=inner_%s_adc(0, rem0 + 0x%016X, rem1, &ee);'%(tag,negc))
e('    cc=inner_%s_adc(cc, quo0, rem1 * 0x%016X, &f0);'%(tag,M64))
e('    cc=inner_%s_adc(cc, quo1, 0, &f1); assert(cc==0);'%tag)
e('    ee -= 0x%016X;'%negc)
e('    cc=inner_%s_adc(0, d0, f0, &d0);'%tag)
e('    cc=inner_%s_adc(cc, d1, f1, &d1);'%tag)
for i in range(2,N-1): e('    cc=inner_%s_adc(cc, d%d, 0, &d%d);'%(tag,i,i))
e('    (void)inner_%s_adc(cc, d%d, ee << %d, &d%d);'%(tag,N-1,tsh,N-1))
for i in range(N): e('    d->v%d=d%d;'%(i,i))
e('}')

# ---- lindiv31abs ----
e('\nstatic uint64_t %s_lindiv31abs(%s *d, const %s *a, const %s *b, uint64_t fc, uint64_t gc){'%(tag,tag,tag,tag))
e('    uint64_t sf=sgnw(fc); fc=(fc^sf)-sf; uint64_t sg=sgnw(gc); gc=(gc^sg)-sg;')
e('    uint64_t %s;'%(', '.join('a%d'%i for i in range(N+1))))
e('    uint64_t %s;'%(', '.join('b%d'%i for i in range(N+1))))
e('    unsigned char cc;')
e('    cc=inner_%s_sbb(0, a->v0 ^ sf, sf, &a0);'%tag)
for i in range(1,N): e('    cc=inner_%s_sbb(cc, a->v%d ^ sf, sf, &a%d);'%(tag,i,i))
e('    (void)inner_%s_sbb(cc, 0, 0, &a%d);'%(tag,N))
e('    cc=inner_%s_sbb(0, b->v0 ^ sg, sg, &b0);'%tag)
for i in range(1,N): e('    cc=inner_%s_sbb(cc, b->v%d ^ sg, sg, &b%d);'%(tag,i,i))
e('    (void)inner_%s_sbb(cc, 0, 0, &b%d);'%(tag,N))
e('    uint64_t %s, t;'%(', '.join('d%d'%i for i in range(N+1))))
e('    inner_%s_umul_x2(d0, t, a0, fc, b0, gc);'%tag)
for i in range(1,N): e('    inner_%s_umul_x2_add(d%d, t, a%d, fc, b%d, gc, t);'%(tag,i,i,i))
e('    d%d = t - (a%d & fc) - (b%d & gc);'%(N,N,N))
for i in range(N): e('    d%d = (d%d >> 31)|(d%d << 33);'%(i,i,i+1))
e('    t = sgnw(d%d);'%N)
e('    cc=inner_%s_sbb(0, d0 ^ t, t, &d0);'%tag)
for i in range(1,N): e('    cc=inner_%s_sbb(cc, d%d ^ t, t, &d%d);'%(tag,i,i))
e('    (void)cc;')
for i in range(N): e('    d->v%d=d%d;'%(i,i))
e('    return t;')
e('}')

# ---- tnz/snz approximation block (shared) ----
def approx():
    o=[]
    for i in range(N-1,0,-1): o.append('        uint64_t m%d = a.v%d | b.v%d;'%(i,i,i))
    o.append('        uint64_t tnz%d = sgnw(m%d | -m%d);'%(N-1,N-1,N-1))
    for i in range(N-2,0,-1):
        guard=' '.join('& ~tnz%d'%k for k in range(N-1,i,-1))
        o.append('        uint64_t tnz%d = sgnw(m%d | -m%d) %s;'%(i,i,i,guard))
    o.append('        uint64_t tnzm = %s;'%(' | '.join('(m%d & tnz%d)'%(k,k) for k in range(N-1,0,-1))))
    o.append('        uint64_t tnza = %s;'%(' | '.join('(a.v%d & tnz%d)'%(k,k) for k in range(N-1,0,-1))))
    o.append('        uint64_t tnzb = %s;'%(' | '.join('(b.v%d & tnz%d)'%(k,k) for k in range(N-1,0,-1))))
    o.append('        uint64_t snza = %s;'%(' | '.join('(a.v%d & tnz%d)'%(k-1,k) for k in range(N-1,0,-1))))
    o.append('        uint64_t snzb = %s;'%(' | '.join('(b.v%d & tnz%d)'%(k-1,k) for k in range(N-1,0,-1))))
    o.append('        int64_t s = lzcnt(tnzm);')
    o.append('        uint64_t sm = (uint64_t)((31 - s) >> 63);')
    o.append('        tnza ^= sm & (tnza ^ ((tnza << 32) | (snza >> 32)));')
    o.append('        tnzb ^= sm & (tnzb ^ ((tnzb << 32) | (snzb >> 32)));')
    o.append('        s -= 32 & sm; tnza <<= s; tnzb <<= s;')
    o.append('        uint64_t tzx = ~(%s);'%(' | '.join('tnz%d'%k for k in range(1,N))))
    o.append('        tnza |= a.v0 & tzx; tnzb |= b.v0 & tzx;')
    o.append('        xa = (a.v0 & 0x7FFFFFFF) | (tnza & 0xFFFFFFFF80000000);')
    o.append('        xb = (b.v0 & 0x7FFFFFFF) | (tnzb & 0xFFFFFFFF80000000);')
    return '\n'.join(o)

# ---- gf_div ----
e('\nuint32_t %s_div(%s *d, const %s *x, const %s *y){'%(tag,tag,tag,tag))
e('    %s a,b,u,v; uint64_t xa,xb,f0,g0,f1,g1; uint32_t r;'%tag)
e('    r=~%s_iszero(y); inner_%s_normalize(&a,y); b=MODULUS; u=*x; v=%s_ZERO;'%(tag,tag,tag))
e('    for(int i=0;i<%d;i++){'%outer)
e(approx())
e('        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;')
e('        for(int j=0;j<31;j++){')
e('            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;')
e('            a_odd=-(xa&1); cc=inner_%s_sbb(0,xa,xb,&t0); (void)inner_%s_sbb(cc,0,0,&swap); swap&=a_odd;'%(tag,tag))
e('            t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;')
e('            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; }')
e('        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;')
e('        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;')
e('        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;')
e('        %s na,nb,nu,nv;'%tag)
e('        uint64_t nega=%s_lindiv31abs(&na,&a,&b,f0,g0); uint64_t negb=%s_lindiv31abs(&nb,&a,&b,f1,g1);'%(tag,tag))
e('        f0=(f0^nega)-nega; g0=(g0^nega)-nega; f1=(f1^negb)-negb; g1=(g1^negb)-negb;')
e('        %s_lin(&nu,&u,&v,f0,g0); %s_lin(&nv,&u,&v,f1,g1); a=na; b=nb; u=nu; v=nv;'%(tag,tag))
e('    }')
e('    xa=a.v0; xb=b.v0; f0=1; g0=0; f1=0; g1=1;')
e('    for(int j=0;j<%d;j++){'%final)
e('        uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;')
e('        a_odd=-(xa&1); cc=inner_%s_sbb(0,xa,xb,&t0); (void)inner_%s_sbb(cc,0,0,&swap); swap&=a_odd;'%(tag,tag))
e('        t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(f0^f1); f0^=t2; f1^=t2; t3=swap&(g0^g1); g0^=t3; g1^=t3;')
e('        xa-=a_odd&xb; f0-=a_odd&f1; g0-=a_odd&g1; xa>>=1; f1<<=1; g1<<=1; }')
e('    %s_lin(d,&u,&v,f1,g1);'%tag)
e('    fp_mul(d,d,&INVT%d);'%invt_exp)
e('    return r;')
e('}')
e('\nuint32_t %s_invert(%s *d, const %s *a){ return %s_div(d, &%s_ONE, a); }'%(tag,tag,tag,tag,tag))

# ---- gf_legendre ----
e('\nint32_t %s_legendre(const %s *x){'%(tag,tag))
e('    %s a,b; uint64_t xa,xb,f0,g0,f1,g1,ls;'%tag)
e('    inner_%s_normalize(&a,x); b=MODULUS; ls=0;'%tag)
e('    for(int i=0;i<%d;i++){'%outer)
e(approx())
e('        uint64_t fg0=(uint64_t)1, fg1=(uint64_t)1<<32;')
e('        for(int j=0;j<29;j++){')
e('            uint64_t a_odd,swap,t0,t1,t2; unsigned char cc;')
e('            a_odd=-(xa&1); cc=inner_%s_sbb(0,xa,xb,&t0); (void)inner_%s_sbb(cc,0,0,&swap); swap&=a_odd;'%(tag,tag))
e('            ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;')
e('            xa-=a_odd&xb; fg0-=a_odd&fg1; xa>>=1; fg1<<=1; ls^=(xb+2)>>1; }')
e('        uint64_t fg0z=fg0+0x7FFFFFFF7FFFFFFF, fg1z=fg1+0x7FFFFFFF7FFFFFFF;')
e('        f0=(fg0z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0z>>32)-(uint64_t)0x7FFFFFFF;')
e('        f1=(fg1z&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1z>>32)-(uint64_t)0x7FFFFFFF;')
e('        uint64_t a0=(a.v0*f0+b.v0*g0)>>29; uint64_t b0=(a.v0*f1+b.v0*g1)>>29;')
e('        for(int j=0;j<2;j++){')
e('            uint64_t a_odd,swap,t0,t1,t2,t3; unsigned char cc;')
e('            a_odd=-(xa&1); cc=inner_%s_sbb(0,xa,xb,&t0); (void)inner_%s_sbb(cc,0,0,&swap); swap&=a_odd;'%(tag,tag))
e('            ls^=swap&a0&b0; t1=swap&(xa^xb); xa^=t1; xb^=t1; t2=swap&(fg0^fg1); fg0^=t2; fg1^=t2;')
e('            t3=swap&(a0^b0); a0^=t3; b0^=t3; xa-=a_odd&xb; fg0-=a_odd&fg1; a0-=a_odd&b0; xa>>=1; fg1<<=1; a0>>=1; ls^=(b0+2)>>1; }')
e('        fg0+=0x7FFFFFFF7FFFFFFF; fg1+=0x7FFFFFFF7FFFFFFF;')
e('        f0=(fg0&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g0=(fg0>>32)-(uint64_t)0x7FFFFFFF;')
e('        f1=(fg1&0xFFFFFFFF)-(uint64_t)0x7FFFFFFF; g1=(fg1>>32)-(uint64_t)0x7FFFFFFF;')
e('        %s na,nb; uint64_t nega=%s_lindiv31abs(&na,&a,&b,f0,g0); (void)%s_lindiv31abs(&nb,&a,&b,f1,g1);'%(tag,tag,tag))
e('        ls^=nega&nb.v0; a=na; b=nb;')
e('    }')
e('    xa=a.v0; xb=b.v0;')
e('    for(int j=0;j<%d;j++){'%final)
e('        uint64_t a_odd,swap,t0,t1; unsigned char cc;')
e('        a_odd=-(xa&1); cc=inner_%s_sbb(0,xa,xb,&t0); (void)inner_%s_sbb(cc,0,0,&swap); swap&=a_odd;'%(tag,tag))
e('        ls^=swap&xa&xb; t1=swap&(xa^xb); xa^=t1; xb^=t1; xa-=a_odd&xb; xa>>=1; ls^=(xb+2)>>1; }')
e('    uint32_t rr = 1 - ((uint32_t)ls & 2); rr &= ~%s_iszero(x); return *(int32_t*)&rr;'%tag)
e('}')

frag='\n'.join(L)+'\n'
# INVT constant decl
invt_decl='static const %s INVT%d = { %s };\n'%(tag,invt_exp,', '.join('0x%016X'%v for v in lim(invt_val)))
sys.stdout.write(invt_decl+frag)
