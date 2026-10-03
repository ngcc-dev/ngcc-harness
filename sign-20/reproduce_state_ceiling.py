#!/usr/bin/env python3
"""Certificate for Qing Luan's 256-bit DRBG and level-512 XOF states."""

from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
REF = HERE / "Implementations and Test_Vectors/Implementations/Reference_Implementation"


def main():
    pdf = subprocess.run(
        ["pdftotext", "-layout", str(HERE / "sign-20-spec.pdf"), "-"],
        check=True, stdout=subprocess.PIPE, text=True).stdout
    compact = " ".join(pdf.split())
    assert "Seed length: 32 bytes" in compact
    assert "initialize the internal state V and constant C" in compact

    for level in (384, 512):
        utils = (REF / f"QingLuan-{level}/src/utils.c").read_text()
        assert "#define DRBG_SEED_SIZE   32" in utils
        assert "static THREAD_LOCAL uint8_t  drbg_V[DRBG_SEED_SIZE];" in utils
        assert "memcpy(c_buf + 1, drbg_V, DRBG_SEED_SIZE);" in utils
        assert "drbg_reseed_counter = 1;" in utils
        print(f"QingLuan-{level}: specified/standalone key-generation support <= 2^256")

    level512 = REF / "QingLuan-512"
    params = (level512 / "include/params.h").read_text()
    hash_c = (level512 / "src/hash.c").read_text()
    rsdp = (level512 / "src/rsdp.c").read_text()
    assert "#define PARAM_HASH_BYTES     128" in params
    assert "uint8_t tmp[PARAM_HASH_BYTES + 4];" in hash_c
    assert "memcpy(tmp, ctx->key, PARAM_HASH_BYTES);" in hash_c
    assert "sm3(tmp, PARAM_HASH_BYTES + 4, ctx->squeeze_buf);" in hash_c
    assert rsdp.index("xof_squeeze(&xof, seed_e") < rsdp.index("xof_squeeze(&xof, seed_pk")
    print("QingLuan-512: public Seed_pk tests a 256-bit post-key SM3 state")
    print("ATTACK sign-20-5 CONFIRMED: two nominal levels have at most 2^256 key-generation roots")


if __name__ == "__main__":
    main()
