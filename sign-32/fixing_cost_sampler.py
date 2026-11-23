#!/usr/bin/env python3
"""Fixing-cost estimate for sign-32 (UVW), finding sign-32-3.  Stdlib only.

The costed repair is the rejection sampling of Wave (reference [5] of the
specification): the signer's output is made uniform over the words of
weight w, so that it no longer depends on the secret pairing.  This script
does not implement a sampler.  It computes

  1. the size control (specification p. 11 formula against Table 1);
  2. the submitted weight-loop success rate (Theorem 2, p. 19);
  3. a gap g between k1 and the forced positions, taken as the smallest g
     with 3^-g <= 2^-lambda, moved from k2 to k1 with (n, k, w) unchanged;
  4. the distribution of |e2| and of the zero pairs for a uniform word of
     weight w (Proposition 3, p. 23) and for the two Prange decoders;
  5. the acceptance rate of the two rejection tests when the two internal
     distributions are discretised Gaussians fitted by a small search;
  6. the ratio of matrix-inversion work, repaired over submitted.  A new
     information set, hence a new inversion, is counted for every attempt;
     a stage-1 rejection repeats DecZ and a stage-2 rejection repeats DecXY.

Every value under 3, 5 and 6 is a costing aid, not a recommended parameter.

Run:  python3 -B sign-32/fixing_cost_sampler.py          (about one minute)
"""

from math import ceil, exp, lgamma, log, log2, pi, sqrt

LN2 = log(2.0)
LN3 = log(3.0)
NEG = float("-inf")

# Specification Table 1 (physical p. 15): (lambda, n, k, k1, k2, w, |sig| bytes)
SETS = {
    "UVW128": (128, 9700, 4850, 3250, 1600, 8633, 977),
    "UVW256": (256, 19200, 9600, 6433, 3167, 17088, 1934),
    "UVW512": (512, 39000, 19500, 13066, 6434, 34710, 3928),
}


def lc(n, k):
    if k < 0 or k > n:
        return NEG
    return lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)


def lse(xs):
    m = max(xs)
    if m == NEG:
        return NEG
    return m + log(sum(exp(x - m) for x in xs))


def binom_logpmf(n, p, x):
    if x < 0 or x > n:
        return NEG
    return lc(n, x) + x * log(p) + (n - x) * log(1.0 - p)


def moments(lo, probs):
    s = sum(probs)
    mean = sum((lo + i) * p for i, p in enumerate(probs)) / s
    var = sum((lo + i - mean) ** 2 * p for i, p in enumerate(probs)) / s
    return s, mean, var


def gauss_weights(lo, hi, mu, var):
    ws = [exp(-((x - mu) ** 2) / (2.0 * var)) for x in range(lo, hi + 1)]
    s = sum(ws)
    return [x / s for x in ws]


def window(lo, probs, lam):
    """Central interval outside which each tail of the target is below 2^-lam."""
    eps = 2.0 ** (-lam)
    a, acc = 0, 0.0
    while acc + probs[a] < eps:
        acc += probs[a]
        a += 1
    b, acc = len(probs) - 1, 0.0
    while acc + probs[b] < eps:
        acc += probs[b]
        b -= 1
    return lo + a, lo + b


