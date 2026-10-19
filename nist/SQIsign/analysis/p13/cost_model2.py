#!/usr/bin/env python3
"""Refined cost model (v2) for Wesolowski's p^(1/3) attack / 'going under' at the
SQIsign Round-3 primes, using
  * the validated lattice law  E[#cyclic phi: E->E^(p), deg<=D] = c3 D^1.5/sqrt(p)
    (conj_lattice.sage, analyze_lat.py; det of the ternary lattice = p/4 exactly),
  * the measured smoothness correction 0.95 (smooth_check.py),
  * Monte-Carlo multiplication-table factor E[1/r](X, beta), beta = log B/log X
    (pair_mc.py, out/pair_grid.jsonl), so that
       lam = 0.95 * 1.5 c3/sqrt(p) * (sum_{s in S} sqrt s)^2 * E[1/r],
       G   = psibar * sum_{s in S} s      (table entries = generated j-invariants),
       T   = G / (1 - exp(-lam))          (expected entries over all instances).
Conventions per generated entry:
  wes  : 1 Fp2 operation; reported also x gates(Fp2 mul) from fp2_gates.py
  spec : log2(B)^2 log2(p)^2 bit operations (spec Remark 8.2.1)
Memory: 'cap' = spec caps 2^80/2^120/2^160 entries; 'free' = unbounded free RAM;
        'cbrt' = + w^(1/3) per generated entry (SQIsign-4 sensitivity); 'sqrt' = + w^(1/2).
Precomputation option: B <= Bmax with sum_{l<=B} l^3 ~ B^4/(4 ln B) = 2^target
(Phi_l mod p for all l <= B; Broker-Lauter-Sutherland cost >= l^3 per prime).
usage: python -I cost_model2.py   (Sage python for numpy)"""
import json, math, sys
from pathlib import Path
import numpy as np
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from cost_model import log2_I, C3, SETS, PSIBAR, LN2

SMOOTH_CORR = 0.95
GATES = json.loads((HERE / "model-data/fp2_gates.json").read_text())
GRID = [json.loads(line) for line in
        (HERE / "model-data/pair_grid.jsonl").read_text().splitlines()]
BETAS = sorted({round(r["log2B"] / r["log2X"], 2) for r in GRID})
XS = sorted({r["log2X"] for r in GRID})
TAB = {(r["log2X"], round(r["log2B"] / r["log2X"], 2)): math.log2(r["E_inv_r"]) for r in GRID}


def log2_inv_r(lx, beta):
    """bilinear interpolation of log2 E[1/r]; clamps X to grid range, beta to [0.3, 1]
    (for beta < 0.3 the beta=0.3 value is used: attacker-favourable)."""
    beta = min(1.0, max(0.3, beta))
    lx = min(max(lx, XS[0]), XS[-1])
    def at(x, b):
        if (x, b) in TAB:
            return TAB[(x, b)]
        # beta=0.3 missing for large X: use nearest X that has it
        cands = [(abs(xx - x), v) for (xx, bb), v in TAB.items() if bb == b]
        return min(cands)[1]
    i = max(j for j in range(len(XS)) if XS[j] <= lx)
    i2 = min(i + 1, len(XS) - 1)
    k = max(j for j in range(len(BETAS)) if BETAS[j] <= beta + 1e-9)
    k2 = min(k + 1, len(BETAS) - 1)
    tx = 0 if i2 == i else (lx - XS[i]) / (XS[i2] - XS[i])
    tb = 0 if k2 == k else (beta - BETAS[k]) / (BETAS[k2] - BETAS[k])
    v00, v01 = at(XS[i], BETAS[k]), at(XS[i], BETAS[k2])
    v10, v11 = at(XS[i2], BETAS[k]), at(XS[i2], BETAS[k2])
    return (1 - tx) * ((1 - tb) * v00 + tb * v01) + tx * ((1 - tb) * v10 + tb * v11)


import functools

