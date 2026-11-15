#!/usr/bin/env python3
"""Reproduce SDitH-4: the branch of div_rem_u32_ct_ref at the rsd.c:264
call site (rsd_encode_solution_ref) determines the SDitH secret witness.

The script builds the submitted Reference_Implementation (cat1_fast,
cat3_fast, cat5_fast) into a temporary directory with an instrumented copy of
ct_utils.c. The copy only records, per loop round of div_rem_u32_ct_ref,
whether the `if (rem >= public_d)` branch was taken (an emulated perfect
branch trace). Honest key generation is run for three fixed seeds per
category. From the trace of the rsd.c:264 call, the public key and the public
matrix H (expanded from the public seed by the submitted code), pure-Python
GF(2) linear algebra recovers the secret witness positions. Without the trace
the same linear system is underdetermined (control).

No timing or physical measurement is made. Only the reference helper is
affected; the AVX2 helper div_rem_u32_ct_avx uses cmp/sbb masks.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
PKG = HERE / "submission/submission_package"
REF = PKG / "Reference_Implementation"
OPT = PKG / "Optimized_Implementation"
SETS = ("cat1_fast", "cat3_fast", "cat5_fast")
SEEDS = (101, 102, 103)

# Pinned submission files (byte-identical across all sets of a tree).
PINNED_SHA256 = {
    "Reference_Implementation/*/src/ct_utils.c":
        "eebc30587162374c4070be707ab6d0f07e2af5d3f23ae673144659728b5b43ed",
    "Reference_Implementation/*/src/rsd.c":
        "82241402285599d2ad7e516819e6c7eca5a631b90dfd6bf455c8a22d2ea56ceb",
    "Optimized_Implementation/*/src/ct_utils_avx2.c":
        "ae609d4ae4880fa8ea40be75db204da72a8624d785a21274763553306855c11f",
}

LIB_SRC = (
    "src/commons.c src/sdith_prng.c src/gf256.c src/gf192.c "
    "src/matrix_vector_products_f2.c src/matrix_transpose_f2.c src/ggm.c "
    "src/vole_expansion.c src/piop_circuit.c src/gf128.c src/rsd.c "
    "src/sdith_signature.c src/sdith_signature_parameters.c src/vole_to_piop.c "
    "src/vole_parameters.c generator/sign.c "
    "lib/aes/rijndael256.c lib/aes/aes_ansi_littleendian_ref.c "
    "lib/aes/aes128_ctrle_special_ref.c lib/aes/rijndael256_ctrle_special_ref.c "
    "lib/sha3/KeccakHash.c lib/sha3/KeccakSponge.c lib/sha3/KeccakSpongetimes4.c "
    "lib/sha3/KeccakHashtimes4.c lib/sha3/opt64/KeccakP-1600-opt64.c "
    "lib/sha3/opt64/KeccakP-1600-times4-on1.c"
).split()

# Instrumentation of the helper (applied to a copy in the temporary directory).
# Each anchor must occur exactly once in the pristine ct_utils.c.
CT_PATCHES = [
    ('#include "vole_private.h"\n',
     '#include "vole_private.h"\n'
     "/* REPRO instrumentation: emulated branch trace */\n"
     "void sdith_trace_call(uint32_t secret_n, uint32_t public_d);\n"
     "void sdith_trace_iter(void);\n"
     "void sdith_trace_taken(void);\n"
     "void sdith_trace_end(void);\n"),
    ('  CREQUIRE(public_d != 0, "division by zero");\n',
     '  CREQUIRE(public_d != 0, "division by zero");\n'
     "  sdith_trace_call(secret_n, public_d);\n"),
    ("    rem = (rem << 1) | ((secret_n >> i) & 1);\n",
     "    rem = (rem << 1) | ((secret_n >> i) & 1);\n"
     "    sdith_trace_iter();\n"),
    ("      rem -= public_d;\n",
     "      sdith_trace_taken();\n"
     "      rem -= public_d;\n"),
    ("  *q = quo;\n",
     "  sdith_trace_end();\n"
     "  *q = quo;\n"),
]

DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "api.h"
#include "sdith_signature.h"
#include "vole_private.h"
#include "sdith_rsd.h"

/* deterministic randombytes (splitmix64), replaces the NIST DRBG */
static uint64_t rs;
static uint64_t sm64(void) {
  uint64_t z = (rs += UINT64_C(0x9e3779b97f4a7c15));
  z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
  return z ^ (z >> 31);
}
int randombytes(unsigned char* x, unsigned long long n) { while (n--) *x++ = (unsigned char)sm64(); return 0; }

/* trace sink: one line per helper call. The attacker-visible part is the
 * public divisor and the 32 branch outcomes (MSB-first loop order); the
 * dividend is printed separately as ground truth for the checks only. */
static FILE* tf; static int tracing;
static uint32_t c_n, c_d, c_bits, c_iters, c_taken_this_iter;
void sdith_trace_call(uint32_t n, uint32_t d) { if (!tracing) return; c_n = n; c_d = d; c_bits = 0; c_iters = 0; }
void sdith_trace_iter(void) { if (!tracing) return; c_bits <<= 1; c_iters++; }
void sdith_trace_taken(void) { if (!tracing) return; c_bits |= 1; }
void sdith_trace_end(void) { if (!tracing) return; fprintf(tf, "T %u %u %u truth %u\n", c_d, c_bits, c_iters, c_n); }

static void dump(const char* dir, const char* name, const void* p, size_t n) {
  char fn[4096]; snprintf(fn, sizeof fn, "%s/%s", dir, name);
  FILE* f = fopen(fn, "wb"); if (!f || fwrite(p, 1, n, f) != n) { perror(fn); exit(3); } fclose(f);
}

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  const signature_parameters* P = &SIGNATURE_PARAMS;
  char fn[4096]; snprintf(fn, sizeof fn, "%s/trace.txt", argv[2]);
  tf = fopen(fn, "w"); if (!tf) return 3;
  rs = strtoull(argv[1], 0, 0);
  tracing = 1;
  if (crypto_sign_keypair(pk, sk)) return 4;
  tracing = 0;
  fclose(tf);
  extended_parameters_t par; compute_extended_parameters(&par, P);
  uint64_t nmk = par.rsd_n - par.rsd_codim, slice = par.rsd_codim_slice, cb = (par.rsd_codim + 7) / 8;
  uint8_t* h = aligned_alloc(32, ((nmk * slice + 31) / 32) * 32);
  rsd_expand_public_key_ref(par.rsd_w, par.rsd_n, par.rsd_codim, &par.vole_params,
                            (bitmat_t*)h, slice, (const seed_t*)pk);   /* public: from seedpk */
  uint8_t* hc = malloc(nmk * cb);
  for (uint64_t i = 0; i < nmk; i++) memcpy(hc + i * cb, h + i * slice, cb);
  dump(argv[2], "pk.bin", pk, sizeof pk);
  dump(argv[2], "sk.bin", sk, sizeof sk);
  dump(argv[2], "H.bin", hc, nmk * cb);
  printf("lambda %lu n %lu w %lu codim %lu depth %lu arities",
         (unsigned long)par.lambda, (unsigned long)par.rsd_n, (unsigned long)par.rsd_w,
         (unsigned long)par.rsd_codim, (unsigned long)P->mux_depth);
  for (uint64_t j = 0; j < P->mux_depth; j++) printf(" %lu", (unsigned long)P->mux_arities[j]);
  printf("\n");
  return 0;
}
"""


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(cmd, timeout=300, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, **kw)
    if r.returncode:
        raise RuntimeError(f"command failed ({r.returncode}): {' '.join(map(str, cmd))}\n{r.stdout}{r.stderr}")
    return r


