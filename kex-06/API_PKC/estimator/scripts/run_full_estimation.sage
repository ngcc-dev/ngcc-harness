#!/usr/bin/env sage
# ============================================================
# MAMBA-NIKE Full Security Estimation — MATZOV model
# ============================================================
# All parameters read from parameters.json (single source of truth).
# Attacks: primal (usvp/bdd) + BKW via MATZOV
#          dual + dual_hybrid via ADPS16
# Output:  console table + Markdown summary in results/
# ============================================================

import sys, math, time, os, json, datetime
from pathlib import Path

# Same path logic as estimate_all_matzov_full.py
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))          # .../estimator/scripts
ESTIMATOR_DIR = os.path.dirname(SCRIPT_DIR)                       # .../estimator
PROJECT_ROOT = os.path.dirname(ESTIMATOR_DIR)                     # .../MAMBA-NIKE
sys.path.insert(0, ESTIMATOR_DIR)

from estimator import *
from estimator.reduction import RC
MATZOV = RC.MATZOV
ADPS16 = RC.ADPS16

# ═══════════════════════════════════════════════════════════════
# Load parameters from parameters.json
# ═══════════════════════════════════════════════════════════════
MANIFEST = os.path.join(PROJECT_ROOT, "parameters.json")
with open(MANIFEST, "r") as fh:
    pdata = json.load(fh)

Q = int(pdata["q"])
LOG2Q = int(pdata["log2q"])

PROFILES = []
for item in pdata["profiles"]:
    PROFILES.append((
        item["name"],              # name
        int(item["level"]),        # target security level
        int(item["n"]),            # dimension
        Q,                         # modulus
        int(item["eta_s"]),        # secret CBD parameter
        int(item["eta_r"]),        # error CBD parameter (for u-surface)
        int(item["delta_pk"]),     # pk error scale Δ
        int(item["delta_u"]),      # u error scale Δ
    ))

# ═══════════════════════════════════════════════════════════════
# Helpers
# ═══════════════════════════════════════════════════════════════
def zero_mean_uniform(delta):
    """Uniform(-delta/2, delta/2), mean=0, same variance as exact quant error."""
    h = delta // 2
    return ND.Uniform(-h, h)

def get_best_bits(rdict, key):
    if key in rdict and hasattr(rdict[key], "__contains__") and "rop" in rdict[key]:
        return math.log2(rdict[key]["rop"])
    return None

def get_best_bits_multi(rdict_list, keys):
    best = None
    for rdict in rdict_list:
        for key in keys:
            b = get_best_bits(rdict, key)
            if b is not None and (best is None or b < best):
                best = b
    return best

def get_min_beta(rdict_list):
    beta = None
    for rdict in rdict_list:
        for key in ["usvp", "bdd"]:
            if key in rdict and hasattr(rdict[key], "__contains__") and "beta" in rdict[key]:
                b = rdict[key]["beta"]
                if beta is None or b < beta:
                    beta = b
    return beta

SKIP_DUAL   = ["dual", "dual_hybrid"]
SKIP_PRIMAL = ["usvp", "bdd", "bdd_hybrid", "bdd_mitm_hybrid", "bkw"]

# ═══════════════════════════════════════════════════════════════
# Run estimation
# ═══════════════════════════════════════════════════════════════
results = []
detail_logs = []   # per-profile details for markdown

def safe_estimate(params, red_cost_model, deny_list, label=""):
    """Wrap LWE.estimate with try/except to survive crashes (e.g. BKW NaN)."""
    try:
        return LWE.estimate(params, red_cost_model=red_cost_model,
                           deny_list=deny_list, catch_exceptions=True)
    except Exception as e:
        print(f"\n    [{label}] LWE.estimate crashed: {e}", flush=True)
        return {}

