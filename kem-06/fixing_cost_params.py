#!/usr/bin/env python3
"""Parameter, size and work estimates for kem-06/fixing_cost.md (kem-06-8).

Searches for the smallest BRA parameters at which the maximal-minors (MM) key
recovery on the transposed ideal code reaches the security target, with no
margin, and prints the sizes and operation counts that follow. It is a
costing aid: it implements no part of BRA, and the parameters it prints are
not recommended parameters.

Cost model: security/rank_mm_costs.py (the kem-06-8 witness), imported
unchanged. `search_two` is its ordinary-MM search; `zd_cost` is its ZD-MM
formula, searched here over the guesses (a1, a2) and the contracted block with
no punctures.

Construction constraints (specification, physical pp. 13-16):
  t = m <= n (Definition 3.4);  r = wx*wr2 + wy*wr1 + we <= min((n-k)/2, m-k);
  DFR = 4 * 2^(a*(m+r-a-n)) <= 2^-level, a = m-k-r+1 (Theorem 3.1, gamma = 4);
  m*k - 64 >= max(level, 2*quantum level) (plaintext space);
  gcd(m, n) = 1, so that an irreducible P(X) over F_2 stays irreducible.
Free: m, n, k (not above the submitted k), wx, wy (from the submitted value
to three above). Kept: wr1, wr2, we. Parameters whose m*n would exceed the
submitted m*n by more than CAP are not searched.

With --aligned the ciphertext weights are not kept: for each pair of key
weights searched, (wr1, wr2) is set to (wx, wy). The transposed relation
u = r1 + h*r2 then has the same weights as the key relation, and
r = 2*wx*wy + we. For two pairs of weights (a, b) and (c, d) that both reach
the target, a*d + b*c >= 2*min(a*b, c*d), so no other choice of ciphertext
weights gives a smaller r. The specification's three-block ciphertext
instance is not modeled here, so that run is a lower estimate.

With --we 1 the weight of e is set to 1 instead of the submitted 8. The
three-block ciphertext instance, which we protects, is not modeled here, so
that run is a floor on the size and not a repair. --wr1 and --wr2 replace the
ciphertext weights wr1 and wr2 in the same way, for a spot check of other
ciphertext weights.

Run:  python3 -B kem-06/fixing_cost_params.py             (five runs; about 12 minutes on 16 cores)
      python3 -B kem-06/fixing_cost_params.py --controls   (controls only; under a minute)
      python3 -B kem-06/fixing_cost_params.py --aligned    (five runs; about 9 minutes)
      python3 -B kem-06/fixing_cost_params.py --aligned --we 1 --runs 256:2,512:2   (the floor; about 8 minutes)
      python3 -B kem-06/fixing_cost_params.py --wr1 7 --wr2 5 --runs 256:2
"""
import sys

sys.dont_write_bytecode = True

import argparse
import importlib.util
import os
from math import ceil, gcd
from multiprocessing import get_context

HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "rank_mm_costs", os.path.join(HERE, "..", "security", "rank_mm_costs.py"))
mm = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(mm)

# Specification Table 2 and Table 3, physical p. 16.
SETS = {
    128: dict(m=67, n=121, k=4, wx=4, wy=5, wr1=5, wr2=5, we=8, r=53, eta=80,
              pk=1078, ct=2092, dfr=-130),
    256: dict(m=83, n=161, k=4, wx=5, wy=5, wr1=6, wr2=6, we=8, r=68, eta=128,
              pk=1735, ct=3406, dfr=-262),
    512: dict(m=127, n=221, k=5, wx=6, wy=6, wr1=7, wr2=8, we=8, r=98, eta=256,
              pk=3573, ct=7082, dfr=-523),
}
GAMMA_LOG2 = 2   # Table 3 note: "The DFR is computed by Theorem 3.1 with gamma_q = 4"
WINDOW = 3       # key weights searched from the submitted value to this many above
CAP = 1.75       # m*n searched up to this multiple of the submitted m*n
ALIGNED = False  # --aligned: (wr1, wr2) = (wx, wy) for each pair of key weights


def sizes(m, n):
    """Table 3 note: pks = ceil(mn/8) + 64, cts = 2*ceil(mn/8) + 64 bytes."""
    v = (m * n + 7) // 8
    return v + 64, 2 * v + 64


