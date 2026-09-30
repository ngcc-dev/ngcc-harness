#!/usr/bin/env python3
"""Dependency-free UVW DFR calculation and failure-reaction simulation.

The exact probability calculation follows the submitted decoder's hidden-pair
model.  The simulation conditions on final failures and recovers the secret
pair/ratio triples from their excess co-occurrence, without timing data.
"""

from collections import defaultdict
from itertools import combinations
from math import exp, lgamma, log, log1p
from pathlib import Path
import random
import re
import sys


SETS = (
    (128, 433, 860, 215, 116, -43.317),
    (256, 857, 1708, 427, 232, -42.144),
    (512, 1709, 3412, 853, 463, -41.729),
)


def log_c(n: int, k: int) -> float:
    if k < 0 or k > n:
        return float("-inf")
    return lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)


def probabilities(q: int, n: int, k1: int, w: int, attempts: int = 1000):
    half = n // 2
    log_total = log_c(n, w) + w * log(q - 1)
    single = 0.0
    failure = 0.0
    for t in range(w // 2 + 1):
        for i in range(w - 2 * t + 1):
            if (w - 2 * t - i) & 1:
                continue
            j = (w - 2 * t - i) // 2
            if t + i + j > half:
                continue
            log_count = (
                log_c(half, t)
                + log_c(half - t, i)
                + log_c(half - t - i, j)
                + t * log(q - 1)
                + i * log(2 * (q - 1))
                + j * log((q - 1) * (q - 2))
            )
            p = exp(log_count - log_total)
            clean = half - i - j
            if clean < k1 or clean - t < k1:
                success = 0.0
            else:
                success = exp(log_c(clean - t, k1) - log_c(clean, k1))
            single += p * success
            if success == 0.0:
                failure += p
            elif success < 1.0:
                failure += p * exp(attempts * log1p(-success))
    return single, failure


def build_world(seed: int = 1):
    q, n = 433, 860
    rng = random.Random(seed)
    positions = list(range(n))
    rng.shuffle(positions)
    a_side, b_side = positions[: n // 2], positions[n // 2 :]
    gamma = [rng.randrange(1, q) for _ in a_side]
    inverse = [0] + [pow(x, q - 2, q) for x in range(1, q)]
    truth = set()
    for a, b, g in zip(a_side, b_side, gamma):
        if a < b:
            ratio = g
        else:
            a, b = b, a
            ratio = inverse[g]
        truth.add((a * n + b) * q + ratio)
    return q, n, a_side, b_side, gamma, inverse, truth


def failure_samples(count: int, world, seed: int = 2):
    q, n, a_side, b_side, gamma, _, _ = world
    rng = random.Random(seed)
    # Conditional weights from the exact UVW-128 final-failure calculation.
    t_values = (4, 5, 6, 7, 8)
    raw = [2.0**x for x in (-47.84, -43.46, -47.58, -54.86, -63.42)]
    total = sum(raw)
    cumulative = []
    acc = 0.0
    for value in raw:
        acc += value / total
        cumulative.append(acc)

    all_positions = list(range(n))
    for _ in range(count):
        draw = rng.random()
        t = next(tv for tv, edge in zip(t_values, cumulative) if draw <= edge)
        planted = rng.sample(range(n // 2), t)
        used = set()
        values = {}
        for p in planted:
            b = b_side[p]
            a = a_side[p]
            value = rng.randrange(1, q)
            values[b] = value
            values[a] = gamma[p] * value % q
            used.add(a)
            used.add(b)
        rest = rng.sample([x for x in all_positions if x not in used], 116 - 2 * t)
        for pos in rest:
            values[pos] = rng.randrange(1, q)
        yield sorted(values.items())


def encoded_keys(sample, q: int, n: int, inverse):
    for (a, va), (b, vb) in combinations(sample, 2):
        yield (a * n + b) * q + (va * inverse[vb]) % q


def recover_pairs(failures: int = 1000):
    world = build_world()
    q, n, _, _, _, inverse, truth = world
    # A pair of compact count-min sketches selects repeated triples.  A second
    # deterministic pass counts just those candidates exactly; this avoids the
    # several-hundred-megabyte NumPy vector used by the original witness.
    bits = 24
    mask = (1 << bits) - 1
    sketch1 = bytearray(1 << bits)
    sketch2 = bytearray(1 << bits)

    def slots(key):
        return ((key * 0x9E3779B185EBCA87) & mask,
                ((key ^ (key >> 17)) * 0xC2B2AE3D27D4EB4F) & mask)

    for sample in failure_samples(failures, world):
        for key in encoded_keys(sample, q, n, inverse):
            one, two = slots(key)
            if sketch1[one] < 255:
                sketch1[one] += 1
            if sketch2[two] < 255:
                sketch2[two] += 1

    counts = defaultdict(int)
    for sample in failure_samples(failures, world):
        for key in encoded_keys(sample, q, n, inverse):
            one, two = slots(key)
            if min(sketch1[one], sketch2[two]) >= 5:
                counts[key] += 1

    used = set()
    recovered = []
    for key, count in sorted(counts.items(), key=lambda item: (-item[1], item[0])):
        pair = key // q
        a, b = divmod(pair, n)
        if a in used or b in used:
            continue
        used.add(a)
        used.add(b)
        recovered.append(key)
        if len(recovered) == n // 2:
            break
    correct = sum(key in truth for key in recovered)
    return correct, n // 2


def check_leaks(root: Path):
    rows = []
    for level in (256, 512):
        base = root / f"Implementations/Reference_Implementation/UVW-KEM-{level}"
        params = (base / "include/params.h").read_text(errors="replace")
        source = (base / "src/KEM_AlgorithmInstance.c").read_text(errors="replace")
        k1 = int(re.search(r"#define\s+UVW_K1\s+([0-9]+)", params).group(1))
        allocation = source.index("gf_elem_t *GU_I1")
        loop_start = source.rfind("for (int attempt = 0;", 0, allocation)
        loop = source[loop_start:source.index("free(temp_sk);", allocation)]
        allocates = loop.count("malloc((size_t)k1 * k1 * sizeof(gf_elem_t))") == 2
        # The only frees are inside the allocation-failure branch.  Neither the
        # normal success nor retry path releases the two matrices.
        failure_branch = loop[loop.index("if (!GU_I1"):loop.index("for (int row")]
        normal_free = "free(GU_I1);" in loop.replace(failure_branch, "") or "free(GU_I1_inv);" in loop.replace(failure_branch, "")
        per_attempt = 2 * k1 * k1 * 2
        invalid_free = level == 256 and "free(I1);" in failure_branch
        rows.append((level, k1, per_attempt, allocates and not normal_free, invalid_free))
    return rows


def main() -> int:
    failed = False
    for level, q, n, k1, w, expected_log in SETS:
        single, dfr = probabilities(q, n, k1, w)
        actual_log = log(dfr, 2)
        ok = abs(actual_log - expected_log) < 0.002
        failed |= not ok
        print(
            f"ATTACK uvw-overall-dfr UVW-{level} "
            f"{'CONFIRMED' if ok else 'NOT-CONFIRMED'} "
            f"single_success={single:.8f} DFR_1000=2^{actual_log:.3f}"
        )

    correct, total = recover_pairs()
    recovered = correct == total
    failed |= not recovered
    print(
        "ATTACK uvw-failure-reaction UVW-128 "
        f"{'CONFIRMED' if recovered else 'NOT-CONFIRMED'} "
        f"failure-only hidden pair/ratio recovery={correct}/{total} from 1000 final failures"
    )

    leak_rows = check_leaks(Path(__file__).resolve().parent)
    leak_ok = all(ok for _, _, _, ok, _ in leak_rows)
    invalid_free_ok = any(level == 256 and bad_free for level, _, _, _, bad_free in leak_rows)
    failed |= not invalid_free_ok
    failed |= not leak_ok
    details = ", ".join(
        f"UVW-{level}={1000 * size / 1_000_000_000:.2f}GB/1000 attempts"
        for level, _, size, _, _ in leak_rows
    )
    print(
        "ATTACK uvw-retry-leak UVW-256/512 "
        f"{'CONFIRMED' if leak_ok else 'NOT-CONFIRMED'} {details}"
    )
    print(
        "ATTACK uvw-invalid-free UVW-256 "
        f"{'CONFIRMED' if invalid_free_ok else 'NOT-CONFIRMED'} allocation-failure branch frees stack I1"
    )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
