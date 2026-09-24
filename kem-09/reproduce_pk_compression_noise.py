#!/usr/bin/env python3
"""Check kem-09-3: public-key rounding omitted from Cheetah's DFR derivation.

This is an exact one-coefficient rounding calculation, not a full DFR estimate.
"""

from fractions import Fraction


Q = 7681
N = 640
QBITS = 13
SETS = (
    ("Cheetah128", 1, 5, 10),
    ("Cheetah256", 2, 4, 10),
    ("Cheetah384", 3, 3, 11),
    ("Cheetah512", 4, 2, 11),
)


def rounding_error(x: int, shift: int) -> int:
    # poly.c: compress(x) = x >> shift;
    # decompress(y) = min((y << shift) + 2^(shift-1), q-1).
    y = x >> shift
    return min((y << shift) + (1 << (shift - 1)), Q - 1) - x


def main() -> None:
    for name, rank, eta, db in SETS:
        shift = QBITS - db
        errors = [rounding_error(x, shift) for x in range(Q)]
        second_moment = Fraction(sum(e * e for e in errors), Q)
        # CBD(eta) has zero mean and second moment eta/2. Each product
        # coefficient of e_pk * r has rank*N terms in the iid surrogate.
        added_variance = rank * N * Fraction(eta, 2) * second_moment
        assert min(errors) < 0 < max(errors)
        assert added_variance > 0
        print(
            f"{name}: e_pk in [{min(errors)},{max(errors)}], "
            f"E[e_pk^2]={float(second_moment):.5f}, "
            f"iid Var(e_pk*r)={float(added_variance):.2f}"
        )
    print("CONFIRMED: key compression adds a nonzero e_pk*r noise term")
    print("LIMITATION: no exact DFR or full KEM failure rate is computed")


if __name__ == "__main__":
    main()
