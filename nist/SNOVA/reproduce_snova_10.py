#!/usr/bin/env python3
"""Reproduce SNOVA-10: digest context leak and unchecked allocation in the optional OpenSSL build.

``symmetric.c`` of the submitted Optimized_Implementation has two SHAKE back
ends.  The default one is the bundled Keccak code.  With ``-DUSE_OPENSSL``
(not set by the submitted Makefile) a ``shake_t`` holds an ``EVP_MD_CTX``
pointer: ``shake128_init`` / ``shake256_init`` (``symmetric.c:24-32``) store
the result of ``EVP_MD_CTX_new()`` and pass it to ``EVP_DigestInit_ex``
without a NULL check, and only ``shake_release`` (``symmetric.c:50-52``) frees
it.  No file of the optimized tree calls ``shake_release``: not the one-shot
``shake256`` (``symmetric.c:89-95``) and not the two incremental uses in the
signer (``hash_combined``, ``snova_opt_16.c:364-374``, and the vinegar hash,
``snova_opt_16.c:780-788``).

This script

1. checks the cited source lines against pinned SHA-256 values;
2. compiles the unmodified optimized sources of SNOVA_I_K twice, with and
   without ``-DUSE_OPENSSL`` (no submitted build file is run), together with
   a small embedded driver and counting wrappers that the linker puts in
   front of ``EVP_MD_CTX_new``, ``EVP_MD_CTX_free`` and ``EVP_DigestInit_ex``
   (``-Wl,--wrap=<symbol>``; the wrappers see exactly the calls made by the
   submission code);
3. generates one key from the submitted DRBG with a fixed seed, signs and
   verifies 20 messages and records the contexts allocated and freed by every
   call;
4. in child processes, makes the k-th ``EVP_MD_CTX_new`` of one signature
   return NULL (k = 0..3) and records how the child ends.

Controls: the default build of the same sources never reaches the wrappers
and signs normally under the same forced failure; the ``-DUSE_OPENSSL`` build
signs and verifies when no failure is forced; a temporary copy with
``shake_release`` added at the three hashing sites frees every context.  A
second temporary copy with ``shake_release`` added to ``shake256`` only (the
function the review cites) still leaks the two contexts of the incremental
uses.  The copies are controls, not proposed repairs.  All four builds must
produce the same public key and the same signatures.

Limits: this concerns the optional ``-DUSE_OPENSSL`` build of the optimized
tree only; the submitted Makefile does not set the macro and leaves
``LIBS = -lcrypto`` commented out.  What is shown is a resource leak and a
fail-stop crash when an allocation fails.  The allocation failure is forced
by the wrapper; no real memory exhaustion is produced, no wrong signature is
output and no secret is exposed.  Only SNOVA_I_K is built, with gcc -O3 and
without ``-march=native``; the other 17 directories are covered by a source
check (same ``symmetric.c``, no call of ``shake_release``).  The behaviour of
``EVP_DigestInit_ex`` on a NULL context belongs to the OpenSSL version
recorded in the output.

Build: ``<cc> -std=c11 -O3 [-DUSE_OPENSSL] -I<set> snova_opt_16.c symmetric.c
sign.c aes.c rng.c driver.c wrap.c -Wl,--wrap=EVP_MD_CTX_new
-Wl,--wrap=EVP_MD_CTX_free -Wl,--wrap=EVP_DigestInit_ex -lcrypto``.  These
are the submitted CFLAGS without the warning options and without
``-march=native -mtune=native``.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (no C compiler, no OpenSSL 3.3 or later
development files, or a linker without ``--wrap``).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SCHEME = HERE.name
OPT = HERE / "submission/SNOVA_Round3/Optimized_Implementation"
SET = "SNOVA_I_K"

CANDIDATE = "SNOVA-10"
EVIDENCE = (
    "In the SNOVA optimized implementation built with -DUSE_OPENSSL (an optional build, not the "
    "submitted default), no OpenSSL digest context is ever freed, so every signature leaks 4 "
    "contexts (one more per signing retry), and when EVP_MD_CTX_new returns NULL the unchecked "
    "pointer goes to EVP_DigestInit_ex and the process is terminated by a signal, while the "
    "default build makes no such call and a temporary copy that calls shake_release at the three "
    "hashing sites frees every context; this shows a resource leak and a fail-stop crash under a "
    "forced allocation failure in the optional OpenSSL build only, and no secret exposure."
)

JOBS = 3
SIGNATURES = 20
REVIEW_CONTEXTS_PER_SIGNATURE = 4

SOURCES = ("snova_opt_16.c", "symmetric.c", "sign.c", "aes.c", "rng.c")
WRAPPED = ("EVP_MD_CTX_new", "EVP_MD_CTX_free", "EVP_DigestInit_ex")

SYMMETRIC_C = "4d970520416d66d291a1a4b12bc324ce6a1ba5b6810c9c149d4167b390fdf7cc"
# Files of Optimized_Implementation/SNOVA_I_K that are compiled or cited.
SET_SHA256 = {
    "symmetric.c": SYMMETRIC_C,
    "symmetric.h": "9740e4b354175edd9e7686b898c571c12e75c873ab99095bcc64eeaaa5ee7982",
    "snova_opt_16.c": "be17fd5e17ac4adc4892a9c844d0f16ca6932bc693db00d8d4084f5fbae9ed4e",
    "sign.c": "d79c85f77529a7f9d3787ca909c931c76dce14ba09ccf75e6b956bb1b7be8ddc",
    "aes.c": "42936e69655a11a6443c65e21e05f32a4861c9aa7c949a4674995f12c651ae64",
    "rng.c": "919b953692022f10b87ba9e28fba14c31e2d75d3d3986b72516b13e5094aef04",
    "rng.h": "9e35940ba45d85f419780a79de232d3f98ea15695c701053c884eda3ea537a40",
    "api.h": "7bb2d75096a2d489eb32acf75777c7cf83b4881dbe8b84bf8b0fa7646e7ec192",
    "snova.h": "91b38ca19ec6f12d34fb87a1cf5e299c5e71d4c96199f47e8714472579e5274c",
    "snova_params.h": "38c5eea6fadab312fede97ca97a0b27f30ab68c8b36523b0e9184c99c00925b8",
    "keccak_opt64.h": "f1c22592d19e7353abd9df8a8b5471162a76f7a446b85a904b787fba6e47b5e8",
    "Makefile": "d856f8f3dbd491ce5dd843552e8fdb58ca0bb0cf1a19e46c5b7caef7a7de35fd",
}

# Cited blocks: (file, first line (1-based), text).  They are meaningful
# because the hashes are pinned.
BLOCKS = {
    "openssl_guard": ("symmetric.c", 19, "#ifdef USE_OPENSSL\n"),
    "init": ("symmetric.c", 24, (
        "void shake128_init(shake_t* instance) {\n"
        "\t*(EVP_MD_CTX**)instance = EVP_MD_CTX_new();\n"
        "\tEVP_DigestInit_ex(*(EVP_MD_CTX**)instance, EVP_shake128(), NULL);\n"
        "}\n"
        "\n"
        "void shake256_init(shake_t* instance) {\n"
        "\t*(EVP_MD_CTX**)instance = EVP_MD_CTX_new();\n"
        "\tEVP_DigestInit_ex(*(EVP_MD_CTX**)instance, EVP_shake256(), NULL);\n"
        "}\n")),
    "release": ("symmetric.c", 50, (
        "void shake_release(shake_t* instance) {\n"
        "\tEVP_MD_CTX_free(*(EVP_MD_CTX**)instance);\n"
        "}\n"
        "\n"
        "#else\n")),
    "shake256": ("symmetric.c", 87, (
        "#endif\n"
        "\n"
        "void shake256(uint8_t* out, size_t outlen, const uint8_t* in, size_t inlen) {\n"
        "\tshake_t instance;\n"
        "\tshake256_init(&instance);\n"
        "\tshake_absorb(&instance, in, inlen);\n"
        "\tshake_finalize(&instance);\n"
        "\tshake_squeeze(out, outlen, &instance);\n"
        "}\n")),
    "hash_combined": ("snova_opt_16.c", 364, (
        "\tshake_t state;\n"
        "\tshake256_init(&state);\n"
        "#if HASH_PK\n"
        "\tshake_absorb(&state, pk_seed, BYTES_PK_HASH);\n"
        "#else\n"
        "\tshake_absorb(&state, pk_seed, SEED_LENGTH_PUBLIC);\n"
        "#endif\n"
        "\tshake_absorb(&state, m, mlen);\n"
        "\tshake_absorb(&state, salt, BYTES_SALT);\n"
        "\tshake_finalize(&state);\n"
        "\tshake_squeeze(hash_out, BYTES_HASH, &state);\n"
        "}\n")),
    "vinegar": ("snova_opt_16.c", 780, (
        "\t\tshake_t v_instance;\n"
        "\n"
        "\t\tshake256_init(&v_instance);\n"
        "\t\tshake_absorb(&v_instance, skx->sk_seed, SEED_LENGTH_PRIVATE);\n"
        "\t\tshake_absorb(&v_instance, digest, BYTES_DIGEST);\n"
        "\t\tshake_absorb(&v_instance, salt, BYTES_SALT);\n"
        "\t\tshake_absorb(&v_instance, &num_sign, 1);\n"
        "\t\tshake_finalize(&v_instance);\n"
        "\t\tshake_squeeze(vinegar_in_byte, NUM_GEN_SEC_BYTES, &v_instance);\n"
        "\n")),
    "expand_T12": ("snova_opt_16.c", 418,
                   "\tshake256(sk_data, (SNOVA_o * SNOVA_v * SNOVA_l + 1) / 2, seed, SEED_LENGTH_PRIVATE);\n"),
    "sign_digest": ("sign.c", 54, "\tshake256(digest, BYTES_DIGEST, m, mlen);\n"),
    "makefile_libs": ("Makefile", 9, "# LIBS = -lcrypto\n"),
    "makefile_compile": ("Makefile", 21, "\t$(CC) $(CFLAGS) $(P) -o $@ -c $<\n"),
}

# Temporary copies used as controls: file -> [(anchor, replacement)].  Every
# anchor must occur exactly once in the pristine file.
RELEASE_IN_SHAKE256 = {
    "symmetric.c": [(
        "\tshake_squeeze(out, outlen, &instance);\n",
        "\tshake_squeeze(out, outlen, &instance);\n\tshake_release(&instance);\n")],
}
RELEASE_IN_SIGNER = {
    "snova_opt_16.c": [
        ("\tshake_squeeze(hash_out, BYTES_HASH, &state);\n",
         "\tshake_squeeze(hash_out, BYTES_HASH, &state);\n\tshake_release(&state);\n"),
        ("\t\tshake_squeeze(vinegar_in_byte, NUM_GEN_SEC_BYTES, &v_instance);\n",
         "\t\tshake_squeeze(vinegar_in_byte, NUM_GEN_SEC_BYTES, &v_instance);\n"
         "\t\tshake_release(&v_instance);\n"),
    ],
}

# build name -> (use -DUSE_OPENSSL, patches of temporary copies)
BUILDS = {
    "openssl": (True, {}),
    "default": (False, {}),
    "openssl_copy_release_in_shake256": (True, RELEASE_IN_SHAKE256),
    "openssl_copy_release_at_all_three_sites": (True, {**RELEASE_IN_SHAKE256, **RELEASE_IN_SIGNER}),
}

WRAP_C = r"""
/* Counting wrappers (reproducer harness, not submission code).  With
 * -Wl,--wrap=<symbol> the linker sends every call of <symbol> made by the
 * linked objects to __wrap_<symbol>; __real_<symbol> is the libcrypto one. */
