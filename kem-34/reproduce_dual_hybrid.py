#!/usr/bin/env python3
"""kem-34-5: lattice-estimator costs for the Rudraksh2-I public-key MLWE instances.

Run with Sage's Python and LATTICE_ESTIMATOR_PATH pointing at
malb/lattice-estimator commit 53da598.  Prints the best attack in the
estimator's default gate-count model (MATZOV) and in ADPS16/GSA Core-SVP.
"""

import os, subprocess, sys
from pathlib import Path

ESTIMATOR_COMMIT = "53da5982597709ba0fdf94ea37a84d822310fd84"
# (name, l*n, q, eta, Table 14 CSVP, Table 14 gates, NGCC target)
SETS = (("Rudraksh2-128-I", 9 * 64, 3329, 2, 129, 162, 128),
        ("Rudraksh2-256-I", 9 * 128, 3329, 1, 267, 303, 256),
        ("Rudraksh2-512-I", 8 * 256, 7681, 2, 512, 527, 512))
DENY = ("arora-gb", "bkw", "bdd_mitm_hybrid", "dual_mitm_hybrid")

def main():
    root = Path(__file__).resolve().parents[2]
    path = Path(os.environ.get("LATTICE_ESTIMATOR_PATH", str(root / "lattice-estimator")))
    try:
        rev = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"],
                                      text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError) as exc:
        raise SystemExit("LATTICE_ESTIMATOR_PATH must name a lattice-estimator checkout") from exc
    if rev != ESTIMATOR_COMMIT:
        raise SystemExit(f"lattice-estimator must be at {ESTIMATOR_COMMIT}; found {rev}")
    sys.path.insert(0, str(path))
    try:
        from estimator import LWE, ND, RC
        from sage.all import log, oo
    except ImportError as exc:
        raise SystemExit("run with Sage's Python") from exc
    below = []
    for name, n, q, eta, csvp, gates, target in SETS:
        p = LWE.Parameters(n=n, q=q, Xs=ND.CenteredBinomial(eta), Xe=ND.CenteredBinomial(eta), m=n)
        row = {}
        for model, kw in (("gate-count", {}),
                          ("Core-SVP", {"red_cost_model": RC.ADPS16, "red_shape_model": "gsa"})):
            r = LWE.estimate(p, deny_list=DENY, quiet=True, **kw)
            bits = {k: float(log(v["rop"], 2)) for k, v in r.items() if v["rop"] != oo}
            best = min(bits, key=bits.get)
            row[model] = bits[best]
            print(f"{name} {model}: {bits[best]:.2f} bits ({best}); "
                  + " ".join(f"{k}={v:.1f}" for k, v in sorted(bits.items())))
        print(f"  claims CSVP {csvp}, gates {gates}; target {target}")
        if row["gate-count"] < target:
            below.append(name)
    print("gate-count estimate below target:", ", ".join(below) or "none")
    ok = below == ["Rudraksh2-512-I"]
    print("CONFIRMED kem-34-5" if ok else "NOT CONFIRMED kem-34-5")
    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
