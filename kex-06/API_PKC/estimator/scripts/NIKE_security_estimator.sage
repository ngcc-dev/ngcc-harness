# ============================================================
# lwe_security_estimator.sage
#
# Standalone LWE security estimator for power-of-two modulus
# KEM schemes (e.g., MAMBA-Frost, MAMBA-Viper).
#
# Run:
#   sage lwe_security_estimator.sage
#
# Supports:
#   · Preset parameter sets (NIKE-128/192/256/384/512)
#   · Custom parameter entry via command-line or direct dict
#   · MATZOV, CoreSVP classical, and CoreSVP quantum cost models
#   · pk-surface and u-surface attack anchors
#
# This script does NOT compute DFR.
# ============================================================

import os
import sys
import math
import time
import datetime
from collections import OrderedDict

# ---------- Ensure estimator is importable ----------
_estimator_paths = [
    os.environ.get("LATTICE_ESTIMATOR_PATH", ""),
    os.getcwd(),
    os.path.abspath(os.path.join(os.getcwd(), "..")),
    "/home/dgs/lattice-estimator",
    "/home/dgs/Desktop/lattice-estimator",
]
for _p in _estimator_paths:
    if _p and os.path.isdir(_p) and _p not in sys.path:
        sys.path.insert(0, _p)

from estimator import *
from estimator.reduction import MATZOV as _MATZOV_CLASS, ADPS16 as _ADPS16_CLASS

# ============================================================
# User-configurable flags
# ============================================================

ESTIMATE_MODE = "both"        # "rough" | "full" | "both"
WIDTH_MODEL   = "variance"    # "variance" | "second_moment"
RUN_PRESETS   = True          # run the built-in Frost presets
RUN_CUSTOM    = False         # set True to also run CUSTOM_PARAMS below
QUIET_ESTIMATOR = False       # suppress per-attack estimator output
PRINT_ATTACK_TABLE = True     # print per-surface attack breakdown
LOG_ENABLE       = True       # write all output to a timestamped log file

# ============================================================
# Logging (tee stdout to file)
# ============================================================
class _Tee:
    """Duplicate sys.stdout writes to a log file."""
    def __init__(self, filepath):
        self.file = open(filepath, "w", buffering=1)
        self.stdout = sys.__stdout__

    def write(self, data):
        self.file.write(data)
        self.stdout.write(data)

    def flush(self):
        self.file.flush()
        self.stdout.flush()

    def close(self):
        self.file.close()

_log_tee = None

def log_setup(basename="nike_estimator"):
    """Open a timestamped log file in the current directory and tee stdout."""
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    path = os.path.join(os.getcwd(), "%s_%s.log" % (basename, ts))
    global _log_tee
    _log_tee = _Tee(path)
    sys.stdout = _log_tee
    return path

def log_close():
    """Restore stdout and close the log file."""
    global _log_tee
    if _log_tee is None:
        return
    sys.stdout = _log_tee.stdout
    _log_tee.close()
    _log_tee = None

# ---------- Custom parameter entry ----------
# Fill this dict when RUN_CUSTOM = True.
# Required keys (all powers of two for q, p_pk, p_u, p_v):
#   name, level, n, q, p_pk, p_u, p_v, eta_s, eta_r, b_msg, r_discret
CUSTOM_PARAMS = dict(
    name   = "Custom-256",
    level  = 256,
    n      = 1024,
    q      = 2**14,
    p_pk   = 2**11,
    p_u    = 2**11,
    p_v    = 2**6,
    eta_s  = 2,
    eta_r  = 2,
    b_msg  = 4,
    r_discret = 2,
)

# ============================================================
# Preset parameter sets (Frost family)
# ============================================================
PRESETS = OrderedDict([
    ("NIKE-128", dict(
        level=128,  n=1024,
        q=2**16,    p_pk=2**15, p_u=2**15, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=2,   r_discret=2,
    )),
    ("NIKE-192", dict(
        level=192,  n=1024,
        q=2**14,    p_pk=2**11, p_u=2**11, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=3,   r_discret=2,
    )),
    ("NIKE-256", dict(
        level=256,  n=1024,
        q=2**13,    p_pk=2**8,  p_u=2**8,  p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    )),
    ("NIKE-384", dict(
        level=384,  n=2048,
        q=2**16,    p_pk=2**13, p_u=2**13, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    )),
    ("NIKE-512", dict(
        level=512,  n=4096,
        q=2**13,    p_pk=2**10, p_u=2**10, p_v=2**6,
        eta_s=2,    eta_r=2,   b_msg=4,   r_discret=2,
    )),
])

