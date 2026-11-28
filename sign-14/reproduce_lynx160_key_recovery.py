#!/usr/bin/env python3
"""sign-14-6: one guess of the Lynxer-160 key-recovery route, run at full size.

With the 160-bit matrices as the submitted owf.c reads them, L0 has rank 100.
Guessing u = L0(k) (2^100 values) fixes v2 = S2(u + c0) and a = L2(v2), so
the output y = L1(v1)*a + L3(k + v1 + v2) is F2-linear in (k, v1). Together
with L0(k) = u it leaves an affine space of about 60 unknowns. The inverse
S-box v1 = (k + c0)^-1 then gives the five relations x*v1 = 1, x^2*v1 = x,
x*v1^2 = v1, x^4*v1 = x^3 and x*v1^4 = v1^3 (x = k + c0), 800 quadratic GF(2)
equations, which plain XL solves at degree 3.

For each seed the script generates (iv, key), runs the submitted code through
reproduce_lynx160_dump (built from the unmodified Lynxer-160f sources), and
checks that this model reproduces its y, v1 and v2. It then builds the system
from the public data (iv, y, matrices, constants) for the correct guess
u = L0(k), solves it with reproduce_lynx160_xl at degree 3, maps the solution
back to a key, and checks that the key equals the secret one and that the
submitted code gives the same y. A random wrong guess must give an
inconsistent system. Only the choice of the correct guess uses the secret;
the 2^100 enumeration of guesses is not run. The cost per guess is the
solver's count of 64-bit word XORs.

    python3 -B sign-14/reproduce_lynx160_key_recovery.py <dump> <xl> [seeds]

Standard library only. Written by an analysis agent and reviewed for this
repository; reproduce_lynx160_key_recovery.sh builds the two programs.
"""
import math
import os
import random
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
LAM = 160
MODPOLY = (1 << 160) | 0b101101  # X^160 + X^5 + X^3 + X^2 + 1 (fields.c)
GUESSES_LOG2 = 100               # |image of L0 as read|


class Field:
    def __init__(self):
        self.n, self.mask = LAM, (1 << LAM) - 1

    def mul(self, a, b):
        tab = [0] * 16
        for i in range(1, 16):
            t = 0
            for j in range(4):
                if (i >> j) & 1:
                    t ^= a << j
            tab[i] = t
        r, sh = 0, 0
        while b:
            r ^= tab[b & 15] << sh
            b >>= 4
            sh += 4
        return self.reduce(r)

    def reduce(self, r):
        low = MODPOLY ^ (1 << LAM)
        while r.bit_length() > LAM:
            hi, r = r >> LAM, r & self.mask
            t, j, lw = 0, 0, low
            while lw:
                if lw & 1:
                    t ^= hi << j
                lw >>= 1
                j += 1
            r ^= t
        return r

    def pow(self, a, e):
        r = 1
        for bit in bin(e)[2:]:
            r = self.mul(r, r)
            if bit == '1':
                r = self.mul(r, a)
        return r

    def frob(self, a, i):
        for _ in range(i):
            a = self.mul(a, a)
        return a


def matvec(rows, x):
    y = 0
    for i, r in enumerate(rows):
        if bin(r & x).count('1') & 1:
            y |= 1 << i
    return y


def run_dump(dump, iv, key):
    out = subprocess.run([dump, str(LAM), iv.hex(), key.hex()], capture_output=True, text=True, check=True).stdout
    d = {'L0': [], 'L1': [], 'L2': [], 'L3': []}
    for line in out.splitlines():
        p = line.split()
        if p[0] in d:
            v = 0
            for w, h in enumerate(p[2:]):
                v |= int(h, 16) << (64 * w)
            d[p[0]].append(v)
        elif p[0] != 'lambda':
            d[p[0]] = int.from_bytes(bytes.fromhex(p[1]), 'little')
    m = (1 << LAM) - 1
    d['k'], d['v1'], d['v2'] = d['w'] & m, (d['w'] >> LAM) & m, (d['w'] >> (2 * LAM)) & m
    return d


def owf(F, d, k):
    """owf_lynx_160 as implemented: C0 in both S-boxes, no final inverse."""
    c0 = d['C0']
    v1 = F.pow(k ^ c0, (1 << LAM) - 2)
    u = matvec(d['L0'], k)
    v2 = F.pow(u ^ c0, (1 << 67) - 1)
    return F.mul(matvec(d['L1'], v1), matvec(d['L2'], v2)) ^ matvec(d['L3'], k ^ v1 ^ v2), v1, v2, u


