#!/usr/bin/env python3
"""Certify the secret-dependent Sigurd signing branch in all six sources."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent


def require(text: str, needle: str, path: Path) -> None:
    if needle not in text:
        raise SystemExit(f"not confirmed: {path}: missing {needle!r}")


trees = [
    ROOT / "Implementations" / variant / f"Sigurd-{level}"
    for variant in ("Reference_Implementation", "Optimized_Implementation")
    for level in (128, 256, 512)
]

for tree in trees:
    core_path = tree / "sig_core.c"
    api_path = tree / "SIG_AlgorithmInstance.c"
    core = core_path.read_text(encoding="utf-8")
    api = api_path.read_text(encoding="utf-8")

    # random_e has fixed weight: exactly one selected position per six-column block.
    require(core, "for (i = 0; i < M / 6; ++i)", core_path)
    require(core, "e[i * 6 + (rnd % 6u)] = 1;", core_path)

    # The public-state computation branches on that secret support.
    require(core, "random_e(e_bits, e_seed);", core_path)
    require(core, "if (e_bits[col] != 0u)", core_path)
    require(core, "y_bits[row] ^= 1u;", core_path)

    # The submitted signing API rebuilds this state for every signature.
    require(api, "sig_unpack_secret_key(sk, H_seed, e_seed);", api_path)
    require(api, "sig_build_public_state_direct(H_seed, e_seed", api_path)

print("SOURCE sign-24-3 CONFIRMED: secret-dependent public-state branch in 6/6 trees")