# ============================================================
# Cost-model construction
# ============================================================
def _instantiate(cls):
    """Instantiate a cost-model class if it is a type, else return as-is."""
    if isinstance(cls, type):
        return cls()
    return cls

COST_MODELS = OrderedDict([
    ("MATZOV", dict(
        model   = _instantiate(_MATZOV_CLASS),
        note    = "MATZOV progressive BKZ with AGPS neural-network cost",
    )),
    ("CoreSVP classical", dict(
        model   = _ADPS16_CLASS(mode="classical"),
        note    = "ADPS16 classical Core-SVP  (0.292 * beta)",
    )),
    ("CoreSVP quantum", dict(
        model   = _ADPS16_CLASS(mode="quantum"),
        note    = "ADPS16 quantum Core-SVP    (0.265 * beta)",
    )),
])

# ============================================================
# Integer / power-of-two utilities
# ============================================================
def is_power_of_two(x):
    x = int(x)
    return x > 0 and (x & (x - 1)) == 0

def log2_pow2(x, label="value"):
    x = int(x)
    if not is_power_of_two(x):
        raise ValueError("%s must be a power of two, got %r" % (label, x))
    return int(round(math.log(x, 2)))

def ceil_log2(x):
    x = int(x)
    if x <= 0:
        raise ValueError("ceil_log2 expects positive integer")
    return int(math.ceil(math.log(x, 2)))

def exact_div(num, den, label):
    num, den = int(num), int(den)
    if num % den != 0:
        raise ValueError("%s not integral: %d / %d" % (label, num, den))
    return num // den

# ============================================================
# Parameter validation
# ============================================================
def validate_params(P):
    name = P.get("name", "unnamed")
    q = int(P["q"])
    if not is_power_of_two(q):
        raise ValueError("[%s] q must be a power of two" % name)

    for key in ["p_pk", "p_u", "p_v"]:
        p = int(P[key])
        if not is_power_of_two(p):
            raise ValueError("[%s] %s must be a power of two" % (name, key))
        if p >= q:
            raise ValueError("[%s] %s must be less than q" % (name, key))

# ============================================================
# Size computation
# ============================================================
def compute_sizes(P):
    n = int(P["n"])

    t_pk = log2_pow2(P["p_pk"], "p_pk")
    t_u  = log2_pow2(P["p_u"],  "p_u")
    t_v  = log2_pow2(P["p_v"],  "p_v")
    r_discret = P["r_discret"]

    sk_coeff_bits = ceil_log2(2 * int(P["eta_s"]) + 1)

    seed_pk = 32;  seed_ct = 32;  kem_extra = 64

    pk_body_bits = n * t_pk
    ct_u_bits    = n * t_u
    ct_v_bits    = n * t_v
    sk_pke_bits  = n * sk_coeff_bits
    helper_bits    = n * 2**r_discret

    pk_bytes  = seed_pk  + exact_div(pk_body_bits, 8, "pk body")
    ct_bytes  = seed_ct  + exact_div(ct_u_bits,    8, "ct u") \
                         + exact_div(helper_bits,  8, "ct h")
    sk_bytes  = exact_div(sk_pke_bits, 8, "PKE secret") + pk_bytes + kem_extra
    ss_bytes  = exact_div(int(P["level"]), 8, "ss")

    return dict(
        t_pk=t_pk, t_u=t_u, t_v=t_v,
        r_discret=r_discret,
        sk_coeff_bits=sk_coeff_bits,
        pk_bytes=pk_bytes, ct_bytes=ct_bytes,
        sk_bytes=sk_bytes, ss_bytes=ss_bytes,
        pkct_bytes=pk_bytes + ct_bytes,
        helper_bytes=exact_div(helper_bits, 8, "ct h"),
        fo_bits=int(P["b_msg"]),
    )

