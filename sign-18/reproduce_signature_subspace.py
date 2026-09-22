#!/usr/bin/env python3
"""Measure the fixed Origami-128 signature subspace over GF(16)."""
import ctypes, hashlib, random
from pathlib import Path

MUL = [[0]*16 for _ in range(16)]
for a in range(16):
    for b in range(16):
        x,y,z=a,b,0
        while y:
            if y&1: z ^= x
            y >>= 1; x <<= 1
            if x&16: x ^= 0x13
        MUL[a][b]=z
INV=[0]+[next(b for b in range(1,16) if MUL[a][b]==1) for a in range(1,16)]

class Rank:
    def __init__(self): self.rows={}
    def add(self, row):
        row=row[:]
        for p in sorted(self.rows):
            if row[p]:
                f=row[p]; row=[x^MUL[f][y] for x,y in zip(row,self.rows[p])]
        try: p=next(i for i,x in enumerate(row) if x)
        except StopIteration: return
        row=[MUL[INV[row[p]]][x] for x in row]
        for op,old in list(self.rows.items()):
            if old[p]:
                f=old[p]; self.rows[op]=[x^MUL[f][y] for x,y in zip(old,row)]
        self.rows[p]=row
    def __len__(self): return len(self.rows)

def main():
    lib=ctypes.CDLL(str((Path(__file__).with_name("lib")/"libOrigami-128.so").resolve()))
    for n in ("sig_get_pk_len_bytes","sig_get_sk_len_bytes","sig_get_sn_len_bytes"): getattr(lib,n).restype=ctypes.c_ulonglong
    pn,sn,zn=lib.sig_get_pk_len_bytes(),lib.sig_get_sk_len_bytes(),lib.sig_get_sn_len_bytes()
    def seed(v):
        x=(ctypes.c_ubyte*len(v)).from_buffer_copy(v); assert lib.ngcc_seed(x,len(v))==0
    seed(bytes(48)); pk,sk=(ctypes.c_ubyte*pn)(),(ctypes.c_ubyte*sn)(); a,b=ctypes.c_ulonglong(),ctypes.c_ulonglong()
    assert lib.sig_keygen(pk,ctypes.byref(a),sk,ctypes.byref(b))==0
    rank=Rank(); accepted=attempts=0
    while accepted<180 and attempts<300:
        seed(hashlib.shake_256(b"origami-subspace"+attempts.to_bytes(4,"little")).digest(48))
        raw=attempts.to_bytes(16,"little"); msg=(ctypes.c_ubyte*16).from_buffer_copy(raw)
        sig=(ctypes.c_ubyte*zn)(); zl=ctypes.c_ulonglong(); attempts+=1
        if lib.sig_sign(sk,sn,msg,16,sig,ctypes.byref(zl)): continue
        if lib.sig_verify(pk,pn,sig,zl.value,msg,16): return 2
        rank.add([x for byte in bytes(sig[:100]) for x in (byte&15,byte>>4)]); accepted+=1
    control=Rank(); rng=random.Random(18)
    for _ in range(180): control.add([rng.randrange(16) for _ in range(200)])
    print(f"accepted={accepted}/{attempts}; signature rank={len(rank)}/200; control rank={len(control)}/200")
    ok=accepted==180 and len(rank)==164 and len(control)==180
    print("CONFIRMED: signatures expose 36 constraints" if ok else "NOT CONFIRMED")
    return 0 if ok else 1

if __name__ == "__main__": raise SystemExit(main())