#include <stdio.h>
#include <openssl/evp.h>

EVP_MD_CTX *__real_EVP_MD_CTX_new(void);
void __real_EVP_MD_CTX_free(EVP_MD_CTX *ctx);
int __real_EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl);

long wrap_new = 0;       /* calls of EVP_MD_CTX_new */
long wrap_free = 0;      /* calls of EVP_MD_CTX_free with a context */
long wrap_fail_at = -1;  /* call number of EVP_MD_CTX_new that returns NULL */

EVP_MD_CTX *__wrap_EVP_MD_CTX_new(void) {
    long call = wrap_new++;
    if (call == wrap_fail_at) {
        fputs("wrapper: EVP_MD_CTX_new returns NULL\n", stderr);
        return NULL;
    }
    return __real_EVP_MD_CTX_new();
}

void __wrap_EVP_MD_CTX_free(EVP_MD_CTX *ctx) {
    if (ctx != NULL) wrap_free++;
    __real_EVP_MD_CTX_free(ctx);
}

int __wrap_EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl) {
    int rc;
    if (ctx == NULL) fputs("wrapper: EVP_DigestInit_ex entered with ctx=NULL\n", stderr);
    rc = __real_EVP_DigestInit_ex(ctx, type, impl);
    if (ctx == NULL) fprintf(stderr, "wrapper: EVP_DigestInit_ex returned %d\n", rc);
    return rc;
}
"""

DRIVER_C = r"""
/* Driver for the SNOVA optimized implementation (reproducer harness, not
 * submission code).  The key comes from crypto_sign_keypair with the
 * submitted DRBG seeded with 0,1,...,47.
 *   driver count <n>   sign and verify n messages, print the wrapper counters
 *   driver fail <k>    sign one message; the k-th EVP_MD_CTX_new call of that
 *                      signature (k = 0 is the first) returns NULL */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 33)