def affine_param(rows, nvars, order):
    """Solution set {w0 + sum s_i basis_i} of the GF(2) system rows (bit nvars = constant)."""
    piv = {}
    for r in rows:
        for v in order:
            if (r >> v) & 1 and v in piv:
                r ^= piv[v]
        pv = next((v for v in order if (r >> v) & 1), None)
        if pv is None:
            if (r >> nvars) & 1:
                return None  # inconsistent: this guess is rejected by linear algebra alone
            continue
        for v2 in list(piv):
            if (piv[v2] >> pv) & 1:
                piv[v2] ^= r
        piv[pv] = r
    free = [v for v in range(nvars) if v not in piv]
    w0 = sum(1 << v for v, r in piv.items() if (r >> nvars) & 1)
    basis = []
    for f in free:
        b = 1 << f
        for v, r in piv.items():
            if (r >> f) & 1:
                b |= 1 << v
        basis.append(b)
    return w0, basis, free


class Aff:
    """Field-valued affine map c + sum s_i cols[i] of the unknowns s in F2^n."""
    def __init__(self, c, cols):
        self.c, self.cols = c, list(cols)

    def frob(self, F, e):
        return Aff(F.frob(self.c, e), [F.frob(x, e) for x in self.cols])

    def at(self, s):
        v = self.c
        for i, x in enumerate(self.cols):
            if (s >> i) & 1:
                v ^= x
        return v


def setup(F, d, u):
    """Affine parameterisation of (x = k + c0, v1, k) for the guess u, or None."""
    L0, L1, L2, L3, c0, y = d['L0'], d['L1'], d['L2'], d['L3'], d['C0'], d['y']
    v2 = F.pow(u ^ c0, (1 << 67) - 1)
    a, b = matvec(L2, v2), y ^ matvec(L3, v2)
    nv = 2 * LAM  # k = bits 0..159, v1 = bits 160..319, constant = bit 320
    rows = [L0[i] | (((u >> i) & 1) << nv) for i in range(LAM) if L0[i]]
    colsA = [F.mul(a, matvec(L1, 1 << j)) ^ matvec(L3, 1 << j) for j in range(LAM)]
    colsK = [matvec(L3, 1 << j) for j in range(LAM)]
    for i in range(LAM):
        r = ((b >> i) & 1) << nv
        for j in range(LAM):
            r |= ((colsK[j] >> i) & 1) << j | ((colsA[j] >> i) & 1) << (LAM + j)
        rows.append(r)
    sol = affine_param(rows, nv, list(range(LAM, 2 * LAM)) + list(range(LAM)))
    if sol is None:
        return None
    w0, basis, free = sol
    m = (1 << LAM) - 1
    k_aff = Aff(w0 & m, [bb & m for bb in basis])
    v_aff = Aff(w0 >> LAM, [bb >> LAM for bb in basis])
    return Aff(k_aff.c ^ c0, k_aff.cols), v_aff, k_aff, free


