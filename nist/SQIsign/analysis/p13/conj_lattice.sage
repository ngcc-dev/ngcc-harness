# conj_lattice.sage -- distribution of degrees of isogenies E -> E^(p)
# for (approximately) uniformly random supersingular E, via the Deuring
# correspondence: Hom(E, E^(p)) ~ (P, Nrd/p), P = two-sided ideal of norm p
# in the maximal order O = End(E).
#
# usage: sage conj_lattice.sage <tag> <p> <nsamples> <Kmax> <seed> <outfile>
#   Kmax: enumerate all vectors of degree <= Kmax * (p/2)^(1/3)
# Output (JSON lines): per sample: sorted list of primitive degrees (one per +-pair),
# the rank of the span of those vectors, and the 4 reduced Gram diagonal degrees.
import sys, json, random
from sage.all import *

tag, p, ns, Kmax, seed, outf = sys.argv[1], Integer(sys.argv[2]), int(sys.argv[3]), Integer(sys.argv[4]), int(sys.argv[5]), sys.argv[6]
assert p.is_prime() and p % 4 == 3
random.seed(seed); set_random_seed(seed)
Q = QuaternionAlgebra(QQ, -1, -p)
i, j, k = Q.gens()
O0 = Q.quaternion_order([Q(1), i, (i + j) / 2, (1 + k) / 2])

def random_order():
    # random left O0-ideal of large prime norm N ~ p^2 -> right order is ~uniform
    N = next_prime(ZZ.random_element(p**2, 2 * p**2))
    F = GF(N)
    while True:
        b, c, d = [F.random_element() for _ in range(3)]
        t = -(b**2 + p * (c**2 + d**2))
        if t.is_square():
            a = t.sqrt()
            break
    x = Q([ZZ(a), ZZ(b), ZZ(c), ZZ(d)])
    I = O0.left_ideal([g * x for g in O0.basis()] + [g * N for g in O0.basis()])
    assert I.norm() == N, (I.norm(), N)
    return I.right_order()

def conj_lattice(O):
    B = O.basis()
    T = matrix(ZZ, 4, 4, lambda r, s: (B[r] * B[s].conjugate()).reduced_trace())
    Kp = (T.change_ring(GF(p))).right_kernel().basis_matrix()
    assert Kp.nrows() == 2
    gens = [vector(ZZ, [ZZ(v) for v in row]) for row in Kp.rows()] + [p * vector(ZZ, e) for e in identity_matrix(ZZ, 4).rows()]
    C = matrix(ZZ, gens).echelon_form()[:4]
    assert abs(C.det()) == p**2
    G = C * T * C.transpose()          # Tr(x conj(y)) on P
    assert all(g % p == 0 for g in G.list())
    G = G / p                          # 2 * (Nrd/p) form, integral
    G = G.change_ring(ZZ)
    U = G.LLL_gram()                   # columns: reduced basis
    Gr = U.transpose() * G * U
    return Gr

def enumerate_short(Gr, bound2):
    # all nonzero x with x^T Gr x <= bound2, one per +-pair (Fincke-Pohst, exact QQ)
    n = 4
    Gq = Gr.change_ring(QQ)
    # Cholesky-like decomposition: q(x) = sum_i Qd[i] (x_i + sum_{j>i} mu[i][j] x_j)^2
    A = copy(Gq)
    Qd = [0] * n; mu = [[0] * n for _ in range(n)]
    for ii in range(n):
        Qd[ii] = A[ii, ii]
        for jj in range(ii + 1, n):
            mu[ii][jj] = A[ii, jj] / A[ii, ii]
        for r in range(ii + 1, n):
            for s in range(r, n):
                A[r, s] = A[r, s] - mu[ii][r] * mu[ii][s] * Qd[ii]
                A[s, r] = A[r, s]
    out = []
    x = [0] * n
    def rec(idx, rem):
        if idx < 0:
            out.append(tuple(x)); return
        c = -sum(mu[idx][jj] * x[jj] for jj in range(idx + 1, n))
        r2 = rem / Qd[idx]
        if r2 < 0:
            return
        # x_idx in [c - sqrt(r2), c + sqrt(r2)]
        rr = RealField(200)(r2).sqrt()
        lo = ceil(RealField(200)(c) - rr) - 1
        hi = floor(RealField(200)(c) + rr) + 1
        for v in range(lo, hi + 1):
            t = Qd[idx] * (v - c)**2
            if t <= rem:
                x[idx] = v
                rec(idx - 1, rem - t)
        x[idx] = 0
    rec(n - 1, QQ(bound2))
    res = []
    for v in out:
        if all(t == 0 for t in v):
            continue
        # keep one of +-v: first nonzero coordinate positive
        fz = next(t for t in v if t != 0)
        if fz < 0:
            continue
        vv = vector(ZZ, v)
        res.append(vv)
    return res

D0 = (p / 2) ** (1 / 3)
with open(outf, "w") as fh:
    for sidx in range(ns):
        O = random_order()
        Gr = conj_lattice(O)
        diag = [ZZ(Gr[r, r]) // 2 for r in range(4)]
        Dmax = floor(Kmax * RealField(400)(p / 2) ** (RealField(400)(1) / 3))
        vecs = enumerate_short(Gr, 2 * Dmax)
        degs = []
        prim = []
        for v in vecs:
            dg = ZZ(v * Gr * v) // 2
            g = gcd(list(v))
            if g == 1:
                degs.append(int(dg)); prim.append(v)
        degs.sort()
        rk = matrix(ZZ, prim).rank() if prim else 0
        # determinant of the degree form on the saturated span of the short vectors
        if rk == 3:
            M = matrix(ZZ, prim)
            S = M.row_space().saturation() if hasattr(M.row_space(),'saturation') else None
            Bs = matrix(ZZ, (M.row_module()).basis())
            # saturate: Z^4 intersect QQ-span
            Bsat = (Bs.change_ring(QQ).row_space().intersection((ZZ**4).change_ring(QQ)) if False else None)
            Ksp = M.change_ring(QQ).right_kernel().basis_matrix()
            Lsat = (Ksp.denominator()*Ksp).change_ring(ZZ).right_kernel_matrix()
            det3 = (Lsat * Gr * Lsat.transpose()).det() / 8
        else:
            det3 = 0
        fh.write(json.dumps({"tag": tag, "s": sidx, "Dmax": int(Dmax), "degs": degs, "rank": int(rk), "diag": [int(d) for d in diag], "det3": str(det3)}) + "\n")
        fh.flush()
