#!/usr/bin/env python3
"""Field-operation counts of BAG-Piglet-128 decapsulation for kem-04/fixing_cost.md.

Standard library only. It prints counts; it implements no part of BAG-Piglet.

Decapsulation (src/common/ccakem.c:129-134) runs key generation, decryption
and encryption. The counts follow the reference code under
Implementations/Reference_Implementation/bag_piglet128/src/:

- ring product (ffi_vec_mul -> rbc_poly_mulmod_sparse, rbc/rbc_poly.c:170-176):
  n^2 field multiplications; the reduction modulo the sparse P is additions.
  Key generation does 2 (scheme/bag_piglet.c:118-119), encryption 8 (:260-282)
  and decryption 2 (:360-363).
- encoder (common/augabidulin.c:30-68): (k - 1) n' squarings and k n'
  multiplications.
- decoder (common/augabidulin.c:70-144 and common/gabidulin.c:162-462):
  interpolation of the degree-eps annihilator V2 (q_polynomial.c:130-155),
  n n1 + eps evaluations of V2, interpolation of A and I over k + eps points
  (q_polynomial.c:174-207), the evaluation loop over n', the reconstruction
  loop over the last n' - k - eps positions, and the two left divisions
  (q_polynomial.c:431-489), whose steps each compute an nth root by
  squarings (ffi/ffi_elt.c:59-66).
- inversion (rbc/rbc_elt.c:238-273): the Itoh-Tsujii chain, simulated here.

Data-dependent parts are counted for the generic case: every reconstruction
step has a nonzero u1 entry (update type 1) and every division step lowers
the remainder degree by one. Not counted: the SM3 XOF (six 6144-byte outputs
per decapsulation), hashing, sampling, and the GF(2) rank computations of
support recovery and sampling.
"""

import itertools

N1 = 2      # PARAM_N_1
BLOCK = 2   # api.h

# name: (n, m, n', k, eps)
SETS = {
    "submitted code": (43, 47, 43, 3, 30),
    "set (a)": (55, 43, 43, 3, 34),
    "set (b)": (56, 37, 37, 4, 29),
}


class Count:
    def __init__(self):
        self.mul = 0
        self.sqr = 0

    def add(self, mul=0, sqr=0):
        self.mul += mul
        self.sqr += sqr


def inversion(m):
    """rbc_elt_inv: (multiplications, squarings)."""
    target = m - 1
    top_bit, value = 0, target
    while value > 1:
        value >>= 1
        top_bit += 1
    mul = sqr = 0
    span = 1
    for bit in range(top_bit - 1, -1, -1):
        sqr += span
        mul += 1
        span *= 2
        if (target >> bit) & 1:
            sqr += 1
            mul += 1
            span += 1
    return mul, sqr + 1


def nth_root_sqr(m, n):
    exp = n * (m - 1) % m
    return 1 + max(exp - 1, 0)


def evaluate(c, deg):
    if deg >= 0:
        c.add(mul=deg + 1, sqr=deg)


def scalar_mul(c, deg, nonzero=True):
    if deg >= 0 and nonzero:
        c.add(mul=deg + 1)


def qexp(c, deg):
    if deg >= 0:
        c.add(sqr=deg + 1)


def q_mul(c, deg_p, deg_q):
    """q_polynomial_mul(o, p, q): loops over the coefficients of q, then p."""
    if deg_p >= 0 and deg_q >= 0:
        c.add(mul=(deg_q + 1) * (deg_p + 1), sqr=(deg_q + 1) * deg_p)


def inv(c, m):
    a, b = inversion(m)
    c.add(mul=a, sqr=b)


def left_div(c, m, deg_a, deg_b):
    """Generic case: each step lowers the remainder degree by one."""
    inv(c, m)
    steps = 0
    for deg_s in range(deg_a - deg_b, -1, -1):
        c.add(mul=1, sqr=nth_root_sqr(m, deg_b))
        q_mul(c, deg_b, deg_s)
        steps += 1
    return deg_a - deg_b, steps