def emit(result: dict, output: Path | None) -> None:
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")


def build(gcc: str, work: Path, name: str) -> Path:
    tree = REF / name
    src = work / name
    shutil.copytree(tree, src)
    ct = src / "src/ct_utils.c"
    text = ct.read_text()
    for old, new in CT_PATCHES:
        if text.count(old) != 1:
            raise RuntimeError(f"anchor not unique in ct_utils.c: {old!r}")
        text = text.replace(old, new)
    ct.write_text(text)
    (src / "driver.c").write_text(DRIVER)
    inc = [f"-I{src}/src", f"-I{src}/generator", f"-I{src}/lib/aes", f"-I{src}/lib/sha3",
           f"-I{src}/lib/sha3/opt64"]
    flags = ["-O2", "-std=gnu11", "-DNDEBUG", "-DONLY_REF_IMPLEMENTATION", *inc]
    objs = []
    jobs = []
    for rel in LIB_SRC + ["src/ct_utils.c", "driver.c"]:
        obj = src / (rel.replace("/", "_") + ".o")
        objs.append(obj)
        jobs.append([gcc, *flags, "-c", str(src / rel), "-o", str(obj)])
    with ThreadPoolExecutor(max_workers=3) as ex:
        list(ex.map(run, jobs))
    exe = work / f"keygen_{name}"
    run([gcc, *map(str, objs), "-o", str(exe), "-lm"])
    return exe


