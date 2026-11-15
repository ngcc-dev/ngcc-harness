#!/usr/bin/env python3
"""Reproduce MQOM-4: the opt-in GF(256) table multipliers of MQOM v3 index
their tables with secret field elements.

The submitted reference field back end (FIELDS_REF) has two optional GF(256)
multipliers, selected with -DUSE_GF256_TABLE_MULT (one 65,536-byte table
indexed by both operands) or -DUSE_GF256_TABLE_LOG_EXP (log, exp and zero-mask
tables).  Neither is a default.  This script builds one parameter set three
times (default multiplier, and each table multiplier) from the pristine
submitted sources and runs one key generation and one signature of each build
under Valgrind memcheck with the secret inputs marked undefined:

  keygen: seed_x (first randombytes request);
  sign:   the x part of sk and mseed (first randombytes request).

Released values are defined: mseed_eq and the salt (copied into pk and the
signature), every output after its operation returns, and sig_id at the point
where signing has computed it (it is the first field of the signature and
drives the public grinding loop).  That last declassification is one line
added to a temporary copy of sign.c; nothing under submission/ is modified.

memcheck then reports every use of a secret-derived value as an address or in
a branch.  PASS requires such address uses inside fields/fields_ref.h for both
table builds, in key generation and in signing, and none at all for the
default-multiplier build (the control), whose outputs are nevertheless
secret-derived.

This is taint tracking of data flow.  It is not a timing or cache measurement
and no key is recovered.  Python standard library only; needs gcc and
Valgrind with its development header (valgrind/memcheck.h).
"""

import argparse
import concurrent.futures
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


HERE = Path(__file__).resolve().parent
PACKAGE = HERE / "submission/submission_package_v3"
SET = "mqom3_cat1_gf16_short_ct"
SOURCE_DIR = PACKAGE / "Reference_Implementation" / SET

CANDIDATE = "MQOM-4"
EVIDENCE = (
    "Valgrind memcheck taint tracking of one key generation and one signature of "
    "mqom3_cat1_gf16_short_ct shows secret-derived values used as load addresses "
    "inside the two opt-in GF(256) table multipliers (-DUSE_GF256_TABLE_MULT, "
    "-DUSE_GF256_TABLE_LOG_EXP; FIELDS_REF only) and none with the default "
    "multiplier; this is data-flow evidence only, with no timing or cache "
    "measurement and no key recovery, and neither back end is a default."
)

# Files the claim rests on.  The same bytes are expected in all 18 reference
# and all 18 optimized parameter-set directories.
SOURCE_SHA256 = {
    "fields.h": "ce727a8328883e9b1ddb9a13b0f76861ef209eb40dcd04465853b3245ed00bfa",
    "fields/fields_ref.h": "f5c63730af8fff18922de8dcec9976d488cbe5d36c879873d0bdd0ead5bc11fc",
    "fields/gf256_mult_table.h": "94797d881fa84a7822bc7bf8dcac829cdf54a0e40068690588453cd3e31eb15e",
    "keygen.c": "fa3de73f67dc9a693bd5272fcb708c24480e6e1d265bf0623914aa888e1469d9",
    "sign.c": "c39565e40a87bbef179bb5b8eaef7bfed4f6c1526499bd1f550ab0451ff6b04c",
    "crypto_sign.c": "4ad08581020743fbcfc9ef3b26c969585160991e2b7da584a15c5c56c03dbd62",
}

# Explicit source list (the submitted Makefile is not run).  It is the list the
# repository harness compiles, without generator/rng.c: the driver below
# supplies randombytes().  sign.c is replaced by the patched temporary copy.
SOURCES = """
xof.c prg.c seed_expand.c expand_mq.c keygen.c sign_memopt.c sign_pre.c
crypto_sign.c verify_stream_ot.c verify_stream_ct.c
rijndael/rijndael_ref.c rijndael/rijndael_table.c rijndael/rijndael_aes_ni.c
rijndael/rijndael_vaes.c rijndael/rijndael_ct64.c rijndael/rijndael_external.c
rijndael/rijndael_arm_aes.c
ggm_tree/ggm_tree_small.c ggm_tree/ggm_tree_small_incr.c
ggm_tree/ggm_tree_small_incr_batch.c ggm_tree/ggm_tree_large_common_impl.c
ggm_tree/ggm_tree_large_dfs.c ggm_tree/ggm_tree_large_incr.c
ggm_tree/ggm_tree_large_incr_batch.c ggm_tree/ggm_tree_large_bfs.c
blc/blc_ot_default.c blc/blc_ot_memopt.c blc/blc_ct_default.c blc/blc_ct_memopt.c
piop/piop_default.c piop/piop_memopt.c piop/piop_bitslice.c
sha3/KeccakHash.c sha3/KeccakHashtimes4.c sha3/KeccakSponge.c
sha3/KeccakSpongetimes4.c sha3/opt64/KeccakP-1600-opt64.c
sha3/opt64/KeccakP-1600-times4-on1.c
""".split()
INCLUDE_DIRS = ["", "sha3", "sha3/opt64", "rijndael", "ggm_tree", "blc", "piop",
                "fields", "fields_bitsliced"]

