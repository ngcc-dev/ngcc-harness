#!/usr/bin/env python3
"""FAEST-3: the reference PRG ignores a failing OpenSSL AES backend.

The reference tree is built from the submitted sources with the submitted
Makefile's preprocessor flags (-DNDEBUG -DHAVE_OPENSSL) and a build-time
config.h, created in the temporary directory, that leaves HAVE_AESNI
undefined.  That is the configuration of a target without AES-NI intrinsics:
prg() then goes through generic_prg() and the OpenSSL EVP calls.

A key pair is generated with working OpenSSL.  The same binary then signs
twice: once with working OpenSSL (control) and once with OPENSSL_CONF pointing
at a configuration that activates only the null provider, so every AES fetch
fails.  Both outputs are verified by the same binary with working OpenSSL.

The script reports what crypto_sign returned and whether the output verifies.
Whether the released witness commitment d equals the secret key in the failing
run is recorded as a measurement only: it depends on the contents of buffers
that the failed PRG never wrote.
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
REFERENCE = HERE / "submission/nist_submission_round3/Reference_Implementation"
CANDIDATE = "FAEST-3"
JOBS = 3
TIMEOUT = 180

# Parameter sets run: (directory, faest_paramid_t enumerator).
SETS = (("faest_128f", "FAEST_128F"), ("faest_em_256s", "FAEST_EM_256S"))

SOURCE_SHA256 = {
    "Reference_Implementation/faest_128f/Makefile":
        "336d58a7e5953936673cd55c4f35da254a13c3cde1ce6e8835695cd2e74b0013",
    "Reference_Implementation/faest_128f/aes.c":
        "1022730ced3fcd892eefe9c898108764f2d6f5260a7a326ce0ba3813b8dbb8f4",
    "Reference_Implementation/faest_128f/compat.h":
        "81fb370004a3959aab77464c6fec48688b6ae64a470db14b32318f7eef3cfa89",
    "Reference_Implementation/faest_128f/crypto_sign.c":
        "8027cf72b17c85ab5bb964644897aed8d2e86e08d446a627d721428dfc460c0a",
    "Reference_Implementation/faest_128f/faest_128f.c":
        "65e29537bf587a46ff35f1392f29729655a0ddb611a246f635f50829e2fd4880",
    "Reference_Implementation/faest_128f/faest_impl.c":
        "90fd42dc5982ebd7107e0c26af6cf8bfe374298c69fc91dc354cfc607717af71",
    "Reference_Implementation/faest_em_256s/Makefile":
        "336d58a7e5953936673cd55c4f35da254a13c3cde1ce6e8835695cd2e74b0013",
    "Reference_Implementation/faest_em_256s/aes.c":
        "1022730ced3fcd892eefe9c898108764f2d6f5260a7a326ce0ba3813b8dbb8f4",
    "Reference_Implementation/faest_em_256s/faest_impl.c":
        "90fd42dc5982ebd7107e0c26af6cf8bfe374298c69fc91dc354cfc607717af71",
}

# Build-time configuration header.  With HAVE_CONFIG_H the submitted compat.h
# skips its own feature detection, so HAVE_AESNI stays undefined.
CONFIG_H = """\
/* Created by reproduce_faest_3.py: a target without AES-NI intrinsics. */
#define HAVE_ALIGNED_ALLOC 1
#define HAVE_EXPLICIT_BZERO 1
#define HAVE_MEMPCPY 1
"""

# OpenSSL configuration in which no provider implements AES.
NULL_PROVIDER_CNF = """\
openssl_conf = openssl_init

[openssl_init]
providers = provider_sect

[provider_sect]
null = null_sect

[null_sect]
activate = 1
"""

PROBE_C = r"""
#include <stdio.h>
#include <openssl/evp.h>
int main(void) {
  unsigned char key[16] = {0};
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  int ok = ctx && EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) == 1;
  EVP_CIPHER_CTX_free(ctx);
  printf("evp_aes_init=%d\n", ok);
  return 0;
}
"""

DRIVER_C = r"""
/* Driver for reproduce_faest_3.py.  Modes: keygen, sign, open, layout. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "api.h"
#include "instances.h"

/* Deterministic randombytes for the HAVE_RANDOMBYTES build (splitmix64). */
static uint64_t rng_state = UINT64_C(0x5E0FAE5700000001);
void randombytes(unsigned char* x, unsigned long long xlen) {
  for (unsigned long long i = 0; i < xlen; i++) {
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    x[i] = (unsigned char)((z ^ (z >> 31)) & 0xff);
  }
}

