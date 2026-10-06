#!/usr/bin/env python3
"""Source certificate for kem-02-7's fixed ECC tail overrun."""
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"
checked = 0
for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for level in (576, 864, 1152, 1728, 2304):
        base = ROOT / tree / f"Amoeba-{level}" / "src/backend"
        source = (base / "cpapke.c").read_text()
        params = (base / "params.h").read_text()
        assert "#define RLWE_ECC_N 523" in params
        assert "typedef uint8_t PTXT[RLWE_ECC_N]" in source
        encode = source.split("static void encode_MSB(", 1)[1].split("static void decode_MSB(", 1)[0]
        decode = source.split("static void decode_MSB(", 1)[1].split("static void CPAPKE_KeyGen_raw(", 1)[0]
        for body in (encode, decode):
            assert "i < RLWE_ECC_N; i += 4" in body
            assert "[i + 3]" in body
        assert "encode_MSB(c, pt)" in source
        assert "decode_MSB(pt, c1)" in source
        checked += 1

assert checked == 10
assert 523 % 4 == 3 and 520 + 3 == 523
print("CONFIRMED kem-02-7: all ten implementations read/write PTXT[523] after a 523-byte allocation")