# FIELDS_REF is the back end that contains the table options.  The bitsliced
# Rijndael is selected so that the table cipher of MQOM-1 does not add its own
# secret-indexed loads to the measurement.  valgrind/memcheck.h (client-request
# macros only) is force-included so that the line added to sign.c compiles.
COMMON_FLAGS = [
    "-O2", "-g", "-std=gnu11", "-w", "-DNDEBUG", "-DNO_MISALIGNED_ACCESSES",
    '-DKECCAK_PLATFORM="opt64"',
    "-DMQOM3_PARAM_SECURITY=128", "-DMQOM3_PARAM_BASE_FIELD=4",
    "-DMQOM3_PARAM_TRADEOFF=1", "-DMQOM3_PARAM_OT_VARIANT=0",
    "-DFIELDS_REF", "-DRIJNDAEL_BITSLICE",
    "-include", "valgrind/memcheck.h",
]
BUILDS = {
    "default": [],
    "table_mult": ["-DUSE_GF256_TABLE_MULT"],
    "log_exp": ["-DUSE_GF256_TABLE_LOG_EXP"],
}
MODES = ("keygen", "sign")

# The one line of test instrumentation, added to a temporary copy of sign.c.
SIGN_ANCHOR = "\tret = xof_squeeze(xof_ctx, sig_id, MQOM3_PARAM_DIGEST_SIZE);\n"
SIGN_INSERT = (
    "\tVALGRIND_MAKE_MEM_DEFINED(sig_id, MQOM3_PARAM_DIGEST_SIZE);"
    " /* reproducer: sig_id is released as the first field of the signature */\n"
)