def dfr_log2(m, n, k, r):
    a = m - k - r + 1
    return GAMMA_LOG2 + a * (m + r - a - n)


def smallest_n(lvl, m, r):
    """Smallest admissible (n, k) for this m and decoding weight r, or None."""
    s = SETS[lvl]
    need = max(lvl, 2 * s["eta"]) + 64
    best = None
    for k in range(max(1, ceil(need / m)), s["k"] + 1):
        a = m - k - r + 1
        if a < 1:
            continue
        n = max(m, 2 * r + k, 2 * r + k - 1 + ceil((lvl + GAMMA_LOG2) / a))
        while gcd(m, n) != 1:
            n += 1
        assert dfr_log2(m, n, k, r) <= -lvl and 2 * r <= n - k and r <= m - k and m <= n
        if best is None or n < best[0]:
            best = (n, k)
    return best


def ordinary(ext, blk, w, omega):
    b = mm.search_two(1, ext, blk, (blk, blk), w, omega)
    return None if b is None else b[0]


def zd(ext, blk, w, omega):
    best = None
    for j in (1, 2):
        for a1 in range(0, blk - w[0] + 1):
            for a2 in range(0, blk - w[1] + 1):
                z = mm.zd_cost(1, ext, blk, (blk, blk), w, (a1, a2), (0, 0), j, omega)
                if z is not None and (best is None or z[0] < best):
                    best = z[0]
    return best


def at_least(x, lvl):
    return x is not None and x >= lvl


def transposed_key_ok(lvl, omega, m, n, w):
    """Ordinary MM and ZD-MM on two length-m blocks over F_(2^n)."""
    if not at_least(ordinary(n, m, w, omega), lvl):
        return False
    z = zd(n, m, w, omega)
    return z is None or z >= lvl


def m_cap(lvl, r):
    """Largest m with m*(2r+1) within CAP; n >= 2r + k >= 2r + 1."""
    s = SETS[lvl]
    return int(CAP * s["m"] * s["n"]) // (2 * r + 1)


def search_option(args):
    """Smallest m*n for one pair of key weights; None if none below the cap."""
    lvl, omega, wx, wy = args
    s = SETS[lvl]
    w = (wx, wy)
    wmsg = w if ALIGNED else (s["wr1"], s["wr2"])
    r = wx * wmsg[1] + wy * wmsg[0] + s["we"]
    mmax = m_cap(lvl, r)
    limit = int(CAP * s["m"] * s["n"])

    def ok(m):
        c = smallest_n(lvl, m, r)
        return c is not None and transposed_key_ok(lvl, omega, m, c[0], w)

    # Threshold in m by bisection: the transposed cost rises with m (at the
    # smallest admissible n) at every point evaluated; three values below the
    # threshold are re-checked.
    lo, hi, step = r + 2, r + 2, 4
    if hi > mmax:
        return (lvl, omega, wx, wy, r, None)
    while not ok(hi):
        lo = hi + 1
        if hi >= mmax:
            return (lvl, omega, wx, wy, r, None)
        hi = min(hi + step, mmax)
        step *= 2
    while lo < hi:
        mid = (lo + hi) // 2
        if ok(mid):
            hi = mid
        else:
            lo = mid + 1
    mt = lo
    monotone = not any(ok(m) for m in range(max(r + 2, mt - 3), mt))

    cands = []
    for m in range(mt, mt + 60):
        c = smallest_n(lvl, m, r)
        if c is None:
            continue
        n0, k = c
        for n in range(n0, n0 + 30):
            if gcd(m, n) == 1 and dfr_log2(m, n, k, r) <= -lvl:
                cands.append((m * n, m, n, k))
    for prod, m, n, k in sorted(cands):
        if prod > limit:
            break
        # Every condition must hold; the ordinary-MM searches are cheap and come first.
        key = ordinary(n, m, w, omega)            # key relation, transposed
        if not at_least(key, lvl):
            continue
        spec_key = ordinary(m, n, w, omega)       # specification's instance (Table 6, Attack-2n)
        if not at_least(spec_key, lvl):
            continue
        msg = key if wmsg == w else ordinary(n, m, wmsg, omega)   # u = r1 + h*r2, transposed
        if not at_least(msg, lvl):
            continue
        key_zd = zd(n, m, w, omega)
        if key_zd is not None and key_zd < lvl:
            continue
        msg_zd = key_zd if wmsg == w else zd(n, m, wmsg, omega)
        if msg_zd is not None and msg_zd < lvl:
            continue
        return (lvl, omega, wx, wy, r,
                dict(m=m, n=n, k=k, bits=prod, mt=mt, monotone=monotone,
                     key=key, key_zd=key_zd, spec_key=spec_key, msg=msg, msg_zd=msg_zd))
    return (lvl, omega, wx, wy, r, None)


