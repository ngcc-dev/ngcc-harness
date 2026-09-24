#!/usr/bin/env python3
"""Lightweight algebra checks for AFS-LMDS-1600-S6."""
import math

P = 0b100101  # x^5 + x^2 + 1, bit representation

def gf_mul(a, b):
    res = 0
    aa = a
    bb = b
    while bb:
        if bb & 1:
            res ^= aa
        bb >>= 1
        aa <<= 1
        if aa & 0x20:
            aa ^= P
    return res & 0x1f

def gf_pow(a, e):
    r = 1
    while e:
        if e & 1:
            r = gf_mul(r, a)
        a = gf_mul(a, a)
        e >>= 1
    return r

def gf_inv(a):
    if a == 0:
        raise ZeroDivisionError
    return gf_pow(a, 30)

C = [
    [0x11,0x13,0x02,0x01,0x05],
    [0x10,0x06,0x01,0x02,0x09],
    [0x13,0x11,0x05,0x09,0x02],
    [0x06,0x10,0x09,0x05,0x01],
    [0x01,0x09,0x10,0x11,0x06],
]

def det(mat):
    n = len(mat)
    m = [row[:] for row in mat]
    d = 1
    for col in range(n):
        piv = None
        for r in range(col, n):
            if m[r][col] != 0:
                piv = r
                break
        if piv is None:
            return 0
        if piv != col:
            m[col], m[piv] = m[piv], m[col]
        pv = m[col][col]
        d = gf_mul(d, pv)
        inv = gf_inv(pv)
        for j in range(col, n):
            m[col][j] = gf_mul(m[col][j], inv)
        for r in range(n):
            if r != col and m[r][col] != 0:
                f = m[r][col]
                for j in range(col, n):
                    m[r][j] ^= gf_mul(f, m[col][j])
    return d

def pi(x, y):
    return ((x + y) % 5, (3*x) % 5)

def main():
    # GF nonzero inverses.
    assert all(gf_mul(a, gf_inv(a)) == 1 for a in range(1,32))
    # MDS minors.
    import itertools
    for k in range(1,6):
        for rows in itertools.combinations(range(5), k):
            for cols in itertools.combinations(range(5), k):
                minor = [[C[i][j] for j in cols] for i in rows]
                assert det(minor) != 0, (k, rows, cols)
    # pi 1 + 24 orbit.
    seen = {(0,0)}
    cur = (1,0)
    cyc = []
    for _ in range(24):
        cyc.append(cur)
        seen.add(cur)
        cur = pi(*cur)
    assert cur == (1,0)
    assert len(cyc) == 24 and len(seen) == 25
    # mu inverse.
    def poly_mul_mod(a,b):
        r=0
        for i in range(64):
            if (a>>i)&1:
                for j in range(64):
                    if (b>>j)&1:
                        r ^= 1 << ((i+j) % 64)
        return r
    m = (1<<0) | (1<<17) | (1<<32)
    inv = (1<<30) | (1<<47) | (1<<62)
    assert poly_mul_mod(m, inv) == 1
    print("all AFS-LMDS-1600-S6 algebra checks passed")

if __name__ == "__main__":
    main()
