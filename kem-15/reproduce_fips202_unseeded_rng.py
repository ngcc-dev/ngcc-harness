#!/usr/bin/env python3
"""Check the four submitted FLIT FIPS202 variants' unseeded KEM RNG."""

from hashlib import sha256
from pathlib import Path
from platform import machine
from subprocess import run
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parent / "Implementations/Additional_Implementation"
CPU_FLAGS = Path("/proc/cpuinfo").read_text(errors="ignore").lower() if Path("/proc/cpuinfo").exists() else ""
OPT_SUPPORTED = machine().lower() in {"x86_64", "amd64"} and all(
    flag in CPU_FLAGS for flag in ("avx2", "bmi2", "pclmulqdq")
)
DRIVER = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "rng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    unsigned char seed[55], entropy[32];
    unsigned long long npk = kem_get_pk_len_bytes();
    unsigned long long nsk = kem_get_sk_len_bytes();
    unsigned long long nct = kem_get_ct_len_bytes();
    unsigned long long nss = kem_get_ss_len_bytes();
    unsigned char *pk = malloc(npk), *sk = malloc(nsk), *ct = malloc(nct);
    unsigned char *ss = malloc(nss), *ss_dec = malloc(nss);
    unsigned long long a, b, c, d, e;
    if (argc != 3 || !pk || !sk || !ct || !ss || !ss_dec) return 2;
    memset(seed, argv[1][0], sizeof seed);
    memset(entropy, argv[1][0], sizeof entropy);
    if (init_random_number(&drng_algorithm, seed, sizeof seed)) return 3;
    if (argv[2][0] == 'e') randombytes_init(entropy, NULL, 256);
    if (kem_keygen(pk, &a, sk, &b) || a != npk || b != nsk) return 4;
    if (kem_enc(pk, npk, ss, &c, ct, &d) || c != nss || d != nct) return 5;
    if (kem_dec(sk, nsk, ct, nct, ss_dec, &e) || e != nss || memcmp(ss, ss_dec, nss)) return 7;
    if (fwrite(pk, 1, npk, stdout) != npk || fwrite(sk, 1, nsk, stdout) != nsk ||
        fwrite(ct, 1, nct, stdout) != nct || fwrite(ss, 1, nss, stdout) != nss) return 6;
    return 0;
}
'''
SOURCES = (
    "KEM_AlgorithmInstance.c", "kem.c", "indcpa.c", "ntt.c", "poly.c",
    "decode.c", "packing.c", "reduce.c", "verify.c", "fips202.c",
    "rng.c", "symmetric-shake.c", "drng.c",
)


def check_tree(tree: Path, scratch: Path) -> None:
    rng = (tree / "rng.c").read_text()
    kat = (tree / "KAT_KEM.c").read_text()
    assert "static uint8_t  rng_seed[32] = {0};" in rng
    assert "static uint64_t rng_ctr   = 0;" in rng
    assert "init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES)" in kat
    assert "randombytes_init(" not in kat
    assert not any("randombytes_init(" in (tree / n).read_text() for n in
                   ("KEM_AlgorithmInstance.c", "kem.c", "indcpa.c"))
    if "OPT" in tree.name and not OPT_SUPPORTED:
        print(f"SKIP kem-15-3: {tree.name} runtime check needs AVX2/BMI2/PCLMUL; source checks passed")
        return
    driver = scratch / "driver.c"
    driver.write_text(DRIVER)
    binary = scratch / "test"
    command = ["cc", "-O2", "-std=c11", "-I", str(tree), "-o", str(binary),
               str(driver), *(str(tree / source) for source in SOURCES)]
    if "OPT" in tree.name:
        command.extend(("-mavx2", "-mbmi2", "-mpopcnt", "-mpclmul"))
        command.extend(str(tree / source) for source in
                       ("ntt_avx.c", "basemul_avx.c", "consts.c"))
    build = run(command, capture_output=True, text=True)
    if build.returncode:
        raise RuntimeError(f"{tree.name} build failed:\n{build.stderr}")
    def invoke(seed: str, explicit: bool) -> bytes:
        process = run([binary, seed, "explicit" if explicit else "api"], capture_output=True)
        if process.returncode:
            raise RuntimeError(f"{tree.name} run failed ({process.returncode})")
        return process.stdout
    a, b = invoke("A", False), invoke("B", False)
    control_a, control_b = invoke("A", True), invoke("B", True)
    if a != b or control_a == control_b or a == control_a:
        raise RuntimeError(f"{tree.name}: deterministic/API or explicit-seed control failed")
    print(f"CONFIRMED kem-15-3: {tree.name} API A/B={sha256(a).hexdigest()[:16]} "
          f"explicit A/B={sha256(control_a).hexdigest()[:8]}/{sha256(control_b).hexdigest()[:8]}")


with TemporaryDirectory(prefix="ngcc-flit-fips202-") as temporary:
    scratch = Path(temporary)
    trees = sorted(ROOT.glob("FLIT_FIPS202_*"))
    assert len(trees) == 4, f"expected four FIPS202 trees, found {len(trees)}"
    for tree in trees:
        check_tree(tree, scratch)
