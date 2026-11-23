#!/usr/bin/env python3
"""Fixing-cost aid for kem-18-3 (LoongKEM public consistency equations).  Stdlib only.

It prints numbers for kem-18/fixing_cost.md and implements no scheme:

1. the specification's size formulas (physical p. 20) at the submitted
   parameters, with Table 3 (physical p. 25) as the control;
2. a cost model for the instance that finding kem-18-3 solves: the public
   consistency equations are linear in the k2*N coefficients of r2, so they
   form an LWE instance with n = k2*N unknowns and up to k2*N*(N-1) samples.
   The model is the specification's own primal condition (inequality (1),
   physical p. 27, and section 5.3.1, physical p. 28), with the secret
   rescaled to the error width, and cost 2^(0.292*beta);
3. the smallest k2 at which that model reaches the level, the sizes at that
   k2, and the dominant work counts of one encryption (KEM_Loong.c:119-169).

The model ignores the quotient projection used against Loong384 and the
correlation between equations, both of which help the attacker, and it does
not recompute the failure probability.  The result is a lower bound on the
change.  The parameter values are costing aids, not recommended parameters.
Run: python3 -B kem-18/fixing_cost_unstructured_block.py
"""
import math

Q, DQ = 8191, 13
#        name       lam  N  k1  k2 eta db  du  dv  Table 3 (pk, ct)
SETS = (("Loong128", 128, 12, 48, 4, 5, 10, 10, 4, (1472, 1512)),
        ("Loong256", 256, 16, 64, 6, 4, 10, 10, 10, (3248, 3520)),
        ("Loong384", 384, 20, 72, 8, 3, 11, 11, 6, (6444, 6680)),
        ("Loong512", 512, 24, 80, 10, 2, 11, 11, 4, (10640, 10848)))


def sizes(lam, n, k1, k2, db, du, dv):
    """pksize and ctxsize in bytes, formulas of physical p. 20."""
    return ((lam + k1 * n * db + k2 * n * n * db) // 8 + 16,
            (k1 * n * du + k2 * n * n * du + n * n * dv) // 8)


def xof_bytes(n, k1, k2):
    """Bytes expanded for the public matrix, |x| of Algorithm 20 line 2."""
    return 3 * (k1 * k1 * n + 2 * k1 * k2 * n + k2 * k2 * n * n)


def mults(n, k1, k2):
    """Coefficient multiplications of one encryption, schoolbook as in poly.c:276-291:
    r1*A1, r2*A3, r1*A2, r1*b1 as polynomial products, R2*A4 and R2*B2 as matrix products."""
    return n * n * (k1 * k1 + 2 * k1 * k2 + k1) + n * (k2 * n) ** 2 + k2 * n ** 3


def delta(beta):
    return ((math.pi * beta) ** (1.0 / beta) * beta / (2 * math.pi * math.e)) ** (1.0 / (2 * beta - 2))


def min_beta(n, mmax, ss, se):
    """Smallest beta > 50 with se*sqrt(beta) <= delta^(2beta-d-1) * Vol^(1/d),
    d = n + m + 1, Vol = q^m * (se/ss)^n, over the number of samples m."""
    best = None
    for m in range(max(1, n // 4), min(mmax, 3 * n) + 1, max(1, n // 64)):
        d = n + m + 1

        def ok(beta):
            return (math.log(se) + 0.5 * math.log(beta) <=
                    (2 * beta - d - 1) * math.log(delta(beta)) +
                    (m * math.log(Q) + n * math.log(se / ss)) / d)
        lo, hi = 51, d
        if not ok(hi):
            continue
        while lo < hi:
            mid = (lo + hi) // 2
            if ok(mid):
                hi = mid
            else:
                lo = mid + 1
        best = lo if best is None else min(best, lo)
    return best


def pct(new, old):
    return 100.0 * (new - old) / old


def main():
    for name, lam, n, k1, k2, eta, db, du, dv, ref in SETS:
        pk0, ct0 = sizes(lam, n, k1, k2, db, du, dv)
        print("%s: formulas give pk %d, ct %d; Table 3: pk %d, ct %d; %s"
              % (name, pk0, ct0, ref[0], ref[1], "match" if (pk0, ct0) == ref else "MISMATCH"))
        ss = math.sqrt(eta / 2)                    # one coefficient of r2, psi_eta
        w = 2 ** (DQ - du)                         # rounding step of U
        se = math.sqrt(eta + (w * w - 1) / 6)      # two E4 entries and two rounding errors
        b0 = min_beta(k2 * n, k2 * n * (n - 1), ss, se)
        print("  submitted: k2*N = %d unknowns, %d equations, sigma_s %.2f, sigma_e %.2f,"
              " beta %d (the formula's floor is 51), 2^%.1f"
              % (k2 * n, k2 * n * (n - 1), ss, se, b0, 0.292 * b0))
        k = k2
        while True:
            k += 1
            b = min_beta(k * n, k * n * (n - 1), ss, se)
            if 0.292 * b >= lam:
                break
        print("  smallest k2 at the level: k2 = %d (k2*N = %d), beta %d, 2^%.1f" % (k, k * n, b, 0.292 * b))
        total = (k1 + k2) * n
        k1low = max(0, -(-(total - k * n) // n))
        for label, kk1 in (("k1 kept", k1), ("k1 lowered so that (k1+k2)*N stays %d" % total, k1low)):
            pk, ct = sizes(lam, n, kk1, k, db, du, dv)
            print("    (k1,k2) = (%d,%d), %s: pk %d (%+.0f%%), ct %d (%+.0f%%);"
                  " matrix XOF bytes %d -> %d (x%.1f); encryption multiplications %d -> %d (x%.1f)"
                  % (kk1, k, label, pk, pct(pk, pk0), ct, pct(ct, ct0),
                     xof_bytes(n, k1, k2), xof_bytes(n, kk1, k),
                     xof_bytes(n, kk1, k) / xof_bytes(n, k1, k2),
                     mults(n, k1, k2), mults(n, kk1, k), mults(n, kk1, k) / mults(n, k1, k2)))


if __name__ == "__main__":
    main()
