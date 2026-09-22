#!/usr/bin/env python3
"""Independent arithmetic checks for the FlexTree findings in sign-11/report.md."""

from __future__ import annotations

import ast
from decimal import Decimal, localcontext
from math import comb, log2, prod
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / "sign-11" / "Implementations" / "Reference_Implementation"
REPORT_IDS = (
    "sign-11-1", "sign-11-2", "sign-11-3", "sign-11-4",
    "sign-11-5", "sign-11-6", "sign-11-7", "sign-11-8",
)
EXPECTED_COUNTER_BITS = {
    "Flextree-160s": 133.1198559714,
    "Flextree-160f": 128.8924360408,
    "Flextree-256s": 225.1740651695,
    "Flextree-256f": 224.1964485565,
    "Flextree-384s": 352.2232013350,
    "Flextree-384f": 352.0282290607,
    "Flextree-512s": 480.0169948118,
    "Flextree-512f": 480.0017465928,
}
EXPECTED_OTS_BITS = {
    "Flextree-160s": 33.348,
    "Flextree-160f": 47.254,
    "Flextree-256s": 54.156,
    "Flextree-256f": 75.799,
    "Flextree-384s": 111.653,
    "Flextree-384f": 96.236,
    "Flextree-512s": 126.816,
    "Flextree-512f": 128.000,
}
ORDER = [
    "Flextree-160s", "Flextree-160f", "Flextree-256s", "Flextree-256f",
    "Flextree-384s", "Flextree-384f", "Flextree-512s", "Flextree-512f",
]


def macro(text: str, name: str) -> int:
    match = re.search(rf"^#define\s+{re.escape(name)}\s+(\d+)", text, re.MULTILINE)
    if not match:
        raise ValueError(f"missing {name}")
    return int(match.group(1))


def parameters() -> dict[str, dict[str, object]]:
    out: dict[str, dict[str, object]] = {}
    for directory in sorted(REF.glob("Flextree-*")):
        header = next((directory / "params").glob("params-flextree-*.h"))
        text = header.read_text(encoding="utf-8", errors="replace")
        array = re.search(r"^#define\s+SPX_WOTS_W_ARRAY\s+(\{[^\n]+\})", text, re.MULTILINE)
        if not array:
            raise ValueError(f"missing WOTS array in {header}")
        widths = ast.literal_eval(array.group(1).replace("{", "[").replace("}", "]"))
        out[directory.name] = {
            "n": macro(text, "SPX_N"),
            "h": macro(text, "SPX_FULL_HEIGHT"),
            "t": macro(text, "SPX_PORS_FP_T"),
            "k": macro(text, "SPX_PORS_FP_K"),
            "zb": macro(text, "WOTS_ZERO_BITS"),
            "wanted": macro(text, "WANTED_CHECKSUM"),
            "widths": widths,
        }
    return out


def counter_grinding_bits(h: int, t: int, k: int) -> tuple[float, int]:
    """Raw attack in Kudinov Sec. 2.1, q_sig=2^64 and a 32-bit counter."""
    with localcontext() as ctx:
        ctx.prec = 250
        two = Decimal(2)
        qsig = 1 << 64
        trials = 1 << 32
        p = two ** (-h)
        probability = ((Decimal(1) - p).ln() * Decimal(qsig)).exp()
        loads: list[Decimal] = []
        covers: list[Decimal] = []
        denominator = Decimal(comb(t, k))
        for q in range(100):
            loads.append(probability)
            covers.append(Decimal(0) if q == 0 else Decimal(comb(k * q, k)) / denominator)
            probability *= (Decimal(qsig - q) / Decimal(q + 1)) * p / (Decimal(1) - p)

        best_bits: Decimal | None = None
        best_q0 = 0
        for q0 in range(1, 50):
            tail = sum(loads[q0:], Decimal(0))
            success = Decimal(0)
            for load, cover in zip(loads[q0:], covers[q0:]):
                success += load * (Decimal(1) - (Decimal(1) - cover) ** trials)
            cost = Decimal(1) + Decimal(trials) * tail
            bits = (cost / success).ln() / two.ln()
            if best_bits is None or bits < best_bits:
                best_bits, best_q0 = bits, q0
        assert best_bits is not None
        return float(best_bits), best_q0


