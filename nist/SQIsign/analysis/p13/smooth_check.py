#!/usr/bin/env python3
"""Compare B-smoothness frequency of degrees of cyclic isogenies E -> E^(p)
(from conj_lattice.sage output) with uniform random integers having the same
size distribution. Run with the Sage python (needs cypari2).
usage: python -I smooth_check.py <jsonl> <max_numbers> <seed>"""
import json, random, sys, math
import cypari2
pari = cypari2.Pari()

def largest_pf(n):
    f = pari.factor(n)
    return int(f[0][len(f[0]) - 1]) if n > 1 else 1

def main():
    fn, mx, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    rnd = random.Random(seed)
    degs = [d for l in open(fn) for d in json.loads(l)["degs"]]
    rnd.shuffle(degs)
    degs = degs[:mx]
    # uniform comparison: for each degree d draw u uniform in d*(1 +- 1%) -> same size profile
    unif = [d + rnd.randint(-(d // 100), d // 100) for d in degs]
    lp_d = [largest_pf(d) for d in degs]
    lp_u = [largest_pf(u) for u in unif]
    out = {"n": len(degs), "mean_log2_deg": sum(math.log2(d) for d in degs) / len(degs)}
    rows = []
    for lb in range(4, 26, 2):
        B = 2 ** lb
        fd = sum(1 for x in lp_d if x <= B) / len(degs)
        fu = sum(1 for x in lp_u if x <= B) / len(degs)
        rows.append({"log2B": lb, "frac_lattice": fd, "frac_uniform": fu,
                     "ratio": (fd / fu) if fu else None,
                     "se_ratio": (math.sqrt(1 / max(1, fd * len(degs)) + 1 / max(1, fu * len(degs))))})
    out["rows"] = rows
    print(json.dumps(out, indent=1))

if __name__ == "__main__":
    main()
