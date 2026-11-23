#!/usr/bin/env python3
"""Fixing-cost aid for sign-26 (SQIsign2d-push1/2): work of the specified challenge grinding.

Specification section 5.1(4) (physical p. 33) builds HASH by iterating a hash
ceil(2^lambda / 3^e2) times. The submitted hash_to_challenge makes one XOF call
(sqisigndim2/ref/sqisigndim2x/sign.c:738-769). This script counts the hash calls
that the specified construction adds, and compares them with a lower bound on the
field work of the submitted verifier. Nothing is implemented or measured.

Unit of comparison: 64-bit word operations.
  - One hash iteration: one Keccak-f[1600] permutation, because the iterated value
    (8*NWORDS_FIELD bytes) fits the 136-byte SHAKE256 rate (common/generic/fips202.c:20).
    The submitted permutation (fips202.c:240, 24 rounds) holds 152 XOR, 58 ROL, 50 AND
    and 50 NOT per two rounds: 155 word operations per round.
  - One F_p multiplication: the mulx, addcarryx, subborrowx and cmovznz calls of the
    submitted fiat-crypto routine (gf/ref/lvlN/fp_*.c, fiat_*_mul).

Run:  python3 -B sign-26/fixing_cost_hash_work.py
"""

KECCAK_WORD_OPS = 24 * (152 + 58 + 50 + 50) // 2      # 3720 per permutation

M, S = 3, 2                       # fp2_mul, fp2_sqr in F_p multiplications (gf/ref/gfx/fp2.c:120-145)
XDBLADD = 8 * M + 4 * S           # ec/ref/ecx/ec.c:157
XTPL = 7 * M + 5 * S              # ec/ref/ecx/basis.c:7
JDBL = 6 * M + 6 * S              # ec/ref/ecx/ec.c:753
TH_DBL = 8 * M + 8 * S            # hd/ref/hdx/theta_structure.c:77
TH_EVAL = 4 * M + 4 * S           # hd/ref/hdx/theta_isogenies.c:1033
TH_COMPUT = 11 * M + 8 * S        # hd/ref/hdx/theta_isogenies.c:719, cheaper branch

# level: lambda, e1, e2 (spec section 5.2, physical pp. 35-36); NWORDS_FIELD;
# P_COFACTOR_FOR_2F/3G_BITLENGTH (precomp/ref/lvlN/include/ec_params.h);
# SQISIGN_response_length (klpt_constants.h); word operations of fiat_*_mul
# (lvl1: 32 mulx + 59 addcarryx + 5 subborrowx + 4 cmovznz; lvl3: 128 + 247 + 9 + 8);
# Verify and Sign in ms from spec Table 3 (physical p. 57).
LEVELS = {
    1: dict(lam=128, e1=131, e2=78, nwords=4, cof2=124, cof3=132, n=128, fpmul=100,
            verify_ms=12.54, sign_ms=64.04),
    3: dict(lam=256, e1=263, e2=156, nwords=8, cof2=248, cof3=264, n=260, fpmul=392,
            verify_ms=96.01, sign_ms=466.88),
}


def three_chain_triplings(n):
    """Triplings of ec_eval_three_rec (ec/ref/ecx/isog_chains.c:228-265)."""
    if n <= 1:
        return 0
    right = int(n / 1.5)
    left = n - right
    return left + three_chain_triplings(right) + three_chain_triplings(left)


def theta_rec(length, stack):
    """Doublings, evaluations and steps of theta_chain_comput_rec
    (hd/ref/hdx/theta_isogenies.c:1559-1618), for both kernel points."""
    if length == 0:
        return 0, 0, 0
    if length == 1:
        return 0, 2 * stack, 1
    right = 2 * length // 3
    left = length - right
    d1, e1, s1 = theta_rec(right, stack + 1)
    d2, e2, s2 = theta_rec(left, stack)
    return 2 * left + d1 + d2, e1 + e2, s1 + s2


def verifier_lower_bound(c):
    """F_p multiplications of fixed-length loops in protocols_verif
    (sqisigndim2/ref/sqisigndim2x/sign.c:1009-1147). Left out: square roots,
    inversions, basis retries, the two double-scalar ladders, the matrix
    application, gluing, 3-isogeny evaluations, the small 2-isogeny chain."""
    basis3 = 2 * c["cof3"] * XDBLADD + 2 * (c["e2"] - 1) * XTPL     # ec/ref/ecx/basis.c:461
    chain3 = three_chain_triplings(c["e2"]) * 2 * XDBLADD           # tripling = 2-bit ladder
    basis2 = 2 * (2 * c["cof2"] * XDBLADD)                          # ec/ref/ecx/basis.c:272, twice
    n = c["n"]                                                      # chain length with v = nrsp = 0
    dbl, ev, steps = theta_rec(n - 3, 1)
    dim2 = 4 * (n - 1) * JDBL + (4 + dbl + 2) * TH_DBL + (ev + 2) * TH_EVAL \
        + (steps + 2) * TH_COMPUT                                   # theta_isogenies.c:1621-1752
    return basis3 + chain3 + basis2 + dim2, dim2


def main():
    for lvl, c in LEVELS.items():
        k = -(-(2 ** c["lam"]) // 3 ** c["e2"])          # ceil(2^lambda / 3^e2)
        extra_calls = k - 1
        assert 8 * c["nwords"] <= 136
        hash_ops = extra_calls * KECCAK_WORD_OPS
        lb, dim2 = verifier_lower_bound(c)
        field_ops = lb * c["fpmul"]
        print(f"Level-{lvl}: iterations {k}, added hash calls {extra_calls} per signing attempt and per verification")
        print(f"  added work: {extra_calls} x {KECCAK_WORD_OPS} = {hash_ops} word operations")
        print(f"  verifier lower bound: {lb} F_p multiplications ({dim2} in the (2,2)-chain)"
              f" x {c['fpmul']} = {field_ops} word operations")
        print(f"  verification increase: at most {100.0 * hash_ops / field_ops:.2f}%")
        print(f"  signing increase: at most {100.0 * hash_ops / field_ops * c['verify_ms'] / c['sign_ms']:.2f}%"
              f" (Sign/Verify = {c['sign_ms'] / c['verify_ms']:.2f} in Table 3)")


if __name__ == "__main__":
    main()