DRIVER = r"""
/* Taint driver for reproduce_mqom_4.py.  Not part of the submission. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <valgrind/memcheck.h>
#include "api.h"

static uint64_t rng_state;
static unsigned rng_call;
static unsigned rng_secret_mask;   /* bit i set: request i returns a secret */

static uint8_t rng_byte(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint8_t)(rng_state >> 24);
}

/* Replaces generator/rng.c.  Deterministic; marks secret requests undefined. */
int randombytes(unsigned char *x, unsigned long long n) {
    unsigned long long i;
    for (i = 0; i < n; i++) {
        x[i] = rng_byte();
    }
    if ((rng_secret_mask >> rng_call) & 1u) {
        VALGRIND_MAKE_MEM_UNDEFINED(x, n);
    }
    rng_call++;
    return 0;
}

/* Number of bytes with at least one undefined bit (reads shadow state only). */
static long undefined_bytes(const unsigned char *p, size_t n) {
    unsigned char *v = calloc(n, 1);
    long count = 0;
    size_t i;
    if (v == NULL || VALGRIND_GET_VBITS(p, v, n) != 1) {
        free(v);
        return -1;
    }
    for (i = 0; i < n; i++) {
        count += (v[i] != 0);
    }
    free(v);
    return count;
}

int main(int argc, char **argv) {
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    static const unsigned char msg[33] = "MQOM-4 taint tracking message";
    const size_t mlen = sizeof msg;
    const size_t x_offset = CRYPTO_PUBLICKEYBYTES;   /* sk = pk || x */
    unsigned char *sm = calloc(mlen + CRYPTO_BYTES, 1);
    unsigned char *opened = calloc(mlen + CRYPTO_BYTES, 1);
    unsigned long long smlen = 0, olen = 0;
    unsigned long e0, e1, e2 = 0, e3 = 0;
    long u_pk, u_sk, u_sig = 0;
    int sign_mode, rc;

    if (argc != 2 || (strcmp(argv[1], "keygen") && strcmp(argv[1], "sign"))) {
        fprintf(stderr, "usage: %s keygen|sign\n", argv[0]);
        return 2;
    }
    if (sm == NULL || opened == NULL) {
        return 2;
    }
    sign_mode = !strcmp(argv[1], "sign");

    /* Key generation.  Request 0 is seed_x (secret), request 1 is mseed_eq
     * (public: copied into pk).  In sign mode the key is generated untainted. */
    rng_state = 0x0123456789abcdefULL;
    rng_call = 0;
    rng_secret_mask = sign_mode ? 0u : 1u;
    e0 = VALGRIND_COUNT_ERRORS;
    if (crypto_sign_keypair(pk, sk)) {
        puts("SE1 keypair failed");
        return 3;
    }
    rng_secret_mask = 0;
    e1 = VALGRIND_COUNT_ERRORS;
    u_pk = undefined_bytes(pk, sizeof pk);
    u_sk = undefined_bytes(sk, sizeof sk);
    VALGRIND_MAKE_MEM_DEFINED(pk, sizeof pk);
    VALGRIND_MAKE_MEM_DEFINED(sk, sizeof sk);
    e2 = e3 = e1;

    if (sign_mode) {
        /* Signing.  The x part of sk is secret.  Request 0 is mseed (secret),
         * request 1 is the salt (public: copied into the signature). */
        VALGRIND_MAKE_MEM_UNDEFINED(sk + x_offset, sizeof sk - x_offset);
        rng_call = 0;
        rng_secret_mask = 1u;
        rc = crypto_sign(sm, &smlen, msg, mlen, sk);
        rng_secret_mask = 0;
        e2 = VALGRIND_COUNT_ERRORS;
        u_sig = undefined_bytes(sm, mlen + CRYPTO_BYTES);
        VALGRIND_MAKE_MEM_DEFINED(&rc, sizeof rc);
        VALGRIND_MAKE_MEM_DEFINED(&smlen, sizeof smlen);
        VALGRIND_MAKE_MEM_DEFINED(sm, mlen + CRYPTO_BYTES);
        VALGRIND_MAKE_MEM_DEFINED(sk, sizeof sk);
        if (rc) {
            printf("SE1 sign failed rc=%d\n", rc);
            return 4;
        }
        /* Verification sees released data only. */
        if (crypto_sign_open(opened, &olen, sm, smlen, pk) || olen != mlen
                || memcmp(opened, msg, mlen)) {
            puts("SE1 verify failed");
            return 5;
        }
        e3 = VALGRIND_COUNT_ERRORS;
    }
    printf("SE1 mode=%s valgrind=%d keygen_errors=%lu sign_errors=%lu verify_errors=%lu "
           "undefined_pk_bytes=%ld undefined_sk_bytes=%ld undefined_sig_bytes=%ld "
           "pk_bytes=%ld sk_bytes=%ld sig_bytes=%ld ok=1\n",
           argv[1], (int)RUNNING_ON_VALGRIND, e1 - e0, e2 - e1, e3 - e2,
           u_pk, u_sk, u_sig, (long)CRYPTO_PUBLICKEYBYTES, (long)CRYPTO_SECRETKEYBYTES,
           (long)CRYPTO_BYTES);
    free(sm);
    free(opened);
    return 0;
}
"""

# What the review recorded (gcc 15.2 -O2, Valgrind 3.26).  Counts depend on the
# code the compiler emits, so they are reported for comparison and do not
# decide PASS.
REVIEW_COUNTS = {
    "table_mult": {"keygen": 70656, "sign": 852576},
    "log_exp": {"keygen": 141312, "sign": 1705140},
}
REVIEW_ENTRY_SITES = ["blc_convert.h:188", "blc_convert.h:195", "keygen.c:62",
                      "keygen.c:63", "piop_default.c:128", "piop_default.c:135"]

TAINT_ORIGIN = "Uninitialised value was created by a client request"
ORIGIN_UNKNOWN = "origin not named by memcheck"

