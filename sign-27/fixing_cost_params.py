#!/usr/bin/env python3
"""Fixing-cost aid for sign-27 (SQIsignTriangle).

Part A: parameter estimate for sign-27-3 (and the prime size relevant to sign-27-2).
        The response degree follows log2 q = (1/2) log2 p + (3/2) log2 c1, because
        EquivalentSpecialIdeal (spec Algorithm 3.13, physical p. 34) searches a coset of the
        sublattice {v = lambda*w mod c1} of index c1^3 in a rank-4 lattice. The verifier needs
        e_rsp = bitsize(q) <= f (verify.c:22-24). Part A checks this law against the submitted
        widths and evaluates the size formulas at a lambda-bit challenge modulus.
Part B: work added to Verify by the degree check that the submitters proposed for sign-27-5
        (two Weil pairings and three more points pushed through the (2,2)-chain), relative
        to a count of the submitted verifier.

Nothing is implemented or measured. Unit in Part B: F_p multiplications and squarings;
fp2_mul = 3, fp2_sqr = 2. File:line references are to
Implementations/Reference_Implementation/SQIsignTriangle_lvl1/sqisigntriangle/src.

Run:  python3 -B sign-27/fixing_cost_params.py
"""
import math

# lambda: c, f (spec Table 8, physical p. 57); public key, signature, coefficient bytes
# (spec Table 9, physical p. 58); e_rsp observed in report.md (sign-27-6);
# Verify, Sign and KeyGen averages, 10^6 cycles (spec Table 10, physical p. 59, optimized build).
SETS = {
    128: dict(c=5, f=248, pk=65, sig=204, coeff=28, e_obs=211, verify=4.029, sign=37.276, keygen=13.903),
    160: dict(c=9, f=309, pk=81, sig=255, coeff=35, e_obs=273, verify=7.568, sign=63.981, keygen=21.813),
    256: dict(c=27, f=500, pk=129, sig=408, coeff=56, e_obs=445, verify=27.725, sign=201.325, keygen=108.494),
    512: dict(c=113, f=1016, pk=257, sig=816, coeff=112, e_obs=894, verify=258.839, sign=2698.805, keygen=783.312),
}


def sizes(p_bits, e_bits):
    fp = math.ceil(p_bits / 8)
    co = math.ceil(e_bits / 8)
    return 2 * fp + 1, 2 * fp + 5 * co          # public key, signature (spec section 7.3)


