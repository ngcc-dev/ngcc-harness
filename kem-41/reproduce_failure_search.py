#!/usr/bin/env python3
"""kem-41-4: recompute the posted weak-key failure-search estimate for ZEN-512.

Chenyi Li's PKC Forum post of 2026-11-26 estimates the cost of finding one
decryption failure of ZEN-512 among 2^80 public keys. This script recomputes
the post's figures with its own block enumeration and convolutions. It was
written from the post's note and uses no code from the post's archive, which
was not downloaded. It needs NumPy and about 1 GB of memory, runs in about
two minutes, and implements no part of the scheme.

Model, all from the specification (physical pages):
  - ZEN-512 has n = 2048, q = 769, f, g with coefficients -1, 0, 1 of
    probabilities 3/32, 13/16, 3/32, s, e with 1/8, 3/4, 1/8, and ciphertext
    compression 769 -> 256 (Tables 2-3, pp. 24-25) with half-up rounding
    (section 2.3, p. 8).
  - Decryption works on a = (x^(n/2) + 1) f c', which equals
    tau (g s + f U) + kappa f m over the integers before reduction mod q,
    with U = e + compression error, and, in blocks of four coefficients
    modulo Y^4 + 1 (Y = x^(n/4)), tau = 1 + Y^2 and kappa = Y^2 + Y^3
    (section 5.3, pp. 17-20; Lemma 1 writes the last term with the opposite
    sign, which the symmetry of f makes immaterial here).
  - The failure event is -Z[r] - Z[r + n/4] >= q for one group r, one of the
    projected events of the specification's own bound (pp. 21-22).
  - Compression errors are independent with the law of a uniform input. This
    is the post's assumption and the usual one; it is not verified here.

Search of the post: 212 message bits are fixed to one and 300 are uniform. A
block of f is favourable if f2 + 2 f3 + f4 >= 3, and a key is weak if at least
34 of the 212 blocks paired with the fixed bits are favourable. A candidate is
kept if N_r >= 2172 and H >= 16336, two squared norms the encryptor can
compute. Only a shift of 3 per favourable block is credited.

The post replaces the g s term by a Gaussian given N_r. The script computes
that variant, to compare with the post, and the variant with the exact law of
the term. It also evaluates the same search on a key drawn at random.
"""
import itertools
import math
import sys
from fractions import Fraction

import numpy as np

sys.dont_write_bytecode = True

REPORT_ID = "kem-41-4"
Q, NU, MC = 769, 512, 256            # modulus, groups n/4, compressed alphabet
PREFIX, B_MIN, B_MAX = 212, 34, 63   # fixed-one bits; favourable counts retained by the post
R0, H0 = 2172, 16336                 # the post's thresholds for N_r and H
POOL = QUERIES = 80                  # log2 of the keys and of the decapsulations per key
TAU, KAPPA = (1, 0, 1, 0), (0, 0, 1, 1)
NX, NH, NGX, NGR = 1024, 16384, 1024, 8192
LN2 = math.log(2)

POSTED = {  # the post's table
    "-log2 p1": 3.470474, "-log2 p2": 191.011512, "-log2 alpha": 194.481986,
    "-log2 beta": 79.998988, "per key": 274.480974, "pool": 354.480974,
}
OURS = {  # this script's own figures, quoted in the report
    "log2 P[weak]": -78.5305, "-log2 beta, exact g s term": 79.845708,
    "log2 beta, random key": -123.8702,
    "-log2 alpha, H >= 26000": 843.01, "log2 beta, random key, H >= 26000": -83.679,
}


def mul(a, b):
    """Product in Z[Y]/(Y^4 + 1)."""
    r = [0, 0, 0, 0]
    for i in range(4):
        for j in range(4):
            if i + j < 4:
                r[i + j] += a[i] * b[j]
            else:
                r[i + j - 4] -= a[i] * b[j]
    return r


UNIT = [tuple(int(i == j) for j in range(4)) for i in range(4)]


def direction(w):
    """The vector a with -(f w)[0] - (f w)[1] = a . f."""
    return tuple(-z[0] - z[1] for z in (mul(e, w) for e in UNIT))


def ternary(num, den):
    return {-1: Fraction(num, den), 0: Fraction(den - 2 * num, den), 1: Fraction(num, den)}