FRAME = re.compile(r"^\s+(?:at|by) (0x[0-9A-Fa-f]+): (.+) \((.*)\)$")
CONTEXT = re.compile(r"^(\d+) errors in context \d+ of \d+:$")
SUMMARY = re.compile(r"^ERROR SUMMARY: (\d+) errors from (\d+) contexts")
# A report site counts as "inside the table multiplier" when its source line
# is a table load or a call of the (inlined) GF(256) multiplier.
MULTIPLIER_LINE = re.compile(
    r"gf256_mult_table\[|rijndael_log\[|rijndael_exp\[|zero_mask\[|\bgf(?:16_gf)?256_mult_ref\("
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, timeout):
    """Run a child process; never raises on failure or timeout."""
    try:
        done = subprocess.run(command, capture_output=True, text=True,
                              timeout=timeout, check=False)
        return done.returncode, done.stdout, done.stderr
    except subprocess.TimeoutExpired:
        return -999, "", "timeout"
    except OSError as error:
        return -998, "", str(error)


def static_checks():
    """Source facts the review cites; no compilation."""
    fields_h = (SOURCE_DIR / "fields.h").read_text().split("\n")
    fields_ref = (SOURCE_DIR / "fields/fields_ref.h").read_text().split("\n")
    table_h = (SOURCE_DIR / "fields/gf256_mult_table.h").read_text()

    def lines(text, first, last):
        return [line.strip() for line in text[first - 1:last]]

    pinned = {name: sha256(SOURCE_DIR / name) for name in SOURCE_SHA256}
    copies = sorted(PACKAGE.glob("*_Implementation/mqom3_cat*"))
    same_everywhere = len(copies) == 36 and all(
        sha256(copy / name) == SOURCE_SHA256[name]
        for copy in copies for name in SOURCE_SHA256
    )

    flag_files = sorted(
        str(path.relative_to(SOURCE_DIR))
        for path in SOURCE_DIR.rglob("*")
        if path.suffix in (".c", ".h", ".S", ".s", ".inc") and path.is_file()
        and "USE_GF256_TABLE_" in path.read_text(errors="replace")
    )
    defined_in_source = any(
        re.search(r"#\s*define\s+USE_GF256_TABLE_", (SOURCE_DIR / name).read_text())
        for name in flag_files
    )
    makefiles = [copy / "Makefile" for copy in copies]
    opt_in = bool(makefiles) and not defined_in_source and all(
        text.count("-DUSE_GF256_TABLE_MULT") == 1
        and text.count("-DUSE_GF256_TABLE_LOG_EXP") == 1
        and "ifeq ($(USE_GF256_TABLE_MULT),1)\n" in text
        and "ifeq ($(USE_GF256_TABLE_LOG_EXP),1)\n" in text
        and not re.search(r"^\s*USE_GF256_TABLE_(MULT|LOG_EXP)\s*[:?+]?=", text, re.M)
        for text in (path.read_text() for path in makefiles)
    )

    checks = {
        "sources_match_pinned_sha256": pinned == SOURCE_SHA256,
        "same_sources_in_all_36_set_directories": same_everywhere,
        "table_mult_indexed_by_both_operands_fields_ref_h_140_142": lines(fields_ref, 137, 142) == [
            "#if defined(USE_GF256_TABLE_MULT)",
            "/* If we are asked to, we use the 65 kB multiplication table */",
            '#include "gf256_mult_table.h"',
            "static inline uint8_t gf256_mult_ref(uint8_t x, uint8_t y) {",
            "return gf256_mult_table[((uint16_t)y << 8) | x];",
            "}",
        ],
        "mult_table_has_65536_entries": len(re.findall(r"\b0x[0-9a-fA-F]{2}\b", table_h)) == 65536,
        "log_exp_tables_indexed_by_operands_fields_ref_h_229_233": lines(fields_ref, 229, 234) == [
            "static inline uint8_t gf256_mult_ref(uint8_t x, uint8_t y) {",
            "uint8_t log_x = rijndael_log[x];",
            "uint8_t log_y = rijndael_log[y];",
            "uint8_t res = rijndael_exp[log_x + log_y] & zero_mask[x] & zero_mask[y];",
            "return res;",
            "}",
        ] and fields_ref[142].strip() == "#elif defined(USE_GF256_TABLE_LOG_EXP)",
        "default_multiplier_is_bitwise_fields_ref_h_235": lines(fields_ref, 235, 236) == [
            "#else",
            "/* 32-bit based multiplication */",
        ],
        "source_labels_fields_h_105_112": (
            lines(fields_h, 105, 106) == ["#if defined(USE_GF256_TABLE_MULT)",
                                          "#if defined(GF256_MULT_TABLE_SRAM)"]
            and "(NON constant time) 65 kB GF(256) mult table in SRAM" in fields_h[106]
            and "(NON constant time) 65 kB GF(256) mult table in flash/ROM" in fields_h[108]
            and fields_h[110].strip() == "#elif defined(USE_GF256_TABLE_LOG_EXP)"
            and "(constant time on embedded platforms without cache to SRAM) log/exp GF(256) mult tables"
            in fields_h[111]
        ),
        "table_options_exist_only_in_fields_ref_backend": (
            flag_files == ["fields.h", "fields/fields_ref.h"]
            and lines(fields_h, 103, 104) == ["#if defined(FIELDS_REF)", '#include "fields_ref.h"']
        ),
        "table_options_are_opt_in_never_set_by_source_or_36_makefiles": opt_in,
    }
    return checks, len(copies)


