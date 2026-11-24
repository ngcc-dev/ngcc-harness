#!/usr/bin/env python3
"""Fixing-cost helper: ZC-m linear layers, submitted and v1.1 (hash-31; reused by hash-32).

Checks whether the fixed differences of the Lead findings are still fixed by
the linear layer L = rho' . theta . rho when the shift offsets of the ZC-DM
v1.1 specification (Appendix A, physical pp. 37-38) are used, and prints the
dimension of the space of differences that each linear layer fixes. With
--min-weight it also computes the least number of active bits of a nonzero
difference that a v1.1 layer fixes (Brouwer-Zimmermann enumeration, about 45 s).

Only the linear layers are modelled; chi, iota, the mode and the hash are not.
The submitted offsets are read from the reference permutation sources as a
control. Run from the repository root:
  python3 -B hash-31/fixing_cost_linear_layer.py [--min-weight]
"""
import sys

sys.dont_write_bytecode = True

import itertools
import math
import re
from pathlib import Path

W = 64
M64 = (1 << W) - 1
SRC = Path(__file__).resolve().parent / "ZC-DMC/Implementations/lib/low"

# Offsets (dx, dz): plane y is moved by dx lanes in x and rotated left by dz in z.
# theta: E[x] = ROTL(P[x - dx1], dz1) ^ ROTL(P[x - dx2], dz2).
ZC1536 = {
    "X": 8, "Y": 3,
    "v1": {"rho": {1: (3, 3), 2: (5, 0)}, "rho2": {1: (3, 3), 2: (5, 0)},
           "theta": ((1, 20), (1, 56))},
    "v1.1": {"rho": {1: (3, 3), 2: (5, 1)}, "rho2": {1: (3, 3), 2: (5, 1)},
             "theta": ((1, 23), (1, 56))},
}
ZC1280 = {
    "X": 4, "Y": 5,
    "v1": {"rho": {1: (1, 5), 2: (3, 24), 3: (0, 1), 4: (2, 3)},
           "rho2": {1: (1, 5), 2: (3, 24), 3: (0, 1), 4: (2, 3)},
           "theta": ((1, 5), (1, 14))},
    "v1.1": {"rho": {1: (1, 5), 2: (3, 24), 3: (0, 1), 4: (2, 3)},
             "rho2": {1: (2, 9), 2: (3, 25), 3: (0, 1), 4: (2, 4)},
             "theta": ((1, 5), (1, 14))},
}


def rotl(v, n):
    n %= W
    return ((v << n) | (v >> (W - n))) & M64 if n else v


def rho(A, X, Y, off):
    B = list(A)
    for y, (dx, dz) in off.items():
        for x in range(X):
            B[y * X + (x + dx) % X] = rotl(A[y * X + x], dz)
    return B


def theta(A, X, Y, th):
    (a1, b1), (a2, b2) = th
    P = [0] * X
    for y in range(Y):
        for x in range(X):
            P[x] ^= A[y * X + x]
    E = [rotl(P[(x - a1) % X], b1) ^ rotl(P[(x - a2) % X], b2) for x in range(X)]
    return [A[y * X + x] ^ E[x] for y in range(Y) for x in range(X)]


def linear(A, perm, layer):
    X, Y = perm["X"], perm["Y"]
    return rho(theta(rho(A, X, Y, layer["rho"]), X, Y, layer["theta"]), X, Y, layer["rho2"])


def to_int(A):
    v = 0
    for i, lane in enumerate(A):
        v |= lane << (W * i)
    return v


def rank(vectors):
    basis = {}
    for v in vectors:
        while v:
            h = v.bit_length() - 1
            if h in basis:
                v ^= basis[h]
            else:
                basis[h] = v
                break
    return len(basis)


def periodic_basis(n, p):
    """States whose lanes repeat with period p along z: one basis vector per lane and residue."""
    for lane in range(n):
        for r in range(p):
            A = [0] * n
            A[lane] = sum(1 << z for z in range(r, W, p))
            yield lane * p + r, A


def fixed_space(perm, layer, p):
    """Basis of {D of z-period p : L(D) = D}, on folded coordinates lane * p + (z mod p).

    A folded coordinate stands for 64 / p bits of the state."""
    piv, ker = {}, []
    for j, A in periodic_basis(perm["X"] * perm["Y"], p):
        v, t = to_int(linear(A, perm, layer)) ^ to_int(A), 1 << j
        while v:
            b = v.bit_length() - 1
            if b not in piv:
                piv[b] = (v, t)
                break
            v, t = v ^ piv[b][0], t ^ piv[b][1]
        if not v:
            ker.append(t)
    return ker


