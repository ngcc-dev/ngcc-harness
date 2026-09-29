#!/usr/bin/env python3
"""Show the COMPASS-SIG XOF replay in freshly generated secret keys."""

from __future__ import annotations

import ctypes
from pathlib import Path


ROOT = Path(__file__).resolve().parent
PARAMS = {
    128: (256, 4, 3),
    256: (256, 7, 7),
    384: (512, 6, 5),
    512: (512, 7, 7),
}


def unpack_eta1(data: bytes, n: int) -> list[int]:
    out: list[int] = []
    for byte in data:
        out.extend(1 - ((byte >> shift) & 3) for shift in (0, 2, 4, 6))
    return out[:n]


def repeated_tail(poly: list[int]) -> int:
    n = len(poly)
    return max(
        (n - split for split in range(n // 2, n) if poly[split:] == poly[: n - split]),
        default=0,
    )


def fresh_secret_tails(mode: int) -> list[int]:
    n, ell, kay = PARAMS[mode]
    path = ROOT / "lib" / f"libCOMPASS-SIG-{mode}.so"
    if not path.is_file():
        raise SystemExit(f"missing {path}; run: make -C sign-06")
    lib = ctypes.CDLL(str(path))
    lib.sig_get_pk_len_bytes.restype = ctypes.c_ulonglong
    lib.sig_get_sk_len_bytes.restype = ctypes.c_ulonglong
    pk_len = lib.sig_get_pk_len_bytes()
    sk_len = lib.sig_get_sk_len_bytes()
    pk = (ctypes.c_ubyte * pk_len)()
    sk = (ctypes.c_ubyte * sk_len)()
    pkl = ctypes.c_ulonglong(pk_len)
    skl = ctypes.c_ulonglong(sk_len)
    seed = bytes((17 * i + mode) & 0xFF for i in range(48))
    seed_buf = (ctypes.c_ubyte * len(seed)).from_buffer_copy(seed)
    if lib.ngcc_seed(seed_buf, len(seed)) != 0:
        raise RuntimeError(f"COMPASS-SIG-{mode}: ngcc_seed failed")
    if lib.sig_keygen(pk, ctypes.byref(pkl), sk, ctypes.byref(skl)) != 0:
        raise RuntimeError(f"COMPASS-SIG-{mode}: sig_keygen failed")

    raw = bytes(sk)
    offset = 32 + 32 + 64
    packed = n // 4
    tails: list[int] = []
    for _ in range(ell + kay):
        poly = unpack_eta1(raw[offset : offset + packed], n)
        tails.append(repeated_tail(poly))
        offset += packed
    return tails


def check_sources() -> None:
    copies = sorted((ROOT / "Implementations").glob("**/COMPASS-SIG-*/symmetric-shake.c"))
    if len(copies) != 8:
        raise RuntimeError(f"expected 8 parameter-specific wrapper copies, found {len(copies)}")
    for path in copies:
        text = path.read_text(errors="replace")
        start = text.index("static int fill_squeeze")
        end = text.index("static void state_cleanup", start)
        body = text[start:end]
        if "state->squeeze_pos = 0;" not in body or "pseudoXOF(want * 8" not in body:
            raise RuntimeError(f"wrapper pattern changed in {path}")


def main() -> int:
    check_sources()
    for mode in PARAMS:
        tails = fresh_secret_tails(mode)
        if mode < 384:
            if max(tails) >= 32:
                raise RuntimeError(f"COMPASS-SIG-{mode}: control has a long repeated tail: {tails}")
            verdict = "NOT-CONFIRMED [control]"
        else:
            if min(tails) < 64:
                raise RuntimeError(f"COMPASS-SIG-{mode}: missing duplicated secret tail: {tails}")
            verdict = "CONFIRMED"
        free = sum((PARAMS[mode][0] - t) / PARAMS[mode][0] for t in tails) / len(tails)
        print(
            f"ATTACK xof-replay COMPASS-SIG-{mode} {verdict} "
            f"polynomials={len(tails)} tail_min={min(tails)} tail_max={max(tails)} "
            f"mean_free_fraction={free:.3f}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
