#!/usr/bin/env python3
"""FAEST-5: a failing OS random source is ignored by key generation and signing.

rand_bytes() returns -1 when getrandom(..., GRND_NONBLOCK) fails, and its
callers in both trees discard that result.  The trees are built from the
submitted sources with their OS randomness file (the configuration of the
submitted api_test target, not the KAT one).  An LD_PRELOAD library, compiled
in the temporary directory, makes getrandom() fail with EAGAIN, as it does
before the kernel pool is ready.

Under that fault crypto_sign_keypair returns 0 and the secret OWF key (the last
lambda/8 bytes of sk) is whatever the caller's buffer held: the driver prefills
the buffer and compares.  What the public OWF input part of sk holds is
recorded per tree.  The control runs the same binaries without the preload
library, twice.

Whether two signatures of one message are identical under the fault is a
measurement only, because the signing randomness is then an uninitialised
stack array.  A prefill with both low key bits set, for which the review
predicts a non-terminating key generation loop, is also measurement only.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import time
from pathlib import Path


HERE = Path(__file__).resolve().parent
SUBMISSION = HERE / "submission/nist_submission_round3"
REFERENCE = SUBMISSION / "Reference_Implementation"
AVX2 = SUBMISSION / "Additional_Implementations/avx2/faest_128f"
AARCH64 = SUBMISSION / "Additional_Implementations/aarch64/faest_128f"
CANDIDATE = "FAEST-5"
JOBS = 3
TIMEOUT = 120
LOOP_TIMEOUT = 5

ZERO_FILL = 0x00            # the review's case: zero-initialised buffers
PATTERN_FILL = 0xA4         # recognisable, low key bits 0,0 (inside the key domain)
OUT_OF_DOMAIN_FILL = 0xA7   # low key bits 1,1: rejected by the key generation loop

# AES-128 of the zero block under the zero key (FIPS 197 arithmetic, a public
# constant): the OWF output of the all-zero FAEST-128 key and input.
AES128_ZERO = "66e94bd4ef8a2c3b884cfa59ca342b2e"

# -march=haswell -maes -mpclmul is the instruction set the AVX2 build uses here.
HOST_FLAGS = {"avx", "avx2", "bmi1", "bmi2", "fma", "movbe", "abm", "popcnt",
              "f16c", "rdrand", "fsgsbase", "aes", "pclmulqdq"}

SOURCE_SHA256 = {
    "Reference_Implementation/faest_128f/randomness.c":
        "37c9cf1d793c3475540cd57eaabf5dc48fdcef3a5512aa7d39d0e0869c46b1fa",
    "Reference_Implementation/faest_128f/faest_128f.c":
        "65e29537bf587a46ff35f1392f29729655a0ddb611a246f635f50829e2fd4880",
    "Reference_Implementation/faest_128f/Makefile":
        "336d58a7e5953936673cd55c4f35da254a13c3cde1ce6e8835695cd2e74b0013",
    "Reference_Implementation/faest_em_256s/randomness.c":
        "37c9cf1d793c3475540cd57eaabf5dc48fdcef3a5512aa7d39d0e0869c46b1fa",
    "Reference_Implementation/faest_em_256s/faest_em_256s.c":
        "a1010468505ba9c8f66d3f861b9f1f2a7a6fc963a33a8cbd1c060765e120de10",
    "Additional_Implementations/avx2/faest_128f/randomness_os.c":
        "d6cbdef69f8e005721b8b4c6140b0d070c7dea65284778c2c2c53e91fe476aed",
    "Additional_Implementations/avx2/faest_128f/api.hpp":
        "907d80abf9cfec2868225820e34b2c4828f7dcde3a896724ac0fb4aba85f76da",
    "Additional_Implementations/avx2/faest_128f/Makefile":
        "5dfa36b6a2b79de3e9723dce6ab4732839ff3d24bd510bce65ce6ed9371f05eb",
    "Additional_Implementations/aarch64/faest_128f/randomness_os.c":
        "d6cbdef69f8e005721b8b4c6140b0d070c7dea65284778c2c2c53e91fe476aed",
    "Additional_Implementations/aarch64/faest_128f/api.hpp":
        "907d80abf9cfec2868225820e34b2c4828f7dcde3a896724ac0fb4aba85f76da",
}

SHIM_C = r"""
/* LD_PRELOAD library: getrandom() fails with EAGAIN, as GRND_NONBLOCK does
   before the kernel pool is ready. */
