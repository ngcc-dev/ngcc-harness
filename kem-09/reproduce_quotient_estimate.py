#!/usr/bin/env python3
"""Validate and estimate XuHaomeng's Cheetah degree-128 quotient."""

import math, os, random, sys
from pathlib import Path
Q = 7681

def fold(a): return [sum((-1)**b * a[j + 128*b] for b in range(5)) % Q for j in range(128)]
def negacyclic(a, b):
    out = [0] * len(a)
    for i, x in enumerate(a):
        if x:
            for j, y in enumerate(b):
                if y: out[(i+j) % len(a)] += (-1 if i+j >= len(a) else 1) * x*y
    return [x % Q for x in out]

def main():
    rng = random.Random(9); a, b = [0]*640, [0]*640
    for v in (a, b):
        for i in rng.sample(range(640), 12): v[i] = rng.randrange(Q)
    assert fold(negacyclic(a, b)) == negacyclic(fold(a), fold(b))
    root = Path(__file__).resolve().parents[2]
    sys.path.insert(0, os.environ.get("LATTICE_ESTIMATOR_PATH", str(root / "lattice-estimator")))
    try:
        from estimator import LWE, ND
        from sage.all import log
    except ImportError as e:
        raise SystemExit(
            "run with `sage -python` and set LATTICE_ESTIMATOR_PATH to a "
            "malb/lattice-estimator checkout"
        ) from e
    expected = (27.7, 67.7, 99.3, 133.4); observed = []
    for k, eta, db in ((1,5,10), (2,4,10), (3,3,11), (4,2,11)):
        n = 128*k
        sigma = math.sqrt(5 * (eta/2 + (Q/(2**db*math.sqrt(12)))**2))
        p = LWE.Parameters(n=n, q=Q, Xs=ND.CenteredBinomial(5*eta), Xe=ND.DiscreteGaussian(sigma), m=n)
        bits = float(log(LWE.estimate.rough(p, jobs=1)["usvp"]["rop"], 2)); observed.append(bits)
        print(f"Cheetah-{128*k}: quotient n={n}, uSVP={bits:.2f} bits")
    ok = all(abs(a-b) < .15 for a,b in zip(observed, expected))
    print("CONFIRMED: reported quotient estimates reproduced" if ok else "NOT CONFIRMED")
    return 0 if ok else 1

if __name__ == "__main__": raise SystemExit(main())
