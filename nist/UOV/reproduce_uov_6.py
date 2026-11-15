#!/usr/bin/env python3
"""Reproduce UOV-6: clang's jumps on secret multiplier bits in UOV signing.

The scalar-by-vector multiplications ``gf256v_mul_u32`` and ``gf16v_mul_u32``
(``gf16.h``; ``gf256v_mul_u64`` and ``gf16v_mul_u64`` in ``gf16_u64.h`` of the
amd64 tree) are written as branch-free C: ``r32 ^= (a32) * ((b32 >> k) & 1);``.
The review of 2026-11-15 (clang 21.1.8, gcc 15.2.0, Valgrind 3.26.0, x86-64)
states that clang at -O2 and -O3 vectorises the calling loop and emits one
conditional jump per bit of the multiplier ``b`` in the reference and amd64
trees, and that gcc -O2/-O3, clang -O1 and the AVX2 tree do not.  During
signing the multiplier is a vinegar element (``ov.c:138``, ``ov.c:141``) or an
elimination pivot inverse or row multiplier (``ov.c:146``, ``ov.c:156``).

This script

1. checks the cited source lines against pinned SHA-256 values (the four
   multiplication functions are branch-free C as quoted; ``ov.c`` and the
   callers pass the operands the review names);
2. compiles the reference ``blas_matrix_ref.c`` of uov-Ip and uov-Is with a
   host clang (-O1, -O2, -O3) and with gcc (-O2, -O3) and counts, with
   ``objdump -d -l``, instruction pairs "``test $<single bit>,<operand>``
   directly followed by ``je``/``jne``", in total and where the DWARF line of
   the pair lies inside a multiplication function;
3. builds the unmodified uov-Ip and uov-Is signers (no submitted build file is
   run) with a small embedded driver whose ``randombytes`` can mark its output
   undefined for Valgrind Memcheck, signs once, and classifies every Memcheck
   report by its innermost frame and by its ``ov_sign_core`` frame.  Taint
   modes: ``sign`` (the review's harness: whole secret key and all signing
   randomness undefined; the released signature is declassified before
   verification), ``seed`` (only the 32-byte secret seed undefined, the salt
   stays defined), ``clean`` (nothing undefined; harness control).  In ``seed``
   mode the vector operand of the product at ``ov.c:138`` (the key part S) is
   defined, so a report there depends on the multiplier alone.  For the clang
   builds every reported address is looked up in the disassembly: it must be a
   conditional jump that uses the flags of a single-bit ``test`` (the test is
   directly before the jump, or separated from it only by moves, ``lea``,
   ``nop``, ``pxor`` or ``pshufd``, which leave the flags unchanged);
4. records the ``CC`` line of the submitted Makefiles as a source fact.  No
   Makefile is run.

Controls: the gcc -O2/-O3 and clang -O1 builds of the same sources, and the
clang -O3 build of the AVX2 tree (run only if the host CPU has AVX2 and
AES-NI), report nothing but the known retry branch ``if ( !l1_succ )`` at
``ov.c:150`` / ``ov.c:160`` (UOV-1); the untainted clang -O3 run reports
nothing.

Result with clang 21.1.8 (the counts are measurements, not checks): the jumps
test bits 0..6 of the GF(256) multiplier and bits 0..2 of the GF(16)
multiplier inside the vectorised loop; the top bit is handled without a jump.
Of the 30 (-O2) and 31 (-O3) single-bit pairs in the uov-Ip object, 28 are in
``gf256v_mul_u32``; the other two or three test the parity of a public loop
count (``blas_matrix_ref.c:251``, ``258``) and Memcheck does not report them.

Limits: this is object inspection and taint tracking.  There is no timing,
cache or branch-predictor measurement and no key recovery.  The same branches
on the oil solution (``ov.c:153``, ``163``, ``182``) concern a value the
signature releases and are counted separately.  Only classic uov-Ip and uov-Is
signing is run (reference tree, plus uov-Ip of the amd64 and AVX2 trees); key
generation, the other parameter sets, the GFNI and NEON trees, other
architectures and compilers other than the two found on the host are not
covered.  The result is tied to the compiler versions recorded in the output.

Build: ``<cc> -g <opt> -std=c99 -fno-omit-frame-pointer [-mavx2 -maes] -no-pie
-D_NIST_KAT_ -I<set> -I<nistkat> <set>/*.c taint_driver.c -lcrypto``.  At -O3
these are the submitted CFLAGS without the warning options, plus ``-g``.
``-D_NIST_KAT_`` only selects the ``int randombytes()`` prototype of
``nistkat/rng.h``; ``nistkat/rng.c`` is replaced by the driver.  ``_VALGRIND_``
is not defined, so the declassification that the source applies to the retry
flag is inactive.  ``-no-pie`` (link only) makes the reported addresses equal
to the addresses in the disassembly.

``source_sha256`` lists the cited files, and for every compiled directory one
entry ``<directory>/*``: the SHA-256 of the output of ``sha256sum *`` run in
that directory with ``LC_ALL=C``.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (not x86-64, no host-targeting
clang-21..clang-15 or clang, no gcc, no objdump, no working Valgrind, or no
``valgrind/memcheck.h`` / OpenSSL development files).
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
SUBMISSION = "submission/UOV-R3"
REF = f"{SUBMISSION}/Reference_Implementation"
OPT = f"{SUBMISSION}/Optimized_Implementation"

CANDIDATE = "UOV-6"
EVIDENCE = (
    "Object inspection (objdump) and Valgrind Memcheck taint tracking show that clang at -O2 and "
    "-O3 compiles the branch-free C of UOV's scalar-by-vector field multiplication "
    "(gf256v_mul_u32, gf16v_mul_u32 and their u64 forms) in the reference and amd64 trees into "
    "conditional jumps on single bits of the multiplier, which during signing is a vinegar element "
    "or an elimination multiplier, while gcc -O2/-O3, clang -O1 and the AVX2 tree show only the "
    "known retry branch (UOV-1); this shows a compiler-dependent branch on secret multiplier bits "
    "only, with no timing measurement and no key recovery."
)
EVIDENCE_WITHOUT_AVX2 = EVIDENCE.replace(
    "gcc -O2/-O3, clang -O1 and the AVX2 tree show", "gcc -O2/-O3 and clang -O1 show").replace(
    "no key recovery.", "no key recovery; the AVX2 tree was not run on this host.")

SIGNATURES = 1
JOBS = 3

# tree -> directory, extra compiler flags, files that hold the multiplication
# and its inlined callers.
TREES = {
    "ref": {"path": REF, "arch": [], "mul_files": ("gf16.h", "blas_u32.h")},
    "amd64": {"path": f"{OPT}/amd64", "arch": [],
              "mul_files": ("gf16_u64.h", "blas_u64.h", "gf16.h", "blas_u32.h")},
    "avx2": {"path": f"{OPT}/avx2", "arch": ["-mavx2", "-maes"],
             "mul_files": ("gf16_u64.h", "blas_u64.h", "gf16.h", "blas_u32.h")},
}

# Every compiled directory: (number of files, SHA-256 of its `sha256sum *` listing).
SET_DIGESTS = {
    "ref/Ip": (35, "aaa928be69731067894fd42daa87617fe00e6cad798fb7041ebec315971cf998"),
    "ref/Is": (35, "599c29d4d84b7cf433ef8eabfb08b745708e679263bf7d1c86895d136a25dd91"),
    "amd64/Ip": (37, "05df1c6908f5f1728b3429aea5f4dd352e673558958cd0bc1ab598acfd87e0f8"),
    "avx2/Ip": (51, "e110dd329306a7a636da799de7e84f001ff45896f8c9fdc14cb9cb2b2b31b5c0"),
}

GF16_H = "9da7342eaf703de6710d93fc6c761ffb2181d8e9621c4ae7a9aa95fdf0be8001"
OV_C = "863e59ecf3838fa8643c3a65e0bfa8cb86eeefaec7a31f1260767b2a7444affc"
BLAS_MATRIX_REF_C = "9c7dda6195a6b290396a9897e52821f893502e0b269e2610016ecfb560bfbcf1"
BLAS_U32_H = "a34d9b48ad46d43cae5af6eb6e12a2f4e2d81007665178b3e54f5ec1c40f89ad"
RNG_H = "81209b4388f6171e71b2885fbdaf239435342d2621be3c3cfd34a50374bbc959"
MAKEFILE_REF = "9929bbe4c276f90f2f04741a0ba6ea042a29f64964433d7ba4401194e3dc8cc5"

# Cited files (paths relative to this script's directory).
FILE_SHA256 = {
    f"{REF}/Ip/gf16.h": GF16_H,
    f"{REF}/Ip/blas_u32.h": BLAS_U32_H,
    f"{REF}/Ip/blas_matrix_ref.c": BLAS_MATRIX_REF_C,
    f"{REF}/Ip/ov.c": OV_C,
    f"{REF}/Ip/config.h": "3e78b48229f000ff753a8d4d745d4a7241c260c6eca1c551bfca6c9fedd9195f",
    f"{REF}/Ip/params.h": "37246e6fb555f600df1d0865a60a8ad5c60cd8c7331103e0f81c9a827cdfe919",
    f"{REF}/Is/gf16.h": GF16_H,
    f"{REF}/Is/blas_u32.h": BLAS_U32_H,
    f"{REF}/Is/blas_matrix_ref.c": BLAS_MATRIX_REF_C,
    f"{REF}/Is/ov.c": OV_C,
    f"{REF}/Is/params.h": "fad4bf5765ae92b7010c6305ce484d4bfb2ecfdadd8294b63adc7e3db68629e2",
    f"{REF}/nistkat/rng.h": RNG_H,
    f"{REF}/Makefile": MAKEFILE_REF,
    f"{OPT}/amd64/Ip/gf16_u64.h": "adb0b12e64bbbc3174266a3266dd0cbd2c790926cd3c1a94cb61cf42bb08d0c6",
    f"{OPT}/amd64/Ip/blas_u64.h": "1c0e162f7be45bad00014267ae9584edbf44945a0f9d88109a81756001edff91",
    f"{OPT}/amd64/Ip/blas_matrix_ref.c": BLAS_MATRIX_REF_C,
    f"{OPT}/amd64/Ip/ov.c": OV_C,
    f"{OPT}/amd64/Ip/config.h": "36b8a4814152cf29aea44d1a9395dce6569b19b63b61230bd7a13c99d3cfd6d8",
    f"{OPT}/amd64/nistkat/rng.h": RNG_H,
    f"{OPT}/amd64/Makefile": MAKEFILE_REF,
    f"{OPT}/avx2/Ip/ov.c": OV_C,
    f"{OPT}/avx2/Ip/config.h": "d8fe5704694e542e96e6cb38f705503624674f068de1a0df6cfe52e75d0615b0",
    f"{OPT}/avx2/nistkat/rng.h": RNG_H,
    f"{OPT}/avx2/Makefile": "ff1124ba345876061e715874decccdb0df99d46cbdb50e2afa985d7479fd6eb9",
    f"{OPT}/gfni/Makefile": "b01a24b9fac814202d6e94483874034de39c7f0c2c6c805ff3420acd396d7216",
    f"{OPT}/neon/Makefile": "d887928e4f8f2b2885224b4432cca0618bf5bcb632f64c41d6e8ce7ec1c583e5",
}

# The multiplication functions.  Line numbers are 1-based and are those of the
# pinned files: (function, first line, last line, line of the bit-0 statement,
# number of multiplier bits, word type, result variable, operand variable).
# The statement for bit k >= 1 is five lines after the one for bit k - 1.
MUL_SOURCE = {
    f"{REF}/Ip/gf16.h": [
        ("gf16v_mul_u32", 183, 206, 187, 4, "uint32_t", "r32", "a32"),
        ("gf256v_mul_u32", 251, 293, 255, 8, "uint32_t", "r32", "a32"),
    ],
    f"{OPT}/amd64/Ip/gf16_u64.h": [
        ("gf16v_mul_u64", 30, 53, 35, 4, "uint64_t", "r64", "a64"),
        ("gf256v_mul_u64", 98, 141, 103, 8, "uint64_t", "r64", "a64"),
    ],
}
MUL_RANGES = {
    Path(rel).name: [(first, last) for _, first, last, *_ in functions]
    for rel, functions in MUL_SOURCE.items()
}

# Lines the review cites for the operands (compared with whitespace removed).
CITED_LINES = {
    f"{REF}/Ip/ov.c": {
        85: "gfmat_prod( mat_l1, S, _O * _O_BYTE, _V, vinegar );",
        97: "int ov_sign_core( uint8_t *signature, const sk_t *sk, const uint8_t *message, size_t mlen ) {",
        121: "hash_update(&h_vinegar_copy, sk->sk_seed, LEN_SKSEED );   // H(M||salt||sk_seed ...",
        130: "hash_final_digest( vinegar, _V_BYTE, &h_vinegar);   // H(M||salt||sk_seed||ctr)",
        138: "ov_calc_l1_mat( mat_l1, sk, vinegar );",
        141: "batch_quad_trimat_eval( r_l1_F1, sk->P1, vinegar, _V, _O_BYTE );",
        146: "unsigned l1_succ = gf256mat_gaussian_elim(mat_l1, r_l1_F1, _O);",
        150: "if ( !l1_succ ) {",
        153: "gf256mat_back_substitute(r_l1_F1, mat_l1, _O);",
        156: "unsigned l1_succ = gf16mat_gaussian_elim(mat_l1, r_l1_F1, _O);",
        160: "if ( !l1_succ ) {",
        163: "gf16mat_back_substitute(r_l1_F1, mat_l1, _O);",
        179: "memcpy( w + _V_BYTE, x_o1, _O_BYTE );",
        182: "gfmat_prod(y, sk->O, _V_BYTE, _O, x_o1 );",
    },
    f"{REF}/Ip/blas_matrix_ref.c": {
        107: "gf16v_mul_scalar( ai + i_start, pivot, w_byte - i_start );",
        110: "gf16v_madd( aj + i_start, ai + i_start, gf16v_get_ele(aj, idx), w_byte - i_start );",
        201: "gf256v_madd(c, matA, b[i], n_A_vec_byte);",
        229: "gf256v_mul_scalar( ai + i_start, pivot, w - i_start );",
        232: "gf256v_madd( aj + i_start, ai + i_start, aj[idx], w - i_start );",
    },
    f"{REF}/Ip/blas_u32.h": {
        122: "ax[0] = gf16v_mul_u32( ax[0], gf16_b );",
        188: "ax[0] = gf256v_mul_u32( ax[0], b );",
        257: "cx[0] ^= gf16v_mul_u32( ax[0], gf16_b );",
        325: "cx[0] ^= gf256v_mul_u32( ax[0], gf256_b );",
        365: "_store_u32( accu_c, _load_u32(accu_c) ^ gf256v_mul_u32( _load_u32(a), gf256_b ) );",
    },
    f"{OPT}/amd64/Ip/blas_u64.h": {
        206: "ax[0] = gf256v_mul_u64( ax[0], gf256_b );",
        343: "cx[0] ^= gf256v_mul_u64( ax[0], gf256_b );",
        380: "_store_u64( accu_c, _load_u64(accu_c) ^ gf256v_mul_u64( _load_u64(a), gf256_b ) );",
    },
}

MAKEFILES = [f"{REF}/Makefile", f"{OPT}/amd64/Makefile", f"{OPT}/avx2/Makefile",
             f"{OPT}/gfni/Makefile", f"{OPT}/neon/Makefile"]

# ov.c lines (frame of ov_sign_core) by parameter set.
RETRY_LINE = {"Ip": 150, "Is": 160}            # if ( !l1_succ ): UOV-1
SECRET_LINES = {"Ip": (138, 141, 146), "Is": (138, 141, 156)}   # vinegar, elimination
RELEASED_LINES = {"Ip": (153, 182), "Is": (163, 182)}           # oil solution

# (tree, set, compiler, optimisation flag)
OBJECT_PLAN = [("ref", set_name, compiler, opt)
               for set_name in ("Ip", "Is")
               for compiler, opt in (("clang", "-O1"), ("clang", "-O2"), ("clang", "-O3"),
                                     ("gcc", "-O2"), ("gcc", "-O3"))]

# (tree, set, compiler, optimisation flag, taint modes)
TAINT = {
    "sign": "whole secret key and all signing randomness (salt) undefined",
    "seed": "only the 32-byte secret seed sk[0..32] undefined",
    "clean": "nothing undefined (harness control)",
}
TAINT_PLAN = [
    ("ref", "Ip", "clang", "-O3", ("sign", "seed", "clean")),
    ("ref", "Ip", "clang", "-O2", ("sign",)),
    ("ref", "Ip", "clang", "-O1", ("sign",)),
    ("ref", "Ip", "gcc", "-O2", ("sign", "seed")),
    ("ref", "Ip", "gcc", "-O3", ("sign",)),
    ("ref", "Is", "clang", "-O3", ("sign",)),
    ("ref", "Is", "gcc", "-O2", ("sign",)),
    ("amd64", "Ip", "clang", "-O3", ("sign",)),
    ("amd64", "Ip", "gcc", "-O2", ("sign",)),
]
AVX2_JOB = ("avx2", "Ip", "clang", "-O3", ("sign",))

DRIVER_C = r"""
/* Memcheck taint driver for the UOV signer (reproducer harness, not submission
 * code).  randombytes() replaces nistkat/rng.c: a deterministic xorshift
 * stream whose output is marked undefined while taint_rng is set. */
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
    unsigned long long i;
    for (i = 0; i < xlen; i++) x[i] = rng_next();
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
    int it;
    if (!sm || !m2) return 5;
    if (strcmp(mode, "sign") && strcmp(mode, "seed") && strcmp(mode, "clean")) return 6;
    for (it = 0; it < iters; it++) {
        int rc;
        rng_state = 0x1234567ULL + 7919ULL * (uint64_t)it;
        if (crypto_sign_keypair(pk, sk)) { puts("keypair failed"); return 2; }
        if (!strcmp(mode, "sign")) {
            VALGRIND_MAKE_MEM_UNDEFINED(sk, sizeof sk);
            taint_rng = 1;
        } else if (!strcmp(mode, "seed")) {
            /* sk_t starts with sk_seed[LEN_SKSEED] (ov_keypair.h) */
            VALGRIND_MAKE_MEM_UNDEFINED(sk, LEN_SKSEED);
        }
        rc = crypto_sign(sm, &smlen, m, mlen, sk);
        taint_rng = 0;
        /* The return code, the length and the signed message are released. */
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

PLAIN_C = r"""
#include <stdio.h>
int main(void) { puts("plain"); return 0; }
"""

PROBE_C = r"""
#include <stdio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <valgrind/memcheck.h>
int main(void) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = ctx != NULL && EVP_DigestInit_ex(ctx, EVP_shake256(), NULL) == 1;
    EVP_MD_CTX_free(ctx);
    printf("probe %d %d\n%s\n", ok, (int)RUNNING_ON_VALGRIND, OpenSSL_version(OPENSSL_VERSION));
    return 0;
}
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


def squash(text: str) -> str:
    return "".join(text.split())


# --------------------------------------------------------------------------
# Source checks
# --------------------------------------------------------------------------

CONTROL_FLOW = re.compile(r"\b(?:if|else|for|while|do|switch|case|goto)\b|\?|&&|\|\|")
CALL = re.compile(r"[A-Za-z_]\w*\s*\(")


def directory_digest(directory: Path) -> tuple[int, str]:
    """SHA-256 of what `LC_ALL=C sha256sum *` prints in the directory."""
    if not directory.is_dir():
        return 0, "missing"
    names = sorted(p.name for p in directory.iterdir())
    if any(not (directory / name).is_file() for name in names):
        return len(names), "not a flat directory"
    listing = "".join(f"{sha256(directory / name)}  {name}\n" for name in names)
    return len(names), hashlib.sha256(listing.encode()).hexdigest()


def multiplication_is_branch_free(lines: list, spec: tuple) -> bool:
    name, first, last, bit0, bits, word, result, operand = spec
    if len(lines) < last:
        return False
    ok = squash(lines[first - 1]) == squash(f"static inline {word} {name}({word} a, uint8_t b) {{")
    ok &= squash(lines[last - 1]) == "}"
    ok &= squash(lines[bit0 - 1]) == squash(f"{word} {result} = {operand} * (b32 & 1);")
    for k in range(1, bits):
        ok &= squash(lines[bit0 - 1 + 5 * k]) == squash(f"{result} ^= ({operand}) * ((b32 >> {k}) & 1);")
    body = [line.split("//")[0] for line in lines[first - 1:last]]
    # No control flow, no ?:, no && or ||, no preprocessor line and no call.
    ok &= not any(CONTROL_FLOW.search(line) or line.lstrip().startswith("#") for line in body)
    ok &= not any(CALL.search(line) for line in body[1:])
    return bool(ok)


def source_checks() -> tuple[dict, dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    facts: dict = {}
    problems: list = []

    ok = True
    for rel, expected in FILE_SHA256.items():
        actual = sha256(HERE / rel)
        pinned[f"{SCHEME}/{rel}"] = actual
        if actual != expected:
            problems.append(f"sha256 mismatch {SCHEME}/{rel}: {actual}")
            ok = False
    for key, (count, expected) in SET_DIGESTS.items():
        tree, set_name = key.split("/")
        rel = f"{TREES[tree]['path']}/{set_name}"
        found, actual = directory_digest(HERE / rel)
        pinned[f"{SCHEME}/{rel}/*"] = actual
        if actual != expected or found != count:
            problems.append(f"directory digest mismatch {SCHEME}/{rel}: {found} files, {actual}")
            ok = False
    checks["source_sha256_pinned_files_match"] = ok
    if not ok:
        return checks, pinned, facts, problems

    def text_lines(rel: str) -> list:
        return (HERE / rel).read_text(encoding="utf-8").splitlines()

    u32 = f"{REF}/Ip/gf16.h"
    u64 = f"{OPT}/amd64/Ip/gf16_u64.h"
    checks["source_gf256v_mul_u32_and_gf16v_mul_u32_are_branch_free_c_as_quoted"] = all(
        multiplication_is_branch_free(text_lines(u32), spec) for spec in MUL_SOURCE[u32])
    checks["source_u64_forms_in_amd64_tree_are_branch_free_c_as_quoted"] = all(
        multiplication_is_branch_free(text_lines(u64), spec) for spec in MUL_SOURCE[u64])

    cited = True
    for rel, wanted in CITED_LINES.items():
        lines = text_lines(rel)
        for number, text in wanted.items():
            if number > len(lines) or squash(lines[number - 1]) != squash(text):
                problems.append(f"{SCHEME}/{rel}:{number} is not the cited line")
                cited = False
    # ov_sign_core is defined once, in ov.c; frames of that function are ov.c lines.
    cited &= sum(1 for line in text_lines(f"{REF}/Ip/ov.c") if "ov_sign_core(" in line
                 and line.startswith("int ")) == 1
    checks["source_ov_c_and_caller_operand_lines_as_cited"] = cited

    # Makefile facts (nothing is run).
    cc_lines = {}
    for rel in MAKEFILES:
        found = [line for line in text_lines(rel) if re.match(r"CC\s*[?:+]?=", line)]
        cc_lines[f"{SCHEME}/{rel}"] = found[0] if len(found) == 1 else found
    reference = cc_lines[f"{SCHEME}/{REF}/Makefile"]
    checks["source_reference_makefile_cc_line_is_conditional_default_clang"] = (
        isinstance(reference, str) and re.fullmatch(r"CC \?=\s+clang", reference) is not None)
    cflags = [line for line in text_lines(f"{REF}/Makefile") if line.startswith("CFLAGS")]
    facts["submitted_makefiles"] = {
        "note": ("source fact only, no Makefile is run; '?=' assigns only if CC is not already "
                 "set, and the review states that GNU make's built-in CC=cc counts as set"),
        "cc_lines": cc_lines,
        "reference_cflags_line": cflags[0] if cflags else None,
    }
    return checks, pinned, facts, problems


# --------------------------------------------------------------------------
# Tools
# --------------------------------------------------------------------------

SYMBOL = re.compile(r"^[0-9a-f]+ <(.+)>:$")
SOURCE = re.compile(r"^(\S.*?):(\d+)(?: \(discriminator \d+\))?$")
INSN = re.compile(r"^\s+([0-9a-f]+):\s+(\S+)\s*(.*?)\s*$")
IMMEDIATE = re.compile(r"^\$(0x[0-9a-f]+|\d+),")
KEEPS_FLAGS = re.compile(r"mov\w*|lea\w*|nop\w*|pxor|pshufd")


def disassemble(tools: dict, work: Path, path: Path, with_lines: bool) -> list:
    """Instructions as dicts: address, function, mnemonic, operands, (file, line)."""
    cmd = [tools["objdump"], "-d", "--no-show-raw-insn"] + (["-l"] if with_lines else []) + [path]
    done = run(cmd, work, 300)
    if done.returncode:
        raise RuntimeError(f"objdump failed on {path.name}: {done.stderr[-300:]}")
    listing = []
    function, where = "", ("", 0)
    for raw in done.stdout.splitlines():
        match = INSN.match(raw)
        if match:
            listing.append({"address": int(match.group(1), 16), "function": function,
                            "mnemonic": match.group(2),
                            "operands": re.sub(r"\s*<.*?>", "", match.group(3)), "where": where})
            continue
        match = SYMBOL.match(raw)
        if match:
            function, where = match.group(1), ("", 0)
            continue
        match = SOURCE.match(raw)
        if match:
            where = (match.group(1).rsplit("/", 1)[-1], int(match.group(2)))
    return listing


def single_bit_test(instruction: dict):
    """The mask if the instruction is `test $<one of bits 0..7>, <operand>`."""
    if not re.fullmatch(r"test[bwlq]?", instruction["mnemonic"]):
        return None
    match = IMMEDIATE.match(instruction["operands"])
    if not match:
        return None
    value = int(match.group(1), 0)
    return value if value in (1, 2, 4, 8, 16, 32, 64, 128) else None


def is_conditional_jump(instruction: dict) -> bool:
    mnemonic = instruction["mnemonic"]
    return mnemonic.startswith("j") and not mnemonic.startswith("jmp")


def tested_bit(listing: list, position: int) -> tuple:
    """(mask, adjacent) of the single-bit test whose flags the jump at position uses.

    Up to eight instructions that do not write the flags (moves, lea, nop,
    pxor, pshufd) may stand between the test and the jump."""
    for back in range(1, 10):
        if position < back or listing[position - back]["function"] != listing[position]["function"]:
            break
        mask = single_bit_test(listing[position - back])
        if mask is not None:
            return mask, back == 1
        if not KEEPS_FLAGS.fullmatch(listing[position - back]["mnemonic"]):
            break
    return None, False


def compiles(compiler: str, work: Path, tag: str, source: str, libs: list) -> tuple:
    """Compile a small program; return (binary or None, detail)."""
    path = work / f"{tag}.c"
    path.write_text(source, encoding="utf-8")
    binary = work / tag
    try:
        built = run([compiler, "-std=c99", "-O1", path, *libs, "-o", binary], work, 120)
    except (OSError, subprocess.TimeoutExpired) as error:
        return None, str(error)
    if built.returncode:
        return None, built.stderr.strip()[-300:]
    return binary, ""


def runs_natively(binary: Path, work: Path, expected: str) -> bool:
    try:
        done = run([binary], work, 30)
    except (OSError, subprocess.TimeoutExpired):   # e.g. a cross compiler's output
        return False
    return done.returncode == 0 and done.stdout.splitlines()[:1] == [expected]


def usable_compiler(compiler: str, work: Path, tag: str) -> str:
    """'ok', 'cross' (cannot build and run a plain program) or a reason to skip."""
    plain, _ = compiles(compiler, work, f"plain_{tag}", PLAIN_C, [])
    if not plain or not runs_natively(plain, work, "plain"):
        return "cross"
    probe, detail = compiles(compiler, work, f"probe_{tag}", PROBE_C, ["-lcrypto"])
    if not probe:
        return f"cannot build with valgrind/memcheck.h and OpenSSL: {detail}"
    if not runs_natively(probe, work, "probe 1 0"):
        return "the OpenSSL SHAKE256 probe does not run"
    return "ok"


def find_tools(work: Path) -> dict:
    if platform.machine() not in ("x86_64", "AMD64"):
        raise Skip(f"the object pattern is specific to x86-64, this host is {platform.machine()}")
    tools = {}
    for name in ("valgrind", "objdump", "gcc"):
        tools[name] = shutil.which(name)
        if not tools[name]:
            raise Skip(f"{name} not found")
    state = usable_compiler(tools["gcc"], work, "gcc")
    if state != "ok":
        raise Skip(f"gcc: {state}")
    # Plain "clang" may be a cross compiler: versioned host compilers first.
    tried = []
    for name in [f"clang-{version}" for version in range(21, 14, -1)] + ["clang"]:
        candidate = shutil.which(name)
        if not candidate:
            continue
        state = usable_compiler(candidate, work, name.replace("-", ""))
        tried.append(f"{name}: {state}")
        if state == "ok":
            tools["clang"] = candidate
            break
        if state != "cross":
            raise Skip(f"{name}: {state}")
    if "clang" not in tools:
        raise Skip("no host-targeting clang-21..clang-15 or clang found"
                   + (f" ({'; '.join(tried)})" if tried else ""))
    # Valgrind itself must run: the probe prints 1 for RUNNING_ON_VALGRIND.
    try:
        probe = run([tools["valgrind"], "--tool=memcheck", "-q", work / "probe_gcc"], work, 120)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"valgrind cannot be run: {error}") from error
    if probe.returncode or probe.stdout.splitlines()[:1] != ["probe 1 1"]:
        raise Skip(f"valgrind does not run a test program: {probe.stderr.strip()[-300:]}")
    tools["libcrypto"] = probe.stdout.splitlines()[1].strip()
    try:
        listing = disassemble(tools, work, work / "plain_gcc", False)
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"objdump cannot disassemble a host program: {error}") from error
    if not any(item["mnemonic"].startswith("ret") for item in listing):
        raise Skip("objdump output is not an x86-64 AT&T listing")
    # The object check needs the source lines of clang's debug information.
    plain = work / "plain_lines"
    try:
        built = run([tools["clang"], "-g", "-std=c99", "-O1", work / "plain_gcc.c", "-o", plain], work, 120)
        lines = disassemble(tools, work, plain, True) if built.returncode == 0 else []
    except (RuntimeError, OSError, subprocess.TimeoutExpired):
        lines = []
    if not any(item["where"][0] == "plain_gcc.c" for item in lines):
        raise Skip("objdump -l prints no source lines for objects built by this clang")
    return tools


def host_has_avx2_aes() -> bool:
    try:
        text = Path("/proc/cpuinfo").read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    match = re.search(r"^flags\s*:(.*)$", text, re.M)
    return bool(match) and {"avx2", "aes"} <= set(match.group(1).split())


# --------------------------------------------------------------------------
# Object code: single-bit test-and-branch pairs
# --------------------------------------------------------------------------

def in_multiplication(where: tuple) -> bool:
    name, line = where
    return any(first <= line <= last for first, last in MUL_RANGES.get(name, ()))


def source_flags(tree: str, set_name: str) -> tuple:
    info = TREES[tree]
    set_dir = HERE / info["path"] / set_name
    return set_dir, ["-std=c99", "-fno-omit-frame-pointer", *info["arch"], "-D_NIST_KAT_",
                     f"-I{set_dir}", f"-I{HERE / info['path'] / 'nistkat'}"]


def object_scan(tools: dict, work: Path, job: tuple) -> dict:
    tree, set_name, compiler, opt = job
    set_dir, flags = source_flags(tree, set_name)
    target = work / f"blas_matrix_ref_{tree}_{set_name}_{compiler}_{opt.lstrip('-')}.o"
    done = run([tools[compiler], "-c", "-g", opt, *flags, set_dir / "blas_matrix_ref.c", "-o", target],
               work, 300)
    if done.returncode:
        raise RuntimeError(f"compile failed ({target.name}): {done.stderr[-800:]}")
    listing = disassemble(tools, work, target, True)
    result = {"pairs": 0, "pairs_in_field_multiplication": 0, "bit_masks_in_field_multiplication": [],
              "pairs_by_jump_line": {}}
    masks = set()
    for test, jump in zip(listing, listing[1:]):
        mask = single_bit_test(test)
        if (mask is None or jump["mnemonic"] not in ("je", "jne", "jz", "jnz")
                or jump["function"] != test["function"]):
            continue
        result["pairs"] += 1
        label = f"{jump['where'][0]}:{jump['where'][1]}"
        result["pairs_by_jump_line"][label] = result["pairs_by_jump_line"].get(label, 0) + 1
        # The test is usually attributed to the preceding statement, the jump
        # to the multiplication statement itself.
        if in_multiplication(jump["where"]) or in_multiplication(test["where"]):
            result["pairs_in_field_multiplication"] += 1
            masks.add(mask)
    result["bit_masks_in_field_multiplication"] = [f"0x{mask:02x}" for mask in sorted(masks)]
    result["pairs_by_jump_line"] = dict(sorted(result["pairs_by_jump_line"].items()))
    return result


# --------------------------------------------------------------------------
# Taint runs
# --------------------------------------------------------------------------

MUL_FRAME = re.compile(r"_?gf(?:16|256)v_(?:mul|madd|mul_scalar)_u(?:32|64)(?:_aligned)?")


def build(tools: dict, work: Path, job: tuple) -> Path:
    tree, set_name, compiler, opt = job[:4]
    set_dir, flags = source_flags(tree, set_name)
    binary = work / f"ct_{tree}_{set_name}_{compiler}_{opt.lstrip('-')}"
    # nistkat/rng.c is replaced by the driver's randombytes(); nothing else changes.
    cmd = [tools[compiler], "-g", opt, *flags, "-no-pie", *sorted(set_dir.glob("*.c")),
           work / "taint_driver.c", "-lcrypto", "-o", binary]
    done = run(cmd, work, 600)
    if done.returncode:
        raise RuntimeError(f"build failed ({binary.name}): {done.stderr[-800:]}")
    return binary


def memcheck(tools: dict, work: Path, binary: Path, mode: str) -> list:
    report = work / f"{binary.name}.{mode}.xml"
    cmd = [tools["valgrind"], "--tool=memcheck", "--error-limit=no", "--leak-check=no",
           "--num-callers=30", "--xml=yes", f"--xml-file={report}", binary, mode, str(SIGNATURES)]
    done = run(cmd, work, 480)
    expected = f"ok mode={mode} signatures={SIGNATURES} valgrind=1"
    if done.returncode or done.stdout.strip() != expected:
        raise RuntimeError(f"taint run failed ({binary.name} {mode}): rc={done.returncode} "
                           f"stdout={done.stdout[-200:]!r} stderr={done.stderr[-400:]!r}")
    text = report.read_text(encoding="utf-8", errors="replace")
    counts_block = re.search(r"<errorcounts>(.*?)</errorcounts>", text, re.S)
    if not counts_block:
        raise RuntimeError(f"no <errorcounts> in the Memcheck report of {binary.name} {mode}")
    counts = {pair.findtext("unique"): int(pair.findtext("count"))
              for pair in ET.fromstring("<x>" + counts_block.group(1) + "</x>").iter("pair")}
    errors = []
    # Error by error: the XML document as a whole is not always well formed.
    for block in re.findall(r"<error>.*?</error>", text, re.S):
        element = ET.fromstring(block)
        frames = [{"ip": frame.findtext("ip"),
                   "fn": (frame.findtext("fn") or "?").split(".")[0],
                   "file": (frame.findtext("file") or "?").rsplit("/", 1)[-1],
                   "line": int(frame.findtext("line") or 0)}
                  for frame in element.find("stack").iter("frame")]
        errors.append({"kind": element.findtext("kind"), "frames": frames,
                       "count": counts.get(element.findtext("unique"), 0)})
    if len(errors) != len(counts):
        raise RuntimeError(f"inconsistent Memcheck report of {binary.name} {mode}")
    return errors


def tally(table: dict, key: str, count: int) -> None:
    entry = table.setdefault(key, {"contexts": 0, "reports": 0})
    entry["contexts"] += 1
    entry["reports"] += count


def classify(errors: list, tree: str, set_name: str, listing: list | None) -> dict:
    """Sort every Memcheck report by its innermost frame and its ov_sign_core frame."""
    retry_line = RETRY_LINE[set_name]
    by_site: dict = {}
    by_signing_line: dict = {}
    secret = {"contexts": 0, "reports": 0}
    released = {"contexts": 0, "reports": 0}
    retry: dict = {}
    other: dict = {}
    addresses = set()
    for error in errors:
        frames = error["frames"]
        inner = frames[0] if frames else {"ip": None, "fn": "?", "file": "?", "line": 0}
        signing = next((frame for frame in frames if frame["fn"] == "ov_sign_core"), None)
        conditional = error["kind"] == "UninitCondition"
        if conditional and inner["fn"] == "ov_sign_core" and inner["line"] == retry_line:
            key = f"ov.c:{retry_line}"
            retry[key] = retry.get(key, 0) + error["count"]
        elif (conditional and signing is not None and inner["file"] in TREES[tree]["mul_files"]
              and MUL_FRAME.fullmatch(inner["fn"])):
            tally(by_site, f"{inner['file']}:{inner['line']}", error["count"])
            tally(by_signing_line, f"ov.c:{signing['line']}", error["count"])
            group = (secret if signing["line"] in SECRET_LINES[set_name]
                     else released if signing["line"] in RELEASED_LINES[set_name] else None)
            if group is not None:
                group["contexts"] += 1
                group["reports"] += error["count"]
            addresses.add(int(inner["ip"], 16))
        else:
            key = f"{error['kind']} {inner['fn']} {inner['file']}:{inner['line']}"
            other[key] = other.get(key, 0) + error["count"]
    multiplication = {
        "contexts": sum(entry["contexts"] for entry in by_site.values()),
        "reports": sum(entry["reports"] for entry in by_site.values()),
        "secret_multiplier": secret,
        "released_multiplier": released,
        "by_signing_line": dict(sorted(by_signing_line.items())),
        "by_site": dict(sorted(by_site.items())),
    }
    if listing is not None and addresses:
        index = {item["address"]: position for position, item in enumerate(listing)}
        jumps, after_test, adjacent_tests, masks = 0, 0, 0, set()
        for address in addresses:
            position = index.get(address)
            if position is None or not is_conditional_jump(listing[position]):
                continue
            jumps += 1
            mask, adjacent = tested_bit(listing, position)
            if mask is not None:
                after_test += 1
                adjacent_tests += adjacent
                masks.add(mask)
        multiplication["instructions"] = {
            "addresses": len(addresses), "conditional_jumps": jumps,
            "after_single_bit_test": after_test,
            "directly_after_single_bit_test": adjacent_tests,
            "bit_masks": [f"0x{mask:02x}" for mask in sorted(masks)],
        }
    return {"contexts": len(errors), "reports": sum(error["count"] for error in errors),
            "field_multiplication": multiplication, "retry_branch": retry,
            "other": dict(sorted(other.items()))}


def taint_job(tools: dict, work: Path, job: tuple) -> dict:
    tree, set_name, compiler, opt, modes = job
    binary = build(tools, work, job)
    listing = disassemble(tools, work, binary, False) if compiler == "clang" else None
    return {f"{tree}/{set_name}/{compiler}{opt}/{mode}":
            classify(memcheck(tools, work, binary, mode), tree, set_name, listing) for mode in modes}


def measure(tools: dict, work: Path, with_avx2: bool) -> tuple[dict, dict]:
    (work / "taint_driver.c").write_text(DRIVER_C, encoding="utf-8")
    plan = TAINT_PLAN + ([AVX2_JOB] if with_avx2 else [])
    with ThreadPoolExecutor(max_workers=JOBS) as pool:
        taint = [pool.submit(taint_job, tools, work, job) for job in plan]
        scans = [pool.submit(object_scan, tools, work, job) for job in OBJECT_PLAN]
        runs: dict = {}
        for future in taint:
            runs.update(future.result())
        objects = {f"{tree}/{set_name}/{compiler}{opt}": future.result()
                   for (tree, set_name, compiler, opt), future in zip(OBJECT_PLAN, scans)}
    return objects, runs


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def render(value, depth: int = 0) -> str:
    """JSON text: the top four levels one entry per line, deeper values inline."""
    if isinstance(value, dict) and value and depth < 4:
        pad = "  " * (depth + 1)
        rows = [f"{pad}{json.dumps(key)}: {render(item, depth + 1)}" for key, item in value.items()]
        return "{\n" + ",\n".join(rows) + "\n" + "  " * depth + "}"
    return json.dumps(value)


def emit(result: dict, output: Path | None) -> int:
    result["status"] = result.pop("status")   # keep the status as the last key
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
    checks, pinned, facts, problems = source_checks()
    result["checks"].update(checks)
    result["source_sha256"] = pinned
    result["measurements"].update(facts)
    if problems:
        result["problems"] = problems
    if not all(checks.values()):
        return emit(result, args.output)

    with tempfile.TemporaryDirectory(prefix="uov-se1-") as temporary:
        work = Path(temporary)
        try:
            tools = find_tools(work)
        except Skip as reason:
            result["status"] = "SKIP"
            result["skip_reason"] = str(reason)
            return emit(result, args.output)
        with_avx2 = host_has_avx2_aes()
        if not with_avx2:
            result["evidence"] = EVIDENCE_WITHOUT_AVX2
        tool_facts = {
            "clang": first_line([tools["clang"], "--version"], work),
            "clang_target": first_line([tools["clang"], "-dumpmachine"], work),
            "gcc": first_line([tools["gcc"], "--version"], work),
            "valgrind": first_line([tools["valgrind"], "--version"], work),
            "objdump": first_line([tools["objdump"], "--version"], work),
            "libcrypto": tools["libcrypto"],
            "machine": platform.machine(),
        }
        try:
            objects, runs = measure(tools, work, with_avx2)
        except (RuntimeError, subprocess.TimeoutExpired, OSError, ET.ParseError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    summary = result["measurements"]
    summary["tools"] = tool_facts
    summary["build"] = ("<cc> -g <opt> -std=c99 -fno-omit-frame-pointer [-mavx2 -maes] -D_NIST_KAT_ "
                        "-I<set> -I<nistkat> -no-pie <set>/*.c taint_driver.c (embedded; replaces "
                        "nistkat/rng.c) -lcrypto; _VALGRIND_ is not defined")
    summary["memcheck"] = ("valgrind --tool=memcheck --error-limit=no --leak-check=no "
                           "--num-callers=30 --xml=yes <binary> <mode> <signatures>")
    summary["signatures_per_run"] = SIGNATURES
    summary["taint_modes"] = TAINT
    summary["signing_frames"] = {
        "secret_multiplier": "ov.c:138, ov.c:141 (vinegar), ov.c:146 / ov.c:156 (elimination)",
        "released_multiplier": "ov.c:153 / ov.c:163, ov.c:182 (oil solution, part of the signature)",
        "retry_branch": "ov.c:150 / ov.c:160 (UOV-1)",
    }
    summary["avx2_tree_control"] = (
        "run" if with_avx2 else "not run: the host CPU does not report avx2 and aes")
    summary["object_code_blas_matrix_ref_c"] = objects
    summary["taint_runs"] = runs
    summary["review_values"] = {
        "tools": "clang 21.1.8, gcc 15.2.0, Valgrind 3.26.0, x86-64",
        "single_bit_test_branch_pairs_reference_Ip_clang_O2_O3": "30-31",
        "single_bit_test_branch_pairs_gcc_O2_O3_and_clang_O1": 0,
        "contexts_reference_Ip_sign_clang_O3": 85,
        "sites_gf256v_mul_u32": "gf16.h:255,260,265,270,275,280,285",
        "sites_gf16v_mul_u32": "gf16.h:187,192,197",
        "inlined_into": "blas_u32.h:122,188,254,257,325,365",
    }
    c = result["checks"]

    def pairs(key: str) -> int:
        return objects[key]["pairs_in_field_multiplication"]

    def multiplication(key: str) -> dict:
        return runs[key]["field_multiplication"]

    def instructions(key: str) -> dict:
        return multiplication(key).get("instructions", {})

    def branches_on_secret(key: str) -> bool:
        """Tainted jumps in the multiplication, reached from every secret-operand line."""
        lines = SECRET_LINES[key.split("/")[1]]
        found = multiplication(key)["by_signing_line"]
        return all(found.get(f"ov.c:{line}", {}).get("contexts", 0) > 0 for line in lines)

    def only_retry(key: str) -> bool:
        entry = runs[key]
        return (entry["field_multiplication"]["contexts"] == 0 and not entry["other"]
                and entry["reports"] == sum(entry["retry_branch"].values()) >= SIGNATURES)

    def attempts(key: str) -> int:
        return sum(runs[key]["retry_branch"].values())

    # Object code of the reference blas_matrix_ref.c.
    c["object_clang_O2_O3_jump_on_single_multiplier_bits_gf256v_mul_u32_Ip"] = all(
        pairs(f"ref/Ip/clang{opt}") > 0 for opt in ("-O2", "-O3"))
    c["object_clang_O2_O3_jump_on_single_multiplier_bits_gf16v_mul_u32_Is"] = all(
        pairs(f"ref/Is/clang{opt}") > 0 for opt in ("-O2", "-O3"))
    c["control_object_gcc_O2_O3_and_clang_O1_have_no_such_jump"] = all(
        pairs(f"ref/{name}/{build_name}") == 0
        for name in ("Ip", "Is") for build_name in ("gcc-O2", "gcc-O3", "clang-O1"))
    c["control_object_gcc_O2_O3_and_clang_O1_have_no_single_bit_pair_at_all_Ip"] = all(
        objects[f"ref/Ip/{build_name}"]["pairs"] == 0 for build_name in ("gcc-O2", "gcc-O3", "clang-O1"))

    # Taint tracking, stated behaviour.
    c["taint_clang_O3_jumps_in_field_multiplication_on_secret_multiplier_Ip"] = (
        branches_on_secret("ref/Ip/clang-O3/sign"))
    c["taint_clang_O2_jumps_in_field_multiplication_on_secret_multiplier_Ip"] = (
        branches_on_secret("ref/Ip/clang-O2/sign"))
    c["taint_clang_O3_jumps_remain_with_only_the_secret_seed_undefined_Ip"] = (
        branches_on_secret("ref/Ip/clang-O3/seed"))
    c["taint_clang_O3_jumps_in_gf16v_mul_u32_on_secret_multiplier_Is"] = (
        branches_on_secret("ref/Is/clang-O3/sign"))
    c["taint_clang_O3_jumps_in_u64_forms_on_secret_multiplier_amd64_Ip"] = (
        branches_on_secret("amd64/Ip/clang-O3/sign")
        and any(site.startswith("gf16_u64.h:") for site in multiplication("amd64/Ip/clang-O3/sign")["by_site"]))
    affected = ("ref/Ip/clang-O3/sign", "ref/Ip/clang-O3/seed", "ref/Ip/clang-O2/sign",
                "ref/Is/clang-O3/sign", "amd64/Ip/clang-O3/sign")
    c["taint_reported_addresses_are_conditional_jump_instructions"] = all(
        instructions(key).get("conditional_jumps", 0) == instructions(key).get("addresses", -1) > 0
        for key in affected)
    c["taint_reported_jumps_use_the_flags_of_single_bit_tests"] = all(
        instructions(key).get("after_single_bit_test", 0) == instructions(key).get("addresses", -1) > 0
        for key in affected)
    c["taint_clang_builds_report_the_retry_branch_and_nothing_else"] = all(
        attempts(key) >= SIGNATURES and not runs[key]["other"] for key in affected)

    # Controls.
    c["control_gcc_O2_reports_only_the_retry_branch"] = all(only_retry(key) for key in (
        "ref/Ip/gcc-O2/sign", "ref/Ip/gcc-O2/seed", "ref/Is/gcc-O2/sign", "amd64/Ip/gcc-O2/sign"))
    c["control_gcc_O3_and_clang_O1_report_only_the_retry_branch"] = all(only_retry(key) for key in (
        "ref/Ip/gcc-O3/sign", "ref/Ip/clang-O1/sign"))
    if with_avx2:
        c["control_avx2_tree_clang_O3_reports_only_the_retry_branch"] = only_retry("avx2/Ip/clang-O3/sign")
    c["control_untainted_clang_O3_run_has_no_reports"] = runs["ref/Ip/clang-O3/clean"]["reports"] == 0
    c["control_gcc_and_clang_made_the_same_number_of_attempts"] = (
        attempts("ref/Ip/gcc-O2/sign") == attempts("ref/Ip/clang-O3/sign")
        and attempts("ref/Ip/gcc-O2/seed") == attempts("ref/Ip/clang-O3/seed")
        and attempts("ref/Is/gcc-O2/sign") == attempts("ref/Is/clang-O3/sign")
        and attempts("amd64/Ip/gcc-O2/sign") == attempts("amd64/Ip/clang-O3/sign"))

    result["status"] = "PASS" if all(c.values()) and not problems else "FAIL"
    # The run time goes to stderr so that the JSON is identical on every run.
    print(f"elapsed {time.monotonic() - started:.1f} s", file=sys.stderr)
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
