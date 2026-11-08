#!/usr/bin/env python3
"""Reproduce kem-23-3: short ciphertexts reach Mito's fixed-offset parser.

Extract the hash-pinned submission archive and compile every reference and
optimized Mito KEM with AddressSanitizer into a temporary directory. The only
extra sources are this repository's API shim and a small driver; no candidate
source is edited. For each build, an honest round trip through an exact-size
heap buffer must succeed without a sanitizer report, and a one-byte-short heap
copy of the same ciphertext, passed with its true length, must trigger a
heap-buffer-overflow read in the ciphertext parser.
"""

from __future__ import annotations

import hashlib
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile


ROOT = Path(__file__).resolve().parents[1]
API = ROOT / "api"
ARCHIVE = ROOT / "orig" / "kem-23" / "orig.zip"
ARCHIVE_SHA256 = "ee11788a3e8bf8653d18ade6580c47a331b91e7eac3e31a5a6750a4b4cfb827b"
ARCHIVE_URL = "https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/Mito.zip"
SETS = (
    "Mito-1-128", "Mito-1-256", "Mito-1-512",
    "Mito-1-E-128", "Mito-1-E-256", "Mito-1-E-512",
    "Mito-2-E-128", "Mito-2-E-256", "Mito-2-E-512",
)
FAMILIES = ("Reference_Implementation", "Optimized_Implementation")

DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ngcc_seed(const unsigned char *, unsigned long long);
int kem_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int kem_enc(unsigned char *, unsigned long long, unsigned char *, unsigned long long *,
            unsigned char *, unsigned long long *);
int kem_dec(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
            unsigned char *, unsigned long long *);
unsigned long long kem_get_pk_len_bytes(void), kem_get_sk_len_bytes(void),
    kem_get_ct_len_bytes(void), kem_get_ss_len_bytes(void);

