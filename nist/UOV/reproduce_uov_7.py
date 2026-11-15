#!/usr/bin/env python3
"""Reproduce UOV-7: crypto_sign_open never returns for inputs of 2^32 bytes or more.

``crypto_sign_open`` (``sign.c:130-162``, the same file in all 60 parameter-set
directories of the submission) takes 64-bit lengths but runs its two byte
loops with ``unsigned i`` (``sign.c:137``):

* accept path, ``sign.c:147``: ``for(i = 0; i < *mlen; ++i) m[i] = sm[i];``
* reject path, ``sign.c:157``: ``for(i = 0; i < smlen; ++i) { m[i] = 0; }``

Where ``unsigned`` has 32 bits the index wraps to 0 after 2^32 - 1, so the
reject loop cannot end when ``smlen >= 2^32`` and the accept loop cannot end
when the message has 2^32 bytes or more.  The index stays below 2^32, which is
inside the buffers the caller must provide, so there is no out-of-bounds
write.

This script

1. checks the cited source lines against pinned SHA-256 values;
2. compiles the unmodified reference sources of uov-Ip (no submitted build
   file is run) with gcc, the submitted ``nistkat/rng.c`` and a small embedded
   driver, and generates one key pair from the DRBG with a fixed seed;
3. reject path, one child process per run:
   control: ``crypto_sign_open`` on 2^32 - 1 zero bytes returns -1;
   control: ``crypto_sign_verify`` on 2^32 zero bytes (the call that
   ``crypto_sign_open`` makes at ``sign.c:143``) returns -1;
   then, in the same process and on the same input, ``crypto_sign_open`` on
   the 2^32 zero bytes does not return within the timeout;
   control: a temporary copy of ``sign.c`` in which the index is declared
   ``unsigned long long i`` rejects the same 2^32 zero bytes (the copy is a
   control, not a proposed repair);
4. accept path (run only if the host has the memory and the time budget
   allows it): a message of 2^32 - 1 bytes is signed and opens correctly
   although the signed message is longer than 2^32 bytes (control); a signed
   message of 2^32 bytes does not return from ``crypto_sign_open`` within the
   timeout.

The timeout is counted from the moment the child enters ``crypto_sign_open``
and is the larger of 60 s and four times the measured time of the matching
control call.

To show that the call is looping and not merely slow, the driver sets
``m[0]`` to a marker byte and a 100 ms timer looks at it: every time the
marker has been overwritten the driver counts one rewrite, records the value
and restores the marker.  A loop with a wide enough index writes ``m[0]``
once (the controls); the wrapped loop writes it again on every pass.  The
value written is 0 in the reject path and the first message byte (1) in the
accept path.  ``crypto_sign_open`` never reads ``m``, so the marker does not
change what it does.

Limits: this shows a denial of service (a call that does not return) for
inputs of 2^32 bytes or more, through the submitted NIST API of the reference
code; "does not return" is observed up to the timeout only.  No out-of-bounds
write or other memory corruption is shown.  Only uov-Ip of the reference tree
is built, with gcc -O3; the other 59 directories (reference and optimized
trees) are covered by a source check (identical ``sign.c``).  Inputs below
2^32 bytes are not affected.  The lengths are ``size_t`` in the PQM4 and OQS
variants of the prototype (``sign.c:132``); those are not built.

Build: ``<cc> -O3 -std=c99 -fno-omit-frame-pointer -D_NIST_KAT_ -I<Ip>
-I<nistkat> <Ip>/*.c <nistkat>/rng.c driver.c -lcrypto``: the CFLAGS of the
submitted Makefile with ``KAT=1``, without its warning options.

Memory: about 4 GiB per reject-path run and about 8 GiB per accept-path run;
the runs are made one at a time.  Run time: about 3.5 minutes with the accept
path, about 2 minutes without.  The measured times are part of the output, so
the JSON differs slightly from run to run.

``source_sha256`` lists the cited files and one entry ``<directory>/*``: the
SHA-256 of the output of ``sha256sum *`` run in that directory with
``LC_ALL=C``.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (no C compiler, no OpenSSL development
files, ``unsigned`` wider than 32 bits or ``size_t`` narrower than 64 bits,
less than 5 GiB of available memory, or a failed allocation).
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
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SCHEME = HERE.name
SUBMISSION = "submission/UOV-R3"
REF = f"{SUBMISSION}/Reference_Implementation"
OPT = f"{SUBMISSION}/Optimized_Implementation"
SET = "Ip"

CANDIDATE = "UOV-7"
EVIDENCE = (
    "crypto_sign_open of the UOV reference code (uov-Ip, gcc -O3) runs its byte loops with a "
    "32-bit unsigned index against 64-bit lengths: an all-zero input of 2^32 - 1 bytes is "
    "rejected, while the same call on 2^32 zero bytes does not return within a timeout of at "
    "least 60 s and at least four times the control's time and keeps rewriting m[0], and a "
    "validly signed message of 2^32 bytes does the same in the accept path while one of "
    "2^32 - 1 bytes opens correctly; this shows a denial of service through the submitted API "
    "for inputs of 2^32 bytes or more only, and no out-of-bounds write or other memory corruption."
)
EVIDENCE_WITHOUT_ACCEPT_PATH = EVIDENCE.replace(
    "keeps rewriting m[0], and a validly signed message of 2^32 bytes does the same in the accept "
    "path while one of 2^32 - 1 bytes opens correctly; this shows",
    "keeps rewriting m[0]; this shows").replace(
    "memory corruption.", "memory corruption; the accept path was not run on this host.")

TWO_32 = 1 << 32
GIB = 1 << 30
MIN_TIMEOUT = 60.0          # seconds
TIMEOUT_RATIO = 4.0         # times the control's measured call time
CONTROL_CAP = 240.0         # a control call that takes longer than this fails
BUDGET = 300.0              # the accept path is run only if the total stays below this
REJECT_MEMORY = 5 * GIB     # available memory wanted for the reject path
ACCEPT_MEMORY = 10 * GIB    # available memory wanted for the accept path
MARK = 0xA5

SIGN_C = "a17c11cb832d875a050fad444387ce486a5c2b354c54ea7fe687515e0d7ebc5e"
FILE_SHA256 = {
    f"{REF}/{SET}/sign.c": SIGN_C,
    f"{REF}/{SET}/api.h": "1c99abcdd744f1efbf45dd21a573d26e78a4a6a798bbde5a0cb22f0de0afc34a",
    f"{REF}/{SET}/params.h": "37246e6fb555f600df1d0865a60a8ad5c60cd8c7331103e0f81c9a827cdfe919",
    f"{REF}/{SET}/config.h": "3e78b48229f000ff753a8d4d745d4a7241c260c6eca1c551bfca6c9fedd9195f",
    f"{REF}/nistkat/rng.c": "955e4b249990dd505ed0e43ff20b7a0d4da5a62eba72fd8eba650f75859c3263",
    f"{REF}/nistkat/rng.h": "81209b4388f6171e71b2885fbdaf239435342d2621be3c3cfd34a50374bbc959",
    f"{REF}/Makefile": "9929bbe4c276f90f2f04741a0ba6ea042a29f64964433d7ba4401194e3dc8cc5",
}
# The compiled directory: (number of files, SHA-256 of its `sha256sum *` listing).
SET_DIGEST = (35, "aaa928be69731067894fd42daa87617fe00e6cad798fb7041ebec315971cf998")
SIGN_C_COPIES = 60          # 12 reference and 48 optimized directories

# sign.c:130-162 (1-based), meaningful because the hash is pinned.
OPEN_FIRST_LINE = 130
OPEN_TEXT = """\
int
#if defined(PQM4) || defined(_UTILS_OQS_)
crypto_sign_open(unsigned char *m, size_t *mlen, const unsigned char *sm, size_t smlen, const unsigned char *pk)
#else
crypto_sign_open(unsigned char *m, unsigned long long *mlen, const unsigned char *sm, unsigned long long smlen, const unsigned char *pk)
#endif
{
    unsigned i;
    if ( OV_SIGNATUREBYTES > smlen ) {
        goto badsig;
    }

    *mlen = smlen - OV_SIGNATUREBYTES;
    if(crypto_sign_verify(sm + *mlen, OV_SIGNATUREBYTES, sm, *mlen, pk)) {
      goto badsig;
    } else {
      /* All good, copy msg, return 0 */
      for(i = 0; i < *mlen; ++i)
        m[i] = sm[i];
      return 0;
    }

badsig:
    /* Signature verification failed */
    *mlen = 0;
    // NOTE: This is not a mistake
    // The NIST API requires the message buffer to be at least smlen bytes
    for(i = 0; i < smlen; ++i) {
      m[i] = 0;
    }

    return -1;
}
"""
CITED = {
    134: "crypto_sign_open(unsigned char *m, unsigned long long *mlen, const unsigned char *sm, "
         "unsigned long long smlen, const unsigned char *pk)",
    137: "    unsigned i;",
    143: "    if(crypto_sign_verify(sm + *mlen, OV_SIGNATUREBYTES, sm, *mlen, pk)) {",
    147: "      for(i = 0; i < *mlen; ++i)",
    148: "        m[i] = sm[i];",
    157: "    for(i = 0; i < smlen; ++i) {",
    158: "      m[i] = 0;",
}

# Control build: one changed line in a temporary copy of sign.c.
COPY_OLD = "    unsigned i;\n"
COPY_NEW = "    unsigned long long i;\n"

DRIVER_C = r"""
/* Driver for crypto_sign_open of the UOV reference code (reproducer harness,
 * not submission code).  The key comes from crypto_sign_keypair with the
 * submitted DRBG (nistkat/rng.c) seeded with 0,1,...,47.
 *   driver open <smlen> <s>         crypto_sign_open on smlen zero bytes
 *   driver verify-open <smlen> <s>  crypto_sign_verify on the same input as
 *                                   crypto_sign_open calls it (sign.c:143),
 *                                   then crypto_sign_open
 *   driver sign-open <mlen> <s>     sign a message of mlen bytes (first byte
 *                                   1, the rest 0), then crypto_sign_open
 * The driver ends itself if crypto_sign_open has not returned after <s>
 * seconds, so that it cannot outlive the script that started it.
 * While crypto_sign_open runs, a 100 ms timer looks at m[0]: the driver set it
 * to MARK, and each time it has been overwritten the driver counts one
 * rewrite, records the value and restores MARK. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include "api.h"
#include "rng.h"

#define MARK 0xA5

static volatile unsigned char *watch;
static volatile sig_atomic_t rewrites = 0, last_value = -1, ticks = 0, tick_limit = 0;

static int put(char *to, const char *name, long value) {
    char digits[24];
    int n = 0, k = 0;
    while (*name) to[n++] = *name++;
    do { digits[k++] = (char)('0' + value % 10); value /= 10; } while (value);
    while (k) to[n++] = digits[--k];
    return n;
}

static void look(void) {
    unsigned char seen = *watch;
    char line[96];
    int n;
    if (seen == MARK) return;
    rewrites++;
    last_value = seen;
    *watch = MARK;
    n = put(line, "rewrite=", (long)rewrites);
    n += put(line + n, " value=", (long)seen);
    n += put(line + n, " tick=", (long)ticks);
    line[n++] = '\n';
    if (write(2, line, (size_t)n) < 0) return;
}

static void on_tick(int sig) {
    (void)sig;
    ticks++;
    look();
    if (ticks > tick_limit) _exit(79);
}

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}

int main(int argc, char **argv) {
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    unsigned char entropy[48], *sm, *m, *msg = NULL;
    unsigned long long length, smlen, mlen = 0, opened = 7;
    const char *mode = argc > 1 ? argv[1] : "";
    struct sigaction action;
    struct itimerval every = {{0, 100000}, {0, 100000}}, never = {{0, 0}, {0, 0}};
    double t0;
    int rc, signing = !strcmp(mode, "sign-open");

    if (argc != 4 || (strcmp(mode, "open") && strcmp(mode, "verify-open") && !signing)) return 2;
    length = strtoull(argv[2], NULL, 0);
    tick_limit = 10 * atoi(argv[3]);
#if defined(__linux__)
    prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
    printf("name=%s\nsignature_bytes=%d\n", CRYPTO_ALGNAME, (int)CRYPTO_BYTES);
    printf("sizeof_unsigned=%d\nsizeof_unsigned_long_long=%d\nsizeof_size_t=%d\n",
           (int)sizeof(unsigned), (int)sizeof(unsigned long long), (int)sizeof(size_t));
    fflush(stdout);
    if (sizeof(unsigned) != 4 || sizeof(size_t) < 8) return 78;

    for (int i = 0; i < 48; i++) entropy[i] = (unsigned char)i;
    randombytes_init(entropy, NULL, 256);
    rc = crypto_sign_keypair(pk, sk);
    printf("keypair_rc=%d\n", rc);
    if (rc) return 3;

    if (signing) {
        mlen = length;
        smlen = mlen + CRYPTO_BYTES;
        msg = calloc(mlen, 1);
    } else {
        smlen = length;
        if (smlen < CRYPTO_BYTES) return 2;
    }
    sm = signing ? malloc(smlen) : calloc(smlen, 1);
    m = malloc(smlen);
    if (!sm || !m || (signing && !msg)) {
        printf("alloc_failed=1\n");
        return 77;
    }
    if (signing) {
        msg[0] = 1;
        t0 = now();
        rc = crypto_sign(sm, &smlen, msg, mlen, sk);
        printf("sign_rc=%d\nsmlen=%llu\nsign_seconds=%.3f\n", rc, smlen, now() - t0);
        if (rc) return 4;
    } else if (!strcmp(mode, "verify-open")) {
        t0 = now();
        rc = crypto_sign_verify(sm + smlen - CRYPTO_BYTES, CRYPTO_BYTES, sm, smlen - CRYPTO_BYTES, pk);
        printf("verify_rc=%d\nverify_seconds=%.3f\n", rc, now() - t0);
    }

    m[0] = MARK;
    watch = m;
    memset(&action, 0, sizeof action);
    action.sa_handler = on_tick;
    action.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &action, NULL);
    printf("smlen_passed=%llu\ncalling_open=1\n", smlen);
    fflush(stdout);
    setitimer(ITIMER_REAL, &every, NULL);
    t0 = now();
    rc = crypto_sign_open(m, &opened, sm, smlen, pk);
    t0 = now() - t0;
    setitimer(ITIMER_REAL, &never, NULL);
    signal(SIGALRM, SIG_IGN);
    look();
    printf("open_rc=%d\nopen_mlen=%llu\nopen_seconds=%.3f\nrewrites=%d\nlast_value=%d\n",
           rc, opened, t0, (int)rewrites, (int)last_value);
    if (signing && rc == 0 && opened == mlen && rewrites == 1) {
        m[0] = (unsigned char)last_value;       /* undo the marker */
        printf("output_matches=%d\n", memcmp(m, msg, mlen) == 0);
    }
    printf("returned=1\n");
    return 0;
}
"""

PROBE_C = r"""
#include <stdio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
int main(void) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    int ok = ctx != NULL && EVP_DigestInit_ex(ctx, EVP_shake256(), NULL) == 1;
    EVP_MD_CTX_free(ctx);
    printf("probe %d\n%s\n", ok, OpenSSL_version(OPENSSL_VERSION));
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


