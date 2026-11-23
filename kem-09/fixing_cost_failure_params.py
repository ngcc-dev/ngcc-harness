#!/usr/bin/env python3
"""Fixing-cost aid for kem-09-5 (CheetahKEM decryption failures).  Stdlib only.

It prints numbers for kem-09/fixing_cost.md and implements no scheme:

1. the specification's size formulas (physical p. 17) at the submitted and at
   changed parameters, with Table 3 (physical p. 21) as the control;
2. a stdlib port of the Gaussian failure-boosting model of
   kem-09/reproduce_boost_cost.py (the model of finding kem-09-5): the fewest
   honest encapsulations that give 1,500 failures within a query budget;
3. the primal uSVP condition of the specification (physical pp. 23-24) with
   cost 2^(0.292*beta), for the full-dimension problem at a changed eta.

The parameter values are costing aids, not recommended parameters.
Run: python3 -B kem-09/fixing_cost_failure_params.py      (about 25 seconds)
"""
import math
from math import comb

N, Q, DQ, THR, NF = 640, 7681, 13, 1921, 1500
#        name         k eta db  du  dv  lam  Table 3 (pk, ct)
SETS = (("Cheetah128", 1, 5, 10, 10, 4, 128, (832, 864)),
        ("Cheetah256", 2, 4, 10, 10, 4, 256, (1648, 1728)),
        ("Cheetah384", 3, 3, 11, 11, 4, 384, (2704, 2832)),
        ("Cheetah512", 4, 2, 11, 11, 8, 512, (3600, 4032)))


def sizes(k, lam, db, du, dv):
    """eksize and |c| in bytes, formulas of physical p. 17."""
    return lam // 8 + k * N * db // 8 + 16, (k * N * du + lam * dv) // 8


def lse(terms):
    """log(sum(p * exp(a))) over (a, p)."""
    m = max(a for a, _ in terms)
    return m + math.log(sum(p * math.exp(a - m) for a, p in terms))


def log_half_erfc(z):
    if z < 25.0:
        return math.log(0.5 * math.erfc(z))
    s = 1 - 1 / (2 * z * z) + 3 / (4 * z ** 4) - 15 / (8 * z ** 6)
    return -z * z - math.log(2 * z * math.sqrt(math.pi)) + math.log(s)


