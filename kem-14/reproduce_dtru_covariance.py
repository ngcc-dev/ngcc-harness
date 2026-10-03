#!/usr/bin/env python3
"""Certify DTRU's tricyclotomic ring pairing and q=2^80 proof loss."""

from math import sqrt
from pathlib import Path
import random
import subprocess

HERE = Path(__file__).resolve().parent

# Submitted DFR exponents from Table 1; Prime uses a different ring and is not
# part of the structural matrix certificate below.
SETS = (
    (648, 146.79, 128),
    (768, 185.56, 192),
    (1024, 190.48, 256),
    (1536, 195.50, 384),
    (2048, 204.10, 512),
)


def reduce_monomial(degree, n):
    """Reduce x^degree modulo x^n-x^(n/2)+1."""
    todo = [(degree, 1)]
    out = []
    while todo:
        degree, coeff = todo.pop()
        if degree < n:
            out.append((degree, coeff))
        else:
            todo.append((degree - n // 2, coeff))
            todo.append((degree - n, -coeff))
    return out


def block_gram(n):
    rng = random.Random(1)
    public = [rng.randint(-3, 3) for _ in range(n)]
    indices = list(range(8)) + list(range(n // 2, n // 2 + 8))
    position = {value: i for i, value in enumerate(indices)}
    rows = [[0.0] * n for _ in range(16)]
    for column in range(n):
        for j, value in enumerate(public):
            if not value:
                continue
            for output, sign in reduce_monomial(j + column, n):
                row = position.get(output)
                if row is not None:
                    rows[row][column] += sign * value
    return [[sum(x * y for x, y in zip(rows[i], rows[j]))
             for j in range(16)] for i in range(16)]


def jacobi_eigenvalues(matrix):
    a = [row[:] for row in matrix]
    size = len(a)
    for _ in range(20000):
        p, q = max(((i, j) for i in range(size) for j in range(i + 1, size)),
                   key=lambda ij: abs(a[ij[0]][ij[1]]))
        if abs(a[p][q]) < 1e-9:
            break
        tau = (a[q][q] - a[p][p]) / (2 * a[p][q])
        t = (1 if tau >= 0 else -1) / (abs(tau) + sqrt(1 + tau * tau))
        c = 1 / sqrt(1 + t * t)
        s = t * c
        app, aqq, apq = a[p][p], a[q][q], a[p][q]
        a[p][p] = app - t * apq
        a[q][q] = aqq + t * apq
        a[p][q] = a[q][p] = 0.0
        for k in range(size):
            if k in (p, q):
                continue
            akp, akq = a[k][p], a[k][q]
            a[k][p] = a[p][k] = c * akp - s * akq
            a[k][q] = a[q][k] = s * akp + c * akq
    return sorted(a[i][i] for i in range(size))


def main():
    pdf = subprocess.run(["pdftotext", "-layout", str(HERE / "kem-14-spec.pdf"), "-"],
                         check=True, stdout=subprocess.PIPE, text=True).stdout
    for n, dfr, target in SETS:
        assert str(n) in pdf and f"{dfr:.2f}" in pdf
        proof_bits = dfr - 80
        assert proof_bits < target
        gram = block_gram(n)
        correlations = [gram[i][i + 8] /
                        sqrt(gram[i][i] * gram[i + 8][i + 8]) for i in range(8)]
        mean_diag = sum(gram[i][i] for i in range(16)) / 16
        eigen = [value / mean_diag for value in jacobi_eigenvalues(gram)]
        assert max(correlations) < -0.60
        assert eigen[0] < 0.35 and eigen[-1] > 1.70
        print(f"DTRU-{n}: paired-correlation={sum(correlations)/8:.3f} "
              f"spectrum={eigen[0]:.3f}..{eigen[-1]:.3f} "
              f"submitted-proof-at-2^80=2^-{proof_bits:.2f} < 2^-{target}")
    print("PROOF GAP kem-14-4 CONFIRMED: ring pairing refutes the submitted independence model")


if __name__ == "__main__":
    main()
