#!/usr/bin/env python3
"""Small QR-UOV-structured Hashimoto(k=0) -> square residual experiment.

This uses q=31, ell=3, f=X^3-X-1, which are the field and polynomial of the
submitted q=31, ell=3 family.  Toy dimensions are V=3, M=2 (n=15, m=6).
The public map follows Algorithm 1 of the Round-3 specification exactly at the
algebraic level: P1 and P2 are uniform over GF(q^ell), S' is uniform, P3 is
derived by line 5, and each block is expanded as W Phi_f(g).
"""

from __future__ import annotations
import argparse
import itertools
import random

Q = 31
ELL = 3
V = 3
M = 2
N = V + M
NVARS = ELL * N
NEQS = ELL * M
B = 1


def invp(x):
    return pow(x % Q, Q - 2, Q)


# Elements of GF(31)[X]/(X^3-X-1), as tuples low coefficient first.
def ka(a=0, b=0, c=0):
    return (a % Q, b % Q, c % Q)


KZERO = ka()
KONE = ka(1)


def kadd(a, b):
    return tuple((a[i] + b[i]) % Q for i in range(ELL))


def kneg(a):
    return tuple((-x) % Q for x in a)


def kmul(a, b):
    # X^3 = X + 1.  Reduce degrees 4 then 3.
    c = [0] * 5
    for i in range(3):
        for j in range(3):
            c[i+j] = (c[i+j] + a[i] * b[j]) % Q
    for d in (4, 3):
        c[d-3] = (c[d-3] + c[d]) % Q
        c[d-2] = (c[d-2] + c[d]) % Q
    return tuple(c[:3])


def krand(rng):
    return tuple(rng.randrange(Q) for _ in range(ELL))


def kmat_zero(r, c):
    return [[KZERO for _ in range(c)] for _ in range(r)]


def kmat_transpose(A):
    return [list(x) for x in zip(*A)]


def kmat_mul(A, B):
    out = kmat_zero(len(A), len(B[0]))
    for i in range(len(A)):
        for k in range(len(B)):
            if A[i][k] == KZERO:
                continue
            for j in range(len(B[0])):
                out[i][j] = kadd(out[i][j], kmul(A[i][k], B[k][j]))
    return out


def kmat_add(A, B):
    return [[kadd(A[i][j], B[i][j]) for j in range(len(A[0]))]
            for i in range(len(A))]


def kmat_neg(A):
    return [[kneg(x) for x in row] for row in A]


def phi(g):
    # Column j contains coefficients of X^j*g mod f, as in specification (4).
    cols = []
    xj = KONE
    x = ka(0, 1)
    for _ in range(ELL):
        cols.append(kmul(g, xj))
        xj = kmul(xj, x)
    return [[cols[j][i] for j in range(ELL)] for i in range(ELL)]


def matmul(A, B):
    return [[sum(A[i][k] * B[k][j] for k in range(len(B))) % Q
             for j in range(len(B[0]))] for i in range(len(A))]


def transpose(A):
    return [list(x) for x in zip(*A)]


# W_ij = constant coefficient of X^(i+j) mod f.
_XP = [KONE]
for _ in range(1, 2 * ELL):
    _XP.append(kmul(_XP[-1], ka(0, 1)))
W = [[_XP[i+j][0] for j in range(ELL)] for i in range(ELL)]


def expand_block_matrix(A):
    rows, cols = len(A), len(A[0])
    out = [[0] * (cols * ELL) for _ in range(rows * ELL)]
    for i in range(rows):
        for j in range(cols):
            block = matmul(W, phi(A[i][j]))
            for a in range(ELL):
                for b in range(ELL):
                    out[i*ELL+a][j*ELL+b] = block[a][b]
    return out


def qform(x, A, y=None):
    if y is None:
        y = x
    return sum(x[i] * A[i][j] * y[j]
               for i in range(len(x)) for j in range(len(y))) % Q


def vmat(v, A):
    return [sum(v[i] * A[i][j] for i in range(len(v))) % Q
            for j in range(len(A[0]))]


def rank(A):
    A = [[x % Q for x in row] for row in A]
    r = 0
    for c in range(len(A[0]) if A else 0):
        p = next((i for i in range(r, len(A)) if A[i][c]), None)
        if p is None:
            continue
        A[r], A[p] = A[p], A[r]
        z = invp(A[r][c])
        A[r] = [(x * z) % Q for x in A[r]]
        for i in range(len(A)):
            if i != r and A[i][c]:
                z = A[i][c]
                A[i] = [(A[i][j] - z * A[r][j]) % Q for j in range(len(A[0]))]
        r += 1
        if r == len(A):
            break
    return r


