#!/usr/bin/env python3
"""Reproduce QR-UOV-5: optimized key generation leaves S^T*P1 on the stack.

`Optimized_Implementation/opt/qruov.c` wipes only `sdt` and `sdt_eval` at the
end of `qruov_keygen` (lines 184-185).  The four secret-derived arrays that
are declared at lines 149-152 stay on the stack.  The reference and AVX2
trees wipe their corresponding arrays.

The script builds the submitted 1q127L3 SHAKE sources with a short driver.
The driver runs `crypto_sign_keypair` on a stack that it owns (`makecontext`).
After the call has returned no frame on that stack is live, and the driver
reads it as ordinary memory.  This does not depend on the frame layout that
a compiler chooses.  The driver recomputes `sdt_pi1_eval` of the last
equation from the secret key and searches the dead stack for it.

A pure-Python step then takes the bytes that were read from the stack,
interpolates them, and solves P1 * S = X^T over F_(127^3).  P1 is expanded
from the public seed.

Controls: a later call that reuses the stack removes the match; the
reference build shows no match for its `sdt_pi1`; the optimized source with
the four wipes added shows no match.

Limits: the residue is located with knowledge of the secret key.  Locating
it in a memory dump from public data alone is not shown.  No memory
disclosure primitive is demonstrated.  Only 1q127L3 with the SHAKE expander
is built; the AVX2 tree is checked in source only.

Needs gcc, glibc and the OpenSSL development files.  Prints one JSON object.
Exit status: 0 PASS, 1 FAIL, 77 SKIP (a tool is missing).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
BASE = HERE / "submission/QR_UOV - Round3"
OPT = BASE / "Optimized_Implementation/opt"
AVX2 = BASE / "Optimized_Implementation/avx2"
REF = BASE / "Reference_Implementation/ref"

CANDIDATE = "QR-UOV-5"
EVIDENCE = (
    "Shows that optimized key generation (opt tree, 1q127L3 SHAKE) returns "
    "with sdt_pi1_eval (S^T*P1 of the last equation) intact in its dead "
    "stack and that those bytes determine the whole secret matrix S; "
    "reading them needs an in-process memory-disclosure primitive, which "
    "is not shown, and the residue was located by recomputing it from the "
    "known secret key, not from public data alone."
)

# Files the claim rests on.  Paths are relative to PREFIX.
PREFIX = "submission/QR_UOV - Round3/"
CLAIM_SHA256 = {
    "Optimized_Implementation/opt/qruov.c":
        "c7ddf267a4eec3f75d426b4d1a3d61160dc555fde87c9c08fad2de1100943ef4",
    "Reference_Implementation/ref/qruov.c":
        "7d64ccbc16512a99c60d4f8358a862b3330f3cdb89e9cb81e9ade798f65659ec",
    "Optimized_Implementation/avx2/qruov.c":
        "42efd085c468581f691bea10788b9dae78c559c0c3321d48e1fbcf1260a96a9d",
    "Optimized_Implementation/opt/util.h":
        "377144c22c0e26db2895c948f494b3f3e8df94cfebc6339dbba172e36de548e9",
    "Reference_Implementation/ref/util.h":
        "377144c22c0e26db2895c948f494b3f3e8df94cfebc6339dbba172e36de548e9",
    "Optimized_Implementation/avx2/util.h":
        "377144c22c0e26db2895c948f494b3f3e8df94cfebc6339dbba172e36de548e9",
}

# Every file that the two builds compile or include.  Each tree is pinned by
# one digest over the lines "<sha256>  <name>\n" in sorted name order.
COMMON_C = ["sign.c", "qruov_common.c", "qrop.c", "rng.c", "prg.c", "prim.c",
            "linsys.c", "qruov.c"]
COMMON_H = ["api.h", "fastop.h", "fql.h", "gf.h", "linsys.h", "prg.h",
            "prim.h", "qrop.h", "qruov.h", "qruov_common.h", "qruov_param.h",
            "qruov_tau.h", "rng.h", "util.h"]
OPT_C = COMMON_C + ["emi_transform.c", "emi_transform_gen.c"]
OPT_H = COMMON_H + ["emi_transform.h"]
TREE_SHA256 = {
    "Optimized_Implementation/opt (25 build inputs, combined)":
        "e8384f49c28977ee5cb0b95dab1a4e75bf16955e6e91c8cb5ea9571ba4fc8143",
    "Reference_Implementation/ref (22 build inputs, combined)":
        "a3e59668e6486681eb8a84c1d97b2a681c7d3352beacc6eb0d93e85cb7fcc949",
}

# Parameter set 1q127L3 (qruov_param.h, qruov_tau.h, emi_transform.h).
Q, L, V, M, SMALL_M, LL = 127, 3, 52, 18, 54, 5
TAU_N1, TAU_N2 = 4267, 2916
N1, N2 = L * V * (V + 1) // 2, L * V * M

# PRG_IS_AES and QRUOV_PARAM_* are set as the submitted GNUmakefile sets them
# for PARAM=1q127L3 PRG=shake.  _DEFAULT_SOURCE only makes libc declare
# explicit_bzero.  Both optimization levels are fixed here and both must show
# the stated behaviour; no other flag is tried.
DEFINES = ["-D_DEFAULT_SOURCE", "-DPRG_IS_AES=0", "-DQRUOV_PARAM_1q127L3"]
STACK_BYTES = 4 << 20    # stack region that the driver owns
SCRUB_BYTES = 1 << 20    # part of it that the scrub control overwrites
LEVELS = {
    "-O3": "optimization level of the default CFLAGS in the submitted GNUmakefile",
    "-O2": "optimization level of the review's build script",
}

# Added to a temporary copy of opt/qruov.c for the control build only.
WIPE_ANCHOR = (
    "            chunk_eqs = 0;\n"
    "        }\n"
    "    }\n"
    "    if (chunk_eqs > 0) {\n"
)
WIPE_PATCHED = (
    "            chunk_eqs = 0;\n"
    "        }\n"
    "        secure_zero(sdt_pi1_eval, sizeof(sdt_pi1_eval));\n"
    "        secure_zero(sdt_pi1_reval, sizeof(sdt_pi1_reval));\n"
    "        secure_zero(sdt_pi2_eval, sizeof(sdt_pi2_eval));\n"
    "        secure_zero(pi2t_sd_eval, sizeof(pi2t_sd_eval));\n"
    "    }\n"
    "    if (chunk_eqs > 0) {\n"
)

PROBE_C = r"""
#define _GNU_SOURCE
#include <string.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <openssl/evp.h>
int main(void) { return EVP_shake256() == NULL || memmem("ab", 2, "b", 1) == NULL; }
"""

DRIVER_C = r"""
/* Dead-stack residue check for QR-UOV key generation (1q127L3, SHAKE).
 * crypto_sign_keypair runs on a stack owned by this program (makecontext).
 * After it has returned and the context has ended, no frame on that stack
 * is live.  The whole region is then dead stack and is read as ordinary
 * memory.  Build with -DTREE_OPT or -DTREE_REF.  usage: driver plain|scrub */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>
