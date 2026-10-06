#!/usr/bin/env python3
"""Source certificates for kem-10-5 and kem-10-6."""
import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parent / "Implementations"


def short_ciphertext() -> None:
    checked = 0
    for tree in ("Reference_Implementation", "Optimized_Implementation"):
        for level in (128, 256, 512):
            base = ROOT / tree / f"CMultiURAG-{level}"
            wrapper = (base / f"KEM_CMultiURAG-{level}.c").read_text()
            decaps = (base / "src/kem.c").read_text()
            parser = (base / "src/parsing.c").read_text()
            body = wrapper.split("int kem_dec(", 1)[1].split("\n}", 1)[0]
            assert "(void)ct_len_bytes" in body
            assert "cmultiurag_decaps(ss, ct, sk)" in body
            assert body.count("ct_len_bytes") == 2
            assert "cmultiurag_kem_ciphertext_from_string(U, V, salt, ct)" in decaps
            assert "rbc_mat_from_string(U, CMULTIURAG_PARAM_N, CMULTIURAG_PARAM_N2, ct)" in parser
            assert "ct + CMULTIURAG_MAT_NN2_BYTES" in parser
            assert "ct + CMULTIURAG_MAT_NN2_BYTES + CMULTIURAG_MAT_N1N2_BYTES" in parser
            checked += 1
    assert checked == 6
    print("CONFIRMED kem-10-5: all six decapsulators discard ciphertext length before fixed-offset parsing")


def fourth_limb() -> None:
    base = ROOT / "Reference_Implementation/CMultiURAG-128/src/rbc-79"
    header = (base / "rbc_79.h").read_text()
    source = (base / "rbc_elt.c").read_text()
    assert "#define RBC_79_FIELD_M 79" in header
    assert "#define RBC_79_ELT_SIZE 2" in header
    assert "#define RBC_79_ELT_UR_SIZE 3" in header
    assert "typedef uint64_t rbc_elt_ur[RBC_79_ELT_UR_SIZE]" in header
    body = source.split("void rbc_elt_ur_mul(", 1)[1].split("\n}", 1)[0]
    assert "i<RBC_79_FIELD_M" in body
    assert "offset = i / 64" in body
    assert "j<RBC_79_ELT_SIZE + 1" in body
    assert "o[j+offset] ^=" in body
    assert 2 + 79 // 64 == 3
    print("CONFIRMED kem-10-6: reference 79-bit multiplier accesses limb 3 of a three-limb output")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-id", choices=("kem-10-5", "kem-10-6"), required=True)
    args = parser.parse_args()
    if args.report_id == "kem-10-5":
        short_ciphertext()
    else:
        fourth_limb()