def min_weight(ker, length):
    """Least Hamming weight of a nonzero vector in span(ker), Brouwer-Zimmermann style.

    Bring ker to systematic form on as many disjoint information sets as exist,
    then enumerate sums of t rows of each form until m * (t + 1) reaches the
    least weight found: any vector not enumerated has at least t + 1 ones on
    each of the m sets."""
    gens, used = [], set()
    while True:
        rows, info = list(ker), []
        for c in (c for c in range(length) if c not in used):
            r = len(info)
            p = next((i for i in range(r, len(rows)) if rows[i] >> c & 1), None)
            if p is None:
                continue
            rows[r], rows[p] = rows[p], rows[r]
            for i in range(len(rows)):
                if i != r and rows[i] >> c & 1:
                    rows[i] ^= rows[r]
            info.append(c)
            if len(info) == len(rows):
                break
        if len(info) < len(rows):
            break
        gens.append(rows)
        used |= set(info)
    m = len(gens)
    best = min(bin(v).count("1") for g in gens for v in g)
    t = 1
    while m * (t + 1) < best:
        t += 1
        for g in gens:
            for comb in itertools.combinations(g, t):
                v = 0
                for row in comb:
                    v ^= row
                best = min(best, bin(v).count("1"))
    return best, m, t


def weight(A):
    return sum(bin(v).count("1") for v in A)


def is_fixed(A, perm, layer):
    return linear(A, perm, layer) == A


def source_offsets():
    """Read the submitted offsets from the reference permutations (control)."""
    t = (SRC / "ZuD-1536/plain/ZuD1536-plain.c").read_text()
    th = sorted(set(int(k) for k in re.findall(r"ROTL64\(p7, (\d+)\)", t)))
    r2 = re.search(r"A\[16\]\s*=\s*(ROTL64\()?a2(\d)(, (\d+)\))?;", t)
    r1 = re.search(r"A\[8\]\s*=\s*ROTL64\(a1(\d), (\d+)\);", t)
    s1536 = {"theta": th, "plane1": ((0 - int(r1.group(1))) % 8, int(r1.group(2))),
             "plane2": ((0 - int(r2.group(2))) % 8, int(r2.group(4) or 0))}
    t = (SRC / "ZuD-1280/plain/ZuD1280-plain.c").read_text()
    th = sorted(set(int(k) for k in re.findall(r"ROTL64\(p3, (\d+)\)", t)))
    pl = {}
    for y, lane in ((1, 4), (2, 8), (3, 12), (4, 16)):
        m = re.search(r"A\[%d\]\s*=\s*ROTL64\(a%d(\d), (\d+)\);" % (lane, y), t)
        pl[y] = ((0 - int(m.group(1))) % 4, int(m.group(2)))
    return s1536, {"theta": th, "planes": pl}


def pattern_1536(kind, z):
    A = [0] * 24
    for x in range(8):
        p0 = kind == "W" or (kind == "D" and x % 2 == 1) or (kind == "E" and x % 2 == 0)
        p2 = kind == "W" or (kind == "D" and x % 2 == 0) or (kind == "E" and x % 2 == 1)
        if p0:
            A[x] |= 1 << z
        if p2:
            A[16 + x] |= 1 << z
    return A


def pattern_1280(x0, z0):
    """Four plane-2 bits on the orbit of (2, 48) and their first-rho images in plane 0."""
    A = [0] * 20
    for k in range(4):
        x, z = (x0 + 2 * k) % 4, (z0 + 48 * k) % W
        A[8 + x] |= 1 << z
        A[(x + 3) % 4] |= 1 << ((z + 24) % W)
    return A


