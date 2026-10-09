#!/usr/bin/env python3
"""Message modification for the period-4 iterative trail of ZC-1536 (3 rounds for free).

Trail: Delta = single lane A0[XL] with bits z = Z0 mod 4 (16 active columns per round).
The constant-free round maps Delta to Delta with probability 2^-32 (theta is the identity on
period-4 column parities; plane 0 is not moved by rho).  Per active column the chi input
must satisfy a1 = 0, a2 = 1  (equivalently, chi output y1 = 0, y2 = 1; chi is an involution).

We choose the round-1 chi output y1 (1536 free bits; X = chi(y1) ^ iota) subject to
  R1: y1[plane1,2] conditions in the 16 active columns            (32 linear eqs)
  R2: u2 = L(y1) ^ c2 conditions in the active columns              (32 linear eqs)
  R3: u3 = L(y2) ^ c3 conditions: fix *every* round-2 chi column C that feeds the 32 R3
      condition bits (and the active round-2 columns) to constants chosen so that R3
      holds; this costs 3|C| linear equations on y1.
All three rounds then follow the trail deterministically; rounds >= 4 are paid for
probabilistically (2^-32 each).
Usage (library): build(XL, Z0, rcs, seed) -> (particular y1 int, kernel basis list, info)
"""
import random
from zc import ZC, rotl, M64, RC_IMPL

z = ZC(1536)
Y, X, B = 3, 8, 1536


def to_int(A):
    v = 0
    for i, a in enumerate(A):
        v |= a << (64 * i)
    return v


def to_lanes(v):
    return [(v >> (64 * i)) & M64 for i in range(z.N)]


def bit(y, x, zz):
    return (y * X + x) * 64 + zz


