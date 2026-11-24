#!/usr/bin/env python3
"""Fixing-cost aid for sign-25 (SQIsign2D2): field-operation counts of verification.

Counts the work of the submitted verifiers (protocols_verif_internal, sqisign/sign.c:1016-1266,
with file:line references to Implementations/Reference_Implementation/SQISign2Dsquare-Level1-sec;
the cited files are the same in all eight sets up to line endings, except fp.c in Level2-eff)
and of the checks and steps that the costed repairs add, in the verifier and in the compact
signer. Signing as a whole is not counted. Nothing is implemented or measured: the script only
replays the loop counters of the isogeny strategies and of the discrete-logarithm recursion and
sums the per-routine operation counts read from the reference source.

Unit: one F_p multiplication or squaring (fp_mul / fp_sqr counted alike).
fp2_mul = 3 units, fp2_sqr = 2 units (fp2.c:118-143).

Run:  python3 -B sign-25/fixing_cost_verify_work.py
"""

M, S = 3, 2  # fp2_mul, fp2_sqr in F_p multiplications (fp2.c:118-143)

# name, f, a, b (spec section 4.2, physical pp. 58-59);
# NWORDS_FIELD, BITS_2, BITS_3 (ec_arithmetic/include/fp_constants.h);
# fp_half uses an inversion (fp.c:109-112) in every set but Level2-eff;
# spec Table 6.2 (physical p. 62) reference, 10^6 cycles: Verify uncompressed, Verify
# compressed, Sign compressed (only the two Verify values are used, in control 2).
SETS = [
    ("Level1-eff", 1, 131, 78, 4, 192, 128, True, 14.77, 15.42, 73.64),
    ("Level1-sec", 1, 137, 84, 5, 192, 192, True, 20.76, 23.42, 113.17),
    ("Level2-eff", 61, 161, 96, 5, 192, 192, False, 25.70, 29.04, 134.49),
    ("Level2-sec", 11, 168, 101, 6, 192, 192, True, 36.92, 42.69, 196.17),
    ("Level3-eff", 1, 263, 156, 8, 320, 256, True, 103.44, 106.86, 457.45),
    ("Level3-sec", 1, 264, 163, 9, 320, 320, True, 121.71, 136.72, 636.89),
    ("Level5-eff", 11, 514, 319, 16, 576, 512, True, 1036.21, 1180.68, 5285.58),
    ("Level5-sec", 17, 536, 334, 17, 576, 576, True, 1285.36, 1459.48, 6447.71),
]

# x-only and Jacobian routines (source file:line of the routine)
XDBLADD = 8 * M + 4 * S          # ec.c:182
XDBL = 4 * M + 2 * S             # ec.c:104, 126
XDBL_N = 3 * M + 2 * S           # ec.c:144
XADD = 4 * M + 2 * S             # ec.c:162
XTPL = 7 * M + 5 * S             # basis.c:8
XISOG3 = 2 * M + 3 * S           # xisog.c:96
XEVAL3 = 4 * M + 2 * S           # xeval.c:104, per point
JDBL = 6 * M + 6 * S             # ec.c:906
JADD = 26 * M + 9 * S            # ec.c:945 with two is_jac_equal (ec.c:846)
# theta routines
TH_DBL = 8 * M + 8 * S           # theta_structure.c:77
TH_PRECOMP = 12 * M + 4 * S      # theta_structure.c:15
TH_EVAL = 4 * M + 4 * S          # theta_isogenies.c:1033
TH_COMPUT_PRE = 11 * M + 8 * S   # theta_isogenies.c:719, branch with precomputation
TH_COMPUT_NOPRE = 14 * M + 8 * S # theta_isogenies.c:719, other branch
BASE_CHANGE = 20 * M             # theta_isogenies.c:110
GLUE_EVAL_PT = 2 * BASE_CHANGE + 8 * S + 7 * M          # theta_isogenies.c:472
GLUE_EVAL_BASIS = 4 * JADD + 8 * S + 2 * GLUE_EVAL_PT   # theta_isogenies.c:670
GLUE_EVAL_SPECIAL = BASE_CHANGE + 4 * S + 2 * M          # theta_isogenies.c:537
CHECK_ISOTROPIC = 8 * M          # spec Algorithm 7.37, "at most 8M" (physical p. 92)
CUBICAL = 3 * M + 2 * S          # cubicalDBL and cubicalADD, biextension.c:41, 61


