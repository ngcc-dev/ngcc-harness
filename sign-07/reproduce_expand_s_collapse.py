#!/usr/bin/env python3
"""Runtime and source certificate for the CS ExpandS collapse."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
PARAMS = {
    128: (256, 3, 3),
    256: (512, 3, 3),
    512: (512, 6, 5),
}


def key_blocks(bits: int, seed: bytes) -> list[bytes]:
    lib = ctypes.CDLL(str(ROOT / "lib" / f"libCS-{bits}.so"))
    lib.sig_get_pk_len_bytes.restype = ctypes.c_ulonglong
    lib.sig_get_sk_len_bytes.restype = ctypes.c_ulonglong
    pk_n, sk_n = lib.sig_get_pk_len_bytes(), lib.sig_get_sk_len_bytes()
    pk, sk = (ctypes.c_ubyte * pk_n)(), (ctypes.c_ubyte * sk_n)()
    pk_len, sk_len = ctypes.c_ulonglong(), ctypes.c_ulonglong()
    seed_buf = (ctypes.c_ubyte * len(seed)).from_buffer_copy(seed)
    assert lib.ngcc_seed(seed_buf, len(seed)) == 0
    assert lib.sig_keygen(pk, ctypes.byref(pk_len), sk, ctypes.byref(sk_len)) == 0
    assert (pk_len.value, sk_len.value) == (pk_n, sk_n)

    n, k, ell = PARAMS[bits]
    seedbytes, hbytes = bits // 8, bits // 4
    offset = 2 * seedbytes + hbytes
    polybytes = n * 2 // 8
    raw = bytes(sk)
    return [raw[offset + i * polybytes : offset + (i + 1) * polybytes] for i in range(k + ell)]


def main() -> None:
    source_files = sorted((ROOT / "Implementations").glob("*_Implementation/CS-*/sampling.c"))
    assert len(source_files) == 6
    for path in source_files:
        text = path.read_text()
        assert "rho[HBYTES] = nonce & 0xFF;" in text
        assert "H_Init(&state, rho, SEEDBYTES + 2);" in text

    seed = bytes(range(48))
    changed = bytes([seed[0] ^ 1]) + seed[1:]
    for bits in PARAMS:
        blocks = key_blocks(bits, seed)
        assert len(set(blocks)) == 1
        control = key_blocks(bits, changed)
        assert control[0] != blocks[0]
        print(f"CS-{bits}: identical_secret_blocks={len(blocks)}/{len(blocks)} control=DIFFERENT")
    print("ATTACK sign-07-4 CONFIRMED: ExpandS nonce is outside the XOF input")


if __name__ == "__main__":
    main()
