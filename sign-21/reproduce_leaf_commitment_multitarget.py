#!/usr/bin/env python3
"""Certificate for sign-21-2's fixed-tweak leaf multi-target search."""

from __future__ import annotations

import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
SETS = (
    ("160s", 160, 14, 128),
    ("160f", 160, 21, 128),
    ("256s", 256, 22, 256),
    ("256f", 256, 35, 256),
    ("384s", 384, 34, 384),
    ("384f", 384, 53, 384),
    ("512s", 512, 46, 512),
    ("512f", 512, 72, 512),
)


def source_files() -> list[Path]:
    submitted = sorted((HERE / "Implementations").rglob("bavc.c"))
    if submitted:
        return submitted
    return sorted((HERE / "src").rglob("bavc.c"))


files = source_files()
assert files, "no archived bavc.c source found"
needle = "prg_2_lambda(key, iv, 0, com, lambda);"
for path in files:
    text = path.read_text(encoding="utf-8")
    assert needle in text, f"fixed zero leaf tweak absent from {path}"
if HERE.joinpath("Implementations").exists():
    assert len(files) == 8, f"expected eight reference profiles, found {len(files)}"
    optimized = sorted((HERE / "Implementations" / "Optimized_Implementation").rglob("vector_com.inc"))
    assert len(optimized) == 8, f"expected eight optimized profiles, found {len(optimized)}"
    for path in optimized:
        text = path.read_text(encoding="utf-8")
        assert "(void) tweak;" in text, path
        assert "(void) small_tweak;" in text, path
        assert "static_cast<typename PRG::tweak_t>(0)" in text, path
        assert "PRG::template leaf_hash_2lambda<num_keys>" in text, path

        prgs = path.with_name("prgs.hpp").read_text(encoding="utf-8")
        params = path.with_name("parameters.hpp").read_text(encoding="utf-8")
        assert "key_ptrs, iv_bytes, 0, block_index, active, blocks);" in prgs, path
        assert "plaintext_storage[lane][15] = static_cast<uint8_t>(block_index);" in prgs, path
        assert "leaf_hash::tccr_prg" in params, path
        assert "leaf_hash::shacal2_prg" in params, path
else:
    optimized = []

affected = 0
print("set   lambda tau target log2(2^lambda/tau) verdict")
for name, lam, tau, target in SETS:
    work = lam - math.log2(tau)
    verdict = "below-target" if work < target else "control-above-target"
    affected += work < target
    print(f"{name:4s} {lam:6d} {tau:3d} {target:6d} {work:21.4f} {verdict}")

assert affected == 6
assert all(lam - math.log2(tau) >= target for _, lam, tau, target in SETS[:2])
assert all(lam - math.log2(tau) < target for _, lam, tau, target in SETS[2:])
print(f"reference_copies={len(files)} optimized_copies={len(optimized)} fixed_leaf_tweak=0")
print("LIMITATION: the full exponential seed enumeration is not executed")
print("CERTIFICATE sign-21-2 CONFIRMED: fixed leaf tweak gives below-target multi-target recovery bounds")