#include "api.h"
#include "rng.h"
#include "qruov.h"
#include "qrop.h"
#include "prg.h"
#include "fql.h"
#ifdef TREE_OPT
#include "emi_transform.h"
#endif

/* STACK_BYTES and SCRUB_BYTES come from the command line. */
#define FILL 0xA5 /* not a field element: q = 127 */
#define CHUNK 16

enum { L = QRUOV_L, V = QRUOV_V, M = QRUOV_M, m = QRUOV_m, Q = QRUOV_q };

static uint8_t *stk;
static ucontext_t main_ctx, key_ctx;
static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
static int keypair_rc = -1, do_scrub;

/* A later call that reuses the dead stack.  It fills SCRUB_BYTES of it. */
__attribute__((noinline)) static void scrub(void)
{
    volatile uint8_t buf[SCRUB_BYTES];
    for (size_t i = 0; i < SCRUB_BYTES; i++) buf[i] = (uint8_t)(128 + (i * 13 + 7) % 128);
}

static void on_stack(void)
{
    keypair_rc = crypto_sign_keypair(pk, sk);
    if (do_scrub) scrub();
}

/* Best alignment of nd[] inside hay[].  Every disjoint CHUNK-byte piece of
 * nd is searched everywhere.  For each hit, the bytes that are equal at
 * that alignment are counted.  Returns the best count; 0 means that no
 * piece of nd occurs anywhere in hay. */
