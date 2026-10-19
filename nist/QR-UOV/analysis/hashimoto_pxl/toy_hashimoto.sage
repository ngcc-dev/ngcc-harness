# Toy validation of the "Hashimoto (k = 0) reduction, then PXL/XL on the square residual" forgery.
# Follows Ostuzzi 2026/1122 Sect. 3 (Algorithm 1 Centrifugation, Algorithm 2 Extraction) with k = 0.
# Random homogeneous quadratic map P: F_q^n -> F_q^m (a QR-UOV public map is homogeneous and looks
# random for forgery purposes).  Checks:
#   (1) the residual system has m-b-1 equations in m-b-1 variables (TW would leave m-floor(n/m)+1);
#   (2) its homogeneous top part is semi-regular: Hilbert series == (1+z)^(m-b-1) truncated;
#   (3) a solution lifts to s with P(s) = t (end-to-end forgery of the toy map).
import sys, time
args = sys.argv[1:]
q = int(args[0]) if args else 31
cases = [(45,12,3),(60,12,4),(100,10,8),(84,14,4)] if len(args) < 2 else [tuple(map(int,a.split(','))) for a in args[1:]]
F = GF(q)

def rand_sym(n):
    A = random_matrix(F, n, n); return A + A.transpose()

def find_isotropic(Qs, lin, n, tries=30):
    """non-zero x with x^T Q x = 0 for Q in Qs and l.x = 0 for l in lin.  Uses the linear
    constraints to parametrise, fixes all but len(Qs)+1 free coords at random, solves the rest."""
    Lm = matrix(F, lin) if lin else matrix(F, 0, n)
    K = Lm.right_kernel().basis_matrix()          # rows span solution space of linear part
    d = K.nrows(); e = len(Qs)
    assert d > e, "not enough variables (%d) for %d quadrics" % (d, e)
    R = PolynomialRing(F, 'u', e + 1)
    for _ in range(tries):
        fixed = [F.random_element() for _ in range(d - e - 1)]
        if fixed and all(c == 0 for c in fixed): continue
        coeffs = list(R.gens()) + [R(c) for c in fixed]
        x = vector(R, coeffs) * K.change_ring(R)
        eqs = [x * Q.change_ring(R) * x for Q in Qs]
        # dehomogenise: u_e = 1 (one extra variable gives projective freedom)
        rr = [F.random_element() for _ in range(e + 1)]
        I = R.ideal(eqs + [sum(c * g for c, g in zip(rr, R.gens())) - 1])
        V = I.variety()
        if V:
            sol = V[0]
            xv = vector(F, [sol[g] for g in R.gens()] + fixed) * K
            if xv != 0:
                return xv
    raise RuntimeError("no isotropic vector found")


def rational_points(polys, Rr):
    """F-rational zeros of a zero-dimensional system via lex GB (stdfglm) and back-substitution
    (Sage's variety() is very slow over GF(31) here)."""
    Rl = PolynomialRing(Rr.base_ring(), Rr.variable_names(), order='lex')
    G = Rl.ideal([Rl(p) for p in polys]).groebner_basis('libsingular:stdfglm')
    gens = Rl.gens(); nv = len(gens)
    sols = [dict()]
    for j in reversed(range(nv)):
        new = []
        for sd in sols:
            cands = None
            for g in G:
                h = g.subs(sd)
                if h == 0 or not set(h.variables()).issubset({gens[j]}):
                    continue
                if h.is_constant():
                    cands = []; break
                rts = set(r for r, _ in h.univariate_polynomial().roots())
                cands = rts if cands is None else cands & rts
            if cands is None:
                continue
            for r in cands:
                d2 = dict(sd); d2[gens[j]] = r
                new.append(d2)
        sols = new
    return [{Rr(str(k)): v for k, v in sd.items()} for sd in sols]