# --------------------------------------------------------------------------
# Source checks
# --------------------------------------------------------------------------

def directory_digest(directory: Path) -> tuple[int, str]:
    """SHA-256 of what `LC_ALL=C sha256sum *` prints in the directory."""
    if not directory.is_dir():
        return 0, "missing"
    names = sorted(p.name for p in directory.iterdir())
    if any(not (directory / name).is_file() for name in names):
        return len(names), "not a flat directory"
    listing = "".join(f"{sha256(directory / name)}  {name}\n" for name in names)
    return len(names), hashlib.sha256(listing.encode()).hexdigest()


def source_checks() -> tuple[dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    problems: list = []

    ok = True
    for rel, expected in FILE_SHA256.items():
        actual = sha256(HERE / rel)
        pinned[f"{SCHEME}/{rel}"] = actual
        if actual != expected:
            problems.append(f"sha256 mismatch {SCHEME}/{rel}: {actual}")
            ok = False
    found, actual = directory_digest(HERE / REF / SET)
    pinned[f"{SCHEME}/{REF}/{SET}/*"] = actual
    if (found, actual) != SET_DIGEST:
        problems.append(f"directory digest mismatch {SCHEME}/{REF}/{SET}: {found} files, {actual}")
        ok = False
    checks["source_sha256_pinned_files_match"] = ok
    if not ok:
        return checks, pinned, problems

    lines = (HERE / REF / SET / "sign.c").read_text(encoding="utf-8").splitlines()
    count = OPEN_TEXT.count("\n")
    whole = "\n".join(lines[OPEN_FIRST_LINE - 1:OPEN_FIRST_LINE - 1 + count]) + "\n" == OPEN_TEXT

    def cited(*numbers: int) -> bool:
        return whole and all(lines[number - 1] == CITED[number] for number in numbers)

    checks["source_crypto_sign_open_is_the_pinned_text_of_sign_c_130_162"] = whole
    checks["source_loop_index_is_unsigned_and_lengths_are_unsigned_long_long"] = cited(134, 137)
    checks["source_accept_loop_compares_the_index_with_64_bit_mlen"] = cited(143, 147, 148)
    checks["source_reject_loop_compares_the_index_with_64_bit_smlen"] = cited(157, 158)
    # `i` is declared once and is not converted anywhere in the function.
    checks["source_index_is_declared_once_and_has_no_other_use"] = (
        OPEN_TEXT.count("unsigned i;") == 1 and OPEN_TEXT.count("[i]") == 3
        and OPEN_TEXT.count("++i") == 2 and OPEN_TEXT.count("size_t i") == 0)
    copies = sorted((HERE / REF).glob("*/sign.c")) + sorted((HERE / OPT).glob("*/*/sign.c"))
    checks["source_all_60_copies_of_sign_c_are_identical"] = (
        len(copies) == SIGN_C_COPIES and all(sha256(path) == SIGN_C for path in copies))
    return checks, pinned, problems


# --------------------------------------------------------------------------
# Tools, build and run
# --------------------------------------------------------------------------

def available_memory():
    """MemAvailable in bytes, or None if it cannot be read."""
    try:
        for row in Path("/proc/meminfo").read_text(encoding="utf-8").splitlines():
            if row.startswith("MemAvailable:"):
                return int(row.split()[1]) * 1024
    except (OSError, ValueError, IndexError):
        pass
    return None


def find_tools(work: Path) -> dict:
    if sys.maxsize < TWO_32:
        raise Skip("a 64-bit host is needed for inputs of 2^32 bytes")
    compiler = shutil.which("gcc") or shutil.which("cc")
    if not compiler:
        raise Skip("no C compiler (gcc or cc) found")
    (work / "probe.c").write_text(PROBE_C, encoding="utf-8")
    try:
        built = run([compiler, "-std=c99", "-O1", work / "probe.c", "-lcrypto", "-o", work / "probe"],
                    work, 120)
        probe = run([work / "probe"], work, 60) if built.returncode == 0 else None
    except (OSError, subprocess.TimeoutExpired) as error:
        raise Skip(f"the C compiler or its output cannot be run: {error}") from error
    if probe is None:
        raise Skip(f"cannot build an OpenSSL program: {built.stderr.strip()[-300:]}")
    rows = probe.stdout.splitlines()
    if probe.returncode or rows[:1] != ["probe 1"]:
        raise Skip("the OpenSSL SHAKE256 probe does not run")
    version = run([compiler, "--version"], work, 60).stdout.splitlines()
    return {"cc": compiler, "compiler": version[0].strip() if version else "",
            "libcrypto": rows[1].strip() if len(rows) > 1 else ""}


def build(tools: dict, work: Path, widened: bool) -> Path:
    set_dir = HERE / REF / SET
    nistkat = HERE / REF / "nistkat"
    sources = sorted(set_dir.glob("*.c"))
    binary = work / ("open_driver_copy" if widened else "open_driver")
    if widened:
        # Patched copy in the temporary directory; the submission is not touched.
        text = (set_dir / "sign.c").read_text(encoding="utf-8")
        if text.count(COPY_OLD) != 1:
            raise RuntimeError("anchor line does not occur exactly once in sign.c")
        copy = work / "copy_sign.c"
        copy.write_text(text.replace(COPY_OLD, COPY_NEW), encoding="utf-8")
        sources = [copy if path.name == "sign.c" else path for path in sources]
    cmd = [tools["cc"], "-O3", "-std=c99", "-fno-omit-frame-pointer", "-D_NIST_KAT_",
           f"-I{set_dir}", f"-I{nistkat}", *sources, nistkat / "rng.c",
           work / "driver.c", "-lcrypto", "-o", binary]
    built = run(cmd, work, 600)
    if built.returncode:
        raise RuntimeError(f"build failed ({binary.name}): {built.stderr[-800:]}")
    return binary


def fields(text: str) -> dict:
    return dict(row.split("=", 1) for row in text.splitlines() if "=" in row)


def child(work: Path, binary: Path, tag: str, mode: str, length: int, limit: float) -> dict:
    """Run the driver; stop it `limit` seconds after it has entered crypto_sign_open."""
    out_path, err_path = work / f"{tag}.out", work / f"{tag}.err"
    with open(out_path, "wb") as out, open(err_path, "wb") as err:
        # The last argument is the driver's own limit, a safety net only.
        proc = subprocess.Popen([str(binary), mode, str(length), str(int(limit) + 30)], cwd=work,
                                stdout=out, stderr=err, env=dict(os.environ, LC_ALL="C"))
        started = time.monotonic()
        entered = None      # when the script saw the child enter crypto_sign_open
        waited = None
        stopped = still_running = False
        try:
            while proc.poll() is None:
                current = time.monotonic()
                if entered is None and "calling_open=1\n" in out_path.read_text(encoding="utf-8"):
                    entered = current
                if entered is not None and current - entered >= limit:
                    waited = current - entered
                    stopped = True
                elif current - started >= limit + 2 * CONTROL_CAP:
                    stopped = True      # stuck before the call: reported as a problem
                if stopped:
                    still_running = proc.poll() is None
                    break
                time.sleep(0.02)
        finally:
            if proc.poll() is None:
                proc.kill()
            proc.wait()
    values = fields(out_path.read_text(encoding="utf-8"))
    marks = [fields(row.replace(" ", "\n")) for row in err_path.read_text(encoding="utf-8").splitlines()
             if row.startswith("rewrite=")]
    code = proc.returncode
    if not stopped and code == 77:
        raise Skip(f"the driver could not allocate its buffers ({mode} {length})")
    if not stopped and code == 78:
        raise Skip("unsigned is not 32 bits wide or size_t is narrower than 64 bits on this host")
    if not stopped and code == -signal.SIGKILL:
        raise Skip(f"the driver was killed, probably for lack of memory ({mode} {length})")
    if (not stopped and code != 0) or (stopped and waited is None):
        raise RuntimeError(f"driver failed ({mode} {length}): status {code}, output {values}")
    result = {
        "values": values,
        "returned": values.get("returned") == "1",
        "stopped_by_script_while_running": stopped and still_running,
        "waited": waited,       # seconds between the entry into crypto_sign_open and the stop
        "rewrites_of_m0": int(values["rewrites"]) if "rewrites" in values else len(marks),
        "values_written_to_m0": sorted({int(mark["value"]) for mark in marks}),
    }
    if marks and not result["returned"]:
        result["seconds_of_last_rewrite"] = round(int(marks[-1]["tick"]) / 10, 1)
    return result


def number(entry: dict, key: str):
    value = entry["values"].get(key)
    if value is None:
        return None
    return float(value) if "." in value else int(value)


def timeout_for(control_seconds: float) -> float:
    return max(MIN_TIMEOUT, TIMEOUT_RATIO * control_seconds)


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

    m = result["measurements"]
    c = result["checks"]
    memory = available_memory()
    m["available_memory_gib"] = round(memory / GIB, 1) if memory is not None else None
    accept_note = None
    runs: dict = {}
    with tempfile.TemporaryDirectory(prefix="uov-7-") as temporary:
        work = Path(temporary)
        try:
            if memory is not None and memory < REJECT_MEMORY:
                raise Skip(f"about 4 GiB of free memory is needed, {memory / GIB:.1f} GiB is available")
            tools = find_tools(work)
            (work / "driver.c").write_text(DRIVER_C, encoding="utf-8")
            with ThreadPoolExecutor(max_workers=2) as pool:
                binary, copy_binary = pool.map(lambda widened: build(tools, work, widened), (False, True))
            m["compiler"] = tools["compiler"]
            m["libcrypto"] = tools["libcrypto"]
            m["build"] = ("<cc> -O3 -std=c99 -fno-omit-frame-pointer -D_NIST_KAT_ -I<Ip> -I<nistkat> "
                          "<Ip>/*.c <nistkat>/rng.c driver.c (embedded) -lcrypto")
            m["control_copy"] = ("the same build with a temporary copy of sign.c whose line 137 reads "
                                 "'unsigned long long i;'")
            m["timeout_rule"] = (f"max({MIN_TIMEOUT:.0f} s, {TIMEOUT_RATIO:.0f} x control call time), "
                                 "counted from the entry into crypto_sign_open")

            # Reject path, one run at a time (about 4 GiB each).
            runs["reject_control"] = child(work, binary, "reject_control", "open", TWO_32 - 1, CONTROL_CAP)
            control_seconds = number(runs["reject_control"], "open_seconds")
            if not runs["reject_control"]["returned"] or control_seconds is None:
                raise RuntimeError(f"the control call did not return within {CONTROL_CAP:.0f} s")
            runs["reject"] = child(work, binary, "reject", "verify-open", TWO_32, timeout_for(control_seconds))
            runs["reject_copy"] = child(work, copy_binary, "reject_copy", "open", TWO_32, CONTROL_CAP)

            # Accept path (about 8 GiB each), if memory and time allow.
            predicted = (time.monotonic() - started) + 3 * control_seconds + 1.1 * timeout_for(control_seconds) + 10
            if memory is not None and memory < ACCEPT_MEMORY:
                accept_note = f"not run: about 8 GiB of free memory is needed, {memory / GIB:.1f} GiB is available"
            elif predicted > BUDGET:
                accept_note = (f"not run: the predicted total run time of {predicted:.0f} s is above "
                               f"the budget of {BUDGET:.0f} s")
            else:
                try:
                    runs["accept_control"] = child(work, binary, "accept_control", "sign-open",
                                                   TWO_32 - 1, CONTROL_CAP)
                    accept_seconds = number(runs["accept_control"], "open_seconds")
                    if not runs["accept_control"]["returned"] or accept_seconds is None:
                        raise RuntimeError(f"the accept-path control did not return within {CONTROL_CAP:.0f} s")
                    runs["accept"] = child(work, binary, "accept", "sign-open", TWO_32,
                                           timeout_for(accept_seconds))
                    accept_note = "run"
                except Skip as reason:
                    runs.pop("accept_control", None)
                    accept_note = f"not run: {reason}"
        except Skip as reason:
            result["status"] = "SKIP"
            result["skip_reason"] = str(reason)
            return emit(result, args.output)
        except (RuntimeError, subprocess.TimeoutExpired, OSError, KeyError, ValueError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    control, reject = runs["reject_control"], runs["reject"]
    first = control["values"]
    m["parameter_set"] = first.get("name")
    m["signature_bytes"] = number(control, "signature_bytes")
    m["sizeof"] = {"unsigned": number(control, "sizeof_unsigned"),
                   "unsigned_long_long": number(control, "sizeof_unsigned_long_long"),
                   "size_t": number(control, "sizeof_size_t")}
    m["marker"] = (f"m[0] is set to 0x{MARK:02x} before the call and looked at every 100 ms; a rewrite is "
                   "counted, and the marker restored, each time it has been overwritten")
    m["review_values"] = {"build": "reference Ip",
                          "open_of_2^32_minus_1_zero_bytes": "rejected in about 15 s",
                          "open_of_2^32_zero_bytes": "had not returned after 120 s"}

    def describe(entry: dict, input_bytes: int, control_seconds_=None) -> dict:
        out = {"input_bytes": input_bytes}
        for key in ("sign_rc", "sign_seconds", "verify_rc", "verify_seconds", "open_rc", "open_mlen",
                    "open_seconds", "output_matches"):
            if key in entry["values"]:
                out[key] = number(entry, key)
        out["crypto_sign_open_returned"] = entry["returned"]
        if control_seconds_ is not None:
            out["timeout_seconds"] = round(timeout_for(control_seconds_), 1)
            waited = entry["waited"]
            out["seconds_waited_in_crypto_sign_open"] = round(waited, 1) if waited is not None else None
            out["waited_over_control_call_time"] = (
                round(waited / control_seconds_, 1) if waited is not None and control_seconds_ > 0 else None)
            out["stopped_by_script_while_running"] = entry["stopped_by_script_while_running"]
            if "seconds_of_last_rewrite" in entry:
                out["seconds_of_last_rewrite"] = entry["seconds_of_last_rewrite"]
        out["rewrites_of_m0"] = entry["rewrites_of_m0"]
        out["values_written_to_m0"] = entry["values_written_to_m0"]
        return out

    def hangs(entry: dict, control_seconds_: float) -> bool:
        waited = entry["waited"]
        return (not entry["returned"] and entry["stopped_by_script_while_running"]
                and waited is not None and waited >= MIN_TIMEOUT
                and waited >= TIMEOUT_RATIO * control_seconds_ and "open_rc" not in entry["values"])

    copy = runs["reject_copy"]
    m["reject_path"] = {
        "control_open_of_4294967295_zero_bytes": describe(control, TWO_32 - 1),
        "open_of_4294967296_zero_bytes": describe(reject, TWO_32, control_seconds),
        "control_copy_with_unsigned_long_long_index_open_of_4294967296_zero_bytes": describe(copy, TWO_32),
    }
    c["host_unsigned_is_32_bits_and_lengths_are_64_bits"] = m["sizeof"] == {
        "unsigned": 4, "unsigned_long_long": 8, "size_t": 8}
    c["control_open_of_4294967295_zero_bytes_returns_minus_1"] = (
        control["returned"] and number(control, "open_rc") == -1 and number(control, "open_mlen") == 0)
    c["control_single_pass_writes_m0_once"] = (
        control["rewrites_of_m0"] == 1 and number(control, "last_value") == 0)
    c["control_verify_of_the_4294967296_zero_bytes_returns_minus_1"] = number(reject, "verify_rc") == -1
    c["open_of_4294967296_zero_bytes_does_not_return_within_the_timeout"] = hangs(reject, control_seconds)
    c["open_of_4294967296_zero_bytes_rewrites_m0_again_and_again"] = (
        not reject["returned"] and reject["rewrites_of_m0"] >= 2 and reject["values_written_to_m0"] == [0])
    c["control_copy_with_unsigned_long_long_index_rejects_the_4294967296_zero_bytes"] = (
        copy["returned"] and number(copy, "open_rc") == -1 and number(copy, "open_mlen") == 0
        and copy["rewrites_of_m0"] == 1 and number(copy, "last_value") == 0)

    m["accept_path_status"] = accept_note
    if accept_note == "run":
        accept_control, accept = runs["accept_control"], runs["accept"]
        accept_seconds = number(accept_control, "open_seconds")
        m["accept_path"] = {
            "control_open_of_signed_4294967295_byte_message": describe(
                accept_control, TWO_32 - 1 + m["signature_bytes"]),
            "open_of_signed_4294967296_byte_message": describe(
                accept, TWO_32 + m["signature_bytes"], accept_seconds),
        }
        c["control_signed_message_of_4294967295_bytes_opens_correctly"] = (
            accept_control["returned"] and number(accept_control, "sign_rc") == 0
            and number(accept_control, "smlen") == TWO_32 - 1 + m["signature_bytes"]
            and number(accept_control, "open_rc") == 0
            and number(accept_control, "open_mlen") == TWO_32 - 1
            and number(accept_control, "output_matches") == 1)
        c["control_single_pass_writes_m0_once_with_the_message_byte"] = (
            accept_control["rewrites_of_m0"] == 1 and number(accept_control, "last_value") == 1)
        c["open_of_signed_4294967296_byte_message_does_not_return_within_the_timeout"] = (
            number(accept, "sign_rc") == 0 and number(accept, "smlen") == TWO_32 + m["signature_bytes"]
            and hangs(accept, accept_seconds))
        c["open_of_signed_4294967296_byte_message_rewrites_m0_with_the_message_byte"] = (
            not accept["returned"] and accept["rewrites_of_m0"] >= 2 and accept["values_written_to_m0"] == [1])
    else:
        result["evidence"] = EVIDENCE_WITHOUT_ACCEPT_PATH

    result["status"] = "PASS" if all(c.values()) and not problems else "FAIL"
    print(f"elapsed {time.monotonic() - started:.1f} s", file=sys.stderr)
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
