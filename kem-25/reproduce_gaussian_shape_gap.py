#!/usr/bin/env python3
"""Check the matrix premise used by NEV's Lemma 2; not a KEM attack."""

from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
spec = subprocess.run(
    ["pdftotext", "-layout", str(HERE / "kem-25-spec.pdf"), "-"],
    check=True, capture_output=True, text=True,
).stdout
flat = " ".join(spec.split())
assert "Lemma 2 (Special case" in flat
assert "marginal distribution of x1 is statistically close" in flat
assert "Theorem 1. Let n, q, k" in flat
assert "By Lemma 2" in flat
assert "k = 4 for every set" in (HERE / "pseudocode.md").read_text()


def matmul(a: list[list[int]], b: list[list[int]]) -> list[list[int]]:
    return [[sum(x * y for x, y in zip(row, col)) for col in zip(*b)]
            for row in a]


def transpose(a: list[list[int]]) -> list[list[int]]:
    return [list(row) for row in zip(*a)]


def negacyclic_shift(n: int, step: int) -> list[list[int]]:
    shift = [[0] * n for _ in range(n)]
    for col in range(n):
        pos = col + step
        shift[pos % n][col] = -1 if pos >= n else 1
    return shift


n = 8
for k in (2, 4):
    s = negacyclic_shift(n, n // k)
    v = [[(i == j) - s[i][j] for j in range(n)] for i in range(n)]
    gram = matmul(v, transpose(v))
    identity = [[2 * (i == j) - s[i][j] - s[j][i]
                 for j in range(n)] for i in range(n)]
    assert gram == identity
    offdiag = [(i, j, gram[i][j]) for i in range(n) for j in range(n)
               if i != j and gram[i][j]]
    if k == 2:
        assert not offdiag
    else:
        assert offdiag and all(gram[i][i] == 2 for i in range(n))
        gram_squared = matmul(gram, gram)
        assert all(gram_squared[i][j] - 4 * gram[i][j] + 2 * (i == j) == 0
                   for i in range(n) for j in range(n))
    print(f"k={k} nonzero_off_diagonal_entries={len(offdiag)}")

print("k=4 Gram eigenvalues=2±sqrt(2)")
print("LIMITATION: no NEV attack or concrete parameter re-estimate is shown")
print("PROOF GAP kem-25-2 CONFIRMED: k=4 transformed error is not spherical")