#include <malloc.h>
#define HAVE_MALLINFO2 1
#endif
#endif
#include "api.h"
#include "rng.h"

extern long wrap_new, wrap_free, wrap_fail_at;

#define MAXN 64

/* Bytes in use in the malloc arena (glibc), or -1. */
static long heap_in_use(void) {
#ifdef HAVE_MALLINFO2
    return (long)mallinfo2().uordblks;
#else
    return -1;
#endif
}

static void hex(const char *name, int k, const unsigned char *x, size_t n) {
    if (k >= 0) printf("%s%d=", name, k); else printf("%s=", name);
    for (size_t i = 0; i < n; i++) printf("%02x", x[i]);
    printf("\n");
}

int main(int argc, char **argv) {
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    static unsigned char sm[CRYPTO_BYTES + 8], opened[CRYPTO_BYTES + 8];
    static long row[MAXN][8];
    unsigned char entropy[48], msg[8] = "SNOVA10";
    unsigned long long smlen = 0, mlen = 0;
    const char *mode = argc > 1 ? argv[1] : "";
    long arg = argc > 2 ? atol(argv[2]) : -1;
    int rc;

    if ((strcmp(mode, "count") && strcmp(mode, "fail")) || arg < 0 || arg > MAXN) return 2;
    printf("name=%s\n", CRYPTO_ALGNAME);
    for (int i = 0; i < 48; i++) entropy[i] = (unsigned char)i;
    randombytes_init(entropy, NULL, 256);
    rc = crypto_sign_keypair(pk, sk);
    printf("keypair_rc=%d\nkeypair_new=%ld\nkeypair_free=%ld\n", rc, wrap_new, wrap_free);
    hex("pk", -1, pk, sizeof pk);
    if (rc) return 3;

    if (!strcmp(mode, "fail")) {
        /* The child is expected to crash: no core file, no crash report. */
        struct rlimit none = {0, 0};
        setrlimit(RLIMIT_CORE, &none);
#if defined(__linux__)
        prctl(PR_SET_DUMPABLE, 0);
#endif
        printf("signing=1\n");
        fflush(stdout);
        wrap_fail_at = wrap_new + arg;
        rc = crypto_sign(sm, &smlen, msg, sizeof msg, sk);
        wrap_fail_at = -1;
        printf("sign_returned=1\nsign_rc=%d\nsign_new=%ld\n", rc, wrap_new);
        rc = rc ? rc : crypto_sign_open(opened, &mlen, sm, smlen, pk);
        printf("open_rc=%d\n", rc || mlen != sizeof msg || memcmp(opened, msg, sizeof msg));
        return 0;
    }

    for (int k = 0; k < (int)arg; k++) {
        long new0 = wrap_new, free0 = wrap_free, new1, free1;
        msg[7] = (unsigned char)k;
        rc = crypto_sign(sm, &smlen, msg, sizeof msg, sk);
        new1 = wrap_new;
        free1 = wrap_free;
        row[k][0] = rc;
        row[k][1] = new1 - new0;
        row[k][2] = free1 - free0;
        row[k][3] = heap_in_use();
        rc = crypto_sign_open(opened, &mlen, sm, smlen, pk);
        row[k][4] = rc || mlen != sizeof msg || memcmp(opened, msg, sizeof msg);
        row[k][5] = wrap_new - new1;
        row[k][6] = wrap_free - free1;
        row[k][7] = heap_in_use();
        hex("sm", k, sm, (size_t)smlen);
    }
    for (int k = 0; k < (int)arg; k++) {
        printf("sign_rc%d=%ld\nsign_new%d=%ld\nsign_free%d=%ld\nheap_after_sign%d=%ld\n",
               k, row[k][0], k, row[k][1], k, row[k][2], k, row[k][3]);
        printf("open_rc%d=%ld\nopen_new%d=%ld\nopen_free%d=%ld\nheap_after_open%d=%ld\n",
               k, row[k][4], k, row[k][5], k, row[k][6], k, row[k][7]);
    }
    printf("total_new=%ld\ntotal_free=%ld\ndone=1\n", wrap_new, wrap_free);
    return 0;
}
"""

PROBE_C = r"""
#include <stdio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
EVP_MD_CTX *__real_EVP_MD_CTX_new(void);
static int calls;
EVP_MD_CTX *__wrap_EVP_MD_CTX_new(void) { calls++; return __real_EVP_MD_CTX_new(); }
int main(void) {
    unsigned char out[8];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = ctx != NULL && EVP_DigestInit_ex(ctx, EVP_shake256(), NULL) == 1
             && EVP_DigestSqueeze(ctx, out, sizeof out) == 1;
    EVP_MD_CTX_free(ctx);
    printf("probe %d %d\n%s\n", ok, calls, OpenSSL_version(OPENSSL_VERSION));
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
    return subprocess.run([str(c) for c in cmd], cwd=cwd, capture_output=True, text=True,
                          timeout=timeout, env=dict(os.environ, LC_ALL="C"))


def wrap_flags() -> list:
    return [f"-Wl,--wrap={symbol}" for symbol in WRAPPED]


# --------------------------------------------------------------------------
# Source checks
# --------------------------------------------------------------------------

def source_checks() -> tuple[dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    problems: list = []
    directory = OPT / SET

    ok = True
    for name, expected in SET_SHA256.items():
        rel = f"{SCHEME}/{(directory / name).relative_to(HERE).as_posix()}"
        pinned[rel] = sha256(directory / name)
        if pinned[rel] != expected:
            problems.append(f"sha256 mismatch {rel}: {pinned[rel]}")
            ok = False
    checks["source_sha256_pinned_files_match"] = ok
    if not ok:
        return checks, pinned, problems

    def text(name: str) -> str:
        return (directory / name).read_text(encoding="utf-8")

    def cited(*keys: str) -> bool:
        found = True
        for key in keys:
            name, first, wanted = BLOCKS[key]
            lines = text(name).splitlines(keepends=True)
            count = wanted.count("\n")
            if "".join(lines[first - 1:first - 1 + count]) != wanted:
                problems.append(f"{SCHEME}/.../{SET}/{name}:{first} is not the cited text")
                found = False
        return found

    checks["source_shake_init_passes_EVP_MD_CTX_new_result_on_without_null_check"] = cited(
        "openssl_guard", "init")
    checks["source_only_shake_release_frees_the_context"] = (
        cited("release") and text("symmetric.c").count("EVP_MD_CTX_free") == 1)
    checks["source_shake256_does_not_call_shake_release"] = cited("shake256")
    checks["source_signer_hashes_do_not_call_shake_release"] = (
        cited("hash_combined", "vinegar", "expand_T12", "sign_digest")
        and text("snova_opt_16.c").count("shake256_init(") == 2
        and text("snova_opt_16.c").count("shake_release") == 0
        and text("sign.c").count("shake_release") == 0)
    checks["source_makefile_does_not_enable_the_openssl_build"] = (
        cited("makefile_libs", "makefile_compile")
        and "USE_OPENSSL" not in text("Makefile") and "-lcrypto" not in
        "".join(row for row in text("Makefile").splitlines() if not row.startswith("#")))

    # The other 17 directories: same symmetric.c, and shake_release occurs
    # only as its two definitions and its prototype.
    set_dirs = sorted(p for p in OPT.iterdir() if p.is_dir()) if OPT.is_dir() else []
    same = len(set_dirs) == 18
    for other in set_dirs:
        same &= sha256(other / "symmetric.c") == SYMMETRIC_C
        for path in sorted(other.iterdir()):
            if path.suffix not in (".c", ".h"):
                continue
            wanted = {"symmetric.c": 2, "symmetric.h": 1}.get(path.name, 0)
            same &= path.read_text(encoding="utf-8", errors="replace").count("shake_release") == wanted
    checks["source_no_file_of_the_18_optimized_directories_calls_shake_release"] = same
    return checks, pinned, problems


# --------------------------------------------------------------------------
# Tools, build and run
# --------------------------------------------------------------------------

def find_tools(work: Path) -> dict:
    compiler = shutil.which("gcc") or shutil.which("cc")
    if not compiler:
        raise Skip("no C compiler (gcc or cc) found")
    (work / "probe.c").write_text(PROBE_C, encoding="utf-8")
    try:
        built = run([compiler, "-std=c11", "-O1", work / "probe.c", "-Wl,--wrap=EVP_MD_CTX_new",
                     "-lcrypto", "-o", work / "probe"], work, 120)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"the C compiler cannot be run: {error}") from error
    if built.returncode:
        raise Skip("cannot build an OpenSSL program that uses EVP_DigestSqueeze (OpenSSL 3.3 or "
                   f"later) with -Wl,--wrap: {built.stderr.strip()[-300:]}")
    try:
        probe = run([work / "probe"], work, 60)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"the OpenSSL probe cannot be run: {error}") from error
    rows = probe.stdout.splitlines()
    if probe.returncode or rows[:1] != ["probe 1 1"]:
        raise Skip(f"the OpenSSL SHAKE256 / --wrap probe does not work: {probe.stdout.strip()[-200:]}")
    version = run([compiler, "--version"], work, 60).stdout.splitlines()
    return {"cc": compiler, "compiler": version[0].strip() if version else "",
            "libcrypto": rows[1].strip() if len(rows) > 1 else ""}


