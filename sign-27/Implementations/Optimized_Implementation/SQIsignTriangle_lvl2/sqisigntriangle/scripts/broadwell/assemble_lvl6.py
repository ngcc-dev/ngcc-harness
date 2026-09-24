import re
tag='gf1131016'; N=16; c=113; tsh=1016%64  # =56
qtop=(c<<tsh)-1   # 0x70FFFFFFFFFFFFFF
# ---- CIOS multiplier (C, p+1 trick) to inject into the header ----
cios = '''
    /* C CIOS Montgomery multiply (p+1 trick); compiles to mulx/adcx with -madx -mbmi2.
       N=16 exceeds GP-register capacity, so fp_mul is C here (phase-1). */
    static inline void
    %(t)s_mul(%(t)s *d, const %(t)s *a, const %(t)s *b)
    {
        uint64_t t[%(N1)d];
        for (int i = 0; i < %(N1)d; i++) t[i] = 0;
        for (int i = 0; i < %(N)d; i++) {
            uint64_t bi = b->arr[i], carry = 0, lo, hi;
            unsigned char cc;
            for (int j = 0; j < %(N)d; j++) {
                inner_%(t)s_umul(lo, hi, a->arr[j], bi);
                cc = inner_%(t)s_adc(0, lo, t[j], &lo);
                hi += cc;
                cc = inner_%(t)s_adc(0, lo, carry, &t[j]);
                carry = hi + cc;
            }
            cc = inner_%(t)s_adc(0, t[%(N)d], carry, &t[%(N)d]);
            uint64_t tN1 = cc;
            uint64_t m = t[0];
            inner_%(t)s_umul(lo, hi, m, (uint64_t)%(c)d << %(tsh)d);
            cc = inner_%(t)s_adc(0, t[%(Nm1)d], lo, &t[%(Nm1)d]);
            cc = inner_%(t)s_adc(cc, t[%(N)d], hi, &t[%(N)d]);
            tN1 += cc;
            for (int j = 0; j < %(N)d; j++) t[j] = t[j + 1];
            t[%(N)d] = tN1;
        }
        uint64_t mask = -(t[%(Nm1)d] >> 63);
        unsigned char cc;
        cc = inner_%(t)s_sbb(0, t[0], mask, &t[0]);
        for (int j = 1; j < %(Nm1)d; j++) cc = inner_%(t)s_sbb(cc, t[j], mask, &t[j]);
        (void)inner_%(t)s_sbb(cc, t[%(Nm1)d], mask & 0x%(qtop)016X, &t[%(Nm1)d]);
        for (int j = 0; j < %(N)d; j++) d->arr[j] = t[j];
    }
    static inline void
    %(t)s_square(%(t)s *d, const %(t)s *a) { %(t)s_mul(d, a, a); }
    static inline void
    %(t)s_xsquare(%(t)s *d, const %(t)s *a, unsigned n) {
        if (n == 0) { *d = *a; return; }
        %(t)s_square(d, a);
        while (n-- > 1) %(t)s_square(d, d);
    }
''' % dict(t=tag,N=N,N1=N+1,Nm1=N-1,c=c,tsh=tsh,qtop=qtop)

# ---- 1) header: inject CIOS before first non-inline decl ----
h=open('out_lvl6/%s.h'%tag).read()
anchor='    uint32_t %s_invert('%tag
i=h.find(anchor); assert i>=0,'header anchor'
h=h[:i]+cios+'\n'+h[i:]
open('out_lvl6/%s.h'%tag,'w').write(h)

# ---- 2) .c: remove extern fp_mul/fp_sqr; splice safegcd; rewire fp_mul->gf_mul ----
src=open('out_lvl6/%s.c'%tag).read()
src=re.sub(r'^extern void fp_mul\([^\n]*\n','',src,flags=re.M)
src=re.sub(r'^extern void fp_sqr\([^\n]*\n','',src,flags=re.M)
def remove_func(text, sig):
    i=text.find(sig); assert i>=0,sig
    j=text.find('{',i); depth=0; k=j
    while k<len(text):
        if text[k]=='{':depth+=1
        elif text[k]=='}':
            depth-=1
            if depth==0:break
        k+=1
    end=k+1
    while end<len(text) and text[end]=='\n':end+=1
    return text[:i]+text[end:]
src=remove_func(src,'uint32_t %s_invert('%tag)
src=remove_func(src,'int32_t %s_legendre('%tag)
src=re.sub(r'^static const uint64_t EXP_INV\[\d+\][^\n]*\n','',src,flags=re.M)
src=re.sub(r'^static const uint64_t EXP_LEG\[\d+\][^\n]*\n','',src,flags=re.M)
frag=open('safegcd_lvl6.c').read()
anchor='void %s_div3('%tag
i=src.find(anchor); assert i>=0
src=src[:i]+frag+'\n'+src[i:]
# rewire fp_mul/fp_sqr -> gf_mul/gf_square (in both original .c and the spliced safegcd)
src=src.replace('fp_mul(','%s_mul('%tag).replace('fp_sqr(','%s_square('%tag)
open('out_lvl6/%s.c'%tag,'w').write(src)
print('assembled. checks:')
print(' header has %s_mul:'%tag, ('%s_mul('%tag) in h)
print(' .c gf_mul count:', src.count('%s_mul('%tag), ' fp_mul remaining:', src.count('fp_mul('))
print(' invert/legendre count:', src.count('%s_invert('%tag), src.count('%s_legendre('%tag))
print(' safegcd present:', ('%s_div('%tag) in src, ('%s_lin('%tag) in src)
