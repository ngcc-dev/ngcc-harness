#!/usr/bin/env python3
"""sign-15-16: lattice-estimator cost of ATLAS-128 key recovery, specified vs
implemented secret distribution.

Models the published key t1 = power2round(round(p/q * A s1), d = 10) as LWE
with n = 128*6, m = 128*9, q = 2^23 and error modelled as uniform in
[-2^14, 2^14 - 1]: the q -> p rounding (p = 2^18) and the d = 10 dropped bits
together leave q / 2^15 public levels. The specified secret is uniform on
[-16, 16]; the implemented secret is uniform on [1, 16], i.e. a known shift of
8 plus a uniform value in [-7, 8].
Requires the lattice-estimator (commit 53da598) on LATTICE_ESTIMATOR_PATH.
"""
import os
import sys
from math import log2

sys.path.insert(0, os.environ.get("LATTICE_ESTIMATOR_PATH", "../lattice-estimator"))
from estimator import LWE, RC  # noqa: E402
from estimator.nd import Uniform  # noqa: E402

n, m, q = 128 * 6, 128 * 9, 2 ** 23
for label, secret in (("specified [-16,16]", Uniform(-16, 16)),
                      ("implemented [1,16]", Uniform(-7, 8))):
    params = LWE.Parameters(n=n, q=q, Xs=secret, Xe=Uniform(-2 ** 14, 2 ** 14 - 1), m=m, tag=label)
    costs = LWE.estimate(params, red_cost_model=RC.MATZOV,
                         deny_list=["arora-gb", "bkw"], jobs=4, quiet=True)
    bits = {k: round(log2(float(v["rop"])), 1) for k, v in costs.items()}
    print(f"{label}: best 2^{min(bits.values())}  {bits}", flush=True)