for name, target, n, q, eta_s, eta_r, d_pk, d_u in PROFILES:
    Xs = ND.CenteredBinomial(eta_s)
    Xr = ND.CenteredBinomial(eta_r)
    Xe_pk = zero_mean_uniform(d_pk)
    Xe_u  = zero_mean_uniform(d_u)

    pk = LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe_pk, m=n, tag=f"{name}-pk")
    u  = LWE.Parameters(n=n, q=q, Xs=Xr, Xe=Xe_u,  m=n, tag=f"{name}-u")

    t0 = time.time()

    print(f"\n{'='*65}")
    print(f"  {name}  (n={n}, q={q}, Δpk={d_pk}, Δu={d_u}, ηs={eta_s}, ηr={eta_r})")
    print(f"{'='*65}")

    # MATZOV: primal + BKW (skip dual/d-hyb).
    # For n >= 2048, also skip BKW (it crashes with NaN).
    matzov_skip = list(SKIP_DUAL)
    if n >= 2048:
        matzov_skip.append("bkw")
        print("  [MATZOV] primal only (BKW skipped for n≥2048) ...", end=" ", flush=True)
    else:
        print("  [MATZOV] primal + BKW ...", end=" ", flush=True)
    r_matz_pk = safe_estimate(pk, MATZOV, matzov_skip, label=f"{name}-pk/MATZOV")
    r_matz_u  = safe_estimate(u,  MATZOV, matzov_skip, label=f"{name}-u/MATZOV")
    print("done.", flush=True)

    # ADPS16: dual + d-hyb (skip primal/BKW)
    print("  [ADPS16] dual + d-hyb  ...", end=" ", flush=True)
    r_adps_pk = safe_estimate(pk, ADPS16, SKIP_PRIMAL, label=f"{name}-pk/ADPS16")
    r_adps_u  = safe_estimate(u,  ADPS16, SKIP_PRIMAL, label=f"{name}-u/ADPS16")
    print("done.", flush=True)

    dt = time.time() - t0

    all_m = [r_matz_pk, r_matz_u]
    all_a = [r_adps_pk, r_adps_u]

    primal = get_best_bits_multi(all_m, ["usvp", "bdd"])
    bkw    = get_best_bits_multi(all_m, ["bkw"])
    dual   = get_best_bits_multi(all_a, ["dual"])
    dhyb   = get_best_bits_multi(all_a, ["dual_hybrid"])
    beta_m = get_min_beta(all_m)

    matzov_c = primal
    matzov_q = 0.292 * beta_m if beta_m else None

    # Determine best attack
    best_algo = None
    best_bits = None
    for algo, bits in [("primal", primal), ("dual", dual), ("d-hyb", dhyb), ("BKW", bkw)]:
        if bits is not None and (best_bits is None or bits < best_bits):
            best_bits = bits
            best_algo = algo

    mgc = matzov_c - target if matzov_c else None
    mgq = matzov_q - target if matzov_q else None

    def ff(x):
        return f"{x:8.1f}" if x is not None else "      --"

    print(f"  primal={ff(primal)}  dual={ff(dual)}  d-hyb={ff(dhyb)}  BKW={ff(bkw)}")
    print(f"  MATZOV C={ff(matzov_c)}  MATZOV Q={ff(matzov_q)}  (β={beta_m})")
    print(f"  Best attack: {best_algo} ({ff(best_bits)})  [{dt:.0f}s]")

    results.append((name, target, n, q, d_pk, d_u, eta_s, eta_r,
                    primal, dual, dhyb, bkw, matzov_c, matzov_q,
                    beta_m, best_algo, best_bits, mgc, mgq))

    detail_logs.append({
        "name": name, "target": target, "n": n, "q": q,
        "d_pk": d_pk, "d_u": d_u, "eta_s": eta_s, "eta_r": eta_r,
        "primal": primal, "dual": dual, "dhyb": dhyb, "bkw": bkw,
        "matzov_c": matzov_c, "matzov_q": matzov_q,
        "beta": beta_m, "best_algo": best_algo, "best_bits": best_bits,
        "mgc": mgc, "mgq": mgq,
    })

# ═══════════════════════════════════════════════════════════════
# Console summary table
# ═══════════════════════════════════════════════════════════════
print()
print("=" * 125)
print("  MAMBA-NIKE  Consolidated Security Estimates  (MATZOV + Full Attacks)")
print("=" * 125)
hdr = (f"{'Scheme':>22s} {'Tgt':>4s} {'primal':>8s} {'dual':>8s} {'d-hyb':>8s} "
       f"{'BKW':>8s} {'MATZOV C':>9s} {'MATZOV Q':>9s} {'Mgn C':>7s} {'Mgn Q':>7s}  {'β':>6s}  Best Attack")