# ============================================================
# Distribution utilities
# ============================================================
def cbd_variance(eta):
    return float(eta) / 2.0

def cbd_stddev(eta):
    return math.sqrt(cbd_variance(eta))

def qerr_delta(q, p):
    q, p = int(q), int(p)
    if q % p != 0:
        raise ValueError("p must divide q: q=%d, p=%d" % (q, p))
    return q // p

def qerr_mean(delta):
    return 0.5

def qerr_variance(delta):
    delta = float(delta)
    return (delta * delta - 1.0) / 12.0

def qerr_second_moment(delta):
    delta = float(delta)
    return (delta * delta + 2.0) / 12.0

def qerr_width(delta):
    if WIDTH_MODEL == "variance":
        return math.sqrt(qerr_variance(delta))
    if WIDTH_MODEL == "second_moment":
        return math.sqrt(qerr_second_moment(delta))
    raise ValueError("Unknown WIDTH_MODEL: %s" % WIDTH_MODEL)

def qerr_support(delta):
    delta = int(delta)
    if delta % 2 != 0:
        raise ValueError("quantization delta must be even, got %d" % delta)
    h = delta // 2
    return -h + 1, h

# ============================================================
# Build ND distributions (with fallback to variance-matched Gaussian)
# ============================================================
def make_gaussian(sigma):
    sigma = float(sigma)
    errors = []
    for ctor in [
        lambda: ND.DiscreteGaussian(stddev=sigma),
        lambda: ND.DiscreteGaussian(sigma=sigma),
        lambda: ND.DiscreteGaussian(sigma),
    ]:
        try:
            return ctor()
        except Exception as e:
            errors.append(repr(e))
    raise RuntimeError("Cannot construct DiscreteGaussian(%r): %s" % (sigma, errors))

def make_uniform_qerr(delta):
    a, b = qerr_support(delta)
    for ctor in [
        lambda: ND.Uniform(a, b),
        lambda: ND.Uniform(lb=a, ub=b),
    ]:
        try:
            return ctor()
        except Exception:
            pass
    # Fallback: variance-matched Gaussian
    print("  [WARN] ND.Uniform(%d,%d) unavailable; using variance-matched Gaussian" % (a, b))
    return make_gaussian(qerr_width(delta))

def make_cbd(eta):
    eta = int(eta)
    for ctor in [
        lambda: ND.CenteredBinomial(eta),
        lambda: ND.CenteredBinomial(k=eta),
    ]:
        try:
            return ctor()
        except Exception:
            pass
    print("  [WARN] ND.CenteredBinomial(%d) unavailable; using Gaussian fallback" % eta)
    return make_gaussian(cbd_stddev(eta))

def make_lwe_params(n, q, Xs, Xe, m, tag):
    for ctor in [
        lambda: LWEParameters(n=n, q=q, Xs=Xs, Xe=Xe, m=m, tag=tag),
        lambda: LWE.Parameters(n=n, q=q, Xs=Xs, Xe=Xe, m=m, tag=tag),
    ]:
        try:
            return ctor()
        except Exception:
            pass
    raise RuntimeError("Cannot construct LWE params for %s" % tag)

# ============================================================
# Parse estimator output
# ============================================================
import re
ROP_RE = re.compile(r"rop:\s*(?:[^2]*?)2\^([0-9.+\-]+)")

def record_rop_bits(record):
    """Extract log2(rop) from an estimator result record."""
    if record is None:
        return None

    # Try dict-like access
    try:
        val = record["rop"]
        s = str(val)
        m = ROP_RE.search(s)
        if m:
            return float(m.group(1))
        return float(log(RR(val), 2))
    except Exception:
        pass

    # Try string representation
    s = str(record)
    m = ROP_RE.search(s)
    if m:
        return float(m.group(1))

    return None

def best_attack(result):
    """Return (name, log2_rop) of the most efficient attack."""
    best_name, best_bits = None, None
    for name, rec in result.items():
        bits = record_rop_bits(rec)
        if bits is None:
            continue
        if best_bits is None or bits < best_bits:
            best_bits, best_name = bits, str(name)
    return best_name, best_bits