def part_a():
    print("Part A: sign-27-3, challenge modulus of lambda bits")
    print(" control: submitted sizes from 2*Fp+1 and 2*Fp+4*coeff+q, and the degree law")
    for lam, s in SETS.items():
        p_bits = math.log2(s["c"]) + s["f"]
        pk, sig = sizes(math.ceil(p_bits), 8 * s["coeff"])
        law = 0.5 * p_bits + 1.5 * (lam / 2)
        print(f"  lambda={lam}: log2 p={p_bits:.1f} pk={pk} (table {s['pk']}) sig={sig} (table {s['sig']});"
              f" predicted log2 q={law:.1f}, q field {8 * s['coeff']} bits, observed e_rsp {s['e_obs']}, f={s['f']}")
        assert (pk, sig) == (s["pk"], s["sig"])
    print(" with c1 of lambda bits and the submitted primes: predicted log2 q against f")
    for lam, s in SETS.items():
        p_bits = math.log2(s["c"]) + s["f"]
        print(f"  lambda={lam}: log2 q={0.5 * p_bits + 1.5 * lam:.1f} > f={s['f']}")
    print(" zero-margin estimate: log2 p = 3*lambda, e_rsp = 3*lambda (cofactor c ignored)")
    for lam, s in SETS.items():
        p_old = math.log2(s["c"]) + s["f"]
        p_new = 3 * lam
        e_new = 0.5 * p_new + 1.5 * lam
        pk, sig = sizes(p_new, e_new)
        chain = e_new / (8 * s["coeff"])            # (2,2)-chain length against the q field width
        mul = (p_new / p_old) ** 2                  # schoolbook field multiplication
        print(f"  lambda={lam}: p {p_old:.0f} -> {p_new} bits; pk {s['pk']} -> {pk} (+{100 * (pk / s['pk'] - 1):.1f}%);"
              f" sig {s['sig']} -> {sig} (+{100 * (sig / s['sig'] - 1):.1f}%);"
              f" chain x{chain:.2f}, torsion x{p_new / p_old:.2f}, field mul x{mul:.2f};"
              f" verify x{p_new / p_old * mul:.1f} to x{chain * mul:.1f}")
    print(" calibration of the work law e_rsp*(log2 p)^2 on spec Table 10 (Verify, optimized build)")
    keys = list(SETS)
    exps = {"sign": [], "keygen": []}
    for a, b in zip(keys, keys[1:]):
        pa = math.log2(SETS[a]["c"]) + SETS[a]["f"]
        pb = math.log2(SETS[b]["c"]) + SETS[b]["f"]
        model = (SETS[b]["coeff"] / SETS[a]["coeff"]) * (pb / pa) ** 2
        line = f"  {a}->{b}: law x{model:.2f}, Verify table x{SETS[b]['verify'] / SETS[a]['verify']:.2f}"
        for op in ("sign", "keygen"):
            r = SETS[b][op] / SETS[a][op]
            k = math.log(r) / math.log(pb / pa)
            exps[op].append(k)
            line += f"; {op} table x{r:.2f} = (log2 p ratio)^{k:.2f}"
        print(line)
    p_old = math.log2(5) + 248
    for op in ("sign", "keygen"):
        lo, hi = min(exps[op]), max(exps[op])
        print(f"  {op} at lambda=128, 384-bit prime, exponents {lo:.2f} to {hi:.2f}:"
              f" x{(384 / p_old) ** lo:.1f} to x{(384 / p_old) ** hi:.1f}")
    print(" sign-27-2 (Lead), lambda=128 only: figures quoted in report.md")
    lo, hi, target = 138.78, 143.51, 143.0
    for name, cost in (("interval low end", lo), ("midpoint", (lo + hi) / 2)):
        rel = (target - cost) / target
        bits = p_old * (1 + rel)
        pk, sig = sizes(math.ceil(bits), 0.5 * bits + 96)
        print(f"  {name}: deficit {target - cost:.2f} bits, relative {100 * rel:.1f}%;"
              f" log2 p {p_old:.1f} -> {bits:.1f}; pk {pk} (+{100 * (pk / 65 - 1):.1f}%),"
              f" sig {sig} (+{100 * (sig / 204 - 1):.1f}%)")
    for bits in (326, 384):
        pk, sig = sizes(bits, 0.5 * bits + 96)
        print(f"  a {bits}-bit prime at the submitted challenge width: pk {pk} (+{100 * (pk / 65 - 1):.0f}%),"
              f" sig {sig} (+{100 * (sig / 204 - 1):.0f}%)")


# ---- Part B ---------------------------------------------------------------------------
M, S = 3, 2
XDBL = 4 * M + 2 * S              # ec/ref/lvlx/ec.c:257
XADD = 4 * M + 2 * S              # ec.c:278
XDBLADD = 8 * M + 4 * S           # ec.c:301
DBLW = 3 * M + 5 * S              # ec/ref/lvlx/ec_jac.c:150
JDBL = 6 * M + 6 * S              # ec_jac.c:110
TH_DBL = 8 * M + 8 * S            # hd/ref/lvlx/theta_structure.c:34
TH_PRE = 12 * M + 4 * S           # theta_structure.c:5
TH_EVAL = 4 * M + 4 * S           # hd/ref/lvlx/theta_isogenies.c:855
TH_COMPUTE = 17 * M + 8 * S       # theta_isogenies.c:618 with the verify branch
GLUE_EVAL_PT = 2 * (11 * M + 5 * S) + 12 * M + 2 * 16 * M + 8 * S    # theta_isogenies.c:501
GLUE_SPECIAL = (4 + 16) * M + 4 * S + 3 * M                          # theta_isogenies.c:555
CUB_DBLADD = 6 * M + 4 * S        # ec/ref/lvlx/biextension.c:43


