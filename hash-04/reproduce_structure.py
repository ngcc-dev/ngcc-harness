#!/usr/bin/env python3
"""Exact arithmetic certificates for CHAMP findings hash-04-1 and hash-04-2."""

from fractions import Fraction


SETS = (
    ("CHAMP-512", 2**128 - 15449, 192),
    ("CHAMP-1024", 2**256 - 36113, 384),
)
A = ((-2, 1), (4, -3))
B = ((-5, 2), (-6, 2))


def det(matrix):
    return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]


def multiply(left, right):
    return tuple(
        tuple(sum(left[i][k] * right[k][j] for k in range(2)) for j in range(2))
        for i in range(2)
    )


def subtract(left, right):
    return tuple(tuple(left[i][j] - right[i][j] for j in range(2)) for i in range(2))


assert det(A) == 2
assert det(B) == 2
assert det(subtract(A, B)) == -5
assert det(subtract(multiply(A, B), multiply(B, A))) == 2

for name, prime, sample_exponent in SETS:
    fiber_size = prime * (prime * prime - 1)
    samples = 1 << sample_exponent
    # For any distribution on at most N values, the no-collision probability
    # is maximized by the uniform distribution and is at most exp(-x), where
    # x=t(t-1)/(2N).  x>1/2-2^-191 here, so the collision probability exceeds
    # 1-exp(-1/2), approximately 0.393469.
    exponent = Fraction(samples * (samples - 1), 2 * fiber_size)
    assert exponent > Fraction(49, 100)
    print(
        f"CONFIRMED hash-04-1 {name}: determinant fiber has "
        f"{fiber_size.bit_length()}-bit cardinality bound; "
        f"2^{sample_exponent} samples give collision probability >0.39"
    )

print(
    "CONFIRMED hash-04-2 premises: det(A)=det(B)=2, det(A-B)=-5, "
    "det(AB-BA)=2"
)
print(
    "LIMITATION hash-04-2: the positive-word collision estimate remains "
    "heuristic and is not instantiated"
)

