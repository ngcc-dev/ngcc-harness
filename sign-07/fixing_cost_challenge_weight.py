#!/usr/bin/env python3
"""Fixing-cost arithmetic for sign-07-2 (CS): challenge weight and repetition count.

Stdlib only. Prints numbers for sign-07/fixing_cost.md. It implements no part
of the scheme: it evaluates binomial counts and the repetition-count formula of
Theorem 3.2 (specification, physical p. 23) on the Table 3 parameters
(physical p. 24).

Usage: python3 -B sign-07/fixing_cost_challenge_weight.py
"""
from math import comb, log2, sqrt

# Table 3 (physical p. 24). p0/p1/p2 = (a, b, B) of y0, y1, y2.
# M0 = the per-iteration constants m_i = 2^64 / M_i of the reference params.h
# (lines 31, 55, 79); Mtab = "Signature repetition count" of Table 3.
SETS = {
    "CS-128": dict(lam=128, n=256, k=3, l=3, tau=23, taup=4,
                   p0=(5, 5, 116), p1=(10, 9, 2779), p2=(6, 9, 1962),
                   beta=32, alpha=2016, Mtab=1.75,
                   M0=[16754060490623551488, 16762958675265769472,
                       16770730982686846976, 16777320755843377152,
                       16782655996990345216, 17413884194018961408]),
    "CS-256": dict(lam=256, n=512, k=3, l=3, tau=44, taup=3,
                   p0=(6, 6, 278), p1=(9, 10, 5385), p2=(9, 9, 2569),
                   beta=128, alpha=8064, Mtab=1.96,
                   M0=[17620290888719722496, 17621784278981462016,
                       17623226973804584960, 17624617865583456256,
                       17625955695161976832, 17627239040357314560,
                       17628466310185975808, 17629635734405001216,
                       17630745355846195200, 17631793025200351232,
                       17632776383313438720, 17633692857687846912,
                       17634539649000501248, 17635313719477792768,
                       18071621945441501184]),
    "CS-512": dict(lam=512, n=512, k=6, l=5, tau=118, taup=6,
                   p0=(6, 6, 261), p1=(12, 11, 12793), p2=(8, 11, 9771),
                   beta=32, alpha=4032, Mtab=2.00,
                   M0=[17819932119395569664, 17820095570143191040,
                       17820168299748177920, 17820145520275107840,
                       17820022064670330880, 17819792397902534656,
                       17819450603968778240, 17818990350020651008,
                       17818404877047054336, 17817686985319770112,
                       17816829001564188672, 17815822777217662976,
                       17814659644361285632, 17813330417851742208,
                       17811825350397577216, 17810134137097097216,
                       17808245855887214592, 17806148981017155584,
                       17803831322687092736, 18120912975124414464]),
}


class CMD:
    """Exact counts of D_{a,b}: sum of a differences of two uniform b-bit values."""

    def __init__(self, a, b):
        w = 1 << b
        cur = [1]
        for _ in range(2 * a):          # 2a uniform terms, sliding-window sum
            out, s = [0] * (len(cur) + w - 1), 0
            for i in range(len(out)):
                if i < len(cur):
                    s += cur[i]
                if i >= w:
                    s -= cur[i - w]
                out[i] = s
            cur = out
        self.half = a * (w - 1)
        self.cnt = cur
        self.lg = [log2(c) for c in cur]
        self.ps = [0]
        for c in cur:
            self.ps.append(self.ps[-1] + c)

    def mass(self, t):                  # sum of f(y) over |y| <= t
        t = min(t, self.half)
        return self.ps[self.half + t + 1] - self.ps[self.half - t]

    def maxratio(self, r, lim):         # max over |x| <= lim of log2 f(x)^2/(f(x+r)f(x-r))
        best = 0.0
        for x in range(0, min(lim, self.half - r) + 1):
            h = self.half + x
            best = max(best, 2 * self.lg[h] - self.lg[h + r] - self.lg[h - r])
        return best

    def entropy(self, t):               # entropy in bits of D truncated to [-t, t]
        tot, e = self.mass(t), 0.0
        for x in range(-t, t + 1):
            p = self.cnt[self.half + x] / tot
            e -= p * log2(p)
        return e