def field_costs(f, a, b, nwords, half_inv):
    p = f * 2**a * 3**b - 1
    exp = (64 * nwords - 2) + bin(p >> 2).count("1")   # fp_exp3div4, fp.c:186-208
    fp_inv = exp + 3                                     # fp.c:211
    fp_issq = exp + 2                                    # fp.c:224
    fp_sqrt = exp + 2                                    # fp.c:238 (mul + frommont)
    c = {}
    c["inv2"] = 2 + fp_inv + 2                           # fp2.c:146
    c["issq2"] = 2 + fp_issq                             # fp2.c:191
    c["sqrt2"] = 2 + 2 * fp_sqrt + fp_issq + fp_inv + 3 + (fp_inv + 1 if half_inv else 0)  # fp2.c:203
    return p, c


def three_chain(b, points):
    """Loop counters of ec_eval_three_balanced_strategy (isog_chains.c:402-470)."""
    todo = [b] + [0] * 64
    cur = tpl = ev = 0
    for _ in range(b - 1):
        while todo[cur] != 1:
            cur += 1
            n = todo[cur - 1] // 2
            todo[cur] = todo[cur - 1] - n
            tpl += n
        ev += cur + points
        for i in range(cur):
            todo[i] -= 1
        cur -= 1
    ev += points
    return tpl, ev


def dlog3_rec(length, stack):
    """Field work of fp2_dlog_3e_rec (biextension.c:691-775); a leaf costs one squaring and,
    per stacked pair, a squaring and one or two multiplications (5/3 on average)."""
    if length == 0:
        return 0.0
    if length == 1:
        return S + (stack - 1) * (S + (5 / 3) * M)
    right = length // 2
    left = length - right
    return left * (2 * S + 2 * M) + dlog3_rec(right, stack + 1) + dlog3_rec(left, stack)


def theta_chain(n):
    """Loop counters of theta_chain_comput_strategy with eight_above = 0
    (theta_isogenies.c:1601-1784). Returns Jacobian doublings, gluing basis
    evaluations, theta doublings, precomputations, generic steps by branch, evaluations."""
    todo = [n - 2] + [0] * 64
    cur = 0
    jdbl = 0
    while todo[cur] != 1:
        cur += 1
        prev = todo[cur - 1]
        nd = prev // 2 if prev >= 16 else prev - 1
        jdbl += 2 * nd * 2            # two kernel points, each a pair of curve points
        todo[cur] = prev - nd
    glue_basis = cur
    for i in range(cur):
        todo[i] -= 1
    precomp_done = True               # theta_precomputation after the gluing
    n_pre = 1
    cur -= 1
    thdbl = ev = steps_pre = steps_nopre = 0
    while cur >= 0 and todo[cur]:
        while todo[cur] != 1:
            cur += 1
            nd = todo[cur - 1] // 2
            if not precomp_done:
                n_pre += 1
                precomp_done = True
            thdbl += 2 * nd
            todo[cur] = todo[cur - 1] - nd
        if precomp_done:
            steps_pre += 1
        else:
            steps_nopre += 1
        precomp_done = False
        ev += 2 * cur
        for j in range(cur):
            todo[j] -= 1
        cur -= 1
    ev += 4                            # last two steps, theta_isogenies.c:1766-1774
    return jdbl, glue_basis, thdbl, n_pre, steps_pre, steps_nopre, ev


