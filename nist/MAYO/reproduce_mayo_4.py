#!/usr/bin/env python3
"""Reproduce MAYO-4: a zero-vinegar signing attempt yields oil-space signatures.

With the On-Ramp Round 3 linear term (Algorithm 7, lines 30-39), an attempt
whose vinegar vectors are all zero still produces a solvable system: every M_i
is zero except M_0, which keeps Lambda_v*O + Lambda_o, and y = t.  The signer
therefore returns s_i = (O x_i, x_i), a valid signature whose k blocks are oil
vectors.  This script

1. checks the cited source against pinned SHA-256 values,
2. compiles the unmodified reference implementation of MAYO1, 2, 3 and 5 with
   explicit gcc command lines (no submitted build file is run), and a second
   copy whose ``src/mayo.c`` is patched in the temporary directory with two
   switchable lines: one zeroes the vinegar bytes right after the SHAKE call
   that derives them (the reviewer's fault model), the other zeroes Lambda on
   the signer side (Round 2 behaviour, the control),
3. signs with the faulted signer, verifies every output with
   ``crypto_sign_open`` of the unmodified build, checks every signature block
   against the oil space of the key (O is recomputed here from the known secret
   seed with SHAKE256), and recovers O by row reduction of the signature blocks
   alone.

Controls: with Lambda also zeroed the same fault makes every signing call
fail (100 calls for MAYO1/2, 50 for MAYO3, 25 for MAYO5; the review used 100);
the unfaulted signer produces signatures whose blocks are not oil vectors; and
the patched build with both switches off is byte-identical to the unmodified
build.

Limits: the fault is simulated in software by a memset; no physical fault was
performed and partial faults (some v_i zero) are not tested.  Only the
portable reference implementation is built; the other trees share the same
``src/`` by the review's comparison, which is not repeated here.  One key per
parameter set is used.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (no gcc).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SCHEME = HERE.name
REF_REL = "submission/MAYO/Reference_Implementation"
REF = HERE / REF_REL

CANDIDATE = "MAYO-4"
EVIDENCE = (
    "With the Round 3 linear term, a reference MAYO signer whose vinegar bytes are "
    "forced to zero (fault simulated in software by one added memset; no physical "
    "fault was performed) still returns success for most calls, every such signature "
    "passes the unmodified crypto_sign_open, every signature block lies in the secret "
    "oil space, and row reduction of the blocks of the first one (MAYO1/3/5) or three "
    "(MAYO2) signatures reproduces O exactly, while with Lambda also zeroed on the signer side "
    "(Round 2 behaviour) the same fault makes every tested signing call fail; this does "
    "not show that such a fault is achievable on hardware or that partial faults suffice."
)

FAULT_CALLS = 300
# A failing call runs all 256 attempts; the review used 100 per set.  MAYO3 and
# MAYO5 use fewer calls here to keep the run near two minutes.
CONTROL_CALLS = {"1": 100, "2": 100, "3": 50, "5": 25}
CLEAN_CALLS = 20
IDENTITY_CALLS = 5
JOBS = 3
KEY_SEED = {"1": 0x5E1001, "2": 0x5E1002, "3": 0x5E1003, "5": 0x5E1005}

# Parameters (include/mayo_constants.h, pinned below).
PARAMS = {
    "1": {"n": 88, "m": 80, "o": 8, "k": 10, "O_bytes": 320, "pk_seed_bytes": 16, "sk_seed_bytes": 24},
    "2": {"n": 86, "m": 64, "o": 13, "k": 5, "O_bytes": 475, "pk_seed_bytes": 16, "sk_seed_bytes": 24},
    "3": {"n": 118, "m": 108, "o": 10, "k": 11, "O_bytes": 540, "pk_seed_bytes": 16, "sk_seed_bytes": 32},
    "5": {"n": 154, "m": 142, "o": 12, "k": 12, "O_bytes": 852, "pk_seed_bytes": 16, "sk_seed_bytes": 40},
}
REVIEW = {
    "faulty_sign_success_of_300": {"1": 287, "2": 299, "3": 300, "5": 300},
    "signatures_to_recover_O": {"1": 1, "2": 3, "3": 1, "5": 1},
    "control_lambda_zero_failures_of_100": {"1": 100, "2": 100, "3": 100, "5": 100},
}

COMMON = [
    "src/mayo.c", "src/params.c", "src/arithmetic.c",
    "src/common/aes_c.c", "src/common/aes128ctr.c", "src/common/aes_neon.c",
    "src/common/fips202.c", "src/common/mem.c",
]
# Every submitted file that is compiled or included (paths relative to REF).
PINNED = {
    "src/mayo.c": "d57c77dff3bafad3e063e000544eedb4390197e56762cb8b10d5758cede54645",
    "src/params.c": "fea8a5a05f1b4b2fd9bea17f24e2bbf0b1218d9afaff8d90754927419daa8902",
    "src/arithmetic.c": "5823524f2d75d2f7dc29b403547748f2b27d03dc503a9a76d1d1acba716f07b7",
    "src/arithmetic.h": "a2e83b3ac3f271bd00766c43a2a9f4c723035cf19a77c348ffa511226b4ab135",
    "src/simple_arithmetic.h": "a798f2b7427dd00e4bdc5ec77cae6a8246fb543ca91820c96e9731da20cc475b",
    "src/generic/arithmetic_dynamic.h": "4f00e4ad9d31767e95ff9df401c4b2ef4c50f911a24fe43f90669e571d4fa2c8",
    "src/generic/arithmetic_fixed.h": "9d9d49d0965d80b54192183c5b974d8a454b6fac4c1054d19e5bed866c02ae9c",
    "src/generic/echelon_form.h": "98fab8fb6ac03ff1bb530f3d5fb7702912710577e5d7f10d7ebbe3a8d06382b8",
    "src/generic/ef_inner_loop.h": "d7c67cce1913af04ce2ac70ba452cb8c897ba6510ddc922cb29acd6074fbc5ec",
    "src/generic/generic_arithmetic.h": "2ffc349c2acd4d6e79b08c2479b01372a870cdd7d86c93124b4d55f281c88d86",
    "src/common/aes_c.c": "2603e648f531fe9540e5418dfdb3de97fe5d6f324de04da161adc7a220557f78",
    "src/common/aes128ctr.c": "64d8a8c90873a019c8bae682a387cb6624394704d03a9ec0f506a13c41f003a7",
    "src/common/aes_neon.c": "f9ef4fb7677b52a78c6aa5d310e3ac2cbb8972c8ea3264462f040b544150eee7",
    "src/common/fips202.c": "0aaf71e66f40b7220b2092ee8e918e3592614b008df8964e240136169cc9fd46",
    "src/common/mem.c": "8387376571d2d74d1f8d996e6253b30149836ef69515a3f709a01be7a0fcf3c8",
    "src/common/aes_ctr.h": "3c61526e24a00427fda4f6e6762042815e3f77adfd63dec4932c735b07a442da",
    "src/common/fips202.h": "bba48df66211fa43f4a40a4aa0aba0fc85eb63ac415a853e2ecd62c105f67fd2",
    "include/mayo.h": "bd98575ab86a90a874d4e794772b85e37f3ff7d50f6b528620136ff1b2d95902",
    "include/mayo_constants.h": "489f2d2f07287b27719332b311dea454e8d42b57dce5cd9be9222c889b7854fe",
    "include/mem.h": "6ab679c9c83113bbaf9df66e631589b554abb000a546b9f43f6692e5427fe058",
    "include/randombytes.h": "e3a160e3622eea56aff096a2f9322c8a992a67e2704c826d1d7c9d72d0c36b56",
    "src/mayo_1/api.c": "ae82f3fbf89789ad25e99917dcc30e42a59f1bd9d6fc1f85becd3bd723a18fa6",
    "src/mayo_1/api.h": "42cc19fab04171a58cba11bc00b65820fcf1c290064e167389af3a2e650f8212",
    "src/mayo_2/api.c": "47bc71115a7e42dcc815201265a21707d8b55d6f194ff68754f58252403ddbdc",
    "src/mayo_2/api.h": "3761aaeaa6fbc8c744e12a5528222d762ec810c2467632f1625ce9383c98fe81",
    "src/mayo_3/api.c": "a12a33085a128ae6e83c2beef721eb044d34bd432690f6b94878da5c38a8b983",
    "src/mayo_3/api.h": "50c0313891a723d28bb1857df97186e9907e882fc863230a104d8dfad7e4a20b",
    "src/mayo_5/api.c": "dcab121ef9d97074f2f70b2f0a559623482c10c070b5ed30dd05a32e15a62914",
    "src/mayo_5/api.h": "e225522450233e9b99f8549d401d7ccddf879fb4433677b17338c11b2c829c28",
}

# Patch anchors in src/mayo.c (each must occur exactly once).
ANCHOR_INCLUDE = "#include <string.h>\n"
ANCHOR_VINEGAR = (
    "        shake256(V, param_k * param_v_bytes + param_r_bytes, tmp,\n"
    "                 param_digest_bytes + param_salt_bytes + param_sk_seed_bytes + 1);\n"
)
ANCHOR_LAMBDA = "    derive_lambda(p, tenc + param_m_bytes, lambda);\n"
PATCH_INCLUDE = "extern int repro_fault_v0, repro_fault_l0; /* reproducer switches */\n"
PATCH_VINEGAR = (
    "        if (repro_fault_v0) memset(V, 0, param_k * param_v_bytes); "
    "/* simulated fault: zero vinegar */\n"
)
PATCH_LAMBDA = (
    "    if (repro_fault_l0) memset(lambda, 0, sizeof(lambda)); "
    "/* control: Round 2 signer, no linear term */\n"
)

DRIVER_C = r"""
/* Reproducer driver (not submission code).  randombytes() is a deterministic
 * xorshift stream so that keys and signatures are reproducible.
 *   sign <none|v0|v0l0> <seed> <calls>  prints pk, sk and one line per call
 *   open                                reads "pk <hex>" then "sm <hex>" lines */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <api.h>