def compression_error():
    """Law of Decompress(Compress(c)) - c for uniform c, half-up rounding."""
    law = {}
    for c in range(Q):
        u = ((2 * c * MC + Q) // (2 * Q)) % MC
        d = ((2 * u * Q + MC) // (2 * MC)) % Q
        e = (d - c) % Q
        law[e - Q if e > Q // 2 else e] = law.get(e - Q if e > Q // 2 else e, 0) + Fraction(1, Q)
    return law


def block_laws():
    """Single-block laws of (projection, squared norm), exact, then as floats."""
    f_law, s_law = ternary(3, 32), ternary(1, 8)
    u_law = {}
    for a, p in s_law.items():
        for b, r in compression_error().items():
            u_law[a + b] = u_law.get(a + b, 0) + p * r
    blocks = [(f, math.prod(f_law[c] for c in f)) for f in itertools.product((-1, 0, 1), repeat=4)]
    fav = [(f, p) for f, p in blocks if f[1] + 2 * f[2] + f[3] >= 3]
    non = [(f, p) for f, p in blocks if f[1] + 2 * f[2] + f[3] < 3]
    p_fav = sum(p for _, p in fav)
    laws = {k: {} for k in ("fav", "non", "any1", "rnd", "g", "h0", "h1", "nr")}

    def add(name, key, p):
        laws[name][key] = laws[name].get(key, 0) + p

    def dot(a, f):
        return sum(a[i] * f[i] for i in range(4))

    for u in itertools.product(sorted(u_law), repeat=4):
        pu = math.prod(u_law[c] for c in u)
        tu = mul(TAU, u)
        a0 = direction(tu)                                       # message bit 0
        a1 = direction([tu[i] + KAPPA[i] for i in range(4)])     # message bit 1
        n0, n1 = sum(c * c for c in a0), sum(c * c for c in a1)
        add("h0", n0, pu)
        add("h1", n1, pu)
        for f, pf in fav:    # the shift kappa f is credited separately, as 3
            add("fav", (dot(a0, f), n1), pu * pf / p_fav)
        for f, pf in non:
            add("non", (dot(a1, f), n1), pu * pf / (1 - p_fav))
        for f, pf in blocks:
            add("any1", (dot(a1, f), n1), pu * pf)
            add("rnd", (dot(a0, f), n0), pu * pf / 2)
            add("rnd", (dot(a1, f), n1), pu * pf / 2)
    for s in itertools.product((-1, 0, 1), repeat=4):
        ps = math.prod(s_law[c] for c in s)
        a = direction(mul(TAU, s))
        n = sum(c * c for c in a)
        add("nr", n, ps)
        for g, pg in blocks:
            add("g", (dot(a, g), n), ps * pg)
    floats = {k: {key: float(p) for key, p in d.items()} for k, d in laws.items()}
    return floats, p_fav, u_law


def moments(law, tx, th):
    """log MGF, mean and covariance of a two-dimensional law tilted by (tx, th)."""
    keys = np.array(list(law.keys()), dtype=float)
    w = np.array(list(law.values())) * np.exp(tx * keys[:, 0] + th * keys[:, 1])
    m = w.sum()
    w /= m
    mean = w @ keys
    c = keys - mean
    return math.log(m), mean, (c * w[:, None]).T @ c


def tilted_spectrum(law, tx, th, nx, nh):
    arr = np.zeros((nx, nh))
    for (x, h), p in law.items():
        arr[x % nx, h % nh] += p * math.exp(tx * x + th * h)
    return np.fft.rfft2(arr / arr.sum())


def unwrap(n, centre):
    """Representatives of the residues 0..n-1 nearest to centre."""
    i = np.arange(n)
    return i + n * np.round((centre - i) / n)


def direct_power(law, n):
    """n-fold convolution of a one-dimensional law, without FFT."""
    base = np.zeros(max(law) + 1)
    for x, p in law.items():
        base[x] = p
    out = np.ones(1)
    for bit in bin(n)[2:]:
        out = np.convolve(out, out)
        if bit == "1":
            out = np.convolve(out, base)
    return out


def log2_tail(parts, threshold, n=1 << 15):
    """log2 P[sum >= threshold] for independent one-dimensional parts, by a tilted FFT."""
    two_d = [({(0, x): p for x, p in law.items()}, pw) for law, pw in parts]
    th = 0.0
    for _ in range(1000):
        stats = [(moments(law, 0.0, th), pw) for law, pw in two_d]
        mean = sum(pw * m[1][1] for m, pw in stats)
        var = sum(pw * m[2][1, 1] for m, pw in stats)
        step = (threshold - mean) / var
        th += max(-0.004, min(0.004, step))
        if abs(step) < 1e-14:
            break
    spec = 1.0
    for law, pw in two_d:
        spec = spec * tilted_spectrum(law, 0.0, th, 1, n)[0] ** pw
    out = np.fft.irfft(spec, n)
    x = unwrap(n, mean)
    sel = x >= threshold
    logm = sum(pw * m[0] for m, pw in stats)
    return (logm - th * threshold + math.log((out[sel] * np.exp(-th * (x[sel] - threshold))).sum())) / LN2


class Search:
    """P[X + G >= thr, H >= h0, N_r >= r0] for block parts of (X, H) and the law of (G, N_r)."""

    def __init__(self, g_law, nr_power):
        self.g = g_law
        self.nr = nr_power
        self.spectra = {}

    def tilt(self, parts, thr, r0, h0):
        """Tilt (tx, th, tr) that centres the three constraints (damped Newton)."""
        tx, th, tr = 0.15, 0.03, 0.0
        for _ in range(2000):
            mean, cov = np.zeros(2), np.zeros((2, 2))
            for law, pw in parts:
                _, mu, c = moments(law, tx, th)
                mean += pw * mu
                cov += pw * c
            _, mug, covg = moments(self.g, tx, tr)
            mug, covg = NU * mug, NU * covg
            res = np.array([thr - mean[0] - mug[0], h0 - mean[1], r0 - mug[1]])
            jac = np.array([[cov[0, 0] + covg[0, 0], cov[0, 1], covg[0, 1]],
                            [cov[1, 0], cov[1, 1], 0.0],
                            [covg[1, 0], 0.0, covg[1, 1]]])
            if tr <= 0.0 and res[2] < 0:      # the N_r constraint is slack
                step = np.append(np.linalg.solve(jac[:2, :2], res[:2]), 0.0)
            else:
                step = np.linalg.solve(jac, res)
            step = np.clip(0.5 * step, -np.array([0.02, 0.004, 0.004]), [0.02, 0.004, 0.004])
            tx, th, tr = tx + step[0], th + step[1], max(0.0, tr + step[2])
            if np.abs(step).max() < 1e-13:
                return tx, th, tr
        raise RuntimeError("tilt did not converge")

    def spectrum(self, law, tilt, shape):
        key = (id(law), tilt, shape)
        if key not in self.spectra:
            self.spectra[key] = tilted_spectrum(law, *tilt, *shape)
        return self.spectra[key]

    def log2_prob(self, parts, thr, r0, h0, tilt):
        """(log2 P with G given N_r Gaussian, log2 P with the exact law of (G, N_r))."""
        tx, th, tr = tilt
        spec, logm, mean = 1.0, 0.0, np.zeros(2)
        for law, pw in parts:
            lm, mu, _ = moments(law, tx, th)
            logm += pw * lm
            mean += pw * mu
            spec = spec * self.spectrum(law, (tx, th), (NX, NH)) ** pw
        out = np.fft.irfft2(spec, s=(NX, NH))
        h, x = unwrap(NH, mean[1]), unwrap(NX, mean[0])
        sel = h >= h0
        fx = out[:, sel] @ np.exp(-th * (h[sel] - h0))    # tilted law of X on H >= h0
        logm -= th * h0 + tx * thr
        # G given N_r = rho taken as N(0, 3 rho / 16), the post's approximation
        big = fx > fx.max() * 1e-13
        rho = np.arange(r0, len(self.nr))
        rho = rho[self.nr[rho] > self.nr[rho].max() * 1e-30]
        z = (thr - x[big])[None, :] / np.sqrt(3 * rho / 16)[:, None] / math.sqrt(2)
        tails = 0.5 * np.frompyfunc(math.erfc, 1, 1)(z).astype(float)
        gauss = float(self.nr[rho] @ (tails @ (fx[big] * np.exp(-tx * (x[big] - thr)))))
        # exact law of (G, N_r)
        lg, xg, e_g = self.g_part(tx, tr, r0)
        ox = np.argsort(x)
        conv = np.convolve(fx[ox], e_g)
        s = x[ox][0] + xg[0] + np.arange(len(conv))
        sel = s >= thr
        exact = float((conv[sel] * np.exp(-tx * (s[sel] - thr))).sum())
        return (logm + math.log(gauss)) / LN2, (logm + lg + math.log(exact)) / LN2

    def g_part(self, tx, tr, r0):
        """log MGF factor, sorted support and tilted law of G on N_r >= r0."""
        key = ("g", tx, tr, r0)
        if key not in self.spectra:
            lg, mug, _ = moments(self.g, tx, tr)
            eg = np.fft.irfft2(tilted_spectrum(self.g, tx, tr, NGX, NGR) ** NU, s=(NGX, NGR))
            rho, xg = unwrap(NGR, NU * mug[1]), unwrap(NGX, NU * mug[0])
            sel = rho >= r0
            e_g = eg[:, sel] @ np.exp(-tr * (rho[sel] - r0))
            og = np.argsort(xg)
            self.spectra[key] = (NU * lg - tr * r0, xg[og], e_g[og])
        return self.spectra[key]


def check(name, value, expected, tol, failures):
    ok = abs(value - expected) <= tol
    print(f"  {name}: {value:.6f} (expected {expected}) {'ok' if ok else 'MISMATCH'}")
    if not ok:
        failures.append(name)


def main():
    failures = []
    laws, p_fav, u_law = block_laws()
    print("Block laws")
    counts = {u: int(p * 8 * Q) for u, p in sorted(u_law.items())}
    print(f"  favourable block probability {p_fav}; law of U times {8 * Q}: {counts}")
    if p_fav != Fraction(495, 32768) or counts != {-2: 256, -1: 1792, 0: 2048, 1: 1793, 2: 262, 3: 1}:
        failures.append("block laws")

    binom = [math.comb(PREFIX, k) * p_fav ** k * (1 - p_fav) ** (PREFIX - k) for k in range(PREFIX + 1)]
    weak = sum(binom[B_MIN:])
    weight = [float(b / weak) for b in binom]
    nr = direct_power(laws["nr"], NU)
    nr_tail = np.cumsum(nr[::-1])[::-1]
    mixed = {h: laws["h0"].get(h, 0) / 2 + laws["h1"].get(h, 0) / 2 for h in set(laws["h0"]) | set(laws["h1"])}

    def screen(r0, h0):
        return -math.log2(nr_tail[r0]), -log2_tail([(laws["h1"], PREFIX), (mixed, NU - PREFIX)], h0)

    print("Weak keys and the public screen")
    check("log2 P[weak]", math.log2(weak), OURS["log2 P[weak]"], 5e-4, failures)
    print(f"  expected weak keys among 2^{POOL}: {2.0 ** POOL * float(weak):.2f}")
    l1, l2 = screen(R0, H0)
    check("-log2 p1", l1, POSTED["-log2 p1"], 2e-6, failures)
    check("-log2 p2", l2, POSTED["-log2 p2"], 2e-6, failures)
    check("-log2 alpha", l1 + l2, POSTED["-log2 alpha"], 2e-6, failures)

    search = Search(laws["g"], nr)

    def weak_parts(k):
        return [(laws["fav"], k), (laws["non"], PREFIX - k), (laws["rnd"], NU - PREFIX)]

    tilt = search.tilt(weak_parts(B_MIN), Q - 3 * B_MIN, R0, H0)
    beta = np.zeros(2)
    for k in range(B_MIN, B_MAX + 1):
        lp = search.log2_prob(weak_parts(k), Q - 3 * k, R0, H0, tilt)
        beta += weight[k] * 2.0 ** (np.array(lp) + l1 + l2)
    print("Failure probability of a kept candidate, averaged over weak keys")
    b_gauss, b_exact = -math.log2(beta[0]), -math.log2(beta[1])
    check("-log2 beta", b_gauss, POSTED["-log2 beta"], 2e-6, failures)
    check("-log2 beta, exact g s term", b_exact, OURS["-log2 beta, exact g s term"], 2e-6, failures)
    check("per key", l1 + l2 + b_gauss, POSTED["per key"], 4e-6, failures)
    check("pool", POOL + l1 + l2 + b_gauss, POSTED["pool"], 4e-6, failures)
    print(f"  decapsulations: 2^{QUERIES} per key, 2^{POOL + QUERIES} over the pool")

    print("The same search on a key drawn at random")
    parts = [(laws["any1"], PREFIX), (laws["rnd"], NU - PREFIX)]
    lp = search.log2_prob(parts, Q, R0, H0, search.tilt(parts, Q, R0, H0))[1]
    check("log2 beta, random key", lp + l1 + l2, OURS["log2 beta, random key"], 5e-4, failures)
    s1, s2 = screen(R0, 26000)
    lp = search.log2_prob(parts, Q, R0, 26000, search.tilt(parts, Q, R0, 26000))[1]
    check("-log2 alpha, H >= 26000", s1 + s2, OURS["-log2 alpha, H >= 26000"], 2e-2, failures)
    check("log2 beta, random key, H >= 26000", lp + s1 + s2, OURS["log2 beta, random key, H >= 26000"], 5e-3, failures)

    if failures:
        print(f"FAIL {REPORT_ID}: mismatch in {', '.join(failures)}")
        return 1
    print(f"LEAD {REPORT_ID}: posted ZEN-512 failure-search figures reproduced in the posted model; "
          "no failure produced")
    return 0


if __name__ == "__main__":
    sys.exit(main())