def counts(name, f, a, b, nwords, bits2, bits3, half_inv):
    p, fc = field_costs(f, a, b, nwords, half_inv)
    inv2, issq2, sqrt2 = fc["inv2"], fc["issq2"], fc["sqrt2"]
    cof2 = (f * 3**b).bit_length()     # P_COFACTOR_FOR_2F_BITLENGTH (control below)
    cof3 = (f * 2**a).bit_length()     # P_COFACTOR_FOR_3G_BITLENGTH
    diff_pt = 17 * M + 6 * S + sqrt2                       # basis.c:62
    jinv = 3 * M + 4 * S + inv2                            # ec.c:814

    # 3^b basis from a hint: two cofactor ladders (basis.c:281-336)
    basis3_hint = 2 * cof3 * XDBLADD + 10 * M + inv2 + diff_pt
    # 3^b basis without a hint, spec Algorithm 2.3 (physical p. 18), expected cost:
    # per attempt two x candidates, one ladder, b-1 triplings; R succeeds with
    # probability 8/9, S with probability (8/9)(3/4).
    attempt = 2 * (4 * M + S + issq2) + cof3 * XDBLADD + (b - 1) * XTPL
    basis3_nohint = (9 / 8 + 3 / 2) * attempt + diff_pt
    # E_A[3] basis of the compact verifier (basis.c:339-447): P 9/8 attempts, Q 3/2 attempts
    basis_e3 = (9 / 8 + 3 / 2) * (2 * (4 * M + S + issq2) + cof3 * XDBLADD + b * XTPL) \
        + XDBL + 4 * M + 3 * S + sqrt2
    # 2^a basis from a hint (basis.c:623-684): one A24 normalisation, half an inversion
    # on average for the hint_A branch, two cofactor ladders, difference point;
    # a curve that is not decoded with C = 1 (E_chl in the uncompressed verifier)
    # costs one more inversion (ec.c:33-45)
    basis2_hint = (inv2 + M) + 0.5 * inv2 + 2 * M + 2 * cof2 * XDBLADD + diff_pt
    # matrix applied to the basis: three bounded double-scalar ladders (id2iso.c:52-88,
    # ec.c:631-778); every one of the BITS_2 iterations runs when f = a
    biscal2 = bits2 * (XDBL_N + 2 * XADD) + XADD + inv2 + M
    matrix = 3 * biscal2
    # challenge kernel R + [chl]S (ec.c:489-628)
    biscal3 = bits3 * (XDBL_N + 2 * XADD) + XADD + inv2 + M
    # 3^b-isogeny chain (isog_chains.c:402-470)
    tpl0, ev0 = three_chain(b, 0)
    chain3 = tpl0 * XTPL + ev0 * XEVAL3 + b * XISOG3
    chain3_dual = chain3 + 12 * M + 7 * S                  # xisog.c:127, last step
    # dimension-2 chain (theta_isogenies.c:1601-1784)
    jdbl, gb, thdbl, npre, sp, snp, ev = theta_chain(a)
    assert sp + snp == a - 3           # gluing + (a-3) generic steps + two last steps
    lift = 2 * (3 * M + inv2 + 15 * M + 2 * S + 2 * M + S + sqrt2)   # ec.c:1156, twice
    gluing = 4 * JDBL + 8 * S + 4 * XDBL + 84 * M + (21 * M + inv2) + (9 * M + inv2) \
        + 2 * BASE_CHANGE + 8 * S                            # theta_isogenies.c:176
    last_two = (17 * M + 8 * S + 2 * sqrt2) + (10 * M + 4 * S + 3 * sqrt2)  # :869, :967
    split = 2 * M + sqrt2 + 16 * M + 4 * M + 8 * S           # :1108, :1382
    dim2 = lift + jdbl * JDBL + gluing + gb * GLUE_EVAL_BASIS + npre * TH_PRECOMP \
        + thdbl * TH_DBL + sp * TH_COMPUT_PRE + snp * TH_COMPUT_NOPRE + ev * TH_EVAL \
        + last_two + split
    push = GLUE_EVAL_SPECIAL + (a - 1) * TH_EVAL + 16 * M + 4 * M    # :2076
    # ec.c:1317: one square root always, a second in two of the three branches
    standard = 7 * M + 2 * S + (5 / 3) * sqrt2 + 2 * inv2

    # submitted verifiers, sign.c:1018-1125 and 1126-1265 (hash calls not counted)
    uncompressed = basis3_hint + 2 * jinv + biscal3 + chain3_dual + 2 * basis2_hint \
        + (inv2 + M) + matrix + dim2 + push + 3 * jinv
    compressed = 2 * basis2_hint + matrix + dim2 + 2 * jinv + standard + basis3_hint \
        + biscal3 + chain3 + 2 * jinv + basis_e3 + 2 * push

    # sign-25-1: checks that the specification places in verification and the code omits
    order3 = b * XTPL                           # Algorithm 7.32 line 15, one tripling per step
    isotropy = (a - 3 + 1) * CHECK_ISOTROPIC    # Algorithm 7.36 line 22 on a-3 steps, plus gluing
    fix1 = order3 + isotropy
    # for comparison: the order tests that the source holds under NDEBUG and assert,
    # run as written (basis.c:661-668, 684-690 on two bases; theta_isogenies.c:1637,
    # 1646, 1695-1696; test routines ec.h:456-486), in addition to fix1
    dbg = 2 * (2 * M + issq2 + issq2 + 3 * a * XDBL) + a * XDBL + a * JDBL + 2 * 3 * JDBL
    # sign-25-3: forward challenge isogeny from the hash-derived challenge (compact only).
    # E_com is not transmitted, so it is first brought to one model, as the code does for
    # E_chall with ec_curve_standardized (sign.c:864, 1189).
    fwd = biscal3 + chain3_dual + order3 + 2 * jinv
    fix3_hint = standard + basis3_hint + fwd
    fix3_nohint = standard + basis3_nohint + fwd
    fix3_floor = standard + basis3_hint + biscal3   # model, basis and kernel point only
    # sign-25-3, compact signer: the submitted code takes the challenge kernel from the image
    # basis (sign.c:785-792). The corrected signer adds the steps of the uncompressed signer
    # (sign.c:767-784): basis search with hint (basis.c:145) and change of basis
    # (id2iso.c:657, biextension.c:1168, 1007), and the same model of E_com.
    bits3m1 = (3**b - 1).bit_length()                        # ladder length, THREEpFm1
    pow3g = bits2 * S + bin(f).count("1") * M                # fp2_pow_vartime by f*2^a, fp2.c:290
    tate = (69 * M + inv2) + bits3m1 * 11 * CUBICAL + 15 * M + (12 * M + inv2) \
        + 5 * (M + pow3g) + 4 * (inv2 + dlog3_rec(b, 1))
    change3 = (inv2 + M) + lift + 4 * (JADD + S) + tate + biscal3
    sign3 = standard + 18 * M + basis3_nohint + change3
    return dict(p=p, cof2=cof2, cof3=cof3, U=uncompressed, C=compressed, dim2=dim2,
                chain3=chain3, basis3_hint=basis3_hint, basis3_nohint=basis3_nohint,
                biscal3=biscal3, fix1=fix1, fix3_hint=fix3_hint, fix3_nohint=fix3_nohint,
                fix3_floor=fix3_floor, sign3=sign3, dbg=dbg,
                steps=(jdbl, gb, thdbl, npre, sp, snp, ev),
                tpl=tpl0, ev3=ev0)


