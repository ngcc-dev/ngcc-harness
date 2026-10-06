#!/usr/bin/env python3
"""Source certificate for kem-06-6's short-ciphertext parser path."""
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"
checked = 0
for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for level in (128, 256, 512):
        base = ROOT / tree / f"BRA-{level}"
        wrapper = (base / f"KEM_BRA-{level}.c").read_text()
        decaps = (base / "src/kem.c").read_text()
        parser = (base / "src/parsing.c").read_text()
        body = wrapper.split("int kem_dec(", 1)[1].split("\n}", 1)[0]
        assert "(void)ct_len_bytes" in body
        assert "bra_decaps(ss, ct, sk)" in body
        assert body.count("ct_len_bytes") == 2  # parameter and discarded value
        assert "bra_kem_ciphertext_from_string(u, v, salt, ct)" in decaps
        assert "rbc_qre_from_string(u, ct)" in parser
        assert "rbc_qre_from_string(v, ct + BRA_VEC_N_BYTES)" in parser
        assert "memcpy(salt, ct + 2 * BRA_VEC_N_BYTES, BRA_SALT_BYTES)" in parser
        checked += 1

assert checked == 6
print("CONFIRMED kem-06-6: all six decapsulators discard ciphertext length before fixed-offset parsing")
