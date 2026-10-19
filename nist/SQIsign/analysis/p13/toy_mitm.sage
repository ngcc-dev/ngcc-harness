# toy_mitm.sage -- real-isogeny toy run of Wesolowski's table attack.
# For random supersingular E/F_{p^2}: enumerate all cyclic isogenies from E of
# B-smooth degree <= X (as j-invariant chains via modular polynomials, primes in
# non-decreasing order, non-backtracking), store j -> degree, and test whether some
# entry's conjugate j^p is also an entry (=> isogeny E -> E^(p) of degree s1*s2).
# Compare the empirical success rate with the model
#   lam = sum_{n in S*S} 1.5*c3*sqrt(n)/sqrt(p)   (exact enumeration of S*S),
# P = 1 - exp(-lam).  Also reports the mean table size vs sum_{s in S} psi(s).
# usage: sage toy_mitm.sage <bits> <B> <X> <trials> <seed>
import sys, json
from sage.all import *
from sage.schemes.elliptic_curves.mod_poly import classical_modular_polynomial

bits, B, X, trials, seed = [int(a) for a in sys.argv[1:6]]
set_random_seed(seed)
p = next_prime(2**bits)
while p % 4 != 3:
    p = next_prime(p)
F = GF(p**2, 'a')
R = PolynomialRing(F, 'y'); y = R.gen()
primes = [l for l in prime_range(B + 1)]
PHI = {l: classical_modular_polynomial(l) for l in primes}
PR = PHI[2].parent()
Xv, Yv = PR.gens()

def nbrs(j, l):
    # roots (with multiplicity) of Phi_l(j, Y) in F_{p^2}
    coeffs = [0] * (l + 2)
    for (e1, e2), c in PHI[l].dict().items():
        coeffs[e2] += F(c) * j**e1
    f = R(coeffs)
    out = []
    for r, m in f.roots():
        out += [r] * m
    return out

def random_ss():
    j = F(1728)
    prev = None
    for _ in range(4 * bits):
        ns = nbrs(j, 2)
        cand = list(ns)
        if prev is not None and prev in cand:
            cand.remove(prev)
        prev, j = j, cand[ZZ.random_element(len(cand))]
    return j

# S = B-smooth s <= X
S = [s for s in range(1, X + 1) if s == 1 or max(prime_factors(s)) <= B]
def table(j0):
    tab = {j0: 1}
    count = 1
    # stack entries: (j, prev_j, degree, last_prime)
    stack = [(j0, None, 1, 2)]
    while stack:
        j, pj, d, lp = stack.pop()
        for l in primes:
            if l < lp or d * l > X:
                continue
            ns = nbrs(j, l)
            if pj is not None and l == lp and pj in ns:
                ns.remove(pj)        # non-backtracking (same prime)
            for jn in ns:
                count += 1
                tab.setdefault(jn, d * l)
                stack.append((jn, j, d * l, l))
    return tab, count

def success(j0):
    tab, cnt = table(j0)
    for jj in tab:
        if jj**p in tab:
            return True, cnt
    return False, cnt

c3 = 4 * pi.n() / (3 * zeta(3).n())
SS = set(a * b for a in S for b in S if a * b <= X * X)
SS = [n for n in SS if any(n % a == 0 and n // a <= X for a in S)]
lam = sum(1.5 * c3 * sqrt(n.n() if hasattr(n, 'n') else float(n)) for n in SS) / sqrt(float(p))
psisum = sum(s * prod(1 + 1 / q for q in prime_factors(s)) for s in S)
succ = 0; cnts = 0
for t in range(trials):
    j0 = random_ss()
    ok, cnt = success(j0)
    succ += ok; cnts += cnt
print(json.dumps({"p": int(p), "B": B, "X": X, "trials": trials, "succ_rate": float(succ / trials),
                  "se": float(sqrt(max(succ, 1)) / trials), "model_lam": float(lam), "model_P": float(1 - exp(-lam)),
                  "mean_table": float(cnts / trials), "psi_sum": float(psisum)}))
