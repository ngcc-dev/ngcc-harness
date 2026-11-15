#!/usr/bin/env python3
"""Reproduce SNOVA-9: vinegar hash absorbs 64 digest bytes, target hash len_digest.

The internal deterministic signing function of the reference code takes a
digest pointer, a digest length and a caller-supplied salt.  The vinegar is
SHAKE256(private seed || digest[0..63] || salt || attempt): exactly
BYTES_DIGEST = 64 digest bytes.  The target is SHAKE256(pkd || digest || salt)
over all ``len_digest`` bytes.  Two digests that agree in their first 64 bytes
therefore get the same vinegar and different targets.

This script

1. checks the cited source lines against pinned SHA-256 values,
2. compiles the unmodified reference sources of SNOVA_I_K, SNOVA_V_B and
   SNOVA_I_S (no submitted build file is run) with a small embedded driver,
3. signs four 80-byte digests with one key and one salt through the internal
   API, and two messages through ``crypto_sign`` with a repeated salt,
4. analyses the signatures with GF(16) arithmetic in pure Python, using the
   private seed that the driver prints: the vinegar of each signature is
   recovered as X_v + T12*X_o and compared with a hashlib recomputation, the
   XOR of two signatures is tested for membership in the oil space [T12; I],
   and the rank of span{(S^a applied blockwise) * columns of the XOR} is
   computed (the S-power maps are public).

Controls: a digest that differs inside the first 64 bytes, and two different
messages signed through ``crypto_sign`` with the same salt, give unrelated
vinegar; their XOR is not in the oil space.  A third control builds a temporary
copy of ``snova_ref.c`` (SNOVA_I_K) in which the vinegar hash absorbs
``len_digest`` bytes: the relation disappears.  That copy is a control, not a
proposed repair.

Limits: the defect is reachable only through the internal API
(``SNOVA_NAMESPACE(sign)``) with a caller-chosen digest length and a repeated
salt.  ``crypto_sign`` always passes a 64-byte digest and a fresh salt and is
not affected.  The oil-space test uses the known private key; no key recovery
from public data is performed.  The optimized and additional trees are covered
by a source check only.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (no C compiler).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


HERE = Path(__file__).resolve().parent
SCHEME = HERE.name
ROUND3 = HERE / "submission/SNOVA_Round3"
REF = ROUND3 / "Reference_Implementation"

CANDIDATE = "SNOVA-9"
EVIDENCE = (
    "Through the internal signing function of the SNOVA reference code, two 80-byte "
    "digests that agree in their first 64 bytes and share a caller-supplied salt get "
    "the same vinegar and different targets, so the XOR of the two valid signatures "
    "lies in the secret oil space (checked against T12 from the known private key); "
    "this concerns the internal API only, the NIST API crypto_sign always passes a "
    "64-byte digest and is not affected, and no key recovery from public data is run."
)

JOBS = 3
DIGEST_BYTES = 80
BYTES_DIGEST = 64
BYTES_SALT = 16

# Reference files, identical in all 18 parameter-set directories.
COMMON_SHA256 = {
    "snova_ref.c": "ef5dc33896f799c19631f888dd8c9192565e219df54e7932849188ae2e7e117c",
    "symmetric_ref.c": "88af5829f0bd9dfbff74b58abda0fa18abc5835c45be304bb26d2edab02e204e",
    "sign.c": "d79c85f77529a7f9d3787ca909c931c76dce14ba09ccf75e6b956bb1b7be8ddc",
    "aes.c": "42936e69655a11a6443c65e21e05f32a4861c9aa7c949a4674995f12c651ae64",
    "rng.c": "919b953692022f10b87ba9e28fba14c31e2d75d3d3986b72516b13e5094aef04",
    "snova.h": "91b38ca19ec6f12d34fb87a1cf5e299c5e71d4c96199f47e8714472579e5274c",
    "symmetric.h": "9740e4b354175edd9e7686b898c571c12e75c873ab99095bcc64eeaaa5ee7982",
    "api.h": "7bb2d75096a2d489eb32acf75777c7cf83b4881dbe8b84bf8b0fa7646e7ec192",
    "rng.h": "9e35940ba45d85f419780a79de232d3f98ea15695c701053c884eda3ea537a40",
    "keccak_opt64.h": "f1c22592d19e7353abd9df8a8b5471162a76f7a446b85a904b787fba6e47b5e8",
}
SOURCES = ("snova_ref.c", "symmetric_ref.c", "sign.c", "aes.c", "rng.c")

# Parameter sets run, with the rank of span{S^a-images of the XOR} that the
# review of 2026-11-15 states ("12 of 12", "25 of 25", "16 of 20").
SETS = {
    "SNOVA_I_K": {
        "params_sha256": "38c5eea6fadab312fede97ca97a0b27f30ab68c8b36523b0e9184c99c00925b8",
        "review_rank": 12,
    },
    "SNOVA_V_B": {
        "params_sha256": "aef0ca4df1f998d1d6b9a8031f69d1d6667d71173dc6d9395e6cf5e455c11010",
        "review_rank": 25,
    },
    "SNOVA_I_S": {
        "params_sha256": "b0e6afa6b23506a6d5df886d7751f009e122d1e0cbb09417d34317c149fbba2e",
        "review_rank": 16,
    },
}

# Cited lines (1-based).  They are meaningful because the hashes are pinned.
REF_LINES = {
    "vinegar": ("snova_ref.c", 882, "\t\tshake_absorb(&v_instance, digest, BYTES_DIGEST);\n"),
    "target": ("snova_ref.c", 501, "\tshake_absorb(&state, digest, len_digest);\n"),
    "target_call": ("snova_ref.c", 853,
                    "\thash_combined(sign_hashb, digest, len_digest, "
                    "seed + SEED_LENGTH_PUBLIC + SEED_LENGTH_PRIVATE, salt);\n"),
    "gf16": ("snova_ref.c", 155,
             "\tuint8_t F_star[15] = {1, 2, 4, 8, 3, 6, 12, 11, 5, 10, 7, 14, 15, 13, 9};"
             "  // Z2[x]/(x^4+x+1)\n"),
    "api_digest": ("sign.c", 54, "\tshake256(digest, BYTES_DIGEST, m, mlen);\n"),
    "api_call": ("sign.c", 56, "\tres = SNOVA_NAMESPACE(sign)(&skx_d, sig, digest, BYTES_DIGEST, salt);\n"),
    "digest_len": ("snova.h", 62, "#define BYTES_DIGEST 64\n"),
}

# The same two lines in the trees that are not built here (source check only).
OTHER_TREES = [
    ("Optimized_Implementation/SNOVA_I_K/snova_opt_16.c",
     "be17fd5e17ac4adc4892a9c844d0f16ca6932bc693db00d8d4084f5fbae9ed4e",
     (784, "\t\tshake_absorb(&v_instance, digest, BYTES_DIGEST);\n"),
     (755, "\thash_combined(sign_hashb, digest, len_digest, skx->pk_hash, salt);\n")),
    ("Optimized_Implementation/SNOVA_V_B/snova_opt_16_5.c",
     "9937528115964adc20dc06927f7a2b2ea6bf2ff5200cd48353b8aa6d86d55be0",
     (806, "\t\tshake_absorb(&v_instance, digest, BYTES_DIGEST);\n"),
     (777, "\thash_combined(sign_hashb, digest, len_digest, skx->pk_hash, salt);\n")),
    ("Additional_Implementations/AVX2/SNOVA_I_K/src/snova_core/snova_rect_sign.h",
     "0d21025a4e105b9b54639d9cc93889182e9817efd876cc1e916f1652b61dfdac",
     (302, "        shake_absorb(&v_instance, digest, BYTES_DIGEST);\n"),
     (198, "    hash_combined(sign_hashb, digest, len_digest, "
           "seed + SEED_LENGTH_PUBLIC + SEED_LENGTH_PRIVATE, salt);\n")),
]

# Control build: one changed line in a temporary copy of snova_ref.c.
COPY_SET = "SNOVA_I_K"
COPY_OLD = "\t\tshake_absorb(&v_instance, digest, BYTES_DIGEST);\n"
COPY_NEW = "\t\tshake_absorb(&v_instance, digest, len_digest);\n"

# Digest index -> (role, byte flipped relative to digest 0).
DIGESTS = {
    0: ("base", None),
    1: ("differs in byte 79 (outside the first 64 bytes; the review's pair)", 79),
    2: ("differs in byte 64 (first byte not absorbed by the vinegar hash)", 64),
    3: ("control: differs in byte 63 (inside the first 64 bytes)", 63),
}

DRIVER_C = r"""
/* Driver for the SNOVA reference internal signing API (reproducer harness,
 * not submission code).  The key comes from crypto_sign_keypair with the
 * submitted DRBG seeded with 0,1,...,47. */