def build(tools: dict, work: Path, name: str) -> Path:
    use_openssl, patches = BUILDS[name]
    directory = OPT / SET
    sources = []
    for source in SOURCES:
        path = directory / source
        if source in patches:
            # Patched copy in the temporary directory; the submission is not touched.
            text = path.read_text(encoding="utf-8")
            for anchor, replacement in patches[source]:
                if text.count(anchor) != 1:
                    raise RuntimeError(f"anchor does not occur exactly once in {source}: {anchor!r}")
                text = text.replace(anchor, replacement)
            path = work / f"{name}_{source}"
            path.write_text(text, encoding="utf-8")
        sources.append(path)
    binary = work / f"driver_{name}"
    cmd = [tools["cc"], "-std=c11", "-O3", *(["-DUSE_OPENSSL"] if use_openssl else []),
           f"-I{directory}", *sources, work / "driver.c", work / "wrap.c", *wrap_flags(),
           "-lcrypto", "-o", binary]
    built = run(cmd, work, 300)
    if built.returncode:
        raise RuntimeError(f"build failed ({binary.name}): {built.stderr[-800:]}")
    return binary


def fields(text: str) -> dict:
    return dict(row.split("=", 1) for row in text.splitlines() if "=" in row)


def count_run(work: Path, binary: Path) -> dict:
    done = run([binary, "count", SIGNATURES], work, 120)
    values = fields(done.stdout)
    if done.returncode or values.get("done") != "1" or values.get("name") != SET:
        raise RuntimeError(f"driver failed ({binary.name}): rc={done.returncode} {done.stderr[-400:]}")

    def column(key: str) -> list:
        return [int(values[f"{key}{k}"]) for k in range(SIGNATURES)]

    signatures = hashlib.sha256()
    for k in range(SIGNATURES):
        signatures.update(bytes.fromhex(values[f"sm{k}"]))
    heap_sign, heap_open = column("heap_after_sign"), column("heap_after_open")
    result = {
        "keypair": {"rc": int(values["keypair_rc"]), "new": int(values["keypair_new"]),
                    "free": int(values["keypair_free"])},
        "sign": {"rc": column("sign_rc"), "new": column("sign_new"), "free": column("sign_free")},
        "open": {"rc": column("open_rc"), "new": column("open_new"), "free": column("open_free")},
        "total_new": int(values["total_new"]),
        "total_free": int(values["total_free"]),
        "public_key_sha256": hashlib.sha256(bytes.fromhex(values["pk"])).hexdigest(),
        "signed_messages_sha256": signatures.hexdigest(),
    }
    if min(heap_sign + heap_open) >= 0:
        # Growth of the bytes in use in the malloc arena: over each
        # crypto_sign call after the first, and over each crypto_sign_open.
        result["heap_growth"] = {
            "per_crypto_sign": [heap_sign[k] - heap_open[k - 1] for k in range(1, SIGNATURES)],
            "per_crypto_sign_open": [heap_open[k] - heap_sign[k] for k in range(SIGNATURES)],
        }
    return result


