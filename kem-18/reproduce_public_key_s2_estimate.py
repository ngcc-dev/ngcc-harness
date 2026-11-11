#!/usr/bin/env python3
"""kem-18-5: lattice-estimator cost of the s1 block once s2 is known.

After s2 is recovered, b1 - A2 s2 = A1 s1 + e1 is modelled as plain LWE with
n = m = K1*N, q = 8191, a CBD(eta) secret and an error with the variance of
CBD(eta) plus the public-key rounding to d_b bits. The further B2 equations in
s1 are ignored, so these figures are not lower bounds.
Requires the lattice-estimator (commit 53da598) on LATTICE_ESTIMATOR_PATH.
"""
import os
import sys
from math import log2, sqrt

sys.path.insert(0, os.environ.get("LATTICE_ESTIMATOR_PATH", "../lattice-estimator"))
from estimator import LWE, ND, RC  # noqa: E402

for level, n_, k1, eta, db in ((128, 12, 48, 5, 10), (256, 16, 64, 4, 10)):
    step = 2 ** (13 - db)
    sd = sqrt(eta / 2 + (step * step - 1) / 12)
    params = LWE.Parameters(n=k1 * n_, q=8191, Xs=ND.CenteredBinomial(eta),
                            Xe=ND.DiscreteGaussian(sd), m=k1 * n_, tag=f"Loong{level} s1")
    costs = LWE.estimate(params, red_cost_model=RC.MATZOV,
                         deny_list=["arora-gb", "bkw"], jobs=4, quiet=True)
    bits = {k: round(log2(float(v["rop"])), 1) for k, v in costs.items()}
    print(f"Loong{level} s1 after s2: n={k1 * n_} best 2^{min(bits.values())}  {bits}", flush=True)
