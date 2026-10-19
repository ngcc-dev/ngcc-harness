#!/usr/bin/env python3
"""Analyze conj_lattice.sage output: counts of cyclic isogenies E -> E^(p) of
degree <= D versus the ternary-lattice volume prediction
   E[#] = c3 * D^(3/2) / sqrt(p),  c3 = 4*pi / (3*zeta(3))   (one per +-pair, primitive),
the distribution of the minimal degree lambda1 relative to (p/2)^(1/3), and
small-prime divisibility of the degrees versus uniform integers.
usage: python3 -I analyze_lat.py <p> <files...>"""
import json, math, sys

ZETA3 = 1.2020569031595942
C3 = 4 * math.pi / (3 * ZETA3)

def main():
    p = int(sys.argv[1])
    files = sys.argv[2:]
    rows = [json.loads(l) for f in files for l in open(f)]
    n = len(rows)
    D0 = (p / 2) ** (1 / 3)
    Dmax = min(r["Dmax"] for r in rows)
    out = {"p_log2": math.log2(p), "samples": n, "c3_theory": C3,
           "rank3_all": all(r["rank"] == 3 for r in rows if r["degs"]),
           "det3_is_p_over_4": all(r["det3"] == f"{p}/4" for r in rows if r["rank"] == 3)}
    # counts at thresholds
    tab = []
    for t in [2 ** (k / 2) for k in range(-12, 1 + int(2 * math.log2(Dmax / D0)))]:
        D = t * D0
        if D > Dmax:
            break
        cnt = sum(sum(1 for d in r["degs"] if d <= D) for r in rows) / n
        pred = C3 * D ** 1.5 / math.sqrt(p)
        hit = sum(1 for r in rows if r["degs"] and r["degs"][0] <= D) / n
        tab.append({"D_over_D0": round(t, 4), "mean_count": cnt, "pred": pred,
                    "ratio": cnt / pred if pred else None,
                    "Pr_lambda1_le_D": hit, "1-exp(-pred)": 1 - math.exp(-pred)})
    out["counts"] = tab
    l1 = sorted(r["degs"][0] / D0 for r in rows if r["degs"])
    out["lambda1_over_D0_quantiles"] = {q: l1[int(q * (len(l1) - 1))] for q in (0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 0.99)} if l1 else None
    out["lambda1_max_over_D0"] = max(l1) if l1 else None
    out["lambda1_mean_over_D0"] = sum(l1) / len(l1) if l1 else None
    # divisibility statistics of primitive degrees vs uniform integers
    alld = [d for r in rows for d in r["degs"]]
    div = {}
    for q in (2, 3, 4, 5, 7, 8, 9, 11, 13, 25, 49):
        div[q] = {"freq": sum(1 for d in alld if d % q == 0) / len(alld), "uniform": 1 / q}
    out["divisibility"] = div
    out["n_degrees"] = len(alld)
    print(json.dumps(out, indent=1))

if __name__ == "__main__":
    main()
