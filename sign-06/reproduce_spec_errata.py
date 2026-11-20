#!/usr/bin/env python3
"""Certify two errors in COMPASS-SIG's security analysis (sign-06-10, sign-06-11).

sign-06-10: Section 4.1 says the family {H'_A(y) = w1} obtained by applying
Split(., gamma2) to A*y is universal. Split takes x1 = round(p*x/q) mod p with
p = ceil(q/(2*gamma2 + 1)). When p = 2 the high part is even in x, so y and -y
give the same w1 for every A.

sign-06-11: Proposition 1 prints the probability as
1 - 2*Phi(-2a/sigma) - (sigma/a)*(rho(-2a/sigma) - rho(0)). Its proof averages
Pr_x = 1 - Phi((x-a)/sigma) - Phi((-a-x)/sigma) over x uniform on [-a, a];
that average has + in place of the second -, and the printed form exceeds one.
"""

import math
from pathlib import Path
import shutil
import subprocess
import sys


spec = Path(__file__).resolve().with_name("sign-06-spec.pdf")
if shutil.which("pdftotext") is None:
    print("SKIP sign-06-10/11: pdftotext is unavailable")
    sys.exit(77)
text = " ".join(subprocess.check_output(["pdftotext", "-layout", str(spec), "-"], text=True).split())
for phrase in ("is a universal hash family. It directly follows that", "is also universal, with a",
               "x = x1 · 2γ + x0 ,", "Proposition 1.", "it holds that"):
    if phrase not in text:
        raise SystemExit(f"NOT REPRODUCED sign-06-10/11: specification text missing: {phrase!r}")

# Table 1: level, q, n, k, l, eta_s, eta_e, d, gamma1, gamma2, tau, B.
rows = (
    (128, 2081281, 256, 3, 4, 1, 1, 4, 215, 130080, 30, 560),
    (256, 2081281, 256, 7, 7, 1, 1, 5, 217, 520320, 60, 1120),
    (384, 8380417, 512, 5, 6, 1, 1, 5, 218, 1047552, 78, 1760),
    (512, 8380417, 512, 7, 7, 1, 1, 5, 219, 2095104, 120, 2240),
)
for row in rows:
    if " ".join(map(str, row)) not in text:
        raise SystemExit(f"NOT REPRODUCED sign-06-10: Table 1 row {row[0]} differs")


def high(x, p, q):
    """x1 = round(p*x/q) mod+ p for x in [0, q); q is odd, so no ties occur."""
    return ((2 * p * x + q) // (2 * q)) % p


# sign-06-10. round(p*x/q) is a non-decreasing step function of x, so the
# set where x1 = 1 at p = 2 is an interval [lo, hi]; x1 = 0 elsewhere (values
# rounding to 2 reduce to 0). x1(-x) = x1(x) for all x exactly when the
# interval is symmetric under x -> q - x, i.e. lo + hi = q.
for level, q, *_rest in rows:
    gamma2 = _rest[7]
    p = -(-q // (2 * gamma2 + 1))
    if level in (256, 512):
        lo = next(x for x in range(q // 4 - 2, q // 4 + 3) if high(x, p, q) == 1)
        hi = next(x for x in range(3 * q // 4 + 2, 3 * q // 4 - 3, -1) if high(x, p, q) == 1)
        if p != 2 or high(lo - 1, p, q) != 0 or high(hi + 1, p, q) != 0 or lo + hi != q:
            raise SystemExit(f"NOT REPRODUCED sign-06-10: high-part symmetry fails at {level}")
        # y = (1, 0, ..., 0) and -y: A*y is the first column a of A and A*(-y) = -a.
        print(f"COMPASS-SIG-{level}: p = {p}; x1 = 1 exactly on [{lo}, {hi}], symmetric under x -> q - x, "
              "so w1(A*y) = w1(-A*y) for every A and the family is not universal")
    else:
        x = next(x for x in range(1, q) if high(x, p, q) != high((q - x) % q, p, q))
        print(f"COMPASS-SIG-{level} [control]: p = {p}; x1({x}) = {high(x, p, q)} but "
              f"x1(-{x}) = {high(q - x, p, q)}, so this sign counterexample does not apply")


# sign-06-11. Standard normal density and distribution function.
def rho(x):
    return math.exp(-x * x / 2) / math.sqrt(2 * math.pi)


def Phi(x):
    return 0.5 * math.erfc(-x / math.sqrt(2))


def printed(a, s):
    t = 2 * a / s
    return 1 - 2 * Phi(-t) - (s / a) * (rho(-t) - rho(0))


def corrected(a, s):
    t = 2 * a / s
    return 1 - 2 * Phi(-t) + (s / a) * (rho(-t) - rho(0))


def average(a, s, steps=20000):
    """Simpson average of the proof's Pr_x over x uniform on [-a, a]."""
    h = 2 * a / steps
    f = lambda x: 1 - Phi((x - a) / s) - Phi((-a - x) / s)
    total = f(-a) + f(a) + sum((4 if i % 2 else 2) * f(-a + i * h) for i in range(1, steps))
    return total * h / 3 / (2 * a)


for a, s in ((1.0, 0.5), (1.0, 1.0), (1.0, 0.1), (3.0, 2.0)):
    avg, pr, co = average(a, s), printed(a, s), corrected(a, s)
    if abs(avg - co) > 1e-9 or not pr > 1 >= avg:
        raise SystemExit(f"NOT REPRODUCED sign-06-11: a={a}, sigma={s}: average {avg}, printed {pr}")
    print(f"a = {a}, sigma = {s}: average of Pr_x {avg:.6f} = corrected form {co:.6f}; printed form {pr:.6f}")

print("PROOF GAP sign-06-10 CONFIRMED: w1 = Split(A*y, gamma2) is not universal at 256 and 512 (y and -y collide)")
print("ERRATUM sign-06-11 CONFIRMED: Proposition 1's printed probability has the wrong sign and exceeds one")