def divrem_trace(n: int, d: int):
    """Transcription of ct_utils.c:27-39 returning (q, r, branch outcomes as an MSB-first int)."""
    rem = quo = bits = 0
    for i in range(31, -1, -1):
        rem = (rem << 1) | ((n >> i) & 1)
        b = rem >= d
        bits = (bits << 1) | int(b)
        if b:
            rem -= d
            quo |= 1 << i
    return quo, rem, bits


def gf2_solve(rows, nv):
    """rows: list of (mask, rhs). Returns (rank, solution or None, consistent)."""
    piv = {}
    for mask, rhs in rows:
        while mask:
            b = mask.bit_length() - 1
            if b in piv:
                pm, pr = piv[b]
                mask ^= pm
                rhs ^= pr
            else:
                piv[b] = (mask, rhs)
                break
        else:
            if rhs:
                return len(piv), None, False
    rank = len(piv)
    if rank != nv:
        return rank, None, True
    sol = [0] * nv
    for b in sorted(piv):  # back substitution, low pivots first
        pm, pr = piv[b]
        v = pr
        rest = pm ^ (1 << b)
        while rest:
            lb = rest & -rest
            v ^= sol[lb.bit_length() - 1]
            rest ^= lb
        sol[b] = v
    return rank, sol, True


