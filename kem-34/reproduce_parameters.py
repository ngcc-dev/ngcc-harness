#!/usr/bin/env python3
"""Check the three inexpensive Rudraksh2 parameter/source witnesses."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations/Reference_Implementation/lwekem128"
M4 = ROOT / "Implementations/Additional_Implementations/Cortex_M4"


def macro(path: Path, name: str) -> int:
    match = re.search(rf"^\s*#\s*define\s+{name}\s+([0-9]+)", path.read_text(errors="replace"), re.M)
    if not match:
        raise RuntimeError(f"{name} not found in {path}")
    return int(match.group(1))


def main() -> int:
    failures = 0

    params = REF / "params.h"
    symbytes = macro(params, "KEM_SYMBYTES")
    kem = (REF / "KEM_lwekem128.c").read_text(errors="replace")
    message_path = (
        symbytes == 16
        and "get_random_number (&drng_algorithm, buf, KEM_SYMBYTES * 8)" in kem
        and "hash_g (kr, buf, 2 * KEM_SYMBYTES)" in kem
        and "memcpy (ss, kr, KEM_SYMBYTES)" in kem
    )
    print(
        "ATTACK rudraksh-message-dimension lwekem128 "
        f"{'CONFIRMED' if message_path else 'NOT-CONFIRMED'} "
        f"encapsulation message={8 * symbytes} bits"
    )
    failures += not message_path

    beta_rows = []
    for level in (128, 256):
        ref_beta = macro(ROOT / f"Implementations/Reference_Implementation/lwekem{level}/minal.c", "MINAL_BETA")
        m4_beta = macro(
            M4 / f"lwekem{level}/crypto_kem/Rudraksh2/Rudraksh2/minal.c",
            "MINAL_BETA",
        )
        beta_rows.append((level, ref_beta, m4_beta))
    beta_bad = all(ref == 220 and m4 == 0 for _, ref, m4 in beta_rows)
    print(
        "ATTACK rudraksh-m4-beta lwekem128/256 "
        f"{'CONFIRMED' if beta_bad else 'NOT-CONFIRMED'} "
        + ", ".join(f"L{level}: reference={ref}, Cortex-M4={m4}" for level, ref, m4 in beta_rows)
    )
    failures += not beta_bad

    ntt_rows = []
    for level, n in ((128, 64), (256, 128), (512, 256)):
        q = 4001
        supported = (q - 1) % (2 * n) == 0
        ntt_rows.append((level, n, supported))
    ntt_bad = not any(supported for _, _, supported in ntt_rows)
    print(
        "ATTACK rudraksh-ii-ntt Rudraksh2-II "
        f"{'CONFIRMED' if ntt_bad else 'NOT-CONFIRMED'} "
        "q-1=4000 has v2=5; maximum negacyclic NTT degree=16, requested n=64/128/256"
    )
    failures += not ntt_bad

    # Both API lengths are named in the definitions but never consulted.
    enc_body = kem[kem.index("kem_enc ("):kem.index("kem_dec (")]
    dec_body = kem[kem.index("kem_dec ("):]
    ignored = enc_body.count("pk_len_bytes") == 1 and dec_body.count("ct_len_bytes") == 1
    print(
        "ATTACK rudraksh-length-contract lwekem128 "
        f"{'CONFIRMED' if ignored else 'NOT-CONFIRMED'} "
        "pk_len_bytes and ct_len_bytes occur only as formal parameters"
    )
    failures += not ignored
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