static size_t best_match(const uint8_t *hay, size_t hn, const uint8_t *nd, size_t nn, long *where)
{
    size_t best = 0;
    long best_base = -1, last_base = -2;
    for (size_t p = 0; p + CHUNK <= nn; p += CHUNK) {
        const uint8_t *h = hay;
        size_t left = hn;
        while (left >= CHUNK) {
            const uint8_t *hit = memmem(h, left, nd + p, CHUNK);
            if (!hit) break;
            long base = (long)(hit - hay) - (long)p;
            if (base != last_base && base != best_base) {
                size_t cnt = 0;
                for (size_t i = 0; i < nn; i++) {
                    long a = base + (long)i;
                    if (a >= 0 && (size_t)a < hn && hay[a] == nd[i]) cnt++;
                }
                if (cnt > best) { best = cnt; best_base = base; }
                last_base = base;
            }
            left -= (size_t)(hit - h) + 1;
            h = hit + 1;
        }
    }
    *where = best_base;
    return best;
}

static void hex(const char *name, const uint8_t *p, size_t n)
{
    printf("%s=", name);
    for (size_t i = 0; i < n; i++) printf("%02x", p[i]);
    printf("\n");
}

static const uint8_t *hay;
static size_t hay_len;

/* depth: distance of the array start below the top of the stack region. */
static long report(const char *name, const uint8_t *nd, size_t nn)
{
    long base;
    size_t cnt = best_match(hay, hay_len, nd, nn, &base);
    printf("match.%s=%zu/%zu depth=%ld\n", name, cnt, nn, cnt ? (long)hay_len - base : -1L);
    return (cnt && base >= 0 && (size_t)base + nn <= hay_len) ? base : -1;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    do_scrub = !strcmp(argv[1], "scrub");
    unsigned char seed[48] = {1, 2, 3}; /* fixed DRBG seed, as in the review */
    randombytes_init(seed, NULL, 256);

    stk = mmap(NULL, STACK_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stk == MAP_FAILED) return 3;
    memset(stk, FILL, STACK_BYTES);
    if (getcontext(&key_ctx)) return 3;
    key_ctx.uc_stack.ss_sp = stk;
    key_ctx.uc_stack.ss_size = STACK_BYTES;
    key_ctx.uc_link = &main_ctx;
    makecontext(&key_ctx, on_stack, 0);
    if (swapcontext(&main_ctx, &key_ctx)) return 3;
    if (keypair_rc) return 4;

    /* The key-generation stack is dead from here on.  main has its own. */
    size_t low = 0;
    while (low < STACK_BYTES && stk[low] == FILL) low++;
    hay = stk + low;
    hay_len = STACK_BYTES - low;
    printf("params=%d,%d,%d,%d,%d,%d,%d\n", Q, L, V, M, m, QRUOV_tau_n1, QRUOV_tau_n2);
    printf("stack_used=%zu\n", hay_len);
    hex("pk_seed", pk, SEED_PK_LEN);
    hex("sk", sk, SEED_SK_LEN);

    /* Expected arrays, recomputed from the secret key with submitted functions. */
    static uint8_t sd[QRUOV_tau_n2], sdt[M * V * L], pi1[V * V * L], pi2t[M * V * L];
    prg g;
    prepare_sd_from_seed(sd, sk);
    prepare_sdt_from_sd(sdt, sd);
    prg_init(&g, pk);
    prepare_pi1_from_prg(pi1, &g, (uint16_t)(m - 1));
    prepare_pi2t_from_prg(pi2t, &g, (uint16_t)(m - 1));
    prg_free(&g);
    hex("sd", sd, QRUOV_n2);
    report("sdt", sdt, sizeof sdt);
    report("pi1_public", pi1, sizeof pi1);
#ifdef TREE_OPT
    enum { LL = QRUOV_LL };
    static uint8_t soa[L * V * V], sdt_eval[LL * M * V], pi1_eval[LL * V * V], pi2t_eval[LL * M * V];
    static uint8_t sdt_pi1_eval[LL * M * V], sdt_pi1_reval[LL * M * V];
    static uint8_t sdt_pi2_eval[LL * M * M], pi2t_sd_eval[LL * M * M];
    for (int i = 0; i < M * V; i++) for (int l = 0; l < L; l++) soa[l * M * V + i] = sdt[i * L + l];
    evaluate(sdt_eval, M * V, soa, M * V, M * V);
    for (int i = 0; i < M * V; i++) for (int l = 0; l < L; l++) soa[l * M * V + i] = pi2t[i * L + l];
    evaluate(pi2t_eval, M * V, soa, M * V, M * V);
    for (int i = 0; i < V * V; i++) for (int l = 0; l < L; l++) soa[l * V * V + i] = pi1[i * L + l];
    evaluate(pi1_eval, V * V, soa, V * V, V * V);
    for (int p = 0; p < LL; p++) for (int i = 0; i < M; i++) {
        for (int j = 0; j < V; j++) {
            uint32_t s = 0;
            for (int k = 0; k < V; k++) s += (uint32_t)sdt_eval[p * M * V + i * V + k] * pi1_eval[p * V * V + j * V + k];
            sdt_pi1_eval[p * M * V + i * V + j] = (uint8_t)(s % Q);
        }
        for (int j = 0; j < M; j++) {
            uint32_t s = 0;
            for (int k = 0; k < V; k++) s += (uint32_t)sdt_eval[p * M * V + i * V + k] * pi2t_eval[p * M * V + j * V + k];
            sdt_pi2_eval[p * M * M + i * M + j] = (uint8_t)(s % Q);
            pi2t_sd_eval[p * M * M + j * M + i] = (uint8_t)(s % Q);
        }
    }
    reevaluate(sdt_pi1_reval, M * V, sdt_pi1_eval, M * V, M * V);
    report("sdt_eval", sdt_eval, sizeof sdt_eval);
    long at = report("sdt_pi1_eval", sdt_pi1_eval, sizeof sdt_pi1_eval);
    report("sdt_pi1_reval", sdt_pi1_reval, sizeof sdt_pi1_reval);
    report("sdt_pi2_eval", sdt_pi2_eval, sizeof sdt_pi2_eval);
    report("pi2t_sd_eval", pi2t_sd_eval, sizeof pi2t_sd_eval);
    /* The residue: bytes read from the dead stack, not the recomputed copy. */
    if (at >= 0) hex("residue", hay + at, sizeof sdt_pi1_eval);
#else
    static uint8_t sdt_pi1[M * V * L], sdt_pi2[M * M * L];
    fql_mul_MxV_VxVt(sdt_pi1, sdt, pi1);
    fql_mul_MxV_MxVt(sdt_pi2, sdt, pi2t);
    report("sdt_pi1", sdt_pi1, sizeof sdt_pi1);
    report("sdt_pi2", sdt_pi2, sizeof sdt_pi2);
    hex("expected_sdt_pi1", sdt_pi1, sizeof sdt_pi1);
#endif
    uint8_t pat[64];
    for (size_t i = 0; i < sizeof pat; i++) pat[i] = (uint8_t)(128 + (i * 13 + 7) % 128);
    printf("scrub_pattern=%d\n", memmem(hay, hay_len, pat, sizeof pat) != NULL);
    return 0;
}
"""


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree_digest(tree: Path, names: list[str]) -> str:
    digest = hashlib.sha256()
    for name in sorted(names):
        digest.update(f"{sha(tree / name)}  {name}\n".encode())
    return digest.hexdigest()


def source_hashes() -> dict[str, str]:
    found = {}
    for name in CLAIM_SHA256:
        path = BASE / name
        found[PREFIX + name] = sha(path) if path.is_file() else "missing"
    for key, tree, names in (
        (list(TREE_SHA256)[0], OPT, OPT_C + OPT_H),
        (list(TREE_SHA256)[1], REF, COMMON_C + COMMON_H),
    ):
        try:
            found[PREFIX + key] = tree_digest(tree, names)
        except OSError:
            found[PREFIX + key] = "missing"
    return found


def lines_are(text: str, first: int, expected: list[str]) -> bool:
    lines = text.splitlines()
    return [line.strip() for line in lines[first - 1:first - 1 + len(expected)]] == expected


def source_checks() -> dict[str, bool]:
    opt = (OPT / "qruov.c").read_text(encoding="ascii")
    ref = (REF / "qruov.c").read_text(encoding="ascii")
    avx2 = (AVX2 / "qruov.c").read_text(encoding="ascii")
    util = (OPT / "util.h").read_text(encoding="ascii")
    unwiped = ["sdt_pi1_eval", "sdt_pi1_reval", "sdt_pi2_eval", "pi2t_sd_eval"]
    # Body of qruov_keygen in the opt tree (lines 133-186).
    keygen = opt.partition("void qruov_keygen(")[2].partition("int qruov_sign(")[0]
    return {
        "opt_declares_four_secret_derived_arrays_at_149_152": lines_are(opt, 149, [
            "uint8_t sdt_pi1_eval[QRUOV_LL * QRUOV_M * QRUOV_V];",
            "uint8_t sdt_pi1_reval[QRUOV_LL * QRUOV_M * QRUOV_V];",
            "uint8_t sdt_pi2_eval[QRUOV_LL * QRUOV_M * QRUOV_M];",
            "uint8_t pi2t_sd_eval[QRUOV_LL * QRUOV_M * QRUOV_M];",
        ]),
        "opt_wipes_only_sdt_and_sdt_eval_at_184_185": (
            lines_are(opt, 184, [
                "secure_zero(sdt, sizeof(sdt));",
                "secure_zero(sdt_eval, sizeof(sdt_eval));",
            ])
            and opt.count("secure_zero(") == 2
            and all(f"uint8_t {name}[" in keygen for name in unwiped)
            and not any(word in keygen for word in ("memset", "bzero", "cleanse"))
        ),
        "ref_wipes_sdt_sdt_pi1_sdt_pi2_at_74_76": lines_are(ref, 74, [
            "secure_zero(sdt, sizeof(sdt));",
            "secure_zero(sdt_pi1, sizeof(sdt_pi1));",
            "secure_zero(sdt_pi2, sizeof(sdt_pi2));",
        ]),
        "avx2_wipes_corresponding_arrays_at_212_218": lines_are(avx2, 212, [
            "secure_zero(sdt_p2_eval, sizeof(sdt_p2_eval));",
            "secure_zero(p2t_sd_eval, sizeof(p2t_sd_eval));",
            "secure_zero(sdt_p1_reval, sizeof(sdt_p1_reval));",
            "secure_zero(sdt_p1_eval, sizeof(sdt_p1_eval));",
            "secure_zero(sdt_eval, sizeof(sdt_eval));",
            "secure_zero(sdvnni, sizeof(sdvnni));",
            "secure_zero(sdbuf, sizeof(sdbuf));",
        ]),
        "secure_zero_is_explicit_bzero_on_glibc": "explicit_bzero(ptr, len);" in util,
    }


# ---- attacker-side model: pure Python, public seed plus the residue bytes ----

def rejection_sample(raw: bytes, length: int) -> list[int]:
    """fastop_rejection_sample of the submitted fastop.h."""
    value = [byte & Q for byte in raw]
    tau = len(value)
    aux = length
    while aux < tau and value[aux] == Q:
        aux += 1
    for i in range(length):
        if value[i] == Q:
            if aux < tau:
                value[i] = value[aux]
                aux += 1
                while aux < tau and value[aux] == Q:
                    aux += 1
            else:
                value[i] = 0
    return value[:length]


ZERO = (0, 0, 0)


def fmul(a, b):
    """F_(127^3) = F_127[x]/(x^3 - x - 1), as fql_mul with (fc, fe, fc0) = (1, 1, 1)."""
    acc = [0] * 5
    for i in range(3):
        for j in range(3):
            acc[i + j] += a[i] * b[j]
    for t in (4, 3):
        acc[t - 3] += acc[t]
        acc[t - 2] += acc[t]
    return (acc[0] % Q, acc[1] % Q, acc[2] % Q)


def fsub(a, b):
    return ((a[0] - b[0]) % Q, (a[1] - b[1]) % Q, (a[2] - b[2]) % Q)


def finv(a):
    result, base, exponent = (1, 0, 0), a, Q**3 - 2
    while exponent:
        if exponent & 1:
            result = fmul(result, base)
        base = fmul(base, base)
        exponent >>= 1
    return result


def public_p1(seed_pk: bytes, equation: int):
    """Symmetric V x V matrix P_(equation,1), expanded from the public seed."""
    raw = hashlib.shake_128(seed_pk + (2 * equation).to_bytes(2, "big")).digest(TAU_N1)
    coeffs = rejection_sample(raw, N1)
    matrix = [[ZERO] * V for _ in range(V)]
    k = 0
    for a in range(V):
        for b in range(a, V):
            matrix[a][b] = matrix[b][a] = tuple(coeffs[k:k + L])
            k += L
    return matrix


def interpolate(residue: bytes):
    """Evaluation domain [LL][M][V] -> M x V matrix over F_(127^3).

    The five planes are the values of the unreduced degree-4 product at
    0, 1, -1, 2 and infinity (evaluate_specialized_L3 in emi_transform.c).
    """
    inv2, inv6 = pow(2, -1, Q), pow(6, -1, Q)
    plane = M * V
    matrix = []
    for i in range(M):
        row = []
        for j in range(V):
            e0, e1, e2, e3, e4 = (residue[p * plane + i * V + j] for p in range(LL))
            c0, c4 = e0, e4
            c2 = ((e1 + e2) * inv2 - c0 - c4) % Q
            odd = (e1 - e2) * inv2 % Q                   # c1 + c3
            c3 = (e3 - c0 - 4 * c2 - 16 * c4 - 2 * odd) * inv6 % Q
            c1 = (odd - c3) % Q
            # x^3 = x + 1 and x^4 = x^2 + x
            row.append(((c0 + c3) % Q, (c1 + c3 + c4) % Q, (c2 + c4) % Q))
        matrix.append(row)
    return matrix


def solve_for_s(p1, x):
    """Solve P1 * S = X^T (V x V, M right-hand sides) over F_(127^3)."""
    work = [p1[r][:] + [x[c][r] for c in range(M)] for r in range(V)]
    for c in range(V):
        pivot = next((r for r in range(c, V) if work[r][c] != ZERO), None)
        if pivot is None:
            return None
        work[c], work[pivot] = work[pivot], work[c]
        inverse = finv(work[c][c])
        pivot_row = work[c] = [fmul(inverse, value) for value in work[c]]
        for r in range(V):
            factor = work[r][c]
            if r != c and factor != ZERO:
                row = work[r]
                for j in range(c, V + M):
                    row[j] = fsub(row[j], fmul(factor, pivot_row[j]))
    return [[work[r][V + c] for c in range(M)] for r in range(V)]


def as_matrix_s(sd: bytes):
    """Row-major V x M matrix S from the sd buffer of prepare_sd_from_seed."""
    return [[tuple(sd[(i * M + j) * L:(i * M + j + 1) * L]) for j in range(M)]
            for i in range(V)]


# ---- builds and runs ----

def compile_driver(gcc: str, level: str, tree: Path, sources: list[Path],
                   define: str, driver: Path, output: Path) -> None:
    command = [gcc, level, *DEFINES, define, f"-DSTACK_BYTES={STACK_BYTES}u",
               f"-DSCRUB_BYTES={SCRUB_BYTES}u", "-I", str(tree),
               *map(str, sources), str(driver), "-lcrypto", "-o", str(output)]
    done = subprocess.run(command, capture_output=True, text=True, timeout=300)
    if done.returncode:
        raise RuntimeError(f"build failed ({output.name}): {done.stderr[-2000:]}")


def run_driver(binary: Path, mode: str) -> dict:
    done = subprocess.run([str(binary), mode], capture_output=True, text=True,
                          timeout=120)
    if done.returncode:
        raise RuntimeError(f"{binary.name} {mode}: exit {done.returncode}: "
                           f"{done.stderr[-500:]}")
    parsed: dict = {"match": {}}
    for line in done.stdout.splitlines():
        key, _, value = line.partition("=")
        if key.startswith("match."):
            fraction, depth = value.split(" depth=")
            matched, size = fraction.split("/")
            parsed["match"][key[6:]] = {
                "matched_bytes": int(matched), "array_bytes": int(size),
                "depth_below_stack_top": int(depth),
            }
        else:
            parsed[key] = value
    return parsed


def full(run: dict, name: str) -> bool:
    entry = run["match"][name]
    return entry["matched_bytes"] == entry["array_bytes"]


def absent(run: dict, name: str) -> bool:
    """No 16-byte piece of the array occurs anywhere in the used stack."""
    return run["match"][name]["matched_bytes"] == 0


def mostly(run: dict, name: str) -> bool:
    """At least 90% of the array is present at one alignment.

    Used only for the public array that shows that the scan works.  A later
    call may overwrite a few bytes at the top of the dead frame.
    """
    entry = run["match"][name]
    return 10 * entry["matched_bytes"] >= 9 * entry["array_bytes"]


def emit(result: dict, output: Path | None) -> int:
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")
    return {"PASS": 0, "SKIP": 77}.get(result["status"], 1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    pinned = {PREFIX + name: digest
              for name, digest in {**CLAIM_SHA256, **TREE_SHA256}.items()}
    result: dict = {
        "candidate": CANDIDATE,
        "evidence": EVIDENCE,
        "checks": {},
        "measurements": {},
        "source_sha256": pinned,
        "status": "FAIL",
    }
    checks, measurements = result["checks"], result["measurements"]

    found = source_hashes()
    checks["source_hashes_match_pins"] = found == pinned
    if found != pinned:
        measurements["source_hash_mismatch"] = sorted(
            name for name in pinned if found[name] != pinned[name])
        return emit(result, args.output)
    checks.update(source_checks())

    gcc = shutil.which("gcc")
    if gcc is None:
        result["status"] = "SKIP"
        measurements["skip_reason"] = "gcc not found"
        return emit(result, args.output)
    version = subprocess.run([gcc, "--version"], capture_output=True, text=True)
    measurements["build"] = {
        "compiler": version.stdout.splitlines()[0] if version.stdout else "unknown",
        "parameter_set": "1q127L3, SHAKE public expander",
        "command": "gcc <level> " + " ".join(DEFINES) + " -DTREE_<OPT|REF> "
                   "-DSTACK_BYTES=.. -DSCRUB_BYTES=.. -I <tree> <tree sources> "
                   "driver.c -lcrypto",
        "control_patch": "four secure_zero calls for the arrays of lines "
                         "149-152, added at the end of the equation loop in "
                         "a temporary copy of opt/qruov.c",
        "levels": LEVELS,
        "opt_sources": " ".join(OPT_C),
        "ref_sources": " ".join(COMMON_C),
    }

    with tempfile.TemporaryDirectory(prefix="qruov-se1-") as temporary:
        work = Path(temporary)
        probe = work / "probe.c"
        probe.write_text(PROBE_C)
        probed = subprocess.run(
            [gcc, str(probe), "-lcrypto", "-o", str(work / "probe")],
            capture_output=True, text=True, timeout=120)
        if probed.returncode:
            result["status"] = "SKIP"
            measurements["skip_reason"] = (
                "OpenSSL headers/libcrypto or glibc ucontext not available")
            return emit(result, args.output)

        driver = work / "driver.c"
        driver.write_text(DRIVER_C)
        original = (OPT / "qruov.c").read_text(encoding="ascii")
        checks["wipe_patch_anchor_occurs_once"] = original.count(WIPE_ANCHOR) == 1
        if not checks["wipe_patch_anchor_occurs_once"]:
            return emit(result, args.output)
        wiped_dir = work / "opt_wipes_added"
        wiped_dir.mkdir()
        (wiped_dir / "qruov.c").write_text(
            original.replace(WIPE_ANCHOR, WIPE_PATCHED), encoding="ascii")

        opt_sources = [OPT / name for name in OPT_C]
        wiped_sources = [wiped_dir / "qruov.c" if name == "qruov.c" else OPT / name
                         for name in OPT_C]
        ref_sources = [REF / name for name in COMMON_C]
        jobs = []
        for level in LEVELS:
            tag = level.lstrip("-")
            jobs += [
                (level, OPT, opt_sources, "-DTREE_OPT", work / f"opt_{tag}"),
                (level, OPT, wiped_sources, "-DTREE_OPT", work / f"opt_wipes_added_{tag}"),
                (level, REF, ref_sources, "-DTREE_REF", work / f"ref_{tag}"),
            ]
        try:
            with ThreadPoolExecutor(max_workers=3) as pool:
                list(pool.map(
                    lambda job: compile_driver(gcc, job[0], job[1], job[2], job[3],
                                               driver, job[4]), jobs))
            runs = {}
            for level in LEVELS:
                tag = level.lstrip("-")
                runs[level] = {
                    "opt": run_driver(work / f"opt_{tag}", "plain"),
                    "opt_after_scrub": run_driver(work / f"opt_{tag}", "scrub"),
                    "opt_wipes_added": run_driver(work / f"opt_wipes_added_{tag}", "plain"),
                    "ref": run_driver(work / f"ref_{tag}", "plain"),
                }
        except (RuntimeError, subprocess.TimeoutExpired) as error:
            checks["builds_and_runs_completed"] = False
            measurements["error"] = str(error)
            return emit(result, args.output)
    checks["builds_and_runs_completed"] = True

    every = [run for level in runs.values() for run in level.values()]
    first = runs[next(iter(LEVELS))]["opt"]
    expected_params = f"{Q},{L},{V},{M},{SMALL_M},{TAU_N1},{TAU_N2}"
    checks["all_builds_same_parameters_and_keypair"] = all(
        run["params"] == expected_params and run["sk"] == first["sk"]
        and run["pk_seed"] == first["pk_seed"] and run["sd"] == first["sd"]
        for run in every)

    unwiped = ["sdt_pi1_eval", "sdt_pi1_reval", "sdt_pi2_eval", "pi2t_sd_eval"]
    opt_runs = [runs[level]["opt"] for level in LEVELS]
    scrub_runs = [runs[level]["opt_after_scrub"] for level in LEVELS]
    wiped_runs = [runs[level]["opt_wipes_added"] for level in LEVELS]
    ref_runs = [runs[level]["ref"] for level in LEVELS]

    # Stated behaviour.
    checks["opt_sdt_pi1_eval_fully_present_in_dead_stack"] = all(
        full(run, "sdt_pi1_eval") and "residue" in run for run in opt_runs)
    checks["opt_other_three_unwiped_arrays_fully_present"] = all(
        full(run, name) for run in opt_runs for name in unwiped[1:])
    # Same frame, opposite behaviour: the two arrays that opt does wipe.
    checks["opt_wiped_sdt_and_sdt_eval_absent"] = all(
        absent(run, "sdt") and absent(run, "sdt_eval") for run in opt_runs)
    # Control 1: a later call that reuses the dead stack removes the match.
    checks["control_scrub_removes_match"] = all(
        absent(run, name) for run in scrub_runs for name in unwiped)
    checks["control_scrub_covers_used_stack_and_is_seen"] = all(
        scrub["scrub_pattern"] == "1" and plain["scrub_pattern"] == "0"
        and int(plain["stack_used"]) < SCRUB_BYTES <= int(scrub["stack_used"])
        for plain, scrub in zip(opt_runs, scrub_runs))
    # Control 2: the reference tree, which wipes sdt_pi1 and sdt_pi2.
    checks["control_ref_no_match_for_sdt_pi1"] = all(
        absent(run, "sdt_pi1") and absent(run, "sdt_pi2") and absent(run, "sdt")
        for run in ref_runs)
    checks["control_ref_scan_sees_unwiped_public_pi1"] = all(
        mostly(run, "pi1_public") for run in ref_runs)
    # Control 3: opt source with the four missing wipes added.
    checks["control_opt_with_wipes_added_no_match"] = all(
        absent(run, name) for run in wiped_runs for name in unwiped)
    checks["control_opt_with_wipes_added_scan_sees_public_pi1"] = all(
        mostly(run, "pi1_public") for run in wiped_runs)

    # Recovery of S from the bytes read from the stack and the public seed.
    seed_pk = bytes.fromhex(first["pk_seed"])
    secret_key = bytes.fromhex(first["sk"])
    real_sd = bytes.fromhex(first["sd"])
    real_s = as_matrix_s(real_sd)
    model_sd = bytes(rejection_sample(
        hashlib.shake_256(secret_key + b"\x00\x00").digest(TAU_N2), N2))
    checks["python_model_of_S_equals_library_S"] = model_sd == real_sd

    residues = {run["residue"] for run in opt_runs if "residue" in run}
    checks["residue_bytes_same_at_both_levels"] = len(residues) == 1
    recovered_entries = 0
    interpolated_equals_ref = False
    if len(residues) == 1:
        x = interpolate(bytes.fromhex(residues.pop()))
        x_aos = bytes(value for row in x for element in row for value in element)
        interpolated_equals_ref = all(
            bytes.fromhex(run["expected_sdt_pi1"]) == x_aos for run in ref_runs)
        recovered = solve_for_s(public_p1(seed_pk, SMALL_M - 1), x)
        if recovered is not None:
            recovered_entries = sum(
                recovered[i][j] == real_s[i][j] for i in range(V) for j in range(M))
    checks["interpolated_residue_equals_ref_sdt_pi1"] = interpolated_equals_ref
    checks["recovered_S_equals_real_S"] = recovered_entries == V * M

    # matched_bytes: bytes of the recomputed array that are equal at the best
    # alignment in the used part of the dead stack (0: no 16-byte piece of it
    # occurs anywhere).  Compare with array_bytes.
    measurements["array_bytes"] = {
        name: entry["array_bytes"]
        for run in (first, ref_runs[0]) for name, entry in run["match"].items()
    }
    measurements["matched_bytes"] = {
        level: {
            name: {array: entry["matched_bytes"] for array, entry in run["match"].items()}
            for name, run in level_runs.items()
        }
        for level, level_runs in runs.items()
    }
    measurements["stack_used_bytes"] = {
        level: {name: int(run["stack_used"]) for name, run in level_runs.items()}
        for level, level_runs in runs.items()
    }
    measurements["sdt_pi1_eval_depth_below_stack_top"] = {
        level: runs[level]["opt"]["match"]["sdt_pi1_eval"]["depth_below_stack_top"]
        for level in LEVELS
    }
    measurements["stack_region_bytes"] = STACK_BYTES
    measurements["scrub_bytes"] = SCRUB_BYTES
    measurements["recovery"] = {
        "equation_index": SMALL_M - 1,
        "residue_bytes": LL * M * V,
        "entries_of_S_recovered": recovered_entries,
        "entries_of_S_total": V * M,
        "columns_of_S_solved": M,
    }
    result["status"] = "PASS" if all(checks.values()) else "FAIL"
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
