#!/usr/bin/env python3
"""Recover Sigurd's witness from a public zero-zeta signature transcript."""
import sys

lines = open(sys.argv[1], encoding="ascii").read().splitlines()
cols, raw_len, embedded, count = map(int, lines[0].split())
width = raw_len * 4
terms = []
for line in lines[1:1 + cols * 6]:
    terms.append([int(line[j * width:(j + 1) * width], 16) for j in range(6)])
gline = lines[1 + cols * 6]
response = [int(gline[j * width:(j + 1) * width], 16) for j in range(6)]
rows = []
for j in range(6):
    for bit in range(raw_len * 16):
        mask = sum(((terms[u][j] >> bit) & 1) << u for u in range(count))
        rows.append((mask, (response[j] >> bit) & 1))
for block in range(cols):
    rows.append((0b111111 << (6 * block), 1))
for line in lines[2 + cols * 6:2 + cols * 6 + embedded]:
    h, y = line.split()
    rows.append((int(h[::-1], 2), int(y)))

pivots = {}
inconsistent = False
for mask, rhs in rows:
    while mask:
        bit = mask.bit_length() - 1
        if bit in pivots:
            oldmask, oldrhs = pivots[bit]
            mask ^= oldmask
            rhs ^= oldrhs
        else:
            pivots[bit] = (mask, rhs)
            break
    if not mask and rhs:
        inconsistent = True
print(f"equations={len(rows)} unknowns={count} rank={len(pivots)} inconsistent={inconsistent}")
if inconsistent or len(pivots) != count:
    sys.exit(1)
solution = 0
for bit in sorted(pivots):
    mask, rhs = pivots[bit]
    if rhs ^ ((mask & solution).bit_count() & 1):
        solution |= 1 << bit
print("".join(str((solution >> bit) & 1) for bit in range(count)))
