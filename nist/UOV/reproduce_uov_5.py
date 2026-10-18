#!/usr/bin/env python3
"""Pin the recommended-signer/proved-signer difference and proof-bound arithmetic."""

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC_SHA256 = "ce9044adf1b4e407467968438fa66794882bcaa6411d3509bed72ff54380181a"
SOURCE_SHA256 = "863e59ecf3838fa8643c3a65e0bfa8cb86eeefaec7a31f1260767b2a7444affc"


def ordered(text, needles):
    position = -1
    for needle in needles:
        position = text.find(needle, position + 1)
        if position < 0:
            return False
    return True


def main():
    spec = ROOT / "UOV-spec.pdf"
    signer = ROOT / "submission/UOV-R3/Reference_Implementation/Ip/ov.c"
    signer_text = signer.read_text(encoding="utf-8")
    checks = {
        "pinned_specification": hashlib.sha256(spec.read_bytes()).hexdigest() == SPEC_SHA256,
        "pinned_recommended_signer": hashlib.sha256(signer.read_bytes()).hexdigest() == SOURCE_SHA256,
        "recommended_signer_fixes_salt_then_retries_vinegar": ordered(signer_text, [
            "randombytes( salt, _SALT_BYTE )",
            "hash_update(&h_m_salt_secret, message, mlen)",
            "hash_update(&h_m_salt_secret, salt, _SALT_BYTE)",
            "while ( MAX_ATTEMPT_VINEGAR > n_attempt )",
            "hash_final_digest( vinegar, _V_BYTE, &h_vinegar)",
            "if ( !l1_succ )",
            "continue;",
        ]),
    }

    salt_bits = 128
    signing_queries = 2 ** 64
    for hash_queries in (0, 2 ** 64):
        numerator = 1 - (hash_queries + signing_queries) * signing_queries / 2 ** salt_bits
        checks[f"nonpositive_bound_qh_{hash_queries}"] = numerator <= 0
        print(f"Theorem-1 numerator at qs=2^64, qh={hash_queries}: {numerator:g}")

    print("PASS UOV-5" if all(checks.values()) else f"FAIL UOV-5: {checks}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