# ============================================================
# Estimator runner
# ============================================================
def run_estimator(params, red_cost_model):
    """Run LWE.estimate / LWE.estimate.rough and return a dict
       {mode_name: {algo_name: cost_record, ...}}."""
    if red_cost_model is None:
        raise ValueError("red_cost_model is required")

    out = OrderedDict()

    if ESTIMATE_MODE in ("rough", "both"):
        res = LWE.estimate.rough(
            params,
            catch_exceptions=True, quiet=QUIET_ESTIMATOR,
        )
        out["rough"] = res

    if ESTIMATE_MODE in ("full", "both"):
        res = LWE.estimate(
            params, red_cost_model=red_cost_model,
            catch_exceptions=True, quiet=QUIET_ESTIMATOR,
        )
        out["full"] = res

    if ESTIMATE_MODE not in ("rough", "full", "both"):
        raise ValueError("Unknown ESTIMATE_MODE: %s" % ESTIMATE_MODE)

    return out

# ============================================================
# Estimate a single attack surface
# ============================================================
def estimate_surface(tag, lwe_n, q, m_samples, eta_secret,
                     sigma_error, Xe=None, xe_model="variance-matched Gaussian"):
    Xs = make_cbd(eta_secret)
    if Xe is None:
        Xe = make_gaussian(sigma_error)
    params = make_lwe_params(n=lwe_n, q=q, Xs=Xs, Xe=Xe, m=m_samples, tag=tag)

    print("")
    print("  Surface: %s" % tag)
    print("    RLWE dim n = %d,  samples m = %d,  q = %s" %
          (lwe_n, m_samples, fmt_pow2(q)))
    print("    secret = CBD(%d)   (std = %.4f)" %
          (eta_secret, cbd_stddev(eta_secret)))
    print("    error  = %s   (sigma = %.4f)" %
          (xe_model, sigma_error))

    selected = OrderedDict()
    for cm_name, cm in COST_MODELS.items():
        try:
            results = run_estimator(params, red_cost_model=cm["model"])
        except Exception as e:
            print("    [%s] ERROR: %s" % (cm_name, repr(e)))
            selected[cm_name] = None
            continue

        selected[cm_name] = OrderedDict()
        for mode, result in results.items():
            best_name, best_bits = best_attack(result)
            selected[cm_name][mode] = dict(best=best_name, bits=best_bits)

            if PRINT_ATTACK_TABLE and result:
                print("    [%s / %s]  best: %s  (2^%.2f)" %
                      (cm_name, mode, best_name, best_bits))
                for algo, rec in sorted(result.items()):
                    b = record_rop_bits(rec)
                    if b is not None:
                        print("        %-24s  rop = 2^%.2f" % (algo, b))

    return selected

# ============================================================
# Compute surface widths
# ============================================================
def surface_widths(P):
    q = int(P["q"])
    dpk = qerr_delta(q, P["p_pk"])
    du  = qerr_delta(q, P["p_u"])
    dv  = qerr_delta(q, P["p_v"])
    return dict(
        delta_pk=dpk, delta_u=du, delta_v=dv,
        sigma_pk=qerr_width(dpk),
        sigma_u =qerr_width(du),
        sigma_v =qerr_width(dv),
    )

# ============================================================
# Formatting helpers
# ============================================================
def fmt_pow2(x):
    try:
        if is_power_of_two(x):
            return "2^%d" % log2_pow2(x)
    except Exception:
        pass
    return str(x)

def fmt_bits(x):
    return "NA" if x is None else "%.2f" % float(x)

# ============================================================
# Pretty-print table
# ============================================================
def print_table(title, headers, rows, indent=""):
    print("")
    if title:
        print(indent + title)
    rows = [[str(c) for c in r] for r in rows]
    headers = [str(h) for h in headers]
    if not rows:
        print(indent + "  <empty>")
        return
    widths = []
    for j, h in enumerate(headers):
        w = len(h)
        for r in rows:
            if j < len(r):
                w = max(w, len(r[j]))
        widths.append(w)
    def render(r):
        return indent + "  " + "  ".join(
            r[j].ljust(widths[j]) for j in range(len(headers)))
    print(render(headers))
    print(indent + "  " + "  ".join("-" * w for w in widths))
    for r in rows:
        print(render(r))