def order(dx, dz, X):
    return math.lcm(X // math.gcd(dx % X, X), W // math.gcd(dz % W, W))


def main():
    s1536, s1280 = source_offsets()
    print("Submitted offsets read from the reference sources (control):")
    print("  ZC-1536 theta shifts", s1536["theta"], "plane 1", s1536["plane1"],
          "plane 2", s1536["plane2"])
    print("  ZC-1280 theta shifts", s1280["theta"], "planes", s1280["planes"])
    assert s1536["theta"] == [20, 56] and s1536["plane1"] == (3, 3) and s1536["plane2"] == (5, 0)
    assert s1280["theta"] == [5, 14] and s1280["planes"] == ZC1280["v1"]["rho"]

    print("\ntheta cancellation: ROTL(p,a) ^ ROTL(p,b) = 0 exactly for lanes p of period gcd(b-a, 64)")
    for name, (a, b) in (("ZC-1536 v1", (20, 56)), ("ZC-1536 v1.1", (23, 56)),
                         ("ZC-1280 v1 and v1.1", (5, 14))):
        g = math.gcd(b - a, W)
        print(f"  {name}: b-a = {b - a}, period {g}, {2 ** g} cancelling lane values")

    print("\nplane-2 displacement over both plane shifts, and its order on the lane grid")
    for name, perm in (("ZC-1536", ZC1536), ("ZC-1280", ZC1280)):
        for ver in ("v1", "v1.1"):
            d1, d2 = perm[ver]["rho"][2], perm[ver]["rho2"][2]
            dx, dz = (d1[0] + d2[0]) % perm["X"], (d1[1] + d2[1]) % W
            print(f"  {name} {ver}: ({dx}, {dz}), order {order(dx, dz, perm['X'])}")

    th_only = dict(ZC1536["v1"], theta=ZC1536["v1.1"]["theta"])
    rho_only = dict(ZC1536["v1.1"], theta=ZC1536["v1"]["theta"])
    variants = (("v1", ZC1536["v1"]), ("v1.1", ZC1536["v1.1"]),
                ("v1 rho + v1.1 theta", th_only), ("v1.1 rho + v1 theta", rho_only))

    print("\nZC-1536, Delta A0[0] = 0x1111111111111111")
    D = [0] * 24
    D[0] = 0x1111111111111111
    for name, layer in variants:
        L = linear(D, ZC1536, layer)
        print(f"  {name}: fixed = {L == D}, weight of L(D) = {weight(L)}")

    print("\nZC-1536, 8-column patterns D, E, W at each z (number of the 64 values of z where L fixes them)")
    for name, layer in variants:
        counts = {k: sum(is_fixed(pattern_1536(k, z), ZC1536, layer) for z in range(W))
                  for k in "DEW"}
        print(f"  {name}: D {counts['D']}, E {counts['E']}, W {counts['W']}")

    print("\nZC-1536, plane-2 offset (a, 0) with no z-rotation, v1.1 theta: plane-2 bits on the"
          " orbit of x under +2a at z = 0 and their first-rho images in plane 0")
    found = []
    for a in range(8):
        layer = dict(ZC1536["v1.1"], rho={1: (3, 3), 2: (a, 0)}, rho2={1: (3, 3), 2: (a, 0)})
        A = [0] * 24
        x = 0
        while not A[16 + x] & 1:
            A[16 + x] |= 1
            A[(x + a) % 8] ^= 1
            x = (x + 2 * a) % 8
        found.append(f"a={a}: {'fixed' if is_fixed(A, ZC1536, layer) else 'not fixed'} ({weight(A)} bits)")
    print("  " + "; ".join(found))

    print("\nZC-1280, 8-bit pattern: 4 plane-2 bits on the orbit of (2, 48) and their first-rho images in plane 0")
    rate_lanes = 11  # ZC-1280-512: r = 704 bits
    for ver in ("v1", "v1.1"):
        n_fixed = sum(is_fixed(pattern_1280(x, z), ZC1280, ZC1280[ver])
                      for x in range(4) for z in range(W))
        print(f"  {ver}: fixed at {n_fixed} of 256 start positions")
    A = pattern_1280(0, 0)
    cols = {(i % 4, z) for i, v in enumerate(A) for z in range(W) if v >> z & 1}
    print(f"  start x = 0: weight {weight(A)}, {len(cols)} distinct columns, "
          f"rate-only at -512: {all(v == 0 for v in A[rate_lanes:])}")

    print("\nDimension of the differences that L fixes: all / of z-period dividing 8 / constant along z")
    rows = [("ZC-1536 " + n, ZC1536, layer) for n, layer in variants]
    rows += [("ZC-1280 " + v, ZC1280, ZC1280[v]) for v in ("v1", "v1.1")]
    for name, perm, layer in rows:
        dims = [len(fixed_space(perm, layer, p)) for p in (W, 8, 1)]
        print(f"  {name}: {dims[0]} / {dims[1]} / {dims[2]}")

    if "--min-weight" in sys.argv[1:]:
        print("\nLeast number of active bits of a nonzero difference that the v1.1 L fixes")
        print("  (v1: 8, the D and 8-bit ZC-1280 patterns above)")
        for name, perm in (("ZC-1536", ZC1536), ("ZC-1280", ZC1280)):
            ker = fixed_space(perm, perm["v1.1"], 8)
            assert len(ker) == len(fixed_space(perm, perm["v1.1"], W))  # all have period 8
            best, m, t = min_weight(ker, perm["X"] * perm["Y"] * 8)
            print(f"  {name} v1.1: {best} folded bits x 8 = {best * 8} "
                  f"({m} information set, sums of up to {t} rows enumerated)")


if __name__ == "__main__":
    main()
