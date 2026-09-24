#!/usr/bin/env python3
"""Check public RC32(round,lane) constants used by AFS-TrEDM-S6 profiles."""
MASK = (1 << 64) - 1

def splitmix64_value(x: int) -> int:
    x = (x + 0x9E3779B97F4A7C15) & MASK
    x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & MASK
    x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & MASK
    return (x ^ (x >> 31)) & MASK

def rc32(round_idx: int, lane: int) -> int:
    seed = 0xB4C5F1D62E1738A9
    idx = round_idx * 25 + lane
    z = splitmix64_value((seed + idx * 0x9E3779B97F4A7C15) & MASK)
    return (z ^ (z >> 32)) & 0xffffffff

def main():
    ok = True
    for nr in (12, 20, 24):
        vals = [rc32(r,l) for r in range(nr) for l in range(25)]
        zeros = [i for i,v in enumerate(vals) if v == 0]
        dup = len(vals) - len(set(vals))
        print(f"nr={nr}: constants={len(vals)}, unique={len(set(vals))}, zeros={len(zeros)}, duplicates={dup}")
        if zeros or dup:
            ok = False
    if not ok:
        raise SystemExit(1)

if __name__ == "__main__":
    main()
