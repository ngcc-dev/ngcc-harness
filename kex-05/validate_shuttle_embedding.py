#!/usr/bin/env python3
"""Check that Loom embeds the attacked Shuttle signer byte for byte."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOOM = ROOT / "kex-05/Implementations and Test_Vectors/Implementations/Reference_Implementation"
SHUTTLE = ROOT / (
    "sign-23/Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/"
    "Implementations/Reference_Implementation"
)
FILES = [
    "sign.c", "polyvec.c", "sampler.c", "sampler_u.c", "irs.c",
    "rounding.c", "packing.c", "poly.c", "poly_ntt.c", "reduce.c",
    "rans.c", "approx_exp.c", "approx_log.c",
]

for level in ("128", "256", "512"):
    for name in FILES:
        loom = (LOOM / f"LoomKEX-{level}/sig" / name).read_bytes()
        shuttle = (SHUTTLE / f"SHUTTLE-{level}" / name).read_bytes()
        assert loom == shuttle, f"different: level={level} file={name}"
    protocol = (LOOM / f"LoomKEX-{level}/loom/loom.c").read_text()
    assert "crypto_sign_signature_rnd" in protocol
    assert "crypto_sign_verify" in protocol

print("CONFIRMED kex-05-3: all three Loom signing modules are byte-identical to Shuttle")
print("CONFIRMED Loom authentication calls the embedded signer and verifier")
