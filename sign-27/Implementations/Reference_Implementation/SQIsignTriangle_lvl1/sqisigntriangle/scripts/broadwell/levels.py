# Shared level definitions + constant formulas for the broadwell gf backend.
LEVELS = {
    'lvl1': dict(tag='gf5248',   c=5,   f=248, N=4),
    'lvl3': dict(tag='gf65376',  c=65,  f=376, N=6),
    'lvl5': dict(tag='gf27500',  c=27,  f=500, N=8),
    'lvl2': dict(tag='gf9309',   c=9,   f=309, N=5),
    'lvl6': dict(tag='gf1131016',c=113, f=1016,N=16),
}
MASK64 = (1<<64)-1

def derive(c, f, N):
    p = c*(1<<f) - 1
    BITS = 64*N
    nbits = p.bit_length()
    tsh = f % 64
    headroom = BITS - nbits
    assert p < (1<<BITS)
    def limbs(x):
        return [ (x>>(64*i)) & MASK64 for i in range(N) ]
    nres = lambda x: (x * (1<<BITS)) % p
    ONE = nres(1)
    MINUS_ONE = nres(p-1)
    R2 = (1<<(2*BITS)) % p
    inv3 = pow(3, -1, p)
    PM1O3 = nres(((p-1)*inv3) % p)
    Total = 2*nbits - 2
    outer = 4*N - 1
    inner_div = 31
    final = Total - inner_div*outer
    invt_exp = Total - BITS
    INVT = (1<<(BITS - invt_exp)) % p   # = 2^(2BITS-Total) mod p
    d = dict(p=p, BITS=BITS, nbits=nbits, tsh=tsh, headroom=headroom,
             MODULUS=limbs(p), p2=limbs(2*p), ONE=limbs(ONE), MINUS_ONE=limbs(MINUS_ONE),
             R2=limbs(R2), PM1O3=limbs(PM1O3), INVT=limbs(INVT), invt_exp=invt_exp,
             Total=Total, outer=outer, final=final,
             add_fold=(1<<(BITS-f))-c, sub_fold=(1<<(BITS-f))-2*c, half_fold=c<<(tsh-1))
    return d

def hexL(arr):  # C-style little-endian limb list
    return "{ " + ", ".join("0x%016X"%v for v in arr) + " }"

if __name__ == '__main__':
    for name in ['lvl1','lvl3','lvl5','lvl2','lvl6']:
        L=LEVELS[name]; d=derive(L['c'],L['f'],L['N'])
        print(f"== {name} tag={L['tag']} c={L['c']} f={L['f']} N={L['N']} nbits={d['nbits']} BITS={d['BITS']} tsh={d['tsh']} hr={d['headroom']}")
        print(f"   outer={d['outer']} inner=31 final={d['final']} Total={d['Total']} INVT_exp={d['invt_exp']} INVT={hexL(d['INVT'])}")
        print(f"   add_fold=0x{d['add_fold']:X}<<{d['tsh']}  sub_fold=0x{d['sub_fold']:X}<<{d['tsh']}  half_fold=0x{d['half_fold']:X}")
        print(f"   MODULUS  = {hexL(d['MODULUS'])}")
        print(f"   ONE      = {hexL(d['ONE'])}")
        print(f"   MINUS_ONE= {hexL(d['MINUS_ONE'])}")
        print(f"   R2       = {hexL(d['R2'])}")
        print(f"   PM1O3    = {hexL(d['PM1O3'])}")
