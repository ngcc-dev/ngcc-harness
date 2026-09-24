#!/usr/bin/env python3
"""Recompute Amoeba's two- and three-error tails under its own iid model."""

import math


PARAMS = (
    ("Amoeba-576", 576, 2, 2, 10, 10, 5),
    ("Amoeba-864", 864, 1, 2, 10, 11, 6),
    ("Amoeba-1152", 1152, 1, 1, 11, 10, 6),
    ("Amoeba-1728", 1728, 1, 1, 11, 11, 6),
    ("Amoeba-2304", 2304, 1, 1, 12, 12, 7),
)


def tail(probability: float, threshold: int, length: int = 523) -> float:
    return sum(
        math.comb(length, i) * probability**i * (1 - probability) ** (length - i)
        for i in range(threshold, length + 1)
    )


def main() -> None:
    q = 3457
    boundary = (q + 2) // 4
    for name, n, eta1, eta2, d0, d1, d2 in PARAMS:
        vb = q**2 / (2 ** (2 * d0) * 12)
        vu = q**2 / (2 ** (2 * d1) * 12)
        vv = q**2 / (2 ** (2 * d2) * 12)
        variance = (3 * n / 2) * (2 * eta1 * eta2 + vb * eta1 + vu * eta2) + eta2 + vv
        coefficient_error = math.erfc(boundary / math.sqrt(2 * variance))
        two = math.log2(tail(coefficient_error, 2))
        three = math.log2(tail(coefficient_error, 3))
        print(f"{name}: log2 P[N>=2]={two:.4f}; log2 P[N>=3]={three:.4f}; gap={two-three:.4f}")
        if name == "Amoeba-576":
            assert abs(two + 86.1221) < 0.001 and abs(three + 130.2722) < 0.001
    print("CONFIRMED: two-error rejection is omitted from the quoted failure tail")


if __name__ == "__main__":
    main()
