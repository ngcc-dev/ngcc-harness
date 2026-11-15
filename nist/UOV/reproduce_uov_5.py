#!/usr/bin/env python3
"""Pin the recommended-signer/proved-signer difference and proof-bound arithmetic."""

import hashlib
import math
from fractions import Fraction
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

    # q_h = 2^128: numerator negative already at q_s = 1.
    numerator = 1 - Fraction(2 ** 128 + 1, 2 ** salt_bits)
    checks["negative_at_qs_1_when_qh_2^128"] = numerator < 0
    # q_h = 2^96: numerator >= 1/2 holds at q_s = 2^30 and already fails at q_s = 2^31.
    half = Fraction(1, 2)
    numerator_below = 1 - Fraction((2 ** 96 + 2 ** 30) * 2 ** 30, 2 ** salt_bits)
    numerator_at = 1 - Fraction((2 ** 96 + 2 ** 31) * 2 ** 31, 2 ** salt_bits)
    checks["qh_2^96_keeps_half_only_below_about_qs_2^31"] = numerator_below >= half > numerator_at
    print(f"Theorem-1 numerator at qh=2^96: qs=2^30 {float(numerator_below):.6f}, qs=2^31 {float(numerator_at):.6f}")
    # Kosuge-Xagawa Proposition 3 additive term (3/2) qs sqrt((qs + qro + 1)/2^128), qro = 0.
    kx_log2 = math.log2(1.5) + 64 + (math.log2(2 ** 64 + 1) - salt_bits) / 2
    kx_one = (salt_bits / 2 - math.log2(1.5)) * 2 / 3
    checks["kx_term_2^32.58_at_qs_2^64"] = abs(kx_log2 - 32.585) < 0.01
    checks["kx_term_reaches_one_at_qs_2^42.28"] = abs(kx_one - 42.277) < 0.01
    print(f"Kosuge-Xagawa additive term: log2 {kx_log2:.2f} at qs=2^64; reaches one at qs=2^{kx_one:.2f}")

    print("PASS UOV-5" if all(checks.values()) else f"FAIL UOV-5: {checks}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
