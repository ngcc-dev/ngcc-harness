#!/usr/bin/env python3
"""Message modification for the period-4 ZC-1536 trail in the REAL hash setting:
the chaining value S is fixed (known, from a prefix), only the message block (rate)
is free.  Variables: the plane-0 rate bits of X = S ^ (M || 0^c).  All plane-1/2 bits
of X are constants (capacity from S; plane-1 rate lanes chosen by us), so the round-1
chi output y1 is AFFINE in the variables, and rounds 1..3 of the trail can again be
enforced by linear algebra (round-1 conditions on capacity bits must be met by S,
i.e. by the choice of prefix).

build_rate(S, rate, XL, Z0, rcs, plane1, seed) -> (particular, kernel, info, fixed-X)
"""
import random
import mm
from mm import z, LROW, bit, solve, cond_bits, active_cols, chi_int, to_int, to_lanes, B, X, Y
from zc import M64


def affine_y1(Xl, rate, rcs):
    """Return (vars, Y0, Yv): y1 = Y0 ^ sum v_i Yv_i, vars = plane-0 rate bit positions."""
    nlanes = rate // 64
    vars_ = [(x, zz) for x in range(X) if x < nlanes for zz in range(64)]   # plane-0 lanes in rate
    base = list(Xl)
    for (x, zz) in vars_:
        base[x] &= ~(1 << zz) & M64
    Y0 = 0
    Yv = []
    A = list(base)
    A[0] ^= rcs[0]
    y_zero = chi_int(to_int(A))
    Y0 = y_zero
    for (x, zz) in vars_:
        Bv = list(A)
        Bv[x] ^= 1 << zz
        Yv.append(chi_int(to_int(Bv)) ^ y_zero)      # column-local, affine in v (a1,a2 fixed)
    return vars_, Y0, Yv


def par(v):
    return bin(v).count("1") & 1


def build_rate(Xl, rate, XL, Z0, rcs, seed=1, ff=0):
    """ff: constant (int over state bits) added to the round-3 chi input (EDMC middle feed-forward)."""
    vars_, Y0, Yv = affine_y1(Xl, rate, rcs)
    nv = len(vars_)

    def row_for(lrow):
        r = 0
        for i, yv in enumerate(Yv):
            if par(lrow & yv):
                r |= 1 << i
        return r, par(lrow & Y0)

    # check round-1 conditions on the fixed bits
    A = list(Xl)
    for b_, v in cond_bits(XL, Z0):
        if ((A[b_ // 64] >> (b_ % 64)) & 1) != v:
            return None, "round-1 condition on fixed bit violated"
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
    act = set(active_cols(XL, Z0))
    C |= act
    rng = random.Random(seed)
    Cl = sorted(C)
    # equations e <-> (column, plane): M v = t, with t linear in the chosen u2|C values
    erows, ebit, econst = [], [], []
    for (x, zz) in Cl:
        for y in range(Y):
            j = bit(y, x, zz)
            r, c0 = row_for(LROW[j])
            c2bit = (rcs[1] >> zz) & 1 if (y == 0 and x == 0) else 0
            erows.append(r); ebit.append(j); econst.append(c0 ^ c2bit)
    # left kernel of M: combinations of equations whose rows cancel
    piv = {}
    deps = []
    for e, r in enumerate(erows):
        comb = 1 << e
        while r:
            p = r.bit_length() - 1
            if p in piv:
                r ^= piv[p][0]; comb ^= piv[p][1]
            else:
                piv[p] = (r, comb)
                break
        if r == 0:
            deps.append(comb)
    for attempt in range(60):
        # u2 on C: planes 1,2 fixed (active: 0,1; others random), plane-0 bit w free
        # step A: all u2|C bits as unknowns (index 3*col + plane); impose the linear
        # dependency constraints and the active-column values; take a random solution
        # and keep its plane-1/2 bits as the fixed values for step B.
        uidx = {}
        for i, (x, zz) in enumerate(Cl):
            for y in range(Y):
                uidx[bit(y, x, zz)] = 3 * i + y
        aeqs = []
        for (x, zz) in act:
            aeqs.append((1 << uidx[bit(1, x, zz)], 0))
            aeqs.append((1 << uidx[bit(2, x, zz)], 1))
        for comb in deps:
            r, c = 0, 0
            cc = comb
            while cc:
                low = cc & -cc; e = low.bit_length() - 1; cc ^= low
                r ^= 1 << uidx[ebit[e]]; c ^= econst[e]
            aeqs.append((r, c))
        asol = solve(aeqs, 3 * len(Cl))
        if asol is None:
            return None, "dependency constraints inconsistent"
        ap, ak = asol
        u = ap
        for k in ak:
            if rng.getrandbits(1):
                u ^= k
        fixed = {}
        for i, (x, zz) in enumerate(Cl):
            fixed[(x, zz)] = ((u >> (3 * i + 1)) & 1, (u >> (3 * i + 2)) & 1)
        widx = {c: i for i, c in enumerate(Cl)}
        # y2 column bits as affine functions of w (bitmask over w, const)
        y2aff = {}
        for (x, zz) in Cl:
            a1, a2 = fixed[(x, zz)]
            i = widx[(x, zz)]
            for w in (0, 1):
                yv = [w ^ ((1 - a1) & a2), a1 ^ ((1 - a2) & w), a2 ^ ((1 - w) & a1)]
                if w == 0:
                    y0 = yv
                else:
                    for y in range(Y):
                        y2aff[bit(y, x, zz)] = ((1 << i) if (y0[y] ^ yv[y]) else 0, y0[y])
        weqs = []
        for b_, v in cond_bits(XL, Z0):                 # R3 on y2 (u3 = L(y2) ^ c3 ^ ff)
            v ^= (ff >> b_) & 1
            r, c = 0, 0
            t2 = LROW[b_]
            while t2:
                low = t2 & -t2; j = low.bit_length() - 1; t2 ^= low
                rr, cc = y2aff[j]
                r ^= rr; c ^= cc
            weqs.append((r, v ^ c))
        u2aff = {}
        for (x, zz) in Cl:
            a1, a2 = fixed[(x, zz)]
            u2aff[bit(0, x, zz)] = (1 << widx[(x, zz)], 0)
            u2aff[bit(1, x, zz)] = (0, a1)
            u2aff[bit(2, x, zz)] = (0, a2)
        for comb in deps:                               # lambda . t = 0
            r, c = 0, 0
            e = 0
            cc = comb
            while cc:
                low = cc & -cc; e = low.bit_length() - 1; cc ^= low
                rr, c1 = u2aff[ebit[e]]
                r ^= rr; c ^= c1 ^ econst[e]
            weqs.append((r, c))
        wsol = solve(weqs, len(Cl))
        if wsol is None:
            continue
        wp, wk = wsol
        w = wp
        for k in wk:
            if rng.getrandbits(1):
                w ^= k
        eqs = []
        for e in range(len(erows)):
            rr, c1 = u2aff[ebit[e]]
            val = (par(rr & w)) ^ c1
            eqs.append((erows[e], val ^ econst[e]))
        sol = solve(eqs, nv)
        if sol is not None:
            part, kern = sol
            return (part, kern, vars_, {"C": len(C), "eqs": len(eqs), "dim": len(kern), "nv": nv,
                                        "attempt": attempt, "deps": len(deps)}), None
    return None, "inconsistent"


def x_from_vars(Xl, vars_, v):
    A = list(Xl)
    for i, (x, zz) in enumerate(vars_):
        A[x] = (A[x] & ~(1 << zz)) | (((v >> i) & 1) << zz)
    return A
