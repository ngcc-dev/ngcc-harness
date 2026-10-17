#!/usr/bin/env python3
"""Certify sign-22-7: Rhyme's Gaussian width makes the (1+4eps)^Q_S proof
factor vacuous within the required signing-query budget.

Theorem 4.3 (physical p. 41) needs sigma >= eta_eps(Z^{nD}); Corollary 4
(p. 44) and Theorem 4.6 (p. 46) multiply the no-message advantage by
(1 + 4 eps)^Q_S.  With the specification's convention
rho_s(x) = exp(-pi x^2 / s^2) (Section 2.7, p. 11) and Z^k self-dual, the
smallest eps with eta_eps(Z^k) <= s is exactly theta(s)^k - 1, where
theta(s) = sum_j exp(-pi s^2 j^2).  Table 3 (p. 53) gives
sigma/sqrt(2 pi); the submitted CDT tables realise that standard deviation.

The script checks the 16 submitted params_tables.h copies (SHAKE and SM3,
reference and optimized), confirms that each X' CDT has the stated
standard deviation, evaluates eps and the Q_S multiplier at 2^34, 2^64 and
2^80 queries, and runs two controls: the exact eps never exceeds the
Lemma 2.2 upper bound, and a width chosen for eps <= 2^-80 keeps the
multiplier below 2 at 2^64 queries.  It optionally checks the theorem text
in the PDF when pdftotext is installed.
"""

import math
import re
import shutil
import subprocess
import sys
from pathlib import Path

CAND = Path(__file__).resolve().parent
IMPL = CAND / "Implementations and Test_Vectors" / "Implementations"
ID = "sign-22-7"

# Table 3 (physical p. 53): level -> (n, D, sigma/sqrt(2 pi))
TABLE3 = {128: (256, 10, 1.295870), 256: (512, 10, 1.316036),
          384: (512, 14, 1.326374), 512: (1024, 10, 1.335898)}
REQUIRED_LOG2_QS = 64     # NGCC per-key signing lifetime
BUDGET_LOG2_QS = 80       # NGCC evaluation ceiling


def mode_block(text, level):
    start = re.search(rf"#(?:if|elif) RHYME_MODE == {level}\b", text)
    if not start:
        raise AssertionError(f"RHYME_MODE {level} block missing")
    tail = text[start.end():]
    end = re.search(r"\n#(?:elif RHYME_MODE|else|endif)\b", tail)
    return tail[:end.start()] if end else tail


def define(block, name):
    m = re.search(rf"#define {name} (\d+)", block)
    if not m:
        raise AssertionError(f"{name} missing")
    return int(m.group(1))


def cdt_std(block, name):
    m = re.search(rf"static const uint64_t {name}\[\d+\] = \{{(.*?)\}};", block, re.S)
    if not m:
        raise AssertionError(f"{name} missing")
    cdf = [int(x) for x in re.findall(r"(\d+)ULL", m.group(1))]
    var, prev = 0.0, 0
    for mag, c in enumerate(cdf):  # entry m = P(|X| <= m) in Q0.63
        var += (c - prev) / 2**63 * mag * mag
        prev = c
    return math.sqrt(var)


def eps_exact(s, k):
    theta = sum(math.exp(-math.pi * s * s * j * j) for j in range(1, 12))
    return math.expm1(k * math.log1p(2 * theta))


def eps_lemma22(s, k):
    # Lemma 2.2 with max ||b_i|| = 1: eta <= sqrt(ln(k - 1 + 2k/eps)/pi)
    return 2 * k / (math.exp(math.pi * s * s) - k + 1)


def log_multiplier(eps, log2_qs):
    return 2.0**log2_qs * math.log1p(4 * eps)   # ln((1+4eps)^Q_S)


def check_pdf():
    if shutil.which("pdftotext") is None:
        print("  spec text: pdftotext unavailable, PDF wording check skipped")
        return
    text = subprocess.run(["pdftotext", "-layout", str(CAND / "sign-22-spec.pdf"), "-"],
                          check=True, stdout=subprocess.PIPE, text=True).stdout
    flat = " ".join(text.split())
    assert "σ ≥ ηε (ZnD )" in flat, "Theorem 4.3 smoothing condition not found"
    assert "εzk = 4ε + O(ε2 )" in flat, "Theorem 4.3 eps_zk not found"
    assert flat.count("(1 + 4ε)QS") >= 4, "(1+4eps)^Q_S factor not found"
    assert "ρ√Σ,c : x → exp(−π(x − c)⊤ Σ −1 (x − c))" in flat, "Gaussian convention not found"
    row = "1.295870 1.316036 1.326374 1.335898"
    assert row in flat, "Table 3 sigma/sqrt(2 pi) row not found"
    print("  spec text: Theorem 4.3 condition, (1+4eps)^Q_S factor (x%d), "
          "Section 2.7 convention and Table 3 widths found" % flat.count("(1 + 4ε)QS"))


