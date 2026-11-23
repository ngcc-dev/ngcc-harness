#!/usr/bin/env python3
"""Fixing-cost count for sign-22-1 in the Rhyme-SHAKE profile.

Counts Keccak-f[1600] permutations in verification and signing of the
submitted pure-SHAKE build (Makefile: -DRHYME_NO_AES), and again when
mu = H_gen(pk || M) is computed by a sponge of capacity 2*lambda (rate
104 bytes at 384, 72 bytes at 512) with a 2*lambda-bit output.  An
operation-count model then gives the share of the permutations in
verification.  It prints counts and estimates only.  Nothing is measured
and no scheme is implemented.

Call lists follow src/sign.c, src/sampler.c, src/poly.c and src/fips202.c of
Reference_Implementation/Rhyme-SHAKE/Rhyme-SHAKE-512 (the per-set trees are
identical; the set is chosen by RHYME_MODE).
"""

from math import ceil, log2

SHAKE128, SHAKE256 = 168, 136
KECCAK_OPS = 24 * 156   # word operations per permutation (24 rounds)
MLEN = 32               # short message, bytes

# level: n, q, K, L, d, D, tau, bits(q), bits(2q), repetitions (Table 3, p. 53)
SETS = {
    256: (512, 9473, 2, 3, 4, 10, 58, 14, 15, 2.99),
    384: (512, 11777, 3, 4, 6, 14, 113, 14, 15, 2.95),
    512: (1024, 18433, 2, 3, 4, 10, 115, 15, 16, 2.69),
}


def perms(inlen, outlen, rate):
    # fips202.c:359-376 permutes once per filled block while absorbing;
    # fips202.c:410-426 permutes once per block squeezed.
    return inlen // rate + ceil(outlen / rate)


def count(level, wide):
    n, q, K, L, d, D, tau, qb, q2b, reps = SETS[level]
    pk = 32 + K * ((n * qb + 7) // 8)            # params.h:80,89
    w_bytes = K * ((n * q2b + 7) // 8)           # packing.h:41
    ctilde = level // 4                          # params.h:12
    if wide:
        rate_mu, mu = (1600 - 2 * level) // 8, level // 4
    else:
        rate_mu, mu = SHAKE256, 64               # params.h:11
    # expand_A: 2 bytes per try, accepted with probability q / 2^bits
    # (poly.c:227-242); at least 4 blocks (poly.c:260).
    tries = n / (q / 2 ** qb)
    expand = K * (L - 1) * max(4, ceil(2 * tries / SHAKE128))
    p_mu = perms(pk + MLEN, mu, rate_mu)         # sign.c:311-315, 508-512
    p_ct = perms(w_bytes + mu, ctilde, SHAKE256)  # sign.c:369-373, 521-525
    mask = n if n in (512, 1024) else 256        # sampler.c:233-239
    ch_tries = sum(mask / (i + 1) for i in range(n - tau, n))
    p_ch = ceil(2 * ch_tries / SHAKE256)         # sampler.c:227-240
    verify = expand + p_mu + p_ct + p_ch
    # Signing, per iteration: y1, D-1 polynomials X', d polynomials e and the
    # Sample_Z stream.  A sample takes 8 bytes and one sign byte when it is
    # not zero (sampler.c:126-144); Sample_Z takes 8 bytes (sampler.c:367).
    stream = (9.0 + (D - 1) * 8.7 + d * 8.85 + 8.0) * n
    per_iter = ceil(stream / SHAKE256) + p_ct + p_ch
    sign = expand + p_mu + perms(32 + mu, 64, SHAKE256) + reps * per_iter
    return verify, sign, p_mu, p_ct, expand, p_ch


def sm3_as_built(level, wide):
    """SM3 compression calls of Rhyme-SM3 verification as built.

    The SM3 backend is KDF-SM3: every 32 output bytes cost one SM3 hash of
    tag || input || counter (sm3_xof.c:62-104).  The ruleset counts calls and
    bytes for an ideal XOF instead, so this figure is context only.
    """
    n, q, K, L, d, D, tau, qb, q2b, reps = SETS[level]
    pk = 32 + K * ((n * qb + 7) // 8)
    w_bytes = K * ((n * q2b + 7) // 8)
    ctilde = level // 4
    mu = level // 4 if wide else 64

    def sm3(nbytes):                    # compression calls of one SM3 hash
        return (nbytes + 9 + 63) // 64

    def kdf(inlen, outlen):
        return ceil(outlen / 32) * sm3(1 + inlen + 4)

    tries = n / (q / 2 ** qb)
    blocks = max(4, ceil(2 * tries / SHAKE128))
    mask = n if n in (512, 1024) else 256
    ch_tries = sum(mask / (i + 1) for i in range(n - tau, n))
    return (K * (L - 1) * kdf(34, blocks * SHAKE128)
            + kdf(pk + MLEN, mu)
            + kdf(w_bytes + mu, ctilde)
            + kdf(ctilde, ceil(2 * ch_tries / SHAKE256) * SHAKE256))


def other_ops(level, butterfly, basemul, decode):
    """Word operations of verification outside Keccak (model)."""
    n, q, K, L, d, D, tau, qb, q2b, reps = SETS[level]
    tries = n / (q / 2 ** qb)
    transforms = K + 1 + (L - 1) + K             # sign.c:46-69, 506
    butterflies = (n // 2) * int(log2(n // 4))   # ntt.c:50-66
    return ((decode + 7) * (1 + d) * n           # rANS decode, norm checks
            + 6 * K * n                          # unpack_pk
            + 5 * tries * K * (L - 1)            # rejection parse
            + (butterfly * butterflies + 2 * n) * transforms
            + basemul * n * K * L                # ntt.c:89-113
            + 25 * K * n)                        # sub, add, freeze, j-term, pack_w


def main():
    for level in sorted(SETS):
        v0, s0, mu0, ct0, exp0, ch0 = count(level, False)
        print(f"Rhyme-SHAKE-{level}: verification {v0} permutations "
              f"(expand_A {exp0}, mu {mu0}, c_tilde {ct0}, challenge {ch0}); "
              f"signing {s0:.0f}")
        lo = other_ops(level, 10, 14, 20)
        hi = other_ops(level, 16, 22, 40)
        h = v0 * KECCAK_OPS
        print(f"  model: Keccak {h} operations, other work {lo:.0f} to "
              f"{hi:.0f}; Keccak share {100 * h / (h + hi):.0f}% to "
              f"{100 * h / (h + lo):.0f}%")
        if level == 256:
            continue
        v1, s1, mu1, ct1, _, _ = count(level, True)
        add = (v1 - v0) * KECCAK_OPS
        print(f"  wide mu: mu {mu0} -> {mu1}, c_tilde {ct0} -> {ct1}; "
              f"verification {v0} -> {v1} permutations "
              f"(+{100 * (v1 - v0) / v0:.1f}%), signing {s0:.0f} -> {s1:.0f} "
              f"(+{100 * (s1 - s0) / s0:.2f}%)")
        print(f"  verification work +{100 * add / (h + hi):.1f}% to "
              f"+{100 * add / (h + lo):.1f}% (model)")
        c0, c1 = sm3_as_built(level, False), sm3_as_built(level, True)
        print(f"  context, Rhyme-SM3-{level} as built: verification {c0} -> "
              f"{c1} SM3 compression calls (+{100 * (c1 - c0) / c0:.1f}%) "
              f"if mu is widened through KDF-SM3; not counted")


if __name__ == "__main__":
    main()