def ots_extension_bits(widths: list[int], target_sum: int) -> float:
    """-log2 E_x prod_i((w_i-x_i)/w_i), for x in the constant-sum set."""
    total = sum(width - 1 for width in widths)
    counts = [0] * (total + 1)
    weights = [0] * (total + 1)
    counts[0] = weights[0] = 1
    for width in widths:
        next_counts = [0] * (total + 1)
        next_weights = [0] * (total + 1)
        for subtotal, count in enumerate(counts):
            if not count:
                continue
            for digit in range(width):
                next_counts[subtotal + digit] += count
                next_weights[subtotal + digit] += weights[subtotal] * (width - digit)
        counts, weights = next_counts, next_weights
    return log2(counts[target_sum] * prod(widths)) - log2(weights[target_sum])


def static_source_checks() -> None:
    spec = subprocess.run(
        ["pdftotext", "-layout", str(ROOT / "sign-11" / "sign-11-spec.pdf"), "-"],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout
    compact_spec = " ".join(spec.split())
    hash160 = (REF / "Flextree-160f" / "hash_sm3.c").read_text(errors="replace")
    hash384 = (REF / "Flextree-384f" / "hash_sm3.c").read_text(errors="replace")
    aux = (REF / "Flextree-384f" / "auxfunc.c").read_text(errors="replace")
    signer = (REF / "Flextree-384f" / "sign.c").read_text(errors="replace")

    # Deterministic PRFMSG is a raw secret-prefix SM3 construction; the shipped
    # signer nevertheless always selects fresh optrand.
    assert "optRand is set to PK.seed" in compact_spec
    assert "memcpy(buf, sk_prf, SPX_N);" in hash160
    assert "memcpy(buf + SPX_N, optrand, SPX_N);" in hash160
    assert "sm3hash(256, buf, in_len_bits, temp_out);" in hash160
    assert "pseudoXOF(out_len_bits, buf, in_len_bits, R);" in hash384
    assert "randombytes(optrand, SPX_N);" in signer

    # pseudoXOF is counter mode over the 256-bit SM3 state/output.
    assert "unsigned int digest[8];" in aux
    assert "unsigned int ct = 1;" in aux
    assert "sm3_bit(cascade_msg_ct, msg_len_bits + 32, K + i * 32);" in aux
    assert "actual security strength of these instances may not reach" in compact_spec


def main() -> None:
    params = parameters()
    static_source_checks()
    print("set              counter-grind  q0   OTS-extension  spec/code-sum  zero-window  reused-addresses")
    reused_counts = []
    rounding_mismatches = []
    for name in ORDER:
        p = params[name]
        n, h, t, k = (int(p[key]) for key in ("n", "h", "t", "k"))
        zb, wanted = int(p["zb"]), int(p["wanted"])
        widths = list(p["widths"])
        counter_bits, q0 = counter_grinding_bits(h, t, k)
        total_sum = sum(width - 1 for width in widths)
        spec_sum = total_sum // 2
        code_digit_sum = total_sum - wanted
        ots_bits = ots_extension_bits(widths, spec_sum)
        overlap = min(zb, 8 - zb)
        effective = 8 * n - overlap
        pors_height = (t - 1).bit_length()
        short_start = t - (1 << (pors_height - 1))
        reused = max(0, min(2 * short_start, 1 << (pors_height - 1)) - short_start)
        reused_counts.append(reused)
        print(
            f"{name:17s} {counter_bits:13.3f} {q0:3d}"
            f" {ots_bits:14.3f} {spec_sum:5d}/{code_digit_sum:<5d} {effective:4d}/{8*n} bits"
            f" {reused:8d}"
        )
        assert abs(counter_bits - EXPECTED_COUNTER_BITS[name]) < 1e-6
        assert abs(ots_bits - EXPECTED_OTS_BITS[name]) < 0.001
        assert wanted == spec_sum
        if code_digit_sum != spec_sum:
            rounding_mismatches.append(name)

    assert rounding_mismatches == ["Flextree-384f", "Flextree-512s"]
    assert min(reused_counts) == 516 and max(reused_counts) == 95017
    print("PASS: source conditions and independent values match the reports")


if __name__ == "__main__":
    main()