# ============================================================
# Pick best record across surfaces for a given cost model
# ============================================================
def best_record_for_model(selected_bits, cost_model_name):
    flat = []
    for surf_name, out in selected_bits.items():
        if out is None:
            continue
        if cost_model_name not in out:
            continue
        if out[cost_model_name] is None:
            continue
        for mode, rec in out[cost_model_name].items():
            bits = rec["bits"]
            if bits is None:
                continue
            flat.append((bits, surf_name, mode, rec["best"]))
    if not flat:
        return None
    flat.sort(key=lambda x: x[0])
    return flat[0]

# ============================================================
# Evaluate a single parameter set
# ============================================================
def evaluate_one(P):
    name = P["name"]
    validate_params(P)

    n = int(P["n"])
    q = int(P["q"])

    sizes  = compute_sizes(P)
    widths = surface_widths(P)

    # ---------- header ----------
    print("")
    print("=" * 72)
    print("Profile: %s   (target %d-bit)" % (name, P["level"]))
    print("=" * 72)

    # ---------- parameter table ----------
    print_table("Parameter summary", ["Item", "Value"], [
        ["target",               P["level"]],
        ["n (ring dim)",         n],
        ["q",                    fmt_pow2(q)],
        ["eta_s / eta_r",        "%d / %d" % (P["eta_s"], P["eta_r"])],
        ["b_msg",                P["b_msg"]],
        ["p_pk / p_u / p_v",     "%s / %s / %s" % (
            fmt_pow2(P["p_pk"]), fmt_pow2(P["p_u"]), fmt_pow2(P["p_v"]))],
        ["t_pk / t_u / t_v",     "%d / %d / %d" %
            (sizes["t_pk"], sizes["t_u"], sizes["t_v"])],
        ["FO plaintext bits",    sizes["fo_bits"]],
    ], indent="  ")

    # ---------- size table ----------
    print_table("Key and ciphertext sizes (bytes)", [
        "pk", "ct", "pk+ct", "sk", "ss", "sk coeff bits",
    ], [[
        str(sizes["pk_bytes"]),
        str(sizes["ct_bytes"]),
        str(sizes["pkct_bytes"]),
        str(sizes["sk_bytes"]),
        str(sizes["ss_bytes"]),
        str(sizes["sk_coeff_bits"]),
    ]], indent="  ")

    # ---------- quantization-error table ----------
    print_table("Quantization-error widths", [
        "Comp.", "Delta", "Mean", "Variance", "E[e^2]", "Width (used)",
    ], [
        ["pk", str(widths["delta_pk"]),
         "%.4f" % qerr_mean(widths["delta_pk"]),
         "%.4f" % qerr_variance(widths["delta_pk"]),
         "%.4f" % qerr_second_moment(widths["delta_pk"]),
         "%.4f" % widths["sigma_pk"]],
        ["u",  str(widths["delta_u"]),
         "%.4f" % qerr_mean(widths["delta_u"]),
         "%.4f" % qerr_variance(widths["delta_u"]),
         "%.4f" % qerr_second_moment(widths["delta_u"]),
         "%.4f" % widths["sigma_u"]],
        ["v",  str(widths["delta_v"]),
         "%.4f" % qerr_mean(widths["delta_v"]),
         "%.4f" % qerr_variance(widths["delta_v"]),
         "%.4f" % qerr_second_moment(widths["delta_v"]),
         "%.4f" % widths["sigma_v"]],
    ], indent="  ")

    # ---------- estimate surfaces ----------
    selected_bits = OrderedDict()

    # pk surface: recover secret s from public key
    print("\n  --- pk surface (recover secret s from pk) ---")
    out_pk = estimate_surface(
        tag="%s-pk" % name,
        lwe_n=n, q=q, m_samples=n,
        eta_secret=P["eta_s"],
        sigma_error=widths["sigma_pk"],
        Xe=make_uniform_qerr(widths["delta_pk"]),
        xe_model="uniform chi_{q,p_pk}",
    )
    selected_bits["pk"] = out_pk

    # u surface: recover randomness r from ciphertext U
    print("\n  --- u surface (recover randomness r from ct U) ---")
    out_u = estimate_surface(
        tag="%s-u" % name,
        lwe_n=n, q=q, m_samples=n,
        eta_secret=P["eta_r"],
        sigma_error=widths["sigma_u"],
        Xe=make_uniform_qerr(widths["delta_u"]),
        xe_model="uniform chi_{q,p_u}",
    )
    selected_bits["u"] = out_u

    # ---------- per-cost-model summary ----------
    print_table("Per cost-model selected records", [
        "Model", "Surface", "Mode", "Attack", "log2(rop)",
    ], [
        [cm_name,
         best_record_for_model(selected_bits, cm_name)[1],
         best_record_for_model(selected_bits, cm_name)[2],
         best_record_for_model(selected_bits, cm_name)[3],
         fmt_bits(best_record_for_model(selected_bits, cm_name)[0])]
        if best_record_for_model(selected_bits, cm_name) is not None
        else [cm_name, "NA", "NA", "NA", "NA"]
        for cm_name in COST_MODELS
    ], indent="  ")

    # ---------- collect for final summary ----------
    row = [name, P["level"], n,
           fmt_pow2(q), sizes["t_pk"], sizes["t_u"], sizes["t_v"],
           sizes["pk_bytes"], sizes["ct_bytes"],
           sizes["pkct_bytes"], sizes["sk_bytes"], sizes["ss_bytes"]]

    sec = OrderedDict()
    for cm_name in COST_MODELS:
        best_rec = best_record_for_model(selected_bits, cm_name)
        if best_rec is not None:
            sec[cm_name] = best_rec[0]
        else:
            sec[cm_name] = None

    return row, sec