def chain(n):
    """Loop counters of _theta_chain_compute_impl without extra torsion
    (hd/ref/lvlx/theta_isogenies.c:1058-1244)."""
    todo = [n - 2] + [0] * 64
    cur = 0
    jd = 0
    while todo[cur] != 1:
        cur += 1
        prev = todo[cur - 1]
        nd = prev // 2 if prev >= 16 else prev - 1
        jd += 2 * 2 * nd                       # two kernel points on two curves
        todo[cur] = prev - nd
    glue = cur
    for i in range(cur):
        todo[i] -= 1
    cur -= 1
    done = True
    pre, dbl, ev, steps = 1, 0, 0, 0
    while cur >= 0 and todo[cur]:
        while todo[cur] != 1:
            cur += 1
            nd = todo[cur - 1] // 2
            if not done:
                pre += 1
                done = True
            dbl += 2 * nd
            todo[cur] = todo[cur - 1] - nd
        steps += 1
        done = False
        ev += 2 * cur
        for j in range(cur):
            todo[j] -= 1
        cur -= 1
    ev += 4
    return jd, glue, dbl, pre, steps, ev


def part_b():
    print()
    print("Part B: sign-27-5, proposed degree check, work in F_p multiplications")
    print(" (hash calls and the Miller-Rabin tests of the challenge are not counted)")
    print(f"  {'lambda':>6}{'e_rsp':>7}{'verify':>9}{'chain %':>9}{'added':>8}{'added %':>9}")
    for lam, s in SETS.items():
        p_bits = math.log2(s["c"]) + s["f"]
        e = s["e_obs"]
        f = s["f"]
        cbits = s["c"].bit_length()
        exp = 1.1 * p_bits                      # one F_p inversion, square root or Legendre symbol
        inv2, sqrt2, issq2 = exp + 4, 3 * exp, exp + 2
        diff = 17 * M + 6 * S + sqrt2           # ec/ref/lvlx/basis.c:22
        # decoding, verification/ref/lvlx/encode_verification.c:202-241
        basis_aux = 3 * issq2 + inv2 + 2 * cbits * XDBLADD + diff          # basis.c:292
        biscal = 3 * (e * (XDBL + 2 * XADD) + XADD)                        # ec.c:384, 624
        scale = 2 * 3 * (f - e) * XDBL                                     # ec.c:604, both bases
        # verification, verification/ref/lvlx/verify.c:5-86
        basis_pk = inv2 + 2 * cbits * XDBLADD + diff                       # basis.c:340
        mul_q = 3 * e * XDBLADD                                            # ec.c:339, 612
        order = 2 * 3 * e * XDBL                                           # ec.h:570, 599
        jd, glue, dbl, pre, steps, ev = chain(e)
        lift = 2 * (3 * M + inv2 + 15 * M + 2 * S + 3 * M + S + sqrt2)     # basis.c:79, 121
        gluing = 4 * JDBL + 8 * S + 60 * M + inv2 + 2 * 20 * M + 8 * S + 17 * M   # theta_isogenies.c:271, 396
        last = (17 * M + 8 * S + 2 * sqrt2) + (10 * M + 4 * S + 3 * sqrt2)        # :714, :803
        dim2 = lift + jd * DBLW + gluing + glue * 2 * GLUE_EVAL_PT + pre * TH_PRE + dbl * TH_DBL \
            + steps * TH_COMPUTE + ev * TH_EVAL + last + 40 * M
        jinv = 3 * M + 4 * S + inv2                                        # ec.c:193
        verify = basis_aux + biscal + scale + basis_pk + mul_q + order + dim2 + 3 * jinv
        # proposed check: Weil pairings of the basis and of its image (biextension.c:221),
        # three x-only points through the chain (theta_isogenies.c:1058), one power by q
        weil = 2 * (e - 1) * CUB_DBLADD + (9 * M + inv2) + inv2 + 20 * M
        push = GLUE_SPECIAL + (e - 1) * TH_EVAL + 16 * M + 6 * M
        added = 2 * weil + 3 * push + e * (S + M / 2 * 1.0)
        print(f"  {lam:>6}{e:>7}{verify:>9.0f}{100 * dim2 / verify:>9.1f}{added:>8.0f}{100 * added / verify:>9.1f}")


if __name__ == "__main__":
    part_a()
    part_b()