#include <errno.h>
#include <sys/types.h>
ssize_t getrandom(void* buf, size_t len, unsigned int flags) {
  (void)buf; (void)len; (void)flags;
  errno = EAGAIN;
  return -1;
}
"""

PROBE_C = r"""
#include <stdio.h>
#include <sys/random.h>
int main(void) {
  unsigned char buf[16];
  printf("getrandom=%ld\n", (long)getrandom(buf, sizeof buf, GRND_NONBLOCK));
  return 0;
}
"""

# Build prerequisites that are not executables: probed, SKIP when absent.
OPENSSL_PROBE_C = """\
#include <openssl/evp.h>
int main(void) { return EVP_aes_128_ecb() == NULL; }
"""
CXX23_PROBE = """\
int main() { [[assume(true)]]; return 0; }
"""

DRIVER_C = r"""
/* Driver for reproduce_faest_5.py; linked against either tree.
   Usage: driver <fill byte>.  The fill byte is what the caller's key buffers
   hold before crypto_sign_keypair. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

int rand_bytes(uint8_t* dst, size_t num_bytes);   /* randomness.h, both trees */

static const unsigned char MSG[4] = "abc";
static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
static unsigned char sm1[sizeof MSG + CRYPTO_BYTES], sm2[sizeof MSG + CRYPTO_BYTES];
static unsigned char opened[sizeof MSG + CRYPTO_BYTES];