_cache = {}


def cmd(a, b):
    if (a, b) not in _cache:
        _cache[(a, b)] = CMD(a, b)
    return _cache[(a, b)]


def repetitions(P, tau=None, taup=None, p0=None, p1=None, p2=None):
    """Theorem 3.2. The z0 shift is counted on the tau'' coefficients of the
    block (the reading that reproduces the params.h constants)."""
    n, k, l = P["n"], P["k"], P["l"]
    tau, taup = tau or P["tau"], taup or P["taup"]
    (a0, b0, B0), (a1, b1, B1), (a2, b2, B2) = p0 or P["p0"], p1 or P["p1"], p2 or P["p2"]
    f0, f1, f2 = cmd(a0, b0), cmd(a1, b1), cmd(a2, b2)
    d1, d2 = l * n, k * n
    N = -(-tau // taup)

    def lgA(i):
        ti = min(i * taup, tau)
        return (n * log2(f0.mass(B0 - i)) + d1 * log2(f1.mass(B1 - ti))
                + d2 * log2(f2.mass(B2 - ti)))

    lA = [lgA(i) for i in range(N + 1)]
    Mi = []
    for i in range(N):
        tpp = min(taup, tau - i * taup)
        tn = min((i + 1) * taup, tau)
        m = (tpp * f0.maxratio(1, B0 - i - 1) + d1 * f1.maxratio(tpp, B1 - tn)
             + d2 * f2.maxratio(tpp, B2 - tn))
        Mi.append(2 ** (lA[i] - lA[i + 1] + m / 2))
    bound = 2 ** (2 * (n * a0 * b0 + d1 * a1 * b1 + d2 * a2 * b2) - lA[0])
    total = bound
    for x in Mi:
        total *= x
    # expected Reject iterations run per signature
    p, per_attempt = 1.0 / bound, 0.0
    for x in Mi:
        per_attempt += p
        p /= x
    return dict(N=N, Mi=Mi, bound=bound, M=total, iters=per_attempt * total)


def mask_bits(P, p0, p1, p2):
    return 2 * P["n"] * (p0[0] * p0[1] + P["l"] * p1[0] * p1[1] + P["k"] * p2[0] * p2[1])


def scaled(p, da):
    a, b, B = p
    return (a + da, b, round(B * sqrt((a + da) / a)))


def main():
    print("1. Challenge count (finding's model: binomial(n, tau) hash trials)")
    new_tau = {}
    for name, P in SETS.items():
        n, tau, lam = P["n"], P["tau"], P["lam"]
        lb = log2(comb(n, tau))
        t = next((t for t in range(tau, n // 2 + 1) if comb(n, t) >= 1 << lam), None)
        new_tau[name] = t
        line = "   %s: n=%d tau=%d  log2 C(n,tau)=%.2f  with signs %.2f;" % (name, n, tau, lb, lb + tau)
        if t:
            line += "  smallest tau with C(n,tau) >= 2^%d: %d (%.2f)" % (lam, t, log2(comb(n, t)))
        else:
            line += "  no tau: max log2 C(n,n/2)=%.2f < %d" % (log2(comb(n, n // 2)), lam)
        print(line)

    print("2. Control: Theorem 3.2 against params.h M0[] and Table 3")
    base = {}
    for name, P in SETS.items():
        r = base[name] = repetitions(P)
        code = [2 ** 64 / m for m in P["M0"]]
        dev = max(abs(a / b - 1) for a, b in zip(r["Mi"], code))
        print("   %s: N=%d  M_0 model %.5f code %.5f  max deviation over all M_i %.2f%%  "
              "bound rejection %.4f  M model %.3f  Table 3 %.2f  iterations/signature %.2f"
              % (name, r["N"], r["Mi"][0], code[0], 100 * dev, r["bound"], r["M"], P["Mtab"], r["iters"]))

    print("3. Repaired weight, other parameters unchanged")
    for name, P in SETS.items():
        t = new_tau[name]
        cases = [(t, P["taup"])] if t else [(200, P["taup"]), (256, P["taup"])]
        if t:
            cases += [(t, tp) for tp in range(P["taup"] - 1, 1, -1)]
        for tau, taup in cases:
            r, b = repetitions(P, tau=tau, taup=taup), base[name]
            print("   %s tau=%d tau'=%d: log2 C=%.2f  N=%d  M %.3f (x%.3f)  iterations/signature %.2f (x%.3f)"
                  % (name, tau, taup, log2(comb(P["n"], tau)), r["N"], r["M"], r["M"] / b["M"],
                     r["iters"], r["iters"] / b["iters"]))

    print("4. Repaired weight with wider masks (a increased, B scaled by sqrt)")
    for name, das in (("CS-128", (1, 2, 1)), ("CS-256", (1, 2, 2))):
        P, b, tau = SETS[name], base[name], new_tau[name]
        q0, q1, q2 = (scaled(P[k], d) for k, d in zip(("p0", "p1", "p2"), das))
        r = repetitions(P, tau=tau, p0=q0, p1=q1, p2=q2)
        h0 = cmd(*q0[:2]).entropy(q0[2] - r["N"]) - cmd(*P["p0"][:2]).entropy(P["p0"][2] - b["N"])
        h1 = cmd(*q1[:2]).entropy(q1[2] - tau) - cmd(*P["p1"][:2]).entropy(P["p1"][2] - P["tau"])
        nbytes = (P["n"] * h0 + P["l"] * P["n"] * h1) / 8
        print("   %s tau=%d (a0,a1,a2)=(%d,%d,%d) (B0,B1,B2)=(%d,%d,%d): M %.3f (x%.3f)  "
              "iterations/signature x%.3f  mask bits/attempt x%.3f  entropy z0 %+.3f z1 %+.3f bits/coeff = %+.1f bytes"
              % (name, tau, q0[0], q1[0], q2[0], q0[2], q1[2], q2[2], r["M"], r["M"] / b["M"],
                 r["iters"] / b["iters"], mask_bits(P, q0, q1, q2) / mask_bits(P, P["p0"], P["p1"], P["p2"]),
                 h0, h1, nbytes))

    print("5. Verification bound B2' = B2 - tau + alpha/4 + 1 + tau*beta/2 (physical p. 21)")
    for name, P in SETS.items():
        B2, tau, al, be = P["p2"][2], P["tau"], P["alpha"], P["beta"]
        t = new_tau[name]
        line = "   %s: submitted %d (tau*beta/2 = %d)" % (name, B2 - tau + al // 4 + 1 + tau * be // 2, tau * be // 2)
        if t:
            line += ";  tau=%d: %d;  tau=%d without the beta term: %d" % (
                t, B2 - t + al // 4 + 1 + t * be // 2, t, B2 - t + al // 4 + 1)
        print(line)

    print("6. CS-512 with all 2^512 binary polynomials: weight ~ Binomial(512, 1/2), same model at each weight")
    P, b = SETS["CS-512"], base["CS-512"]
    mass = em = ei = 0.0
    for w in range(176, 337):
        pr = comb(P["n"], w) / 2 ** P["n"]
        r = repetitions(P, tau=w)
        mass, em, ei = mass + pr, em + pr * r["M"], ei + pr * r["iters"]
    print("   weights 176..336 carry %.9f of the mass:  mean M x%.3f  mean iterations/signature x%.3f"
          % (mass, em / mass / b["M"], ei / mass / b["iters"]))


if __name__ == "__main__":
    main()
