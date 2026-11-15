#!/usr/bin/env python3
"""Reproduce SNOVA-8: the optimized signer's pivot search under clang.

The submitted Optimized_Implementation calls its Gaussian elimination
"constant time".  Its helper ``ct_is_not_zero`` is ``return val != 0;`` and the
pivot mask is built from it.  This script

1. checks the cited source lines against pinned SHA-256 values,
2. compiles the unmodified optimized sources (no submitted build file is run)
   with a host clang and with gcc, linked against a small embedded driver whose
   ``randombytes`` can mark its output undefined for Valgrind Memcheck,
3. signs with the secret key marked undefined and counts Memcheck's
   "Conditional jump or move depends on uninitialised value(s)" reports that
   are located at the pivot-mask loop.  Three taint modes are used: the whole
   secret key plus the signing RNG output, the 32-byte private seed alone, and
   nothing at all (harness control).

Stated result (review of 2026-11-15, clang 21 on x86-64): the clang builds
branch on the pivot, at -O3 once per row pair (i, j), j > i, of each signing
attempt (at -Os inside the row loop, so more often); the gcc builds of the
same source do not.  The gcc builds are the control.

Limits: this is taint tracking, not a timing measurement, and no key material
is recovered.  The submitted Makefile sets ``CC = gcc``.  Only the portable
(no ``-m`` flags) builds of SNOVA_I_K and SNOVA_V_S are run; the additional
AVX2/GFNI tree is not built here.  Other clang versions and other
architectures may compile the same line differently.

One measurement goes beyond the review and is recorded without a check: a
temporary copy of the signer in which the commented-out bitwise body of
``ct_is_not_zero`` replaces ``return val != 0;`` is built with clang -O3 and
run in the same way (``variant_bitwise_helper_not_in_review``).

Exit status: 0 PASS, 1 FAIL, 77 SKIP (no host-targeting clang-21..clang-15, no
working Valgrind, or no ``valgrind/memcheck.h``).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SCHEME = HERE.name
OPT_REL = "submission/SNOVA_Round3/Optimized_Implementation"
OPT = HERE / OPT_REL

CANDIDATE = "SNOVA-8"
EVIDENCE = (
    "Valgrind Memcheck taint tracking shows that clang builds (-O3 and -Os) of the "
    "submitted optimized SNOVA signer execute a conditional branch on the secret "
    "Gaussian-elimination pivot, at the mask the source builds from ct_is_not_zero "
    "('return val != 0;'), while gcc builds of the same source do not; this shows a "
    "compiler-dependent branch only, with no timing measurement and no key recovery, "
    "and the submitted Makefile uses gcc."
)

SIGNATURES = 4
JOBS = 3

# Files shared by every optimized parameter-set directory (18 identical copies).
COMMON_SHA256 = {
    "symmetric.c": "4d970520416d66d291a1a4b12bc324ce6a1ba5b6810c9c149d4167b390fdf7cc",
    "sign.c": "d79c85f77529a7f9d3787ca909c931c76dce14ba09ccf75e6b956bb1b7be8ddc",
    "aes.c": "42936e69655a11a6443c65e21e05f32a4861c9aa7c949a4674995f12c651ae64",
    "snova.h": "91b38ca19ec6f12d34fb87a1cf5e299c5e71d4c96199f47e8714472579e5274c",
    "symmetric.h": "9740e4b354175edd9e7686b898c571c12e75c873ab99095bcc64eeaaa5ee7982",
    "api.h": "7bb2d75096a2d489eb32acf75777c7cf83b4881dbe8b84bf8b0fa7646e7ec192",
    "rng.h": "9e35940ba45d85f419780a79de232d3f98ea15695c701053c884eda3ea537a40",
    "keccak_opt64.h": "f1c22592d19e7353abd9df8a8b5471162a76f7a446b85a904b787fba6e47b5e8",
}

# The two signer sources.  Line numbers are 1-based and are those cited by the
# review; they are meaningful because the file hashes are pinned.
SIGNERS = {
    "snova_opt_16.c": {
        "sha256": "be17fd5e17ac4adc4892a9c844d0f16ca6932bc693db00d8d4084f5fbae9ed4e",
        "copies": 12,
        "makefile_sha256": "d856f8f3dbd491ce5dd843552e8fdb58ca0bb0cf1a19e46c5b7caef7a7de35fd",
        "helper_lines": (69, 72),
        "pivot_lines": (1184, 1189),
        "retry_lines": (1218, 1250),
    },
    "snova_opt_16_5.c": {
        "sha256": "9937528115964adc20dc06927f7a2b2ea6bf2ff5200cd48353b8aa6d86d55be0",
        "copies": 6,
        "makefile_sha256": "f11d1d1b028d45dee42129ee8080ff3c6f9972a2ecb4d6bc4e01ff96e8e45fb8",
        "helper_lines": (69, 72),
        "pivot_lines": (1206, 1211),
        "retry_lines": (1240, 1272),
    },
}

HELPER_TEXT = (
    "static inline uint32_t ct_is_not_zero(uint8_t val) {\n"
    "\t// return (val | (val >> 1) | (val >> 2) | (val >> 3)) & 1;\n"
    "\treturn val != 0;\n"
    "}\n"
)
PIVOT_TEXT = (
    "\t\t\tfor (int j = i + 1; j < SNOVA_o * SNOVA_lr; ++j) {\n"
    "\t\t\t\tint16_t mask = ct_is_not_zero(gauss16[i][i]) - 1;\n"
    "\t\t\t\tfor (int k = 0; k < SNOVA_olr; ++k) {\n"
    "\t\t\t\t\tgauss16[i][k] ^= mask & gauss16[j][k];\n"
    "\t\t\t\t}\n"
    "\t\t\t}\n"
)
RETRY_TEXT = ("\t\tif (!flag_redo) {\n", "\t} while (flag_redo);\n")
# Extra measurement only: enable the commented-out helper body in a temporary
# copy (the line count is unchanged, so the cited line numbers still apply).
VARIANT_OLD = (
    "\t// return (val | (val >> 1) | (val >> 2) | (val >> 3)) & 1;\n"
    "\treturn val != 0;\n"
)
VARIANT_NEW = (
    "\treturn (val | (val >> 1) | (val >> 2) | (val >> 3)) & 1;\n"
    "\t// return val != 0;\n"
)
VARIANT_KEY = "SNOVA_I_K/clang-O3/sign/bitwise-helper-variant"
# sign.c line 57 tests the return value of the signer, i.e. the same retry
# outcome (the signer fails only after 254 rejected attempts).
SIGN_C_RESULT_LINE = (57, "\tif (!res) {\n")

# Parameter sets that are compiled and run.
RUN_SETS = {
    "SNOVA_I_K": {
        "signer": "snova_opt_16.c",
        "params_sha256": "38c5eea6fadab312fede97ca97a0b27f30ab68c8b36523b0e9184c99c00925b8",
    },
    "SNOVA_V_S": {
        "signer": "snova_opt_16_5.c",
        "params_sha256": "1d465c9f979b12cc4dc62b8446f8ed38d64aa1410b31230d5386314ec7e62e30",
    },
}

# (set, compiler, optimisation flag, taint modes, source variant)
#   mode "sign":  whole secret-key blob and all signing RNG output undefined.
#   mode "ssec":  only the 32-byte private seed sk[16..48] undefined.
#   mode "clean": nothing marked undefined (harness control).
TAINT = {
    "sign": "whole secret key and signing RNG output",
    "ssec": "32-byte private seed only",
    "clean": "nothing (harness control)",
}
PLAN = [
    ("SNOVA_I_K", "clang", "-O3", ("sign", "ssec", "clean"), None),
    ("SNOVA_I_K", "clang", "-Os", ("sign",), None),
    ("SNOVA_I_K", "gcc", "-O3", ("sign", "ssec"), None),
    ("SNOVA_I_K", "gcc", "-Os", ("sign",), None),
    ("SNOVA_V_S", "clang", "-O3", ("sign",), None),
    ("SNOVA_V_S", "gcc", "-O3", ("sign",), None),
    ("SNOVA_I_K", "clang", "-O3", ("sign",), "bitwise-helper-variant"),
]

DRIVER_C = r"""
/* Memcheck taint driver for the SNOVA optimized signer (reproducer harness,
 * not submission code).  randombytes() is a deterministic xorshift stream;
 * when taint_rng is set its output is marked undefined. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <valgrind/memcheck.h>
#include "api.h"

static int taint_rng = 0;
static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint8_t rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint8_t)(rng_state >> 24);
}

int randombytes(unsigned char *x, unsigned long long xlen) {
    for (unsigned long long i = 0; i < xlen; i++) x[i] = rng_next();
    if (taint_rng) VALGRIND_MAKE_MEM_UNDEFINED(x, xlen);
    return 0;
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "sign";
    int iters = argc > 2 ? atoi(argv[2]) : 1;
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    unsigned char m[33] = "ctgrind message for taint testing";
    unsigned long long mlen = sizeof m, smlen = 0, m2len = 0;
    unsigned char *sm = malloc(sizeof m + CRYPTO_BYTES);
    unsigned char *m2 = malloc(sizeof m + CRYPTO_BYTES);
    if (!sm || !m2) return 5;
    if (strcmp(mode, "sign") && strcmp(mode, "ssec") && strcmp(mode, "clean")) return 6;
    for (int it = 0; it < iters; it++) {
        rng_state = 0x1234567ULL + 7919ULL * (uint64_t)it;
        if (crypto_sign_keypair(pk, sk)) { puts("keypair failed"); return 2; }
        if (!strcmp(mode, "sign")) {
            VALGRIND_MAKE_MEM_UNDEFINED(sk, sizeof sk);
            taint_rng = 1;
        } else if (!strcmp(mode, "ssec")) {
            VALGRIND_MAKE_MEM_UNDEFINED(sk + SEED_LENGTH_PUBLIC, SEED_LENGTH_PRIVATE);
        }
        int rc = crypto_sign(sm, &smlen, m, mlen, sk);
        taint_rng = 0;
        /* The return code, length and signature are released values. */
        VALGRIND_MAKE_MEM_DEFINED(&rc, sizeof rc);
        VALGRIND_MAKE_MEM_DEFINED(&smlen, sizeof smlen);
        VALGRIND_MAKE_MEM_DEFINED(sm, sizeof m + CRYPTO_BYTES);
        VALGRIND_MAKE_MEM_DEFINED(sk, sizeof sk);
        if (rc) { printf("sign rc=%d\n", rc); return 3; }
        if (crypto_sign_open(m2, &m2len, sm, smlen, pk) || m2len != mlen || memcmp(m, m2, mlen)) {
            puts("verify failed");
            return 4;
        }
    }
    free(sm);
    free(m2);
    printf("ok mode=%s signatures=%d valgrind=%d\n", mode, iters, (int)RUNNING_ON_VALGRIND);
    return 0;
}
"""

PROBE_C = r"""
#include <stdio.h>
#include <valgrind/memcheck.h>
int main(void) { printf("probe %d\n", (int)RUNNING_ON_VALGRIND); return 0; }
"""


class Skip(Exception):
    pass


def sha256(path: Path) -> str:
    if not path.is_file():
        return "missing"
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(cmd, cwd: Path, timeout: int) -> subprocess.CompletedProcess:
    return subprocess.run(
        [str(c) for c in cmd], cwd=cwd, capture_output=True, text=True,
        timeout=timeout, env=dict(os.environ, LC_ALL="C"),
    )


def first_line(cmd, cwd: Path) -> str:
    done = run(cmd, cwd, 60)
    text = (done.stdout or done.stderr).strip().splitlines()
    return text[0].strip() if text else ""


def lines(path: Path, first: int, last: int) -> str:
    return "".join(path.read_text(encoding="utf-8").splitlines(keepends=True)[first - 1:last])


def read_params(path: Path) -> dict:
    found = dict(re.findall(r"#define\s+SNOVA_(\w+)\s+(\w+)", path.read_text(encoding="utf-8")))
    return {k: int(found[k]) for k in ("v", "o", "q", "l", "r", "m1")}


# --------------------------------------------------------------------------
# Source checks
# --------------------------------------------------------------------------

def source_checks() -> tuple[dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    problems: list = []

    def pin(path: Path, expected: str) -> bool:
        rel = f"{SCHEME}/{path.relative_to(HERE).as_posix()}"
        actual = sha256(path)
        pinned[rel] = actual
        if actual != expected:
            problems.append(f"sha256 mismatch {rel}: {actual}")
            return False
        return True

    ok = True
    set_dirs = sorted(p for p in OPT.iterdir() if p.is_dir()) if OPT.is_dir() else []
    per_signer = {name: [] for name in SIGNERS}
    # Every optimized set uses one of the two pinned signer sources.
    all_sets = len(set_dirs) == 18
    for directory in set_dirs:
        present = [name for name in SIGNERS if (directory / name).is_file()]
        if len(present) != 1:
            problems.append(f"{directory.name}: expected exactly one signer source, found {present}")
            all_sets = False
            continue
        per_signer[present[0]].append(directory)
    for name, info in SIGNERS.items():
        all_sets &= len(per_signer[name]) == info["copies"]
        for directory in per_signer[name]:
            if sha256(directory / name) != info["sha256"]:
                problems.append(f"sha256 mismatch {directory.name}/{name}")
                all_sets = False
            if sha256(directory / "Makefile") != info["makefile_sha256"]:
                problems.append(f"sha256 mismatch {directory.name}/Makefile")
                all_sets = False
    checks["source_all_18_optimized_sets_use_the_two_pinned_signer_sources"] = all_sets

    helper_ok = pivot_ok = retry_ok = makefile_ok = True
    for set_name, entry in RUN_SETS.items():
        directory = OPT / set_name
        info = SIGNERS[entry["signer"]]
        ok &= pin(directory / entry["signer"], info["sha256"])
        ok &= pin(directory / "snova_params.h", entry["params_sha256"])
        ok &= pin(directory / "Makefile", info["makefile_sha256"])
        for name, digest in COMMON_SHA256.items():
            ok &= pin(directory / name, digest)
        if not ok:
            continue
        signer = directory / entry["signer"]
        helper_ok &= lines(signer, *info["helper_lines"]) == HELPER_TEXT
        pivot_ok &= lines(signer, *info["pivot_lines"]) == PIVOT_TEXT
        for number, text in zip(info["retry_lines"], RETRY_TEXT):
            retry_ok &= lines(signer, number, number) == text
        retry_ok &= lines(directory / "sign.c", SIGN_C_RESULT_LINE[0], SIGN_C_RESULT_LINE[0]) == SIGN_C_RESULT_LINE[1]
        makefile = (directory / "Makefile").read_text(encoding="utf-8")
        makefile_ok &= len(re.findall(r"^CC = gcc$", makefile, re.M)) == 1 and "clang" not in makefile
    checks["source_sha256_pinned_files_match"] = ok
    checks["source_ct_is_not_zero_is_a_plain_comparison"] = ok and helper_ok
    checks["source_pivot_mask_is_built_from_ct_is_not_zero"] = ok and pivot_ok
    checks["source_retry_flag_lines_as_cited"] = ok and retry_ok
    checks["source_submitted_makefile_sets_cc_gcc"] = ok and makefile_ok
    return checks, pinned, problems


# --------------------------------------------------------------------------
# Tools
# --------------------------------------------------------------------------

def compiler_works(compiler: str, work: Path, tag: str) -> tuple[bool, str]:
    """Compile and run a tiny program: is this a host compiler with the Valgrind header?"""
    source = work / f"probe_{tag}.c"
    source.write_text(PROBE_C, encoding="utf-8")
    binary = work / f"probe_{tag}"
    built = run([compiler, "-std=c11", "-O1", source, "-o", binary], work, 120)
    if built.returncode:
        return False, built.stderr.strip()[-400:]
    try:
        ran = run([binary], work, 30)
    except OSError as error:  # e.g. a cross compiler's output
        return False, str(error)
    return (ran.returncode == 0 and ran.stdout.startswith("probe ")), ran.stderr.strip()[-400:]


def find_tools(work: Path) -> dict:
    valgrind = shutil.which("valgrind")
    if not valgrind:
        raise Skip("valgrind not found")
    gcc = shutil.which("gcc")
    if not gcc:
        raise Skip("gcc not found")
    usable, detail = compiler_works(gcc, work, "gcc")
    if not usable:
        raise Skip(f"gcc cannot build a program that includes valgrind/memcheck.h: {detail}")
    # Valgrind itself must run: the probe prints 1 for RUNNING_ON_VALGRIND.
    try:
        probe = run([valgrind, "--tool=memcheck", "-q", work / "probe_gcc"], work, 120)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"valgrind cannot be run: {error}") from error
    if probe.returncode or probe.stdout.strip() != "probe 1":
        raise Skip(f"valgrind does not run a test program: {probe.stderr.strip()[-300:]}")
    clang = None
    tried = []
    # Plain "clang" may be a cross compiler; search versioned host compilers.
    for version in range(21, 14, -1):
        candidate = shutil.which(f"clang-{version}")
        if not candidate:
            continue
        usable, detail = compiler_works(candidate, work, f"clang{version}")
        tried.append(f"clang-{version}: {'ok' if usable else detail}")
        if usable:
            clang = candidate
            break
    if not clang:
        raise Skip("no host-targeting clang-21..clang-15 found" + (f" ({'; '.join(tried)})" if tried else ""))
    return {"valgrind": valgrind, "gcc": gcc, "clang": clang, "objdump": shutil.which("objdump")}


# --------------------------------------------------------------------------
# Build, run, parse
# --------------------------------------------------------------------------

def build(tools: dict, work: Path, job: tuple) -> Path:
    set_name, compiler, opt, _, variant = job
    directory = OPT / set_name
    signer = directory / RUN_SETS[set_name]["signer"]
    name = f"{set_name}_{compiler}_{opt.lstrip('-')}"
    if variant:
        # Patched copy in the temporary directory; the submission is not touched.
        text = signer.read_text(encoding="utf-8")
        if text.count(VARIANT_OLD) != 1:
            raise RuntimeError("variant anchor does not occur exactly once")
        name += "_variant"
        signer = work / f"{name}_{signer.name}"
        signer.write_text(text.replace(VARIANT_OLD, VARIANT_NEW), encoding="utf-8")
    binary = work / name
    # rng.c is replaced by the driver's randombytes(); nothing else is changed.
    cmd = [tools[compiler], "-std=c11", "-g", opt, "-no-pie", f"-I{directory}",
           signer, directory / "symmetric.c", directory / "sign.c", directory / "aes.c",
           work / "taint_driver.c", "-o", binary]
    done = run(cmd, work, 300)
    if done.returncode:
        raise RuntimeError(f"build failed ({name}): {done.stderr[-800:]}")
    return binary


def memcheck(tools: dict, work: Path, binary: Path, mode: str) -> dict:
    report = work / f"{binary.name}.{mode}.xml"
    cmd = [tools["valgrind"], "--tool=memcheck", "--error-limit=no", "--leak-check=no",
           "--num-callers=12", "--xml=yes", f"--xml-file={report}",
           binary, mode, str(SIGNATURES)]
    done = run(cmd, work, 480)
    expected = f"ok mode={mode} signatures={SIGNATURES} valgrind=1"
    if done.returncode or done.stdout.strip() != expected:
        raise RuntimeError(f"taint run failed ({binary.name} {mode}): rc={done.returncode} "
                           f"stdout={done.stdout[-200:]!r} stderr={done.stderr[-400:]!r}")
    text = report.read_text(encoding="utf-8", errors="replace")
    counts_block = re.search(r"<errorcounts>(.*?)</errorcounts>", text, re.S)
    if not counts_block:
        raise RuntimeError(f"no <errorcounts> in Memcheck report for {binary.name} {mode}")
    counts = {pair.findtext("unique"): int(pair.findtext("count"))
              for pair in ET.fromstring("<x>" + counts_block.group(1) + "</x>").iter("pair")}
    errors = []
    # The XML is read error by error: Valgrind 3.26 emits a malformed
    # <leak_summary>, so the document as a whole is not always well formed.
    for block in re.findall(r"<error>.*?</error>", text, re.S):
        element = ET.fromstring(block)
        frames = [{"ip": f.findtext("ip"), "fn": f.findtext("fn") or "",
                   "file": (f.findtext("file") or "").rsplit("/", 1)[-1],
                   "line": int(f.findtext("line") or 0)}
                  for f in element.find("stack").iter("frame")]
        errors.append({"kind": element.findtext("kind"), "frames": frames,
                       "count": counts.get(element.findtext("unique"), 0)})
    return {"errors": errors, "total": sum(counts.values())}


def classify(report: dict, set_name: str) -> dict:
    """Locate every report by its frame in the signer (or in crypto_sign)."""
    signer = RUN_SETS[set_name]["signer"]
    info = SIGNERS[signer]
    sign_fn = f"_{set_name}_SNOVA_OPT_sign"
    low, high = info["pivot_lines"]
    result = {"total_reports": report["total"], "pivot_hits": 0, "retry_flag_hits": 0,
              "other_reports": 0, "attempts": 0, "sites": {}, "pivot_ip": None}
    for error in report["errors"]:
        # Skip inlined helpers (ct_is_not_zero): the site is the first frame
        # that belongs to the signer or to its caller.
        site = next((f for f in error["frames"] if f["fn"] in (sign_fn, "crypto_sign", "main")), None)
        if site is None:
            site = error["frames"][0] if error["frames"] else {"fn": "?", "line": 0, "ip": None}
        where = {sign_fn: signer, "crypto_sign": "sign.c"}.get(site["fn"], site["fn"])
        key = f"{where}:{site['line']}"
        if error["kind"] != "UninitCondition":
            key += f" [{error['kind']}]"
        result["sites"][key] = result["sites"].get(key, 0) + error["count"]
        conditional = error["kind"] == "UninitCondition"
        if conditional and site["fn"] == sign_fn and low <= site["line"] <= high:
            result["pivot_hits"] += error["count"]
            result["pivot_ip"] = result["pivot_ip"] or site["ip"]
        elif conditional and ((site["fn"] == sign_fn and site["line"] in info["retry_lines"])
                              or (site["fn"] == "crypto_sign" and site["line"] == SIGN_C_RESULT_LINE[0])):
            result["retry_flag_hits"] += error["count"]
            # "if (!flag_redo)" runs once per signing attempt.
            if site["fn"] == sign_fn and site["line"] == info["retry_lines"][0]:
                result["attempts"] += error["count"]
        else:
            result["other_reports"] += error["count"]
    return result


def instruction_at(tools: dict, work: Path, binary: Path, set_name: str, ip: str | None):
    """Disassemble the signer and return the instruction at ip with two predecessors."""
    if not tools["objdump"] or not ip:
        return None
    done = run([tools["objdump"], "-d", "--no-show-raw-insn",
                f"--disassemble=_{set_name}_SNOVA_OPT_sign", binary], work, 120)
    if done.returncode:
        return None
    listing = []
    for line in done.stdout.splitlines():
        match = re.match(r"^\s*([0-9a-f]+):\s+(.*\S)\s*$", line)
        if match:
            listing.append((int(match.group(1), 16), re.sub(r"\s+", " ", re.sub(r"\s*<.*?>", "", match.group(2)))))
    target = int(ip, 16)
    for index, (address, _) in enumerate(listing):
        if address == target:
            return "; ".join(text for _, text in listing[max(0, index - 2):index + 1])
    return None


def is_conditional_jump(instructions: str | None) -> bool:
    if not instructions:
        return False
    mnemonic = instructions.split("; ")[-1].split()[0]
    return mnemonic.startswith("j") and not mnemonic.startswith("jmp")


def measure(tools: dict, work: Path) -> dict:
    (work / "taint_driver.c").write_text(DRIVER_C, encoding="utf-8")
    with ThreadPoolExecutor(max_workers=JOBS) as pool:
        binaries = list(pool.map(lambda job: build(tools, work, job), PLAN))
        jobs = [(job, binary, mode) for job, binary in zip(PLAN, binaries) for mode in job[3]]
        reports = list(pool.map(lambda item: memcheck(tools, work, item[1], item[2]), jobs))
    runs = {}
    for (job, binary, mode), report in zip(jobs, reports):
        set_name, compiler, opt, _, variant = job
        entry = classify(report, set_name)
        ip = entry.pop("pivot_ip")
        if compiler == "clang" and ip:
            # The instruction at the reported address, after its two predecessors.
            entry["pivot_instructions"] = instruction_at(tools, work, binary, set_name, ip)
        runs[f"{set_name}/{compiler}{opt}/{mode}" + (f"/{variant}" if variant else "")] = entry
    return runs


def row_pairs(set_name: str) -> int:
    """Number of row pairs (i, j), j > i, of the o*l*r by o*l*r signing system."""
    params = read_params(OPT / set_name / "snova_params.h")
    size = params["o"] * params["l"] * params["r"]
    return size * (size - 1) // 2


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def render(value, depth: int = 0) -> str:
    """JSON text: the top three levels one entry per line, deeper values inline."""
    if isinstance(value, dict) and value and depth < 3:
        pad = "  " * (depth + 1)
        rows = [f"{pad}{json.dumps(key)}: {render(value[key], depth + 1)}" for key in sorted(value)]
        return "{\n" + ",\n".join(rows) + "\n" + "  " * depth + "}"
    return json.dumps(value, sort_keys=True)


def emit(result: dict, output: Path | None) -> int:
    rendered = render(result) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(rendered, encoding="utf-8")
    sys.stdout.write(rendered)
    sys.stdout.flush()
    print(f"{result['status']} {CANDIDATE}", file=sys.stderr)
    return {"PASS": 0, "SKIP": 77}.get(result["status"], 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path, help="also write the JSON result to this file")
    args = parser.parse_args()
    started = time.monotonic()

    result = {"candidate": CANDIDATE, "evidence": EVIDENCE, "checks": {}, "measurements": {},
              "source_sha256": {}, "status": "FAIL"}
    checks, pinned, problems = source_checks()
    result["checks"].update(checks)
    result["source_sha256"] = pinned
    if problems:
        result["problems"] = problems
    if not all(checks.values()):
        return emit(result, args.output)

    with tempfile.TemporaryDirectory(prefix="snova-se1-") as temporary:
        work = Path(temporary)
        try:
            tools = find_tools(work)
        except Skip as reason:
            result["status"] = "SKIP"
            result["skip_reason"] = str(reason)
            return emit(result, args.output)
        result["measurements"]["tools"] = {
            "clang": first_line([tools["clang"], "--version"], work),
            "clang_target": first_line([tools["clang"], "-dumpmachine"], work),
            "gcc": first_line([tools["gcc"], "--version"], work),
            "valgrind": first_line([tools["valgrind"], "--version"], work),
            "objdump": first_line([tools["objdump"], "--version"], work) if tools["objdump"] else None,
            "machine": platform.machine(),
        }
        try:
            runs = measure(tools, work)
        except (RuntimeError, subprocess.TimeoutExpired, OSError, ET.ParseError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    variant_run = runs.pop(VARIANT_KEY)
    summary = result["measurements"]
    summary["build"] = ("<compiler> -std=c11 -g <-O3|-Os> -no-pie -I<set> <signer>.c symmetric.c sign.c aes.c "
                        "taint_driver.c (embedded; replaces rng.c)")
    summary["signatures_per_run"] = SIGNATURES
    summary["taint_modes"] = TAINT
    pairs = {name: row_pairs(name) for name in RUN_SETS}
    summary["row_pairs_per_attempt"] = pairs
    summary["runs"] = runs
    c = result["checks"]

    def hits(key: str) -> int:
        return runs[key]["pivot_hits"]

    # Stated behaviour: clang builds branch on tainted data at the pivot mask.
    c["clang_O3_branches_on_tainted_pivot_I_K"] = hits("SNOVA_I_K/clang-O3/sign") > 0
    c["clang_Os_branches_on_tainted_pivot_I_K"] = hits("SNOVA_I_K/clang-Os/sign") > 0
    c["clang_O3_branch_remains_with_only_private_seed_tainted_I_K"] = hits("SNOVA_I_K/clang-O3/ssec") > 0
    c["clang_O3_branches_on_tainted_pivot_V_S"] = hits("SNOVA_V_S/clang-O3/sign") > 0
    # One report per row pair (i, j), j > i, per signing attempt (4,560 for I_K).
    c["clang_O3_hits_are_one_per_row_pair_per_attempt"] = all(
        runs[key]["attempts"] >= SIGNATURES
        and runs[key]["pivot_hits"] == pairs[key.split("/")[0]] * runs[key]["attempts"]
        for key in ("SNOVA_I_K/clang-O3/sign", "SNOVA_I_K/clang-O3/ssec", "SNOVA_V_S/clang-O3/sign"))
    # Optional (needs GNU objdump): the reported address holds a conditional jump.
    disassembled = [runs[key].get("pivot_instructions")
                    for key in ("SNOVA_I_K/clang-O3/sign", "SNOVA_V_S/clang-O3/sign")]
    if all(disassembled):
        c["clang_O3_pivot_report_is_a_conditional_jump_instruction"] = all(
            is_conditional_jump(item) for item in disassembled)
    # Controls: the gcc -O3 builds of the same sources have no report there,
    # and report nothing but the (intended, released) retry outcome.
    gcc_controls = ("SNOVA_I_K/gcc-O3/sign", "SNOVA_I_K/gcc-O3/ssec", "SNOVA_V_S/gcc-O3/sign")
    c["control_gcc_O3_no_tainted_branch_at_pivot"] = all(hits(key) == 0 for key in gcc_controls)
    c["control_gcc_O3_reports_only_the_retry_flag"] = all(
        runs[key]["other_reports"] == 0 and runs[key]["attempts"] >= SIGNATURES
        and runs[key]["total_reports"] == runs[key]["retry_flag_hits"] for key in gcc_controls)
    # Harness control: with nothing marked undefined the clang build is silent,
    # so the reports above come from the taint and not from the program itself.
    c["control_clang_O3_untainted_run_has_no_reports"] = (
        runs["SNOVA_I_K/clang-O3/clean"]["total_reports"] == 0)
    c["control_gcc_and_clang_made_the_same_number_of_attempts"] = all(
        runs[f"{name}/gcc-O3/{mode}"]["attempts"] == runs[f"{name}/clang-O3/{mode}"]["attempts"]
        for name, mode in (("SNOVA_I_K", "sign"), ("SNOVA_I_K", "ssec"), ("SNOVA_V_S", "sign")))

    key = "SNOVA_I_K/clang-O3/sign"
    summary["I_K_clang_O3_pivot_hits_per_attempt"] = (
        runs[key]["pivot_hits"] / runs[key]["attempts"] if runs[key]["attempts"] else None)
    summary["I_K_clang_O3_total_reports_seed_only"] = runs["SNOVA_I_K/clang-O3/ssec"]["total_reports"]
    summary["I_K_gcc_Os_pivot_hits_not_in_review"] = hits("SNOVA_I_K/gcc-Os/sign")
    summary["variant_bitwise_helper_not_in_review"] = {
        "description": ("temporary copy of snova_opt_16.c with 'return (val | (val >> 1) | (val >> 2) | "
                        "(val >> 3)) & 1;' in place of 'return val != 0;', clang -O3, SNOVA_I_K, mode sign; "
                        "recorded only, no check depends on it"),
        "run": variant_run,
    }
    summary["review_values"] = {
        "I_K_pivot_hits_per_attempt": 4560,
        "I_K_seed_only_total_reports_4_signatures_5_attempts": 22810,
    }
    result["status"] = "PASS" if all(c.values()) else "FAIL"
    # The run time goes to stderr so that the JSON is identical on every run.
    print(f"elapsed {time.monotonic() - started:.1f} s", file=sys.stderr)
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
