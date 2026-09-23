#!/usr/bin/env python3
"""Independently verify the published Octarine-512 relaxed-SIS certificate."""

import hashlib
import io
import urllib.request

import numpy as np


URL = (
    "https://raw.githubusercontent.com/amcrypto-jp/octarine-cryptanalysis/"
    "v1.0.1/data/sis_witness_5120.npz"
)
SHA256 = "b69e56e17c43ea946407b4647ec0bab508d0a6fd161fcb5a44bec469528e95f4"
N = 5120
Q = 1 << 24


def main() -> None:
    with urllib.request.urlopen(URL, timeout=60) as response:
        archive = response.read()
    assert hashlib.sha256(archive).hexdigest() == SHA256
    with np.load(io.BytesIO(archive), allow_pickle=False) as data:
        b, c, x = (data[name] for name in ("B_mod4", "C_mod4", "x"))
    assert b.shape == c.shape == (N, N) and x.shape == (2 * N,)
    assert np.all(x % (Q // 4) == 0)
    assert np.max(np.abs(x)) == Q // 4 and np.count_nonzero(x) == 7683

    # Products in uint8 wrap modulo 256, preserving the needed modulo-4 result.
    w = np.asarray(x // (Q // 4), dtype=np.uint8)
    residual = (b @ w[:N] + c @ w[N:]) & 3
    assert not np.any(residual)
    changed = w.copy()
    changed[0] = 0
    assert np.any((b @ changed[:N] + c @ changed[N:]) & 3)

    euf_bound = 2 * (1 << 20) + 1 + 4 * (1 << 13) * 87
    suf_bound = max(2 * ((1 << 21) - 174), 4 * (1 << 20) + 2)
    strict_response_bound = 2 * ((1 << 21) - 174)
    assert euf_bound == 4_947_969 and suf_bound == 4_194_306
    assert Q // 4 <= min(euf_bound, suf_bound)
    assert Q // 4 >= strict_response_bound
    print(
        "CONFIRMED sign-16-2: full 5120-row relaxed-SIS certificate; "
        "actual signature response bound fails"
    )


if __name__ == "__main__":
    main()
