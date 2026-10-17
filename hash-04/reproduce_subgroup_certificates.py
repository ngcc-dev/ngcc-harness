#!/usr/bin/env python3
"""hash-04-2: reduced-size CHAMP subgroup collision certificates.

Mounir IDRASSI's public package (MIT code licence; not vendored here) gives an
explicit two-stage subgroup search that outputs ordinary equal-length positive
CHAMP words, and twelve collision certificates over 16-28-bit safe primes.
This script

  1. checks an independent Python CHAMP model (the generators and the
     inverse-entry finalization of the submitted source) against the submitted
     CHAMP-512 and CHAMP-1024 libraries;
  2. fetches the package at the pinned commit (exit 77 when offline), checks
     pinned file hashes, and runs its own package and certificate verifiers;
  3. regenerates one collision at each of the four reduced field sizes with the
     package's seeded search and verifies those too;
  4. independently re-checks every saved certificate with integer products of
     CHAMP's generators and the CHAMP finalization at the reduced prime, with a
     one-bit mutation control and a check that the pairs do not collide in the
     submitted full-size libraries.

It does not run, or establish the cost of, a full-size collision search.
Set NGCC_CHAMP_PACKAGE to a local copy of the pinned package to run offline.
"""

import ctypes
import hashlib
import io
import json
import os
import random
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CAND = ROOT / "hash-04"
ID = "hash-04-2"
COMMIT = "ec52ce8cc917280f10f796f6989918e49d1efb9f"
REPO = "https://github.com/amcrypto-jp/champ-cryptanalysis"
TARBALL = f"https://codeload.github.com/amcrypto-jp/champ-cryptanalysis/tar.gz/{COMMIT}"
PINNED = {
    "verify_package.py": "a58153298881dcf140503c2a0534b6ac0de90b99845a066aa02e6d4dfc50cf0d",
    "SHA256SUMS": "169b8b58c5d41dea0ee89e67c517bfc76e030f55e9f99838da2402afa6cfacb1",
    "code/check_collisions.py": "0e4d81733b8e27d2f22fb52364499d497a4334ba826695969901227f55eb8bfb",
    "code/subgroup.py": "5c772f6df674391f7101088c25da0dd9f9acd070008595a21fb88be95be59066",
    "code/model.py": "072fb12a5e239b5db4a17a4b54474bab403ff22ca2879f2367a517363f0f335e",
    "data/collisions.json": "74b0011ea107c899fadd1d2417cf88eb526594d0e5855a01b5228a52589a4270",
}

# Submitted generators (CryptHash_AlgorithmInstance.c:18-19, stored column-major
# at :37-47): A for bit 0, B for bit 1, multiplied on the right, MSB first.
A = ((-2, 1), (4, -3))
B = ((-5, 2), (-6, 2))
FULL = {512: (1 << 128) - 15449, 1024: (1 << 256) - 36113}


def integer_product(bits):
    """Product of the generators over Z, without any reduction."""
    m = ((1, 0), (0, 1))
    for bit in bits:
        g = B if bit == "1" else A
        m = tuple(tuple(sum(m[i][k] * g[k][j] for k in range(2)) for j in range(2))
                  for i in range(2))
    return m


def finalize(matrix, p):
    """Invert each nonzero entry mod p; serialize column-major, little-endian
    (CryptHash_AlgorithmInstance.c:297-320)."""
    width = (p.bit_length() + 7) // 8
    out = b""
    for value in (matrix[0][0], matrix[1][0], matrix[0][1], matrix[1][1]):
        value %= p
        out += (pow(value, -1, p) if value else 0).to_bytes(width, "little")
    return out


def model_digest(bits, p):
    return finalize(integer_product_mod(bits, p), p)


def integer_product_mod(bits, p):
    m = ((1, 0), (0, 1))
    for bit in bits:
        g = B if bit == "1" else A
        m = tuple(tuple(sum(m[i][k] * g[k][j] for k in range(2)) % p for j in range(2))
                  for i in range(2))
    return m