def nullspace(A, ncols):
    A = [[x % Q for x in row] for row in A]
    r = 0
    pivots = []
    for c in range(ncols):
        p = next((i for i in range(r, len(A)) if A[i][c]), None)
        if p is None:
            continue
        A[r], A[p] = A[p], A[r]
        z = invp(A[r][c])
        A[r] = [(x * z) % Q for x in A[r]]
        for i in range(len(A)):
            if i != r and A[i][c]:
                z = A[i][c]
                A[i] = [(A[i][j] - z * A[r][j]) % Q for j in range(ncols)]
        pivots.append(c)
        r += 1
        if r == len(A):
            break
    free = [c for c in range(ncols) if c not in pivots]
    basis = []
    for f in free:
        x = [0] * ncols
        x[f] = 1
        for i, p in enumerate(pivots):
            x[p] = (-A[i][f]) % Q
        basis.append(x)
    return basis


def lincomb(coeff, basis):
    return [sum(coeff[i] * basis[i][j] for i in range(len(basis))) % Q
            for j in range(len(basis[0]))]


def public_map(rng):
    Sd = [[krand(rng) for _ in range(M)] for _ in range(V)]
    Ps = []
    for _ in range(NEQS):
        P1 = kmat_zero(V, V)
        for i in range(V):
            for j in range(i, V):
                P1[i][j] = P1[j][i] = krand(rng)
        P2 = [[krand(rng) for _ in range(M)] for _ in range(V)]
        t1 = kmat_mul(kmat_mul(kmat_transpose(Sd), P1), Sd)
        t2 = kmat_mul(kmat_transpose(P2), Sd)
        t3 = kmat_mul(kmat_transpose(Sd), P2)
        P3 = kmat_add(kmat_neg(t1), kmat_add(t2, t3))
        P = kmat_zero(N, N)
        for i in range(V):
            for j in range(V): P[i][j] = P1[i][j]
            for j in range(M):
                P[i][V+j] = P2[i][j]
                P[V+j][i] = P2[i][j]
        for i in range(M):
            for j in range(M): P[V+i][V+j] = P3[i][j]
        Pb = expand_block_matrix(P)
        assert Pb == transpose(Pb)
        Ps.append(Pb)
    # Basis of hidden oil space (-S'o,o), expanded over the power basis.
    oils = []
    for j in range(M):
        for a in range(ELL):
            oK = [KZERO] * N
            unit = [0] * ELL; unit[a] = 1
            oK[V+j] = tuple(unit)
            for i in range(V):
                oK[i] = kneg(kmul(Sd[i][j], oK[V+j]))
            oils.append([c for x in oK for c in x])
    assert all(qform(o, P, oo) == 0 for P in Ps for o in oils for oo in oils)
    return Ps, oils


def find_isotropic(Qs, lin, rng, trials=250_000):
    K = nullspace(lin, NVARS)
    if len(K) <= len(Qs):
        raise RuntimeError("isotropic search dimension too small")
    for _ in range(trials):
        c = [rng.randrange(Q) for _ in K]
        if not any(c):
            continue
        x = lincomb(c, K)
        if all(qform(x, A) == 0 for A in Qs):
            return x
    raise RuntimeError("no isotropic vector found")


def centrifuge(Ps, rng, max_restarts=100):
    PL = Ps[:B+1]
    for restart in range(max_restarts):
        O0 = []
        try:
            for _ in range(NEQS-B):
                lin = [vmat(o, P) for o in O0 for P in PL] + list(O0)
                O0.append(find_isotropic(PL, lin, rng))
            if rank(O0) == len(O0):
                return O0, restart
        except RuntimeError:
            pass
    raise RuntimeError("centrifugation failed")


def extraction_space(Ps, O0, rng):
    lin = [vmat(o, P) for o in O0 for P in Ps[:B]]
    K = nullspace(lin, NVARS)
    candidates = list(K)
    candidates += [lincomb([rng.randrange(Q) for _ in K], K) for _ in range(100)]
    for x in candidates:
        if (rank(O0 + [x]) == len(O0) + 1 and
                qform(x, Ps[0]) != 0):
            O = O0 + [x]
            assert rank(O) == NEQS
            return O
    raise RuntimeError("no extraction complement")


def restrict(Ps, O):
    return [[[qform(O[a], P, O[b]) for b in range(NEQS)]
             for a in range(NEQS)] for P in Ps]


def monomials(n, d):
    if n == 1:
        return [(d,)]
    return [(i,) + r for i in range(d+1) for r in monomials(n-1, d-i)]


def addexp(a, b):
    return tuple(x+y for x,y in zip(a,b))


def hilbert_top(quads, n, maxd=5):
    # quad dicts keyed by exponent tuples of total degree 2.
    out = []
    for d in range(maxd+1):
        md = monomials(n, d); pos = {e:i for i,e in enumerate(md)}
        rows = []
        if d >= 2:
            for mm in monomials(n, d-2):
                for f in quads:
                    row = [0] * len(md)
                    for e,c in f.items(): row[pos[addexp(mm,e)]] = c
                    rows.append(row)
        out.append(len(md)-rank(rows))
    return out


