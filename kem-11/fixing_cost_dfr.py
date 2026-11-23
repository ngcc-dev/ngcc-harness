#!/usr/bin/env python3
"""Fixing-cost control for kem-11-1 (COMPASS-KEM-384 and -512).

Recomputes the decryption failure rate from the model of specification
section 3.3 (physical p. 19) and prints the union bound over 256 and over n
message coefficients beside Table 3.  It shows which coefficient count the
stated bound corresponds to.  It prints numbers only and implements no scheme.
"""
import math

def comp(x, d):                      # Comp_d, specification 2.3.3, physical p. 11
    if x > 0:
        x -= 1
    return x >> d

def decomp(y, d):                    # Decomp_d, physical p. 12
    return (y << d) + (1 << (d - 1))

def err_dist(q, d):                  # Decomp_d(Comp_d(x)) - x for uniform x
    dist = {}
    for x in range(q):
        e = decomp(comp(x, d), d) - x
        dist[e] = dist.get(e, 0) + 1
    return {e: c / q for e, c in dist.items()}

def variance(dist):
    mean = sum(e * p for e, p in dist.items())
    return sum((e - mean) ** 2 * p for e, p in dist.items())

def upper_tail(t, sigma):            # Pr[N(0, sigma^2) > t]
    return 0.5 * math.erfc(t / (sigma * math.sqrt(2)))

# name: (n, q, k, l, eta, (d, d1, d2), Table 3 log2 DFR)   Table 1, physical p. 18
SETS = {
    "COMPASS-KEM-128": (256, 3329, 2, 2, 3, (2, 2, 8), -189.27),
    "COMPASS-KEM-256": (256, 3329, 4, 4, 2, (2, 2, 6), -181.13),
    "COMPASS-KEM-384": (512, 7681, 3, 3, 3, (2, 2, 8), -404.51),
    "COMPASS-KEM-512": (512, 7681, 4, 4, 4, (2, 2, 6), -250.31),
}

for name, (n, q, k, l, eta, (d, d1, d2), stated) in SETS.items():
    var_s = eta / 2.0                # centered binomial variance
    var_t = variance(err_dist(q, d))
    var_1 = variance(err_dist(q, d1))
    e2 = err_dist(q, d2)
    sigma = math.sqrt(n * (k * var_t * var_s + l * var_1 * var_s))
    bound = (q - 1) / 4.0
    p = sum(pr * (upper_tail(bound - e, sigma) + upper_tail(bound + e, sigma))
            for e, pr in e2.items())
    per = math.log2(p)
    print(f"{name}: per coefficient 2^{per:.2f}; "
          f"x256 2^{per + 8:.2f}; x{n} 2^{per + math.log2(n):.2f}; "
          f"Table 3 2^{stated:.2f}")
