#!/usr/bin/env python3
"""Static certificate for Amoeba's shared 256-bit SM3 chaining-value bottleneck."""

import re
from pathlib import Path


root = Path(__file__).resolve().parent / "Implementations/Reference_Implementation"
for size in (1728, 2304):
    base = root / f"Amoeba-{size}/src"
    params = (base / "backend/params.h").read_text()
    kem = (base / "backend/ccakem.c").read_text()
    sm3 = (base / "symmetric/auxfunc.c").read_text()

    for name, value in (("RLWE_MSG_LEN", 64), ("RLWE_SEED_LEN", 64), ("RLWE_KEY_LEN", 64)):
        if not re.search(rf"#define\s+{name}\s+{value}\b", params):
            raise SystemExit(f"NOT REPRODUCED kem-02-8: Amoeba-{size} {name}")

    required = (
        "get_random_number(&drng_algorithm, m, RLWE_MSG_LEN * 8)",
        "ID(m + RLWE_MSG_LEN + 1, pk)",
        "outLen < RLWE_SEED_LEN * 2",
        "sm3hash(256, m, (RLWE_MSG_LEN + 1 + RLWE_SEED_LEN) << 3, Kr + outLen)",
        "m[RLWE_MSG_LEN]++",
        "outLen += 32",
        "memcpy(state, ct, RLWE_CCA_CT_LEN)",
        "memcpy(state + RLWE_CCA_CT_LEN, Kr, RLWE_SEED_LEN)",
        "pseudoXOF(RLWE_KEY_LEN << 3, state,",
    )
    for needle in required:
        if needle not in kem:
            raise SystemExit(f"NOT REPRODUCED kem-02-8: Amoeba-{size} lacks {needle!r}")
    if "static void sm3_bit_compress" not in sm3 or "sm3_bit(msg, msg_len_bits, digest)" not in sm3:
        raise SystemExit(f"NOT REPRODUCED kem-02-8: Amoeba-{size} no SM3 compression path")
    print(f"CONFIRMED kem-02-8: Amoeba-{size} shares one 256-bit first-block SM3 state across G outputs")

print("BOUND kem-02-8: 2^256 chaining states; random 512-bit key false-match probability <= 2^-256")