def pct(x, base):
    return 100.0 * x / base


def main():
    print("Unit: F_p multiplications and squarings. Hash calls are not counted.")
    print()
    print("Control 1: cofactor ladder lengths (ec_params.h P_COFACTOR_FOR_2F/3G_BITLENGTH)")
    hdr = {"Level1-eff": (124, 132), "Level1-sec": (134, 138), "Level2-eff": (159, 167),
           "Level2-sec": (164, 172), "Level3-eff": (248, 264), "Level3-sec": (259, 265),
           "Level5-eff": (510, 518), "Level5-sec": (534, 541)}
    rows = []
    for (name, f, a, b, nw, b2, b3, hi, tu, tc, ts) in SETS:
        r = counts(name, f, a, b, nw, b2, b3, hi)
        ok = (r["cof2"], r["cof3"]) == hdr[name]
        print(f"  {name}: computed ({r['cof2']},{r['cof3']}) header {hdr[name]} {'ok' if ok else 'MISMATCH'}")
        rows.append((name, a, b, r, tu, tc))
    print()
    print("Baseline counts and control 2: compressed/uncompressed Verify ratio, model vs Table 6.2")
    print(f"  {'set':<11}{'uncomp':>10}{'comp':>10}{'dim-2 %U':>10}{'3-chain %U':>11}{'model C/U':>11}{'table C/U':>11}{'cycles/unit U':>15}")
    for (name, a, b, r, tu, tc) in rows:
        print(f"  {name:<11}{r['U']:>10.0f}{r['C']:>10.0f}{pct(r['dim2'], r['U']):>10.1f}"
              f"{pct(r['chain3'], r['U']):>11.1f}{r['C'] / r['U']:>11.3f}{tc / tu:>11.3f}"
              f"{tu * 1e6 / r['U']:>15.0f}")
    print()
    print("sign-25-1: specified order and isotropy checks, added work; last column adds the")
    print("source's debug-only order tests run as written, in % of uncompressed Verify")
    print(f"  {'set':<11}{'added':>9}{'uncomp %':>10}{'comp %':>9}{'with debug tests %':>20}")
    for (name, a, b, r, tu, tc) in rows:
        print(f"  {name:<11}{r['fix1']:>9.0f}{pct(r['fix1'], r['U']):>10.1f}{pct(r['fix1'], r['C']):>9.1f}"
              f"{pct(r['fix1'] + r['dbg'], r['U']):>20.1f}")
    print()
    print("sign-25-3: forward challenge isogeny in compact Verify, added work in % of compact Verify")
    print(f"  {'set':<11}{'floor':>8}{'hint':>8}{'no hint':>9}{'hint+25-1':>11}{'nohint+25-1':>13}")
    for (name, a, b, r, tu, tc) in rows:
        print(f"  {name:<11}{pct(r['fix3_floor'], r['C']):>8.1f}{pct(r['fix3_hint'], r['C']):>8.1f}"
              f"{pct(r['fix3_nohint'], r['C']):>9.1f}"
              f"{pct(r['fix3_hint'] + r['fix1'], r['C']):>11.1f}"
              f"{pct(r['fix3_nohint'] + r['fix1'], r['C']):>13.1f}")
    print()
    print("sign-25-3: steps added to the compact signer, in F_p multiplications and in % of")
    print("compact Verify work (signing itself is not counted, and no signing percentage is given)")
    print(f"  {'set':<11}{'added':>9}{'% Verify':>10}")
    for (name, a, b, r, tu, tc) in rows:
        print(f"  {name:<11}{r['sign3']:>9.0f}{pct(r['sign3'], r['C']):>10.1f}")
    print()
    print("Compact signature with a two-byte basis hint for E_com (Table 6.1, physical p. 61)")
    for name, sig in [("Level1-eff", 168), ("Level1-sec", 178), ("Level2-eff", 208),
                      ("Level2-sec", 218), ("Level3-eff", 326), ("Level3-sec", 338),
                      ("Level5-eff", 648), ("Level5-sec", 678)]:
        print(f"  {name}: {sig} -> {sig + 2} bytes, +{200.0 / sig:.2f}%")


if __name__ == "__main__":
    main()