def relation_coeffs(F, prods, aff, n):
    """sum A*B + aff as field coefficients over monomials [1][s_i][s_i s_j, i<j colex]."""
    co = [0] * (1 + n + n * (n - 1) // 2)
    if aff is not None:
        co[0] = aff.c
        for i in range(n):
            co[1 + i] = aff.cols[i]
    for A, B in prods:
        co[0] ^= F.mul(A.c, B.c)
        for i in range(n):
            co[1 + i] ^= F.mul(A.c, B.cols[i]) ^ F.mul(B.c, A.cols[i])
        P = [[F.mul(A.cols[i], B.cols[j]) for j in range(n)] for i in range(n)]
        for i in range(n):
            co[1 + i] ^= P[i][i]
        for j in range(1, n):
            base = 1 + n + j * (j - 1) // 2
            for i in range(j):
                co[base + i] ^= P[i][j] ^ P[j][i]
    return co


def equations(F, x, v, n):
    one = Aff(1, [0] * n)
    x2, x4, v2, v4 = x.frob(F, 1), x.frob(F, 2), v.frob(F, 1), v.frob(F, 2)
    rels = [([(x, v)], one), ([(x2, v)], x), ([(x, v2)], v),
            ([(x4, v), (x, x2)], None), ([(x, v4), (v, v2)], None)]
    eqs = []
    for prods, aff in rels:
        rows = [0] * LAM
        for mon, c in enumerate(relation_coeffs(F, prods, aff, n)):
            t = 0
            while c:
                if c & 1:
                    rows[t] |= 1 << mon
                c >>= 1
                t += 1
        eqs += [e for e in rows if e]
    return eqs


def solve(xl, path, n, eqs, expected):
    nb = (1 + n + n * (n - 1) // 2 + 7) // 8
    with open(path, 'wb') as f:
        f.write(struct.pack('<iii', n, len(eqs), nb))
        for e in eqs:
            f.write(e.to_bytes(nb, 'little'))
        f.write(bytes((expected >> i) & 1 for i in range(n)))
    out = subprocess.run([xl, path, '3'], capture_output=True, text=True, check=True).stdout
    res, sol = None, None
    for line in out.splitlines():
        if line.startswith('RESULT'):
            res = dict(kv.split('=') for kv in line.split()[1:])
        elif line.startswith('SOLUTION'):
            sol = sum(1 << i for i, ch in enumerate(line.split()[1]) if ch == '1')
    return res, sol


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    dump, xl = sys.argv[1], sys.argv[2]
    seeds = [int(s) for s in (sys.argv[3] if len(sys.argv) > 3 else '1').split(',')]
    F = Field()
    tmp = tempfile.mkdtemp()
    costs = []
    for seed in seeds:
        rng = random.Random(seed)
        iv = bytes(rng.getrandbits(8) for _ in range(LAM // 8))
        key = bytes(rng.getrandbits(8) for _ in range(LAM // 8))
        d = run_dump(dump, iv, key)
        y, v1, v2, u = owf(F, d, d['k'])
        if (y, v1, v2) != (d['y'], d['v1'], d['v2']) or d['k'] != int.from_bytes(key, 'little'):
            print(f'FAIL seed {seed}: the model does not reproduce the submitted code')
            return 1
        print(f'seed {seed}: model reproduces y, v1 and v2 of the submitted code; '
              f'L0 as read has {sum(1 for r in d["L0"] if r)} nonzero rows')
        # correct guess
        x, v, k_aff, free = setup(F, d, u)
        n = len(free)
        truth = sum(1 << i for i, f in enumerate(free) if ((d['k'] | (d['v1'] << LAM)) >> f) & 1)
        eqs = equations(F, x, v, n)
        res, sol = solve(xl, os.path.join(tmp, f'c{seed}.bin'), n, eqs, truth)
        if res is None or res['solved'] != '1' or sol is None:
            print(f'FAIL seed {seed}: degree-3 XL did not solve the correct guess ({res})')
            return 1
        k_rec = k_aff.at(sol)
        y_rec = run_dump(dump, iv, k_rec.to_bytes(LAM // 8, 'little'))['y']
        if k_rec != d['k'] or y_rec != d['y']:
            print(f'FAIL seed {seed}: recovered key differs')
            return 1
        costs.append(float(res['log2_wordops']))
        print(f'  correct guess: {len(eqs)} equations in {n} unknowns; degree-3 XL matrix '
              f'{res["rows"]} x {res["cols"]}, rank {res["rank"]}, {res["linear_relations"]} linear relations; '
              f'key recovered and y reproduced by the submitted code; 2^{res["log2_wordops"]} word XORs, '
              f'{res["elim_s"]} s')
        # wrong guess
        while True:
            uw = matvec(d['L0'], rng.getrandbits(LAM))
            if uw != u:
                break
        s = setup(F, d, uw)
        if s is None:
            print('  wrong guess: rejected by the linear part')
        else:
            xw, vw, _, fw = s
            rw, _ = solve(xl, os.path.join(tmp, f'w{seed}.bin'), len(fw), equations(F, xw, vw, len(fw)), 0)
            if rw is None or rw['inconsistent'] != '1':
                print(f'FAIL seed {seed}: a wrong guess was not rejected ({rw})')
                return 1
            print(f'  wrong guess: degree-3 XL system inconsistent (rank {rw["rank"]} of {rw["cols"]} columns)')
    per = max(costs) + 6  # 64-bit word XORs to bit operations
    print(f'cost: 2^{GUESSES_LOG2} guesses x 2^{per:.1f} bit operations per guess = 2^{GUESSES_LOG2 + per:.1f} '
          f'bit operations (estimate; the enumeration is not run); NGCC target 2^128, Lynxer-160 design level 2^160')
    print('ATTACK sign-14-6: one full-size guess of the Lynxer-160 key recovery solves to the secret key, '
          f'and a wrong guess is rejected; total about 2^{GUESSES_LOG2 + per:.1f} bit operations: CONFIRMED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
