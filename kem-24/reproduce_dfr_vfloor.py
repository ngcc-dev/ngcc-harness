#!/usr/bin/env python3
"""kem-24-3: Scabbard-256 decryption noise carries the nonnegative v residual.

Checks that Algorithm 10/the reference code floor the compressed v while the
submitted DFR program rounds to nearest, then measures the per-coefficient
noise m' - mu of honest Scabbard-256 encryptions, both as shipped (u
truncated, kem-24-1) and with the specified +h on u. The measured mean,
spread and tails are compared with an exact scalar model: 1152 independent
products of uniform rounding errors and CBD(2) coefficients, conditioned on
the residue the code produces, plus a uniform v residual in {0, 4, ..., 508}.
The submitted Minal decoder is characterized over the whole two-coordinate
noise grid: every symbol decodes when both coordinates lie in [-1020, 1020],
and none decodes when either coordinate lies outside [-1024, 1024]. One
coordinate outside the latter therefore fails the message whatever the other
coordinates do, so the model's probability p_lb of that event is a lower bound
on the model DFR, needing no independence assumption. The union-scale
estimate 128 p_ub, with p_ub from the sure-success interval, is printed for
comparison. Both are model values, not measured failure rates.

NGCC_LONG=1 measures ten times as many encryptions.
"""
import math
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import tempfile
from math import comb
from pathlib import Path

HERE = Path(__file__).resolve().parent
IMPL = HERE / "Implementations"
REF = IMPL / "Reference_Implementation"
N_TERMS, Q, STEP, RESIDUALS = 1152, 8192, 4, 128
SURE_OK = SURE_FAIL = None  # derived from the submitted decoder in decoder_interval()
V_LINE = ("v.coeffs[i] = ((v.coeffs[i] - m.coeffs[i]) & SCABBARD_Q) >> "
          "(SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));")
U_SHIPPED = "u.vec[i].coeffs[j] = (u.vec[i].coeffs[j]) & SCABBARD_Q;"
U_SPEC = "u.vec[i].coeffs[j] = (u.vec[i].coeffs[j] + h1) & SCABBARD_Q;"


def fail(msg):
    print(f"NOT CONFIRMED kem-24-3: {msg}")
    sys.exit(1)


def source_checks():
    for level in (128, 256, 512):
        src = (REF / f"scabbard{level}" / "indcpa.c").read_text()
        if V_LINE not in src or "h2" in src:
            fail(f"scabbard{level} v compression differs from the archived floor shift")
    spec = shutil.which("pdftotext")
    dfr = (IMPL / "Additional_Implementations/Security_and_DFR_Analysis/DFR_Computation/"
           "dfr_analysis/joint_distribution_2d.c").read_text()
    if not re.search(r"return MOD\(\(int\) round\(frac\), mod_to\);", dfr):
        fail("submitted DFR program no longer rounds to nearest in mod_switch")
    print("source: all three reference trees floor v with no rounding constant; "
          "the submitted DFR program models compression by nearest rounding")
    if spec:
        text = subprocess.run([spec, "-layout", str(HERE / "kem-24-spec.pdf"), "-"],
                              capture_output=True, text=True).stdout
        if "≫ (ϵq − ϵt − 2)" not in text or "∆m = ⟨e, s′ ⟩ − ⟨s′ , e⟩" not in text:
            fail("specification text differs (Algorithm 10 line 8 or the DFR noise term)")
        print("spec: Algorithm 10 line 8 floors v; §1.5 writes the noise as "
              "<e,s'> - <s',e> with no v-compression term")


def decoder_interval(scratch):
    global SURE_OK, SURE_FAIL
    exe = scratch / "decoder"
    r = subprocess.run(["gcc", "-O2", "-w", "-I.", "-o", str(exe), "minal.c",
                        str(HERE / "reproduce_dfr_decoder.c")],
                       cwd=REF / "scabbard256", capture_output=True, text=True)
    if r.returncode:
        print(r.stderr)
        fail("decoder build failed")
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    f = r.stdout.split()
    if r.returncode or len(f) != 8 or f[0] != "sure_success" or f[7] != "0":
        fail(f"submitted Minal decoder characterization failed: {r.stdout.strip()}")
    SURE_OK, SURE_FAIL = (int(f[1]), int(f[2])), (int(f[4]), int(f[5]))
    print(f"decoder: over all 16 symbols and the full grid, Scabbard-256 Minal decoding "
          f"succeeds when both coordinates lie in [{SURE_OK[0]}, {SURE_OK[1]}] and fails "
          f"when either lies outside [{SURE_FAIL[0]}, {SURE_FAIL[1]}]")


