#!/usr/bin/env python3
"""Pair Monte Carlo for the MITM success mass.

S = {B-smooth s <= X}.  Pairs (s1, s2) in S x S are sampled with weight
sqrt(s1 s2) (each s_i ~ sqrt(s)-weighted on [1, X], conditioned on P(s) <= B by
rejection).  For n = s1 s2 let r(n) = #{d | n : n/X <= d <= X} (the number of
ordered splits of a cyclic degree-n isogeny that the table can see).  Then
   sum_{n in S*S} sqrt(n)        = (sum_{s in S} sqrt s)^2 * E[1/r]   (distinct phi; table attack)
   sum_{(s1,s2)} sqrt(s1 s2)     = (sum_{s in S} sqrt s)^2             (golden collisions with multiplicity; vOW)
Also reports the mean number of prime factors (with multiplicity) of s and the
mean of the largest prime factor (log2) -- path length / step cost for vOW.
usage: python -I pair_mc.py <log2X> <log2B> <pairs> <seed>   (Sage python: cypari2, gmpy2)"""
import json, math, random, sys
import cypari2, gmpy2
pari = cypari2.Pari()
pari.allocatemem(10**9)


def two_pow(x):
    return int(gmpy2.mpz(2) ** int(x)) if x == int(x) else int(2.0 ** x)


def sample_smooth(rnd, X, B, stats):
    while True:
        y = rnd.randint(1, int(gmpy2.isqrt(gmpy2.mpz(X) ** 3)))
        s = max(1, int(gmpy2.iroot(gmpy2.mpz(y) ** 2, 3)[0]))
        stats["draws"] += 1
        if s == 1:
            return s, {}
        f = pari.factor(s)
        ps = {int(f[0][i]): int(f[1][i]) for i in range(len(f[0]))}
        if max(ps) <= B:
            return s, ps


def count_window_divisors(ps, n, X, cap=10**6):
    lo = -(-n // X)
    divs = [1]
    for q, e in ps.items():
        new = []
        for d in divs:
            m = d
            for _ in range(e + 1):
                if m > X:
                    break
                new.append(m)
                m *= q
        divs = new
        if len(divs) > cap:
            return None
    return sum(1 for d in divs if d >= lo)


def main():
    lX, lB, npairs, seed = float(sys.argv[1]), float(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
    rnd = random.Random(seed)
    X = two_pow(lX)
    B = two_pow(min(lB, lX))
    stats = {"draws": 0, "accepted": 0}
    inv_r = []
    rs = []
    omega = []
    lpf = []
    skipped = 0
    for _ in range(npairs):
        s1, f1 = sample_smooth(rnd, X, B, stats)
        s2, f2 = sample_smooth(rnd, X, B, stats)
        stats["accepted"] += 2
        for s, f in ((s1, f1), (s2, f2)):
            omega.append(sum(f.values()))
            lpf.append(math.log2(max(f)) if f else 0.0)
        ps = dict(f1)
        for q, e in f2.items():
            ps[q] = ps.get(q, 0) + e
        r = count_window_divisors(ps, s1 * s2, X)
        if r is None:
            skipped += 1
            continue
        inv_r.append(1 / r)
        rs.append(r)
    m = len(inv_r)
    mean_inv = sum(inv_r) / m
    var = sum((x - mean_inv) ** 2 for x in inv_r) / max(1, m - 1)
    out = {"log2X": lX, "log2B": lB, "pairs": m, "skipped_huge": skipped,
           "E_inv_r": mean_inv, "se_E_inv_r": math.sqrt(var / m),
           "E_r": sum(rs) / m, "accept_rate": stats["accepted"] / stats["draws"],
           "mean_Omega": sum(omega) / len(omega), "mean_log2_lpf": sum(lpf) / len(lpf)}
    print(json.dumps(out))


if __name__ == "__main__":
    main()
