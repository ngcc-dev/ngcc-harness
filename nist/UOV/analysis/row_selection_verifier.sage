# Independent verifier toy for UOV-4's partial-tower rank premise.
# usage: sage UOV/analysis/row_selection_verifier.sage MODE e seed trials ARGS
#   MODE planted v a m          : generic planted UOV-type forms (oil dim a in n=v+a)
#   MODE inter   V O a mm       : UOV(V,O) (n=V+O even), Beullens intersection with all 3O-2 forms
#                                 (mm=0) or mm random combinations of them, projected to a random
#                                 (v'+a)-dim subspace (a <= o' = 2O-V)
#   MODE generic n a m          : no oil; random alternating forms, tests partial-tower series (J=0,1,2)
# Reports: rank(L0), rank(L0+L1), [rank(L0+L1+L2)], predictions, kernel==oil Plucker,
#          and sparse-preferring SQUARE selections: (N-H0-s) plain + (H0+s) div2 rows.
import sys, itertools, random, time
from math import comb
MODE = sys.argv[1]; e, seed, trials = [int(x) for x in sys.argv[2:5]]; P = [int(x) for x in sys.argv[5:9]]
set_random_seed(seed); random.seed(seed)
K = GF(2**e, 'z')

def series(n, m, J, D):
    A = [comb(n, i) for i in range(D+1)]
    def mul(A, B):
        C = [0]*(D+1)
        for i in range(D+1):
            if A[i]:
                for j in range(D+1-i): C[i+j] += A[i]*B[j]
        return C
    if J is None:
        B = [0]*(D+1)
        for i in range(D//2+1): B[2*i] = (-1)**i*comb(m, i)
        return mul(A, B)
    for j in range(J+1):
        s = 2**(j+1); B = [0]*(D+1)
        for k in range(D//s+1): B[s*k] = (-1)**k*comb(m+k-1, k)
        A = mul(A, B)
    return A
def tdim(c, a):
    for d in range(a+1):
        if c[d] <= 0: return 0
    return c[a]

def rand_alt(nn, oil_from=None):
    W = matrix(K, nn, nn)
    for p in range(nn):
        for q in range(p+1, nn):
            if oil_from is not None and p >= oil_from and q >= oil_from: continue
            c = K.random_element(); W[p, q] = c; W[q, p] = c
    return W
def rand_inv(nn):
    while True:
        T = random_matrix(K, nn, nn)
        if T.is_invertible(): return T

def inst_planted(v, a, m):
    n = v+a; T = rand_inv(n)
    forms = [T.transpose()*rand_alt(n, v)*T for _ in range(m)]
    B = T.inverse()[:, v:]          # n x a oil basis (columns)
    return n, forms, B

def inst_inter(V, O, a, mm):
    n = V+O; assert n % 2 == 0
    T = rand_inv(n)
    G = [T.transpose()*rand_alt(n, V)*T for _ in range(O)]
    Ob = T.inverse()[:, V:]
    def rc():
        while True:
            X = sum(K.random_element()*g for g in G)
            if X.is_invertible(): return X
    b1 = rc(); b2 = rc()
    Mi = b2.inverse()*b1            # M^{-1} with M = b1^{-1} b2
    F = []
    for g in G:
        F += [g, Mi.transpose()*g*Mi, g*Mi + (g*Mi).transpose()]
    # independent span
    vecs = matrix(K, [f.list() for f in F]); rk = vecs.rank()
    # oil' = O cap M O = {x in O : Mi x in O}
    # x = Ob y ; Mi Ob y in colspace(Ob)
    Aug = Ob.augment(Mi*Ob)  # want y,z: Mi Ob y = Ob z  -> [Mi Ob | Ob] [y;-z]=0
    Kr = (Mi*Ob).augment(Ob).right_kernel_matrix()
    Wp = (Ob*Kr[:, :O].transpose())
    Wp = Wp.column_space().basis_matrix().transpose()
    op = Wp.ncols(); vp = n - op
    assert op == 2*O - V, (op, 2*O-V)
    if mm > 0:
        F = [sum(K.random_element()*f for f in F) for _ in range(mm)]
    # project to random (vp+a)-dim subspace H: x = R y, y supported on first vp+a coords
    R = rand_inv(n); h = vp + a
    S = R[:, :h]
    forms = [S.transpose()*f*S for f in F]
    # oil cap H: Wp z = S y  ->
    Kr2 = Wp.augment(S).right_kernel_matrix()
    Y = Kr2[:, op:].transpose()   # columns y (in H coords)
    Y = Y.column_space().basis_matrix().transpose()
    assert Y.ncols() == a, Y.ncols()
    return h, forms, Y, rk, (vp, op)

def tower_coeff(W, S):
    # divided power omega^(|S|/2) coefficient on even set S: sum over perfect matchings (char 2, no signs)
    if not S: return K(1)
    p = S[0]; tot = K(0)
    for i in range(1, len(S)):
        c = W[p, S[i]]
        if c: tot += c*tower_coeff(W, S[1:i] + S[i+1:])
    return tot

def build(n, a, forms, lev):
    subsets = list(itertools.combinations(range(n), a)); idx = {s: i for i, s in enumerate(subsets)}
    rows = []
    d = 2**(lev+1)
    for W in forms:
        if d == 2:
            tw = {(p, q): W[p, q] for p, q in itertools.combinations(range(n), 2) if W[p, q]}
        else:
            tw = {}
            for S in itertools.combinations(range(n), d):
                c = tower_coeff(W, list(S))
                if c: tw[S] = c
        for Kset in itertools.combinations(range(n), a-d):
            sK = set(Kset); row = []
            for S, c in tw.items():
                if sK.isdisjoint(S):
                    row.append((idx[tuple(sorted(Kset+S))], c))
            rows.append(row)
    return subsets, rows

def tomat(rows, N):
    M = matrix(K, len(rows), N)
    for i, r in enumerate(rows):
        for j, c in r: M[i, j] = c
    return M

for t in range(trials):
    t0 = time.time()
    extra = ''
    if MODE == 'planted':
        v, a, m = P[:3]; n, forms, B = inst_planted(v, a, m)
    elif MODE == 'inter':
        V, O, a, mm = P; n, forms, B, rk, (vp, op) = inst_inter(V, O, a, mm); v = n - a; m = len(forms)
        extra = f" [UOV({V},{O}) -> v'={vp} o'={op}; {rk} independent of {3*O} intersection forms; using {m}; projected a={a}]"
    elif MODE == 'generic':
        n, a, m = P[:3]; v = n - a; forms = [rand_alt(n) for _ in range(m)]; B = None
    N = comb(n, a)
    H = {J: tdim(series(n, m, J, a), a) for J in (0, 1, 2)}
    print(f"[{MODE}] GF(2^{e}) n={n} v={v} a={a} m={m} N={N} pred H0={H[0]} H1={H[1]} H2={H[2]}{extra}")
    subsets, L0 = build(n, a, forms, 0)
    _, L1 = build(n, a, forms, 1) if a >= 4 else (None, [])
    L2 = build(n, a, forms, 2)[1] if a >= 8 else []
    w0 = sum(len(r) for r in L0)/len(L0); w1 = (sum(len(r) for r in L1)/len(L1)) if L1 else 0
    M0 = tomat(L0, N); r0 = M0.rank()
    M01 = M0.stack(tomat(L1, N)); r01 = M01.rank()
    planted = B is not None
    exp0 = N - max(H[0], 1 if planted else 0); exp1 = N - max(H[1], 1 if planted else 0)
    msg = f"  rank L0 {r0} (pred {exp0}) | rank L0+L1 {r01} (pred {exp1}) | weights {w0:.1f}/{comb(v+2,2)} {w1:.1f}/{comb(v+4,4)} | rows L0 {len(L0)} L1 {len(L1)}"
    if L2:
        M012 = M01.stack(tomat(L2, N)); r012 = M012.rank(); exp2 = N - max(H[2], 1 if B is not None else 0)
        msg += f" | rank L0+L1+L2 {r012} (pred {exp2})"
    print(msg)
    if planted:
        nu = vector(K, [B.matrix_from_rows(list(S)).det() for S in subsets])
        ok = (M01*nu == 0)
        kd = N - r01
        print(f"  oil Plucker annihilated: {ok}; corank(L0+L1) = {kd}")
        if H[0] > 1 and H[1] <= 1:
            for s in (0, 1, 2, 4):
                succ = 0; kerok = 0; R = 6
                for _ in range(R):
                    S0 = random.sample(L0, N - H[0] - s); S1 = random.sample(L1, H[0] + s)
                    Ms = tomat(S0 + S1, N)
                    rs = Ms.rank()
                    if rs == N - 1:
                        succ += 1
                        kv = Ms.right_kernel().basis_matrix()
                        kerok += int(kv.nrows() == 1 and kv.stack(nu.row()).rank() == 1)
                print(f"  square selection {N-H[0]-s} plain + {H[0]+s} div2: rank N-1 in {succ}/{R}, kernel==oil in {kerok}/{R}")
    print(f"  time {time.time()-t0:.1f}s"); sys.stdout.flush()
