#!/usr/bin/env python3
"""Arithmetic behind sign-15/fixing_cost.md (MORNING-ATLAS).  Stdlib only.

Prints sizes, challenge-space sizes, rejection probabilities and work counts
for the submitted parameter sets and for the values costed in the note.  It
implements no part of the scheme and recommends no parameters.

Sources: specification Table 1 (physical p. 5), Table 2 (p. 22), Table 3
(p. 23), Events 1-5 and equations (4)-(14) (pp. 17-20), zeta (p. 25), the
384-bit set (p. 48); reference code SIG_lwrdsa*.c, poly.c, packing.c, params.h.

Models: a coefficient of c*s1 is normal with variance kappa*E[s^2], except in
tail_cut(), which is exact; a coefficient of c*s2 is normal with variance
kappa/12 (p. 18); a coefficient of c*t0 is normal with variance
kappa*2^(2*pkdrop)/12 (p. 19).

Run: python3 -B sign-15/fixing_cost_params.py
"""
from math import ceil, comb, erfc, exp, log2, pi, sqrt

# n, log p, k, l, eta, kappa, beta1, beta2, log gamma1, log gamma2bar,
# pkdrop, omega (Table 2), and the Table 1 sizes (pk, sk, sig).
SETS = {
    "ATLAS-128": (128, 18, 9, 6, 16, 31, 51, 8, 19, 13, 10, 128, (1328, 2128, 2081)),
    "ATLAS-192": (128, 19, 13, 10, 8, 69, 33, 12, 19, 14, 10, 128, (2112, 3152, 3365)),
    "ATLAS-256": (256, 20, 8, 7, 4, 60, 73, 12, 19, 15, 10, 128, (2848, 4016, 4656)),
    "ATLAS-512": (512, 22, 8, 7, 4, 60, 73, 12, 21, 17, 10, 128, (6688, 7920, 10081)),
}


def phi(x):
    return exp(-x * x / 2) / sqrt(2 * pi)


def Phi(x):
    return 0.5 * erfc(-x / sqrt(2))


def two_sided_tail(x):
    return erfc(x / sqrt(2))


def second_moment(lo, hi):
    return sum(v * v for v in range(lo, hi + 1)) / (hi - lo + 1)


def sizes(n, logp, k, l, lg1, d, omega, sign_bytes=8):
    pk = n // 8 * k * (logp - d + 1) + 32
    sk = 112 + n // 8 * (6 * l + d * k)
    hint = omega if n <= 256 else omega * 9 // 8
    sig = n // 8 * l * (lg1 + 1) + hint + k + n // 8 + sign_bytes
    return pk, sk, sig


def challenge_bits(n, kappa):
    return log2(comb(n, kappa)) + kappa


def tail_cut(eta, kappa, log2_tail):
    """Smallest b with P[|sum of kappa uniform[-eta,eta] values| > b] <=
    2^-log2_tail, by exact convolution (the signs of c do not matter)."""
    w, dist = 2 * eta + 1, [1]
    for _ in range(kappa):
        new = [0] * (len(dist) + w - 1)
        for i, v in enumerate(dist):
            for j in range(w):
                new[i + j] += v
        dist = new
    top, above, total = kappa * eta, 0, w ** kappa
    for v in range(top, -1, -1):
        nxt = above + dist[v + top]          # number of outcomes >= v
        if 2 * nxt * 2 ** log2_tail > total:
            return v
        above = nxt
    return 0


def excluded_single(b1, sigma, bounded):
    """Mean number of mask values excluded per coefficient, masks independent.

    With c*s1 = v, the accepted mask values lie in the support
    [-(g1-1), g1-1] and in the window |y+v| < g1-b1; the number excluded is
    max(b1-v,0) + max(b1+v,0), which is 2*b1 whenever |v| <= b1.
    """
    if bounded:
        return 2.0 * b1
    a = b1 / sigma
    return 2 * (b1 * Phi(a) + sigma * phi(a))


