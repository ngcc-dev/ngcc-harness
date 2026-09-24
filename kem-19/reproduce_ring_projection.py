#!/usr/bin/env python3
"""Check Lore-512's exact two-factor CRT projection over Z/1028Z.

This is an algebraic preflight, not a lattice key-recovery experiment.
"""

import random


Q = 1028
N = 768
INV3 = 343  # 3 * 343 = 1 (mod 1028)


def project(a: list[int]) -> tuple[list[int], list[int]]:
    short = [(a[i] - a[i + 256] + a[i + 512]) % Q for i in range(256)]
    long = [(a[i] - a[i + 512]) % Q for i in range(256)]
    long += [(a[i + 256] + a[i + 512]) % Q for i in range(256)]
    return short, long


def reconstruct(short: list[int], long: list[int]) -> list[int]:
    # B-(y-2)A=3 for y=x^256, A=y+1, B=y^2-y+1.
    # Thus e_short=B/3 and e_long=-(y-2)A/3 are CRT idempotents.
    result = [0] * N
    for residue, terms in ((short, ((0, 1), (256, -1), (512, 1))),
                           (long, ((0, 2), (256, 1), (512, -1)))):
        for i, coefficient in enumerate(residue):
            for shift, multiplier in terms:
                degree = i + shift
                sign = -1 if degree >= N else 1
                result[degree % N] = (result[degree % N] +
                                      sign * INV3 * multiplier * coefficient) % Q
    return result


def multiply_original(a: list[int], b: list[int]) -> list[int]:
    result = [0] * N
    for i, ai in enumerate(a):
        if ai == 0:
            continue
        for j, bj in enumerate(b):
            if bj == 0:
                continue
            degree = i + j
            result[degree % N] += (-1 if degree >= N else 1) * ai * bj
    return [x % Q for x in result]


def multiply_factor(a: list[int], b: list[int], degree: int) -> list[int]:
    product = [0] * (2 * degree - 1)
    for i, ai in enumerate(a):
        if ai:
            for j, bj in enumerate(b):
                if bj:
                    product[i + j] += ai * bj
    if degree == 256:  # x^256 = -1
        for k in range(len(product) - 1, degree - 1, -1):
            product[k - degree] -= product[k]
    else:  # x^512 = x^256 - 1
        for k in range(len(product) - 1, degree - 1, -1):
            product[k - degree] -= product[k]
            product[k - 256] += product[k]
    return [x % Q for x in product[:degree]]


def main() -> None:
    rng = random.Random(19)
    a = [rng.choice((-2, -1, 0, 0, 0, 1, 2)) for _ in range(N)]
    b = [0] * N
    for position in (0, 11, 255, 256, 399, 512, 767):
        b[position] = rng.choice((-2, -1, 1, 2))
    short, long = project(a)
    assert reconstruct(short, long) == [x % Q for x in a]
    actual = project(multiply_original(a, b))
    b_short, b_long = project(b)
    assert actual[0] == multiply_factor(short, b_short, 256)
    assert actual[1] == multiply_factor(long, b_long, 512)
    assert all(-6 <= (x if x <= Q // 2 else x - Q) <= 6 for x in short)
    assert all(-4 <= (x if x <= Q // 2 else x - Q) <= 4 for x in long)
    print("CONFIRMED: Lore-512 splits into degree-256 and degree-512 quotient rings")
    print("CONFIRMED: projected small secret remains bounded and CRT reconstruction is exact")
    print("LIMITATION: no lattice cost or full key recovery is verified")


if __name__ == "__main__":
    main()
