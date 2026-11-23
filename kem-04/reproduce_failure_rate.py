#!/usr/bin/env python3
"""kem-04-4: the coded BAG-Piglet-128 parameters give a 2^-82.2 failure rate.

The specification states BAG-Piglet-128 twice. Table 1 (physical p. 8) lists
n = 47 and m = 43; Table 2 on the same page defines the ring by P(X) of degree
43 and the field by f(X) of degree 47, and the submitted parameters.h has
n = 43, m = 47. The failure rate of physical p. 4,

    DFR = q^(delta (n' - n n1)) * sum_{i<eps} prod_{j<i}
          (q^delta - q^j)(q^(n n1 - n') - q^j) / (q^i - q^j),

depends on n through the tail length n n1 - n'. This script reads n, n', n1,
k and eps from parameters.h in the reference and optimized trees, evaluates
the formula for both readings, and checks the other three levels against
Table 5 (physical p. 21). Standard library only; it evaluates the
specification's formula and implements no part of the scheme.
"""
from math import log2
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
TREES = ("Reference_Implementation", "Optimized_Implementation")
TABLE5 = {128: -130, 256: -273, 384: -394, 512: -520}  # physical p. 21
TABLE1_N128 = 47  # physical p. 8


def params(tree, level):
    text = (HERE / "Implementations" / tree / f"bag_piglet{level}" / "src/scheme/parameters.h").read_text()
    get = lambda name: int(re.search(rf"#define {name}\s+(\d+)", text).group(1))
    return {k: get(v) for k, v in (("n", "PARAM_N"), ("np", "PARAM_N_"), ("n1", "PARAM_N_1"),
                                   ("k", "PARAM_K"), ("eps", "PARAM_epsilon"), ("m", "PARAM_M"))}


def log2_dfr(delta, tail, eps, q=2):
    """log2 of the failure rate of physical p. 4, with tail = n n1 - n'."""
    total = 0
    for i in range(eps):
        num = den = 1
        for j in range(i):
            num *= (q ** delta - q ** j) * (q ** tail - q ** j)
            den *= q ** i - q ** j
        total += num // den
    bits = total.bit_length()
    lg = log2(total) if bits < 1000 else (bits - 60) + log2(total >> (bits - 60))
    return lg - delta * tail * log2(q)


def main():
    ok = True
    for level in (128, 256, 384, 512):
        p = [params(t, level) for t in TREES]
        if p[0] != p[1]:
            print(f"BAG-Piglet-{level}: the two trees differ: {p}")
            return 1
        p = p[0]
        delta = (p["np"] - p["k"] + p["eps"]) // 2
        lg = log2_dfr(delta, p["n"] * p["n1"] - p["np"], p["eps"])
        print(f"BAG-Piglet-{level}: code n={p['n']} m={p['m']} n'={p['np']} k={p['k']} eps={p['eps']} "
              f"delta={delta}: log2 DFR {lg:.1f} (Table 5: {TABLE5[level]})")
        if level == 128:
            lg1 = log2_dfr(delta, TABLE1_N128 * p["n1"] - p["np"], p["eps"])
            print(f"BAG-Piglet-128: Table 1 n={TABLE1_N128}: log2 DFR {lg1:.1f} (Table 5: -130)")
            for q in (64, 80):
                print(f"  expected failures among 2^{q} honest ciphertexts at the coded rate: "
                      f"2^{q + lg:.1f} = {2 ** (q + lg):.3g}")
            ok &= round(lg1) == TABLE5[128] and lg < -82 and lg > -83 and p["n"] == 43 and p["m"] == 47
        else:
            ok &= round(lg) == TABLE5[level]
    if not ok:
        print("NOT CONFIRMED kem-04-4: the parameters or failure rates differ from the archive")
        return 1
    print("CONFIRMED kem-04-4: the coded BAG-Piglet-128 parameters give a failure rate of 2^-82.2 by the "
          "specification's formula; only Table 1's n = 47 gives the claimed 2^-130, and the other levels match Table 5")
    return 0


if __name__ == "__main__":
    sys.exit(main())
