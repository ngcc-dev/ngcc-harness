#!/usr/bin/env python3
"""Fixing-cost counts for sign-29 (Tins).  Standard library only.

Prints (1) the signature-size control from the specification's formula and
(2) the SM3 compression calls that the submitted reference code makes per
signature and per verification, before and after the two costed repairs:

  sign-29-7  ChildNodeGen also absorbs the lambda-bit parent seed;
  sign-29-1  every leaf expands a k-bit delta instead of a mu-bit delta.

It counts calls and bytes only.  It implements no part of the scheme and
measures nothing.

Run:  python3 -B sign-29/fixing_cost_hash_work.py
"""

from math import ceil

# Specification Table 2 (physical p. 17): lambda, k, n, mu, N, tau, Topen, omega.
SETS = {
    "Tins-128": dict(lam=128, k=276, n=140, mu=12, N=4096, tau=12, topen=125, om=6),
    "Tins-256": dict(lam=256, k=528, n=268, mu=12, N=4096, tau=24, topen=255, om=4),
    "Tins-512": dict(lam=512, k=1044, n=524, mu=12, N=4096, tau=47, topen=510, om=6),
}
TABLE5_SIG = {"Tins-128": 3284, "Tins-256": 13012, "Tins-512": 51187}

# Grinding acceptance of the submitted code, from report sign-29-8, and the
# specification's "approximately 0.2" chance that an opening fits Topen.
GRIND_TRIES = {"Tins-128": 267 / 0.2, "Tins-256": 257 / 0.2, "Tins-512": 65.5 / 0.2}

MLEN = 32  # message bytes assumed in the one Hash2 call; the term is negligible


def sm3(bits):
    """Compression calls of sm3_bit() (auxfunc.c:122, drng.c:128)."""
    return bits // 512 + (1 if bits % 512 <= 447 else 2)


def xof(out_bits, in_bits):
    """pseudoXOF (auxfunc.c:482): one SM3 of msg||ct per 256 output bits."""
    return ceil(out_bits / 256) * sm3(in_bits + 32)


def hmac(bits):
    """hmac() (auxfunc.c:187): inner hash over a 512-bit key block, then outer."""
    return sm3(512 + bits) + sm3(512 + 256)


def hash2l(lam, bits):
    """Hash with 2*lambda output bits as the reference code instantiates it."""
    if lam == 128:                      # sm3hash(256, ...)
        return sm3(bits)
    if lam == 256:                      # pseudohash_512 (auxfunc.c:224)
        return hmac(16 + bits) + sm3(16 + bits) + sm3(512)
    if lam == 512:                      # pseudohash_1024 (auxfunc.c:353)
        return (hmac(16 + bits) + sm3(16 + bits) + hmac(256) + sm3(512)
                + hmac(256) + sm3(512) + hmac(256) + sm3(768))
    raise ValueError(lam)


def drng_init(nonce_bytes):
    """SM3_DRNG_Instantiate (drng.c:240): two SM3_df calls, two hashes each."""
    return 2 * sm3((5 + nonce_bytes) * 8) + 2 * sm3(61 * 8)


def drng_gen(bits):
    """SM3_DRNG_Generate (drng.c:268): one hash per 256 bits, one for the update."""
    return ceil(bits / 256) * sm3(55 * 8) + sm3(56 * 8)


def work(p, name, fix7=False, fix1=False):
    """SM3 compression calls of (signing, verification), and the per-leaf parts."""
    lam, k, n, mu, N, tau, om = (p[x] for x in ("lam", "k", "n", "mu", "N", "tau", "om"))
    leaves = tau * N
    node, salt, com = lam // 8, lam // 4, lam // 4

    child = xof(lam, 2 * lam if fix7 else lam)          # bavc_commit.c:121,123
    commit = hash2l(lam, (salt + node + 5) * 8)         # bavc_commit.c:151
    hcom = hash2l(lam, leaves * com * 8 + 8)            # bavc_commit.c:158
    delta_bits = k if fix1 else mu
    exp_init = drng_init(salt + node)                   # bavc_commit.c:342
    exp_gen = drng_gen(delta_bits + 2 * n - 4)          # bavc_commit.c:357
    hsh = hash2l(lam, (1 + salt + com) * 8 + 2 * (n - 2) * tau)
    hpiop = hash2l(lam, (1 + node + salt + MLEN + com) * 8 + 2 * k * tau + k)
    chal = drng_init(salt + 8) + drng_gen(tau * mu + om)
    keyexp_pk = drng_init(node) + drng_gen(n * k) + drng_gen((n - 1) * k)
    keyexp_sk = drng_init(node) + 2 * drng_gen(n - 2)

    sign = (keyexp_pk + keyexp_sk + drng_gen(2 * lam) + drng_gen(lam)
            + 2 * (leaves - 1) * child + leaves * commit + hcom
            + leaves * (exp_init + exp_gen) + hsh + hpiop
            + GRIND_TRIES[name] * chal)
    # The verifier rebuilds every subtree it is given: at most all internal nodes.
    verify = (keyexp_pk + chal + 2 * (leaves - 1) * child
              + tau * (N - 1) * commit + hcom
              + leaves * exp_init + tau * (N - 1) * exp_gen + hsh + hpiop)
    per_leaf = dict(tree=2 * child, commit=commit, hcom=hcom / leaves,
                    expand=exp_init + exp_gen)
    return sign, verify, per_leaf