#include <stdio.h>
#include <string.h>
#include "api.h"
#include "rng.h"

#define NDIG 4
#define DLEN 80

static void hex(const char *name, int k, const unsigned char *x, size_t n) {
    if (k >= 0) printf("%s%d=", name, k); else printf("%s=", name);
    for (size_t i = 0; i < n; i++) printf("%02x", x[i]);
    printf("\n");
}

int main(void) {
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
    static unsigned char sig[NDIG][CRYPTO_BYTES], sm[2][CRYPTO_BYTES + 8], opened[CRYPTO_BYTES + 8];
    static expanded_SK skx;
    static expanded_PK pkx;
    unsigned char entropy[48], d[NDIG][DLEN], salt[BYTES_SALT];
    const unsigned char msg[2][8] = {"SE-2 m0", "SE-2 m1"};

    for (int i = 0; i < 48; i++) entropy[i] = (unsigned char)i;
    randombytes_init(entropy, NULL, 256);
    if (crypto_sign_keypair(pk, sk)) return 2;
    if (SNOVA_NAMESPACE(sk_expand)(&skx, sk) || SNOVA_NAMESPACE(pk_expand)(&pkx, pk)) return 3;

    /* Four 80-byte "digests": 1 and 2 differ from 0 only after byte 63,
     * 3 differs from 0 in byte 63.  One caller-supplied salt for all. */
    for (int i = 0; i < DLEN; i++) d[0][i] = (unsigned char)(3 * i + 1);
    for (int k = 1; k < NDIG; k++) memcpy(d[k], d[0], DLEN);
    d[1][79] ^= 1;
    d[2][64] ^= 1;
    d[3][63] ^= 1;
    memset(salt, 0x42, sizeof salt);

    printf("name=%s\n", CRYPTO_ALGNAME);
    hex("pk", -1, pk, sizeof pk);
    hex("sk", -1, sk, sizeof sk);
    hex("salt", -1, salt, sizeof salt);
    for (int k = 0; k < NDIG; k++) {
        int rc = SNOVA_NAMESPACE(sign)(&skx, sig[k], d[k], DLEN, salt);
        printf("sign_rc%d=%d\n", k, rc);
        hex("digest", k, d[k], DLEN);
        hex("sig", k, sig[k], CRYPTO_BYTES);
    }
    for (int k = 0; k < NDIG; k++) {
        printf("verify_own%d=%d\n", k, SNOVA_NAMESPACE(verify)(&pkx, sig[k], d[k], DLEN));
        printf("verify_other%d=%d\n", k, SNOVA_NAMESPACE(verify)(&pkx, sig[k], d[(k + 1) % NDIG], DLEN));
    }

    /* NIST API control: two messages; the DRBG is reset before each call so
     * that crypto_sign draws the same salt twice. */
    for (int k = 0; k < 2; k++) {
        unsigned long long smlen = 0, mlen = 0;
        for (int i = 0; i < 48; i++) entropy[i] = (unsigned char)(0xa5 ^ i);
        randombytes_init(entropy, NULL, 256);
        int rc = crypto_sign(sm[k], &smlen, msg[k], sizeof msg[k], sk);
        int open_rc = crypto_sign_open(opened, &mlen, sm[k], smlen, pk);
        printf("api_sign_rc%d=%d\n", k, rc);
        printf("api_open_rc%d=%d\n", k, open_rc || mlen != sizeof msg[k] || memcmp(opened, msg[k], sizeof msg[k]));
        hex("api_msg", k, msg[k], sizeof msg[k]);
        hex("api_sm", k, sm[k], (size_t)smlen);
    }
    return 0;
}
"""


def sha256(path: Path) -> str:
    if not path.is_file():
        return "missing"
    return hashlib.sha256(path.read_bytes()).hexdigest()


def line(path: Path, number: int) -> str:
    text = path.read_text(encoding="utf-8").splitlines(keepends=True)
    return text[number - 1] if 0 < number <= len(text) else ""


def read_params(path: Path) -> dict:
    found = dict(re.findall(r"#define\s+SNOVA_(\w+)\s+(\w+)", path.read_text(encoding="utf-8")))
    return {k: int(found[k]) for k in ("v", "o", "q", "l", "r", "m1")}


# --------------------------------------------------------------------------
# Source checks
# --------------------------------------------------------------------------

def source_checks() -> tuple[dict, dict, list]:
    checks: dict = {}
    pinned: dict = {}
    problems: list = []

    def pin(path: Path, expected: str) -> bool:
        rel = f"{SCHEME}/{path.relative_to(HERE).as_posix()}"
        actual = sha256(path)
        pinned[rel] = actual
        if actual != expected:
            problems.append(f"sha256 mismatch {rel}: {actual}")
            return False
        return True

    ok = True
    for set_name, entry in SETS.items():
        directory = REF / set_name
        ok &= pin(directory / "snova_params.h", entry["params_sha256"])
        for name, digest in COMMON_SHA256.items():
            ok &= pin(directory / name, digest)
    for rel, digest, _, _ in OTHER_TREES:
        ok &= pin(ROUND3 / rel, digest)
    checks["source_sha256_pinned_files_match"] = ok

    set_dirs = sorted(p for p in REF.iterdir() if p.is_dir()) if REF.is_dir() else []
    checks["source_all_18_reference_sets_share_the_pinned_signer_and_wrapper"] = (
        len(set_dirs) == 18 and all(
            sha256(directory / name) == COMMON_SHA256[name]
            for directory in set_dirs for name in ("snova_ref.c", "sign.c", "snova.h")))

    def cited(*keys: str) -> bool:
        if not ok:
            return False
        return all(line(REF / set_name / REF_LINES[key][0], REF_LINES[key][1]) == REF_LINES[key][2]
                   for set_name in SETS for key in keys)

    checks["source_vinegar_hash_absorbs_fixed_BYTES_DIGEST"] = cited("vinegar", "digest_len")
    checks["source_target_hash_absorbs_len_digest"] = cited("target", "target_call")
    checks["source_crypto_sign_always_passes_BYTES_DIGEST"] = cited("api_digest", "api_call", "digest_len")
    checks["source_gf16_modulus_as_modelled"] = cited("gf16")
    checks["source_same_two_lines_in_optimized_and_additional_trees"] = ok and all(
        line(ROUND3 / rel, vinegar[0]) == vinegar[1] and line(ROUND3 / rel, target[0]) == target[1]
        for rel, _, vinegar, target in OTHER_TREES)
    return checks, pinned, problems


# --------------------------------------------------------------------------
# Build and run
# --------------------------------------------------------------------------

def run(cmd, cwd: Path, timeout: int) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], cwd=cwd, capture_output=True, text=True,
                          timeout=timeout, env=dict(os.environ, LC_ALL="C"))


def build_and_run(compiler: str, work: Path, set_name: str, changed: bool = False) -> dict:
    directory = REF / set_name
    sources = [directory / name for name in SOURCES]
    binary = work / f"dump_{set_name}{'_changed' if changed else ''}"
    if changed:
        # Patched copy in the temporary directory; the submission is not touched.
        text = sources[0].read_text(encoding="utf-8")
        if text.count(COPY_OLD) != 1:
            raise RuntimeError("anchor line does not occur exactly once in snova_ref.c")
        sources[0] = work / f"changed_{set_name}_snova_ref.c"
        sources[0].write_text(text.replace(COPY_OLD, COPY_NEW), encoding="utf-8")
    # Flags of the submitted reference Makefile without its warning options.
    cmd = [compiler, "-std=c11", "-O3", "-DSNOVA_REF=1", f"-I{directory}",
           *sources, work / "se2_driver.c", "-o", binary]
    built = run(cmd, work, 300)
    if built.returncode:
        raise RuntimeError(f"build failed ({binary.name}): {built.stderr[-800:]}")
    done = run([binary], work, 120)
    if done.returncode:
        raise RuntimeError(f"driver failed ({binary.name}): rc={done.returncode} {done.stderr[-400:]}")
    values = dict(row.split("=", 1) for row in done.stdout.splitlines() if "=" in row)
    if values.get("name") != set_name:
        raise RuntimeError(f"driver output for {set_name} is incomplete")
    return values


# --------------------------------------------------------------------------
# GF(16) = F2[x]/(x^4+x+1), matrices as lists of rows
# --------------------------------------------------------------------------

def _gf_mul(a: int, b: int) -> int:
    product = 0
    for bit in range(4):
        if (b >> bit) & 1:
            product ^= a << bit
    for bit in (6, 5, 4):
        if (product >> bit) & 1:
            product ^= 0x13 << (bit - 4)
    return product


MUL = [[_gf_mul(a, b) for b in range(16)] for a in range(16)]
INV = [0] + [next(b for b in range(1, 16) if MUL[a][b] == 1) for a in range(1, 16)]


def mat_mul(a: list, b: list) -> list:
    columns = list(zip(*b))
    out = []
    for row in a:
        table = [MUL[x] for x in row]
        new = []
        for column in columns:
            acc = 0
            for t, y in zip(table, column):
                acc ^= t[y]
            new.append(acc)
        out.append(new)
    return out


def mat_add(a: list, b: list) -> list:
    return [[x ^ y for x, y in zip(ra, rb)] for ra, rb in zip(a, b)]


def rank(matrix: list) -> int:
    rows = [row[:] for row in matrix]
    done = 0
    width = len(rows[0]) if rows else 0
    for column in range(width):
        pivot = next((i for i in range(done, len(rows)) if rows[i][column]), None)
        if pivot is None:
            continue
        rows[done], rows[pivot] = rows[pivot], rows[done]
        scale = MUL[INV[rows[done][column]]]
        rows[done] = [scale[x] for x in rows[done]]
        for i in range(len(rows)):
            if i != done and rows[i][column]:
                factor = MUL[rows[i][column]]
                rows[i] = [x ^ factor[y] for x, y in zip(rows[i], rows[done])]
        done += 1
    return done


def nibbles(data: bytes, count: int) -> list:
    """q=16 field decoding of the reference code: low nibble first."""
    out = []
    for byte in data:
        out.append(byte & 15)
        out.append(byte >> 4)
    if len(out) < count:
        raise ValueError("not enough bytes")
    return out[:count]


def to_matrix(flat: list, rows: int, cols: int) -> list:
    return [flat[i * cols:(i + 1) * cols] for i in range(rows)]


# --------------------------------------------------------------------------
# Model of the SNOVA objects needed here
# --------------------------------------------------------------------------

class Model:
    def __init__(self, params: dict, sk: bytes):
        self.v, self.o, self.l, self.r = params["v"], params["o"], params["l"], params["r"]
        self.n = self.v + self.o
        l = self.l
        # Symmetric matrix S (reference set_S) and its powers S^0..S^(l-1).
        s = [[8 - (i + j) for j in range(l)] for i in range(l)]
        if l == 5:
            s[4][4] = 9
        self.s_powers = [[[int(i == j) for j in range(l)] for i in range(l)]]
        for _ in range(1, l):
            self.s_powers.append(mat_mul(s, self.s_powers[-1]))
        self.private_seed = sk[16:48]
        # T12 (reference expand_T12): v x o blocks sum_k c[k]*S^k, c from SHAKE256(private seed).
        count = self.o * self.v * l
        coefficients = nibbles(hashlib.shake_256(self.private_seed).digest((count + 1) // 2), count)
        self.t12 = [[0] * (self.o * l) for _ in range(self.v * l)]
        for i in range(self.v):
            for j in range(self.o):
                c = coefficients[(i * self.o + j) * l:(i * self.o + j + 1) * l]
                for a in range(l):
                    for b in range(l):
                        value = 0
                        for k in range(l):
                            value ^= MUL[c[k]][self.s_powers[k][a][b]]
                        self.t12[i * l + a][j * l + b] = value

    def decode_signature(self, signature: bytes) -> list:
        """Signature bytes (without the 16-byte salt) -> (n*l) x r matrix, row-major."""
        return to_matrix(nibbles(signature[:-BYTES_SALT], self.n * self.l * self.r), self.n * self.l, self.r)

    def split(self, x: list) -> tuple[list, list]:
        return x[:self.v * self.l], x[self.v * self.l:]

    def in_oil_space(self, x: list) -> bool:
        """Every column of x lies in the column space of [T12; I]."""
        top, bottom = self.split(x)
        return top == mat_mul(self.t12, bottom)

    def vinegar_of(self, x: list) -> list:
        """The signer sets X_v = V + T12*X_o, so V = X_v + T12*X_o (characteristic 2)."""
        top, bottom = self.split(x)
        return mat_add(top, mat_mul(self.t12, bottom))

    def vinegar_hash(self, absorbed: bytes, salt: bytes, attempt: int) -> list:
        count = self.v * self.l * self.r
        stream = hashlib.shake_256(self.private_seed + absorbed + salt + bytes([attempt])).digest((count + 1) // 2)
        return to_matrix(nibbles(stream, count), self.v * self.l, self.r)

    def attempt_of(self, x: list, absorbed: bytes, salt: bytes) -> int:
        """Attempt counter whose vinegar hash over `absorbed` equals the vinegar of x (0: none)."""
        vinegar = self.vinegar_of(x)
        for attempt in range(1, 255):
            if self.vinegar_hash(absorbed, salt, attempt) == vinegar:
                return attempt
        return 0

    def s_images(self, x: list) -> list:
        """[ (S^0 blockwise)*x | (S^1 blockwise)*x | ... ]: an (n*l) x (l*r) matrix."""
        l = self.l
        out = [[] for _ in range(self.n * l)]
        for power in self.s_powers:
            for block in range(self.n):
                image = mat_mul(power, x[block * l:(block + 1) * l])
                for a in range(l):
                    out[block * l + a].extend(image[a])
        return out


def analyse(set_name: str, values: dict) -> tuple[dict, dict]:
    params = read_params(REF / set_name / "snova_params.h")
    sk = bytes.fromhex(values["sk"])
    salt = bytes.fromhex(values["salt"])
    model = Model(params, sk)
    digests = [bytes.fromhex(values[f"digest{k}"]) for k in range(4)]
    signatures = [bytes.fromhex(values[f"sig{k}"]) for k in range(4)]
    x = [model.decode_signature(s) for s in signatures]
    attempts = [model.attempt_of(x[k], digests[k][:BYTES_DIGEST], salt) for k in range(4)]
    vinegar = [model.vinegar_of(xk) for xk in x]
    oil_dim = model.o * model.l

    def pair(k: int) -> dict:
        delta = mat_add(x[0], x[k])
        images = model.s_images(delta)
        return {
            "digest_bytes_differing_from_base": [i for i in range(DIGEST_BYTES) if digests[0][i] != digests[k][i]],
            "first_64_digest_bytes_equal": digests[0][:BYTES_DIGEST] == digests[k][:BYTES_DIGEST],
            "same_vinegar": vinegar[0] == vinegar[k],
            "xor_nonzero_entries": sum(1 for row in delta for e in row if e),
            "xor_in_oil_space": model.in_oil_space(delta),
            "s_images_in_oil_space": model.in_oil_space(images),
            "rank_of_span_of_s_images": rank(images),
        }

    pairs = {f"pair_0_{k}": pair(k) for k in (1, 2, 3)}

    # NIST API control: same salt (DRBG reset), two different messages.
    api_sig = [bytes.fromhex(values[f"api_sm{k}"])[:len(signatures[0])] for k in range(2)]
    api_msg = [bytes.fromhex(values[f"api_msg{k}"]) for k in range(2)]
    api_x = [model.decode_signature(s) for s in api_sig]
    api_salt = [s[-BYTES_SALT:] for s in api_sig]
    api_attempts = [model.attempt_of(api_x[k], hashlib.shake_256(api_msg[k]).digest(BYTES_DIGEST), api_salt[k])
                    for k in range(2)]
    api_delta = mat_add(api_x[0], api_x[1])

    positives = (pairs["pair_0_1"], pairs["pair_0_2"])
    control = pairs["pair_0_3"]
    measured_rank = pairs["pair_0_1"]["rank_of_span_of_s_images"]
    checks = {
        "digests_are_80_bytes_and_differ_only_in_the_stated_byte": all(len(d) == DIGEST_BYTES for d in digests)
            and all(pairs[f"pair_0_{k}"]["digest_bytes_differing_from_base"] == [DIGESTS[k][1]] for k in (1, 2, 3)),
        "internal_api_signs_80_byte_digests": all(values[f"sign_rc{k}"] == "0" for k in range(4)),
        "each_signature_verifies_under_its_own_digest": all(values[f"verify_own{k}"] == "0" for k in range(4)),
        "no_signature_verifies_under_another_digest": all(values[f"verify_other{k}"] != "0" for k in range(4)),
        "signatures_differ": len(set(signatures)) == 4,
        "salt_is_the_caller_supplied_one": all(s[-BYTES_SALT:] == salt for s in signatures),
        "model_reproduces_vinegar_of_every_signature": all(attempts) and all(api_attempts),
        "same_vinegar_when_first_64_digest_bytes_agree": all(
            p["first_64_digest_bytes_equal"] and p["same_vinegar"] for p in positives)
            and attempts[0] == attempts[1] == attempts[2],
        "xor_of_signatures_lies_in_secret_oil_space": all(
            p["xor_nonzero_entries"] > 0 and p["xor_in_oil_space"] and p["s_images_in_oil_space"]
            for p in positives),
        "rank_of_span_equals_review_value": measured_rank == SETS[set_name]["review_rank"],
        "control_digest_differing_in_byte_63_gives_other_vinegar": (
            not control["first_64_digest_bytes_equal"] and not control["same_vinegar"]
            and not control["xor_in_oil_space"]),
        "control_crypto_sign_with_repeated_salt_not_in_oil_space": (
            values["api_sign_rc0"] == values["api_sign_rc1"] == "0"
            and values["api_open_rc0"] == values["api_open_rc1"] == "0"
            and api_salt[0] == api_salt[1] and api_sig[0] != api_sig[1]
            and model.vinegar_of(api_x[0]) != model.vinegar_of(api_x[1])
            and not model.in_oil_space(api_delta)),
    }
    measurements = {
        "parameters": {k: params[k] for k in ("v", "o", "l", "r", "m1")},
        "oil_space_dimension": oil_dim,
        "columns_available_l_times_r": model.l * model.r,
        "rank_of_span_of_s_images": measured_rank,
        "review_rank": SETS[set_name]["review_rank"],
        "signing_attempt_of_each_signature": attempts,
        **pairs,
        "crypto_sign_control": {
            "same_salt": api_salt[0] == api_salt[1],
            "signing_attempts": api_attempts,
            "xor_in_oil_space": model.in_oil_space(api_delta),
            "rank_of_span_of_s_images": rank(model.s_images(api_delta)),
        },
        "public_key_sha256": hashlib.sha256(bytes.fromhex(values["pk"])).hexdigest(),
    }
    return checks, measurements


def analyse_changed_copy(set_name: str, values: dict) -> tuple[bool, dict]:
    """Control: the copy whose vinegar hash absorbs len_digest bytes."""
    params = read_params(REF / set_name / "snova_params.h")
    salt = bytes.fromhex(values["salt"])
    model = Model(params, bytes.fromhex(values["sk"]))
    digests = [bytes.fromhex(values[f"digest{k}"]) for k in range(4)]
    x = [model.decode_signature(bytes.fromhex(values[f"sig{k}"])) for k in range(4)]
    attempts_full = [model.attempt_of(x[k], digests[k], salt) for k in range(4)]
    attempts_64 = [model.attempt_of(x[k], digests[k][:BYTES_DIGEST], salt) for k in range(4)]
    in_oil = {f"0_{k}": model.in_oil_space(mat_add(x[0], x[k])) for k in (1, 2, 3)}
    same_vinegar = {f"0_{k}": model.vinegar_of(x[0]) == model.vinegar_of(x[k]) for k in (1, 2, 3)}
    holds = (
        all(values[f"sign_rc{k}"] == "0" and values[f"verify_own{k}"] == "0" for k in range(4))
        and all(attempts_full) and not any(attempts_64)
        and not any(in_oil.values()) and not any(same_vinegar.values()))
    return holds, {
        "set": set_name,
        "changed_line": "snova_ref.c:882 absorbs len_digest instead of BYTES_DIGEST (temporary copy)",
        "signing_attempt_of_each_signature_80_bytes_absorbed": attempts_full,
        "vinegar_matches_64_byte_model": [bool(a) for a in attempts_64],
        "same_vinegar": same_vinegar,
        "xor_in_oil_space": in_oil,
    }


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def render(value, depth: int = 0) -> str:
    """JSON text: the top three levels one entry per line, deeper values inline."""
    if isinstance(value, dict) and value and depth < 3:
        pad = "  " * (depth + 1)
        rows = [f"{pad}{json.dumps(key)}: {render(value[key], depth + 1)}" for key in sorted(value)]
        return "{\n" + ",\n".join(rows) + "\n" + "  " * depth + "}"
    return json.dumps(value, sort_keys=True)


def emit(result: dict, output: Path | None) -> int:
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

    compiler = shutil.which("gcc") or shutil.which("cc")
    if not compiler:
        result["status"] = "SKIP"
        result["skip_reason"] = "no C compiler (gcc or cc) found"
        return emit(result, args.output)

    with tempfile.TemporaryDirectory(prefix="snova-se2-") as temporary:
        work = Path(temporary)
        (work / "se2_driver.c").write_text(DRIVER_C, encoding="utf-8")
        version = run([compiler, "--version"], work, 60).stdout.splitlines()
        result["measurements"]["compiler"] = version[0].strip() if version else ""
        result["measurements"]["build"] = ("<compiler> -std=c11 -O3 -DSNOVA_REF=1 -I<set> snova_ref.c "
                                           "symmetric_ref.c sign.c aes.c rng.c se2_driver.c (embedded)")
        result["measurements"]["digests"] = {str(k): role for k, (role, _) in DIGESTS.items()}
        try:
            jobs = [(name, False) for name in SETS] + [(COPY_SET, True)]
            with ThreadPoolExecutor(max_workers=JOBS) as pool:
                outputs = list(pool.map(lambda job: build_and_run(compiler, work, *job), jobs))
        except (RuntimeError, subprocess.TimeoutExpired, OSError) as error:
            result.setdefault("problems", []).append(str(error))
            return emit(result, args.output)

    per_set = {}
    try:
        for set_name, values in zip(SETS, outputs):
            per_set[set_name], result["measurements"][set_name] = analyse(set_name, values)
        copy_holds, result["measurements"]["control_copy_absorbing_len_digest"] = analyse_changed_copy(
            COPY_SET, outputs[-1])
    except (KeyError, ValueError) as error:
        result.setdefault("problems", []).append(f"bad driver output: {error!r}")
        return emit(result, args.output)
    # A check passes only if it holds for every parameter set run.
    for name in next(iter(per_set.values())):
        result["checks"][name] = all(per_set[set_name][name] for set_name in SETS)
    result["checks"]["control_copy_absorbing_len_digest_removes_the_relation"] = copy_holds
    failed = {set_name: [k for k, v in c.items() if not v] for set_name, c in per_set.items()}
    if any(failed.values()):
        result["failed_checks_by_set"] = {k: v for k, v in failed.items() if v}
    result["status"] = "PASS" if all(result["checks"].values()) else "FAIL"
    # The run time goes to stderr so that the JSON is identical on every run.
    print(f"elapsed {time.monotonic() - started:.1f} s", file=sys.stderr)
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