static int evp_aes_init(void) {
  unsigned char key[16] = {0};
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  int ok = ctx && EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL) == 1;
  EVP_CIPHER_CTX_free(ctx);
  return ok;
}

static const unsigned char MSG[16] = "prg failure test";

static int load(const char* path, unsigned char* buf, size_t len) {
  FILE* f = fopen(path, "rb");
  if (!f) return -1;
  size_t got = fread(buf, 1, len, f);
  fclose(f);
  return got == len ? 0 : -1;
}

static int store(const char* path, const unsigned char* buf, size_t len) {
  FILE* f = fopen(path, "wb");
  if (!f) return -1;
  size_t put = fwrite(buf, 1, len, f);
  return (fclose(f) == 0 && put == len) ? 0 : -1;
}

int main(int argc, char** argv) {
  static unsigned char keys[CRYPTO_PUBLICKEYBYTES + CRYPTO_SECRETKEYBYTES];
  unsigned char* pk = keys;
  unsigned char* sk = keys + CRYPTO_PUBLICKEYBYTES;
  const size_t smsize = sizeof MSG + CRYPTO_BYTES;
  if (argc < 2) return 2;

  if (!strcmp(argv[1], "layout")) {
    const faest_paramset_t* p = faest_get_paramset(PARAMID);
    unsigned lb = p->lambda / 8;
    /* signature_d() in faest_impl.c */
    unsigned d_offset = (p->tau - 1) * p->ell / 8 + 10 * ((p->n_mult + 7) / 8) + 4 * lb;
    printf("lambda=%u\nd_offset=%u\nc_bytes=%u\n", (unsigned)p->lambda, d_offset,
           (unsigned)((p->tau - 1) * p->ell / 8));
    printf("pk_bytes=%d\nsk_bytes=%d\nsig_bytes=%d\nmsg_bytes=%zu\n",
           CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES, sizeof MSG);
    return 0;
  }
  if (argc < 3) return 2;

  printf("evp_aes_init=%d\n", evp_aes_init());
  if (!strcmp(argv[1], "keygen")) {
    int rc = crypto_sign_keypair(pk, sk);
    printf("rc=%d\n", rc);
    return store(argv[2], keys, sizeof keys) ? 3 : 0;
  }
  if (load(argv[2], keys, sizeof keys)) return 3;
  if (argc < 4) return 2;

  unsigned char* sm = malloc(smsize);
  unsigned char* out = malloc(smsize);
  if (!sm || !out) return 3;
  if (!strcmp(argv[1], "sign")) {
    unsigned long long smlen = 0;
    memset(sm, 0xA5, smsize);   /* so that zero bytes in the output were written */
    int rc = crypto_sign(sm, &smlen, MSG, sizeof MSG, sk);
    printf("rc=%d\nsmlen=%llu\n", rc, smlen);
    return store(argv[3], sm, smsize) ? 3 : 0;
  }
  if (!strcmp(argv[1], "open")) {
    unsigned long long mlen = 0;
    if (load(argv[3], sm, smsize)) return 3;
    int rc = crypto_sign_open(out, &mlen, sm, smsize, pk);
    printf("rc=%d\nmessage_ok=%d\n", rc,
           rc == 0 && mlen == sizeof MSG && memcmp(out, MSG, sizeof MSG) == 0);
    return 0;
  }
  return 2;
}
"""


class Skip(Exception):
    pass


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    return subprocess.run(command, text=True, capture_output=True, **kwargs)


def compile_many(gcc, jobs):
    """jobs: list of (source, object, flags).  Explicit gcc command lines."""
    def one(job):
        source, obj, flags = job
        result = run([gcc, *flags, "-c", str(source), "-o", str(obj)])
        if result.returncode:
            raise RuntimeError(f"compile failed: {source}\n{result.stderr[-2000:]}")
    with concurrent.futures.ThreadPoolExecutor(max_workers=JOBS) as pool:
        list(pool.map(one, jobs))


def build(gcc, tmp, set_name, paramid, tag, extra):
    """Build the reference tree of one parameter set plus the driver."""
    src = REFERENCE / set_name
    out = tmp / f"{set_name}_{tag}"
    out.mkdir()
    # CPPFLAGS of the submitted Makefile: -I. -Isha3 -DNDEBUG -DHAVE_OPENSSL
    # -DFAEST_EXPORT= ; -DHAVE_RANDOMBYTES is what its KAT target adds.
    flags = ["-O2", "-std=gnu11", *extra, f"-I{src}", f"-I{src / 'sha3'}",
             "-DNDEBUG", "-DHAVE_OPENSSL", "-DFAEST_EXPORT=", "-DHAVE_RANDOMBYTES"]
    sources = sorted(src.glob("*.c")) + sorted((src / "sha3").glob("*.c"))
    jobs = [(s, out / f"{s.parent.name}_{s.stem}.o", flags) for s in sources]
    driver = tmp / "driver.c"
    jobs.append((driver, out / "driver.o", flags + [f"-DPARAMID={paramid}"]))
    compile_many(gcc, jobs)
    exe = out / "driver"
    link = run([gcc, *[str(j[1]) for j in jobs], "-o", str(exe), "-lcrypto"])
    if link.returncode:
        raise RuntimeError(f"link failed: {link.stderr[-2000:]}")
    return exe


def fields(text):
    out = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            out[key.strip()] = value.strip()
    return out


def child(exe, args, env):
    """Run one driver mode; return (returncode or None on timeout, fields)."""
    try:
        result = run([str(exe), *map(str, args)], env=env, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return None, {}
    return result.returncode, fields(result.stdout)


def source_checks():
    """The source lines the review cites, read from the pristine tree."""
    base = REFERENCE / "faest_128f"
    aes = (base / "aes.c").read_text().splitlines()
    makefile = (base / "Makefile").read_text().splitlines()
    generic_prg = aes[725:749]                     # aes.c:726-749
    return {
        "source_makefile_line4_sets_ndebug":
            makefile[3].startswith("CPPFLAGS+=") and "-DNDEBUG" in makefile[3].split(),
        "source_aes_c_1027_discards_evp_init_result":
            aes[1026].strip() == "EVP_EncryptInit_ex(ctx, cipher, NULL, key, NULL);",
        "source_generic_prg_checks_backend_only_by_assert":
            aes[725].startswith("static void generic_prg(")
            and [n + 726 for n, line in enumerate(generic_prg)
                 if "assert(ret == 0);" in line] == [730, 735, 743]
            and not any("if (ret" in line or "return" in line for line in generic_prg),
        "source_prg_falls_back_to_generic_prg_at_790":
            aes[789].strip() == "generic_prg(key, internal_iv, out, seclvl, outlen);",
    }


def experiment(gcc, tmp, set_name, paramid, env_ok, env_null, with_aesni_control):
    checks, measure = {}, {}
    generic = build(gcc, tmp, set_name, paramid, "generic",
                    ["-DHAVE_CONFIG_H", f"-I{tmp / 'cfg'}"])
    rc, layout = child(generic, ["layout"], env_ok)
    if rc != 0:
        raise RuntimeError("layout query failed")
    layout = {k: int(v) for k, v in layout.items()}
    lb = layout["lambda"] // 8
    msg, pkb, skb = layout["msg_bytes"], layout["pk_bytes"], layout["sk_bytes"]

    keys = tmp / f"{set_name}.keys"
    rc, kg = child(generic, ["keygen", keys], env_ok)
    if rc != 0 or kg.get("rc") != "0" or kg.get("evp_aes_init") != "1":
        raise RuntimeError("key generation with working OpenSSL failed")
    key_material = keys.read_bytes()
    owf_key = key_material[pkb + skb - lb:]        # sk = OWF input || OWF key

    def sign_and_open(tag, exe, env):
        sm = tmp / f"{set_name}_{tag}.sm"
        started = time.monotonic()
        rc, sig = child(exe, ["sign", keys, sm], env)
        seconds = time.monotonic() - started
        if rc != 0 or not sm.exists():
            return {"exit": rc, "sign_rc": None, "evp": None, "open_rc": None,
                    "message_ok": False, "seconds": seconds, "sm": b""}
        # Verification always runs with working OpenSSL.
        orc, opened = child(exe, ["open", keys, sm], env_ok)
        return {"exit": rc, "sign_rc": int(sig["rc"]), "evp": int(sig["evp_aes_init"]),
                "smlen": int(sig["smlen"]),
                "open_rc": int(opened["rc"]) if orc == 0 else None,
                "message_ok": opened.get("message_ok") == "1",
                "seconds": seconds, "sm": sm.read_bytes()}

    control = sign_and_open("control", generic, env_ok)
    failing = sign_and_open("null_provider", generic, env_null)

    checks["control_backend_works"] = control["evp"] == 1
    checks["control_working_openssl_signs_and_verifies"] = (
        control["sign_rc"] == 0 and control["open_rc"] == 0 and control["message_ok"])
    checks["null_provider_makes_evp_aes_init_fail"] = failing["evp"] == 0
    checks["crypto_sign_returns_0_with_failing_backend"] = (
        failing["exit"] == 0 and failing["sign_rc"] == 0
        and failing.get("smlen") == msg + layout["sig_bytes"])
    checks["failing_backend_output_does_not_verify"] = (
        failing["open_rc"] == -1 and not failing["message_ok"])

    def describe(sm):
        sig = sm[msg:]
        d = sig[layout["d_offset"]:layout["d_offset"] + lb]
        c = sig[:layout["c_bytes"]]
        return {
            "d_first_lambda_bits_equal_owf_key": int(len(d) == lb and d == owf_key),
            "zero_bytes_in_c": c.count(0),
            "c_bytes": len(c),
        }

    measure["lambda"] = layout["lambda"]
    measure["signature_bytes"] = layout["sig_bytes"]
    measure["d_offset_in_signature"] = layout["d_offset"]
    measure["null_provider"] = {
        "crypto_sign_rc": failing["sign_rc"], "crypto_sign_open_rc": failing["open_rc"],
        "sign_seconds": round(failing["seconds"], 3), **describe(failing["sm"])}
    measure["control"] = {
        "crypto_sign_rc": control["sign_rc"], "crypto_sign_open_rc": control["open_rc"],
        "sign_seconds": round(control["seconds"], 3), **describe(control["sm"])}

    if with_aesni_control:
        # The default build (compat.h selects HAVE_AESNI on x86): the PRG takes
        # the AES-NI path and never calls OpenSSL, so the same fault has no effect.
        aesni = build(gcc, tmp, set_name, paramid, "aesni", [])
        native = sign_and_open("aesni_null_provider", aesni, env_null)
        checks["control_aesni_build_unaffected_by_null_provider"] = (
            native["evp"] == 0 and native["sign_rc"] == 0
            and native["open_rc"] == 0 and native["message_ok"])
        measure["aesni_build_null_provider"] = {
            "crypto_sign_rc": native["sign_rc"], "crypto_sign_open_rc": native["open_rc"],
            **describe(native["sm"])}
    return checks, measure


def cpu_has_aes():
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("flags"):
                return "aes" in line.split(":", 1)[1].split()
    except OSError:
        pass
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path, help="also write the JSON here")
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "Shows that the reference signer, built with the submitted -DNDEBUG "
            "-DHAVE_OPENSSL flags and without the AES-NI path, returns 0 and emits a "
            "non-verifying signature when OpenSSL provides no AES, while the same binary "
            "signs and verifies with working OpenSSL; it does not show that key bytes "
            "appear in the output on every platform (measured only), and the no-AES-NI "
            "build is emulated on x86 with a build-time config.h."
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
            actual = sha256(REFERENCE.parent / name)
            result["source_sha256"][name] = actual
            if actual != pinned:
                result["checks"]["pinned_sources"] = False
        result["checks"].update(source_checks())

        with tempfile.TemporaryDirectory(prefix="faest_se1_") as name:
            tmp = Path(name)
            (tmp / "cfg").mkdir()
            (tmp / "cfg/config.h").write_text(CONFIG_H)
            (tmp / "driver.c").write_text(DRIVER_C)
            (tmp / "probe.c").write_text(PROBE_C)
            cnf = tmp / "null_provider.cnf"
            cnf.write_text(NULL_PROVIDER_CNF)

            env_ok = {k: v for k, v in os.environ.items()
                      if not k.startswith("OPENSSL_") and k != "LD_PRELOAD"}
            env_null = dict(env_ok, OPENSSL_CONF=str(cnf))

            probe = tmp / "probe"
            built = run([gcc, "-O2", str(tmp / "probe.c"), "-o", str(probe), "-lcrypto"])
            if built.returncode:
                raise Skip("OpenSSL headers or libcrypto not available to gcc")
            ok = fields(run([str(probe)], env=env_ok, timeout=TIMEOUT).stdout)
            null = fields(run([str(probe)], env=env_null, timeout=TIMEOUT).stdout)
            if ok.get("evp_aes_init") != "1":
                raise Skip("AES is not available from this libcrypto by default")
            if null.get("evp_aes_init") != "0":
                raise Skip("this libcrypto still provides AES under the null-provider "
                           "configuration; the fault cannot be injected here")

            aesni = cpu_has_aes()
            result["measurements"]["host_cpu_has_aes_ni"] = int(aesni)
            for set_name, paramid in SETS:
                checks, measure = experiment(
                    gcc, tmp, set_name, paramid, env_ok, env_null,
                    with_aesni_control=aesni and set_name == SETS[0][0])
                for key, value in checks.items():
                    result["checks"][f"{set_name}_{key}"] = value
                result["measurements"][set_name] = measure
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
