#!/usr/bin/env python3
"""Certify the 128-bit K2K encryption-seed input in PLAC512/512Star."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent / "Implementations/Reference_Implementation"
COMMON = ROOT / "Common/primitives/kem/POLARLAC-512"


def check_variant(name: str) -> None:
    base = ROOT / "CreTAKE512" / name
    twokem = (base / "twokem.c").read_text()
    aux = (base / "auxfunc.c").read_text()
    kex = (base / "KEX_AlgorithmInstance.c").read_text()

    assert "unsigned char buf3[PKE_MESSAGE_BYTES + SEED_BYTES]" in twokem
    assert twokem.count("buf3, sizeof(buf3), buf4") == 2
    assert "memcpy(buf3, m, PKE_MESSAGE_BYTES)" in twokem
    assert "memcpy(buf3 + PKE_MESSAGE_BYTES, buf2, sizeof(buf2))" in twokem
    assert "memcpy(cascade_msg_ct, msg, (msg_len_bits + 7) / 8)" in aux
    assert "memcpy(sta + *sta_len_bytes, k_j, ss_l)" in kex
    assert "const unsigned char *k_j = sta + SEED_BYTES" in kex


def main() -> None:
    params = (COMMON / "params.h").read_text()
    pke = (COMMON / "pke.c").read_text()
    assert "#define KEM_SEED_LEN_BYTES 64" in params
    assert "#define MESSAGE_LEN_BYTES 64" in params
    assert "#define PKE_MESSAGE_BYTES     MESSAGE_LEN_BYTES" in params
    assert "sample_screened_polyvec(&r, seed, &nonce)" in pke
    assert "Encode_m(hatm, (uint8_t *)m)" in pke
    assert pke.index("sample_screened_polyvec(&r, seed, &nonce)") < pke.index("poly_zero(&v)")
    for variant in ("CreTAKE-K2K-PLAC512", "CreTAKE-K2K-PLAC512Star"):
        check_variant(variant)

    driver = Path(__file__).resolve().with_name("reproduce_k2k_prefix_native.c")
    source = ROOT / "CreTAKE512/CreTAKE-K2K-PLAC512"
    with tempfile.TemporaryDirectory(prefix="ngcc-cretake-prefix-") as temporary:
        binary = Path(temporary) / "prefix_check"
        subprocess.run(["cc", "-O2", "-std=c11", "-I", str(source), str(driver),
                        str(source / "auxfunc.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("ATTACK kex-03-7 CONFIRMED: both PLAC512 variants absorb only 16 message bytes; 2^128 prefix search bounds k_i")


if __name__ == "__main__":
    main()