print(hdr)
print("-" * 125)

for r in results:
    name, target, n, q, d_pk, d_u, eta_s, eta_r, \
        pr, du, dh, bk, mc, mq, beta, best_algo, best_bits, mgc, mgq = r

    def ff(x):
        return f"{x:8.1f}" if x is not None else "      --"
    def ffs(x):
        return f"{x:+7.1f}" if x is not None else "     --"
    bstr = f"{beta:6d}" if beta else "    --"
    best_str = f"{best_algo} ({best_bits:.1f})" if best_algo else "--"

    print(f"{name:>22s} {target:>4d} {ff(pr)} {ff(du)} {ff(dh)} {ff(bk)} "
          f"{ff(mc)} {ff(mq)} {ffs(mgc)} {ffs(mgq)}  {bstr}  {best_str}")

print("-" * 125)
print("primal + BKW   → MATZOV cost model")
print("dual + d-hyb   → ADPS16 cost model (MATZOV dual not available)")
print("MATZOV C = min(usvp, bdd) under MATZOV")
print("MATZOV Q = 0.292 × β (Core-SVP classical, β from MATZOV primal)")
print("Error: zero-mean Uniform(−Δ/2, Δ/2), same variance as exact distribution")
print("=" * 125)

# ═══════════════════════════════════════════════════════════════
# Generate Markdown summary
# ═══════════════════════════════════════════════════════════════
now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
ts_str  = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

out_dir = os.path.join(PROJECT_ROOT, "estimator", "results")
os.makedirs(out_dir, exist_ok=True)

md_path = os.path.join(out_dir, f"NIKE_security_summary_{ts_str}.md")
txt_path = os.path.join(out_dir, f"NIKE_security_summary_{ts_str}.txt")

md = []
def W(s=""):
    md.append(s)

W("# MAMBA-NIKE 安全性估计总结")
W()
W(f"**生成时间**: {now_str}")
W(f"**脚本**: `estimator/scripts/run_full_estimation.sage`")
W(f"**参数来源**: `parameters.json`（唯一数据源）")
W()
W("## 估计配置")
W()
W("| 项目 | 设置 |")
W("|------|------|")
W(f"| 代价模型 | MATZOV (progressive BKZ + AGPS neural-network cost) |")
W(f"| 攻击覆盖 | primal (usvp/bdd) + BKW (MATZOV); dual + dual_hybrid (ADPS16) |")
W(f"| 模数 q | {Q} (2^{LOG2Q}) |")
W(f"| 错误分布 | zero-mean Uniform(−Δ/2, Δ/2) |")
W()
W("---")
W()
W("## 参数概览")
W()
W("| 方案 | 安全级别 | n | q | Δpk | Δu | ηs | ηr |")
W("|------|---------|---|----|-----|----|----|----|")
for item in pdata["profiles"]:
    W(f"| {item['name']} | {item['level']} | {item['n']} | {Q} | {item['delta_pk']} | {item['delta_u']} | {item['eta_s']} | {item['eta_r']} |")
W()
W("---")
W()
W("## 安全性估计结果")
W()
W("| 方案 | 目标 | primal | dual | d-hyb | BKW | MATZOV C | MATZOV Q | 裕度 C | 裕度 Q | β | 最佳攻击 |")
W("|------|------|--------|------|-------|-----|----------|----------|--------|--------|----|----------|")

for r in results:
    name, target, n, q, d_pk, d_u, eta_s, eta_r, \
        pr, du, dh, bk, mc, mq, beta, best_algo, best_bits, mgc, mgq = r

    def fmt_v(x):
        return f"{x:.1f}" if x is not None else "--"
    def fmt_m(x):
        return f"{x:+.1f}" if x is not None else "--"

    beta_str = str(beta) if beta else "--"
    best_str = f"{best_algo} ({fmt_v(best_bits)})" if best_algo else "--"

    W(f"| {name} | {target} | {fmt_v(pr)} | {fmt_v(du)} | {fmt_v(dh)} | {fmt_v(bk)} | {fmt_v(mc)} | {fmt_v(mq)} | {fmt_m(mgc)} | {fmt_m(mgq)} | {beta_str} | {best_str} |")

