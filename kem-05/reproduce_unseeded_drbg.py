#!/usr/bin/env python3
"""Reproduce BIKE-MLThre's deterministic default-build key generation."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC = ROOT / "Implementations" / "reference"
EXPECTED_PK_PREFIX = (
    "b505bc3a4fd4de9a86991932daad9337"
    "bf3343b2ea3697bd11443da3e1388027"
)

DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "api.h"
#include "kem.h"
#include "FromNIST/rng.h"

int main(int argc, char **argv) {
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    if (argc == 2) {
        unsigned char entropy[48];
        memset(entropy, argv[1][0] == '1' ? 0x11 : 0x22, sizeof entropy);
        randombytes_init(entropy, NULL, 256);
    }
    if (crypto_kem_keypair(pk, sk) != 0) return 2;
    for (size_t i = 0; i < 32; i++) printf("%02x", pk[i]);
    putchar('\n');
    return 0;
}
'''


def main() -> int:
    names = [
        "auxfunc", "conversions", "decode_ml", "hash_wrapper", "kem",
        "mlthre_model", "mlthre_policy", "mlthre_runtime_sampling", "ntl",
        "ring_buffer", "sampling", "threshold", "utilities", "xof_prng",
    ]
    with tempfile.TemporaryDirectory(prefix="bike-unseeded-") as directory:
        exe = Path(directory) / "witness"
        command = [
            "gcc", "-x", "c", "-", "-x", "none", "-O3", "-std=c99",
            f"-I{SRC}", "-DNIST_RAND=1", "-DBIKE_SECURITY_128",
            "-DBIKE_MLTHRE_ENABLED=1",
            *(str(SRC / f"{name}.c") for name in names),
            str(SRC / "FromNIST" / "rng.c"), "-lcrypto", "-lm", "-o", str(exe),
        ]
        subprocess.run(command, input=DRIVER, text=True, check=True)

        def run(*args: str) -> str:
            return subprocess.check_output([str(exe), *args], text=True).strip()

        unseeded_a = run()
        unseeded_b = run()
        seeded_a = run("1")
        seeded_b = run("2")

    assert unseeded_a == unseeded_b == EXPECTED_PK_PREFIX
    assert seeded_a != seeded_b
    assert seeded_a != unseeded_a and seeded_b != unseeded_a
    print(f"unseeded_pk_prefix={unseeded_a}")
    print("fresh_process_repeat=IDENTICAL")
    print("distinct_seed_control=DISTINCT")
    print("BIKE_UNSEEDED_DEFAULT_KEYGEN=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
