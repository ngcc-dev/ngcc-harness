#!/usr/bin/env python3
"""Certify submitted uses of the external ICCS DRBG as a protocol PRG/XOF."""

from __future__ import annotations

import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent

CASES = {
    "kem-01-3": ("kem-01", "Implementations/Reference_Implementation/Aigis-Enc+-*/hashkdf.c",
                 ("init_random_number(state, input, inlen);", "get_random_number(state, output, nblocks * 256);")),
    "sign-01-7": ("sign-01", "Implementations/Implementations/Reference_Implementation/Aigis-Sig+-*/hashkdf.c",
                  ("init_random_number(state, input, inlen);", "get_random_number(state, output, nblocks * 256);")),
    "kem-06-5": ("kem-06", "Implementations/Reference_Implementation/BRA-*/src/parsing.c",
                 ("random_source_seed_with_len(&sk_seedexpander, sk_seed", "Regenerate support sets and split (deterministic process matching keygen)")),
    "kem-07-3": ("kem-07", "Implementations/Reference_Implementation/BRQC-*/src/parsing.c",
                 ("random_source_seed_with_len(&sk_seedexpander, sk_seed", "Regenerate support sets and split (deterministic process matching keygen)")),
    "kem-10-4": ("kem-10", "Implementations/Reference_Implementation/CMultiURAG-*/src/parsing.c",
                 ("random_source_seed_with_len(&sk_seedexpander, sk_seed", "rbc_mat_set_random_from_support(&sk_seedexpander")),
    "sign-12-4": ("sign-12", "Implementations/Reference_Implementation/Galas-*/ngcc/SIG_AlgorithmInstance.c",
                  ("init_random_number(&drng, sd, sd_len);", "get_random_number(&drng, k,", "get_random_number(&drng, x,")),
    "kem-23-2": ("kem-23", "Implementations/Reference_Implementation/Mito-*/symmetric.c",
                 ("void xof_init(DRNG_ctx *xof_ctx", "init_random_number(xof_ctx, seed, seed_size);", "get_random_number(xof_ctx, output, output_size * 8);")),
    "sign-23-2": ("sign-23", "Implementation codes and test vectors/Shuttle */Implementations/Reference_Implementation/SHUTTLE-*/symmetric.c",
                  ("void xof256_init(xof_ctx *ctx", "init_random_number(ctx, seed", "get_random_number(ctx, out")),
    "kex-05-4": ("kex-05", "Implementations and Test_Vectors/Implementations/Reference_Implementation/LoomKEX-*/loom/symmetric-iccs.c",
                 ("void xof256_init(xof_ctx *ctx", "init_random_number(ctx, seed", "get_random_number(ctx, out")),
    "sign-29-2": ("sign-29", "Implementations/Reference_Implementation/Tins*/SIG_TINS*.c",
                  ("init_random_number(&pk_expander", "init_random_number(&sk_expander")),
    "sign-32-4": ("sign-32", "Implementations/Reference_Implementation/UVW-*/SIG_AlgorithmInstance.c",
                  ("static void init_seeded_drng_nonce", "init_random_number(drng, seeded", "generate_f3_matrix")),
    "kem-36-8": ("kem-36", "Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-*/src/sample.c",
                 ("void generate_error_vector", "init_random_number(&local_drng, hash_input", "get_random_number(&local_drng")),
    "kem-38-7": ("kem-38", "Implementations/Reference_Implementation/UVW-KEM-*/src/KEM_AlgorithmInstance.c",
                 ("init_random_number(&drng_h1_prime", "expand_GU_Dec(seed", "init_random_number(&drng_u")),
    "kem-40-3": ("kem-40", "Implementations/Reference_Implementation/yuanyang-*/kem.c",
                 ("static int yy_encrypt(", "init_random_number(&drng,seed", "yy_encrypt(output,seed,message,h);")),
}


def check(report_id: str) -> None:
    candidate, pattern, needles = CASES[report_id]
    paths = sorted((ROOT / candidate).glob(pattern))
    assert paths, f"{report_id}: no sources matched {pattern}"
    for path in paths:
        source = path.read_text(encoding="utf-8", errors="replace")
        for needle in needles:
            assert needle in source, f"{report_id}: {needle!r} absent from {path}"
    print(f"CONFIRMED {report_id}: {len(paths)} submitted source copy/copies reset the external DRBG for deterministic protocol expansion")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-id", choices=sorted(CASES))
    args = parser.parse_args()
    for report_id in ([args.report_id] if args.report_id else CASES):
        check(report_id)


if __name__ == "__main__":
    main()
