#!/usr/bin/env python3
"""FAEST-4: AVX2 crypto_sign_open accepts smlen < CRYPTO_BYTES unchecked.

api.hpp:67 of the AVX2 tree computes `smlen - FAEST_SIGNATURE_BYTES<P>` with
no prior length check, so a short input gives a message length near 2^64 and
a signature pointer in front of the caller's buffer.  The reference tree
rejects the same input at crypto_sign.c:34-37.

Both trees of faest_128f are built from the submitted sources and driven with
the same C program.  Each crypto_sign_open call runs in its own child process
with a timeout, on an all-zero input of exactly smlen bytes.

The first step of the AVX2 verifier tests the grinding bits of the claimed
signature, which for smlen >= 21 is the byte sm[smlen - 21] of the caller's
input.  The pass condition therefore uses the short lengths 5169 and 100,
whose outcome is fixed by the input.  smlen = 0, where that byte lies in front
of the buffer, and a short input with a grinding bit set are recorded as
measurements only.
"""

import argparse
import concurrent.futures
import hashlib
import json
import shutil
import signal
import subprocess
import tempfile
import time
from pathlib import Path


HERE = Path(__file__).resolve().parent
SUBMISSION = HERE / "submission/nist_submission_round3"
REFERENCE = SUBMISSION / "Reference_Implementation/faest_128f"
AVX2 = SUBMISSION / "Additional_Implementations/avx2/faest_128f"
AARCH64 = SUBMISSION / "Additional_Implementations/aarch64/faest_128f"
CANDIDATE = "FAEST-4"
JOBS = 3
TIMEOUT = 60
CRYPTO_BYTES = 5170
SHORT = (5169, 100)                 # outcome fixed by the all-zero input
GRIND_BYTE_FROM_END = 21            # sm[smlen - 21] holds the grinding bits

# -march=haswell -maes -mpclmul is the instruction set the AVX2 build uses here.
HOST_FLAGS = {"avx", "avx2", "bmi1", "bmi2", "fma", "movbe", "abm", "popcnt",
              "f16c", "rdrand", "fsgsbase", "aes", "pclmulqdq"}

SOURCE_SHA256 = {
    "Additional_Implementations/avx2/faest_128f/api.hpp":
        "907d80abf9cfec2868225820e34b2c4828f7dcde3a896724ac0fb4aba85f76da",
    "Additional_Implementations/avx2/faest_128f/api.cpp":
        "91fd1a577117bda6bc72820ae0c953e14b78df9435319763bc92796513946d23",
    "Additional_Implementations/avx2/faest_128f/faest.inc":
        "58086c874ca32b3f253a22869996166b82e737eb27b4a7c2bef6de3e8ec009fe",
    "Additional_Implementations/avx2/faest_128f/faest_sig.hpp":
        "842cf7b1ccb22e8b262a2a80df96b97aa774a03cacd89c5e96e835a2eb9eb796",
    "Additional_Implementations/avx2/faest_128f/Makefile":
        "5dfa36b6a2b79de3e9723dce6ab4732839ff3d24bd510bce65ce6ed9371f05eb",
    "Additional_Implementations/aarch64/faest_128f/api.hpp":
        "907d80abf9cfec2868225820e34b2c4828f7dcde3a896724ac0fb4aba85f76da",
    "Reference_Implementation/faest_128f/crypto_sign.c":
        "8027cf72b17c85ab5bb964644897aed8d2e86e08d446a627d721428dfc460c0a",
    "Reference_Implementation/faest_128f/Makefile":
        "336d58a7e5953936673cd55c4f35da254a13c3cde1ce6e8835695cd2e74b0013",
}

# Build prerequisites that are not executables: probed, SKIP when absent.
OPENSSL_PROBE_C = """\
#include <openssl/evp.h>
int main(void) { return EVP_aes_128_ecb() == NULL; }
"""
CXX23_PROBE = """\
int main() { [[assume(true)]]; return 0; }
"""