def fail_run(work: Path, binary: Path, call: int) -> dict:
    """Child process: the call-th EVP_MD_CTX_new of one signature returns NULL."""
    try:
        done = run([binary, "fail", call], work, 120)
    except subprocess.TimeoutExpired:
        return {"timeout": True}
    values = fields(done.stdout)
    name = None
    if done.returncode < 0:
        try:
            name = signal.Signals(-done.returncode).name
        except ValueError:
            name = f"signal {-done.returncode}"
    return {
        "timeout": False,
        "signal": name,
        "exit_status": done.returncode if done.returncode >= 0 else None,
        "reached_crypto_sign": values.get("signing") == "1",
        "crypto_sign_returned": values.get("sign_returned") == "1",
        "sign_rc": int(values["sign_rc"]) if "sign_rc" in values else None,
        "open_rc": int(values["open_rc"]) if "open_rc" in values else None,
        "wrapper_returned_null": "wrapper: EVP_MD_CTX_new returns NULL" in done.stderr,
        "EVP_DigestInit_ex_entered_with_null": "wrapper: EVP_DigestInit_ex entered with ctx=NULL" in done.stderr,
        "EVP_DigestInit_ex_returned": "wrapper: EVP_DigestInit_ex returned" in done.stderr,
    }


def summary(entry: dict) -> dict:
    """Counters of one build, per call."""
    sign, opened = entry["sign"], entry["open"]
    out = {
        "keypair_new_free": [entry["keypair"]["new"], entry["keypair"]["free"]],
        "crypto_sign_new_per_call": sign["new"],
        "crypto_sign_free_per_call": sign["free"],
        "crypto_sign_new_histogram": {str(k): v for k, v in sorted(Counter(sign["new"]).items())},
        "crypto_sign_open_new_per_call": sorted(set(opened["new"])),
        "crypto_sign_open_free_per_call": sorted(set(opened["free"])),
        "total_new": entry["total_new"],
        "total_free": entry["total_free"],
        "never_freed": entry["total_new"] - entry["total_free"],
    }
    if "heap_growth" in entry:
        out["heap_bytes_growth_per_crypto_sign"] = {
            str(k): v for k, v in sorted(Counter(entry["heap_growth"]["per_crypto_sign"]).items())}
        out["heap_bytes_growth_per_crypto_sign_open"] = {
            str(k): v for k, v in sorted(Counter(entry["heap_growth"]["per_crypto_sign_open"]).items())}
    return out


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def render(value, depth: int = 0) -> str:
    """JSON text: the top three levels one entry per line, deeper values inline."""
    if isinstance(value, dict) and value and depth < 3:
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
    checks, pinned, problems = source_checks()
    result["checks"].update(checks)
    result["source_sha256"] = pinned
    if problems:
        result["problems"] = problems
    if not all(checks.values()):
        return emit(result, args.output)

    with tempfile.TemporaryDirectory(prefix="snova-10-") as temporary:
        work = Path(temporary)
        try:
            tools = find_tools(work)
        except Skip as reason:
            result["status"] = "SKIP"
            result["skip_reason"] = str(reason)
            return emit(result, args.output)
        (work / "driver.c").write_text(DRIVER_C, encoding="utf-8")
        (work / "wrap.c").write_text(WRAP_C, encoding="utf-8")
        try:
            with ThreadPoolExecutor(max_workers=JOBS) as pool:
                binaries = dict(zip(BUILDS, pool.map(lambda name: build(tools, work, name), BUILDS)))
            counts = {name: count_run(work, binary) for name, binary in binaries.items()}
            # Forced allocation failure: one child per allocation of a signature.
            forced = {f"openssl/call_{call}": fail_run(work, binaries["openssl"], call)
                      for call in range(REVIEW_CONTEXTS_PER_SIGNATURE)}
            forced["default/call_0"] = fail_run(work, binaries["default"], 0)
        except (RuntimeError, subprocess.TimeoutExpired, OSError, KeyError, ValueError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    m = result["measurements"]
    m["compiler"] = tools["compiler"]
    m["libcrypto"] = tools["libcrypto"]
    m["parameter_set"] = SET
    m["build"] = ("<cc> -std=c11 -O3 [-DUSE_OPENSSL] -I<set> snova_opt_16.c symmetric.c sign.c aes.c "
                  "rng.c driver.c wrap.c (embedded) " + " ".join(wrap_flags()) + " -lcrypto")
    m["signatures"] = SIGNATURES
    m["review_contexts_per_signature"] = REVIEW_CONTEXTS_PER_SIGNATURE
    m["allocation_sites_per_signature"] = (
        "source reading: shake256() from expand_T12 (snova_opt_16.c:418) and from crypto_sign "
        "(sign.c:54), hash_combined (snova_opt_16.c:365), and the vinegar hash once per signing "
        "attempt (snova_opt_16.c:782)")
    m["temporary_copies"] = {
        "openssl_copy_release_in_shake256": "shake_release(&instance) added at the end of shake256() "
                                            "(symmetric.c:94)",
        "openssl_copy_release_at_all_three_sites": "the same, plus shake_release after the squeeze in "
                                                   "hash_combined (snova_opt_16.c:374) and in the "
                                                   "vinegar hash (snova_opt_16.c:788)",
    }
    m["difference_from_review"] = (
        "the review attributes the leak to shake256() (symmetric.c:89-95); measured, 2 of the 4 "
        "contexts of a signature come from shake256() and 2 from the incremental hashes in "
        "snova_opt_16.c (hash_combined and the vinegar hash), which do not call shake_release either")
    m["contexts"] = {name: summary(entry) for name, entry in counts.items()}
    m["forced_null_from_EVP_MD_CTX_new"] = forced
    m["same_outputs"] = {name: {"public_key_sha256": entry["public_key_sha256"],
                                "signed_messages_sha256": entry["signed_messages_sha256"]}
                         for name, entry in counts.items()}

    c = result["checks"]

    def works(name: str) -> bool:
        entry = counts[name]
        return (entry["keypair"]["rc"] == 0 and not any(entry["sign"]["rc"])
                and not any(entry["open"]["rc"]))

    ossl, default = counts["openssl"], counts["default"]
    partial = counts["openssl_copy_release_in_shake256"]
    full = counts["openssl_copy_release_at_all_three_sites"]
    most_common = Counter(ossl["sign"]["new"]).most_common(1)[0][0]

    # Stated behaviour: contexts are allocated and never freed.
    c["openssl_build_signs_and_verifies"] = works("openssl")
    c["openssl_build_allocates_4_contexts_per_signature_or_more_on_a_retry"] = (
        min(ossl["sign"]["new"]) == most_common == REVIEW_CONTEXTS_PER_SIGNATURE)
    c["openssl_build_frees_no_context_in_any_call"] = (
        ossl["total_free"] == 0 and not any(ossl["sign"]["free"]) and not any(ossl["open"]["free"])
        and ossl["total_new"] == ossl["keypair"]["new"] + sum(ossl["sign"]["new"]) + sum(ossl["open"]["new"])
        and ossl["keypair"]["new"] > 0 and min(ossl["open"]["new"]) > 0)
    c["copy_with_shake_release_in_shake256_only_still_leaks_2_contexts_per_signature"] = (
        works("openssl_copy_release_in_shake256") and partial["sign"]["new"] == ossl["sign"]["new"]
        and all(freed == 2 for freed in partial["sign"]["free"])
        and min(new - freed for new, freed in zip(partial["sign"]["new"], partial["sign"]["free"])) == 2)

    # Stated behaviour: a NULL context crashes the signer.
    crashes = [forced[f"openssl/call_{call}"] for call in range(REVIEW_CONTEXTS_PER_SIGNATURE)]
    c["openssl_build_forced_null_context_ends_signing_by_a_signal"] = all(
        not entry["timeout"] and entry["signal"] is not None and entry["reached_crypto_sign"]
        and not entry["crypto_sign_returned"] and entry["wrapper_returned_null"] for entry in crashes)
    c["null_context_reaches_EVP_DigestInit_ex_which_does_not_return"] = all(
        not entry["timeout"] and entry["EVP_DigestInit_ex_entered_with_null"]
        and not entry["EVP_DigestInit_ex_returned"] for entry in crashes)

    # Controls.
    c["control_default_build_never_calls_EVP_MD_CTX_new"] = (
        works("default") and default["total_new"] == 0 and default["total_free"] == 0)
    control = forced["default/call_0"]
    c["control_default_build_signs_normally_under_the_same_forced_failure"] = (
        not control["timeout"] and control["signal"] is None and control["exit_status"] == 0
        and control["crypto_sign_returned"] and control["sign_rc"] == 0 and control["open_rc"] == 0
        and not control["wrapper_returned_null"])
    c["control_copy_with_shake_release_at_all_three_sites_frees_every_context"] = (
        works("openssl_copy_release_at_all_three_sites")
        and full["sign"]["new"] == ossl["sign"]["new"] and full["sign"]["free"] == full["sign"]["new"]
        and full["open"]["free"] == full["open"]["new"]
        and full["keypair"]["free"] == full["keypair"]["new"] > 0
        and full["total_free"] == full["total_new"] == ossl["total_new"])
    c["control_all_four_builds_give_the_same_key_and_signatures"] = len(
        {(entry["public_key_sha256"], entry["signed_messages_sha256"]) for entry in counts.values()}) == 1

    result["status"] = "PASS" if all(c.values()) and not problems else "FAIL"
    # The run time goes to stderr so that the JSON is identical on every run.
    print(f"elapsed {time.monotonic() - started:.1f} s", file=sys.stderr)
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