def decoder(n, m, nd, k, eps):
    parts = {}
    kg = k + eps
    t = (nd - kg) // 2

    c = Count()
    for i in range(eps):            # q_polynomial_set_interpolate, o1 of degree i
        evaluate(c, i)
        inv(c, m)
        c.add(mul=1)
        qexp(c, i)
        scalar_mul(c, i)
    parts["V2 interpolation"] = c

    c = Count()
    for _ in range(n * N1 + eps):   # suffix, error support and prefix evaluations
        evaluate(c, eps)
    parts["V2 evaluations"] = c

    c = Count()
    for i in range(kg):             # interpolate_vect_and_zero, o1 degree i, o2 degree i - 1
        evaluate(c, i)
        evaluate(c, i - 1)
        inv(c, m)
        c.add(mul=1)
        scalar_mul(c, i)
        qexp(c, i)
        scalar_mul(c, i)
    parts["A, I interpolation"] = c

    c = Count()
    for _ in range(nd):             # A(g_i), V0(y_i) = 0, I(g_i), V1(y_i) with V1 = 1
        evaluate(c, kg)
        evaluate(c, kg - 1)
        evaluate(c, 0)
    parts["u0, u1 evaluations"] = c

    c = Count()
    dn0, dn1, dv0, dv1 = 0, -1, -1, 0
    for i in range(kg, nd):         # update type 1
        inv(c, m)
        c.add(mul=2, sqr=1)
        scalar_mul(c, dn1)
        qexp(c, dn1)
        scalar_mul(c, dv1)
        qexp(c, dv1)
        scalar_mul(c, dn1)
        scalar_mul(c, dv1)
        c.add(mul=2 * (nd - 1 - i), sqr=nd - 1 - i)
        dn0, dn1, dv0, dv1 = (dn1 + 1 if dn1 >= 0 else -1), max(dn0, dn1), dv1 + 1, max(dv0, dv1)
    parts["reconstruction"] = c

    c = Count()
    q_mul(c, dn1, kg)               # N1 o A
    deg_q, _ = left_div(c, m, dn1 + kg, dv1)
    deg_f = max(deg_q, kg - 1)      # + I
    left_div(c, m, deg_f, eps)      # / V2
    parts["divisions"] = c
    return parts, (t, dn1, dv1)


def operation_counts(n, m, nd, k, eps):
    ring = Count()
    ring.add(mul=(2 + 2 + 8) * n * n)
    enc = Count()
    enc.add(mul=k * nd, sqr=(k - 1) * nd)
    parts, degs = decoder(n, m, nd, k, eps)
    return ring, enc, parts, degs


def total(counts):
    c = Count()
    for x in counts:
        c.add(mul=x.mul, sqr=x.sqr)
    return c


def cost(c, m, m0, per_m, sqr_weight):
    """A multiplication costs 1, or m/m0 when charged in proportion to m (the
    bit-serial loop of rbc/rbc_elt.c:190-205). A squaring (rbc/rbc_elt.c:211-236)
    works byte by byte on one 64-bit word whatever m is, and is charged 0 or 1
    multiplication of the submitted field."""
    unit = m / m0 if per_m else 1
    return unit * c.mul + sqr_weight * c.sqr


def main():
    print("BAG-Piglet-128 decapsulation, field operations (multiplications / squarings):")
    data = {}
    for name, (n, m, nd, k, eps) in SETS.items():
        ring, enc, parts, (t, dn1, dv1) = operation_counts(n, m, nd, k, eps)
        dec = total(parts.values())
        allc = total([ring, enc, dec])
        data[name] = (m, ring, enc, dec, allc)
        print(f"  {name}: n = {n}, m = {m}, n' = {nd}, k = {k}, eps = {eps}, t = {t}; "
              f"inversion {inversion(m)[0]} mul + {inversion(m)[1]} sqr; final deg N1, V1 = {dn1}, {dv1}")
        print(f"    12 ring products: {ring.mul} / 0; encoder: {enc.mul} / {enc.sqr}")
        for label, c in parts.items():
            print(f"    decoder, {label}: {c.mul} / {c.sqr}")
        print(f"    decoder total: {dec.mul} / {dec.sqr}; decapsulation total: {allc.mul} / {allc.sqr}")
        share_lo = dec.mul / allc.mul
        share_hi = (dec.mul + dec.sqr) / (allc.mul + allc.sqr)
        print(f"    decoder share of the counted work: {100 * share_lo:.1f}% (squarings free) to "
              f"{100 * share_hi:.1f}% (a squaring costs a multiplication)")
    print("Decapsulation, repaired / submitted counted work:")
    print("  (multiplication charged 1 or m/47; squaring charged 0 or 1 submitted multiplication)")
    base = data["submitted code"]
    for name in ("set (a)", "set (b)"):
        new = data[name]
        ratios = []
        for per_m, w in itertools.product((False, True), (0, 1)):
            r = cost(new[4], new[0], base[0], per_m, w) / cost(base[4], base[0], base[0], per_m, w)
            ratios.append(r)
            print(f"  {name}: per-m {per_m!s:5}, squaring {w}: {r:.3f}")
        ring_only = new[1].mul / base[1].mul
        print(f"  {name}: range {min(ratios):.3f} to {max(ratios):.3f}; ring products alone "
              f"{ring_only:.3f} (unit) and {ring_only * new[0] / base[0]:.3f} (per m)")


if __name__ == "__main__":
    main()
