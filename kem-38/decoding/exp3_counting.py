"""Reproduce the spec's (u,u)D counting model (eq. 55) for UVW128 and compare
with the exact weight distribution of the dual-side GRS-doubled subcode.
Also an ISD cost check with CryptographicEstimators (Stern, q-ary)."""
import sys
from math import comb, log2, exp
from fractions import Fraction

q, n, k, k1, k2, w = 433, 860, 430, 215, 215, 116
h = n // 2

def lg(x):
    return log2(x) if x > 0 else float('-inf')

def lgfrac(num, den):
    return log2(num) - log2(den)

def lE1(t):   # log2 of (u,u)D words of weight t, random U model (spec eq. 55)
    if t % 2:
        return float('-inf')
    return lgfrac(comb(h, t // 2) * (q - 1) ** (t // 2), q ** (h - k1))

def lE2(t):   # log2 of non-(u,u)D words
    return lgfrac(comb(n, t) * (q - 1) ** t * (q ** k2 - 1), q ** (n - k) * q ** k2)

def E1(t):
    return 2.0 ** lE1(t)

def E2(t):
    return 2.0 ** lE2(t)

def lP(t):
    a, b = lE1(t), lE2(t)
    if a == float('-inf'):
        return float('-inf')
    m = max(a, b)
    return a - (m + log2(2 ** (a - m) + 2 ** (b - m)))

def mds_weight(nn, kk, t):
    """number of weight-t words in an [nn,kk] MDS code over F_q."""
    d = nn - kk + 1
    if t < d:
        return 0
    return comb(nn, t) * sum((-1) ** j * comb(t, j) * (q ** (t - d + 1 - j) - 1) for j in range(t - d + 1))

def E1_dual(t):  # (x,-x)D' words of C^perp: x in GRS[430,215], exact MDS count
    if t % 2:
        return 0
    return mds_weight(h, h - k2, t // 2)

print("t     log2 E1(primal,random U)  log2 E1(dual, exact GRS)  log2 E2   log2 P=E1/(E1+E2)")
for t in [330, 334, 336, 338, 340, 360, 380, 400, 410, 420, 430, 432, 434, 440, 460]:
    print(f"{t:4d}  {lE1(t):10.2f}  {lg(E1_dual(t)):12.2f}  {lE2(t):10.2f}  {lP(t):10.2f}")
# first t with E1>=1, E2>=1
t1 = min(t for t in range(2, n, 2) if lE1(t) >= 0)
t2 = min(t for t in range(1, n) if lE2(t) >= 0)
t1d = min(t for t in range(2, n, 2) if E1_dual(t) >= 1)
print(f"first weight with E1>=1: {t1}; E2>=1: {t2}; dual exact GRS-doubled E1>=1: {t1d}")
print(f"E2/E1 at t=430: 2^{lE2(430)-lE1(430):.1f} (spec: 2^428)")
# GV-type: min distance estimate of random [430,215] over F_433 and of random [860,430]
dU = min(t for t in range(1, h) if lgfrac(comb(h, t) * (q - 1) ** t, q ** (h - k1)) >= 0)
dC = min(t for t in range(1, n) if lgfrac(comb(n, t) * (q - 1) ** t, q ** (n - k)) >= 0)
print(f"random [430,215]: first weight with >=1 expected word: {dU} (GRS[430,215] min distance 216); random [860,430]: {dC}")

# ISD cost for finding weight-t codewords via CryptographicEstimators (Stern, q-ary)
try:
    from cryptographic_estimators.SDFqEstimator import SDFqEstimator
    for t in [336, 380, 410, 420]:
        lnsol = max(0.0, max(lE1(t), lE2(t)))
        nsol = 2.0 ** lnsol
        est = SDFqEstimator(n=n, k=k, w=t, q=q)
        res = est.estimate()
        best = min(res.items(), key=lambda kv: kv[1]['estimate']['time'])
        ft = best[1]['estimate']['time']
        print(f"t={t}: nsolutions=2^{lnsol:.1f}  best {best[0]} log2 time={ft:.2f} (mem {best[1]['estimate']['memory']:.1f})"
              f"  log2 F/P = {ft - lP(t):.2f}")
except Exception as e:
    print("estimator failed:", repr(e))

# message attack figure for reference (unique decoding regime)
try:
    est = SDFqEstimator(n=n, k=k, w=w, q=q)
    res = est.estimate()
    print("message SD, best:", min(((a, r['estimate']['time']) for a, r in res.items()), key=lambda x: x[1]))
except Exception as e:
    print("estimator failed:", repr(e))

# ---- decoder-side counting for kem-38-5 context ----
# expected number of 'cancelled' pairs in a weight-w error: both coordinates of a hidden pair hit
# with the ratio that cancels in c11 - c12.
pairs_both = comb(w, 2) / (n - 1)        # expected pairs with both coordinates in supp(e)
canc = pairs_both / (q - 1)              # equal (scaled) values
print(f"expected pairs with both coords in supp(e): {pairs_both:.3f}; expected cancelled positions: {canc:.5f}")
# Poisson tail: P(>= c cancelled)
lam = canc
def pois_tail(c):
    s = sum(exp(-lam) * lam ** j / __import__('math').factorial(j) for j in range(c))
    return 1 - s
for c in range(1, 8):
    print(f"  P(>= {c} cancelled) ~ 2^{lg(pois_tail(c)):.1f}")