def library(bits_out):
    function = ctypes.CDLL(str(CAND / "lib" / f"libCHAMP-{bits_out}.so")).CryptHash
    function.argtypes = (ctypes.c_int, ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p)
    function.restype = ctypes.c_int

    def digest(bit_string):
        padded = bit_string.ljust((len(bit_string) + 7) // 8 * 8, "0")
        data = (int(padded, 2).to_bytes(len(padded) // 8, "big") if padded else b"") + bytes(8)
        source = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        output = (ctypes.c_ubyte * (bits_out // 8))()
        if function(bits_out, source, len(bit_string), output) != 0:
            raise RuntimeError(f"CHAMP-{bits_out}: CryptHash failed")
        return bytes(output)

    return digest


def obtain_package(workdir):
    local = os.environ.get("NGCC_CHAMP_PACKAGE")
    if local:
        return Path(local).resolve()
    try:
        with urllib.request.urlopen(TARBALL, timeout=60) as response:
            payload = response.read()
    except Exception as error:  # offline, proxy, HTTP error
        print(f"SKIP {ID}: cannot fetch {REPO} at {COMMIT}: {error}")
        return None
    with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
        for member in archive.getmembers():
            name = Path(member.name)
            if name.is_absolute() or ".." in name.parts or not (member.isfile() or member.isdir()):
                raise SystemExit(f"NOT CONFIRMED {ID}: unsafe archive member {member.name}")
        if hasattr(tarfile, "data_filter"):
            archive.extractall(workdir, filter="data")
        else:
            archive.extractall(workdir)
    return Path(workdir) / f"champ-cryptanalysis-{COMMIT}"


def run(command, cwd):
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True, timeout=600,
                            env={"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                                 "PYTHONDONTWRITEBYTECODE": "1"})
    return result.returncode, (result.stdout + result.stderr).strip().splitlines()


def main():
    missing = [str(CAND / "lib" / f"libCHAMP-{n}.so") for n in FULL
               if not (CAND / "lib" / f"libCHAMP-{n}.so").is_file()]
    if missing:
        print(f"SKIP {ID}: missing {', '.join(missing)} (build: make -C hash-04)")
        return 77
    libs = {n: library(n) for n in FULL}

    # 1. Independent model equals the submitted libraries.
    rng = random.Random(4)
    for n, p in FULL.items():
        for length in (0, 1, 7, 8, 13, 64, 255, 1000):
            word = "".join(rng.choice("01") for _ in range(length))
            if model_digest(word, p) != libs[n](word):
                print(f"NOT CONFIRMED {ID}: CHAMP model disagrees with CHAMP-{n}")
                return 1
        print(f"[model]    CHAMP-{n}: generator/finalization model equals library on 8 lengths")

    with tempfile.TemporaryDirectory(prefix="champ-package-") as work:
        package = obtain_package(work)
        if package is None:
            return 77
        # 2. Pinned content and the package's own verifiers.
        for name, expected in PINNED.items():
            path = package / name
            got = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else "missing"
            if got != expected:
                print(f"NOT CONFIRMED {ID}: {name} does not match the pinned commit ({got})")
                return 1
        print(f"[package]  {REPO} @ {COMMIT[:12]}: pinned file hashes match")
        python = sys.executable
        rc, out = run([python, "-I", "-B", str(package / "verify_package.py")], work)
        print(f"[package]  verify_package.py: {out[-1] if out else rc}")
        if rc:
            return 1
        rc, out = run([python, "-I", "-B", str(package / "code" / "check_collisions.py")], work)
        print(f"[package]  check_collisions.py (saved): {out[-1] if out else rc}")
        if rc:
            return 1

        # 3. Regenerate one seeded collision per reduced field size.  subgroup.py
        # imports its sibling model.py, so isolation is limited to -s -E.
        regenerated = Path(work) / "regenerated.json"
        rc, out = run([python, "-s", "-E", "-B", str(package / "code" / "subgroup.py"),
                       "--bits", "16", "20", "24", "28", "--trials", "1",
                       "--output", str(regenerated)], work)
        print(f"[package]  subgroup.py --bits 16 20 24 28 --trials 1: {out[-1] if out else rc}")
        if rc:
            return 1
        rc, out = run([python, "-I", "-B", str(package / "code" / "check_collisions.py"),
                       str(regenerated)], work)
        print(f"[package]  check_collisions.py (regenerated): {out[-1] if out else rc}")
        if rc:
            return 1
        saved = json.loads((package / "data" / "collisions.json").read_text())
        fresh = json.loads(regenerated.read_text())

    # 4. Independent re-check of every saved and regenerated certificate.
    stable = lambda r: {k: v for k, v in r.items() if k != "seconds"}
    seed0 = [stable(r) for r in saved if r["seed"] == 0]
    replay = [stable(r) for r in fresh] == seed0
    print(f"[replay]   regenerated seed-0 records equal the saved records: {replay}")
    ok = replay
    for record in saved + fresh:
        u, v, p = record["u"], record["v"], record["p"]
        U, V = integer_product(u), integer_product(v)
        good = (u != v and len(u) == len(v) == record["collision_bits"]
                and set(u + v) <= {"0", "1"} and U != V
                and all((x - y) % p == 0 for rx, ry in zip(U, V) for x, y in zip(rx, ry))
                and finalize(U, p) == finalize(V, p) == bytes.fromhex(record["digest_le"]))
        mutated = v[:-1] + ("1" if v[-1] == "0" else "0")
        control = finalize(integer_product(mutated), p) != finalize(U, p)
        full = all(libs[n](u) != libs[n](v) for n in FULL)
        ok &= good and control and full
        if record is saved[-1] or not (good and control and full):
            print(f"[recheck]  p={p} ({p.bit_length()} bits), seed {record['seed']}, "
                  f"{len(u)}-bit words: integer-product collision {good}; "
                  f"one-bit mutation differs {control}; distinct in CHAMP-512/1024 libraries {full}")
    print(f"[recheck]  {len(saved)} saved + {len(fresh)} regenerated certificates rechecked "
          "independently")

    if ok:
        print(f"CONFIRMED {ID} (Lead evidence): pinned subgroup-search certificates are exact equal-length "
              "positive-word CHAMP collisions at 16-28-bit safe primes; full-size cost "
              "(about 2^64 / 2^128) remains a projection")
        return 0
    print(f"NOT CONFIRMED {ID}: a certificate or control failed")
    return 1


if __name__ == "__main__":
    sys.exit(main())