DRIVER_C = r"""
/* Driver for reproduce_faest_4.py; linked against either tree. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

/* Deterministic randombytes for both builds (splitmix64). */
static uint64_t rng_state = UINT64_C(0x5E0FAE5700000002);
void randombytes(unsigned char* x, unsigned long long xlen) {
  for (unsigned long long i = 0; i < xlen; i++) {
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    x[i] = (unsigned char)((z ^ (z >> 31)) & 0xff);
  }
}

static const unsigned char MSG[8] = "message";

int main(int argc, char** argv) {
  static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  unsigned long long mlen = 0;
  if (argc < 2) return 2;
  if (crypto_sign_keypair(pk, sk)) return 3;

  if (!strcmp(argv[1], "roundtrip")) {
    unsigned char* sm = malloc(sizeof MSG + CRYPTO_BYTES);
    unsigned char* m = malloc(sizeof MSG + CRYPTO_BYTES);
    unsigned long long smlen = 0;
    if (!sm || !m) return 3;
    int sign_rc = crypto_sign(sm, &smlen, MSG, sizeof MSG, sk);
    int rc = crypto_sign_open(m, &mlen, sm, smlen, pk);
    printf("crypto_bytes=%d\nsign_rc=%d\nrc=%d\nmessage_ok=%d\n", CRYPTO_BYTES, sign_rc, rc,
           rc == 0 && mlen == sizeof MSG && memcmp(m, MSG, sizeof MSG) == 0);
    return 0;
  }

  /* open <smlen> [<index> <value>]: zero input of exactly smlen bytes,
     optionally with one byte set. */
  if (strcmp(argv[1], "open") || argc < 3) return 2;
  unsigned long long smlen = strtoull(argv[2], 0, 0);
  unsigned char* sm = calloc(1, smlen ? smlen : 1);
  unsigned char* m = malloc(CRYPTO_BYTES + 64);
  if (!sm || !m) return 3;
  if (argc >= 5) sm[strtoull(argv[3], 0, 0)] = (unsigned char)strtoul(argv[4], 0, 0);
  printf("calling=1\n");
  fflush(stdout);
  int rc = crypto_sign_open(m, &mlen, sm, smlen, pk);
  printf("rc=%d\n", rc);
  return 0;
}
"""


class Skip(Exception):
    pass


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    return subprocess.run(command, text=True, capture_output=True, **kwargs)


def compile_many(jobs):
    """jobs: list of (command, object).  Explicit compiler command lines."""
    def one(job):
        command, _ = job
        result = run(command)
        if result.returncode:
            raise RuntimeError(f"compile failed: {' '.join(command[-4:])}\n"
                               f"{result.stderr[-2000:]}")
    with concurrent.futures.ThreadPoolExecutor(max_workers=JOBS) as pool:
        list(pool.map(one, jobs))


def build_reference(gcc, tmp):
    out = tmp / "reference"
    out.mkdir()
    # CPPFLAGS of the submitted Makefile, plus -DHAVE_RANDOMBYTES (its KAT target).
    flags = ["-O2", "-std=gnu11", f"-I{REFERENCE}", f"-I{REFERENCE / 'sha3'}",
             "-DNDEBUG", "-DHAVE_OPENSSL", "-DFAEST_EXPORT=", "-DHAVE_RANDOMBYTES"]
    sources = sorted(REFERENCE.glob("*.c")) + sorted((REFERENCE / "sha3").glob("*.c"))
    sources.append(tmp / "driver.c")
    jobs = []
    for source in sources:
        obj = out / f"{source.parent.name}_{source.stem}.o"
        jobs.append(([gcc, *flags, "-c", str(source), "-o", str(obj)], obj))
    compile_many(jobs)
    exe = out / "driver"
    link = run([gcc, *[str(obj) for _, obj in jobs], "-o", str(exe), "-lcrypto"])
    if link.returncode:
        raise RuntimeError(f"link failed: {link.stderr[-2000:]}")
    return exe