# rows of L (as ints over input bits)
_cols = []
for j in range(B):
    A = [0] * z.N
    A[j // 64] = 1 << (j % 64)
    _cols.append(to_int(z.lin(A)))
LROW = [0] * B
for j, v in enumerate(_cols):
    while v:
        low = v & -v
        LROW[low.bit_length() - 1] |= 1 << j
        v ^= low


def chi_int(v):
    return to_int(z.chi(to_lanes(v)))


def lin_int(v):
    return to_int(z.lin(to_lanes(v)))


def solve(eqs, nvars=B):
    """eqs: list of (row int, rhs bit). Returns (particular, kernel basis) or None."""
    piv = {}       # pivot bit -> (row, rhs)
    for row, rhs in eqs:
        for p, (pr, prhs) in piv.items():
            if (row >> p) & 1:
                row ^= pr
                rhs ^= prhs
        if row == 0:
            if rhs:
                return None
            continue
        p = row.bit_length() - 1
        # eliminate p from existing pivots (keep reduced)
        for q in list(piv):
            pr, prhs = piv[q]
            if (pr >> p) & 1:
                piv[q] = (pr ^ row, prhs ^ rhs)
        piv[p] = (row, rhs)
    part = 0
    for p, (row, rhs) in piv.items():
        if rhs:
            part |= 1 << p
    # free variables -> kernel vectors
    free = [j for j in range(nvars) if j not in piv]
    kern = []
    for f in free:
        v = 1 << f
        for p, (row, rhs) in piv.items():
            if (row >> f) & 1:
                v |= 1 << p
        kern.append(v)
    return part, kern


def active_cols(XL, Z0):
    return [(XL, zz) for zz in range(Z0 % 4, 64, 4)]


def cond_bits(XL, Z0):
    """(bit index, required value) of the chi-input conditions for the active columns"""
    out = []
    for (x, zz) in active_cols(XL, Z0):
        out.append((bit(1, x, zz), 0))
        out.append((bit(2, x, zz), 1))
    return out


def build(XL, Z0, rcs, seed=1):
    for s in range(seed, seed + 50):      # a random y2 choice can make the system inconsistent: retry
        r = _build(XL, Z0, rcs, s)
        if r is not None:
            return r
    raise RuntimeError("no consistent system")


def _build(XL, Z0, rcs, seed):
    rng = random.Random(seed)
    eqs = []
    # R1 on y1 (chi output): y1 bit plane1 = 0, plane2 = 1
    for b_, v in cond_bits(XL, Z0):
        eqs.append((1 << b_, v))
    # R2 on u2 = L(y1) ^ c2 (c2 only on lane 0 plane 0 -> never touches plane 1/2 bits)
    for b_, v in cond_bits(XL, Z0):
        eqs.append((LROW[b_], v))
    # R3: u3 = L(y2) ^ c3 ; condition bits depend on y2 bits in LROW[b]
    needed = 0
    for b_, v in cond_bits(XL, Z0):
        needed |= LROW[b_]
    C = set()
    t = needed
    while t:
        low = t & -t
        j = low.bit_length() - 1
        t ^= low
        C.add(((j // 64) % X, j % 64))
    C |= set(active_cols(XL, Z0))
    act = set(active_cols(XL, Z0))
    # choose y2 on the columns of C: active columns have u2=(a0,0,1) -> y2=(a0^1,0,1)
    # other columns free; impose the 32 R3 equations on y2 restricted to C
    y2eqs = []
    for (x, zz) in act:
        y2eqs.append((1 << bit(1, x, zz), 0))
        y2eqs.append((1 << bit(2, x, zz), 1))
    for b_, v in cond_bits(XL, Z0):
        y2eqs.append((LROW[b_], v ^ ((rcs[2] >> (b_ % 64)) & 1 if b_ < 64 else 0)))
    sol = solve(y2eqs)
    assert sol is not None
    part, kern = sol
    y2 = part
    for k in kern:
        if rng.getrandbits(1):
            y2 ^= k
    # restrict to C, convert to required u2 on C via chi (involution, column-wise)
    u2full = chi_int(y2)
    for (x, zz) in C:
        for y in range(Y):
            j = bit(y, x, zz)
            c2bit = (rcs[1] >> zz) & 1 if (y == 0 and x == 0) else 0
            eqs.append((LROW[j], ((u2full >> j) & 1) ^ c2bit))
    sol = solve(eqs)
    if sol is None:
        return None
    part, kern = sol
    return part, kern, {"C": len(C), "eqs": len(eqs), "dim": len(kern), "seed": seed}


def delta(XL, Z0):
    A = [0] * z.N
    A[XL] = sum(1 << zz for zz in range(Z0 % 4, 64, 4))
    return A


def x_from_y1(y1, rcs):
    A = z.chi(to_lanes(y1))          # chi^-1 = chi (3-bit chi is an involution)
    A[0] ^= rcs[0]
    return A


def trail_rounds(A, D, rcs):
    """number of leading rounds for which the pair (A, A^D) keeps difference D"""
    Bp = [a ^ d for a, d in zip(A, D)]
    for r, rc in enumerate(rcs):
        A = z.round(A, rc)
        Bp = z.round(Bp, rc)
        if [a ^ b for a, b in zip(A, Bp)] != D:
            return r
    return len(rcs)


if __name__ == "__main__":
    import sys
    rcs = RC_IMPL[6:]                    # implemented h = last six rounds (rc6..rc1)
    for XL, Z0 in ((7, 0), (0, 0), (3, 1)):
        part, kern, info = build(XL, Z0, rcs)
        D = delta(XL, Z0)
        rng = random.Random(5)
        hist = [0] * 7
        for t in range(2000):
            y1 = part
            for k in kern:
                if rng.getrandbits(1):
                    y1 ^= k
            hist[trail_rounds(x_from_y1(y1, rcs), D, rcs)] += 1
        print(f"lane A0[{XL}] z={Z0}mod4 (seed {info['seed']}): |C|={info['C']} eqs={info['eqs']} solution-space dim={info['dim']}; "
              f"2000 random solutions, #rounds the trail holds (0..6): {hist}")


# ---------------------------------------------------------------------------
# Generalisation: arbitrary period-4 single-bit-per-column trail D1 -> D2 -> D3 ...
# (D_{r+1} = L(D_r) because chi keeps every 1-bit column difference).
def gen_conds(Dl):
    """chi-input conditions (bit, value) for a difference with single-bit active columns"""
    out = []
    for lane, v in enumerate(Dl):
        y0, x = divmod(lane, X)
        while v:
            low = v & -v
            zz = low.bit_length() - 1
            v ^= low
            out.append((bit((y0 + 1) % Y, x, zz), 0))
            out.append((bit((y0 + 2) % Y, x, zz), 1))
    return out


def gen_cols(Dl):
    return sorted({(lane % X, zz) for lane, v in enumerate(Dl) for zz in range(64) if (v >> zz) & 1})


def build_general(D1, rcs, seed=1, ff=0):
    """3 rounds of linear message modification for trail D1 (free start, all 1536 bits free)."""
    D2 = z.lin(D1)
    rng = random.Random(seed)
    for attempt in range(60):
        eqs = [(1 << b_, v) for b_, v in gen_conds(D1)]                     # R1 on y1
        for b_, v in gen_conds(D2):                                          # R2 on u2
            c2bit = (rcs[1] >> (b_ % 64)) & 1 if b_ < 64 else 0
            eqs.append((LROW[b_], v ^ c2bit))
        c3 = gen_conds(z.lin(D2))
        needed = 0
        for b_, v in c3:
            needed |= LROW[b_]
        C = {((j // 64) % X, j % 64) for j in range(B) if (needed >> j) & 1} | set(gen_cols(D2))
        y2eqs = []
        for b_, v in gen_conds(D2):          # active round-2 columns: y2 bits (y0+1)=0,(y0+2)=1
            y2eqs.append((1 << b_, v))
        for b_, v in c3:
            c3bit = (rcs[2] >> (b_ % 64)) & 1 if b_ < 64 else 0
            y2eqs.append((LROW[b_], v ^ c3bit ^ ((ff >> b_) & 1)))
        sol = solve(y2eqs)
        if sol is None:
            continue
        part, kern = sol
        y2 = part
        for k in kern:
            if rng.getrandbits(1):
                y2 ^= k
        u2full = chi_int(y2)
        for (x, zz) in C:
            for y in range(Y):
                j = bit(y, x, zz)
                c2bit = (rcs[1] >> zz) & 1 if (y == 0 and x == 0) else 0
                eqs.append((LROW[j], ((u2full >> j) & 1) ^ c2bit))
        sol = solve(eqs)
        if sol is not None:
            return sol[0], sol[1], {"C": len(C), "eqs": len(eqs), "dim": len(sol[1]), "attempt": attempt}
    raise RuntimeError("inconsistent")
