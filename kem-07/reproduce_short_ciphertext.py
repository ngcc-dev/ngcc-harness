#!/usr/bin/env python3
"""Source certificate for kem-07-4's short-ciphertext parser path."""
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"
checked = 0
for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for level in (128, 256, 512):
        base = ROOT / tree / f"BRQC-{level}"
        wrapper = (base / f"KEM_BRQC-{level}.c").read_text()
        decaps = (base / "src/kem.c").read_text()
        parser = (base / "src/parsing.c").read_text()
        body = wrapper.split("int kem_dec(", 1)[1].split("\n}", 1)[0]
        assert "(void)ct_len_bytes" in body
        assert "brqc_decaps(ss, ct, sk)" in body
        assert body.count("ct_len_bytes") == 2  # parameter and discarded value
        assert "brqc_kem_ciphertext_from_string(u, v, salt, ct)" in decaps
        assert "rbc_qre_from_string(u, ct)" in parser
        assert "rbc_qre_from_string(v, ct + BRQC_VEC_N_BYTES)" in parser
        assert "memcpy(salt, ct + 2 * BRQC_VEC_N_BYTES, BRQC_SALT_BYTES)" in parser
        checked += 1

assert checked == 6
print("CONFIRMED kem-07-4: all six decapsulators discard ciphertext length before fixed-offset parsing")