def excluded_pair(b1, sigma, rho):
    """Same for two coefficients that share one mask value, as the submitted
    rej_gamma1m1 produces for gamma1 = 2^19: max(b1-min,0) + max(b1+max,0).

    The two c*s1 values are a common normal part of variance rho*sigma^2 plus
    independent parts.  rho = E[s]^2 / E[s^2]: a nonzero mean of s gives
    neighbouring coefficients of c*s1 a common term (ATLAS-128 code only).
    """
    sa, sb = sigma * sqrt(rho), sigma * sqrt(1 - rho)
    steps = 4000
    hm = 16 * sb / steps
    grid = [(-8 * sb + i * hm) for i in range(steps + 1)]
    dens = [2 * phi(x / sb) * Phi(x / sb) / sb * hm for x in grid]   # max of two

    def f(shift):                            # E[max(shift + max, 0)]
        return sum(w * (shift + x) for x, w in zip(grid, dens) if shift + x > 0)

    if sa == 0:
        return 2 * f(b1)
    outer = 200
    ha = 16 * sa / outer
    total = 0.0
    for i in range(outer + 1):
        a = -8 * sa + i * ha
        total += phi(a / sa) / sa * ha * f(b1 + a)
    return 2 * total


def event1(n, l, lg1, b1, sigma, bounded, paired, rho=0.0):
    span = 2 * (1 << lg1) - 1
    if paired:
        return (1 - excluded_pair(b1, sigma, rho) / span) ** (n * l // 2)
    return (1 - excluded_single(b1, sigma, bounded) / span) ** (n * l)


def event2(n, k, lg2b, b2):
    g2b = 1 << lg2b
    return ((2 * (g2b - b2) - 1) / (2 * g2b)) ** (n * k)


def event3_count(n, k, kappa, b2):
    return n * k * two_sided_tail((b2 - 0.5) / sqrt(kappa / 12))


def work(n, k, l, p1, p2):
    """Expected attempts, polynomial multiplications and XOF output bytes of
    one signature, counted from sig_sign and the samplers it calls."""
    attempts = 1 / (p1 * p2)
    mul = k * l + attempts * (k * l + l + p1 * k + p1 * p2 * k)
    mat = (15 if n == 512 else 5) * 168          # expand_mat buffer per entry
    mask = (8 if n == 512 else 1) * 6 * 136      # mask buffers per polynomial
    xof = k * l * mat + attempts * (l * mask + 136)
    return attempts, mul, xof


def row(tag, name, kappa, b1, b2, s2m, bounded=False, paired=False, rho=0.0):
    n, lp, k, l, eta, _, _, _, lg1, lg2b, d, om, tab = SETS[name]
    sigma = sqrt(kappa * s2m)
    p1 = event1(n, l, lg1, b1, sigma, bounded, paired, rho)
    p2 = event2(n, k, lg2b, b2)
    a, m, x = work(n, k, l, p1, p2)
    over = "none" if bounded else f"{two_sided_tail(b1 / sigma):.1e}"
    print(f"  {tag:<34} kappa={kappa:<3} beta1={b1:<3} beta2={b2:<2} "
          f"sd(c*s1)={sigma:5.1f} P[|c*s1|>beta1]={over:<7} "
          f"P1={p1:.4f} P2={p2:.4f} attempts={a:6.3f} mul={m:7.1f} xof={x:7.0f}")
    return a, m, x


def ratio(tag, base, new):
    print(f"  -> {tag}: attempts x{new[0] / base[0]:.3f}, "
          f"multiplications x{new[1] / base[1]:.3f}, XOF bytes x{new[2] / base[2]:.3f}")


print("1. Size control: Table 3 formulas against Table 1 (pk, sk, sig)")
for name, (n, lp, k, l, eta, kappa, b1, b2, lg1, lg2b, d, om, tab) in SETS.items():
    print(f"  {name}: formula {sizes(n, lp, k, l, lg1, d, om)}; Table 1 {tab}")
print("  ATLAS-512 signature: 10080 by the formula; 10081 in Table 1 and params.h:165")

print("\n2. Challenge space, log2(C(n,kappa) * 2^kappa), and bytes for kappa sign bits")
for name, n, kappa in (("ATLAS-128", 128, 31), ("ATLAS-192 code", 128, 64),
                       ("ATLAS-192 specification", 128, 69), ("ATLAS-256", 256, 60),
                       ("ATLAS-512", 512, 60), ("ATLAS-512 kappa=117", 512, 117),
                       ("ATLAS-512 kappa=118", 512, 118)):
    print(f"  {name:<24} n={n} kappa={kappa:<3} bits={challenge_bits(n, kappa):7.2f} "
          f"sign bytes={(kappa + 7) // 8}")
print(f"  signature with the wider sign field: ATLAS-192 {3365 + 1} (+{100 / 3365:.2f}%), "
      f"ATLAS-512 {10081 + 7} (+{700 / 10081:.2f}%)")

print("\n3. Event3 (p. 18): expected number of the n*k coefficients of c*s2 with")
print("   |coefficient| >= beta2 - 1/2")
for name, n, k, kappa, b2 in (("ATLAS-128", 128, 9, 31, 8), ("ATLAS-192", 128, 13, 69, 12),
                              ("ATLAS-256", 256, 8, 60, 12), ("ATLAS-512", 512, 8, 60, 12),
                              ("384-bit set (p. 48)", 256, 13, 133, 18)):
    f = event3_count(n, k, kappa, b2)
    print(f"  {name:<22} kappa={kappa:<3} beta2={b2:<2} {f:.2e} = 2^{log2(f):.1f}")
base3 = event3_count(512, 8, 60, 12)
for b2 in range(12, 19):
    f = event3_count(512, 8, 118, b2)
    mark = "  <= submitted ATLAS-512 value" if f <= base3 else ""
    mark += ", <= 2^-10" if f <= 2 ** -10 else ""
    print(f"  ATLAS-512 kappa=118    beta2={b2} {f:.2e} = 2^{log2(f):.1f}{mark}")

print("\n4. Expected signing attempts by the specification's formula (14)")
for name, (n, lp, k, l, eta, kappa, b1, b2, lg1, lg2b, d, om, tab) in SETS.items():
    e1, e2 = n * b1 * l / (1 << lg1), n * b2 * k / (1 << lg2b)
    print(f"  {name}: n*beta1*l/gamma1={e1:.4f} n*beta2*k/gamma2bar={e2:.4f} "
          f"attempts=exp({e1 + e2:.4f})={exp(e1 + e2):.3f}")

print("\n5. Submitted code against the costed values (exact counts of accepted values)")
spec = {e: second_moment(-e, e) for e in (4, 8, 16)}
code128 = second_moment(1, 16)               # rej_eta, recorded as sign-15-16
code192 = second_moment(-7, 8)
rho128 = 8.5 ** 2 / code128                  # mean of [1,16] is 8.5
rho192 = 0.5 ** 2 / code192                  # mean of [-7,8] is 0.5
# beta1 that no coefficient of c*s1 is expected to exceed in 2^80 signatures
cut128 = tail_cut(16, 31, 80 + 10)           # n*l = 768 <= 2^10
cut192 = tail_cut(8, 69, 80 + 11)            # n*l = 1280 <= 2^11
print(f"  tail cut: ATLAS-128 beta1={cut128} (kappa*eta=496); "
      f"ATLAS-192 beta1={cut192} (kappa*eta=552)")

print(" ATLAS-128")
b = row("submitted code (paired masks)", "ATLAS-128", 31, 51, 8, code128, paired=True, rho=rho128)
f = row("mask decoder corrected", "ATLAS-128", 31, 51, 8, spec[16])
t = row("and beta1 at the tail cut", "ATLAS-128", 31, cut128, 8, spec[16], bounded=True)
r = row("and beta1 = kappa*eta", "ATLAS-128", 31, 496, 8, spec[16], bounded=True)
ratio("decoder only", b, f)
ratio("decoder and beta1 (tail cut)", b, t)
ratio("decoder and beta1 (kappa*eta)", b, r)

print(" ATLAS-192")
b = row("submitted code (paired, kappa=64)", "ATLAS-192", 64, 33, 12, code192, paired=True, rho=rho192)
f = row("decoder corrected, kappa=69", "ATLAS-192", 69, 33, 12, spec[8])
t = row("and beta1 at the tail cut", "ATLAS-192", 69, cut192, 12, spec[8], bounded=True)
r = row("and beta1 = kappa*eta", "ATLAS-192", 69, 552, 12, spec[8], bounded=True)
ratio("decoder and kappa", b, f)
ratio("decoder, kappa and beta1 (tail cut)", b, t)
ratio("decoder, kappa and beta1 (kappa*eta)", b, r)

print(" ATLAS-256")
b = row("submitted code (paired masks)", "ATLAS-256", 60, 73, 12, spec[4], paired=True)
f = row("mask decoder corrected", "ATLAS-256", 60, 73, 12, spec[4])
ratio("decoder only", b, f)

print(" ATLAS-512")
b1s = ceil(73 * sqrt(118 / 60))
b = row("submitted code", "ATLAS-512", 60, 73, 12, spec[4])
f = row("kappa=118, beta2=17", "ATLAS-512", 118, 73, 17, spec[4])
g = row(f"kappa=118, beta2=17, beta1={b1s}", "ATLAS-512", 118, b1s, 17, spec[4])
h = row("kappa=118, beta2=17, beta1=472", "ATLAS-512", 118, 472, 17, spec[4], bounded=True)
ratio("kappa and beta2", b, f)
ratio("kappa, beta2 and beta1 scaled by sqrt(118/60)", b, g)
ratio("kappa, beta2 and beta1 = kappa*eta", b, h)

print("\n6. A larger gamma1 with beta1 = kappa*eta: signature bytes against Event1")
print("   (all other values held; q, zeta and zeta' are not re-derived)")
for name, kappa, paired_kappa, paired_s2m, b1_sub, rho in (
        ("ATLAS-128", 31, 31, code128, 51, rho128), ("ATLAS-192", 69, 64, code192, 33, rho192)):
    n, lp, k, l, eta, _, _, _, lg1, lg2b, d, om, tab = SETS[name]
    p1_sub = event1(n, l, 19, b1_sub, sqrt(paired_kappa * paired_s2m), False, True, rho)
    for lg in (19, 20, 21, 22):
        p1 = event1(n, l, lg, kappa * eta, 1.0, True, False)
        extra = (lg - 19) * n * l // 8
        print(f"  {name} gamma1=2^{lg}: signature +{extra} bytes (+{100 * extra / tab[2]:.1f}%), "
              f"attempts x{p1_sub / p1:.3f}, zeta'=2*gamma1+2={2 * (1 << lg) + 2}, q/2={1 << 22}")

print("\n7. ATLAS-512 at kappa = 118: other conditions that contain kappa")
for kappa in (60, 118):
    zeta = max((1 << 21) - 73, 8 * (2 * (1 << 17) + 1 + kappa * 512))
    hw = 512 * 8 * 1024 * sqrt(kappa / 12) / (sqrt(2 * pi) * ((1 << 17) - 12))
    print(f"  kappa={kappa}: kappa*2^(pkdrop-1)={kappa * 512} against gamma2bar={1 << 17}; "
          f"zeta (p. 25)={zeta} against 4*gamma2={4 << 20}; "
          f"expected hint weight (eq. 11)={hw:.1f} against omega=128")
print(f"  control, 384-bit set (p. 48): zeta={16 * (2 * (1 << 16) + 1 + 133 * 512)} "
      "(stated 3186704)")

print("\n8. Secret key with wider secret and hash values (not transmitted)")
print(f"  ATLAS-256, tr 48 -> 64 bytes: {4016 + 16} (+{1600 / 4016:.1f}%)")
print(f"  ATLAS-512, tr 48 -> 128 and K 32 -> 64 bytes: {7920 + 80 + 32} (+{11200 / 7920:.1f}%)")