def search_level(pool, lvl, omega):
    s = SETS[lvl]
    opts = [(wx, wy) for wx in range(s["wx"], s["wx"] + WINDOW + 1)
            for wy in range(s["wy"], s["wy"] + WINDOW + 1)]
    best = None
    rows = pool.map(search_option, [(lvl, omega, wx, wy) for wx, wy in opts], 1)
    for res in rows:
        found = res[5]
        if found and (best is None or found["bits"] < best["bits"]):
            best = dict(found, wx=res[2], wy=res[3], r=res[4])
    return best, rows


def fmt(x):
    return "none" if x is None else f"{x:.1f}"


def kar(size):
    """Field multiplications in rbc_kar_mul for two polynomials of `size` coefficients."""
    if size == 1:
        return 1
    if size == 2:
        return 3
    h = (size + 1) // 2
    return 2 * kar(h) + kar(size - h)


def field_mul_words(m):
    """Word operations in the reference rbc_elt_ur_mul: (63 + m) * (ceil(m/64) + 1)."""
    return (63 + m) * ((m + 63) // 64 + 1)


def work(m, n, k):
    """Field multiplications per operation, times word operations per multiplication."""
    ring = kar(n)
    decoder = 2 * (n - k) ** 2 + (n - k) * (m - 2)
    f = field_mul_words(m)
    return {"KGen": ring * f, "Encaps": (2 * ring + k * n) * f,
            "Decaps": (3 * ring + k * n + decoder) * f}


def controls():
    print("Controls at the submitted parameters (Table 2 and Table 3, physical p. 16)")
    good = True
    for lvl, s in SETS.items():
        m, n, k = s["m"], s["n"], s["k"]
        r = s["wx"] * s["wr2"] + s["wy"] * s["wr1"] + s["we"]
        pk, ct = sizes(m, n)
        d = dfr_log2(m, n, k, r)
        c = smallest_n(lvl, m, r)
        w, wmsg = (s["wx"], s["wy"]), (s["wr1"], s["wr2"])
        row = (ordinary(m, n, w, 2.0), ordinary(n, m, w, 2.0), zd(n, m, w, 2.0),
               ordinary(n, m, wmsg, 2.0), zd(n, m, wmsg, 2.0))
        fine = (pk, ct, d, r) == (s["pk"], s["ct"], s["dfr"], s["r"]) and c[0] <= n
        good &= fine
        wk = work(m, n, k)
        print(f"  BRA-{lvl}: mn = {m * n} bits; pk {pk}, ct {ct} bytes; r = {r}; DFR 2^{d}; "
              f"smallest admissible n at this m and k <= {k}: {c[0]} (k = {c[1]}): "
              f"{'ok' if fine else 'MISMATCH'}")
        print(f"    MM at omega 2: specification's instance {fmt(row[0])}; transposed key "
              f"{fmt(row[1])}, ZD-MM {fmt(row[2])}; transposed u = r1 + h*r2 {fmt(row[3])}, "
              f"ZD-MM {fmt(row[4])}")
        print(f"    work model: ring product {kar(n)} field multiplications, field multiplication "
              f"{field_mul_words(m)} word operations; Decaps/Encaps = "
              f"{wk['Decaps'] / wk['Encaps']:.1f}, Encaps/KGen = {wk['Encaps'] / wk['KGen']:.1f}")
    return good


def report(lvl, omega, best, rows):
    s = SETS[lvl]
    print(f"BRA-{lvl}, omega = {omega:g}: smallest m*n per pair of key weights (wx, wy)")
    for _, _, wx, wy, r, found in sorted(rows, key=lambda x: (x[5] is None, x[5]["bits"] if x[5] else 0, x[2], x[3])):
        if found is None:
            print(f"  ({wx}, {wy}) r = {r}: none with mn up to {100 * (CAP - 1):+.0f}%")
            continue
        print(f"  ({wx}, {wy}) r = {r}: m = {found['m']}, n = {found['n']}, k = {found['k']}, "
              f"mn = {found['bits']} ({100 * (found['bits'] / (s['m'] * s['n']) - 1):+.1f}%); "
              f"threshold m = {found['mt']}"
              f"{'' if found['monotone'] else ' (NOT monotone below the threshold)'}")
    if best is None:
        print("  no admissible parameters found")
        return
    m, n, k = best["m"], best["n"], best["k"]
    pk, ct = sizes(m, n)
    wr = (best["wx"], best["wy"]) if ALIGNED else (s["wr1"], s["wr2"])
    print(f"  smallest: (wx, wy) = ({best['wx']}, {best['wy']}), (wr1, wr2) = ({wr[0]}, {wr[1]}), "
          f"we = {s['we']}, m = {m}, n = {n}, k = {k}, "
          f"r = {best['r']}, DFR 2^{dfr_log2(m, n, k, best['r'])}")
    print(f"    transposed key MM {fmt(best['key'])}, ZD-MM {fmt(best['key_zd'])}; specification's "
          f"instance {fmt(best['spec_key'])}; transposed u = r1 + h*r2 MM {fmt(best['msg'])}, "
          f"ZD-MM {fmt(best['msg_zd'])}; target {lvl}")
    print(f"    pk {s['pk']} -> {pk} bytes ({100 * (pk / s['pk'] - 1):+.1f}%); "
          f"ct {s['ct']} -> {ct} bytes ({100 * (ct / s['ct'] - 1):+.1f}%)")
    w0, w1 = work(s["m"], s["n"], s["k"]), work(m, n, k)
    print(f"    work model: ring product {kar(n)} field multiplications, field multiplication "
          f"{field_mul_words(m)} word operations")
    print("    work (field multiplications x word operations of the reference multiplier): "
          + "; ".join(f"{op} {100 * (w1[op] / w0[op] - 1):+.0f}%" for op in w0))
    f0, f1 = field_mul_words(s["m"]), field_mul_words(m)
    print("    field multiplications only: "
          + "; ".join(f"{op} {100 * (w1[op] * f0 / (w0[op] * f1) - 1):+.0f}%" for op in w0))


def main():
    global ALIGNED
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--controls", action="store_true", help="print the controls and stop")
    ap.add_argument("--jobs", type=int, default=min(16, os.cpu_count() or 1))
    ap.add_argument("--runs", default="128:2,256:2,512:2,256:3,512:3",
                    help="comma-separated level:omega pairs")
    ap.add_argument("--we", type=int, help="weight of e to use instead of the submitted 8 (floor)")
    ap.add_argument("--wr1", type=int, help="weight of r1 to use instead of the submitted one")
    ap.add_argument("--wr2", type=int, help="weight of r2 to use instead of the submitted one")
    ap.add_argument("--aligned", action="store_true",
                    help="set (wr1, wr2) = (wx, wy) for each pair of key weights searched")
    args = ap.parse_args()
    if args.aligned and (args.wr1 is not None or args.wr2 is not None):
        ap.error("--aligned sets wr1 and wr2; do not combine it with --wr1 or --wr2")
    good = controls()
    if args.controls:
        return 0 if good else 1
    for name in ("we", "wr1", "wr2"):
        value = getattr(args, name)
        if value is not None:
            for s in SETS.values():
                s[name] = value
            print(f"Changed weight: {name} = {value} in every set; "
                  f"the three-block ciphertext instance is not modeled")
    if args.aligned:
        ALIGNED = True
        print("Ciphertext weights: (wr1, wr2) = (wx, wy) for each pair of key weights; "
              "the three-block ciphertext instance is not modeled")
    runs = [(int(a), float(b)) for a, b in (x.split(":") for x in args.runs.split(","))]
    with get_context("fork").Pool(args.jobs) as pool:
        for lvl, omega in runs:
            best, rows = search_level(pool, lvl, omega)
            report(lvl, omega, best, rows)
            sys.stdout.flush()
    return 0 if good else 1


if __name__ == "__main__":
    sys.exit(main())
