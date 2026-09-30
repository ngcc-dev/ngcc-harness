#!/usr/bin/env python3
"""Check Weaver's sampler and half-modulus specification/code mismatches."""

from pathlib import Path
import re
import subprocess


HERE = Path(__file__).resolve().parent
result = subprocess.run(
    ["pdftotext", "-layout", str(HERE / "kem-39-spec.pdf"), "-"],
    check=True,
    stdout=subprocess.PIPE,
    text=True,
)
spec = result.stdout
assert "non-overlapping w-bit little-endian candidates" in spec
assert "SampleIndex(ρ, i · n + j, |B|)" in spec
assert "(q − 1)/2" in spec

matrix_mismatches = 0
lifting_mismatches = 0
halfq_mismatches = 0
for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for set_name in ("WeaverKEM-128", "WeaverKEM-256", "WeaverKEM-512"):
        directory = HERE / "Implementations" / tree / set_name
        indcpa = (directory / "indcpa.c").read_text(errors="replace")
        lift = (directory / "poly_invq.c").read_text(errors="replace")
        params = (directory / "params.h").read_text(errors="replace")
        msgenc = (directory / "msgenc.c").read_text(errors="replace")

        assert "prf(buf, GEN_INVQ_RAND_BYTES, seed, nonce++)" in lift
        lifting_mismatches += 1

        assert "#define WEAVER_HALFQ ((WEAVER_Q + 1) / 2)" in params
        assert "mask & WEAVER_HALFQ" in msgenc
        halfq_mismatches += 1

        if set_name != "WeaverKEM-128":
            assert "LEMIRE_REJ_THRESHOLD" in indcpa
            assert re.search(r"val\s*=.*buf\[pos \+ 1\]\s*<<\s*8", indcpa)
            assert "prod = val * (uint32_t)WEAVER_Q" in indcpa
            matrix_mismatches += 1

print(f"16-bit matrix parsers differing from 13-bit specification: {matrix_mismatches}")
print(f"sequential lifting streams differing from SampleIndex: {lifting_mismatches}")
print(f"(q+1)/2 encoders differing from (q-1)/2: {halfq_mismatches}")
assert (matrix_mismatches, lifting_mismatches, halfq_mismatches) == (4, 6, 6)
print("WEAVER_SAMPLER_SPECIFICATION_MISMATCHES_CONFIRMED")
print("WEAVER_HALF_MODULUS_MISMATCH_CONFIRMED")
