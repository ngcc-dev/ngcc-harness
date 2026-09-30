#!/usr/bin/env python3
"""Trace TriQ-KEX's decrypted-message-dependent KEM re-encryption sampler."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parent
BASE = ROOT / "TriQ-KEX/Implementations/Reference_Implementation"


def check(level: int) -> bool:
    tree = BASE / f"TriQ-KEX-{level}"
    kem = (tree / "src/common/kem.c").read_text(errors="replace")
    pke = (tree / "src/ref/triq_pke.c").read_text(errors="replace")
    vector = (tree / "src/ref/vector.c").read_text(errors="replace")

    decrypt = kem.index("triq_pke_decrypt((uint64_t *)m_prime")
    derive = kem.index("hash_g(K_theta_prime", decrypt)
    reencrypt = kem.index("triq_pke_encrypt(&c_kem_prime_t", derive)
    compare = kem.index("vect_compare", reencrypt)
    sampler_calls = pke.count("vect_sample_fixed_weight_bd(&theta_xof_ctx")
    loop = vector[vector.index("void vect_sample_fixed_weight_bd"):]
    variable_loop = (
        "for (iter = 0; iter < N_max; iter++)" in loop
        and "vect_generate_random_support2(ctx, support, weight);" in loop
        and "if (!vect_check_bounded_density" in loop
        and "break;" in loop
    )
    return decrypt < derive < reencrypt < compare and sampler_calls == 2 and variable_loop


def main() -> int:
    results = {level: check(level) for level in (128, 256, 384, 512)}
    ok = all(results.values())
    print(
        "ATTACK triq-reencrypt-sampler TriQ-KEX-128/256/384/512 "
        f"{'CONFIRMED' if ok else 'NOT-CONFIRMED'} "
        "m_prime -> theta_prime -> two bounded-density rejection samplers before ciphertext comparison; "
        + ", ".join(f"L{level}={'yes' if value else 'no'}" for level, value in results.items())
    )
    print("LIMITATION source/dataflow witness only; full key recovery is not demonstrated")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
