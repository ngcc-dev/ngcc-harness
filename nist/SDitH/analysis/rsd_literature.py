#!/usr/bin/env python3
"""Reproduce published algebraic-RSD estimates on SDitH v3 parameters.

This script uses only the Python standard library.  It implements:

* Briaud--Oeygarden 2023, Modeling 2, Equation (24) and Proposition 9;
* Cueto Noval et al. 2025, the degree-2 equation-counting threshold and
  the two large-field hypotheses in Theorem 2; and
* Wang et al. 2025, the optimized binary attack in Theorem 4.

These are attack estimators, not cryptographic lower bounds.  See README.md in
this directory for source links, modeling limits, and parameter conventions.
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Iterable, NamedTuple


@dataclass(frozen=True)
class RSDInstance:
    name: str
    n: int
    k: int
    weight: int
    target: int

    @property
    def block_size(self) -> int:
        quotient, remainder = divmod(self.n, self.weight)
        if remainder:
            raise ValueError(f"{self.name}: n must be divisible by the weight")
        return quotient


SDITH = (
    RSDInstance("category I", 10_360, 9_928, 56, 143),
    RSDInstance("category III", 18_396, 17_804, 73, 207),
    RSDInstance("category V", 19_864, 19_064, 104, 272),
)


class BOEstimate(NamedTuple):
    cost: float
    fixed_blocks: int
    zeros_per_block: int
    witness_degree: int
    probability_cost: float
    matrix_log2: float


class WangEstimate(NamedTuple):
    cost: float
    f_bar: int
    u_bar: int
    coefficient_blocks: int
    probability_cost: float
    iteration_cost: float
    remaining_variables: int


def log2_sum(values: Iterable[float]) -> float:
    """Return log2(sum(2**x for x in values)) without overflowing."""

    values = tuple(values)
    maximum = max(values)
    return maximum + math.log2(sum(2.0 ** (value - maximum) for value in values))


def normalize_blocks(n: int, k: int, weight: int) -> tuple[int, int, int]:
    """Apply the papers' convention when n is not a multiple of the weight.

    The final residual coordinates are fixed to zero.  The number of parity
    equations n-k stays fixed, so k decreases by the number removed.
    SDitH itself needs no normalization; the published benchmark checks do.
    """

    block_size = n // weight
    normalized_n = block_size * weight
    normalized_k = k - (n - normalized_n)
    return normalized_n, normalized_k, block_size


def bo_degree_and_monomials(
    *,
    weight: int,
    full_block_terms: int,
    fixed_blocks: int,
    fixed_block_terms: int,
    parity_equations: int,
    base_cost: float,
    incumbent_cost: float,
    maximum_degree: int = 256,
) -> tuple[int, int] | None:
    """Evaluate B&O (24), divided by (1-z), and its monomial series.

    ``degree_series`` holds coefficients of

      (1+a1*z)^f (1+a0*z)^(w-f) / ((1+z)^m (1-z)).

    The first non-positive coefficient estimates the witness degree.  The
    second recurrence computes coefficients of the numerator, whose partial
    sum is M_{<=d} in Proposition 9.  Newton's recurrence avoids symbolic
    polynomial dependencies and retains exact integer coefficients.
    """

    degree_series = [1]
    monomial_series = [1]
    monomial_sum = 1

    for degree in range(1, maximum_degree + 1):
        degree_numerator = 0
        monomial_numerator = 0
        for offset in range(degree):
            sign = -1 if offset % 2 else 1
            numerator_power_sum = sign * (
                fixed_blocks * fixed_block_terms ** (offset + 1)
                + (weight - fixed_blocks) * full_block_terms ** (offset + 1)
            )
            # Dividing by (1+z)^m subtracts m*(-1)^offset.  Dividing by
            # (1-z) contributes +1 to every logarithmic-derivative term.
            degree_power_sum = (
                numerator_power_sum - sign * parity_equations + 1
            )
            degree_numerator += degree_power_sum * degree_series[degree - 1 - offset]
            monomial_numerator += (
                numerator_power_sum * monomial_series[degree - 1 - offset]
            )

        if degree_numerator % degree or monomial_numerator % degree:
            raise AssertionError("Newton recurrence lost integer divisibility")
        degree_series.append(degree_numerator // degree)
        monomial_series.append(monomial_numerator // degree)
        monomial_sum += monomial_series[-1]

        # M_{<=d} is increasing.  Once its lower-bound cost cannot beat an
        # existing candidate, no later degree for this (f,u) can beat it.
        if (
            degree >= 2
            and math.isfinite(incumbent_cost)
            and base_cost + 2.0 * math.log2(monomial_sum) >= incumbent_cost
        ):
            return None

        if degree_series[-1] <= 0:
            return degree, monomial_sum

    raise RuntimeError(
        f"no Hilbert-series fall through degree {maximum_degree}; "
        "increase maximum_degree"
    )


def estimate_briaud_oeygarden(n: int, k: int, weight: int) -> BOEstimate:
    """Minimize the B&O Proposition 9 binary Modeling-2 estimator."""

    n, k, block_size = normalize_blocks(n, k, weight)
    parity_equations = n - k
    best: BOEstimate | None = None

    # B&O Section 7 excludes the region already reduced to regular Prange:
    # u < k/h - 1.  Descending u/f finds a low-degree incumbent promptly.
    valid_u = [
        u for u in range(block_size - 1) if (u + 1) * weight < k
    ]
    for zeros_per_block in reversed(valid_u):
        for fixed_blocks in reversed(range(weight + 1)):
            if fixed_blocks * zeros_per_block >= k + 1:
                continue

            probability_cost = -fixed_blocks * math.log2(
                1.0 - zeros_per_block / block_size
            )
            # Appendix B.2 and the discussion before the result tables use
            # this sparse-row estimate over F_2.
            nonzero_terms = min(
                k + 1 - fixed_blocks * zeros_per_block,
                k / 2.0 + 1,
            )
            base_cost = probability_cost + math.log2(3.0 * nonzero_terms)
            if best is not None and base_cost >= best.cost:
                continue

            result = bo_degree_and_monomials(
                weight=weight,
                full_block_terms=block_size - 1,
                fixed_blocks=fixed_blocks,
                fixed_block_terms=block_size - 1 - zeros_per_block,
                parity_equations=parity_equations,
                base_cost=base_cost,
                incumbent_cost=best.cost if best is not None else math.inf,
            )
            if result is None:
                continue
            witness_degree, monomial_sum = result
            if witness_degree < 2:
                raise AssertionError("B&O search entered the excluded Prange region")
            cost = base_cost + 2.0 * math.log2(monomial_sum)
            candidate = BOEstimate(
                cost,
                fixed_blocks,
                zeros_per_block,
                witness_degree,
                probability_cost,
                math.log2(monomial_sum),
            )
            if best is None or candidate.cost < best.cost:
                best = candidate

    if best is None:
        raise RuntimeError("B&O search found no candidate")
    return best


def threshold_ratios(instance: RSDInstance) -> tuple[float, float, bool, bool]:
    """Return the 2025 degree-2 ratio, asymptotic ratio, and field tests."""

    block_size = instance.block_size
    exact = (
        instance.weight * math.comb(block_size, 2)
        / math.comb(instance.k + 1, 2)
    )
    asymptotic = (
        instance.weight * block_size * block_size
        / (2.0 * instance.k * instance.k)
    )
    q = 2
    return exact, asymptotic, q >= block_size - 1, q >= math.sqrt(block_size)


def estimate_wang(n: int, k: int, weight: int) -> WangEstimate:
    """Minimize Wang et al. Theorem 4 for binary RSD."""

    n, k, block_size = normalize_blocks(n, k, weight)
    parity_equations = n - k
    full_blocks_in_parity_part = parity_equations // (block_size - 1)
    best: WangEstimate | None = None

    for coefficient_blocks in range(weight + 1):
        coefficient_variables = coefficient_blocks * (block_size - 1)
        for f_bar in range(weight - coefficient_blocks + 1):
            for u_bar in range(block_size - 1):
                n0 = k - weight - f_bar - (weight - coefficient_blocks) * u_bar
                remaining = n0 - coefficient_variables
                # The displayed T2/T3 formula contains log2(v^2/2) and
                # log2(5v/2); v=0 is a degenerate boundary, not a valid cheap
                # attack.  v=1 also makes the former denominator negative.
                if n0 <= 0 or remaining < 2:
                    continue

                m1 = (
                    f_bar * math.comb(block_size - u_bar - 2, 2)
                    + (weight - coefficient_blocks - f_bar)
                    * math.comb(block_size - u_bar - 1, 2)
                )
                # Theorem 4 condition (15): m1-v(v-1)/2 > 5v/2.
                if 2 * m1 <= remaining * (remaining + 4):
                    continue

                retained_parity_columns = full_blocks_in_parity_part * (
                    block_size - u_bar - 2
                )
                t1_left = parity_equations - retained_parity_columns
                t1_right = n0 + parity_equations - retained_parity_columns
                if t1_left <= 0 or t1_right <= 0:
                    continue
                t1 = parity_equations * t1_left * t1_right
                t2 = (
                    remaining**4
                    * n0**2
                    / (8.0 * math.log2(remaining * remaining / 2.0))
                )
                t3 = (
                    1.25
                    * (
                        2 * remaining * remaining * coefficient_variables
                        + remaining * coefficient_variables * coefficient_variables
                    )
                    + (2.5 * remaining) ** 3 / math.log2(2.5 * remaining)
                )

                iteration_cost = log2_sum(
                    (
                        math.log2(t1),
                        math.log2(t2),
                        coefficient_blocks * math.log2(block_size) + math.log2(t3),
                    )
                )
                probability_cost = (
                    -f_bar * math.log2(1.0 - (u_bar + 1) / block_size)
                    - (weight - coefficient_blocks - f_bar)
                    * math.log2(1.0 - u_bar / block_size)
                )
                candidate = WangEstimate(
                    probability_cost + iteration_cost,
                    f_bar,
                    u_bar,
                    coefficient_blocks,
                    probability_cost,
                    iteration_cost,
                    remaining,
                )
                if best is None or candidate.cost < best.cost:
                    best = candidate

    if best is None:
        raise RuntimeError("Wang search found no non-degenerate candidate")
    return best


def check_paper_benchmarks() -> None:
    """Detect formula/search transcription errors with published table rows."""

    bo = estimate_briaud_oeygarden(2**14, 3_482, 198)
    if (bo.fixed_blocks, bo.zeros_per_block, bo.witness_degree) != (197, 11, 2):
        raise AssertionError(f"B&O Table 2 parameter mismatch: {bo}")
    if round(bo.cost) != 106:
        raise AssertionError(f"B&O Table 2 cost mismatch: {bo.cost}")

    wang = estimate_wang(2**14, 3_482, 338)
    if (wang.f_bar, wang.u_bar, wang.coefficient_blocks) != (100, 6, 3):
        raise AssertionError(f"Wang Table 7 parameter mismatch: {wang}")
    if not math.isclose(wang.cost, 118.53, abs_tol=0.01):
        raise AssertionError(f"Wang Table 7 cost mismatch: {wang.cost}")

    print("Published-table self-checks:")
    print(
        "  B&O 2023 Table 2 (2^14,3482,198): "
        f"{bo.cost:.6f} bits, (f,u,d)=({bo.fixed_blocks},"
        f"{bo.zeros_per_block},{bo.witness_degree}); table rounds to 106 [PASS]"
    )
    print(
        "  Wang 2025 Table 7 (2^14,3482,338): "
        f"{wang.cost:.6f} bits, (f_bar,u_bar,g)=({wang.f_bar},"
        f"{wang.u_bar},{wang.coefficient_blocks}); table gives 118.53 [PASS]"
    )


def main() -> None:
    check_paper_benchmarks()
    print("\nExact SDitH v3 instances:")

    for instance in SDITH:
        bo = estimate_briaud_oeygarden(instance.n, instance.k, instance.weight)
        wang = estimate_wang(instance.n, instance.k, instance.weight)
        exact_ratio, asymptotic_ratio, q_ge_b_minus_1, q_ge_sqrt_b = (
            threshold_ratios(instance)
        )

        if bo.cost <= instance.target or wang.cost <= instance.target:
            raise AssertionError(f"{instance.name}: attack estimate reaches target")
        if exact_ratio >= 1.0:
            raise AssertionError(f"{instance.name}: conjectural degree-2 threshold met")
        if q_ge_b_minus_1 or q_ge_sqrt_b:
            raise AssertionError(f"{instance.name}: unexpected large-field regime")

        print(
            f"  {instance.name}: q=2 n={instance.n} k={instance.k} "
            f"w={instance.weight} b={instance.block_size} target={instance.target}"
        )
        print(
            f"    B&O: {bo.cost:.6f} bits; (f,u,d)=({bo.fixed_blocks},"
            f"{bo.zeros_per_block},{bo.witness_degree}); "
            f"-log2(P)={bo.probability_cost:.6f}; log2(M)={bo.matrix_log2:.6f}"
        )
        print(
            f"    2025 degree-2 ratio: {exact_ratio:.8f} "
            f"(short by {1.0 / exact_ratio:.2f}x); "
            f"asymptotic ratio={asymptotic_ratio:.8f}; "
            f"q>=b-1={q_ge_b_minus_1}; q>=sqrt(b)={q_ge_sqrt_b}"
        )
        print(
            f"    Wang: {wang.cost:.6f} bits; "
            f"(f_bar,u_bar,g)=({wang.f_bar},{wang.u_bar},"
            f"{wang.coefficient_blocks}); v={wang.remaining_variables}; "
            f"-log2(P)={wang.probability_cost:.6f}; "
            f"log2(iteration)={wang.iteration_cost:.6f}"
        )
        print("    target comparisons: B&O PASS; Wang PASS; threshold PASS")


if __name__ == "__main__":
    main()