static void hex(const char* name, const unsigned char* p, size_t n) {
  printf("%s=", name);
  for (size_t i = 0; i < n; i++) printf("%02x", p[i]);
  printf("\n");
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  unsigned char probe[16] = {0};
  unsigned long long len1 = 0, len2 = 0, mlen = 0;
  int fill = (int)strtoul(argv[1], 0, 0);
  memset(pk, fill, sizeof pk);
  memset(sk, fill, sizeof sk);

  printf("rand_bytes_rc=%d\n", rand_bytes(probe, sizeof probe));
  fflush(stdout);
  int rc = crypto_sign_keypair(pk, sk);
  printf("keypair_rc=%d\n", rc);
  hex("sk", sk, sizeof sk);
  hex("pk", pk, sizeof pk);
  fflush(stdout);

  int rc1 = crypto_sign(sm1, &len1, MSG, sizeof MSG, sk);
  int rc2 = crypto_sign(sm2, &len2, MSG, sizeof MSG, sk);
  printf("sign_rc1=%d\nsign_rc2=%d\n", rc1, rc2);
  printf("signatures_identical=%d\n", len1 == len2 && memcmp(sm1, sm2, len1) == 0);
  printf("open_rc=%d\n", crypto_sign_open(opened, &mlen, sm1, len1, pk));
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


def build_reference(gcc, tmp, set_name):
    src = REFERENCE / set_name
    out = tmp / f"reference_{set_name}"
    out.mkdir()
    # CPPFLAGS of the submitted Makefile.  Without -DHAVE_RANDOMBYTES the
    # submitted randomness.c uses the OS source, as in its api_test target.
    flags = ["-O2", "-std=gnu11", f"-I{src}", f"-I{src / 'sha3'}",
             "-DNDEBUG", "-DHAVE_OPENSSL", "-DFAEST_EXPORT="]
    sources = sorted(src.glob("*.c")) + sorted((src / "sha3").glob("*.c"))
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
    out = tmp / "avx2_faest_128f"
    out.mkdir()
    # CPPFLAGS of the submitted Makefile: -I. -Isha3 -DHAVE_OPENSSL, with
    # randomness_os.c as in its api_test target.  The generic sources
    # instantiate templates for parameter sets whose tables this directory
    # does not define; the submitted Makefile drops them with -flto, here the
    # linker drops the unreferenced sections.
    arch = ["-O2", "-march=haswell", "-maes", "-mpclmul",
            "-ffunction-sections", "-fdata-sections"]
    cpp = [f"-I{AVX2}", f"-I{AVX2 / 'sha3'}", "-DHAVE_OPENSSL"]
    c_sources = [AVX2 / "randomness_os.c", tmp / "driver.c"]
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


def call(exe, fill, env, timeout=TIMEOUT):
    """One driver process.  Returns its fields, with 'ended' describing the exit."""
    try:
        result = run([str(exe), hex(fill)], env=env, timeout=timeout)
    except subprocess.TimeoutExpired as expired:
        out = fields(expired.stdout.decode() if isinstance(expired.stdout, bytes)
                     else (expired.stdout or ""))
        out["ended"] = "timeout"
        return out
    out = fields(result.stdout)
    out["ended"] = "exit 0" if result.returncode == 0 else f"returncode {result.returncode}"
    return out


def statement_calls(lines, numbers):
    """True if each cited line is a bare `rand_bytes(...);` statement."""
    return all(lines[n - 1].strip().startswith("rand_bytes(")
               and lines[n - 1].rstrip().endswith(";") for n in numbers)


def source_checks():
    ref_rand = (REFERENCE / "faest_128f/randomness.c").read_text().splitlines()
    ref_api = (REFERENCE / "faest_128f/faest_128f.c").read_text().splitlines()
    em_api = (REFERENCE / "faest_em_256s/faest_em_256s.c").read_text().splitlines()
    avx_rand = (AVX2 / "randomness_os.c").read_text().splitlines()
    avx_api = (AVX2 / "api.hpp").read_text().splitlines()

    def getrandom_path(lines, first):
        block = [line.strip() for line in lines[first - 1:first + 6]]
        return block == [
            "int rand_bytes(uint8_t* dst, size_t len) {",
            "const ssize_t ret = getrandom(dst, len, GRND_NONBLOCK);",
            "if (ret < 0 || (size_t)ret != len) {",
            "return -1;",
            "}",
            "return 0;",
            "}",
        ]

    return {
        "source_reference_randomness_c_38_44_returns_minus1": getrandom_path(ref_rand, 38),
        "source_avx2_randomness_os_c_31_37_returns_minus1": getrandom_path(avx_rand, 31),
        "source_reference_callers_discard_result":
            statement_calls(ref_api, (40, 44, 112, 123))
            and statement_calls(em_api, (40, 44, 112, 123)),
        "source_avx2_callers_discard_result": statement_calls(avx_api, (32, 34, 46, 59)),
        "source_aarch64_files_identical_to_avx2": all(
            (AARCH64 / name).read_bytes() == (AVX2 / name).read_bytes()
            for name in ("api.hpp", "randomness_os.c")),
    }


def host_flags():
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("flags"):
                return set(line.split(":", 1)[1].split())
    except OSError:
        pass
    return set()


def experiment(exe, env_ok, env_fail, sk_bytes, pk_bytes, key_bytes, aes128):
    checks, measure = {}, {}

    def prefill(fill, n):
        return f"{fill:02x}" * n

    def owf_key(r):                     # sk = OWF input || OWF key
        return r.get("sk", "")[2 * (sk_bytes - key_bytes):]

    def owf_input(r, fill):
        part = r.get("sk", "")[:2 * (sk_bytes - key_bytes)]
        if part == prefill(fill, sk_bytes - key_bytes):
            return "caller's buffer"
        return "zero" if part == prefill(0, sk_bytes - key_bytes) else part

    zero = call(exe, ZERO_FILL, env_fail)
    pattern = call(exe, PATTERN_FILL, env_fail)
    checks["rand_bytes_returns_minus1_when_getrandom_fails"] = (
        zero.get("rand_bytes_rc") == "-1" and pattern.get("rand_bytes_rc") == "-1")
    checks["keypair_returns_0_when_getrandom_fails"] = (
        zero.get("keypair_rc") == "0" and pattern.get("keypair_rc") == "0")
    checks["owf_key_is_the_callers_buffer"] = (
        len(zero.get("sk", "")) == 2 * sk_bytes
        and owf_key(zero) == prefill(ZERO_FILL, key_bytes)
        and owf_key(pattern) == prefill(PATTERN_FILL, key_bytes))
    if aes128:
        # pk = OWF input || AES-128_key(input); both halves known in advance.
        checks["zero_buffer_gives_public_key_of_the_all_zero_key"] = (
            zero.get("pk") == "00" * 16 + AES128_ZERO)

    first, second = call(exe, ZERO_FILL, env_ok), call(exe, ZERO_FILL, env_ok)
    checks["control_keys_are_fresh_and_differ_between_runs"] = all(
        r.get("rand_bytes_rc") == "0" and r.get("keypair_rc") == "0"
        and len(r.get("sk", "")) == 2 * sk_bytes and len(r.get("pk", "")) == 2 * pk_bytes
        and owf_key(r) != prefill(ZERO_FILL, key_bytes)
        for r in (first, second)) and owf_key(first) != owf_key(second) \
        and first.get("pk") != second.get("pk")
    checks["control_signs_and_verifies"] = all(
        r.get("sign_rc1") == "0" and r.get("sign_rc2") == "0" and r.get("open_rc") == "0"
        and r.get("ended") == "exit 0" for r in (first, second))

    def numbers(r):
        return {
            "ended": r.get("ended"),
            "crypto_sign_rc": [int(r[k]) for k in ("sign_rc1", "sign_rc2") if k in r],
            "two_signatures_of_one_message_identical":
                int(r["signatures_identical"]) if "signatures_identical" in r else None,
            "crypto_sign_open_rc": int(r["open_rc"]) if "open_rc" in r else None,
        }

    measure["getrandom_fails_zero_buffer"] = {
        "sk": zero.get("sk"), "pk": zero.get("pk"), **numbers(zero)}
    measure["getrandom_fails_pattern_buffer"] = {
        "sk": pattern.get("sk"), "pk": pattern.get("pk"),
        "owf_input_part_of_sk": owf_input(pattern, PATTERN_FILL), **numbers(pattern)}
    measure["control"] = numbers(first)
    measure["sk_bytes"], measure["owf_key_bytes"] = sk_bytes, key_bytes

    # Not a pass condition: a stale buffer outside the key domain.
    stuck = call(exe, OUT_OF_DOMAIN_FILL, env_fail, timeout=LOOP_TIMEOUT)
    measure["getrandom_fails_buffer_with_low_key_bits_set"] = (
        f"keygen still running after {LOOP_TIMEOUT} s"
        if stuck["ended"] == "timeout" and "keypair_rc" not in stuck
        else f"keypair_rc={stuck.get('keypair_rc')}, {stuck['ended']}")
    return checks, measure


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path, help="also write the JSON here")
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "Shows that with getrandom() failing, key generation of the reference "
            "(faest_128f, faest_em_256s) and AVX2 (faest_128f) builds returns 0 and leaves "
            "the caller's buffer as the secret OWF key, while the same binaries produce "
            "fresh, differing keys when getrandom() works; it does not run the AArch64 "
            "tree or the other parameter sets, and repeated signatures under the fault "
            "are measured, not required."
        ),
        "checks": {},
        "measurements": {},
        "source_sha256": {},
    }
    started = time.monotonic()
    try:
        gcc = shutil.which("gcc")
        if gcc is None:
            raise Skip("gcc not found")
        result["checks"]["pinned_sources"] = True
        for name, pinned in SOURCE_SHA256.items():
            actual = sha256(SUBMISSION / name)
            result["source_sha256"][name] = actual
            if actual != pinned:
                result["checks"]["pinned_sources"] = False
        result["checks"].update(source_checks())

        with tempfile.TemporaryDirectory(prefix="faest_se3_") as name:
            tmp = Path(name)
            for filename, text in (("driver.c", DRIVER_C), ("shim.c", SHIM_C),
                                   ("probe.c", PROBE_C)):
                (tmp / filename).write_text(text)
            shim = tmp / "libfailgetrandom.so"
            built = run([gcc, "-O2", "-shared", "-fPIC", str(tmp / "shim.c"), "-o", str(shim)])
            if built.returncode:
                raise Skip("cannot build a shared library with gcc")
            probe = tmp / "probe"
            built = run([gcc, "-O2", str(tmp / "probe.c"), "-o", str(probe)])
            if built.returncode:
                raise Skip("no <sys/random.h> getrandom() on this system")
            if not have_openssl(gcc, tmp):
                raise Skip("OpenSSL headers or libcrypto not available to gcc "
                           "(the reference build needs them)")

            env_ok = {k: v for k, v in os.environ.items() if k != "LD_PRELOAD"}
            env_fail = dict(env_ok, LD_PRELOAD=str(shim))
            ok = fields(run([str(probe)], env=env_ok, timeout=TIMEOUT).stdout)
            fail = fields(run([str(probe)], env=env_fail, timeout=TIMEOUT).stdout)
            if ok.get("getrandom") != "16":
                raise Skip("getrandom() does not work on this host")
            if fail.get("getrandom") != "-1":
                raise Skip("LD_PRELOAD does not interpose getrandom() here")

            # (label, binary, sk bytes, pk bytes, OWF key bytes, AES-128 OWF)
            targets = [("reference_faest_128f", build_reference(gcc, tmp, "faest_128f"),
                        32, 32, 16, True),
                       ("reference_faest_em_256s", build_reference(gcc, tmp, "faest_em_256s"),
                        64, 64, 32, False)]
            gxx = shutil.which("g++")
            missing = sorted(HOST_FLAGS - host_flags())
            avx2_run = gxx is not None and not missing and have_cxx23(gxx, tmp)
            result["measurements"]["avx2_tree_run"] = int(avx2_run)
            if avx2_run:
                targets.append(("avx2_faest_128f", build_avx2(gcc, gxx, tmp),
                                32, 32, 16, True))
            result["measurements"]["build_seconds"] = round(time.monotonic() - started, 1)

            for label, exe, sk_bytes, pk_bytes, key_bytes, aes128 in targets:
                checks, measure = experiment(exe, env_ok, env_fail, sk_bytes, pk_bytes,
                                             key_bytes, aes128)
                for key, value in checks.items():
                    result["checks"][f"{label}_{key}"] = value
                result["measurements"][label] = measure
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
