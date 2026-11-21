#!/usr/bin/env sage
"""Small GF(16) unbalanced char-2 Schur-Macaulay toy checks for MAYO-6."""

from itertools import combinations
from math import comb

set_random_seed(20261121)
F = GF(16, "a")


def fo(n, p, d):
    if d < 0:
        return 0
    return sum(((-1) ** i) * comb(n, i) * comb(n + d - p * i - 1, d - p * i) for i in range(d // p + 1))


def go(n, p, d):
    if d < 0:
        return 0
    return sum(((-1) ** (d - p * i)) * comb(n, d - p * i) * comb(n + i - 1, i) for i in range(d // p + 1))


def uo(n1, n2, p, d1, d2):
    if d1 < 0 or d2 < 0:
        return 0
    return fo(n1, p, d1 - 1) * fo(n2, p, d2) - fo(n1, p, d1) * fo(n2, p, d2 - 1)


def sto(n1, n2, m, p, r, d1, d2):
    pr1 = p ** (r + 1)
    total = 0
    for i in range((d1 + 1) // 2 + 1):
        gi = go(m, pr1, i)
        if gi == 0:
            continue
        for j in range(d2 // 2 + 1):
            gj = go(m, pr1, j)
            if gj == 0:
                continue
            for k in range(min(d1 + 1 - 2 * i, d2 - 2 * j) + 1):
                gk = go(m, pr1, k)
                if gk == 0:
                    continue
                total += gi * gj * gk * uo(n1, n2, p, d1 + 1 - 2 * i - k, d2 - 2 * j - k)
    return total


def run_toy(n, o, m, k2):
    v = n - o
    while True:
        T = Matrix(F, n, n, [F.random_element() for _ in range(n * n)])
        if T.is_invertible():
            break
    Ps = []
    for _ in range(m):
        U = Matrix(F, n, n)
        for i in range(v):
            for j in range(i + 1, n):
                U[i, j] = F.random_element()
        M = T.transpose() * U * T
        P = {(i, j): M[i, j] + M[j, i] for i in range(n) for j in range(i + 1, n) if M[i, j] + M[j, i] != 0}
        Ps.append(P)

    n1, n2 = n, n - k2
    d1, d2 = o, o - k2
    mon1 = list(combinations(range(n1), d1))
    mon2 = list(combinations(range(n2), d2))
    idx1 = {m1: i for i, m1 in enumerate(mon1)}
    idx2 = {m2: j for j, m2 in enumerate(mon2)}
    N2 = len(mon2)
    N = len(mon1) * N2
    rows = []

    for P in Ps:
        for u in combinations(range(n1), d1 - 2):
            u_set = set(u)
            xterms = [(idx1[tuple(sorted(u + (i, j)))], c) for (i, j), c in P.items() if i not in u_set and j not in u_set]
            if not xterms:
                continue
            for w_idx in range(N2):
                rows.append([(xi * N2 + w_idx, c) for xi, c in xterms])

    if d1 >= 4:
        for P in Ps:
            items = list(P.items())
            gam2 = {}
            for a_i in range(len(items)):
                (i1, j1), c1 = items[a_i]
                for b_i in range(a_i + 1, len(items)):
                    (i2, j2), c2 = items[b_i]
                    if i1 not in (i2, j2) and j1 not in (i2, j2):
                        key = tuple(sorted((i1, j1, i2, j2)))
                        gam2[key] = gam2.get(key, F(0)) + c1 * c2
            gam2 = {k: val for k, val in gam2.items() if val != 0}
            for u in combinations(range(n1), d1 - 4):
                u_set = set(u)
                xterms = [(idx1[tuple(sorted(u + quad))], c) for quad, c in gam2.items() if not (u_set & set(quad))]
                if not xterms:
                    continue
                for w_idx in range(N2):
                    rows.append([(xi * N2 + w_idx, c) for xi, c in xterms])

    e = 0
    while (1 << e) <= d2:
        step = 1 << e
        if d1 + step <= n1:
            for S in combinations(range(n1), d1 + step):
                S_proj = [x for x in S if x < n2]
                if len(S_proj) < step:
                    continue
                for T_mon in combinations(range(n2), d2 - step):
                    T_set = set(T_mon)
                    avail = [x for x in S_proj if x not in T_set]
                    if len(avail) < step:
                        continue
                    row = []
                    for moved in combinations(avail, step):
                        moved_set = set(moved)
                        newS = tuple(x for x in S if x not in moved_set)
                        newT = tuple(sorted(T_mon + moved))
                        row.append((idx1[newS] * N2 + idx2[newT], F(1)))
                    if row:
                        rows.append(row)
        e += 1

    M = Matrix(F, len(rows), N, sparse=False)
    for r_i, row in enumerate(rows):
        for c_i, val in row:
            M[r_i, c_i] += val
    nullity = N - M.rank()
    pred = sto(n1, n2, m, 2, 1, d1, d2)
    print(f"toy (n={n}, o={o}, m={m}, k2={k2}, d1={d1}, d2={d2}): N={N}, rows={len(rows)}, nullity={nullity}, Sto={pred}")
    return nullity, pred


cases = [
    (7, 4, 4, 1),
    (7, 4, 5, 2),
    (8, 4, 2, 2),
    (8, 4, 3, 2),
    (8, 4, 4, 2),
    (8, 4, 5, 2),
    (8, 4, 6, 2),
    (9, 4, 5, 2),
    (9, 4, 6, 2),
    (9, 4, 7, 2),
    (9, 4, 5, 3),
    (9, 4, 6, 3),
]
ok = True
sto_le_zero_count = 0
for n, o, m, k2 in cases:
    nullity, pred = run_toy(n, o, m, k2)
    if pred <= 0:
        sto_le_zero_count += 1
        ok &= (nullity == 1)
    if (n, o, m, k2) == (8, 4, 2, 2):
        ok &= (pred == 59 and nullity > pred)
assert sto_le_zero_count == 10 and ok, f"unexpected toy check result (sto_le_zero={sto_le_zero_count}, ok={ok})"
print("PASS unbalanced_schur_toy")