int main(int argc, char **argv)
{
    unsigned char seed[48];
    unsigned long long pl = kem_get_pk_len_bytes(), sl = kem_get_sk_len_bytes();
    unsigned long long cl = kem_get_ct_len_bytes(), kl = kem_get_ss_len_bytes(), ol;
    unsigned char *pk = malloc(pl), *sk = malloc(sl), *ct = malloc(cl);
    unsigned char *key = malloc(kl), *out = malloc(kl), *input;
    int shortened = argc > 1 && !strcmp(argv[1], "short");

    memset(seed, 0x23, sizeof seed);
    if (!pk || !sk || !ct || !key || !out || ngcc_seed(seed, sizeof seed)
        || kem_keygen(pk, &pl, sk, &sl) || kem_enc(pk, pl, key, &kl, ct, &cl))
        return 2;
    /* The tested buffer is a separate allocation of exactly the stated length. */
    input = malloc(shortened ? cl - 1 : cl);
    if (!input)
        return 2;
    memcpy(input, ct, shortened ? cl - 1 : cl);
    ol = kl;
    if (kem_dec(sk, sl, input, shortened ? cl - 1 : cl, out, &ol))
        return 3;
    if (!shortened && (ol != kl || memcmp(out, key, kl)))
        return 4;
    printf("ct=%llu %s returned\n", cl, shortened ? "short" : "honest");
    return 0;
}
"""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def archive_bytes() -> bytes:
    if ARCHIVE.is_file():
        return ARCHIVE.read_bytes()
    try:
        with urllib.request.urlopen(ARCHIVE_URL, timeout=40) as response:
            return response.read()
    except (OSError, TimeoutError, urllib.error.URLError):
        try:
            return subprocess.run(
                ["curl", "-fsSL", "-A", "Mozilla/5.0", "--max-time", "60", ARCHIVE_URL],
                check=True, capture_output=True,
            ).stdout
        except (OSError, subprocess.CalledProcessError) as exc:
            print(f"SKIP kem-23-3: official archive unavailable: {exc}")
            sys.exit(77)


def extract_sources(destination: Path) -> None:
    data = archive_bytes()
    require(hashlib.sha256(data).hexdigest() == ARCHIVE_SHA256,
            "SHA-256 differs from the pinned Mito submission")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for member in archive.namelist():
            parts = member.split("/")
            if (len(parts) != 4 or parts[0] != "Implementations"
                    or parts[1] not in FAMILIES or parts[2] not in SETS
                    or not parts[3].endswith((".c", ".h"))):
                continue
            target = destination.joinpath(*parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            require(not target.exists(), f"duplicate archive member: {member}")
            target.write_bytes(archive.read(member))


def compiler() -> str:
    return os.environ.get("CC", "cc")


def asan_available(directory: Path) -> bool:
    probe = directory / "probe.c"
    probe.write_text("int main(void) { return 0; }\n")
    result = subprocess.run(
        [compiler(), "-fsanitize=address", "-o", str(directory / "probe"), str(probe)],
        capture_output=True,
    )
    return result.returncode == 0


def build(source: Path, family: str, name: str, driver: Path, output: Path) -> None:
    require((source / "KEM_AlgorithmInstance.c").is_file(),
            f"missing submitted source: {source}")
    files = sorted(p for p in source.glob("*.c") if p.name != "KAT_KEM.c")
    command = [
        compiler(), "-O1", "-g", "-std=gnu11", "-fsanitize=address",
        "-fno-omit-frame-pointer", "-D_GNU_SOURCE", "-DNGCC_BUILD_KEM",
        '-DNGCC_ID="kem-23"', '-DNGCC_ALG="Mito"', f'-DNGCC_VARIANT="{family}"',
        "-I", str(source), "-I", str(API), "-o", str(output), str(driver),
        *(str(path) for path in files), str(API / "link_shim.c"), "-lm",
    ]
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(
            f"compile failed for {family}/{name} (exit {result.returncode}):\n"
            f"{result.stdout}{result.stderr}"
        )


def run(binary: Path, mode: str) -> subprocess.CompletedProcess[str]:
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:exitcode=99")
    return subprocess.run([str(binary), mode], text=True, capture_output=True, env=env)


def check(binary: Path, family: str, name: str) -> None:
    honest = run(binary, "honest")
    require(honest.returncode == 0 and "AddressSanitizer" not in honest.stderr,
            f"{family}/{name}: honest round trip failed (exit {honest.returncode})\n"
            f"{honest.stdout}{honest.stderr}")
    short = run(binary, "short")
    report = short.stderr
    require(short.returncode == 99
            and "AddressSanitizer: heap-buffer-overflow" in report
            and "READ of size" in report
            and "mito_c_kem_from_string" in report
            and "parsing.c" in report
            and "kem_dec" in report,
            f"{family}/{name}: short ciphertext did not give a parser over-read "
            f"(exit {short.returncode})\n{short.stdout}{report}")
    size = report.split("READ of size", 1)[1].split()[0]
    print(f"{family}/{name}: honest PASS; short ciphertext heap-buffer-overflow "
          f"READ of size {size} in mito_c_kem_from_string")


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="mito-short-") as directory:
        temporary = Path(directory)
        if not asan_available(temporary):
            print(f"SKIP kem-23-3: {compiler()} cannot build with -fsanitize=address")
            sys.exit(77)
        source_root = temporary / "source"
        extract_sources(source_root)
        driver = temporary / "driver.c"
        driver.write_text(DRIVER)
        for family in FAMILIES:
            for name in SETS:
                binary = temporary / f"{family}-{name}"
                build(source_root / "Implementations" / family / name,
                      family, name, driver, binary)
                check(binary, family, name)
    print("RESULT kem-23-3 CONFIRMED: 18/18 honest round trips; "
          "18/18 short-ciphertext parser over-reads under AddressSanitizer")


if __name__ == "__main__":
    main()