def solve_one(Pt, target):
    ny = NEQS-B
    # First equation is a*z^2=t0.
    a = Pt[0][-1][-1]
    if a == 0:
        return None
    zs = [z for z in range(Q) if a*z*z % Q == target[0]]
    for z in zs:
        # Equation B has yy=0 and is linear in y after fixing z.
        A = Pt[B]
        coeff = [(2 * A[i][-1] * z) % Q for i in range(ny)]
        const = (A[-1][-1] * z*z - target[B]) % Q
        piv = next((i for i,c in enumerate(coeff) if c), None)
        if piv is None:
            continue
        free = [i for i in range(ny) if i != piv]
        invc = invp(coeff[piv])
        # Substitute y_piv = alpha0 + sum alpha_j*u_j.
        alpha0 = (-const * invc) % Q
        alpha = [(-coeff[j] * invc) % Q for j in free]

        # Return residual evaluator and its homogeneous quadratic top.
        def y_from(u):
            y = [0] * ny
            for j,v in zip(free,u): y[j] = v
            y[piv] = (alpha0 + sum(alpha[k]*u[k] for k in range(len(u)))) % Q
            return y

        residual = []
        tops = []
        nr = len(free)
        for ei in range(B+1, NEQS):
            R = Pt[ei]
            def ev(u, R=R, ei=ei):
                x = y_from(u)+[z]
                return (qform(x,R)-target[ei]) % Q
            residual.append(ev)
            # Obtain quadratic coefficients from the linear part of y(u).
            L = [[0]*nr for _ in range(ny)]
            for k,j in enumerate(free): L[j][k] = 1
            for k in range(nr): L[piv][k] = alpha[k]
            top = {}
            for i in range(nr):
                for j in range(i,nr):
                    c = 0
                    for r in range(ny):
                        for s in range(ny):
                            c += L[r][i]*R[r][s]*L[s][j]
                    if i != j:
                        for r in range(ny):
                            for s in range(ny):
                                c += L[r][j]*R[r][s]*L[s][i]
                    c %= Q
                    if c:
                        e = [0]*nr; e[i]+=1; e[j]+=1
                        top[tuple(e)] = c
            tops.append(top)

        # Brute force the square residual (4 variables at this toy size).
        for u in itertools.product(range(Q), repeat=len(free)):
            if all(f(u) == 0 for f in residual):
                return y_from(u)+[z], tops
    return None


def run(seed):
    rng = random.Random(seed)
    Ps, oils = public_map(rng)
    O0, restarts = centrifuge(Ps, rng)
    O = extraction_space(Ps, O0, rng)
    Pt = restrict(Ps, O)
    ny = NEQS-B
    struct1 = all(Pt[i][a][c] == 0 for i in range(B)
                  for a in range(ny) for c in range(NEQS))
    struct2 = all(Pt[B][a][c] == 0 for a in range(ny) for c in range(ny))
    for attempt in range(100):
        target = [rng.randrange(Q) for _ in range(NEQS)]
        sol = solve_one(Pt, target)
        if sol is None:
            continue
        u, tops = sol
        s = [sum(O[j][i]*u[j] for j in range(NEQS)) % Q for i in range(NVARS)]
        good = all(qform(s, Ps[i]) == target[i] for i in range(NEQS))
        hs = hilbert_top(tops, NEQS-B-1, 5)
        return {"seed": seed, "restarts": restarts, "target_attempt": attempt+1,
                "residual": NEQS-B-1, "structure": struct1 and struct2,
                "hilbert": hs, "semiregular": hs == [1,4,6,4,1,0],
                "forgery": good}
    raise RuntimeError("no solvable target")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trials", type=int, default=3)
    ap.add_argument("--seed", type=int, default=20261018)
    args = ap.parse_args()
    if args.trials < 1:
        ap.error("--trials must be positive")
    print("QR-UOV structured toy: q=31 ell=3 f=X^3-X-1 V=3 M=2 n=15 m=6 b=1")
    print("Hashimoto conditions:",
          NVARS-(NEQS-B-1)*(B+2)>B+1,
          NVARS-NEQS+1>B*(NEQS-B))
    print("W=", W, "rank", rank(W))
    assert rank(W) == ELL
    for g in (KONE, ka(0,1), ka(7,11,13)):
        A = matmul(W, phi(g)); assert A == transpose(A)
    for i in range(args.trials):
        result = run(args.seed+i)
        assert result["residual"] == NEQS-B-1
        assert result["structure"], result
        assert result["hilbert"] == [1, 4, 6, 4, 1, 0], result
        assert result["semiregular"], result
        assert result["forgery"], result
        print(result)


if __name__ == "__main__":
    main()