def parse_memcheck(text):
    """Return (total errors, contexts) from a memcheck log with its error list."""
    lines = [re.sub(r"^==\d+== ?", "", line) for line in text.splitlines()]
    total = None
    for line in lines:
        match = SUMMARY.match(line)
        if match:
            total = int(match.group(1))
    contexts = []
    index = 0
    while index < len(lines):
        match = CONTEXT.match(lines[index])
        if not match:
            index += 1
            continue
        context = {"count": int(match.group(1)), "what": lines[index + 1].strip(),
                   "frames": [], "origin": None, "origin_frames": []}
        index += 2
        target = context["frames"]
        while index < len(lines) and lines[index].strip():
            frame = FRAME.match(lines[index])
            if frame:
                target.append((frame.group(1), frame.group(2), frame.group(3)))
            else:
                context["origin"] = lines[index].strip()
                target = context["origin_frames"]
            index += 1
        contexts.append(context)
    return total, contexts


def summarize(contexts):
    """Classify memcheck contexts of one run."""
    kinds, sites, entries, origins = {}, {}, {}, {}
    all_in_fields_ref = True
    all_multiplier_lines = True
    fields_ref = (SOURCE_DIR / "fields/fields_ref.h").read_text().split("\n")
    for context in contexts:
        count = context["count"]
        what = re.sub(r" of size \d+$", "", context["what"])
        kinds[what] = kinds.get(what, 0) + count
        origin = context["origin"] or ORIGIN_UNKNOWN
        origins[origin] = origins.get(origin, 0) + count
        frames = context["frames"]
        if not frames:
            all_in_fields_ref = all_multiplier_lines = False
            continue
        _, function, location = frames[0]
        site = f"{function} ({location})"
        sites[site] = sites.get(site, 0) + count
        name, _, number = location.partition(":")
        if name != "fields_ref.h" or not number.isdigit():
            all_in_fields_ref = all_multiplier_lines = False
        elif not (0 < int(number) <= len(fields_ref)
                  and MULTIPLIER_LINE.search(fields_ref[int(number) - 1])):
            all_multiplier_lines = False
        # First frame outside the field arithmetic headers: where the secret
        # operand enters the multiplier.
        for _, _, where in frames:
            if where.partition(":")[0] not in ("fields_ref.h", "fields.h"):
                entries[where] = entries.get(where, 0) + count
                break
    return {
        "kinds": dict(sorted(kinds.items())),
        "sites": dict(sorted(sites.items())),
        "entry_sites": dict(sorted(entries.items())),
        "origins": dict(sorted(origins.items())),
        "all_sites_in_fields_ref_h": all_in_fields_ref,
        "all_site_lines_are_multiplier_lines": all_multiplier_lines,
    }


