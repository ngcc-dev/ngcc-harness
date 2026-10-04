#!/usr/bin/env python3
"""Certificate for sign-21-4's quantum TCCR proof-bound gap."""

from __future__ import annotations

import math
import random
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REF = ROOT / "Implementations" / "Reference_Implementation"


def macro(text: str, name: str) -> int:
    match = re.search(rf"^#define {re.escape(name)} ([0-9]+)$", text, re.MULTILINE)
    assert match, f"missing {name}"
    return int(match.group(1))


def derive_l(text: str, profile: str) -> tuple[int, int, int, int]:
    prefix = f"RESOLVED_ALPHA_{profile.upper()}"
    lam = macro(text, prefix + "_CSP")
    tau = macro(text, prefix + "_TAU")
    grind = macro(text, prefix + "_POW_LEVEL")
    k = (lam - grind) // tau + 1
    tau1 = (lam - grind) % tau
    tau0 = tau - tau1
    length = tau1 * (1 << k) + tau0 * (1 << (k - 1))
    return lam, tau, grind, length


def check_submitted_parameters() -> list[tuple[str, int, int, int, int, int, float, float]]:
    rows = []
    profiles = (
        ("160s", 128),
        ("160f", 128),
        ("256s", 128),
        ("256f", 128),
        ("384s", 256),
        ("384f", 256),
        ("512s", 256),
        ("512f", 256),
    )
    expected_l = {"160s": 28672, "160f": 3328}
    for profile, block_bits in profiles:
        directory = REF / f"ReSolveD-alpha-{profile}"
        text = (directory / "parameters.h").read_text(encoding="utf-8")
        lam, tau, _grind, length = derive_l(text, profile)
        if profile in expected_l:
            assert length == expected_l[profile]

        tccr = (directory / "tccr.c").read_text(encoding="utf-8")
        assert "const size_t x_l_bytes   = block_bytes - 1;" in tccr
        assert "for (size_t i = 0; i != 2; ++i)" in tccr

        enc = (directory / "enc.c").read_text(encoding="utf-8")
        block_line = f"case {lam}: return {block_bits} / 8;"
        assert block_line in enc

        d_up = 2 * math.ceil(math.log2(length)) + 5
        d_mh = d_up * tau
        gap = lam - block_bits
        up_threshold = gap - math.log2(2 * d_up)
        mh_threshold = gap - math.log2(2 * d_mh)
        rows.append(
            (profile, lam, block_bits, length, tau, d_mh, up_threshold, mh_threshold)
        )
    return rows


def scaled_permutation_invariant() -> tuple[int, int, int]:
    # Equations (87)-(88), reduced to the first output block.  The real format
    # has an eight-bit block tag; two toy tag bits leave a larger visible left
    # domain at this small size without changing the permutation invariant.
    n = 12
    tag_bits = 2
    domain = 1 << (n - tag_bits)
    rng = random.Random(0x2104_2026)
    permutation = list(range(1 << n))
    rng.shuffle(permutation)
    gamma_l = rng.randrange(domain)

    def transformed(w_l: int) -> int:
        plaintext = w_l ^ gamma_l
        z_l = permutation[plaintext] ^ plaintext
        return z_l ^ w_l

    real = [transformed(w_l) for w_l in range(domain)]
    assert len(set(real)) == domain

    random_function = [rng.randrange(1 << n) for _ in range(domain)]
    collisions = domain - len(set(random_function))
    assert collisions > 0
    return n, domain, collisions


def main() -> None:
    rows = check_submitted_parameters()
    n, domain, control_collisions = scaled_permutation_invariant()
    for profile, lam, block, length, tau, d_mh, up_threshold, mh_threshold in rows:
        target = {160: 80, 256: 128, 384: 192, 512: 256}[lam]
        assert mh_threshold < target
        at_q80 = 1 + 80 + math.log2(d_mh) - (lam - block)
        print(
            f"profile={profile} lambda={lam} n={block} L={length} tau={tau} "
            f"D_multi_hiding={d_mh} "
            f"quantum_target={target} "
            f"single_signature_MH_error_bits={mh_threshold:.2f} "
            f"log2_Qsig_UP_bound_one={up_threshold:.2f} "
            f"log2_Qsig_MH_bound_one={mh_threshold:.2f} "
            f"log2_MH_term_at_Qsig_2^80={at_q80:.2f}"
        )
    print(
        f"scaled_tccr_control=n{n} inputs={domain} "
        f"real_collisions=0 random_function_collisions={control_collisions}"
    )
    print("PROOF GAP sign-21-4 CONFIRMED: quantum TCCR error misses every target")


if __name__ == "__main__":
    main()