def analyse(name: str, seed: int, out: Path, info: dict) -> dict:
    lam, n, w, codim = info["lambda"], info["n"], info["w"], info["codim"]
    mu = info["arities"]
    m = n // w
    lb = lam // 8
    cb = (codim + 7) // 8
    pk = (out / "pk.bin").read_bytes()
    sk = (out / "sk.bin").read_bytes()
    hb = (out / "H.bin").read_bytes()
    y = int.from_bytes(pk[lb:], "little")

    def col(j):
        return (1 << j) if j < codim else int.from_bytes(hb[(j - codim) * cb:(j - codim + 1) * cb], "little")

    # ground truth: decode the secret witness from sk (code convention rsd.c:259-272)
    wb = int.from_bytes(sk[lb + cb:], "little")
    mi = sum(u - 1 for u in mu)
    pos = []
    for i in range(w):
        bp = i * mi
        p = 0
        mul = 1
        for u in mu:
            dig = 0
            for k in range(u - 1):
                if (wb >> (bp + k)) & 1:
                    dig = k + 1
            p += dig * mul
            mul *= u
            bp += u - 1
        pos.append(p)
    acc = 0
    for i, p in enumerate(pos):
        acc ^= col(i * m + p)
    hx_equals_y = acc == y

    # parse the trace
    recs = []
    for line in (out / "trace.txt").read_text().split("\n"):
        if line.startswith("T "):
            t = line.split()
            recs.append((int(t[1]), int(t[2]), int(t[3]), int(t[5])))
    depth = len(mu)
    site63 = recs[:w]
    site264 = recs[w:]
    layout_ok = (len(recs) == w + w * depth
                 and all(r[0] == m and r[2] == 32 for r in site63)
                 and all(site264[i * depth + j][0] == mu[j] and site264[i * depth + j][2] == 32
                         for i in range(w) for j in range(depth)))
    # rsd.c:63: outcomes are the quotient of the 32-bit draw; the remainder is the position
    site63_quotient = all(r[1] == r[3] // m for r in site63)
    site63_rem_is_pos = [r[3] % m for r in site63] == pos
    # rsd.c:264: the first-level outcomes are pos // mu_1, and the whole per-chunk
    # trace is a function of pos // mu_1 alone
    leak = [site264[i * depth][1] for i in range(w)]
    level1_is_pos_div_mu1 = leak == [p // mu[0] for p in pos]

    def chunk_trace(p):
        tr = []
        for u in mu:
            p, _, bits = divrem_trace(p, u)
            tr.append(bits)
        return tr

    trace_matches_emulation = all(
        [site264[i * depth + j][1] for j in range(depth)] == chunk_trace(pos[i]) for i in range(w))
    trace_function_of_quotient = all(
        chunk_trace(p) == chunk_trace(mu[0] * (p // mu[0])) for p in pos)

    # attacker: candidates mu1*q + t < m, one-hot per chunk, y = Hx
    cand = [[mu[0] * q + t for t in range(mu[0]) if mu[0] * q + t < m] for q in leak]
    vars_ = [(i, c) for i in range(w) for c in cand[i]]
    nv = len(vars_)
    rows = []
    for r in range(codim):
        mask = 0
        for v, (i, c) in enumerate(vars_):
            if (col(i * m + c) >> r) & 1:
                mask |= 1 << v
        rows.append((mask, (y >> r) & 1))
    for i in range(w):
        mask = 0
        for v, (ii, _) in enumerate(vars_):
            if ii == i:
                mask |= 1 << v
        rows.append((mask, 1))
    rank, sol, consistent = gf2_solve(rows, nv)
    rec = None
    if sol is not None:
        rec = [None] * w
        for v, (i, c) in enumerate(vars_):
            if sol[v]:
                rec[i] = c
    recovered = rec == pos

    # control: no trace -> every position in [0, m) is a candidate
    crow = []
    nv0 = w * m
    colcache = [col(j) for j in range(n)]
    for r in range(codim):
        mask = 0
        for j in range(n):
            if (colcache[j] >> r) & 1:
                mask |= 1 << j
        crow.append((mask, (y >> r) & 1))
    for i in range(w):
        crow.append((((1 << m) - 1) << (i * m), 1))
    rank0, _, consistent0 = gf2_solve(crow, nv0)

    return {
        "set": name, "seed": seed,
        "hx_equals_y": hx_equals_y,
        "trace_layout_ok": layout_ok,
        "site63_outcomes_are_draw_quotient": site63_quotient,
        "site63_remainder_is_position": site63_rem_is_pos,
        "site264_level1_outcomes_eq_pos_div_mu1": level1_is_pos_div_mu1,
        "site264_trace_matches_emulation": trace_matches_emulation,
        "site264_trace_function_of_pos_div_mu1": trace_function_of_quotient,
        "unknowns": nv, "equations": len(rows), "kernel_dim": nv - rank,
        "consistent": consistent, "recovered": recovered,
        "control_unknowns": nv0, "control_equations": len(crow),
        "control_kernel_dim": nv0 - rank0, "control_consistent": consistent0,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    t0 = time.time()
    result = {
        "candidate": "SDitH-4",
        "evidence": ("Emulated ideal branch trace in software (instrumented copy of the reference "
                     "div_rem_u32_ct_ref, temporary build only): the outcomes of the rsd.c:264 call "
                     "during honest key generation equal pos // mu_1 and, with y = Hx, determine the "
                     "secret witness; no timing or physical measurement, reference helper only "
                     "(the AVX2 table path is not affected)."),
    }
    gcc = shutil.which("gcc", path="/usr/bin") or shutil.which("gcc")
    if not gcc:
        result.update(status="SKIP", reason="gcc not found", checks={}, measurements={}, source_sha256={})
        emit(result, args.output)
        return 77

    ct_files = sorted(REF.glob("*/src/ct_utils.c"))
    rsd_files = sorted(REF.glob("*/src/rsd.c"))
    avx_files = sorted(OPT.glob("*/src/ct_utils_avx2.c"))
    opt_ct_files = sorted(OPT.glob("*/src/ct_utils.c"))
    source_sha = {
        "Reference_Implementation/*/src/ct_utils.c": PINNED_SHA256["Reference_Implementation/*/src/ct_utils.c"],
        "Reference_Implementation/*/src/rsd.c": PINNED_SHA256["Reference_Implementation/*/src/rsd.c"],
        "Optimized_Implementation/*/src/ct_utils_avx2.c":
            PINNED_SHA256["Optimized_Implementation/*/src/ct_utils_avx2.c"],
    }
    ct_lines = (REF / "cat1_fast/src/ct_utils.c").read_text().split("\n")
    rsd_lines = (REF / "cat1_fast/src/rsd.c").read_text().split("\n")
    avx_text = (OPT / "cat1_fast/src/ct_utils_avx2.c").read_text()
    avx_fn = avx_text[avx_text.index("EXPORT void div_rem_u32_ct_avx"):]
    avx_fn = avx_fn[:avx_fn.index("\n}\n")]
    checks = {
        "sources_pinned_all_sets": (
            len(ct_files) == len(rsd_files) == len(avx_files) == len(opt_ct_files) == 12
            and all(sha(p) == source_sha["Reference_Implementation/*/src/ct_utils.c"] for p in ct_files + opt_ct_files)
            and all(sha(p) == source_sha["Reference_Implementation/*/src/rsd.c"] for p in rsd_files)
            and all(sha(p) == source_sha["Optimized_Implementation/*/src/ct_utils_avx2.c"] for p in avx_files)),
        "source_ct_utils_33_branch": ct_lines[32].strip() == "if (rem >= public_d) {",
        "source_rsd_264_call_on_position": (
            rsd_lines[263].strip() == "vole_params->ct.div_rem_u32(&si, &sij, si, (uint32_t)arj);"
            and rsd_lines[259].strip() == "uint32_t si = solution[i];"
            and "rsd_encode_solution_ref" in "\n".join(rsd_lines[200:260])),
        "source_rsd_63_call_on_draw": (
            rsd_lines[62].strip() == "vole_params->ct.div_rem_u32(&quo, &solution[i], pos, npw);"),
        "control_avx2_helper_has_no_branch": ("if (" not in avx_fn and "sbbq" in avx_fn),
    }

    per_key = []
    with tempfile.TemporaryDirectory(prefix="sdith-se1-") as tmp:
        work = Path(tmp)
        for name in SETS:
            exe = build(gcc, work, name)
            for seed in SEEDS:
                out = work / f"{name}_{seed}"
                out.mkdir()
                r = run([str(exe), str(seed), str(out)], timeout=120)
                t = r.stdout.split()
                info = {"lambda": int(t[1]), "n": int(t[3]), "w": int(t[5]), "codim": int(t[7]),
                        "arities": [int(x) for x in t[11:11 + int(t[9])]]}
                res = analyse(name, seed, out, info)
                res.update({"lambda": info["lambda"], "n": info["n"], "w": info["w"],
                            "m": info["n"] // info["w"], "codim": info["codim"],
                            "arities": info["arities"]})
                per_key.append(res)

    keybool = ["hx_equals_y", "trace_layout_ok", "site63_outcomes_are_draw_quotient",
               "site63_remainder_is_position", "site264_level1_outcomes_eq_pos_div_mu1",
               "site264_trace_matches_emulation", "site264_trace_function_of_pos_div_mu1"]
    for k in keybool:
        checks[k + "_all_keys"] = all(r[k] for r in per_key)
    checks["witness_recovered_all_keys"] = all(r["recovered"] and r["kernel_dim"] == 0 for r in per_key)
    checks["control_without_trace_underdetermined"] = all(
        r["control_consistent"] and r["control_kernel_dim"] > 0 for r in per_key)
    checks["nine_keys_three_categories"] = len(per_key) == 9 and len({r["lambda"] for r in per_key}) == 3

    measurements = {
        "keys": len(per_key),
        "keys_recovered": sum(r["recovered"] for r in per_key),
        "unknowns_min": min(r["unknowns"] for r in per_key),
        "unknowns_max": max(r["unknowns"] for r in per_key),
        "equations_min": min(r["equations"] for r in per_key),
        "equations_max": max(r["equations"] for r in per_key),
        "kernel_dim_max": max(r["kernel_dim"] for r in per_key),
        "control_kernel_dim_min": min(r["control_kernel_dim"] for r in per_key),
        "control_kernel_dim_max": max(r["control_kernel_dim"] for r in per_key),
        "per_key": [{k: r[k] for k in ("set", "seed", "lambda", "n", "w", "m", "codim", "arities",
                                         "unknowns", "equations", "kernel_dim", "recovered",
                                         "control_unknowns", "control_equations", "control_kernel_dim")}
                    for r in per_key],
        "runtime_seconds": round(time.time() - t0, 1),
    }
    result.update(checks=checks, measurements=measurements, source_sha256=source_sha,
                  status="PASS" if all(checks.values()) else "FAIL")
    emit(result, args.output)
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