def target_e2_weight(n, w, t):
    """Proposition 3: log P(|v - u| = t) for (u||v) uniform of weight w."""
    h = n // 2
    terms = []
    i0 = max(0, 2 * t - w)
    if (i0 + w) % 2:
        i0 += 1
    for i in range(i0, min(t, n - w) + 1, 2):
        terms.append(lc(h, i) + lc(h - i, t - i) + lc(h - t, (w + i) // 2 - t)
                     + ((w + 3 * i) // 2) * LN2)
    if not terms:
        return NEG
    return lse(terms) - lc(n, w) - w * LN2


def best_acceptance(target, tlo, cols, clo, mu0, var0, lam):
    """Search a discretised Gaussian mixing law over the columns `cols`
    (cols[c][x - tlo] = decoder probability of x given parameter clo + c) that
    maximises min over the window of mixture/target."""
    wlo, whi = window(tlo, target, lam)
    idx = range(wlo - tlo, whi - tlo + 1)
    best = (0.0, None, None)
    chi = clo + len(cols) - 1
    for f in (1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.8, 2.0, 2.3, 2.6, 3.0):
        var = max(var0, 1.0) * f
        for dmu in (-2.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0):
            ws = gauss_weights(clo, chi, mu0 + dmu * sqrt(var) / 4.0, var)
            alpha = 1e9
            for x in idx:
                p = 0.0
                for c, wgt in enumerate(ws):
                    if wgt > 1e-30:
                        p += wgt * cols[c][x]
                r = p / target[x]
                if r < alpha:
                    alpha = r
                    if alpha <= best[0]:
                        break
            if alpha > best[0]:
                best = (alpha, mu0 + dmu * sqrt(var) / 4.0, var)
    return best, (wlo, whi)


def analyse(name, lam, n, k, k1, k2, w, sig_stated):
    h = n // 2
    print("== %s  (n, k, k1, k2, w) = (%d, %d, %d, %d, %d)" % (name, n, k, k1, k2, w))

    # 1. size control
    bits = lam + k * log2(3.0)
    print(" 1. |sig| = lambda + k*log2(3) = %.1f bits = %d bytes; Table 1 states %d."
          % (bits, ceil(bits / 8), sig_stated))
    print("    The repair keeps n and k, so the signature and the public key keep their sizes.")

    # 2. Theorem 2
    psucc = exp(lc(n - 2 * k1, w - 2 * k1) + (w - 2 * k1) * LN2 - (n - 2 * k1) * LN3)
    print(" 2. Theorem 2: Psucc = %.4f, 1/Psucc = %.1f iterations of the weight loop." % (psucc, 1 / psucc))

    # 3. gap
    g = ceil(lam / log2(3.0))
    k1g, k2g = k1 + g, k2 - g
    mz, mxy = h - k2, h - k1
    mzg, mxyg = h - k2g, h - k1g
    print(" 3. gap g = %d: (k1, k2) -> (%d, %d); forced positions stay %d, so the expected"
          % (g, k1g, k2g, k1g - g))
    print("    weight (2/3)(n + k1 - g) stays %.1f.  Inverted matrices: %d -> %d and %d -> %d."
          % (2.0 * (n + k1g - g) / 3.0, mz, mzg, mxy, mxyg))

    # 4. stage 1: |e2|
    rho = w / n
    mu_t = h * (1.0 - (1.0 - rho) ** 2 - rho * rho / 2.0)
    sd_guess = sqrt(h * 0.25)
    tlo, thi = int(mu_t - 16 * sd_guess), int(mu_t + 16 * sd_guess)
    q1 = [exp(target_e2_weight(n, w, t)) for t in range(tlo, thi + 1)]
    s1, m1, v1 = moments(tlo, q1)
    kvar = mzg * 2.0 / 9.0
    print(" 4. uniform word of weight w: |e2| has mean %.1f, variance %.1f (mass in band %.6f)."
          % (m1, v1, s1))
    print("    DecZ with t nonzero information-set positions: |e2| = t + Bin(%d, 2/3), variance %.1f;"
          % (mzg, kvar))
    t_mean = m1 - mzg * 2.0 / 3.0
    print("    matching the mean needs t = %.1f, inside [0, %d]." % (t_mean, k2g))

    # 5a. stage-1 acceptance
    sd_t = sqrt(max(v1 - kvar, 1.0))
    clo, chi = max(0, int(t_mean - 14 * sd_t * 1.8)), min(k2g, int(t_mean + 14 * sd_t * 1.8))
    kern = {}
    for d in range(tlo - chi, thi - clo + 1):
        kern[d] = exp(binom_logpmf(mzg, 2.0 / 3.0, d))
    cols = [[kern[x - c] for x in range(tlo, thi + 1)] for c in range(clo, chi + 1)]
    (a1, mu_d2, var_d2), win1 = best_acceptance(q1, tlo, cols, clo, t_mean, v1 - kvar, lam)
    print(" 5. stage 1 (weight of e2): D2 ~ discretised Gaussian(mean %.1f, variance %.1f) gives"
          % (mu_d2, var_d2))
    print("    acceptance >= %.4f on the window %s (target tails below 2^-%d)." % (a1, win1, lam))

    # 5b. stage 2: zero pairs given |e2| = t2
    c = n - w                      # 2*Z0 + Z1 = n - w
    free = h - (k1g - g)           # positions of e1 that are not forced
    worst2, rows = 1.0, []
    for dt in (-3.0, 0.0, 3.0):
        t2 = int(round(m1 + dt * sqrt(v1)))
        zlo, zhi = max(0, (c - t2 + 1) // 2), c // 2
        lq = [lc(h - t2, z) - z * LN2 + lc(t2, c - 2 * z) + (c - 2 * z) * LN2 for z in range(zlo, zhi + 1)]
        nrm = lse(lq)
        q2 = [exp(x - nrm) for x in lq]
        s2, mz0, vz0 = moments(zlo, q2)
        # keep the band where the target is not negligible
        blo = max(zlo, int(mz0 - 16 * sqrt(vz0)))
        bhi = min(zhi, int(mz0 + 16 * sqrt(vz0)))
        q2b = q2[blo - zlo: bhi - zlo + 1]
        j_mean = (c - 2 * mz0) * 1.5           # E[Z1] = (2/3) j
        jlo, jhi = max(0, t2 - (k1g - g), int(j_mean - 400)), min(free, t2, int(j_mean + 400))
        cols2, succ = [], {}
        for j in range(jlo, jhi + 1):
            lp = [binom_logpmf(free - j, 1.0 / 3.0, z) + binom_logpmf(j, 2.0 / 3.0, c - 2 * z)
                  for z in range(blo, bhi + 1)]
            tot = lse(lp)
            succ[j] = exp(tot)
            cols2.append([exp(x - tot) for x in lp])
        # variance of the decoder at the mean j, to seed the search
        jm = min(max(int(round(j_mean)), jlo), jhi)
        _, dm, dv = moments(blo, cols2[jm - jlo])
        var_j0 = max(vz0 - dv, 0.5) * 9.0
        (a2, mu_j, var_j), win2 = best_acceptance(q2b, blo, cols2, jlo, j_mean, var_j0, lam)
        rows.append((t2, mz0, vz0, dv, mu_j, var_j, a2, 1.0 / succ[jm]))
        worst2 = min(worst2, a2)
    for (t2, mz0, vz0, dv, mu_j, var_j, a2, loops) in rows:
        print("    stage 2 at |e2| = %d: zero pairs have mean %.1f, variance %.1f (decoder at fixed split %.1f);"
              % (t2, mz0, vz0, dv))
        print("      split ~ Gaussian(mean %.1f, variance %.1f) gives acceptance >= %.4f;"
              " weight loop 1/P = %.1f." % (mu_j, var_j, a2, loops))

    # 6. inversion work
    base = mz ** 3 + mxy ** 3
    gap_only = (mzg ** 3 + mxyg ** 3) / base - 1.0
    rej_only = (mz ** 3 / a1 + mxy ** 3 / worst2) / base - 1.0
    both = (mzg ** 3 / a1 + mxyg ** 3 / worst2) / base - 1.0
    print(" 6. inversion work m^3 (DecZ share %.1f%% as submitted):" % (100.0 * mz ** 3 / base))
    restart = (mzg ** 3 + mxyg ** 3) / (a1 * worst2) / base - 1.0
    print("    gap only %+.1f%%; rejection only (g = 0) %+.1f%%; gap and rejection %+.1f%%."
          % (100 * gap_only, 100 * rej_only, 100 * both))
    print("    if a stage-2 rejection also repeated DecZ: %+.1f%%." % (100 * restart))
    loop = (1 / psucc) * ((k1 * mxy) + (2.0 / 3.0) * mxy * mxy)
    print("    weight-loop products as submitted are %.2f%% of the inversion count (4/3)m^3/0.56."
          % (100.0 * loop / ((4.0 / 3.0) * base / 0.5601)))
    print()


def main():
    for name, (lam, n, k, k1, k2, w, sig) in SETS.items():
        analyse(name, lam, n, k, k1, k2, w, sig)


if __name__ == "__main__":
    main()
