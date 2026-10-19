#!/usr/bin/env python3
"""Reproduce QR-UOV-4's Hashimoto(k_H=0) -> PXL cost-model lead.

This is an integer/formula check.  It does not execute PXL or a full-size
forgery.  The affine Hilbert-series convention and gate conversion are those
used in QR-UOV Round 3 section 7.2.
"""

from math import comb, e, inf, log2


SETS = (
    # name, q, original n, m, Hashimoto b, TW residual, target
    ("1q127L10", 127, 600, 60, 10, 51, 143),
    ("5q127L10", 127, 1150, 110, 9, 101, 272),
    ("3q127L3", 127, 306, 78, 2, 76, 207),
    ("5q31L10", 31, 1240, 120, 9, 111, 272),
)


def hilbert_coeff(nvars: int, equations: int, degree: int) -> int:
    """[z^degree] (1-z^2)^equations / (1-z)^nvars."""
    return sum(
        (-1) ** j
        * comb(equations, j)
        * comb(degree - 2 * j + nvars - 1, nvars - 1)
        for j in range(min(equations, degree // 2) + 1)
    )


def pxl(q: int, n: int, equations: int, omega: float):
    """Return the best affine PXL gate estimate and its (k,D,memory)."""
    best = (inf, None, None, None)
    field_bits = log2(q)
    gate_factor = 2 * field_bits * field_bits + field_bits
    for k in range(1, n):
        # Specification equation (10) uses n-k+1 in the denominator.
        degree = next(
            d for d in range(400)
            if hilbert_coeff(n - k + 1, equations, d) <= 0
        )
        alpha = sum(
            max(hilbert_coeff(n - k, equations, d), 0)
            for d in range(degree + 1)
        )
        left = (
            k * k * alpha
            * comb(n - k + degree, degree)
            * comb(n + degree, degree)
        )
        right_log = k * field_bits + log2(
            alpha * alpha * comb(k + degree, degree) + alpha**omega
        )
        left_log = log2(left)
        operation_log = max(left_log, right_log) + log2(
            1 + 2 ** -abs(left_log - right_log)
        )
        gate_log = operation_log + log2(gate_factor)
        memory_elements_log = log2(comb(k + degree, degree)) + 2 * log2(
            comb(n - k + degree, degree)
        )
        if gate_log < best[0]:
            best = gate_log, k, degree, memory_elements_log
        if k * field_bits > best[0] + 5:
            break
    return best


def hashimoto_checks(n: int, m: int, b: int):
    first_left = n - (m - b - 1) * (b + 2)
    first_right = b + 1
    second_left = n - m + 1
    second_right = b * (m - b)
    assert first_left > first_right
    assert second_left > second_right
    return first_left, first_right, second_left, second_right


def logsum(*values: float) -> float:
    top = max(values)
    return top + log2(sum(2 ** (v - top) for v in values))


def main() -> int:
    expected = {
        "1q127L10": (49, 141.9491, 148.2413, 110.272),
        "5q127L10": (100, 270.0483, 288.3258, 211.150),
        "3q127L3": (75, 208.7364, 220.1555, 165.583),
        "5q31L10": (110, 273.9012, 290.3125, 221.786),
    }
    # Random square MQ systems are often modeled with Poisson(1) solution
    # count. This is a sensitivity assumption for conditioned residuals, not
    # an observed success probability at the submitted dimensions.
    assumed_success = 1 - 1 / e
    restart_bits = -log2(assumed_success)
    assert abs(restart_bits - 0.6617) < 0.001
    print("set       residual  spec-w2.37  margin  w2.81  log2(elements)  log2(bits)")
    for name, q, n, m, b, tw_residual, target in SETS:
        checks = hashimoto_checks(n, m, b)
        residual = m - b - 1
        assert residual < tw_residual
        low = pxl(q, residual, residual, 2.37)
        finite = pxl(q, residual, residual, 2.81)
        bit_memory = low[3] + log2(log2(q))
        exp_residual, exp_low, exp_finite, exp_mem = expected[name]
        assert residual == exp_residual
        assert abs(low[0] - exp_low) < 0.001
        assert abs(finite[0] - exp_finite) < 0.001
        assert abs(low[3] - exp_mem) < 0.001

        # Bound the omitted Hashimoto work in the same model.  Its
        # centrifugation performs (m-b) solves of MQ(b+1,b+1), followed by
        # one MQ(b,b) square step.  Both are negligible here.
        centrifuge = pxl(q, b + 1, b + 1, 2.37)[0] + log2(m - b)
        square = pxl(q, b, b, 2.37)[0]
        total = logsum(low[0], centrifuge, square)
        assert total - low[0] < 0.001

        # A logarithmic memory-access charge, as used in the sensitivity
        # analysis, multiplies the dominant arithmetic by log2(storage bits).
        charged = low[0] + log2(bit_memory)
        print(
            f"{name:9s} {residual:8d}  {low[0]:10.4f}  "
            f"{low[0]-target:+6.4f}  {finite[0]:6.4f}  "
            f"{low[3]:14.3f}  {bit_memory:10.3f}"
        )
        print(
            f"  eq7={checks}; PXL(k,D)=({low[1]},{low[2]}); "
            f"full={total:.4f}; log-memory={charged:.4f}; "
            f"heuristic-restarts={low[0]+restart_bits:.4f}; "
            f"min-success-to-beat-target={2**(low[0]-target):.4f}"
        )

    print("PASS: QR-UOV-4 formula and sensitivity checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
