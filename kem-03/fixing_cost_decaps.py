#!/usr/bin/env python3
"""Operation counts for kem-03/fixing_cost.md (finding kem-03-3 and the repaired sets).

Counts calls of rbc_elt_mul in the submitted BAG-Loong reference code, first
with the submitted public support pool and then with the random supports that
Algorithm 1 specifies. Standard library only; it counts and implements no
scheme. Paths are under
Implementations/Reference_Implementation/Loong-Block-ms-128/ (the four
parameter trees share these sources).

Cost unit: one rbc_elt_mul call. A squaring is one call and an inversion is
2m - 1 calls (lib/rbc-47/rbc_elt.c:208-211, 241-263). Not counted: XOF output,
rank computations over F_2, and the dependence of one call on m and on the
weight of its first operand (rbc_elt.c:185-200).
"""
import sys

sys.dont_write_bytecode = True

# level: n, m, n', n1, n2, eps, k, A_(X,Y) = (t1, t2, g), A_(R1,E|R2) = (r1, r2, g'); Table 1, physical p. 8
SUBMITTED = {
    128: (42, 47, 46, 10, 10, 33, 3, (5, 5, 4), (5, 5, 3)),
    256: (65, 67, 67, 12, 13, 51, 4, (5, 6, 3), (6, 6, 3)),
    384: (83, 83, 83, 14, 15, 66, 5, (6, 6, 3), (7, 7, 4)),
    512: (104, 97, 97, 15, 15, 73, 6, (7, 7, 4), (7, 7, 4)),
}
# Sets costed by fixing_cost_params.py. Costing aids, not recommended parameters.
COSTED = {
    128: (40, 47, 47, 11, 10, 36, 3, (5, 4, 2), (4, 6, 2)),
    256: (67, 73, 73, 13, 14, 59, 4, (6, 5, 1), (5, 7, 1)),
    384: (78, 101, 101, 15, 16, 83, 4, (9, 9, 9), (10, 9, 9)),
    512: (91, 127, 127, 17, 16, 98, 5, (10, 10, 10), (11, 10, 10)),
}


def pool(r1, r2, g):
    """Exponents of beta chosen by sample_support_pair_logical (src/loong_pke.c:391-415)."""
    return set(range(r1)), set(range(g)) | set(range(r1, r1 + r2 - g))


def submitted_error_rank(key, enc):
    """Dimension spanned by the exponents of R2*Y + E - R1*X with the public pool."""
    sx, sy = pool(*key)      # X and Y, src/loong_pke.c:754-769
    sr1, sr2 = pool(*enc)    # R1 and E share one support, R2 the other, src/loong_pke.c:891-911
    exps = {a + b for a in sr2 for b in sy} | {a + b for a in sr1 for b in sx} | sr1
    return len(exps), max(exps)


def specified_error_rank(key, enc):
    return enc[0] * key[0] + enc[1] * key[1] - enc[2] * key[2]


def interpolation(big_k, m):
    """qpoly_set_interpolate_vect_and_zero on big_k points (src/qpoly.c:473-541)."""
    return 7 * big_k * (big_k - 1) // 2 + big_k * (2 * m + 3) + 1


def decoder(tau, npr, k, m):
    """augabidulin_code_decode at tail rank tau, no residual error (src/augabidulin.c:107-179)."""
    big_k = k + tau
    return (2 * tau * tau + tau                      # V2, src/qpoly.c:424-471
            + npr * (2 * tau + 1)                    # V2 at n' points, src/augabidulin.c:163-165
            + 2 * interpolation(big_k, m)            # src/gabidulin.c:182 and :390
            + npr * (4 * big_k + 1)                  # src/gabidulin.c:192-203
            + (2 * m - 1) + k * (1 + m - tau) + (2 * tau + 1) * k * (k + 1) // 2)  # src/gabidulin.c:415


def theorem21(tau, total, npr, k):
    """Operation count in the proof of Theorem 2.1 (physical p. 12)."""
    return (total - npr) * tau + tau * tau + npr * tau + npr * npr + k * tau


def counts(row, tau):
    n, m, npr, n1, n2, _, k, _, _ = row
    keygen = n * n * n1                                   # S = H*X, src/loong_pke.c:771
    enc = n2 * n * n + n2 * n * n1 + npr * (2 * k - 1)    # src/loong_pke.c:913-915, src/gabidulin.c:39-51
    dec = n2 * n * n1 + decoder(tau, npr, k, m)           # src/loong_pke.c:1008-1018
    return keygen, enc, dec + enc                         # decapsulation re-encrypts, src/loong_kem.c:217-232


def xof_bytes(row):
    n, m, npr, n1, n2, _, _, _, enc = row
    elt = (m + 7) // 8
    public = (npr * 1024 + n * n) * elt                   # src/loong_pke.c:19, 592-611
    sampler = 65536 + sum(2 * 4096 * r + ((r + 7) // 8) * size
                          for r, size in ((enc[1], n2 * n), (enc[0], n2 * n), (enc[0], n2 * n1)))
    return 2 * public + sampler


def pct(new, old):
    return f"{100 * (new / old - 1):+.0f}%"


def main():
    for level, row in SUBMITTED.items():
        n, m, npr, n1, n2, eps, k, key, enc = row
        tb, top = submitted_error_rank(key, enc)
        tc = specified_error_rank(key, enc)
        kg, en, de_b = counts(row, tb)
        _, _, de_c = counts(row, tc)
        d_b, d_c = decoder(tb, npr, k, m), decoder(tc, npr, k, m)
        s_b, s_c = theorem21(tb, n1 * n2, npr, k), theorem21(tc, n1 * n2, npr, k)
        print(f"BAG-Loong-{level}")
        print(f"  error rank: submitted pool at most {tb} (largest exponent {top} < m = {m});"
              f" specified {tc}; delta {(npr - k + eps) // 2}; decoder cap eps = {eps}")
        print(f"  submitted code: keygen {kg}, encaps {en}, decaps {de_b} calls (decoder {d_b})")
        print(f"  specified supports, same parameters: decoder {d_c} (x{d_c / d_b:.2f}),"
              f" decaps {de_c} ({pct(de_c, de_b)}); keygen and encaps unchanged")
        print(f"  with the Theorem 2.1 count for the decoder: {s_b} -> {s_c} (x{s_c / s_b:.2f}),"
              f" decaps {pct(de_c - d_c + s_c, de_b - d_b + s_b)}")
        print(f"  XOF output per decapsulation, unchanged: {xof_bytes(row)} bytes;"
              f" uncounted work needed to bring decaps under +10%: {((de_c - de_b) / 0.10 - de_b) / de_b:.1f}"
              f" times the submitted calls")
        if level in COSTED:
            new = COSTED[level]
            kg2, en2, de2 = counts(new, specified_error_rank(new[7], new[8]))
            f = new[1] / m
            print(f"  costed parameter set, specified supports: keygen {kg2} ({pct(kg2, kg)}),"
                  f" encaps {en2} ({pct(en2, en)}), decaps {de2} ({pct(de2, de_b)})")
            print(f"  the same with one call charged in proportion to m ({m} -> {new[1]}):"
                  f" keygen {pct(f * kg2, kg)}, encaps {pct(f * en2, en)}, decaps {pct(f * de2, de_b)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
