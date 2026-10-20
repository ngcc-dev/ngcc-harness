#!/usr/bin/env python3
"""Check that AFS-KEX's three receive passes ignore message lengths."""

from pathlib import Path

root = Path(__file__).resolve().parent / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
checks = {
    "kex_generate_pass2_msg_b": ("m1", "crypto_kem_dec(K_B,"),
    "kex_generate_pass3_msg_a": ("m2", "crypto_kem_dec(K_A,"),
    "kex_generate_pass4_msg_b": ("m3", "m3[i]"),
}
for tree in ("AFS_KEX_C128", "AFS_KEX_C256", "AFS_KEX_C512"):
    source = (root / tree / "KEX_AlgorithmInstance.c").read_text()
    for function, (message, required_read) in checks.items():
        body = source.split(f"int {function}(", 1)[1].split("\nint ", 1)[0]
        assert f"(void){message}_len_bytes;" in body, (tree, function, "length used")
        assert required_read in body, (tree, function, "fixed-size read absent")
        assert f"if ({message}_len_bytes" not in body, (tree, function, "length checked")
print("CONFIRMED kex-02-7: all three AFS-KEX receive passes ignore supplied message lengths in all three sets")
