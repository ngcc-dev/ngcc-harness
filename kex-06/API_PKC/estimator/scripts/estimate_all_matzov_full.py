"""
MAMBA-NIKE Full Security Estimation - MATZOV-only model
========================================================
Strategy (optimized with deny_list):
  - primal + BKW   : LWE.estimate() with MATZOV, deny_list=["dual","dual_hybrid"]
  - dual / d-hyb   : LWE.estimate() with ADPS16, deny_list=[all primal + BKW]
  - Error dist     : zero-mean Uniform(-Delta/2, Delta/2) for compatibility
                     (BKW needs mu=0; same variance as exact Uniform(-Delta/2+1, Delta/2))

Columns:
  primal, dual, d-hyb, BKW  -- best log2(rop) per attack family
  MATZOV C = min(usvp,bdd) under MATZOV
  MATZOV Q = 0.292 * beta   (beta from MATZOV primal)
"""

import sys, math, time, os
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from estimator import *
from estimator.reduction import RC
MATZOV = RC.MATZOV   # instance, not class
ADPS16 = RC.ADPS16   # instance, not class

PROFILES = [
    ("NIKE-128", 128,  512,  8192, 2, 2, 16, 16),
    ("NIKE-192", 192, 1024, 8192, 2, 2,  4,  4),
    ("NIKE-256", 256, 1024, 8192, 2, 2,  8,  8),
    ("NIKE-384", 384, 2048, 8192, 2, 2,  2,  2),
    ("NIKE-512", 512, 2048, 8192, 2, 2,  8,  8),
]

def zero_mean_uniform(delta):
    """True quant error: Uniform(-delta/2+1, delta/2), mean=+0.5
    Approx (same var): Uniform(-delta/2, delta/2), mean=0"""
    h = delta // 2
    return ND.Uniform(-h, h)

def get_best_bits(rdict, key):
    """Extract log2(rop) from result dict for exact key match."""
    if key in rdict and hasattr(rdict[key], "__contains__") and "rop" in rdict[key]:
        return math.log2(rdict[key]["rop"])
    return None

def get_best_bits_multi(rdict_list, keys):
    """Best log2(rop) across multiple result dicts and keys."""
    best = None
    for rdict in rdict_list:
        for key in keys:
            b = get_best_bits(rdict, key)
            if b is not None and (best is None or b < best):
                best = b
    return best

def get_min_beta(rdict_list):
    """Min beta from usvp or bdd across result dicts."""
    beta = None
    for rdict in rdict_list:
        for key in ["usvp", "bdd"]:
            if key in rdict and hasattr(rdict[key], "__contains__") and "beta" in rdict[key]:
                b = rdict[key]["beta"]
                if beta is None or b < beta:
                    beta = b
    return beta

# Attacks we skip per cost model to avoid redundant computation
SKIP_DUAL   = ["dual", "dual_hybrid"]
SKIP_PRIMAL = ["usvp", "bdd", "bdd_hybrid", "bdd_mitm_hybrid", "bkw"]

results = []