for (n, m, b) in cases:
    set_random_seed(1000 + n + m + b)
    c1 = n - (m - b - 1) * (b + 2) > b + 1
    c2 = n - m + 1 > b * (m - b)
    tw = m - (n // m) + 1
    print("q=%d n=%d m=%d b=%d k=0: Ostuzzi eq.(7) conditions %s %s; residual square size %d (TW: %d)" % (q, n, m, b, c1, c2, m - b - 1, tw))
    if not (c1 and c2):
        continue
    t0 = time.time()
    Ps = [rand_sym(n) for _ in range(m)]          # x^T P_i x ; char odd so polar form = x^T P_i y
    t = vector(F, [F.random_element() for _ in range(m)])
    PL = Ps[:b + 1]; PB = Ps[:b]
    # Centrifugation: O0 (dim m-b) isotropic for P_L and pairwise orthogonal
    restarts = 0
    while True:   # Ostuzzi Remark 2: on a local obstruction, rerandomise from o_1
        O0 = []
        try:
            for i in range(m - b):
                lin = [o * P for o in O0 for P in PL] + [o for o in O0]   # polar orthogonality; rows "o" = work in F^n/<O0>
                x = find_isotropic(PL, [list(r) for r in lin], n)
                O0.append(x)
            print("   centrifugation: O0 of dim %d found" % len(O0), flush=True) if len(O0) == m - b else None
            break
        except RuntimeError:
            restarts += 1
            if restarts > 50: raise
    # Ot (dim b): P_B-orthogonal to O0, independent
    lin = [list(o * P) for o in O0 for P in PB]
    K = matrix(F, lin).right_kernel().basis_matrix()
    Ot = []
    Bcur = matrix(F, O0)
    for r in K.rows():
        if (Bcur.stack(r)).rank() > Bcur.rank():
            Ot.append(r); Bcur = Bcur.stack(r)
        if len(Ot) == b: break
    assert len(Ot) == b
    Obasis = O0 + Ot                               # y-part (m-b vars) then z-part (b vars)
    S = matrix(F, Obasis).transpose()              # n x m
    Pt = [S.transpose() * P * S for P in Ps]       # m x m
    # structure checks
    ok_red = all(Pt[i][a, c] == 0 for i in range(b) for a in range(m - b) for c in range(m))
    ok_ov = all(Pt[b][a, c] == 0 for a in range(m - b) for c in range(m - b))
    tc = time.time() - t0
    # Extraction (k = 0): square step MQ(b,b) in z, then eq b+1 linear in y, then square residual
    for attempt in range(20):   # centrifugation is target-independent; new salt = new t
        if attempt > 0: t = vector(F, [F.random_element() for _ in range(m)])
        R = PolynomialRing(F, ['y%d' % i for i in range(m - b)] + ['z%d' % i for i in range(b)])
        X = vector(R, R.gens()); ys = R.gens()[:m - b]; zs = R.gens()[m - b:]
        eqs = [X * Pt[i].change_ring(R) * X - t[i] for i in range(m)]
        assert all(set(e.variables()).issubset(set(zs)) for e in eqs[:b])
        Rz = PolynomialRing(F, ['z%d' % i for i in range(b)])
        sq = [Rz(e.subs({y: 0 for y in ys})) for e in eqs[:b]]
        Vz = Rz.ideal(sq).variety()
        done = False
        for zsol in Vz:
            zval = {zs[i]: zsol[Rz.gen(i)] for i in range(b)}
            e1 = eqs[b].subs(zval)                     # linear in y
            assert e1.degree() <= 1
            # eliminate one y variable via e1
            piv = next(y for y in ys if e1.coefficient({y: 1}) != 0)
            sub = {piv: piv - e1 / e1.coefficient({piv: 1})}
            rest = [e.subs(zval).subs(sub) for e in eqs[b + 1:]]
            free = [y for y in ys if y != piv]
            Rr = PolynomialRing(F, [str(y) for y in free], order='degrevlex')
            rest = [Rr(r) for r in rest]
            nres = len(rest)
            # semi-regularity of top-degree part: Hilbert series vs (1+z)^nres
            top = [r.homogeneous_components()[2] for r in rest]
            H = Rr.ideal(top).hilbert_series()
            PS = PowerSeriesRing(ZZ, 'w', default_prec=nres + 2)
            hs = PS(H.numerator()) / PS(H.denominator())
            pred = PS((1 + PS.gen()) ^ nres)
            semireg = all(hs[d] == pred[d] for d in range(nres + 1))
            t1 = time.time()
            GB = Rr.ideal(rest).groebner_basis()
            maxdeg = max(g.degree() for g in GB)
            V = rational_points(rest, Rr)
            for vs in V:
                yval = {Rr.gen(i): vs[Rr.gen(i)] for i in range(nres)}
                full = {}
                for y in free: full[y] = yval[Rr(str(y))]
                full[piv] = (piv - e1 / e1.coefficient({piv: 1})).subs(zval).subs({y: full[y] for y in free}) if False else None
                # recompute piv from e1 = 0
                e1v = e1.subs({y: full[y] for y in free})
                full[piv] = -e1v.constant_coefficient() / e1v.coefficient({piv: 1})
                u = vector(F, [full[y] for y in ys] + [zval[z] for z in zs])
                s = S * u
                good = all(s * Ps[i] * s == t[i] for i in range(m))
                print("   residual %dx%d: top-part HS==(1+z)^%d: %s; GB max degree %d (semi-regular square bound %s); "
                      "forgery P(s)=t: %s; structure ok: %s %s; centrifugation %.1fs (%d restarts), solve %.1fs" % (
                      nres, len(free), nres, semireg, maxdeg,
                      'n+1=%d (square: Macaulay bound)' % (nres + 1),
                      good, ok_red, ok_ov, tc, restarts, time.time() - t1))
                done = good
                break
            if done: break
        if not done:
            pass
        if done:
            print("   (target attempt %d)" % (attempt + 1)); break
    sys.stdout.flush()
