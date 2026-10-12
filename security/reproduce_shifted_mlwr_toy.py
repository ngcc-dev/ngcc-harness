#!/usr/bin/env python3
"""Exact small-instance counterexample to lossless shift/postprocessing in MLWR.

This is not an attack on Loom or Weaver parameters.
"""

from collections import defaultdict
from fractions import Fraction


Q = 17
PRECISION = 3
ETA = 3
SHIFT = 8


def cbd() -> dict[int, Fraction]:
    weights = {0: 1}
    for _ in range(ETA):
        next_weights: dict[int, int] = defaultdict(int)
        for value, count in weights.items():
            next_weights[value] += 2 * count
            next_weights[value + 1] += count
            next_weights[value - 1] += count
        weights = dict(next_weights)
    return {value: Fraction(count, 4**ETA) for value, count in weights.items()}


def compress(value: int) -> int:
    # Weaver §2.1: round(2^d (x mod q) / q), ties upward, mod 2^d.
    return ((2**PRECISION * (value % Q) + Q // 2) // Q) % (2**PRECISION)


def distance(shift: int) -> Fraction:
    # k=1, ell=2, with the public shift only in the second output as in Loom.
    random: dict[tuple[int, int], Fraction] = defaultdict(Fraction)
    for u0 in range(Q):
        for u1 in range(Q):
            random[compress(u0), compress(u1 + shift)] += Fraction(1, Q * Q)
    total = Fraction()
    for a0 in range(Q):
        for a1 in range(Q):
            real: dict[tuple[int, int], Fraction] = defaultdict(Fraction)
            for secret, probability in cbd().items():
                real[compress(a0 * secret), compress(a1 * secret + shift)] += probability
            total += sum((abs(real[key] - random[key]) for key in real.keys() | random.keys()), Fraction())
    return total / (2 * Q * Q)


def main() -> None:
    unshifted = distance(0)
    shifted = distance(SHIFT)
    assert unshifted == Fraction(73806, 83521)
    assert shifted == Fraction(2381945, 2672672)
    assert shifted > unshifted
    print(f"q={Q}, k=1, ell=2, d={PRECISION}, CBD_eta={ETA}, shift=(0,{SHIFT})")
    print(f"optimal one-sample distinguishing advantage: unshifted={float(unshifted):.6f}, shifted={float(shifted):.6f}")
    print("PROOF GAP kex-05-5 CONFIRMED: a public shift before rounding can increase distance")
    print("NO CONCRETE ATTACK: Loom and Weaver use much larger, structured parameters")


if __name__ == "__main__":
    main()
