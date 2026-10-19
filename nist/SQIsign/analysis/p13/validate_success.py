#!/usr/bin/env python3
"""End-to-end check of the per-instance success model at the real primes.

For each sampled random maximal order (conj_lattice.sage output: all cyclic
degrees <= 16 (p/2)^(1/3)), decide whether Wesolowski's table with
S = {B-smooth s <= X} would succeed, i.e. whether some cyclic phi: E -> E^(p)
has deg phi = s1 s2 with s1, s2 in S (P(n) <= min(B,X), divisor in [n/X, X]).
Compare the empirical success rate with 1 - exp(-lam) from cost_model2.inst
(and with Wesolowski's u^-u lower bound).  Requires X^2 <= 16 (p/2)^(1/3).
usage: python -I validate_success.py <p> <log2X> <log2B> <jsonl...>   (Sage python)"""
import json, math, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import cypari2
pari = cypari2.Pari()
pari.allocatemem(2 * 10**9)
from cost_model2 import inst
from pair_mc import count_window_divisors


def decide(n, X, Bp):
    # quick reject: strip primes < 2^16 by trial division; if the cofactor is a prime > Bp, reject
    f = pari.factor(n, 2 ** 16)
    cof = int(f[0][len(f[0]) - 1])
    if cof > Bp and pari.isprime(cof):
        return False
    f = pari.factor(n)
    ps = {int(f[0][i]): int(f[1][i]) for i in range(len(f[0]))}
    if max(ps) > Bp:
        return False
    return (count_window_divisors(ps, n, X) or 0) > 0


def main():
    p = int(sys.argv[1]); lX = float(sys.argv[2]); lB = float(sys.argv[3]); files = sys.argv[4:]
    X = int(2 ** lX); Bp = int(2 ** min(lB, lX))
    rows = [json.loads(l) for f in files for l in open(f)]
    D0 = (p / 2) ** (1 / 3)
    assert X * X <= 16 * D0 * 1.0001, "X^2 beyond enumerated range"
    succ = 0; nvec = 0
    for r in rows:
        ok = False
        for d in r["degs"]:
            if d > X * X:
                break
            nvec += 1
            if decide(d, X, Bp):
                ok = True
                break
        succ += ok
    lG, llam, lT = inst(math.log2(p), lX, lB)
    lam = 2 ** llam
    u = (math.log2(p) - 1) / (3 * min(lB, lX))
    print(json.dumps({"log2p": math.log2(p), "log2X": lX, "log2B": lB, "samples": len(rows),
                      "empirical_success": succ / len(rows), "se": math.sqrt(max(succ, 1)) / len(rows),
                      "model_1_minus_exp_lam": -math.expm1(-lam), "model_lam": lam,
                      "wes_u^-u": u ** (-u) if u > 1 else 1.0}))


if __name__ == "__main__":
    main()