def emit(result, output):
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")
    return {"PASS": 0, "FAIL": 1, "SKIP": 77}[result["status"]]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": EVIDENCE,
        "parameter_set": SET,
        "source_directory": str(SOURCE_DIR.relative_to(HERE)),
        "checks": {},
        "measurements": {},
        "source_sha256": dict(SOURCE_SHA256),
        "status": "FAIL",
    }

    gcc = shutil.which("gcc")
    valgrind = shutil.which("valgrind")
    if not gcc or not valgrind:
        result["status"] = "SKIP"
        result["skip_reason"] = "gcc not found" if not gcc else "valgrind not found"
        return emit(result, args.output)

    try:
        checks, copies = static_checks()
    except (OSError, IndexError) as error:
        result["checks"]["submission_sources_readable"] = False
        result["error"] = str(error)
        return emit(result, args.output)
    result["checks"].update(checks)
    result["measurements"]["set_directories_compared"] = copies

    with tempfile.TemporaryDirectory(prefix="mqom_se1_") as scratch:
        work = Path(scratch)

        # Valgrind's client-request header is needed to mark and release data.
        probe = work / "probe.c"
        probe.write_text("#include <valgrind/memcheck.h>\nint main(void){return (int)RUNNING_ON_VALGRIND;}\n")
        code, _, _ = run([gcc, "-fsyntax-only", str(probe)], 60)
        if code != 0:
            result["status"] = "SKIP"
            result["skip_reason"] = "valgrind/memcheck.h not found"
            return emit(result, args.output)

        # Patched copy of sign.c: one declassification line after sig_id.
        pristine = (SOURCE_DIR / "sign.c").read_text()
        anchor_once = pristine.count(SIGN_ANCHOR) == 1
        result["checks"]["sign_c_anchor_occurs_exactly_once"] = anchor_once
        if not anchor_once:
            return emit(result, args.output)
        patched = work / "patched"
        patched.mkdir()
        (patched / "sign.c").write_text(pristine.replace(SIGN_ANCHOR, SIGN_ANCHOR + SIGN_INSERT))
        (work / "taint_driver.c").write_text(DRIVER)

        includes = [f"-I{SOURCE_DIR / sub}" if sub else f"-I{SOURCE_DIR}" for sub in INCLUDE_DIRS]
        sources = [str(SOURCE_DIR / name) for name in SOURCES]

        def build(tag):
            binary = work / f"se1_{tag}"
            command = ([gcc] + COMMON_FLAGS + BUILDS[tag] + includes + sources
                       + [str(patched / "sign.c"), str(work / "taint_driver.c"), "-o", str(binary)])
            code, _, err = run(command, 300)
            return tag, code, err, binary

        # The per-context error list is printed by -s (Valgrind 3.16 and later)
        # or by -v (earlier releases).
        version = run([valgrind, "--version"], 30)[1].strip()
        release = re.search(r"(\d+)\.(\d+)", version)
        listing = "-s" if release and (int(release.group(1)), int(release.group(2))) >= (3, 16) else "-v"

        def taint_run(job):
            tag, mode, binary = job
            log = work / f"memcheck_{tag}_{mode}.log"
            command = [valgrind, "--tool=memcheck", "--error-limit=no", "--num-callers=30",
                       "--track-origins=yes", "--leak-check=no", listing,
                       f"--log-file={log}", str(binary), mode]
            code, out, _ = run(command, 240)
            text = log.read_text(errors="replace") if log.exists() else ""
            return tag, mode, code, out, text

        with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
            built = {tag: (code, err, binary) for tag, code, err, binary in pool.map(build, BUILDS)}
        build_ok = all(code == 0 for code, _, _ in built.values())
        result["checks"]["three_builds_compile"] = build_ok
        if not build_ok:
            result["build_errors"] = {tag: err[-2000:] for tag, (code, err, _) in built.items() if code}
            return emit(result, args.output)

        jobs = [(tag, mode, built[tag][2]) for tag in BUILDS for mode in MODES]
        with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
            finished = list(pool.map(taint_run, jobs))

        compiler = run([gcc, "--version"], 30)[1].splitlines()
        result["measurements"]["tools"] = {"gcc": compiler[0] if compiler else "", "valgrind": version}

    runs = {}
    for tag, mode, code, out, text in finished:
        driver = {}
        for line in out.splitlines():
            if line.startswith("SE1 mode="):
                driver = dict(item.split("=", 1) for item in line.split()[1:])
        total, contexts = parse_memcheck(text)
        summary = summarize(contexts)
        phase = int(driver.get(f"{mode}_errors", -1))
        runs[(tag, mode)] = {
            "completed": code == 0 and driver.get("ok") == "1" and driver.get("valgrind") == "1",
            "errors": total,
            "contexts": len(contexts),
            "driver_phase_errors": phase,
            "driver_other_phase_errors": sum(
                int(driver.get(f"{other}_errors", -1))
                for other in ("keygen", "sign", "verify") if other != mode
            ),
            "context_error_sum": sum(context["count"] for context in contexts),
            "undefined_pk_bytes": int(driver.get("undefined_pk_bytes", -1)),
            "undefined_sk_bytes": int(driver.get("undefined_sk_bytes", -1)),
            "undefined_sig_bytes": int(driver.get("undefined_sig_bytes", -1)),
            "sig_bytes": int(driver.get("sig_bytes", -1)),
            **summary,
        }

    def address_uses(tag, mode):
        return runs[(tag, mode)]["kinds"].get("Use of uninitialised value", 0)

    table_runs = [runs[(tag, mode)] for tag in ("table_mult", "log_exp") for mode in MODES]
    control_runs = [runs[("default", mode)] for mode in MODES]
    result["checks"].update({
        "all_six_runs_complete_and_signatures_verify": all(r["completed"] for r in runs.values()),
        "memcheck_and_driver_error_counts_agree": all(
            r["errors"] is not None and r["errors"] == r["driver_phase_errors"] == r["context_error_sum"]
            and r["driver_other_phase_errors"] == 0
            for r in runs.values()
        ),
        "table_mult_keygen_uses_secret_as_address": address_uses("table_mult", "keygen") > 0,
        "table_mult_sign_uses_secret_as_address": address_uses("table_mult", "sign") > 0,
        "log_exp_keygen_uses_secret_as_address": address_uses("log_exp", "keygen") > 0,
        "log_exp_sign_uses_secret_as_address": address_uses("log_exp", "sign") > 0,
        "table_reports_are_addresses_none_is_a_branch": all(
            list(r["kinds"]) == ["Use of uninitialised value"] for r in table_runs
        ),
        "table_reports_all_located_in_fields_ref_h": all(r["all_sites_in_fields_ref_h"] for r in table_runs),
        "table_report_lines_are_table_loads_or_gf256_mult_calls": all(
            r["all_site_lines_are_multiplier_lines"] for r in table_runs
        ),
        # memcheck cannot name an origin for every report; what must not occur
        # is an origin other than the driver's marks (a stack or heap allocation
        # would mean ordinary uninitialised memory, not a tracked secret).
        "table_reports_have_no_origin_other_than_the_taint_marks": all(
            r["origins"].get(TAINT_ORIGIN, 0) > 0
            and set(r["origins"]) <= {TAINT_ORIGIN, ORIGIN_UNKNOWN}
            for r in table_runs
        ),
        "control_default_multiplier_reports_nothing": all(
            r["errors"] == 0 and r["contexts"] == 0 for r in control_runs
        ),
        "control_default_build_outputs_are_secret_derived": (
            runs[("default", "keygen")]["undefined_pk_bytes"] > 0
            and runs[("default", "keygen")]["undefined_sk_bytes"] > 0
            and runs[("default", "sign")]["undefined_sig_bytes"] > 0
        ),
    })

    measurements = result["measurements"]
    for (tag, mode), r in sorted(runs.items()):
        entry = {
            "memcheck_errors": r["errors"],
            "memcheck_contexts": r["contexts"],
            "report_kinds": r["kinds"],
        }
        if tag == "default":
            if mode == "keygen":
                entry["undefined_pk_bytes_before_release"] = r["undefined_pk_bytes"]
                entry["undefined_sk_bytes_before_release"] = r["undefined_sk_bytes"]
            else:
                entry["undefined_signature_bytes_before_release"] = r["undefined_sig_bytes"]
                entry["signature_bytes"] = r["sig_bytes"]
        else:
            entry["report_sites"] = r["sites"]
            entry["report_origins"] = r["origins"]
            entry["entry_sites"] = r["entry_sites"]
            entry["review_reported_errors"] = REVIEW_COUNTS[tag][mode]
            entry["equals_review_count"] = r["errors"] == REVIEW_COUNTS[tag][mode]
        measurements[f"{tag}_{mode}"] = entry
    observed_entries = sorted({site for r in table_runs for site in r["entry_sites"]})
    measurements["entry_sites_all_table_runs"] = observed_entries
    measurements["review_entry_sites"] = REVIEW_ENTRY_SITES
    measurements["review_entry_sites_all_observed"] = set(REVIEW_ENTRY_SITES) <= set(observed_entries)
    measurements["entry_sites_not_listed_in_review"] = sorted(
        set(observed_entries) - set(REVIEW_ENTRY_SITES)
    )
    measurements["executions_per_run"] = 1

    result["status"] = "PASS" if all(result["checks"].values()) else "FAIL"
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
