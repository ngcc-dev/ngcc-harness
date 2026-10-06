#!/usr/bin/env python3
"""Source certificate for reference Phoenix-SM3 high-level digest over-reads."""

from pathlib import Path


ROOT = (Path(__file__).resolve().parent / "Implementations and Test_Vectors" /
        "Implementations" / "Reference_Implementation")

for level in (384, 512):
    for speed in ("s", "f"):
        name = f"Phoenix-SM3-{level}{speed}"
        tree = ROOT / name
        params = (tree / "params" / f"params-phoenix-sm3-{level}{speed}.h").read_text()
        header = (tree / "hash_sm3.h").read_text()
        thash = (tree / "thash_sm3_simple.c").read_text()
        addr = (tree / "hash_sm3.c").read_text()
        assert f"#define SPX_N {level // 8}" in params, name
        assert "#define SPX_SM3_OUTPUT_BYTES 32" in header, name
        assert "unsigned char outbuf[SPX_SM3_OUTPUT_BYTES];" in thash, name
        assert thash.count("memcpy(out, outbuf, SPX_N);") == 2, name
        assert "unsigned char outbuf[SPX_SM3_OUTPUT_BYTES];" in addr, name
        assert "memcpy(out, outbuf, SPX_N);" in addr, name
        print(f"PASS {name}: 32-byte digest copied as {level // 8} bytes")

print("CONFIRMED sign-19-6: reference high-level SM3 hash helpers read past digest buffers")