def leaf_bytes(p, fix7=False, fix1=False):
    """Bytes absorbed plus bytes produced per leaf by the four per-leaf calls."""
    lam, k, n, mu = p["lam"], p["k"], p["n"], p["mu"]
    node, salt, com = lam // 8, lam // 4, lam // 4
    tree = 2 * ((2 * node if fix7 else node) + node)
    commit = (salt + node + 5) + com
    hcom = com
    expand = (salt + node) + ceil(((k if fix1 else mu) + 2 * n - 4) / 8)
    return tree + commit + hcom + expand


def main():
    print("1. Signature size, specification formula (physical p. 16):")
    print("   |sigma| = 2L + 64 + 2L + (L*Topen + tau*2L) + tau*(2n - 4 + k) bits")
    for name, p in SETS.items():
        lam, k, n, tau, topen = p["lam"], p["k"], p["n"], p["tau"], p["topen"]
        bits = 2 * lam + 64 + 2 * lam + (lam * topen + tau * 2 * lam) + tau * (2 * n - 4 + k)
        print("   %s: %d bits = %d bytes; Table 5 states %d; p_mid already takes "
              "tau*k = %d bits" % (name, bits, ceil(bits / 8), TABLE5_SIG[name], tau * k))
    print("   Neither repair adds a transmitted value: delta is derived from the")
    print("   leaf seeds and is never sent; aux holds only alpha and beta.\n")

    print("2. SM3 compression calls of the submitted reference code")
    for name, p in SETS.items():
        s0, v0, leaf0 = work(p, name)
        s7, v7, leaf7 = work(p, name, fix7=True)
        s1, v1, leaf1 = work(p, name, fix1=True)
        sb, vb, leafb = work(p, name, fix7=True, fix1=True)
        leaves = p["tau"] * p["N"]
        print("   %s: %d leaves" % (name, leaves))
        print("     per leaf, submitted : tree %d, commit %d, h_com %.2f, expand %d"
              "  (sum %.2f)" % (leaf0["tree"], leaf0["commit"], leaf0["hcom"],
                               leaf0["expand"], sum(leaf0.values())))
        print("     per leaf, repaired  : tree %d, commit %d, h_com %.2f, expand %d"
              "  (sum %.2f)" % (leafb["tree"], leafb["commit"], leafb["hcom"],
                               leafb["expand"], sum(leafb.values())))
        print("     signing      submitted %10.0f" % s0)
        print("       sign-29-7 only %+6.1f%%   sign-29-1 only %+6.1f%%   both %+6.1f%%"
              % (100 * (s7 / s0 - 1), 100 * (s1 / s0 - 1), 100 * (sb / s0 - 1)))
        print("     verification submitted %10.0f" % v0)
        print("       sign-29-7 only %+6.1f%%   sign-29-1 only %+6.1f%%   both %+6.1f%%"
              % (100 * (v7 / v0 - 1), 100 * (v1 / v0 - 1), 100 * (vb / v0 - 1)))
        print("     grinding share of submitted signing: %.1f%%"
              % (100 * GRIND_TRIES[name]
                 * (drng_init(p["lam"] // 4 + 8) + drng_gen(p["tau"] * p["mu"] + p["om"])) / s0))
    print()

    print("3. Bytes absorbed plus produced per leaf (tree, commit, h_com, expand)")
    for name, p in SETS.items():
        b0 = leaf_bytes(p)
        b7 = leaf_bytes(p, fix7=True)
        b1 = leaf_bytes(p, fix1=True)
        bb = leaf_bytes(p, fix7=True, fix1=True)
        print("   %s: submitted %d; sign-29-7 only %+.1f%%; sign-29-1 only %+.1f%%; "
              "both %+.1f%%" % (name, b0, 100 * (b7 / b0 - 1), 100 * (b1 / b0 - 1),
                                100 * (bb / b0 - 1)))
    print()

    print("4. PRG output per leaf and subfield multiplications per leaf (sign-29-1)")
    for name, p in SETS.items():
        k, n, mu = p["k"], p["n"], p["mu"]
        print("   %s: PRG output %d -> %d bits; GF(2^%d) multiplications for "
              "phi(i)*delta_rnd 1 -> %d" % (name, mu + 2 * n - 4, k + 2 * n - 4, mu, k // mu))


if __name__ == "__main__":
    main()
