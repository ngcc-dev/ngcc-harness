# Compute + validate the reduction magics for the broadwell gf backend.
def recip64(c):
    """64-bit reciprocal magic for floor(h/c), h<2^64.
       Returns (MAGIC, SH): quo = (h*MAGIC)>>(64+SH) == floor(h/c)."""
    for SH in range(0, 16):
        S = 64 + SH
        M = (( (1<<S) + c - 1)//c )           # ceil(2^S/c)
        e = M*c - (1<<S)                        # 0 <= e < c
        if e <= (1<<SH):                        # exact for all h < 2^S/e >= 2^64
            return M, SH
    raise RuntimeError("no recip64")

def small_magic(c, L):
    """Small magic for floor(h/c), h<2^L. smallest S with M=ceil(2^S/c) exact."""
    for S in range(L, L+40):
        M = (((1<<S)+c-1)//c)
        e = M*c - (1<<S)
        if e * (1<<L) <= (1<<S):
            return M, S
    raise RuntimeError("no small magic")

def verify_div(M, shift, c, L, hi_extract=False):
    """Exhaustively verify floor(h/c) for all h<2^L."""
    for h in range(1<<L):
        if hi_extract:
            q = (h*M) >> shift
        else:
            q = (h*M) >> shift
        if q != h//c:
            return False, h
    return True, None

# committed values to reproduce
COMMITTED = {
  'lvl1': dict(c=5,  tsh=56, small=(0xCD,10),   recipSH=2),
  'lvl3': dict(c=65, tsh=56, small=(0xFC1,18),  recipSH=2),
  'lvl5': dict(c=27, tsh=52, small=(0x12F7,17), recipSH=4),
}
print("=== 64-bit reciprocal magic (reproduce committed SH) ===")
for name,d in COMMITTED.items():
    M,SH = recip64(d['c'])
    ok = (SH==d['recipSH'])
    print(f"{name} c={d['c']}: MAGIC=0x{M:016X} SH={SH}  (committed SH={d['recipSH']}) {'OK' if ok else 'MISMATCH'}")

print("\n=== small magic correctness (committed M,S exhaustive over h<2^L) ===")
for name,d in COMMITTED.items():
    c=d['c']; L=64-d['tsh']; M,S=d['small']
    good,bad = verify_div(M,S,c,L)
    print(f"{name} c={c} L={L} committed M=0x{M:X} S={S}: {'EXACT' if good else f'FAIL@{bad}'}")

print("\n=== lvl2 magics (c=9) ===")
c=9; tsh=53; L=64-tsh
M64,SH = recip64(c)
print(f"lvl2 recip64: MAGIC=0x{M64:016X} SH={SH}")
# small magic: use ref-proven 0x71D,S=14; verify; also show auto
good,bad = verify_div(0x71D,14,c,L)
print(f"lvl2 small (ref 0x71D,S=14, L={L}): {'EXACT' if good else f'FAIL@{bad}'}")
Mauto,Sauto = small_magic(c,L)
print(f"lvl2 small (auto): M=0x{Mauto:X} S={Sauto}")
gg,_=verify_div(Mauto,Sauto,c,L); print(f"   auto exact? {gg}")
# also verify the 64-bit magic exact for full range sample
import random
bad64=None
for _ in range(200000):
    h=random.getrandbits(64)
    if ((h*M64)>>(64+SH))!=h//c: bad64=h;break
print(f"lvl2 recip64 exact on 200k random 64-bit h: {'OK' if bad64 is None else f'FAIL@{bad64}'}")
