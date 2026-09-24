#!/usr/bin/env python3
"""Generate AFS-TrEDM-S6 IV constants.

IV domain string:
AFS-TrEDM-v2|d=<d>|r=<r>|c=<c>|b=1600|nr=<nr>|sbox=AFS64_t5_k2|linear=AFS-LMDS-1600-S6
"""
MASK = (1 << 64) - 1

def fnv1a64(s: str) -> int:
    h = 0xcbf29ce484222325
    for b in s.encode("ascii"):
        h ^= b
        h = (h * 0x100000001b3) & MASK
    return h

def splitmix64_next(x: int):
    x = (x + 0x9E3779B97F4A7C15) & MASK
    z = x
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK
    z = (z ^ (z >> 31)) & MASK
    return x, z

def iv_for(d: int, r: int, c: int, nr: int):
    s = f"AFS-TrEDM-v2|d={d}|r={r}|c={c}|b=1600|nr={nr}|sbox=AFS64_t5_k2|linear=AFS-LMDS-1600-S6"
    x = fnv1a64(s)
    out = []
    for _ in range(25):
        x, z = splitmix64_next(x)
        out.append(z)
    return s, out

def main():
    for d, r, c, nr in [(512,1024,576,12),(768,768,832,20),(1024,512,1088,24)]:
        s, iv = iv_for(d, r, c, nr)
        print(f"IV_{d} domain: {s}")
        for i in range(0,25,5):
            print("    " + ", ".join(f"0x{x:016X}ULL" for x in iv[i:i+5]) + ("," if i < 20 else ""))
        print()

if __name__ == "__main__":
    main()
