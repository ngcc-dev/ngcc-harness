#!/usr/bin/env python3
"""Static certificates for the TRIKE h0 leak and secret-indexed decoder."""

from pathlib import Path
import re


HERE = Path(__file__).resolve().parent
base = HERE / "Implementations and Test_Vectors/Implementations"
checked = 0

for tree in ("Reference_Implementation", "Optimized_Implementation"):
    for name in ("TRIKE-2", "TRIKE-5", "TRIKE-7", "TRIKE-9"):
        directory = base / tree / name / "src"
        kem_file = (directory / "KEM_AlgorithmInstance.c").read_text(errors="replace")
        kem = kem_file[kem_file.index("int kem_dec(") :]
        decoder = (directory / "decoder.c").read_text(errors="replace")

        assert re.search(r"\bh0\s*=\s*\(uint8_t \*\)aligned_alloc", kem)
        assert "free(h0);" not in kem
        for temporary in (
            "hash_input",
            "e_calc",
            "e",
            "s",
            "v",
            "u",
            "r2",
            "r1",
            "t2",
            "t1",
            "t0",
        ):
            assert f"free({temporary});" in kem

        # idx is the secret support copied from the secret key by kem_dec.
        assert "idx_to = (i + idx[k]) % PARAM_R" in decoder
        assert "s[idx_to >> 3]" in decoder
        checked += 1
        print(f"{tree}/{name}: h0 not freed; syndrome indexed by secret support")

assert checked == 8
print("TRIKE_H0_RESOURCE_LEAK_CONFIRMED")
print("TRIKE_SECRET_SUPPORT_ADDRESS_DEPENDENCE_CONFIRMED")