def main():
    if not IMPL.is_dir():
        print(f"NOT CONFIRMED {ID}: implementation tree missing")
        return 1
    print("Rhyme smoothing error and Q_S multiplier (Theorem 4.3, Corollary 4, Theorem 4.6)")
    check_pdf()

    ok = True
    safe = []
    epss = []
    for flavour in ("SHAKE", "SM3"):
        for level, (n, dmat, table_sb) in TABLE3.items():
            sbs = set()
            for tree in ("Reference_Implementation", "Optimized_Implementation"):
                hdr = IMPL / tree / f"Rhyme-{flavour}" / f"Rhyme-{flavour}-{level}" / "include" / "params_tables.h"
                block = mode_block(hdr.read_text(), level)
                assert define(block, "RHYME_N") == n
                assert define(block, "RHYME_DMAT") == dmat
                m = re.search(r"sigma_base ?= ?([\d.]+)", block)
                sb = float(m.group(1))
                std = cdt_std(block, "rhyme_cdt_g")
                assert abs(std - sb) < 1e-5, f"{hdr}: X' CDT std {std} != {sb}"
                sbs.add(sb)
            assert len(sbs) == 1, "reference and optimized widths differ"
            sb = sbs.pop()
            if flavour == "SHAKE":
                assert sb == table_sb, f"SHAKE-{level} width {sb} != Table 3 {table_sb}"
            s = sb * math.sqrt(2 * math.pi)
            k = n * dmat
            eps = eps_exact(s, k)
            lemma = eps_lemma22(s, k)
            if not eps <= lemma * (1 + 1e-9):
                print(f"  control FAILED: exact eps {eps:.3e} exceeds Lemma 2.2 bound {lemma:.3e}")
                ok = False
            m34, m64, m80 = (log_multiplier(eps, q) for q in (34, 64, 80))
            q_one_bit = math.log2(math.log(2) / math.log1p(4 * eps))
            print(f"  Rhyme-{flavour}-{level}: sigma_base={sb} s={s:.4f} k=nD={k} "
                  f"eps=2^{math.log2(eps):.2f} (Lemma 2.2 bound 2^{math.log2(lemma):.2f}); "
                  f"multiplier <= 2 only for Q_S <= 2^{q_one_bit:.2f}; "
                  f"ln(mult) at 2^34/2^64/2^80 = {m34:.3g}/{m64:.3g}/{m80:.3g}")
            # vacuous: the multiplier alone exceeds 2^level at the required lifetime
            if not m64 / math.log(2) > level:
                ok = False
            safe.append(q_one_bit)
            epss.append(math.log2(eps))

    # Control: a width giving eps <= 2^-80 at Rhyme-128 keeps the bound meaningful.
    n, dmat, _ = TABLE3[128]
    k = n * dmat
    s_ctl = math.sqrt(math.log(2 * k * 2.0**80) / math.pi)
    eps_ctl = eps_exact(s_ctl, k)
    mult_ctl = math.exp(log_multiplier(eps_ctl, REQUIRED_LOG2_QS))
    print(f"  control: s={s_ctl:.4f} (sigma_base={s_ctl / math.sqrt(2 * math.pi):.4f}) gives "
          f"eps=2^{math.log2(eps_ctl):.2f} and multiplier {mult_ctl:.9f} at Q_S=2^64")
    if not (eps_ctl <= 2.0**-80 and mult_ctl < 2):
        ok = False

    if ok:
        print(f"CONFIRMED {ID}: the submitted widths give eps = 2^{max(epss):.1f}..2^{min(epss):.1f}, so "
              f"(1+4eps)^Q_S exceeds 2 beyond Q_S = 2^{min(safe):.1f}..2^{max(safe):.1f} and exceeds "
              f"2^lambda well before the required 2^{REQUIRED_LOG2_QS} signatures "
              f"(2^{BUDGET_LOG2_QS} budget); all 16 trees")
        return 0
    print(f"NOT CONFIRMED {ID}: the multiplier did not become vacuous or a control failed")
    return 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (AssertionError, OSError) as exc:
        print(f"NOT CONFIRMED {ID}: {exc}")
        sys.exit(1)
