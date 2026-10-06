#!/usr/bin/env python3
"""Check sign-19-4's Phoenix-SM3 message-prehash transfer preconditions."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
BASE = ROOT / "Implementations and Test_Vectors/Implementations"


def require(condition: bool, detail: str) -> None:
    if not condition:
        raise AssertionError(detail)


for family, directory, suffix, expander in (
    ("reference", "Reference_Implementation", "", "mgf1_sm3"),
    ("AVX2", "Optimized_Implementation/avx2", "-avx2", "sm3_xof"),
):
    for level in (384, 512):
        for speed in ("s", "f"):
            name = f"Phoenix-SM3-{level}{speed}"
            instance = BASE / directory / (name + suffix)
            h = (instance / "hash_sm3.c").read_text()
            sig = (instance / "sign.c").read_text()
            params = (instance / "params" / f"params-phoenix-sm3-{level}{speed}.h").read_text()
            header = (instance / "hash_sm3.h").read_text()

            require(f"#define SPX_N {level // 8}" in params, f"{family} {name}: N")
            require("#define SPX_SM3_OUTPUT_BYTES 32" in header, f"{family} {name}: SM3 width")
            require("sm3_inc_finalize(seed + 2*SPX_N" in h, f"{family} {name}: prehash")
            require(f"{expander}(bufp, SPX_DGST_BYTES, seed," in h, f"{family} {name}: expansion")
            require("memcpy(seed, R, SPX_N);" in h, f"{family} {name}: fixed R")
            require("memcpy(seed + SPX_N, pk, SPX_N);" in h, f"{family} {name}: fixed PK")
            require("hash_message(mhash, &tree, &idx_leaf, sig_origin, pk," in sig,
                    f"{family} {name}: signer hash")
            require("hash_message(mhash, &tree, &idx_leaf, sig, pk," in sig,
                    f"{family} {name}: verifier hash")
            require("save_fors_counter(counter, sig_origin);" in sig, f"{family} {name}: counter write")
            require("counter = get_fors_counter(sig);" in sig, f"{family} {name}: counter read")
            print(f"PASS {family} {name}: 256-bit inner digest, serialized R/counter")

print("Bound: one signature, about 2^256 fixed-target SM3 evaluations; search not executed")
print("CONFIRMED sign-19-4: 256-bit Phoenix-SM3 message-prehash ceiling")
