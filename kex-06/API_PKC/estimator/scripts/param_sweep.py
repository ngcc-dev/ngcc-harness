"""
param_sweep.py — NIKE Security Estimation by Varying Delta = q/p

Reads presets from my_params.py, varies p (= p_pk = p_u),
estimates LWE security using the lattice-estimator.

Usage:
    cd lattice-estimator
    python3 param_sweep.py
"""

import sys
import os
import math

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from estimator import *
from estimator.reduction import MATZOV, ADPS16

# ============================================================
# 1.  PRESETS  (from my_params.py)
# ============================================================
PRESETS = {
    "NIKE-128": dict(n=512,  q=2**13, p_pk=2**12, p_u=2**12, p_v=2**6, eta_s=2, eta_r=2),
    "NIKE-192": dict(n=1024, q=2**13, p_pk=2**12, p_u=2**12, p_v=2**6, eta_s=2, eta_r=2),
    "NIKE-256": dict(n=1024, q=2**13, p_pk=2**12, p_u=2**12, p_v=2**6, eta_s=2, eta_r=2),
    "NIKE-384": dict(n=2048, q=2**13, p_pk=2**12, p_u=2**12, p_v=2**6, eta_s=2, eta_r=2),
}

# ============================================================
# 2.  DISTRIBUTION HELPERS
# ============================================================
def cbd_stddev(eta):
    return math.sqrt(float(eta) / 2.0)

def qerr_stddev(delta):
    delta = float(delta)
    return math.sqrt((delta * delta - 1.0) / 12.0)

def qerr_support(delta):
    delta = int(delta)
    if delta % 2 != 0:
        raise ValueError("delta must be even, got %d" % delta)
    h = delta // 2
    return -h + 1, h

def make_secret_dist(eta):
    try:
        return ND.CenteredBinomial(eta)
    except Exception:
        sigma = cbd_stddev(eta)
        return ND.DiscreteGaussian(stddev=sigma)

def make_error_dist(delta):
    a, b = qerr_support(delta)
    try:
        return ND.Uniform(a, b)
    except Exception:
        sigma = qerr_stddev(delta)
        return ND.DiscreteGaussian(stddev=sigma)

# ============================================================
# 3.  ESTIMATE ONE SURFACE
# ============================================================
def estimate_surface(n, q, eta, delta, tag):
    Xs = make_secret_dist(eta)
    Xe = make_error_dist(delta)
    m = n

    try:
        params = LWEParameters(n=n, q=q, Xs=Xs, Xe=Xe, m=m, tag=tag)
    except Exception:
        params = LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe, m=m, tag=tag)

    try:
        results = LWE.estimate(params)
    except Exception as e:
        print(f"  {tag}: estimator failed — {e}")
        return None

    costs = {}
    for name in ["primal", "dual", "dual_hybrid"]:
        r = results.get(name)
        if r is not None:
            try:
                costs[name] = round(math.log2(r["rop"]), 1)
            except Exception:
                costs[name] = round(r["rop"], 1)

    try:
        r = LWE.estimate(params, red_cost_model=MATZOV)
        costs["matzov_c"] = round(math.log2(r["rop"]), 1)
    except Exception:
        costs["matzov_c"] = None

    try:
        r = LWE.estimate(params, red_cost_model=ADPS16)
        costs["csvp_c"] = round(math.log2(r["rop"]), 1)
    except Exception:
        costs["csvp_c"] = None

    try:
        r = LWE.estimate(params, red_cost_model=MATZOV)
        beta = r["beta"]
        costs["csvp_q"] = round(0.265 * beta, 1)
    except Exception:
        costs["csvp_q"] = None

    return costs


# ============================================================
# 4.  SWEEP
# ============================================================
def sweep_preset(name, params, target_level):
    q  = params["q"]
    eta = params["eta_s"]
    n  = params["n"]
    logq = int(math.log2(q))
    t_values = list(range(1, logq))

    print(f"\n{'='*70}")
    print(f"  {name}:  n={n}, q=2^{logq}, eta={eta}, target={target_level} bit")
    print(f"{'='*70}")
    hdr = f"  {'t':>3s}  {'p':>8s}  {'Delta':>8s}  {'sigma_e':>7s}  {'primal':>7s}  {'dual':>7s}  {'d-hyb':>7s}  {'MATZOV':>7s}  {'C-SVP C':>7s}  {'C-SVP Q':>7s}  {'OK':>4s}"
    print(hdr)
    print(f"  {'-'*3}  {'-'*8}  {'-'*8}  {'-'*7}  {'-'*7}  {'-'*7}  {'-'*7}  {'-'*7}  {'-'*7}  {'-'*7}  {'-'*4}")

    rows = []
    for t in t_values:
        p = 2**t
        delta = q // p
        sigma_e = qerr_stddev(delta)

        pk_cost = estimate_surface(n, q, eta, delta, f"{name}-pk(t={t})")
        if pk_cost is None:
            continue
        u_cost  = estimate_surface(n, q, eta, delta, f"{name}-u(t={t})")
        if u_cost is None:
            continue

        primal = min(pk_cost.get("primal", 999), u_cost.get("primal", 999))
        dual   = min(pk_cost.get("dual", 999),   u_cost.get("dual", 999))
        dhyb   = min(pk_cost.get("dual_hybrid", 999), u_cost.get("dual_hybrid", 999))
        matzov = min(pk_cost.get("matzov_c", 999) or 999, u_cost.get("matzov_c", 999) or 999)
        csvp_c = min(pk_cost.get("csvp_c", 999) or 999, u_cost.get("csvp_c", 999) or 999)
        csvp_q = min(pk_cost.get("csvp_q", 999) or 999, u_cost.get("csvp_q", 999) or 999)

        ok = "YES" if primal >= target_level else "NO"

        print(f"  {t:3d}  {str(p):>8s}  {delta:8d}  {sigma_e:7.2f}  {primal:7.1f}  {dual:7.1f}  {dhyb:7.1f}  {matzov:7.1f}  {csvp_c:7.1f}  {csvp_q:7.1f}  {ok:>4s}")

        rows.append(dict(t=t, p=p, delta=delta, sigma_e=sigma_e,
                         primal=primal, dual=dual, dhyb=dhyb,
                         matzov=matzov, csvp_c=csvp_c, csvp_q=csvp_q, ok=ok))
    return rows


# ============================================================
if __name__ == "__main__":
    all_rows = {}
    for name, params in PRESETS.items():
        level = int(name.split("-")[1])
        rows = sweep_preset(name, params, level)
        all_rows[name] = rows

    print(f"\n\n{'='*70}")
    print(f"  SUMMARY")
    print(f"{'='*70}")
    for name, rows in all_rows.items():
        level = int(name.split("-")[1])
        good = [r for r in rows if r["ok"] == "YES"]
        if good:
            best = good[-1]
            print(f"  {name}: t >= {best['t']} (p >= 2^{best['t']}={best['p']}, "
                  f"Delta <= {best['delta']}, sigma_e <= {best['sigma_e']:.2f})")
            print(f"         primal={best['primal']:.1f}  dual={best['dual']:.1f}  "
                  f"d-hyb={best['dhyb']:.1f}  MATZOV={best['matzov']:.1f}  "
                  f"C-SVP C={best['csvp_c']:.1f}  C-SVP Q={best['csvp_q']:.1f}")
        else:
            print(f"  {name}: NO t above target!")