# ============================================================
# Main
# ============================================================
def main():
    # ---------- start logging ----------
    t_start = time.time()
    if LOG_ENABLE:
        log_path = log_setup()
        print("Log file: %s" % log_path)
        print("")

    print("=" * 72)
    print("  RLWE Security Estimator  (power-of-two modulus KEX)")
    print("=" * 72)
    print("  Mode: %s | Width model: %s" % (ESTIMATE_MODE, WIDTH_MODEL))
    print("")

    print_table("Cost models", ["Model", "Description"], [
        [name, cm["note"]] for name, cm in COST_MODELS.items()
    ])

    size_rows   = []
    security_by = OrderedDict()

    # ---------- presets ----------
    if RUN_PRESETS:
        for name, P0 in PRESETS.items():
            P = dict(P0)
            P["name"] = name
            row, sec = evaluate_one(P)
            size_rows.append(row)
            security_by[name] = sec

    # ---------- custom ----------
    if RUN_CUSTOM:
        P = dict(CUSTOM_PARAMS)
        if not P.get("name"):
            P["name"] = "Custom"
        row, sec = evaluate_one(P)
        size_rows.append(row)
        security_by[P["name"]] = sec

    # ============================================================
    # Final summary tables
    # ============================================================
    print("")
    print("=" * 72)
    print("FINAL SUMMARY")
    print("=" * 72)

    print_table("Sizes and parameters", [
        "Scheme", "Target", "n", "q",
        "t_pk", "t_u", "t_v", "pk", "ct", "pk+ct", "sk", "ss",
    ], size_rows)

    sec_rows = []
    for row in size_rows:
        scheme = row[0]
        target = row[1]
        sec = security_by.get(scheme, OrderedDict())
        sec_rows.append([
            scheme, target,
            fmt_bits(sec.get("MATZOV")),
            fmt_bits(sec.get("CoreSVP classical")),
            fmt_bits(sec.get("CoreSVP quantum")),
        ])

    print_table("Security bits (lower = weaker, i.e. cheapest attack)", [
        "Scheme", "Target", "MATZOV", "CoreSVP cl.", "CoreSVP q.",
    ], sec_rows)

    # ---------- runtime ----------
    elapsed = time.time() - t_start
    print("")
    print("=" * 72)
    print("Total runtime: %.2f seconds  (%.2f minutes)" % (elapsed, elapsed / 60.0))
    print("=" * 72)

    if LOG_ENABLE:
        log_close()

if __name__ == "__main__":
    main()
