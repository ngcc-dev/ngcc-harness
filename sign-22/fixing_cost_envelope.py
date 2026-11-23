#!/usr/bin/env python3
"""Fixing-cost check for sign-22-4 and sign-22-8 (Rhyme).

For each parameter set it evaluates the rejection-envelope condition of
Appendix A (physical p. 63), the closed form of physical p. 64, and the
constant M that the submitted sources use, then the acceptance rates that
follow from the submitted M.  It does this for the Table 3 parameters
(physical p. 53; shipped in the Rhyme-SHAKE trees) and for the older table
shipped in the Rhyme-SM3 trees.  It prints numbers only: no signature is
computed and nothing is measured.

Conventions: sigma_y and sigma/sqrt(2 pi) are used as standard deviations,
as the sources do (sampler.c:326-328, params_tables.h:26 "std of y1/z1
Gaussian").
The l2 acceptance uses a normal approximation of ||z||^2 with independent
coefficients and the marginal variance stated in include/params.h:41-45,
    v_marg = sigma_base^2 * (1 + 4 * D * N * (eta / 2)).
"""

import re
from math import erf, exp, sqrt
from pathlib import Path

REF = (Path(__file__).resolve().parent / "Implementations and Test_Vectors"
       / "Implementations" / "Reference_Implementation")

# Table 3 (physical p. 53): sigma/sqrt(2 pi), repetitions.
TABLE3_SIGMA = {128: 1.295870, 256: 1.316036, 384: 1.326374, 512: 1.335898}
TABLE3_REPS = {128: 2.73, 256: 2.99, 384: 2.95, 512: 2.69}


def read_table(family):
    """Parse the per-mode constants of include/params_tables.h."""
    text = (REF / f"Rhyme-{family}" / f"Rhyme-{family}-512" / "include"
            / "params_tables.h").read_text()
    out = {}
    for mode, body in re.findall(
            r"#if RHYME_MODE == (\d+)\n(.*?)\n#endif", text, re.S):
        d = {k: int(v) for k, v in
             re.findall(r"#define RHYME_(\w+) (\d+)", body)}
        m = re.search(r"sigma_base = ([0-9.]+)", body)
        d["SIGMA"] = float(m.group(1)) if m else TABLE3_SIGMA[int(mode)]
        out[int(mode)] = d
    return out


def phi(x):
    return 0.5 * (1.0 + erf(x / sqrt(2.0)))


def evaluate(p):
    n, d, D, eta = p["N"], p["D"], p["DMAT"], p["ETA"]
    sy, b0, r = p["SIGMAY"], p["B0"], p["R"]
    st = 2.0 * p["SIGMA"]                      # sigma_tar = 2 sigma

    def rho_y(x):
        return exp(-x * x / (2.0 * sy * sy))

    def rho_t(v):
        return exp(-v * v / (2.0 * st * st))

    need, closed = 0.0, 0.0
    for c in (0, 1):
        vs = [v for v in range(-r, r + 1) if (v - c) % 2 == 0]
        s_c = sum(rho_t(v) for v in vs)
        for y in range(-(b0 + r), b0 + r + 1):
            # condition of physical p. 63: sum over v with |y + v| <= B1
            a = sum(rho_t(v) / s_c * exp(-(2 * y * v + v * v) / (2.0 * sy * sy))
                    for v in vs if abs(y + v) <= b0)
            need = max(need, a)
        rc = max(vs)
        # closed form of physical p. 64
        closed = max(closed, rho_t(rc) / s_c
                     * exp((2 * b0 * rc + rc * rc) / (2.0 * sy * sy)))
    m_src = p["M_Q60"] / 2.0 ** 60
    zs = range(-b0, b0 + 1)
    num = sum(rho_y(z) for z in zs)
    den = sum(rho_y(y) for y in range(-(b0 + r), b0 + r + 1))
    p_coef = num / den / m_src                 # Pr_acc, physical p. 62
    p_z1 = p_coef ** n
    e2 = sum(z * z * rho_y(z) for z in zs) / num
    e4 = sum(z ** 4 * rho_y(z) for z in zs) / num
    vm = p["SIGMA"] ** 2 * (1 + 4 * D * n * (eta / 2.0))
    mean = n * e2 + d * n * vm
    var = n * (e4 - e2 * e2) + d * n * 2.0 * vm * vm
    p_l2 = phi((p["L2SQ"] - mean) / sqrt(var))
    return need, closed, m_src, p_z1, p_l2, 1.0 / (p_z1 * p_l2)


def main():
    tables = {"Table 3 (Rhyme-SHAKE trees)": read_table("SHAKE"),
              "Rhyme-SM3 trees": read_table("SM3")}
    reps = {}
    for label, table in tables.items():
        print(label)
        for mode in sorted(table):
            p = table[mode]
            need, closed, m_src, p_z1, p_l2, rep = evaluate(p)
            reps[(label, mode)] = rep
            assert m_src >= need, (label, mode)
            print(f"  {mode}: tau={p['TAU']} sigma_y={p['SIGMAY']} "
                  f"B1={p['B0']} R={p['R']}: required M >= {need:.6f}, "
                  f"closed form {closed:.2e}, source M {m_src:.6f}; "
                  f"Pr(z1 accepted) {p_z1:.3f}, Pr(l2) {p_l2:.3f}, "
                  f"repetitions {rep:.2f}")
    a, b = tables
    print("comparison")
    for mode in sorted(TABLE3_REPS):
        print(f"  {mode}: Table 3 states {TABLE3_REPS[mode]:.2f} repetitions; "
              f"model {reps[(a, mode)]:.2f} (Table 3 values) and "
              f"{reps[(b, mode)]:.2f} (SM3 trees), "
              f"ratio {reps[(a, mode)] / reps[(b, mode)]:.3f}")


if __name__ == "__main__":
    main()