def boost(k, eta, db, du, dv, lam, budget=80):
    """Port of reproduce_boost_cost.gaussian_model.

    Returns (log2 queries without preselection, best), where best is None when
    no preselection reaches the budget, else (x, log2 selection probability,
    log2 failure rate, log2 queries, log2 encapsulations).
    """
    n = N * k
    xs = list(range(-eta, eta + 1))
    cb = [comb(2 * eta, j + eta) / 4 ** eta for j in xs]
    w = 2 ** (DQ - du)
    fx = list(range(-eta - w // 2 + 1, eta + w // 2 + 1))
    fp = [0.0] * len(fx)
    for i, x in enumerate(xs):
        for u in range(-w // 2 + 1, w // 2 + 1):
            fp[x + u - fx[0]] += cb[i] / w
    wpk = 2 ** (DQ - db)
    var_ep = eta / 2 + (wpk * wpk - 1) / 12
    var_s = eta / 2

    def cgf(th):
        return n * (lse([(th * var_ep * x * x, p) for x, p in zip(xs, cb)]) +
                    lse([(th * var_s * x * x, p) for x, p in zip(fx, fp)]))

    def tilted_mean(vals, probs, wgt, th):
        a = [th * wgt * x * x for x in vals]
        m = max(a)
        ws = [p * math.exp(ai - m) for ai, p in zip(a, probs)]
        return sum(wi * wgt * x * x for wi, x in zip(ws, vals)) / sum(ws)

    def dcgf(th):
        return n * (tilted_mean(xs, cb, var_ep, th) + tilted_mean(fx, fp, var_s, th))

    mean = n * (var_ep * sum(p * x * x for x, p in zip(xs, cb)) +
                var_s * sum(p * x * x for x, p in zip(fx, fp)))
    top = n * (var_ep * eta ** 2 + var_s * fx[-1] ** 2)
    ev = list(range(-2 ** (DQ - 1 - dv) + 1, 2 ** (DQ - 1 - dv) + 1))
    best, base, i = None, None, 0
    while True:
        x = 1.0 + 0.01 * i
        if x >= top / mean:
            break
        t = x * mean
        if i == 0:
            lsel = 0.0
        else:
            lo, hi = 1e-12, 50.0
            if dcgf(hi) < t:
                break
            for _ in range(70):
                mid = (lo + hi) / 2
                if dcgf(mid) < t:
                    lo = mid
                else:
                    hi = mid
            th = (lo + hi) / 2
            lsel = -(th * t - cgf(th)) / math.log(2)
        sc = math.sqrt(t + eta / 2)
        vals = [log_half_erfc((THR - e) / sc / math.sqrt(2)) for e in ev]
        m = max(vals)
        lp = m + math.log(sum(math.exp(v - m) for v in vals)) - math.log(len(ev)) + math.log(2)
        lbeta = (lp + math.log(lam)) / math.log(2)
        lq = math.log2(NF) - lbeta
        if i == 0:
            base = lq
        if lq <= budget and (best is None or lq - lsel < best[4]):
            best = (x, lsel, lbeta, lq, lq - lsel)
        i += 1
    return base, best


def delta(beta):
    return ((math.pi * beta) ** (1.0 / beta) * beta / (2 * math.pi * math.e)) ** (1.0 / (2 * beta - 2))


def primal_bits(k, eta):
    """Smallest beta with sigma*sqrt(beta) < delta^(2beta-d-1) * q^(m/d), d = n+m+1,
    over the number of samples m <= n; returns 0.292*beta."""
    n, sigma, best = N * k, math.sqrt(eta / 2), None
    for m in range(n // 2, n + 1, 8):
        d = n + m + 1

        def ok(beta):
            return (math.log(sigma) + 0.5 * math.log(beta) <
                    (2 * beta - d - 1) * math.log(delta(beta)) + (m / d) * math.log(Q))
        lo, hi = 51, d - 1
        if not ok(hi):
            continue
        while lo < hi:
            mid = (lo + hi) // 2
            if ok(mid):
                hi = mid
            else:
                lo = mid + 1
        best = lo if best is None else min(best, lo)
    return 0.292 * best


def cost_text(best, lam):
    if best is None:
        return "no preselection reaches the budget (withstands in the model)"
    verdict = "at or above" if best[4] >= lam else "BELOW"
    return ("x=%.2f, failure rate 2^%.1f, queries 2^%.1f, encapsulations 2^%.1f, %s 2^%d"
            % (best[0], best[2], best[3], best[4], verdict, lam))


def row(name, k, eta, db, du, dv, lam, ref, budget=80):
    pk0, ct0 = ref
    pk, ct = sizes(k, lam, db, du, dv)
    _, best = boost(k, eta, db, du, dv, lam, budget)
    print("  %s eta=%d (db,du,dv)=(%d,%d,%d): pk %d (%+.2f%%), ct %d (%+.2f%%); %s"
          % (name, eta, db, du, dv, pk, 100.0 * (pk - pk0) / pk0, ct, 100.0 * (ct - ct0) / ct0,
             cost_text(best, lam)), flush=True)


def main():
    print("1. Size control: formulas of physical p. 17 against Table 3 (physical p. 21)")
    for name, k, eta, db, du, dv, lam, ref in SETS:
        got = sizes(k, lam, db, du, dv)
        print("  %s: pk %d, ct %d; Table 3: pk %d, ct %d; %s"
              % (name, got[0], got[1], ref[0], ref[1], "match" if got == ref else "MISMATCH"))

    print("2. Model control at the submitted parameters, budget 2^80 queries"
          " (compare kem-09/reproduce_boost_cost.py)")
    for name, k, eta, db, du, dv, lam, ref in SETS:
        base, best = boost(k, eta, db, du, dv, lam)
        print("  %s: unboosted 2^%.1f queries for %d failures; %s"
              % (name, base, NF, cost_text(best, lam)), flush=True)

    print("3. Budget 2^64 queries per key at the submitted parameters")
    for name, k, eta, db, du, dv, lam, ref in SETS[:2]:
        _, best = boost(k, eta, db, du, dv, lam, 64)
        print("  %s: %s" % (name, cost_text(best, lam)), flush=True)

    print("4. eta lowered, compression unchanged, budget 2^80; primal uSVP of physical pp. 23-24")
    for name, k, eta, db, du, dv, lam, ref in SETS[:2]:
        for e in range(eta, 0, -1):
            row(name, k, e, db, du, dv, lam, ref)
            print("      full-dimension primal uSVP at eta=%d: 2^%.1f" % (e, primal_bits(k, e)), flush=True)

    print("5. eta kept, compression changed, budget 2^80")
    name, k, eta, db, du, dv, lam, ref = SETS[0]
    for v in (4, 5, 6):
        row(name, k, eta, db, du, v, lam, ref)
    name, k, eta, db, du, dv, lam, ref = SETS[1]
    for b in (10, 11, 12):
        for u in (10, 11, 12):
            for v in (4, 5, 6, 7):
                if (b, u) == (12, 12) and v > 4:
                    continue
                row(name, k, eta, b, u, v, lam, ref)

    print("6. Alternative points named in the note, budget 2^80")
    for i, e, b, u, v in ((0, 4, 10, 10, 8), (0, 5, 11, 11, 4),
                          (1, 2, 11, 10, 5), (1, 2, 11, 11, 4), (1, 4, 12, 12, 5)):
        name, k, eta, db, du, dv, lam, ref = SETS[i]
        row(name, k, e, b, u, v, lam, ref)


if __name__ == "__main__":
    main()
