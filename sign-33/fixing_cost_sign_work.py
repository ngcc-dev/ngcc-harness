#!/usr/bin/env python3
"""Fixing-cost aid for sign-33-6: expected signing work of the VDOO reference
signer with the submitted oil-term filter and with the specified dense one.

Counts only.  It implements no part of the scheme and runs no submitted code.
The loop structure counted is that of vdoo_sign.c (reference trees):

  vdoo_inverse_F, per attempt (lines 178-223):
    v calls of get_randombytes                               (181-188)
    d diagonal equations, solve_diagonal_eq                  (7-64)
    substitute_fixed_vars + gauss_elimination for layer 1    (66-167)
    substitute_fixed_vars + gauss_elimination for layer 2

Units: "reads" are coefficient reads of sk->F, "mults" are gfv_mul calls,
"rng" are get_randombytes calls.  A coefficient that the key generator left
random is nonzero with probability (q-1)/q, and the signer multiplies only
then (the `if (coeff != 0)` branches).

Filters:
  submitted  is_allowed_oil1/2 keep one vinegar-oil column per equation, so
             each layer's matrix is diagonal and gauss_elimination succeeds
             only if every diagonal entry is nonzero: (1-1/q)^o.
  dense      every vinegar-oil product of the layer is kept (specification
             section 4.3), so the matrix is uniform.  gauss_elimination adds
             all lower rows to a zero pivot once and fails if it stays zero:
             1/q^2 per step, 1/q at the last step.

Expected work per signature = expected work per attempt / success probability
(attempts are independent; the two 100-attempt caps are ignored).
"""

PARAMS = {  # specification Table 1 (physical p. 20): q, v, d, o1, o2
    "VDOO-128": (16, 67, 16, 14, 40),
    "VDOO-256": (256, 205, 10, 17, 68),
    "VDOO-512": (256, 260, 10, 10, 175),
}

# Costing aids for the Lead sign-33-9, printed by fixing_cost_wedge_params.py.
# They are compared, with the dense filter, against submitted VDOO-512.
LEAD_PARAMS = {
    "VDOO-512 rebalanced": (256, 260, 10, 44, 141),
    "VDOO-512 more vinegar": (256, 294, 10, 10, 175),
}


def ge(n, q, dense):
    """Return (pass probability, expected mults) of gauss_elimination."""
    reach, mults = 1.0, 0.0
    for i in range(n):
        if dense:
            fail = 1 / q**2 if i < n - 1 else 1 / q
        else:
            fail = 1 / q
        reach *= 1 - fail
        mults += reach * n * (n - i + 1)  # normalise one row, clear n-1 rows
    return reach, mults


def attempt(q, v, d, o1, o2, dense):
    """Expected (reads, mults, rng) of one attempt and its success probability."""
    nz = (q - 1) / q
    reach = 1.0
    reads = mults = 0.0
    rng = float(v)
    for i in range(d):
        k = v + i
        tri = k * (k + 1) // 2
        reads += reach * (k + tri)
        mults += reach * (nz * (k + 2 * tri) + 1)
        rng += reach / q**2          # free diagonal variable when A = 0, B = y
        reach *= 1 - (q - 1) / q**2  # A = 0 and B != y
    for o, fixed in ((o1, v + d), (o2, v + d + o1)):
        tri = fixed * (fixed + 1) // 2
        cols = o if dense else 1     # nonzero vinegar-oil columns per equation
        reads += reach * o * (o * fixed + tri)
        mults += reach * nz * o * (cols * fixed + 2 * tri)
        ok, g = ge(o, q, dense)
        mults += reach * g
        reach *= ok
    return reads, mults, rng, reach


def clean_attempt(q, v, d, o1, o2, dense):
    """(reads, mults) of one attempt in which no stage fails."""
    nz = (q - 1) / q
    reads = mults = 0.0
    for i in range(d):
        k = v + i
        tri = k * (k + 1) // 2
        reads += k + tri
        mults += nz * (k + 2 * tri) + 1
    for o, fixed in ((o1, v + d), (o2, v + d + o1)):
        tri = fixed * (fixed + 1) // 2
        cols = o if dense else 1
        reads += o * (o * fixed + tri)
        mults += nz * o * (cols * fixed + 2 * tri) + o * o * (o + 3) / 2
    return reads, mults


def per_signature(name, dense):
    q, v, d, o1, o2 = PARAMS[name] if name in PARAMS else LEAD_PARAMS[name]
    n, m = v + d + o1 + o2, d + o1 + o2
    reads, mults, rng, p = attempt(q, v, d, o1, o2, dense)
    # apply_invS and apply_invT, once per signature (lines 228-261)
    return p, 1 / p, reads / p + m * m + n * n, mults / p + m * m + n * n, rng / p


def main():
    for name in PARAMS:
        q, v, d, o1, o2 = PARAMS[name]
        sub = per_signature(name, False)
        den = per_signature(name, True)
        full_sub = attempt(q, v, d, o1, o2, False)
        full_den = attempt(q, v, d, o1, o2, True)
        lin_sub = o1 * (v + d) + o2 * (v + d + o1)
        lin_den = o1 * o1 * (v + d) + o2 * o2 * (v + d + o1)
        print(name, "(q, v, d, o1, o2) =", PARAMS[name])
        print(f"  vinegar-oil products per attempt: submitted {lin_sub}, dense {lin_den}")
        for label, r in (("submitted", sub), ("dense", den)):
            print(f"  {label:9s} success/attempt {r[0]:.4f}  attempts {r[1]:7.2f}"
                  f"  reads {r[2]:.3e}  mults {r[3]:.3e}  rng {r[4]:.3e}")
        print(f"  dense/submitted per signature: attempts {den[1]/sub[1]:.3f}"
              f"  reads {den[2]/sub[2]:.3f}  mults {den[3]/sub[3]:.3f}"
              f"  rng {den[4]/sub[4]:.3f}")
        print(f"  dense/submitted per attempt (expected): reads"
              f" {full_den[0]/full_sub[0]:.3f}  mults {full_den[1]/full_sub[1]:.3f}")
        cs = clean_attempt(q, v, d, o1, o2, False)
        cd = clean_attempt(q, v, d, o1, o2, True)
        print(f"  dense/submitted for one attempt without failure: reads"
              f" {cd[0]/cs[0]:.3f}  mults {cd[1]/cs[1]:.3f}")
    base = per_signature("VDOO-512", False)
    for name in LEAD_PARAMS:
        r = per_signature(name, True)
        print(name, "(q, v, d, o1, o2) =", LEAD_PARAMS[name])
        print(f"  dense     success/attempt {r[0]:.4f}  attempts {r[1]:7.2f}"
              f"  reads {r[2]:.3e}  mults {r[3]:.3e}  rng {r[4]:.3e}")
        print(f"  dense here / submitted VDOO-512 per signature: reads {r[2]/base[2]:.3f}"
              f"  mults {r[3]/base[3]:.3f}  rng {r[4]/base[4]:.3f}")


if __name__ == "__main__":
    main()