def build_avx2(gcc, gxx, tmp):
    out = tmp / "avx2"
    out.mkdir()
    # CPPFLAGS of the submitted Makefile: -I. -Isha3 -DHAVE_OPENSSL (asserts stay
    # enabled, as there).  randomness_randombytes.c is what its KAT target links.
    # The generic sources instantiate templates for parameter sets whose tables
    # this directory does not define; the submitted Makefile drops them with
    # -flto, here the linker drops the unreferenced sections.
    arch = ["-O2", "-march=haswell", "-maes", "-mpclmul",
            "-ffunction-sections", "-fdata-sections"]
    cpp = [f"-I{AVX2}", f"-I{AVX2 / 'sha3'}", "-DHAVE_OPENSSL"]
    c_sources = [AVX2 / "randomness_randombytes.c", tmp / "driver.c"]
    c_sources += sorted((AVX2 / "sha3").glob("*.c"))
    cxx_sources = sorted(p for p in AVX2.glob("*.cpp") if p.name != "print_parameters.cpp")
    jobs = []
    for source in cxx_sources:
        obj = out / f"cxx_{source.stem}.o"
        jobs.append(([gxx, *arch, "-std=c++23", *cpp, "-c", str(source), "-o", str(obj)], obj))
    for source in c_sources:
        obj = out / f"c_{source.stem}.o"
        jobs.append(([gcc, *arch, "-std=gnu2x", *cpp, "-c", str(source), "-o", str(obj)], obj))
    for source in sorted((AVX2 / "sha3").glob("*.s")):
        obj = out / f"s_{source.stem}.o"
        jobs.append(([gcc, "-c", str(source), "-o", str(obj)], obj))
    compile_many(jobs)
    exe = out / "driver"
    link = run([gxx, *[str(obj) for _, obj in jobs], "-o", str(exe),
                "-no-pie", "-Wl,--gc-sections"])
    if link.returncode:
        raise RuntimeError(f"link failed: {link.stderr[-2000:]}")
    return exe


def have_openssl(gcc, tmp):
    (tmp / "openssl_probe.c").write_text(OPENSSL_PROBE_C)
    return run([gcc, str(tmp / "openssl_probe.c"), "-o", str(tmp / "openssl_probe"),
                "-lcrypto"]).returncode == 0


def have_cxx23(gxx, tmp):
    (tmp / "cxx23_probe.cpp").write_text(CXX23_PROBE)
    return run([gxx, "-std=c++23", "-Werror", "-c", str(tmp / "cxx23_probe.cpp"),
                "-o", str(tmp / "cxx23_probe.o")]).returncode == 0


def fields(text):
    out = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            out[key.strip()] = value.strip()
    return out


