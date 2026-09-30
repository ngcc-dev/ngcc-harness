#!/usr/bin/env python3
"""Check the two erroneous YuanYang.DSA-512 fixed-point constants."""

from pathlib import Path
import re


HERE = Path(__file__).resolve().parent
REF = HERE / "Implementations/Reference_Implementation/yuanyang-512"
OPT = (
    HERE
    / "Implementations/Optimized_Implementation"
    / "SIG-YuanYang_DSA-x86-Performance Optimization Version"
    / "yuanyang-512"
)
SCALE = 1 << 43


def raw_constant(text: str, name: str) -> int:
    match = re.search(
        rf"\b{name}\s*=\s*YY_FPR_RAW\((0x[0-9a-fA-F]+|[0-9]+)\)", text
    )
    if not match:
        raise AssertionError(f"missing {name}")
    return int(match.group(1), 0)


wide_expected = round((1.0 / (2.0 * (4.0 * 1.026) ** 2)) * SCALE)
delta2_expected = round((1.0 / (2.0 * 69.76**2)) * SCALE)
assert wide_expected == 0x3CCC24A84F
assert delta2_expected == 0x35DE15C3

for tree, compensated in ((REF, False), (OPT, True)):
    path = tree / "fpr.h"
    text = path.read_text(errors="replace")
    wide = raw_constant(text, "fpr_yuanyang_inv_2sqr_large_sampler_sigma")
    delta2 = raw_constant(text, "fpr_yuanyang_inv_2sqrsigma_sig")
    assert wide == 0x3CCC24A85
    assert delta2 == 3701747757852
    sign = (tree / "sign.c").read_text(errors="replace")
    has_shift = (
        "fpr_mul(weight, fpr_yuanyang_inv_2sqrsigma_sig).v "
        ">> YUANYANG_EXTRABITS" in sign
    )
    assert has_shift == compensated
    effective_delta2 = delta2 >> 12 if has_shift else delta2
    print(tree.relative_to(HERE))
    print(
        f"  wide sampler: submitted={wide / SCALE:.12g} "
        f"expected={wide_expected / SCALE:.12g} ratio={wide_expected / wide:.0f}"
    )
    print(
        f"  Delta2: effective={effective_delta2 / SCALE:.12g} "
        f"expected={delta2_expected / SCALE:.12g} "
        f"ratio={effective_delta2 / delta2_expected:.0f} "
        f"{'compensated' if has_shift else 'affected'}"
    )

print("YUANYANG_SAMPLER_CONSTANTS_CONFIRMED")
