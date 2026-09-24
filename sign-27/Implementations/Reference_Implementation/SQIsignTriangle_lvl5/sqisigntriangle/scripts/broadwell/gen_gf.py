#!/usr/bin/env python3
"""Generate gf<tag>.h, gf<tag>.c, fp.c for the broadwell saturated-limb field.
Ports the Pornin-style structure (lvl1/3/5) parameterized by (N,c,f), with:
  - reduction/add/sub/neg/half/partial_reduce/normalize/montgomery_reduce/
    set_small/mul_small/div3/encode/decode/decode_reduce : faithful port
  - gf_mul/gf_square : delegated to the asm fp_mul/fp_sqr (cold-path callers)
  - invert/legendre/sqrt + fp_exp3div4 : Fermat / fixed-exponent square&multiply
    (constant operation sequence; validated vs GMP) instead of safegcd.
Constants/magics computed + validated against committed lvl1/3/5 (see levels.py).
"""
import sys
sys.path.insert(0, '/tmp/gen')

MASK64 = (1<<64)-1
def limbs(x, N): return [(x>>(64*i))&MASK64 for i in range(N)]

def recip64(c):
    for SH in range(0,16):
        S=64+SH; M=(((1<<S)+c-1)//c); e=M*c-(1<<S)
        if e <= (1<<SH): return M,SH
    raise RuntimeError
def small_magic(c,L):
    for S in range(L,L+40):
        M=(((1<<S)+c-1)//c); e=M*c-(1<<S)
        if e*(1<<L) <= (1<<S): return M,S
    raise RuntimeError

def derive(c,f,N):
    p=c*(1<<f)-1; BITS=64*N; nbits=p.bit_length(); tsh=f%64
    inv3=pow(3,-1,p)
    d=dict(p=p,c=c,f=f,N=N,BITS=BITS,nbits=nbits,tsh=tsh,
        MODULUS=limbs(p,N), p2=limbs(2*p,N), ONE=limbs((1<<BITS)%p,N),
        MINUS_ONE=limbs(((p-1)*(1<<BITS))%p,N), R2=limbs((1<<(2*BITS))%p,N),
        PM1O3=limbs(((p-1)*inv3)%p,N),
        add_fold=(1<<(BITS-f))-c, sub_fold=(1<<(BITS-f))-2*c, half_fold=c<<(tsh-1),
        sh=64-(BITS-nbits), tsh_mask=(1<<tsh)-1, topmask=p>>(64*(N-1)))
    d['recip_M'],d['recip_SH']=recip64(c)
    d['small_M'],d['small_S']=small_magic(c,nbits-64*(N-1))  # L = bits of h = 64-tsh? use 64-tsh
    L=64-tsh
    d['small_M'],d['small_S']=small_magic(c,L); d['L']=L
    # exponents
    d['EXP_INV']=limbs(p-2,N);      d['nb_inv']=(p-2).bit_length()
    d['EXP_LEG']=limbs((p-1)//2,N); d['nb_leg']=((p-1)//2).bit_length()
    d['EXP_SQR']=limbs((p+1)//4,N); d['nb_sqr']=((p+1)//4).bit_length()
    d['EXP_E34']=limbs((p-3)//4,N); d['nb_e34']=((p-3)//4).bit_length()
    return d

def H(arr): return "{ " + ", ".join("0x%016X"%v for v in arr)+" }"
def Hc(arr): return ", ".join("0x%016X"%v for v in arr)

# ---------------------------------------------------------------- header ----
def gen_h(tag, d):
    N=d['N']; L=[]; e=L.append
    e('#ifndef %s_h__'%tag); e('#define %s_h__'%tag)
    e('#ifdef __cplusplus'); e('extern "C" {'); e('#endif')
    e('#include <sqisign_namespace.h>')
    for h in ['stddef','stdint','stdio','string','assert']: e('#include <%s.h>'%h)
    e('')
    e('    typedef uint64_t digit_t;')
    e('    typedef union { struct {')
    for i in range(N): e('        uint64_t v%d;'%i)
    e('    }; digit_t arr[%d]; } %s;'%(N,tag))
    e('')
    for nm in ['ZERO','ONE','MINUS_ONE']:
        e('    extern const %s %s_%s;'%(tag,tag,nm))
    e('')
    # ---- inner add/sub/umul (verbatim) ----
    e('''#if (defined _MSC_VER && defined _M_X64) || (defined __x86_64__ && (defined __GNUC__ || defined __clang__))
#include <immintrin.h>
#define inner_%(t)s_adc(cc, a, b, d) _addcarry_u64(cc, a, b, (unsigned long long *)(void *)d)
#define inner_%(t)s_sbb(cc, a, b, d) _subborrow_u64(cc, a, b, (unsigned long long *)(void *)d)
#else
static inline unsigned char inner_%(t)s_adc(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a+(unsigned __int128)b+cc; *d=(uint64_t)t; return (unsigned char)(t>>64);}
static inline unsigned char inner_%(t)s_sbb(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d){
    unsigned __int128 t=(unsigned __int128)a-(unsigned __int128)b-cc; *d=(uint64_t)t; return (unsigned char)(-(uint64_t)(t>>64));}
#endif
#define inner_%(t)s_umul(lo,hi,x,y) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_%(t)s_umul_add(lo,hi,x,y,z) do{ unsigned __int128 _t=(unsigned __int128)(x)*(unsigned __int128)(y)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_%(t)s_umul_x2(lo,hi,x1,y1,x2,y2) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)
#define inner_%(t)s_umul_x2_add(lo,hi,x1,y1,x2,y2,z) do{ unsigned __int128 _t=(unsigned __int128)(x1)*(unsigned __int128)(y1)+(unsigned __int128)(x2)*(unsigned __int128)(y2)+(unsigned __int128)(uint64_t)(z); (lo)=(uint64_t)_t; (hi)=(uint64_t)(_t>>64);}while(0)'''%{'t':tag})
    e('')
    A=lambda nm,i:'%s->v%d'%(nm,i)
    DV=lambda i:'d%d'%i
    def emit_func(sig, body):
        e('    static inline void %s {'%sig);
        for ln in body: e('        '+ln)
        e('    }'); e('')
    # ---- add ----
    decl='uint64_t '+', '.join(DV(i) for i in range(N))+', f; unsigned char cc;'
    body=[decl]
    body.append('cc = inner_%s_adc(0, %s, %s, &d0);'%(tag,A('a',0),A('b',0)))
    for i in range(1,N): body.append('cc = inner_%s_adc(cc, %s, %s, &d%d);'%(tag,A('a',i),A('b',i),i))
    for _r in range(2):
        body.append('f = d%d >> %d;'%(N-1,d['sh']))
        body.append('cc = inner_%s_adc(0, d0, f, &d0);'%tag)
        for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, d%d, 0, &d%d);'%(tag,i,i))
        body.append('(void)inner_%s_adc(cc, d%d, ((uint64_t)0x%X << %d) & -f, &d%d);'%(tag,N-1,d['add_fold'],d['tsh'],N-1))
    for i in range(N): body.append('a->v%d = d%d; (void)b;'%(i,i) if False else 'd->v%d = d%d;'%(i,i))
    emit_func('%s_add(%s *d, const %s *a, const %s *b)'%(tag,tag,tag,tag), body)
    # ---- sub ----
    body=['uint64_t '+', '.join(DV(i) for i in range(N))+', m, f; unsigned char cc;']
    body.append('cc = inner_%s_sbb(0, %s, %s, &d0);'%(tag,A('a',0),A('b',0)))
    for i in range(1,N): body.append('cc = inner_%s_sbb(cc, %s, %s, &d%d);'%(tag,A('a',i),A('b',i),i))
    body.append('(void)inner_%s_sbb(cc, 0, 0, &m);'%tag)
    body.append('cc = inner_%s_sbb(0, d0, m & 2, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_sbb(cc, d%d, 0, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_sbb(cc, d%d, ((uint64_t)0x%X << %d) & m, &d%d);'%(tag,N-1,d['sub_fold'],d['tsh'],N-1))
    body.append('f = d%d >> %d;'%(N-1,d['sh']))
    body.append('cc = inner_%s_adc(0, d0, f, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, d%d, 0, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_adc(cc, d%d, ((uint64_t)0x%X << %d) & -f, &d%d);'%(tag,N-1,d['add_fold'],d['tsh'],N-1))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    emit_func('%s_sub(%s *d, const %s *a, const %s *b)'%(tag,tag,tag,tag), body)
    # ---- neg ----  (2p - a, then fold)
    body=['uint64_t '+', '.join(DV(i) for i in range(N))+', f; unsigned char cc;']
    body.append('cc = inner_%s_sbb(0, (uint64_t)0x%016X, %s, &d0);'%(tag,d['p2'][0],A('a',0)))
    for i in range(1,N):
        body.append('cc = inner_%s_sbb(cc, (uint64_t)0x%016X, %s, &d%d);'%(tag,d['p2'][i],A('a',i),i))
    body.append('f = d%d >> %d;'%(N-1,d['sh']))
    body.append('cc = inner_%s_adc(0, d0, f, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, d%d, 0, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_adc(cc, d%d, ((uint64_t)0x%X << %d) & -f, &d%d);'%(tag,N-1,d['add_fold'],d['tsh'],N-1))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    emit_func('%s_neg(%s *d, const %s *a)'%(tag,tag,tag), body)
    # ---- select / cswap ----
    body=['uint64_t cw = (uint64_t)*(int32_t *)&ctl;']
    for i in range(N): body.append('d->v%d = a0->v%d ^ (cw & (a0->v%d ^ a1->v%d));'%(i,i,i,i))
    emit_func('%s_select(%s *d, const %s *a0, const %s *a1, uint32_t ctl)'%(tag,tag,tag,tag), body)
    body=['uint64_t cw = (uint64_t)*(int32_t *)&ctl, t;']
    for i in range(N): body += ['t = cw & (a->v%d ^ b->v%d); a->v%d ^= t; b->v%d ^= t;'%(i,i,i,i)]
    emit_func('%s_cswap(%s *a, %s *b, uint32_t ctl)'%(tag,tag,tag), body)
    # ---- half ----
    body=['uint64_t '+', '.join(DV(i) for i in range(N))+';']
    for i in range(N-1): body.append('d%d = (a->v%d >> 1) | (a->v%d << 63);'%(i,i,i+1))
    body.append('d%d = a->v%d >> 1;'%(N-1,N-1))
    body.append('d%d += ((uint64_t)0x%X) & -(a->v0 & 1);'%(N-1,d['half_fold']))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    emit_func('%s_half(%s *d, const %s *a)'%(tag,tag,tag), body)
    # ---- mul2 ----
    emit_func('%s_mul2(%s *d, const %s *a)'%(tag,tag,tag), ['%s_add(d, a, a);'%tag])
    # ---- iszero / equals ----
    body=['uint64_t t0, t1, r;']
    body.append('t0 = '+' | '.join('a->v%d'%i for i in range(N))+';')
    inv=' | '.join('~a->v%d'%i for i in range(N-1))+' | (a->v%d ^ 0x%016X)'%(N-1,d['topmask'])
    body.append('t1 = '+inv+';')
    body.append('r = (t0 | -t0) & (t1 | -t1);')
    body.append('return (uint32_t)(r >> 63) - 1;')
    e('    static inline uint32_t %s_iszero(const %s *a) {'%(tag,tag))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    e('    static inline uint32_t %s_equals(const %s *a, const %s *b) { %s d; %s_sub(&d,a,b); return %s_iszero(&d); }'%(tag,tag,tag,tag,tag,tag)); e('')
    # ---- partial_reduce ----
    args=', '.join('uint64_t a%d'%i for i in range(N))
    body=['uint64_t '+', '.join(DV(i) for i in range(N))+', h, quo, rem; unsigned char cc;']
    body.append('h = a%d >> %d;'%(N-1,d['tsh']))
    body.append('a%d &= 0x%016X;'%(N-1,d['tsh_mask']))
    body.append('quo = (0x%X * h) >> %d;'%(d['small_M'],d['small_S']))
    body.append('rem = h - (%d * quo);'%d['c'])
    body.append('cc = inner_%s_adc(0, a0, quo, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, a%d, 0, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_adc(cc, a%d, rem << %d, &d%d);'%(tag,N-1,d['tsh'],N-1))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    e('    static inline void inner_%s_partial_reduce(%s *d, %s) {'%(tag,tag,args))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    # ---- normalize ----
    body=['uint64_t '+', '.join(DV(i) for i in range(N))+', m; unsigned char cc;']
    body.append('cc = inner_%s_sbb(0, a->v0, 0x%016X, &d0);'%(tag,d['MODULUS'][0]))
    for i in range(1,N): body.append('cc = inner_%s_sbb(cc, a->v%d, 0x%016X, &d%d);'%(tag,i,d['MODULUS'][i],i))
    body.append('(void)inner_%s_sbb(cc, 0, 0, &m);'%tag)
    body.append('cc = inner_%s_adc(0, d0, m, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, d%d, m, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_adc(cc, d%d, m & 0x%016X, &d%d);'%(tag,N-1,d['MODULUS'][N-1],N-1))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    e('    static inline void inner_%s_normalize(%s *d, const %s *a) {'%(tag,tag,tag))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    # ---- set_small ----
    body=['uint64_t h, lo, hi, quo, rem;']
    body.append('h = (uint64_t)x << %d;'%(d['BITS']-d['f']))
    body.append('inner_%s_umul(lo, hi, h, 0x%016X); (void)lo;'%(tag,d['recip_M']))
    body.append('quo = hi >> %d;'%d['recip_SH'])
    body.append('rem = h - (%d * quo);'%d['c'])
    body.append('d->v0 = quo;')
    for i in range(1,N-1): body.append('d->v%d = 0;'%i)
    body.append('d->v%d = rem << %d;'%(N-1,d['tsh']))
    e('    static inline void %s_set_small(%s *d, uint32_t x) {'%(tag,tag))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    # ---- mul_small (serial multiply by 32-bit, then fold) ----
    body=['uint64_t '+', '.join('d%d'%i for i in range(N+1))+', lo, hi, carry, b, h, quo, rem; unsigned char cc; (void)cc;']
    body.append('b = (uint64_t)x; carry = 0;')
    for i in range(N):
        body.append('inner_%s_umul(lo, hi, a->v%d, b); cc = inner_%s_adc(0, lo, carry, &d%d); carry = hi + cc;'%(tag,i,tag,i))
    body.append('d%d = carry;'%N)
    body.append('h = (d%d << %d) | (d%d >> %d);'%(N,64-d['tsh'],N-1,d['tsh']))
    body.append('d%d &= 0x%016X;'%(N-1,d['tsh_mask']))
    body.append('inner_%s_umul(lo, hi, h, 0x%016X);'%(tag,d['recip_M']))
    body.append('quo = hi >> %d; rem = h - (%d * quo);'%(d['recip_SH'],d['c']))
    body.append('cc = inner_%s_adc(0, d0, quo, &d0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_adc(cc, d%d, 0, &d%d);'%(tag,i,i))
    body.append('(void)inner_%s_adc(cc, d%d, rem << %d, &d%d);'%(tag,N-1,d['tsh'],N-1))
    for i in range(N): body.append('d->v%d = d%d;'%(i,i))
    e('    static inline void %s_mul_small(%s *d, const %s *a, uint32_t x) {'%(tag,tag,tag))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    # ---- montgomery_reduce ----
    M=d['c']<<d['tsh']
    body=['uint64_t '+', '.join('x%d'%i for i in range(N))+';']
    body.append('uint64_t '+', '.join('f%d'%i for i in range(N))+';')
    body.append('uint64_t '+', '.join('g%d'%i for i in range(2*N))+';')
    body.append('uint64_t '+', '.join('d%d'%i for i in range(N))+';')
    body.append('uint64_t hi, t, w; unsigned char cc;')
    for i in range(N): body.append('x%d = a->v%d;'%(i,i))
    for i in range(N-1): body.append('f%d = x%d;'%(i,i))
    body.append('f%d = x%d + ((x0 * %d) << %d);'%(N-1,N-1,d['c'],d['tsh']))
    body.append('inner_%s_umul(g%d, hi, f0, (uint64_t)%d << %d);'%(tag,N-1,d['c'],d['tsh']))
    for j in range(1,N-1):
        body.append('inner_%s_umul_add(g%d, hi, f%d, (uint64_t)%d << %d, hi);'%(tag,N-1+j,j,d['c'],d['tsh']))
    body.append('inner_%s_umul_add(g%d, g%d, f%d, (uint64_t)%d << %d, hi);'%(tag,2*N-2,2*N-1,N-1,d['c'],d['tsh']))
    body.append('cc = inner_%s_sbb(0, 0, f0, &g0);'%tag)
    for i in range(1,N-1): body.append('cc = inner_%s_sbb(cc, 0, f%d, &g%d);'%(tag,i,i))
    body.append('cc = inner_%s_sbb(cc, g%d, f%d, &g%d);'%(tag,N-1,N-1,N-1))
    for i in range(N,2*N-1): body.append('cc = inner_%s_sbb(cc, g%d, 0, &g%d);'%(tag,i,i))
    body.append('(void)inner_%s_sbb(cc, g%d, 0, &g%d);'%(tag,2*N-1,2*N-1))
    body.append('cc = inner_%s_adc(0, g0, x0, &x0);'%tag)
    for i in range(1,N): body.append('cc = inner_%s_adc(cc, g%d, x%d, &x%d);'%(tag,i,i,i))
    for i in range(N): body.append('cc = inner_%s_adc(cc, g%d, 0, &d%d);'%(tag,N+i,i))
    body.append('(void)cc;')
    andexpr=' & '.join('d%d'%i for i in range(N-1))
    body.append('t = %s & (d%d ^ ~(uint64_t)0x%016X);'%(andexpr,N-1,d['topmask']))
    body.append('cc = inner_%s_adc(0, t, 1, &t);'%tag)
    body.append('(void)inner_%s_sbb(cc, 0, 0, &w); w = ~w;'%tag)
    for i in range(N): body.append('d->v%d = d%d & w;'%(i,i))
    e('    static inline void inner_%s_montgomery_reduce(%s *d, const %s *a) {'%(tag,tag,tag))
    for ln in body: e('        '+ln)
    e('    }'); e('')
    # ---- non-inline decls ----
    for sig in ['uint32_t %s_invert(%s *d, const %s *a)',
                'int32_t %s_legendre(const %s *a)',
                'uint32_t %s_sqrt(%s *d, const %s *a)',
                'void %s_div3(%s *d, const %s *a)',
                'void %s_encode(void *dst, const %s *a)',
                'uint32_t %s_decode(%s *d, const void *src)',
                'void %s_decode_reduce(%s *d, const void *src, size_t len)',
                'void %s_pow(%s *out, const %s *a, const uint64_t *e, int nbits)']:
        n=sig.count('%s'); e('    '+(sig%((tag,)*n))+';')
    e('')
    e('#ifdef __cplusplus'); e('}'); e('#endif'); e('#endif')
    return '\n'.join(L)+'\n'

# ----------------------------------------------------------------- .c ----
def gen_c(tag, d):
    N=d['N']; L=[]; e=L.append
    e('#include "%s.h"'%tag); e('')
    e('extern void fp_mul(%s *out, const %s *a, const %s *b);'%(tag,tag,tag))
    e('extern void fp_sqr(%s *out, const %s *a);'%(tag,tag)); e('')
    e('const %s %s_ZERO = { %s };'%(tag,tag,Hc([0]*N)))
    e('const %s %s_ONE = { %s };'%(tag,tag,Hc(d['ONE'])))
    e('const %s %s_MINUS_ONE = { %s };'%(tag,tag,Hc(d['MINUS_ONE'])))
    e('static const %s R2 = { %s };'%(tag,Hc(d['R2'])))
    e('static const %s MODULUS = { %s };'%(tag,Hc(d['MODULUS'])))
    e('static const %s PM1O3 = { %s };'%(tag,Hc(d['PM1O3']))); e('')
    # exponents
    e('static const uint64_t EXP_INV[%d]  = { %s };'%(N,Hc(d['EXP_INV'])))
    e('static const uint64_t EXP_LEG[%d]  = { %s };'%(N,Hc(d['EXP_LEG'])))
    e('static const uint64_t EXP_SQRT[%d] = { %s };'%(N,Hc(d['EXP_SQR']))); e('')
    # gf_pow: out = a^e (out must NOT alias a)
    e('''void %(t)s_pow(%(t)s *out, const %(t)s *a, const uint64_t *e, int nbits) {
    %(t)s r = %(t)s_ONE, t;
    for (int i = nbits - 1; i >= 0; i--) {
        fp_sqr(&r, &r);
        fp_mul(&t, &r, a);
        uint64_t bit = (e[i >> 6] >> (i & 63)) & 1;
        %(t)s_select(&r, &r, &t, (uint32_t)(-(int64_t)bit));
    }
    *out = r;
}'''%{'t':tag}); e('')
    # invert = a^(p-2)
    e('''uint32_t %(t)s_invert(%(t)s *d, const %(t)s *a) {
    %(t)s tmp = *a;
    %(t)s_pow(d, &tmp, EXP_INV, %(nb)d);
    return ~%(t)s_iszero(a);
}'''%{'t':tag,'nb':d['nb_inv']}); e('')
    # legendre: a^((p-1)/2) -> +1 (QR or 0->0) / -1 (QNR)
    e('''int32_t %(t)s_legendre(const %(t)s *a) {
    %(t)s t, tmp = *a;
    %(t)s_pow(&t, &tmp, EXP_LEG, %(nb)d);
    uint32_t is_qr = %(t)s_equals(&t, &%(t)s_ONE);
    uint32_t z = %(t)s_iszero(a);
    int32_t r = 1 - 2 * (int32_t)(~is_qr & 1); /* +1 if QR, -1 if QNR */
    r &= ~(int32_t)z;                          /* 0 if a==0 */
    return r;
}'''%{'t':tag,'nb':d['nb_leg']}); e('')
    # sqrt: candidate a^((p+1)/4); normalize low bit; verify
    e('''uint32_t %(t)s_sqrt(%(t)s *d, const %(t)s *a) {
    %(t)s y, yn, y2, tmp = *a;
    %(t)s_pow(&y, &tmp, EXP_SQRT, %(nb)d);
    inner_%(t)s_montgomery_reduce(&yn, &y);
    uint32_t ctl = -((uint32_t)yn.v0 & 1);
    %(t)s_neg(&yn, &y);
    %(t)s_select(&y, &y, &yn, ctl);
    fp_sqr(&y2, &y);
    uint32_t r = %(t)s_equals(&y2, a);
    *d = y;
    return r;
}'''%{'t':tag,'nb':d['nb_sqr']}); e('')
    # div3 (El Mrabet)
    e('''void %(t)s_div3(%(t)s *d, const %(t)s *a) {
    const digit_t MAGIC = 0xAAAAAAAAAAAAAAAB;
    uint64_t c0, c1, f0, f1;
    %(t)s t;
    inner_%(t)s_umul(f0, f1, a->arr[%(top)d], MAGIC);
    t.arr[%(top)d] = f1 >> 1;
    c1 = a->arr[%(top)d] - 3 * t.arr[%(top)d];
    for (int32_t i = %(tm1)d; i >= 0; i--) {
        c0 = c1;
        inner_%(t)s_umul(f0, f1, a->arr[i], MAGIC);
        t.arr[i] = f1 >> 1;
        c1 = c0 + a->arr[i] - 3 * t.arr[i];
        t.arr[i] += c0 * ((MAGIC - 1) >> 1);
        f0 = ((c1 >> 1) & c1);
        f1 = ((c1 >> 2) & !(c1 & 0x11));
        f0 |= f1;
        t.arr[i] += f0;
        c1 = c1 - 3 * f0;
    }
    *d = t;
    %(t)s_sub(&t, d, &PM1O3);
    %(t)s_select(d, d, &t, -((c1 & 1) | (c1 >> 1)));
    %(t)s_sub(&t, d, &PM1O3);
    %(t)s_select(d, d, &t, -(c1 == 2));
}'''%{'t':tag,'top':N-1,'tm1':N-2}); e('')
    # enc/dec helpers
    e('''static inline void enc64le(void *dst, uint64_t x){ uint8_t*b=dst; for(int i=0;i<8;i++) b[i]=(uint8_t)(x>>(8*i)); }
static inline uint64_t dec64le(const void *src){ const uint8_t*b=src; uint64_t r=0; for(int i=0;i<8;i++) r|=(uint64_t)b[i]<<(8*i); return r; }'''); e('')
    # encode
    body=['%s x; inner_%s_montgomery_reduce(&x, a); uint8_t *buf = dst;'%(tag,tag)]
    for i in range(N): body.append('enc64le(buf + %d, x.v%d);'%(8*i,i))
    e('void %s_encode(void *dst, const %s *a) {'%(tag,tag))
    for ln in body: e('    '+ln)
    e('}'); e('')
    # decode
    body=['const uint8_t *buf = src; unsigned char cc; uint64_t t;',
          'uint64_t '+', '.join('d%d'%i for i in range(N))+';']
    for i in range(N): body.append('d%d = dec64le(buf + %d);'%(i,8*i))
    body.append('cc = inner_%s_sbb(0, d0, MODULUS.v0, &t);'%tag)
    for i in range(1,N): body.append('cc = inner_%s_sbb(cc, d%d, MODULUS.v%d, &t);'%(tag,i,i))
    body.append('(void)inner_%s_sbb(cc, 0, 0, &t);'%tag)
    for i in range(N): body.append('d->v%d = d%d & t;'%(i,i))
    body.append('fp_mul(d, d, &R2);')
    body.append('return (uint32_t)t;')
    e('uint32_t %s_decode(%s *d, const void *src) {'%(tag,tag))
    for ln in body: e('    '+ln)
    e('}'); e('')
    # decode_reduce
    BB=8*N
    body=['const uint8_t *buf = src;', '*d = %s_ZERO;'%tag, 'if (len == 0) return;',
          'size_t rem = len %% %d;'%BB,
          'if (rem != 0) {', '    uint8_t tmp[%d]; size_t k = len - rem;'%BB,
          '    memcpy(tmp, buf + k, len - k); memset(tmp + len - k, 0, sizeof(tmp) - (len - k));']
    for i in range(N): body.append('    d->v%d = dec64le(&tmp[%d]);'%(i,8*i))
    body += ['    len = k;', '} else {', '    len -= %d;'%BB]
    for i in range(N): body.append('    uint64_t b%d = dec64le(buf + len + %d);'%(i,8*i))
    body.append('    inner_%s_partial_reduce(d, %s);'%(tag,', '.join('b%d'%i for i in range(N))))
    body += ['}', 'while (len > 0) {', '    fp_mul(d, d, &R2); len -= %d;'%BB]
    for i in range(N): body.append('    uint64_t t%d = dec64le(buf + len + %d);'%(i,8*i))
    body.append('    %s t; inner_%s_partial_reduce(&t, %s);'%(tag,tag,', '.join('t%d'%i for i in range(N))))
    body.append('    %s_add(d, d, &t);'%tag)
    body += ['}', 'fp_mul(d, d, &R2);']
    e('void %s_decode_reduce(%s *d, const void *src, size_t len) {'%(tag,tag))
    for ln in body: e('    '+ln)
    e('}')
    return '\n'.join(L)+'\n'

# ----------------------------------------------------------------- fp.c ----
def gen_fpc(tag, d):
    N=d['N']
    return '''#include <assert.h>
#include "fp.h"

const digit_t p[NWORDS_FIELD] = { %(P)s };
const digit_t p2[NWORDS_FIELD] = { %(P2)s };

void fp_sqrt(fp_t *x) { fp_t tmp = *x; (void)%(t)s_sqrt(x, &tmp); }

uint32_t fp_is_square(const fp_t *a) {
    int32_t ls = %(t)s_legendre(a);
    return ~(uint32_t)(ls >> 1);
}

void fp_inv(fp_t *x) { fp_t tmp = *x; (void)%(t)s_invert(x, &tmp); }

// a <- a^((p-3)/4)  (fixed public exponent square-and-multiply)
static const uint64_t EXP_E34[%(N)d] = { %(E34)s };
void fp_exp3div4(fp_t *a) {
    fp_t tmp = *a;
    %(t)s_pow(a, &tmp, EXP_E34, %(nb)d);
}
'''%{'t':tag,'N':N,'P':Hc(d['MODULUS']),'P2':Hc(d['p2']),'E34':Hc(d['EXP_E34']),'nb':d['nb_e34']}

if __name__ == '__main__':
    name=sys.argv[1]; c=int(sys.argv[2]); f=int(sys.argv[3]); N=int(sys.argv[4]); tag=sys.argv[5]
    d=derive(c,f,N)
    import os
    os.makedirs('/tmp/gen/out_'+name, exist_ok=True)
    open('/tmp/gen/out_%s/%s.h'%(name,tag),'w').write(gen_h(tag,d))
    open('/tmp/gen/out_%s/%s.c'%(name,tag),'w').write(gen_c(tag,d))
    open('/tmp/gen/out_%s/fp.c'%name,'w').write(gen_fpc(tag,d))
    print('wrote out_%s/{%s.h,%s.c,fp.c}'%(name,tag,tag))
    print('nb_inv=%d nb_leg=%d nb_sqr=%d nb_e34=%d'%(d['nb_inv'],d['nb_leg'],d['nb_sqr'],d['nb_e34']))
    print('small_M=0x%X small_S=%d recip_M=0x%X recip_SH=%d'%(d['small_M'],d['small_S'],d['recip_M'],d['recip_SH']))