W()
W("> **列说明**:")
W("> - **MATZOV C** = min(usvp, bdd) under MATZOV（经典安全性估计）")
W("> - **MATZOV Q** = 0.292 × β（Core-SVP 经典估计，β 来自 MATZOV primal）")
W("> - **裕度 C/Q** = MATZOV C/Q − 目标安全级别（正值 = 安全）")
W("> - primal + BKW 使用 MATZOV 代价模型；dual + d-hyb 使用 ADPS16 模型")
W()
W("---")
W()
W("## 逐方案详细分析")
W()

for d in detail_logs:
    W(f"### {d['name']}（目标 {d['target']} bit）")
    W()
    W(f"| 参数 | 值 |")
    W(f"|------|----|")
    W(f"| n | {d['n']} |")
    W(f"| q | {d['q']} |")
    W(f"| Δpk | {d['d_pk']} |")
    W(f"| Δu | {d['d_u']} |")
    W(f"| ηs | {d['eta_s']} |")
    W(f"| ηr | {d['eta_r']} |")
    W()
    W(f"| 攻击类型 | 模型 | log₂(rop) |")
    W(f"|----------|------|-----------|")
    for label, model, val in [
        ("primal (usvp/bdd)", "MATZOV", d["primal"]),
        ("dual", "ADPS16", d["dual"]),
        ("dual_hybrid", "ADPS16", d["dhyb"]),
        ("BKW", "MATZOV", d["bkw"]),
    ]:
        vstr = f"{val:.1f}" if val is not None else "--"
        W(f"| {label} | {model} | {vstr} |")
    W()
    W(f"- **MATZOV β**: {d['beta'] if d['beta'] else '--'}")
    W(f"- **MATZOV C**: {d['matzov_c']:.1f}" if d["matzov_c"] else "- **MATZOV C**: --")
    W(f"- **MATZOV Q**: {d['matzov_q']:.1f}" if d["matzov_q"] else "- **MATZOV Q**: --")
    W(f"- **最佳攻击**: {d['best_algo']} ({d['best_bits']:.1f} bit)" if d["best_algo"] else "- **最佳攻击**: --")

    mgc_v = d["mgc"]
    mgq_v = d["mgq"]
    if mgc_v is not None:
        icon_c = "✅" if mgc_v >= 0 else "⚠️"
        W(f"- **安全裕度 (MATZOV C)**: {mgc_v:+.1f} bit {icon_c}")
    if mgq_v is not None:
        icon_q = "✅" if mgq_v >= 0 else "⚠️"
        W(f"- **安全裕度 (MATZOV Q)**: {mgq_v:+.1f} bit {icon_q}")
    W()

W("---")
W()
W("## 结论")
W()

min_mgc = min((r[17] for r in results if r[17] is not None), default=None)
min_mgq = min((r[18] for r in results if r[18] is not None), default=None)

if min_mgc is not None:
    W(f"- 所有方案 MATZOV C 最小安全裕度: **{min_mgc:+.1f} bit**")
if min_mgq is not None:
    W(f"- 所有方案 MATZOV Q 最小安全裕度: **{min_mgq:+.1f} bit**")

# Check which schemes pass
W()
W("### 通过情况")
W()
W("| 方案 | MATZOV C 通过 | MATZOV Q 通过 |")
W("|------|---------------|---------------|")
for r in results:
    mgc = r[17]
    mgq = r[18]
    c_ok = "✅" if mgc is not None and mgc >= 0 else ("⚠️" if mgc is not None else "--")
    q_ok = "✅" if mgq is not None and mgq >= 0 else ("⚠️" if mgq is not None else "--")
    W(f"| {r[0]} | {c_ok} | {q_ok} |")
W()

W("---")
W(f"*报告由 `run_full_estimation.sage` 自动生成于 {now_str}*")

# ── Write files ─────────────────────────────────────────────────────────
md_content = "\n".join(md)

with open(md_path, "w") as f:
    f.write(md_content)
print(f"\n📄 Markdown summary saved to: {md_path}")

with open(txt_path, "w") as f:
    f.write(md_content)
print(f"📄 Plain-text copy saved to: {txt_path}")

print("\n✅ All done.")
