#!/usr/bin/env python3
"""Validate the order-19 quotient hidden in the published QCTM KAT keys.

The quotient parity check is derived from the compressed public key.  The KAT
secret key is used only as an independent oracle: it supplies the permuted
support and Goppa polynomial with which to construct a degree-t0 binary Goppa
check matrix.  Equality of the two binary row spaces is then checked exactly.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Params:
    name: str
    n: int
    t: int
    modulus: int

    m: int = 18
    ell: int = 19

    @property
    def t0(self) -> int:
        return self.t // self.ell

    @property
    def codimension(self) -> int:
        return self.m * (self.t - 1) + 1

    @property
    def qc_rows(self) -> int:
        return self.codimension - self.codimension % self.ell

    @property
    def quotient_length(self) -> int:
        return self.n // self.ell


PARAMS = {
    "QCTM128": Params("QCTM128", 10070, 285, (1 << 18) | (1 << 3) | 1),
    "QCTM256": Params("QCTM256", 19000, 513, (1 << 18) | (1 << 7) | 1),
    "QCTM512": Params("QCTM512", 38000, 1007, (1 << 18) | (1 << 3) | 1),
}


def kat_value(path: Path, label: str, count: int = 0) -> bytes:
    current = None
    prefix = f"{label} = "
    for line in path.read_text().splitlines():
        if line.startswith("Count = "):
            current = int(line.split("=", 1)[1])
        elif current == count and line.startswith(prefix):
            return bytes.fromhex(line[len(prefix) :])
    raise ValueError(f"{label} for Count = {count} not found in {path}")


def bit_lsb(buf: bytes, position: int) -> int:
    return (buf[position // 8] >> (position % 8)) & 1


def bit_msb(buf: bytes, position: int) -> int:
    return (buf[position // 8] >> (7 - position % 8)) & 1


def public_quotient_check(pk: bytes, p: Params) -> list[int]:
    """Return rows of H restricted to vectors constant on each 19-cycle."""
    left_blocks = p.qc_rows // p.ell
    right_columns = p.n - p.qc_rows
    right_blocks = right_columns // p.ell
    tail_rows = p.codimension - p.qc_rows
    rows = [1 << i for i in range(left_blocks)] + [0] * tail_rows

    for row in range(left_blocks):
        base = row * right_columns
        for block in range(right_blocks):
            parity = 0
            for j in range(p.ell):
                parity ^= bit_lsb(pk, base + block * p.ell + j)
            rows[row] |= parity << (left_blocks + block)

    tail_base = left_blocks * right_columns
    for tail in range(tail_rows):
        base = tail_base + tail * right_columns
        for block in range(right_blocks):
            parity = 0
            for j in range(p.ell):
                parity ^= bit_lsb(pk, base + block * p.ell + j)
            rows[left_blocks + tail] |= parity << (left_blocks + block)
    return rows


class GF2m:
    def __init__(self, m: int, modulus: int):
        self.m = m
        self.modulus = modulus
        self.mask = (1 << m) - 1

    def mul(self, a: int, b: int) -> int:
        out = 0
        while b:
            if b & 1:
                out ^= a
            b >>= 1
            a <<= 1
            if a >> self.m:
                a ^= self.modulus
        return out & self.mask

    def square(self, a: int) -> int:
        return self.mul(a, a)

    def pow(self, a: int, exponent: int) -> int:
        out = 1
        while exponent:
            if exponent & 1:
                out = self.mul(out, a)
            exponent >>= 1
            if exponent:
                a = self.square(a)
        return out

    def inv(self, a: int) -> int:
        if not a:
            raise ZeroDivisionError
        return self.pow(a, (1 << self.m) - 2)


def read_field_elements(buf: bytes, count: int, m: int) -> list[int]:
    values = []
    bitpos = 0
    for _ in range(count):
        value = 0
        for j in range(m):
            value |= bit_msb(buf, bitpos) << j
            bitpos += 1
        values.append(value)
    return values


def evaluate(poly: list[int], x: int, field: GF2m) -> int:
    out = 1  # serialized polynomial is monic and omits its leading coefficient
    for coefficient in reversed(poly):
        out = field.mul(out, x) ^ coefficient
    return out


def secret_quotient_description(
    sk: bytes, p: Params
) -> tuple[GF2m, list[int], list[int]]:
    gamma_bytes = ((p.n + p.t) * p.m + 7) // 8
    elements = read_field_elements(sk[:gamma_bytes], p.t + p.n, p.m)
    polynomial = elements[: p.t]
    support = elements[p.t :]
    field = GF2m(p.m, p.modulus)

    # g(x)=M((x-a)^ell), with a=1/eta equal to the x^(t-1)
    # coefficient of the monic serialized g.
    a = polynomial[-1]
    betas = []
    values = []
    for orbit in range(p.quotient_length):
        alpha = support[orbit * p.ell]
        beta = field.pow(alpha ^ a, p.ell)
        betas.append(beta)
        values.append(evaluate(polynomial, alpha, field))
    return field, betas, values


def secret_goppa_quotient_check(
    field: GF2m, betas: list[int], values: list[int], p: Params
) -> list[int]:
    rows = [0] * (p.m * p.t0)
    for orbit, beta in enumerate(betas):
        value = field.inv(values[orbit])
        for degree in range(p.t0):
            for bit in range(p.m):
                rows[degree * p.m + bit] |= ((value >> bit) & 1) << orbit
            value = field.mul(value, beta)
    rows.append((1 << p.quotient_length) - 1)
    return rows


def puncture_check(rows: list[int], coordinate: int) -> list[int]:
    """Parity check of a punctured code via shortening of its dual."""
    pivot = next(row for row in rows if (row >> coordinate) & 1)
    cleared = [row ^ pivot if (row >> coordinate) & 1 else row for row in rows]
    cleared.remove(0)
    low_mask = (1 << coordinate) - 1
    return [
        (row & low_mask) | ((row >> (coordinate + 1)) << coordinate)
        for row in cleared
    ]


def secret_punctured_goppa_check(
    field: GF2m,
    betas: list[int],
    values: list[int],
    p: Params,
    coordinate: int,
) -> list[int]:
    """Ordinary Goppa representation after sending beta[coordinate] to infinity."""
    rows = [0] * (p.m * p.t0)
    beta_j = betas[coordinate]
    output_column = 0
    for source_column, beta_i in enumerate(betas):
        if source_column == coordinate:
            continue
        z_i = field.inv(beta_i ^ beta_j)
        # M_j(z)=z^t0 M(beta_j+1/z), hence
        # 1/M_j(z_i)=1/(z_i^t0 M(beta_i)).
        denominator = field.mul(field.pow(z_i, p.t0), values[source_column])
        value = field.inv(denominator)
        for degree in range(p.t0):
            for bit in range(p.m):
                rows[degree * p.m + bit] |= ((value >> bit) & 1) << output_column
            value = field.mul(value, z_i)
        output_column += 1
    return rows


def rank(rows: list[int]) -> int:
    pivots: dict[int, int] = {}
    for value in rows:
        while value:
            pivot = value.bit_length() - 1
            if pivot not in pivots:
                pivots[pivot] = value
                break
            value ^= pivots[pivot]
    return len(pivots)


def audit(root: Path, p: Params, count: int) -> None:
    kat = root / "kem-32" / "Test_Vectors" / f"KAT_KEM_{p.name}.txt"
    pk = kat_value(kat, "PK", count)
    sk = kat_value(kat, "SK", count)
    public = public_quotient_check(pk, p)
    field, betas, values = secret_quotient_description(sk, p)
    oracle = secret_goppa_quotient_check(field, betas, values, p)
    public_rank = rank(public)
    oracle_rank = rank(oracle)
    union_rank = rank(public + oracle)
    expected = p.m * p.t0 + 1
    equal = public_rank == oracle_rank == union_rank
    print(
        f"{p.name} KAT {count}: original=[{p.n},{p.n-p.codimension}] "
        f"quotient=[{p.quotient_length},{p.quotient_length-public_rank}] "
        f"rank(public)={public_rank} rank(Goppa+parity)={oracle_rank} "
        f"rank(union)={union_rank} expected={expected} "
        f"row_spaces_equal={'yes' if equal else 'NO'}"
    )
    if not equal or public_rank != expected:
        raise SystemExit(1)

    punctured_public = puncture_check(public, 0)
    punctured_oracle = secret_punctured_goppa_check(
        field, betas, values, p, coordinate=0
    )
    punctured_public_rank = rank(punctured_public)
    punctured_oracle_rank = rank(punctured_oracle)
    punctured_union_rank = rank(punctured_public + punctured_oracle)
    punctured_equal = (
        punctured_public_rank
        == punctured_oracle_rank
        == punctured_union_rank
        == p.m * p.t0
    )
    print(
        f"{p.name} KAT {count}: punctured quotient="
        f"[{p.quotient_length-1},{p.quotient_length-1-punctured_public_rank}] "
        f"rank(public)={punctured_public_rank} "
        f"rank(proper-Goppa)={punctured_oracle_rank} "
        f"rank(union)={punctured_union_rank} "
        f"row_spaces_equal={'yes' if punctured_equal else 'NO'}"
    )
    if not punctured_equal:
        raise SystemExit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("sets", nargs="*", choices=sorted(PARAMS))
    parser.add_argument("--count", type=int, default=0)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    for name in args.sets or PARAMS:
        audit(args.root, PARAMS[name], args.count)


if __name__ == "__main__":
    main()