def call(exe, *args):
    """One child process.  Returns a description of how it ended."""
    try:
        result = run([str(exe), *map(str, args)], timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return {"timeout": True, "signal": None, "exit": None, "rc": None, "out": {}}
    out = fields(result.stdout)
    code = result.returncode
    name = None
    if code < 0:
        try:
            name = signal.Signals(-code).name
        except ValueError:
            name = f"signal {-code}"
    return {"timeout": False, "signal": name, "exit": code if code >= 0 else None,
            "rc": int(out["rc"]) if "rc" in out else None, "out": out}


def returned(result, value):
    return (not result["timeout"] and result["signal"] is None
            and result["exit"] == 0 and result["rc"] == value)


def killed(result):
    """Reached crypto_sign_open, did not return from it, died on a signal."""
    return (not result["timeout"] and result["signal"] is not None
            and result["out"].get("calling") == "1" and result["rc"] is None)


def summary(result):
    if result["timeout"]:
        return "timeout"
    if result["signal"]:
        return result["signal"]
    if result["rc"] is not None:
        return f"rc={result['rc']}"
    return f"exit {result['exit']}"


def source_checks():
    api = (AVX2 / "api.hpp").read_text().splitlines()
    open_body = api[63:74]                         # api.hpp:64-74
    ref = (REFERENCE / "crypto_sign.c").read_text().splitlines()
    return {
        "source_avx2_api_hpp_67_subtracts_unchecked":
            api[66].strip() == "unsigned long long m_length = smlen - FAEST_SIGNATURE_BYTES<P>;"
            and "crypto_sign_open" in api[63]
            and sum("smlen" in line for line in open_body) == 2,
        "source_reference_crypto_sign_c_34_checks_length":
            ref[33].strip() == "if (smlen < FAEST_128F_SIGNATURE_SIZE) {"
            and ref[35].strip() == "return -1;",
        "source_aarch64_api_hpp_identical_to_avx2":
            (AARCH64 / "api.hpp").read_bytes() == (AVX2 / "api.hpp").read_bytes(),
    }


def host_flags():
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("flags"):
                return set(line.split(":", 1)[1].split())
    except OSError:
        pass
    return set()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path, help="also write the JSON here")
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "Shows that the AVX2 faest_128f crypto_sign_open is killed by a signal on "
            "all-zero inputs shorter than CRYPTO_BYTES, where the reference build returns "
            "-1 and the AVX2 build returns -1 for a full-length invalid input; it does "
            "not run the AArch64 tree (same api.hpp, compared by content only), the other "
            "eleven parameter sets, or anything beyond the crash."
        ),
        "checks": {},
        "measurements": {},
        "source_sha256": {},
    }
    started = time.monotonic()
    try:
        gcc, gxx = shutil.which("gcc"), shutil.which("g++")
        if gcc is None or gxx is None:
            raise Skip("gcc and g++ are required")
        missing = sorted(HOST_FLAGS - host_flags())
        if missing:
            raise Skip("host CPU lacks " + " ".join(missing) + "; the AVX2 tree cannot run")
        result["checks"]["pinned_sources"] = True
        for name, pinned in SOURCE_SHA256.items():
            actual = sha256(SUBMISSION / name)
            result["source_sha256"][name] = actual
            if actual != pinned:
                result["checks"]["pinned_sources"] = False
        result["checks"].update(source_checks())

        with tempfile.TemporaryDirectory(prefix="faest_se2_") as name:
            tmp = Path(name)
            (tmp / "driver.c").write_text(DRIVER_C)
            if not have_openssl(gcc, tmp):
                raise Skip("OpenSSL headers or libcrypto not available to gcc "
                           "(the reference build needs them)")
            if not have_cxx23(gxx, tmp):
                raise Skip("g++ does not support -std=c++23 (the AVX2 tree needs it)")
            build_started = time.monotonic()
            reference = build_reference(gcc, tmp)
            avx2 = build_avx2(gcc, gxx, tmp)
            result["measurements"]["build_seconds"] = round(
                time.monotonic() - build_started, 1)

            checks, measure = result["checks"], result["measurements"]
            trip_ref, trip_avx2 = call(reference, "roundtrip"), call(avx2, "roundtrip")
            checks["reference_build_signs_and_verifies"] = (
                returned(trip_ref, 0) and trip_ref["out"].get("message_ok") == "1"
                and trip_ref["out"].get("crypto_bytes") == str(CRYPTO_BYTES))
            checks["avx2_build_signs_and_verifies"] = (
                returned(trip_avx2, 0) and trip_avx2["out"].get("message_ok") == "1"
                and trip_avx2["out"].get("crypto_bytes") == str(CRYPTO_BYTES))

            lengths = (*SHORT, 0, CRYPTO_BYTES)
            avx2_runs = {n: call(avx2, "open", n) for n in lengths}
            ref_runs = {n: call(reference, "open", n) for n in lengths}
            checks["avx2_short_smlen_killed_by_signal"] = all(
                killed(avx2_runs[n]) for n in SHORT)
            checks["control_avx2_full_length_invalid_returns_minus1"] = returned(
                avx2_runs[CRYPTO_BYTES], -1)
            checks["control_reference_returns_minus1_for_all_lengths"] = all(
                returned(ref_runs[n], -1) for n in lengths)

            measure["avx2_crypto_sign_open"] = {
                f"smlen_{n}": summary(avx2_runs[n]) for n in lengths}
            measure["reference_crypto_sign_open"] = {
                f"smlen_{n}": summary(ref_runs[n]) for n in lengths}
            # Not pass conditions: smlen = 0 depends on memory in front of the
            # buffer; a set grinding bit makes the AVX2 verifier return early.
            measure["avx2_smlen_0_killed_by_signal"] = int(killed(avx2_runs[0]))
            grind = call(avx2, "open", 100, 100 - GRIND_BYTE_FROM_END, 0x80)
            measure["avx2_smlen_100_with_grinding_bit_set"] = summary(grind)
        result["status"] = "PASS" if all(result["checks"].values()) else "FAIL"
    except Skip as reason:
        result["status"] = "SKIP"
        result["reason"] = str(reason)
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as error:
        result["status"] = "FAIL"
        result["error"] = str(error)
    result["measurements"]["seconds"] = round(time.monotonic() - started, 1)

    text = json.dumps(result, indent=2, sort_keys=True)
    print(text)
    if args.output:
        args.output.write_text(text + "\n")
    return {"PASS": 0, "FAIL": 1, "SKIP": 77}[result["status"]]


if __name__ == "__main__":
    raise SystemExit(main())
