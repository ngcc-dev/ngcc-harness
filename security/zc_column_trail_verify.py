#!/usr/bin/env python3
"""Check reduced-round ZC collisions with the independent Python mode model.

Input: blocks of "MODE n R" followed by "m1 <hex>" and "m2 <hex>".  Each pair
must collide for the stated last-R-round permutation, keep colliding under
common suffixes, and differ under the full 12-round hash (control).
"""
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent / "keccak_techniques" / "zc"))
from zcmodes import zc_hash  # noqa: E402

def pairs(lines):
    head, ms = None, {}
    for line in lines:
        t = line.split()
        if not t or t[0].startswith("#"):
            continue
        if t[0] in ("DM", "DMC") and len(t) == 3:
            head, ms = (t[0], int(t[1]), int(t[2])), {}
        elif t[0] in ("m1", "m2") and len(t) == 2:
            ms[t[0]] = bytes.fromhex(t[1])
            if head and len(ms) == 2:
                yield head + (ms["m1"], ms["m2"])
                ms = {}

def main():
    ok = n_pairs = 0
    for mode, n, rounds, m1, m2 in pairs(sys.stdin):
        n_pairs += 1
        h = lambda m, r=rounds: zc_hash(mode, 1536, n, m, rounds=r)
        reduced = m1 != m2 and h(m1) == h(m2) and all(
            h(m1 + s) == h(m2 + s) for s in (b"", b"abc", b"\x00" * 200))
        control = h(m1, 12) != h(m2, 12)
        print(f"ZC-{mode}-1536-{n} R={rounds}: {len(m1)}-byte messages, "
              f"reduced-round collision {reduced}, 12-round digests differ {control}")
        ok += reduced and control
    return 0 if n_pairs and ok == n_pairs else 1

if __name__ == "__main__":
    raise SystemExit(main())
