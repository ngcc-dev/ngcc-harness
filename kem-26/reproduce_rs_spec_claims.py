#!/usr/bin/env python3
"""kem-26-8 and kem-26-9: two statements about NSS-HQC's Reed-Solomon layer.

kem-26-8. Equation (67) bounds the failure rate by a multinomial over the n1
inner blocks, using per-block error and erasure probabilities taken after the
erasure cap. The script evaluates the forum post's three-block example
exactly: the capped decoder exceeds the Reed-Solomon limit with probability
3/4, while Eq. (67) with the same per-block marginals gives 17/27.

kem-26-9. Section 2.6.2 defines the code by evaluating the message polynomial
at alpha^0 .. alpha^(n1-1) and states that S_j = sum_i r_i alpha^(ij),
j = 1 .. n1-k1, vanish on every codeword. The script encodes messages at each
level's (n1, k1) over GF(256) and counts the nonzero syndromes; the unshortened
length-255 evaluation code is the control, where they do vanish.

Usage: reproduce_rs_spec_claims.py [--report-id kem-26-8|kem-26-9]
"""
import argparse
import itertools
import random
import shutil
import subprocess
import sys
from fractions import Fraction
from math import factorial
from pathlib import Path

HERE = Path(__file__).resolve().parent
REF = HERE / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
LEVELS = {"HQC-128": (46, 16), "HQC-256": (70, 32), "HQC-384": (100, 48), "HQC-512": (130, 64)}


def fail(issue, msg):
    print(f"NOT CONFIRMED {issue}: {msg}")
    sys.exit(1)


def spec_text():
    tool = shutil.which("pdftotext")
    if not tool:
        return None
    out = subprocess.run([tool, "-layout", str(HERE / "kem-26-spec.pdf"), "-"],
                         capture_output=True, text=True).stdout
    return " ".join(out.split())


def erasure_cap(text):
    issue = "kem-26-8"
    if text is not None:
        for phrase in ("Therefore the DFR is bounded by:",
                       "least reliable symbols are declared as erasures",
                       "Since each decapsulation query fails with probability at most"):
            if phrase not in text:
                fail(issue, f"specification text differs ({phrase!r})")
        print("spec: Theorem 9 bounds the DFR by a multinomial in per-block (P_err, P_era); the cap erases "
              "only the least reliable candidates; Eq. (84) charges DFR_NSS per decapsulation query")
    # Three inner blocks, redundancy D = 2, cap 1. Every block is an erasure candidate, wrong
    # with probability 1/2 independently, and exactly one (uniformly chosen) block is erased.
    n1, redundancy = 3, 2
    exact = Fraction(0)
    marg_err = marg_era = Fraction(0)
    for wrong in itertools.product((0, 1), repeat=n1):
        for erased in range(n1):
            p = Fraction(1, 2 ** n1) * Fraction(1, n1)
            t = sum(w for i, w in enumerate(wrong) if i != erased)
            exact += p * (2 * t + 1 > redundancy)
            marg_era += p * (erased == 0)
            marg_err += p * (wrong[0] and erased != 0)
    model = Fraction(0)
    for t in range(n1 + 1):
        for e in range(n1 + 1 - t):
            if 2 * t + e > redundancy:
                coeff = factorial(n1) // (factorial(t) * factorial(e) * factorial(n1 - t - e))
                model += coeff * marg_err ** t * marg_era ** e * (1 - marg_err - marg_era) ** (n1 - t - e)
    print(f"three-block example: per-block P_err = {marg_err}, P_era = {marg_era}; "
          f"exact failure probability {exact}; Eq. (67) value {model}")
    if (marg_err, marg_era, exact, model) != (Fraction(1, 3), Fraction(1, 3), Fraction(3, 4), Fraction(17, 27)):
        fail(issue, "example values differ")
    print("CONFIRMED kem-26-8: with capped erasures the multinomial of Eq. (67) gives 17/27 where the exact "
          "failure probability is 3/4, so it is not an upper bound in general")


def gf_tables():
    exp, log = [0] * 510, [0] * 256
    x = 1
    for i in range(255):
        exp[i] = x
        log[x] = i
        x <<= 1
        if x & 0x100:
            x ^= 0x11D          # X^8 + X^4 + X^3 + X^2 + 1, alpha = X
    for i in range(255, 510):
        exp[i] = exp[i - 255]
    return exp, log


def syndromes_nonzero(exp, log, msg, n, count):
    code = []
    for i in range(n):                                   # c_i = M(alpha^i)
        y = 0
        for coeff in reversed(msg):
            y = (exp[log[y] + i] if y else 0) ^ coeff
        code.append(y)
    nonzero = 0
    for j in range(1, count + 1):                        # S_j = sum_i c_i alpha^(ij)
        s = 0
        for i, c in enumerate(code):
            if c:
                s ^= exp[(log[c] + i * j) % 255]
        nonzero += s != 0
    return nonzero


def syndromes(text):
    issue = "kem-26-9"
    if text is not None:
        for phrase in ("then all syndromes are zero", "if r is a valid codeword, all syndromes are zero"):
            if phrase not in text:
                fail(issue, f"specification text differs ({phrase!r})")
        print("spec: Eqs. (16) and (19) define S_j = sum_i r_i alpha^(ij), j = 1..n1-k1, and state that "
              "they vanish on valid codewords")
    for name, (n1, k1) in LEVELS.items():
        params = (REF / name / "params.h").read_text()
        if f"#define NSS_HQC_N1 {n1}u" not in params or f"#define NSS_HQC_K1 {k1}u" not in params:
            fail(issue, f"{name} (n1, k1) differ from the archived parameter file")
    layer = (REF / "HQC-128" / "code_layer.c").read_text()
    if "rs_eval_encode" not in layer or "rs_try_decode" not in layer or "berlekamp" in layer.lower():
        fail(issue, "the submitted decoder differs from the archive")
    print("source: the code encodes by evaluation and decodes by solving a linear system (rs_try_decode); "
          "it has no Berlekamp-Massey syndrome decoder")
    exp, log = gf_tables()
    rng = random.Random(20261115)
    for name, (n1, k1) in LEVELS.items():
        ones = syndromes_nonzero(exp, log, [1] + [0] * (k1 - 1), n1, n1 - k1)
        msg = [rng.randrange(256) for _ in range(k1)]
        rand = syndromes_nonzero(exp, log, msg, n1, n1 - k1)
        full = syndromes_nonzero(exp, log, msg, 255, 255 - k1)
        print(f"{name}: [n1, k1] = [{n1}, {k1}]: nonzero syndromes {ones} of {n1 - k1} for M(X) = 1, "
              f"{rand} of {n1 - k1} for a random message; length-255 control {full} of {255 - k1}")
        # For M(X) = 1, S_j = (alpha^(n1*j) - 1)/(alpha^j - 1), which is zero only when 255 | n1*j.
        expect = sum((n1 * j) % 255 != 0 for j in range(1, n1 - k1 + 1))
        if ones != expect or rand < n1 - k1 - 2 or full != 0:
            fail(issue, f"{name} syndromes are not as predicted")
    print("CONFIRMED kem-26-9: at all four levels the specified syndromes do not vanish on valid codewords of the "
          "specified evaluation-form code (all 30 are nonzero at NSS-HQC-128); they vanish for the unshortened "
          "length-255 code")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report-id", choices=("kem-26-8", "kem-26-9"))
    args = ap.parse_args()
    text = spec_text()
    if args.report_id in (None, "kem-26-8"):
        erasure_cap(text)
    if args.report_id in (None, "kem-26-9"):
        syndromes(text)


if __name__ == "__main__":
    main()
