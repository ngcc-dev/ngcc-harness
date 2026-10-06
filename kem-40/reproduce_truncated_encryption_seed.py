#!/usr/bin/env python3
"""Static certificate for the YuanYang.KEM-2048 encryption-seed ceiling."""

import re
from pathlib import Path


base = Path(__file__).resolve().parent / "Implementations/Reference_Implementation"


def require(source: str, needles: tuple[str, ...], label: str) -> None:
    for needle in needles:
        if needle not in source:
            raise SystemExit(f"NOT REPRODUCED kem-40-5: {label} lacks {needle!r}")


for degree, expected_seed_bytes in ((512, 17), (1024, 33), (2048, 55)):
    root = base / f"yuanyang-{degree}"
    params = (root / "params.h").read_text()
    hdr = (root / "drng.h").read_text()
    kem = (root / "kem.c").read_text()
    drng = (root / "drng.c").read_text()
    if not re.search(rf"#define\s+YUANYANG_D\s+{degree}u\b", params):
        raise SystemExit(f"NOT REPRODUCED kem-40-5: yuanyang-{degree} degree")
    if not re.search(r"#define\s+SEEDLEN\s+\(55\)", hdr):
        raise SystemExit(f"NOT REPRODUCED kem-40-5: yuanyang-{degree} seed cap")
    require(
        kem,
        (
            "#define YY_SEC YUANYANG_D/4",
            "init_random_number(&drng,seed,1+YY_SEC/8<SEEDLEN ? 1+YY_SEC/8 : SEEDLEN)",
            "get_random_number(&drng, (unsigned char*)blindmessage,YY_SEC)",
            "pseudohash(512,(unsigned char*)blindmessage,YY_SEC+8,(unsigned char*)hashoutput)",
            "(message)[i]^hashoutput[i]",
            "memcpy(ct+k,output+YUANYANG_D,YY_SEC/8)",
            "yy_encode_message(ct,ct_len_bytes,tmphash+YY_SEC/8,msg,h)",
            "pseudohash(512,tmphash,YY_SEC+(*ct_len_bytes)*8+8,tmphash)",
        ),
        f"yuanyang-{degree} kem.c",
    )
    require(
        drng,
        (
            "memcpy(drng->V, seed, sizeof(drng->V))",
            "memcpy(data, drng->V, SEEDLEN)",
            "sm3_bit(data, SEEDLEN * 8, w)",
            "inc_Big_Number(data, SEEDLEN)",
        ),
        f"yuanyang-{degree} drng.c",
    )
    actual_seed_bytes = min(1 + degree // 32, 55)
    if actual_seed_bytes != expected_seed_bytes:
        raise SystemExit(f"NOT REPRODUCED kem-40-5: yuanyang-{degree} length")
    print(f"CONTROL kem-40-5: yuanyang-{degree} seed is {actual_seed_bytes} bytes"
          if degree != 2048 else
          f"CONFIRMED kem-40-5: yuanyang-{degree} consumes only {actual_seed_bytes} of 65 seed bytes")

optimized = Path(__file__).resolve().parent / "Implementations/Optimized_Implementation/yuanyang-2048"
for name in ("kem.c", "drng.c", "drng.h"):
    if (optimized / name).read_bytes() != (base / "yuanyang-2048" / name).read_bytes():
        raise SystemExit(f"NOT REPRODUCED kem-40-5: optimized {name} differs from checked reference")
require((base / "yuanyang-2048/drng.c").read_text(),
        ("malloc(MAX_INT(nonce_len_bytes, SEEDLEN))", "memcpy(seed_material, nonce, nonce_len_bytes)"),
        "full-length DRBG seed handling")
assert 440 + 72 == 512
print("BOUND kem-40-5: at most 2^440 seed-prefix trials; random-key false-match <= 2^-72")