def model(mode):
    """Exact one-coefficient noise law {value: probability}, p_lb and p_ub."""
    us = range(-3, 1) if mode == "floor" else range(-1, 3)
    atom = {}
    for u in us:
        for b in range(-1, 3):
            for z in range(-2, 3):
                for zp in range(-2, 3):
                    e = u * z - b * zp
                    atom[e] = atom.get(e, 0) + comb(4, z + 2) * comb(4, zp + 2)
    low, high = min(atom), max(atom)
    a = [atom.get(e, 0) for e in range(low, high + 1)]
    d, n = len(a) - 1, N_TERMS
    c = [a[0] ** n]
    terms = [(j, x) for j, x in enumerate(a) if j and x]
    for k in range(1, d * n + 1):          # coefficients of A(x)^n, exactly
        s = sum(((n + 1) * j - k) * x * c[k - j] for j, x in terms if j <= k)
        q, r = divmod(s, k * a[0])
        assert r == 0 and q >= 0
        c.append(q)
    base = low * n
    first = (-base) % STEP
    mass = sum(c[first::STEP])
    law, failed_lb, failed_ub = {}, 0, 0
    for k in range(first, len(c), STEP):
        if not c[k]:
            continue
        for j in range(RESIDUALS):
            x = (base + k + STEP * j + Q // 2) % Q - Q // 2
            law[x] = law.get(x, 0) + c[k]
            if not SURE_FAIL[0] <= x <= SURE_FAIL[1]:
                failed_lb += c[k]
            if not SURE_OK[0] <= x <= SURE_OK[1]:
                failed_ub += c[k]
    total = mass * RESIDUALS
    return {x: w / total for x, w in law.items()}, failed_lb / total, failed_ub / total


def build(scratch, variant):
    tree = scratch / variant
    shutil.copytree(REF / "scabbard256", tree)
    if variant == "spec":
        path = tree / "indcpa.c"
        src = path.read_text()
        if U_SHIPPED not in src:
            fail("scabbard256 u rounding differs from the archived truncation")
        path.write_text(src.replace(U_SHIPPED, U_SPEC))
    exe = scratch / f"noise_{variant}"
    srcs = "auxfunc.c cbd.c drng.c indcpa.c packing.c poly.c polyvec.c verify.c minal.c".split()
    r = subprocess.run(["gcc", "-O2", "-w", "-I.", "-o", str(exe), *srcs,
                        str(HERE / "reproduce_dfr_noise.c"), "-lcrypto"],
                       cwd=tree, capture_output=True, text=True)
    if r.returncode:
        print(r.stderr)
        fail(f"{variant} build failed")
    return exe


def run_one(args):
    exe, keys, encs, seed = args
    out = subprocess.run([str(exe), str(keys), str(encs), str(seed)],
                         capture_output=True, text=True, check=True).stdout
    hist, fails, total = {}, 0, 0
    for line in out.splitlines():
        f = line.split()
        if f[0] == "H":
            hist[int(f[1])] = int(f[2])
        elif f[0] == "S":
            total = int(f[1].split("=")[1])
            fails = int(f[2].split("=")[1])
    return hist, total, fails


def measure(exe, workers, keys, encs):
    hist, total, fails = {}, 0, 0
    with multiprocessing.Pool(workers) as pool:
        for h, t, f in pool.map(run_one, [(exe, keys, encs, s + 1) for s in range(workers)]):
            total += t
            fails += f
            for k, v in h.items():
                hist[k] = hist.get(k, 0) + v
    return hist, total, fails


def moments(dist):
    tot = sum(dist.values())
    mean = sum(k * v for k, v in dist.items()) / tot
    sd = math.sqrt(sum((k - mean) ** 2 * v for k, v in dist.items()) / tot)
    return mean, sd


def main():
    if not shutil.which("gcc"):
        print("SKIP kem-24-3: gcc unavailable")
        sys.exit(77)
    source_checks()
    workers = max(1, min(16, os.cpu_count() or 1))
    long_run = os.environ.get("NGCC_LONG") == "1"
    keys, encs = (400, 250) if long_run else (200, 50)
    with tempfile.TemporaryDirectory() as tmp:
        scratch = Path(tmp)
        decoder_interval(scratch)
        for variant, mode in (("shipped", "floor"), ("spec", "nearest")):
            law, p, p_ub = model(mode)
            exe = build(scratch, variant)
            hist, total, fails = measure(exe, workers, keys, encs)
            n = sum(hist.values())
            em, es = moments(hist)
            mm, ms = moments(law)
            print(f"{variant} (u {'truncated' if mode == 'floor' else '+h, as specified'}): "
                  f"{total} encryptions, {n} coefficients, {fails} message failures")
            print(f"  noise mean/sd: measured {em:.1f}/{es:.1f}, model {mm:.1f}/{ms:.1f}")
            if abs(em - mm) > 5 or abs(es / ms - 1) > 0.02 or min(hist) < -es * 4:
                fail(f"{variant} noise moments do not match the model")
            for t in (500, 600, 700, 800):
                et = sum(v for k, v in hist.items() if k >= t)
                mt = sum(v for k, v in law.items() if k >= t)
                print(f"  P[noise >= {t}]: measured {et / n:.3e} ({et}), model {mt:.3e}")
                if mt * n >= 400 and not 0.8 <= (et / n) / mt <= 1.25:
                    fail(f"{variant} tail at {t} does not match the model")
            print(f"  model one-coefficient failure p_lb = 2^{math.log2(p):.2f} (a lower bound on "
                  f"the model DFR); union scale 128 p_ub = 2^{math.log2(128 * p_ub):.2f}; "
                  f"claimed delta for Scabbard-256: 2^-125")
            if variant == "shipped":
                shipped = math.log2(p)
            else:
                spec = math.log2(p)
    print(f"CONFIRMED kem-24-3: measured Scabbard-256 noise has the model's +254 mean offset "
          f"and well-populated tails; the model's one-coefficient failure probability, a lower "
          f"bound on its DFR, is 2^{spec:.1f} as specified and 2^{shipped:.1f} as implemented, "
          f"versus the claimed 2^-125")


if __name__ == "__main__":
    main()
