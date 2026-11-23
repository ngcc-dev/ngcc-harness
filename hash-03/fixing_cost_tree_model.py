#!/usr/bin/env python3
"""Costing aid for hash-03/fixing_cost.md: backward-tree model of hash-03-1.

Evaluates the finding's cost model when z bits of every 736-bit C-Hash-1024
message block are reserved as zero.  It prints exponents and call ratios
only; it implements no part of C-Hash and no repaired mode.

Model (report.md, hash-03-1): a backward step is a c-bit condition, with
c = 736 + z.  With N probes at each of d counters and N forward states, a
full 1,472-bit join is expected when N^(d+1) = 2^(d*c + 1472).  Calls are
counted as the finding counts them: d*N backward probes and f*N forward
calls, where f is the number of prefix blocks needed for N distinct states.
"""

from math import ceil, log2

STATE = 1472
BLOCK = 736
TARGET = 1024


def log2_n(z: int, d: int) -> float:
    c = BLOCK + z
    return c + (STATE - c) / (d + 1)


def log2_calls(z: int, d: int) -> float:
    e = log2_n(z, d)
    f = ceil(e / (BLOCK - z))
    return e + log2(d + f)


def cheapest(z: int) -> tuple[float, int]:
    return min((log2_calls(z, d), d) for d in range(1, 4096))


def main() -> None:
    print(f"control z=0 d=2: log2 N = {log2_n(0, 2):.2f}, "
          f"log2 calls = {log2_calls(0, 2):.2f}")
    print("z    c     d=2      d=3      limit  min log2 calls (d)   calls/byte")
    for z in (0, 64, 272, 280, 288, 352):
        calls, d = cheapest(z)
        print(f"{z:<4d} {BLOCK + z:<5d} {log2_n(z, 2):<8.2f} {log2_n(z, 3):<8.2f} "
              f"{BLOCK + z:<6d} {calls:8.2f} ({d:4d})      "
              f"{BLOCK / (BLOCK - z):.4f}")
    two = min(z for z in range(0, BLOCK, 8) if log2_n(z, 2) >= TARGET)
    lim = min(z for z in range(0, BLOCK, 8) if BLOCK + z >= TARGET)
    cnt = min(z for z in range(0, BLOCK, 8) if cheapest(z)[0] >= TARGET)
    print(f"smallest byte-aligned z, two levels only (log2 N): {two}")
    print(f"smallest byte-aligned z, every depth, exponent only: {lim}")
    print(f"smallest byte-aligned z, every depth, calls counted: {cnt}")
    for z in (cnt, lim, 352):
        print(f"z={z}: {BLOCK - z} message bits per call, "
              f"calls per byte x{BLOCK / (BLOCK - z):.4f}, "
              f"throughput x{(BLOCK - z) / BLOCK:.4f}, "
              f"2^56 blocks = 2^{56 + log2(BLOCK - z):.2f} bits")


if __name__ == "__main__":
    main()