for name, target, n, q, eta_s, eta_r, d_pk, d_u in PROFILES:
    Xs = ND.CenteredBinomial(eta_s)
    Xr = ND.CenteredBinomial(eta_r)
    Xe_pk = zero_mean_uniform(d_pk)
    Xe_u  = zero_mean_uniform(d_u)

    print(f"\n{'='*60}")
    print(f"  {name}  (n={n}, Delta_pk={d_pk}, Delta_u={d_u})")
    print(f"{'='*60}")

    pk = LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe_pk, m=n, tag=f"{name}-pk")
    u  = LWE.Parameters(n=n, q=q, Xs=Xr, Xe=Xe_u,  m=n, tag=f"{name}-u")

    t0 = time.time()

    # MATZOV: primal + BKW only (skip dual/d-hyb which don't work with MATZOV)
    print("  MATZOV primal + BKW...", flush=True)
    r_matz_pk = LWE.estimate(pk, red_cost_model=MATZOV, deny_list=SKIP_DUAL, catch_exceptions=True)
    r_matz_u  = LWE.estimate(u,  red_cost_model=MATZOV, deny_list=SKIP_DUAL, catch_exceptions=True)

    t1 = time.time()
    print(f"  [MATZOV done in {t1-t0:.0f}s]", flush=True)

    # ADPS16: dual + d-hyb only (skip primal/BKW which we already have from MATZOV)
    print("  ADPS16 dual + d-hyb...", flush=True)
    r_adps_pk = LWE.estimate(pk, red_cost_model=ADPS16, deny_list=SKIP_PRIMAL, catch_exceptions=True)
    r_adps_u  = LWE.estimate(u,  red_cost_model=ADPS16, deny_list=SKIP_PRIMAL, catch_exceptions=True)

    dt = time.time() - t0

    # Extract results
    all_m = [r_matz_pk, r_matz_u]
    all_a = [r_adps_pk, r_adps_u]

    primal = get_best_bits_multi(all_m, ["usvp", "bdd"])
    bkw    = get_best_bits_multi(all_m, ["bkw"])
    dual   = get_best_bits_multi(all_a, ["dual"])
    dhyb   = get_best_bits_multi(all_a, ["dual_hybrid"])
    beta_m = get_min_beta(all_m)

    matzov_c = primal
    matzov_q = 0.292 * beta_m if beta_m else None

    def ff(x):
        return f"{x:8.1f}" if x is not None else "      --"

    print(f"  primal={ff(primal)} dual={ff(dual)} d-hyb={ff(dhyb)} BKW={ff(bkw)}")
    print(f"  MATZOV C={ff(matzov_c)}  MATZOV Q={ff(matzov_q)}  (beta={beta_m})  [{dt:.0f}s]")

    results.append((name, target, primal, dual, dhyb, bkw,
                    matzov_c, matzov_q, beta_m))

# Final table
print()
print("=" * 105)
print("  MAMBA-NIKE  Consolidated Security Estimates  (MATZOV-only)")
print("=" * 105)
print(f"{'Scheme':>12s} {'Tgt':>4s} {'primal':>8s} {'dual':>8s} {'d-hyb':>8s} "
      f"{'BKW':>8s} {'MATZOV C':>9s} {'MATZOV Q':>9s} {'Mgn C':>6s} {'Mgn Q':>6s}  beta")
print("-" * 105)
for name, target, pr, du, dh, bk, mc, mq, beta in results:
    def ff(x):
        return f"{x:8.1f}" if x is not None else "      --"
    mgc = mc - target if mc else None
    mgq = mq - target if mq else None
    bstr = f"{beta:6d}" if beta else "    --"
    print(f"{name:>12s} {target:>4d} {ff(pr)} {ff(du)} {ff(dh)} {ff(bk)} "
          f"{ff(mc)} {ff(mq)} {ff(mgc)} {ff(mgq)}  {bstr}")
print("-" * 105)
print("primal+BKW: MATZOV cost model.  dual+d-hyb: ADPS16 (MATZOV dual unavailable).")
print("MATZOV C = min(usvp,bdd) under MATZOV")
print("MATZOV Q = 0.292 * beta  (beta from MATZOV primal)")
print("Errors: zero-mean Uniform(-Delta/2,Delta/2), same variance as exact distribution")
print("=" * 105)

out = "/tmp/nike_matzov_table.txt"
with open(out, "w") as f:
    f.write("MAMBA-NIKE Consolidated Security Estimates (MATZOV-only)\n")
    f.write("=" * 105 + "\n")
    f.write(f"{'Scheme':>12s} {'Tgt':>4s} {'primal':>8s} {'dual':>8s} {'d-hyb':>8s} "
            f"{'BKW':>8s} {'MATZOV C':>9s} {'MATZOV Q':>9s} {'Mgn C':>6s} {'Mgn Q':>6s}  beta\n")
    f.write("-" * 105 + "\n")
    for name, target, pr, du, dh, bk, mc, mq, beta in results:
        def ff(x):
            return f"{x:8.1f}" if x is not None else "      --"
        mgc = mc - target if mc else None
        mgq = mq - target if mq else None
        bstr = f"{beta:6d}" if beta else "    --"
        f.write(f"{name:>12s} {target:>4d} {ff(pr)} {ff(du)} {ff(dh)} {ff(bk)} "
                f"{ff(mc)} {ff(mq)} {ff(mgc)} {ff(mgq)}  {bstr}\n")
    f.write("-" * 105 + "\n")
print(f"Saved: {out}")