@functools.lru_cache(maxsize=None)
def inst(log2p, lx, lb):
    lb = min(lb, lx)
    lG = math.log2(PSIBAR) + log2_I(1.0, lx, lb)
    lsq = log2_I(0.5, lx, lb)
    llam = math.log2(SMOOTH_CORR * 1.5 * C3) - log2p / 2 + 2 * lsq + log2_inv_r(lx, lb / lx)
    lam = 2 ** llam
    lP = math.log2(-math.expm1(-lam)) if lam < 700 else 0.0
    return lG, llam, lG - lP


def bmax(target):
    lb = 10.0
    while (4 * lb - math.log2(4 * lb * LN2)) < target:
        lb += 0.01
    return lb


def per_entry(log2p, lb, lx, conv, g):
    return {"fp2ops": 0.0, "gate1": g, "logp2": 2 * math.log2(log2p),
            "spec": 2 * math.log2(min(lb, lx)) + 2 * math.log2(log2p)}[conv]


def cost(log2p, lx, lb, conv, mem, g=0.0, pre=False):
    lG, llam, lT = inst(log2p, lx, lb)
    per = per_entry(log2p, lb, lx, conv, g)
    if mem == "cbrt":
        per = math.log2(2 ** per + 2 ** (lG / 3))
    elif mem == "sqrt":
        per = math.log2(2 ** per + 2 ** (lG / 2))
    c = lT + per
    if pre:
        b = min(lb, lx)
        c = math.log2(2 ** c + 2 ** (4 * b - math.log2(4 * b * LN2)))
    return c, lG, llam, lT


def optimise(log2p, conv, mem, cap=None, lbmax=None, g=0.0, pre=False):
    best = _opt(log2p, conv, mem, cap, lbmax, [float(x) for x in range(8, 141, 2)], None, g, pre)
    if best is None:
        return None
    b0, x0 = best["log2B"], best["log2X"]
    lbs = [b0 + d / 4 for d in range(-8, 9)]
    best = _opt(log2p, conv, mem, cap, lbmax, lbs, [x0 + d / 4 for d in range(-12, 13)], g, pre)
    if best and cap is not None:
        best["kappa_log2"] = best["log2T_entries"] - (log2p - best["log2mem"]) / 2
    return best


def _opt(log2p, conv, mem, cap, lbmax, lbs, lxs, g, pre):
    best = None
    for lb in lbs:
        if lbmax is not None and lb > lbmax + 1e-9:
            lb = lbmax
        xs = lxs if lxs is not None else [float(x) for x in range(int(max(lb, 20)), int(log2p / 3 + 30), 2)]
        for lx in xs:
            if lx < lb:   # B > X is equivalent to B = X
                continue
            c, lG, llam, lT = cost(log2p, lx, lb, conv, mem, g, pre)
            if cap is not None and lG > cap:
                continue
            if best is None or c < best["cost"]:
                best = dict(cost=c, log2T_entries=lT, log2mem=lG, log2lam=llam, log2B=lb, log2X=lx)
    return best


def main():
    res = {}
    for name, v in SETS.items():
        p = v["c"] * 2 ** v["f"] - 1
        log2p = math.log2(p)
        g = GATES[name]["fp2_mul_gates_log2"]
        r = {"log2p": log2p, "target": v["target"], "Bmax_precomp_log2": bmax(v["target"]),
             "fp2_mul_gates_log2": g, "rows": []}
        for conv in ("fp2ops", "gate1", "logp2", "spec"):
            for mem in ("cap", "free", "cbrt", "sqrt"):
                for pre in (False, True):
                    if conv == "fp2ops" and pre:
                        continue
                    b = optimise(log2p, conv, "free" if mem == "cap" else mem,
                                 cap=v["cap"] if mem == "cap" else None, g=g, pre=pre)
                    row = dict(conv=conv, mem=mem, precomp=pre, **b)
                    row["margin"] = b["cost"] - v["target"]
                    r["rows"].append(row)
        res[name] = r
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