#include <randombytes.h>

#ifdef REPRO_FAULT_BUILD
int repro_fault_v0 = 0, repro_fault_l0 = 0;
#endif

static uint64_t rng_state = 1;
static uint8_t rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint8_t)(rng_state >> 24);
}
int randombytes(unsigned char *x, size_t xlen) {
    for (size_t i = 0; i < xlen; i++) x[i] = rng_next();
    return 0;
}
void randombytes_init(unsigned char *e, unsigned char *p, int s) { (void)e; (void)p; (void)s; }

static void hex(const char *tag, const unsigned char *b, size_t n) {
    fputs(tag, stdout);
    for (size_t i = 0; i < n; i++) printf("%02x", b[i]);
    putchar('\n');
}
static int unhex(const char *s, unsigned char *out, size_t max, size_t *len) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) n--;
    if (n % 2 || n / 2 > max) return -1;
    for (size_t i = 0; i < n / 2; i++) {
        unsigned v;
        if (sscanf(s + 2 * i, "%2x", &v) != 1) return -1;
        out[i] = (unsigned char)v;
    }
    *len = n / 2;
    return 0;
}

int main(int argc, char **argv) {
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    static unsigned char sm[CRYPTO_BYTES + 64], m[64], m2[CRYPTO_BYTES + 64];
    size_t smlen, m2len;
    if (argc < 2) return 9;
    printf("sizes %d %d %d\n", CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES);
    if (!strcmp(argv[1], "sign") && argc == 5) {
        const char *mode = argv[2];
        uint64_t seed = strtoull(argv[3], 0, 0);
        int calls = atoi(argv[4]);
        rng_state = seed ? seed : 1;
        if (crypto_sign_keypair(pk, sk)) { puts("keypair failed"); return 2; }
        hex("pk ", pk, sizeof pk);
        hex("sk ", sk, sizeof sk);
#ifdef REPRO_FAULT_BUILD
        repro_fault_v0 = !strcmp(mode, "v0") || !strcmp(mode, "v0l0");
        repro_fault_l0 = !strcmp(mode, "v0l0");
#else
        if (strcmp(mode, "none")) { puts("fault mode needs the patched build"); return 3; }
#endif
        if (strcmp(mode, "none") && strcmp(mode, "v0") && strcmp(mode, "v0l0")) return 4;
        for (int i = 0; i < calls; i++) {
            size_t mlen = 1 + (size_t)(i % 48);
            for (size_t j = 0; j < mlen; j++) m[j] = (unsigned char)(i * 31 + j * 7 + seed);
            smlen = 0;
            int rc = crypto_sign(sm, &smlen, m, mlen, sk);
            printf("call %d rc %d ", i, rc);
            hex("sm ", sm, rc ? 0 : smlen);
        }
        return 0;
    }
    if (!strcmp(argv[1], "open") && argc == 2) {
        char *line = NULL;
        size_t cap = 0, len;
        if (getline(&line, &cap, stdin) < 0 || strncmp(line, "pk ", 3)
            || unhex(line + 3, pk, sizeof pk, &len) || len != sizeof pk) { puts("bad pk"); return 5; }
        int i = 0;
        while (getline(&line, &cap, stdin) >= 0) {
            if (strncmp(line, "sm ", 3) || unhex(line + 3, sm, sizeof sm, &smlen)) { puts("bad sm"); return 6; }
            m2len = 0;
            int rc = crypto_sign_open(m2, &m2len, sm, smlen, pk);
            printf("open %d rc %d ", i++, rc);
            hex("m ", m2, rc ? 0 : m2len);
        }
        free(line);
        return 0;
    }
    return 9;
}
"""


class Skip(Exception):
    pass


# --------------------------------------------------------------------------
# GF(16) = GF(2)[x]/(x^4 + x + 1), as in src/simple_arithmetic.h
# --------------------------------------------------------------------------

def _gf16_mul(a: int, b: int) -> int:
    p = 0
    for bit in range(4):
        if (a >> bit) & 1:
            p ^= b << bit
    for bit in (6, 5, 4):
        if (p >> bit) & 1:
            p ^= 0b10011 << (bit - 4)
    return p


MUL = [[_gf16_mul(a, b) for b in range(16)] for a in range(16)]
INV = [0] + [next(b for b in range(1, 16) if MUL[a][b] == 1) for a in range(1, 16)]


def decode_nibbles(data: bytes, count: int) -> list[int]:
    out = []
    for byte in data:
        out += [byte & 15, byte >> 4]
    return out[:count]


def secret_O(sk: bytes, p: dict) -> list[list[int]]:
    """O as derived in mayo_expand_sk (src/mayo.c:367-369), v rows by o columns."""
    v, o = p["n"] - p["o"], p["o"]
    stream = hashlib.shake_256(sk).digest(p["pk_seed_bytes"] + p["O_bytes"])
    nib = decode_nibbles(stream[p["pk_seed_bytes"]:], v * o)
    return [nib[r * o:(r + 1) * o] for r in range(v)]


def signature_blocks(sm: bytes, p: dict) -> list[list[int]]:
    n, k = p["n"], p["k"]
    nib = decode_nibbles(sm[: (k * n + 1) // 2], k * n)
    return [nib[i * n:(i + 1) * n] for i in range(k)]


def in_oil_space(block: list[int], O: list[list[int]], o: int) -> bool:
    """s = (O x, x) with x = last o coordinates."""
    x = block[len(O):]
    for r, row in enumerate(O):
        acc = 0
        for c in range(o):
            acc ^= MUL[row[c]][x[c]]
        if acc != block[r]:
            return False
    return True


def rref_on_oil(rows: list[list[int]], v: int, o: int) -> list[list[int]]:
    """Reduced row echelon form, pivoting on the last o coordinates only.

    Uses the signature blocks alone (no secret).  Returns the pivot rows found,
    each normalised so that its oil part is a unit vector."""
    B = [list(r) for r in rows]
    pivots = 0
    for col in range(v, v + o):
        pick = next((i for i in range(pivots, len(B)) if B[i][col]), None)
        if pick is None:
            continue
        B[pivots], B[pick] = B[pick], B[pivots]
        inv = INV[B[pivots][col]]
        B[pivots] = [MUL[inv][e] for e in B[pivots]]
        for i in range(len(B)):
            if i != pivots and B[i][col]:
                f = B[i][col]
                B[i] = [a ^ MUL[f][b] for a, b in zip(B[i], B[pivots])]
        pivots += 1
    return B[:pivots]


def recovered_O(basis: list[list[int]], v: int, o: int):
    """If the basis has o rows with oil part = identity, row j = (O e_j, e_j)."""
    if len(basis) != o:
        return None
    for j, row in enumerate(basis):
        if row[v:] != [1 if c == j else 0 for c in range(o)]:
            return None
    return [[basis[c][r] for c in range(o)] for r in range(v)]


# --------------------------------------------------------------------------
# Source checks and patch
# --------------------------------------------------------------------------

def sha256(path: Path) -> str:
    if not path.is_file():
        return "missing"
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_checks() -> tuple[dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    problems: list = []
    ok = True
    for rel, expected in sorted(PINNED.items()):
        actual = sha256(REF / rel)
        pinned[f"{SCHEME}/{REF_REL}/{rel}"] = actual
        if actual != expected:
            problems.append(f"sha256 mismatch {rel}: {actual}")
            ok = False
    checks["source_sha256_pinned_files_match"] = ok
    if not ok:
        return checks, pinned, problems

    mayo = (REF / "src/mayo.c").read_text(encoding="utf-8")
    constants = (REF / "include/mayo_constants.h").read_text(encoding="utf-8")
    arith_h = (REF / "src/arithmetic.h").read_text(encoding="utf-8")
    checks["source_patch_anchors_occur_exactly_once"] = all(
        mayo.count(anchor) == 1 for anchor in (ANCHOR_INCLUDE, ANCHOR_VINEGAR, ANCHOR_LAMBDA))
    # The vinegar is decoded from V right after the anchor, Lambda is derived
    # once before the attempt loop, and m_calculate_M_and_VPV gets both.
    try:
        at_lambda = mayo.index(ANCHOR_LAMBDA)
        at_loop = mayo.index("for (int ctr = 0; ctr <= 255; ++ctr) {", at_lambda)
        at_v = mayo.index(ANCHOR_VINEGAR, at_loop)
        at_dec = mayo.index("decode(V + i * param_v_bytes, Vdec + i * param_v, param_v);", at_v)
        at_m = mayo.index("m_calculate_M_and_VPV(p, P1, P2_or_L, sk->O, Vdec, lambda, use_l,", at_dec)
        at_s = mayo.index("m_compute_s(p, sk->O, Vdec, x, s);", at_m)
        order_ok = at_lambda < at_loop < at_v < at_dec < at_m < at_s
    except ValueError:
        order_ok = False
    checks["source_signing_flow_as_cited"] = order_ok
    checks["source_m_compute_s_is_v_plus_O_x_then_x"] = (
        "// assemble the signature vector: s_i = [ v_i + O x_i | x_i ]" in arith_h
        and "m_lambda_oil(p, lambda, Mtmp);" in arith_h)
    params_ok = True
    for s, p in PARAMS.items():
        for key, value in p.items():
            match = re.search(rf"#define MAYO_{s}_{key} (\d+)\n", constants)
            params_ok &= bool(match) and int(match.group(1)) == value
    checks["source_parameters_match_mayo_constants_h"] = params_ok
    return checks, pinned, problems


def patched_mayo_c() -> str:
    text = (REF / "src/mayo.c").read_text(encoding="utf-8")
    for anchor in (ANCHOR_INCLUDE, ANCHOR_VINEGAR, ANCHOR_LAMBDA):
        assert text.count(anchor) == 1, f"anchor not unique: {anchor!r}"
    text = text.replace(ANCHOR_INCLUDE, ANCHOR_INCLUDE + PATCH_INCLUDE)
    text = text.replace(ANCHOR_VINEGAR, ANCHOR_VINEGAR + PATCH_VINEGAR)
    text = text.replace(ANCHOR_LAMBDA, ANCHOR_LAMBDA + PATCH_LAMBDA)
    return text


# --------------------------------------------------------------------------
# Build and run
# --------------------------------------------------------------------------

def run(cmd, cwd: Path, timeout: int, stdin: str | None = None) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], cwd=cwd, input=stdin, capture_output=True,
                          text=True, timeout=timeout, env=dict(os.environ, LC_ALL="C"))


def first_line(cmd, cwd: Path) -> str:
    done = run(cmd, cwd, 60)
    text = (done.stdout or done.stderr).strip().splitlines()
    return text[0].strip() if text else ""


CFLAGS = ["-O2", "-std=gnu11", "-DMAYO_BUILD_TYPE_REF"]


def build_command(gcc: str, work: Path, s: str, fault: bool) -> tuple[list, Path]:
    binary = work / f"mayo{s}_{'fault' if fault else 'pristine'}"
    mayo_c = work / "patched" / "mayo.c" if fault else REF / "src/mayo.c"
    sources = [mayo_c] + [REF / rel for rel in COMMON[1:]] + [REF / f"src/mayo_{s}/api.c", work / "driver.c"]
    cmd = [gcc, *CFLAGS, f"-DMAYO_VARIANT=MAYO_{s}"]
    if fault:
        cmd.append("-DREPRO_FAULT_BUILD")
    cmd += [f"-I{REF / d}" for d in ("include", "src", "src/common", "src/generic", f"src/mayo_{s}")]
    cmd += sources + ["-o", binary]
    return cmd, binary


def build(gcc: str, work: Path, s: str, fault: bool) -> tuple[Path, str]:
    cmd, binary = build_command(gcc, work, s, fault)
    done = run(cmd, work, 300)
    if done.returncode:
        raise RuntimeError(f"build failed (MAYO{s} fault={fault}): {done.stderr[-800:]}")
    shown = " ".join(str(c).replace(str(HERE.parent) + "/", "").replace(str(work), "$TMP") for c in cmd)
    return binary, shown


def parse_sign(text: str) -> dict:
    out = {"calls": []}
    for line in text.splitlines():
        parts = line.split()
        if parts[0] == "pk":
            out["pk"] = bytes.fromhex(parts[1])
        elif parts[0] == "sk":
            out["sk"] = bytes.fromhex(parts[1])
        elif parts[0] == "sizes":
            out["sizes"] = [int(x) for x in parts[1:]]
        elif parts[0] == "call":
            # call <i> rc <rc> sm <hex>
            out["calls"].append((int(parts[3]), bytes.fromhex(parts[5]) if len(parts) > 5 else b""))
    return out


def sign(binary: Path, work: Path, mode: str, s: str, calls: int) -> dict:
    done = run([binary, "sign", mode, hex(KEY_SEED[s]), str(calls)], work, 540)
    if done.returncode:
        raise RuntimeError(f"sign failed (MAYO{s} {mode}): rc={done.returncode} {done.stdout[-200:]!r}")
    result = parse_sign(done.stdout)
    result["raw"] = done.stdout
    return result


def open_all(binary: Path, work: Path, pk: bytes, sms: list[bytes]) -> list[tuple[int, bytes]]:
    stdin = "pk " + pk.hex() + "\n" + "".join("sm " + sm.hex() + "\n" for sm in sms)
    done = run([binary, "open"], work, 300, stdin)
    if done.returncode:
        raise RuntimeError(f"open failed: rc={done.returncode} {done.stdout[-200:]!r}")
    results = []
    for line in done.stdout.splitlines():
        parts = line.split()
        if parts[0] == "open":
            results.append((int(parts[3]), bytes.fromhex(parts[5]) if len(parts) > 5 else b""))
    if len(results) != len(sms):
        raise RuntimeError("open: wrong number of results")
    return results


def message_of(sm: bytes, sig_bytes: int) -> bytes:
    return sm[sig_bytes:]


def evaluate_set(gcc: str, work: Path, s: str, binaries: dict) -> dict:
    p = PARAMS[s]
    n, o, k = p["n"], p["o"], p["k"]
    v = n - o
    pristine, fault = binaries[(s, False)], binaries[(s, True)]
    res: dict = {"n": n, "m": p["m"], "o": o, "k": k}

    # Harness control: patched build with both switches off == unmodified build.
    a = sign(pristine, work, "none", s, IDENTITY_CALLS)
    b = sign(fault, work, "none", s, IDENTITY_CALLS)
    res["patched_build_switches_off_identical_to_unmodified"] = a["raw"] == b["raw"]

    # Faulted signer.
    faulty = sign(fault, work, "v0", s, FAULT_CALLS)
    sig_bytes = faulty["sizes"][2]
    O = secret_O(faulty["sk"], p)
    good = [sm for rc, sm in faulty["calls"] if rc == 0]
    res["fault_calls"] = len(faulty["calls"])
    res["fault_success"] = len(good)
    res["fault_success_rc_nonzero_outputs"] = sum(1 for rc, sm in faulty["calls"] if rc != 0 and sm)
    opened = open_all(pristine, work, faulty["pk"], good)
    res["fault_signatures_verified_by_unmodified_open"] = sum(
        1 for (rc, m), sm in zip(opened, good) if rc == 0 and m == message_of(sm, sig_bytes))
    blocks = [signature_blocks(sm, p) for sm in good]
    res["fault_blocks_total"] = sum(len(bl) for bl in blocks)
    res["fault_blocks_in_oil_space"] = sum(in_oil_space(b, O, o) for bl in blocks for b in bl)
    res["fault_blocks_zero"] = sum(1 for bl in blocks for b in bl if not any(b))

    # Recovery from the signature blocks alone, in signing order: each run adds
    # signatures until the oil part has rank o, then compares with the true O.
    runs, exact = [], []
    acc: list = []
    used = 0
    for bl in blocks:
        acc += bl
        used += 1
        basis = rref_on_oil(acc, v, o)
        acc = basis
        if len(basis) == o:
            rec = recovered_O(basis, v, o)
            runs.append(used)
            exact.append(rec == O)
            acc, used = [], 0
    res["recovery_runs"] = len(runs)
    res["recovery_exact_runs"] = sum(exact)
    res["signatures_for_first_recovery"] = runs[0] if runs else None
    res["signatures_per_recovery_histogram"] = {str(x): runs.count(x) for x in sorted(set(runs))}
    res["minimum_signatures_ceil_o_over_k"] = math.ceil(o / k)

    res["faulty_key_pk_sha256"] = hashlib.sha256(faulty["pk"]).hexdigest()

    # Control 2: unfaulted signer; signatures verify, blocks are not oil vectors.
    clean = sign(pristine, work, "none", s, CLEAN_CALLS)
    cgood = [sm for rc, sm in clean["calls"] if rc == 0]
    copened = open_all(pristine, work, clean["pk"], cgood)
    res["clean_success"] = len(cgood)
    res["clean_verified"] = sum(
        1 for (rc, m), sm in zip(copened, cgood) if rc == 0 and m == message_of(sm, sig_bytes))
    cblocks = [b for sm in cgood for b in signature_blocks(sm, p)]
    res["clean_blocks_total"] = len(cblocks)
    res["clean_blocks_in_oil_space"] = sum(in_oil_space(b, O, o) for b in cblocks)
    return res


def control_lambda_zero(work: Path, s: str, binaries: dict) -> dict:
    """Control 1: Lambda also zeroed on the signer side (Round 2 behaviour).

    Every failing call runs all 256 attempts, so this is the slow part."""
    control = sign(binaries[(s, True)], work, "v0l0", s, CONTROL_CALLS[s])
    return {
        "control_lambda_zero_calls": len(control["calls"]),
        "control_lambda_zero_failures": sum(1 for rc, _ in control["calls"] if rc != 0),
        "control_key_pk_sha256": hashlib.sha256(control["pk"]).hexdigest(),
    }


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def emit(result: dict, output: Path | None) -> int:
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
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

    gcc = shutil.which("gcc")
    if not gcc:
        result["status"] = "SKIP"
        result["skip_reason"] = "gcc not found"
        return emit(result, args.output)

    with tempfile.TemporaryDirectory(prefix="mayo-se1-") as temporary:
        work = Path(temporary)
        (work / "driver.c").write_text(DRIVER_C, encoding="utf-8")
        (work / "patched").mkdir()
        (work / "patched" / "mayo.c").write_text(patched_mayo_c(), encoding="utf-8")
        result["measurements"]["tools"] = {
            "gcc": first_line([gcc, "--version"], work),
            "machine": platform.machine(),
        }
        result["measurements"]["patch"] = {
            "file": "src/mayo.c (copy in the temporary directory)",
            "after_includes": PATCH_INCLUDE.strip(),
            "after_vinegar_shake256": PATCH_VINEGAR.strip(),
            "after_derive_lambda": PATCH_LAMBDA.strip(),
        }
        sets = list(PARAMS)
        try:
            with ThreadPoolExecutor(max_workers=JOBS) as pool:
                jobs = [(s, fault) for s in sets for fault in (False, True)]
                built = list(pool.map(lambda job: build(gcc, work, *job), jobs))
                binaries = {job: b[0] for job, b in zip(jobs, built)}
                result["measurements"]["build_commands"] = {
                    f"MAYO{s}_{'fault' if fault else 'pristine'}": b[1]
                    for (s, fault), b in zip(jobs, built)}
                # Longest jobs first: the Lambda = 0 controls of the large sets.
                tasks = [("control", s) for s in ("5", "3", "2", "1")] + [("main", s) for s in sets]
                outcomes = list(pool.map(
                    lambda task: (control_lambda_zero(work, task[1], binaries) if task[0] == "control"
                                  else evaluate_set(gcc, work, task[1], binaries)), tasks))
                per_set = {s: {} for s in sets}
                for (_, s), outcome in zip(tasks, outcomes):
                    per_set[s].update(outcome)
                for s in sets:
                    per_set[s]["control_same_key"] = (
                        per_set[s].pop("control_key_pk_sha256") == per_set[s]["faulty_key_pk_sha256"])
        except (RuntimeError, subprocess.TimeoutExpired, OSError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    result["measurements"]["sets"] = {f"MAYO{s}": r for s, r in per_set.items()}
    c = result["checks"]
    for s, r in per_set.items():
        tag = f"MAYO{s}"
        c[f"{tag}_faulty_signer_succeeds_for_most_calls"] = r["fault_success"] > r["fault_calls"] // 2
        c[f"{tag}_every_faulty_signature_passes_unmodified_open"] = (
            r["fault_success"] > 0 and r["fault_signatures_verified_by_unmodified_open"] == r["fault_success"])
        c[f"{tag}_every_faulty_signature_block_is_in_oil_space"] = (
            r["fault_blocks_total"] > 0 and r["fault_blocks_in_oil_space"] == r["fault_blocks_total"])
        c[f"{tag}_row_reduction_reproduces_O_exactly"] = (
            r["recovery_runs"] > 0 and r["recovery_exact_runs"] == r["recovery_runs"])
        c[f"{tag}_first_recovery_used_review_signature_count"] = (
            r["signatures_for_first_recovery"] == REVIEW["signatures_to_recover_O"][s])
        c[f"{tag}_control_lambda_zero_makes_every_faulty_call_fail"] = (
            r["control_lambda_zero_calls"] == CONTROL_CALLS[s]
            and r["control_lambda_zero_failures"] == CONTROL_CALLS[s] and r["control_same_key"])
        c[f"{tag}_control_unfaulted_signatures_verify_and_are_not_oil"] = (
            r["clean_success"] == CLEAN_CALLS and r["clean_verified"] == CLEAN_CALLS
            and r["clean_blocks_in_oil_space"] == 0)
        c[f"{tag}_control_patched_build_switches_off_identical"] = (
            r["patched_build_switches_off_identical_to_unmodified"])
    result["measurements"]["review_values"] = REVIEW
    result["measurements"]["calls"] = {"fault": FAULT_CALLS,
                                       "control_lambda_zero": {f"MAYO{s}": x for s, x in CONTROL_CALLS.items()},
                                       "clean": CLEAN_CALLS, "key_seed": {f"MAYO{s}": hex(x)
                                                                          for s, x in KEY_SEED.items()}}
    result["measurements"]["seconds"] = round(time.monotonic() - started, 1)
    result["status"] = "PASS" if all(c.values()) else "FAIL"
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
