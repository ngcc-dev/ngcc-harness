#!/usr/bin/env python3
"""Counts for sign-19/fixing_cost.md (sign-19-4, sign-19-8). Standard library only.

For the eight Phoenix-SM3 sets named in the two findings this prints
  - the signature size from the reference headers, against Table 4 (p. 40);
  - an estimate of the expected number of H_msg trials per signature (the
    TFORS forced-pruning search of Algorithm 18), by sampling index sets;
  - SM3 compression calls in signing and verification of a 32-byte message
    (the length used by the submitted benchmark, test/benchmark.c:19), split
    into the message-hash part and the rest;
  - the change when the 32-byte SM3 seed inside H_msg is replaced by a wider
    one, under two cost conventions.
It reads parameter headers and counts. It implements no part of the scheme.

Run: python3 -B sign-19/fixing_cost_msg_hash.py        (about three minutes)
"""

import random
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
REF = HERE / "Implementations and Test_Vectors" / "Implementations" / "Reference_Implementation"
MLEN = 32                      # SPX_MLEN, test/benchmark.c:19

# set: stated signature bytes (Table 4, p. 40), seed bits used as a costing
# aid (see the note), number of sampled index sets, stated reference signing
# and verification cycles (Table 5, p. 42; used only as a control of the counts).
SETS = {
    "192s": (13356, 512, 2000000, 6851044896, 23704145),
    "192f": (30766, 512, 200000, 365448916, 9402944),
    "256s": (24618, 512, 2000000, 6393668360, 40387335),
    "256f": (44906, 512, 200000, 686132265, 17807532),
    "384s": (54726, 768, 2000000, 10140945512, 64223446),
    "384f": (88442, 768, 200000, 2212502840, 40544782),
    "512s": (98476, 1024, 2000000, 15319889130, 25916097),
    "512f": (138454, 1024, 2000000, 3638891876, 80390468),
}


def header(name):
    text = (REF / ("Phoenix-SM3-" + name) / "params" / ("params-phoenix-sm3-%s.h" % name)).read_text()
    def macro(key):
        return int(re.search(r"^#define\s+%s\s+(\d+)" % key, text, re.MULTILINE).group(1))
    keys = ["SPX_N", "SPX_FULL_HEIGHT", "SPX_D", "SPX_TFORS_A", "SPX_TFORS_K_PRIME", "SPX_TFORS_K",
            "SPX_TFORS_SIG_MAX", "SPX_GWOTS_W1", "SPX_GWOTS_W1_LEN", "SPX_GWOTS_W2",
            "SPX_GWOTS_W2_LEN", "SPX_GWOTS_CHECKSUM_W"]
    return {k: macro(k) for k in keys}


def blocks(nbytes):
    """SM3 compression calls for nbytes of input: 64-byte blocks, 9 bytes of padding."""
    return (nbytes + 9 + 63) // 64


def trials(p, samples, seed=20261123):
    """Estimate 1/tau: index sets tried until (k + authCount) * n <= TFORS_SIG_MAX.

    Index sets follow tfors.c:55-73: k distinct set numbers out of k', each with
    a uniform a-bit position, leaf = set + k' * position. For sorted distinct
    leaves x the Octopus count of octopus.c:24-50 equals
    H + 2 - 2k + sum(bit_length(x[i] ^ x[i+1])), H the tree height.
    """
    n, a, kp, k = p["SPX_N"], p["SPX_TFORS_A"], p["SPX_TFORS_K_PRIME"], p["SPX_TFORS_K"]
    height = a + kp.bit_length() - 1
    auth_max = p["SPX_TFORS_SIG_MAX"] // n - k
    limit = auth_max - (height + 2 - 2 * k)
    rng = random.Random(seed)
    sample, bits, sets = rng.sample, rng.getrandbits, range(kp)
    accepted = 0
    for _ in range(samples):
        x = sorted([s + kp * bits(a) for s in sample(sets, k)])
        total, prev = 0, x[0]
        for v in x[1:]:
            total += (prev ^ v).bit_length()
            prev = v
        if total <= limit:
            accepted += 1
    return samples / accepted, accepted, auth_max, height


def main():
    for name, (stated, seed_bits, samples, sign_cycles, verify_cycles) in SETS.items():
        p = header(name)
        n, h, d = p["SPX_N"], p["SPX_FULL_HEIGHT"], p["SPX_D"]
        a, kp, k = p["SPX_TFORS_A"], p["SPX_TFORS_K_PRIME"], p["SPX_TFORS_K"]
        hp = h // d
        w1, l1, w2, l2, wc = (p["SPX_GWOTS_W1"], p["SPX_GWOTS_W1_LEN"], p["SPX_GWOTS_W2"],
                              p["SPX_GWOTS_W2_LEN"], p["SPX_GWOTS_CHECKSUM_W"])
        length = l1 + l2 + 1
        steps = l1 * (w1 - 1) + l2 * (w2 - 1) + (wc - 1)
        size = n + 4 + 2 + p["SPX_TFORS_SIG_MAX"] + d * (length * n + 4) + h * n   # SPX_BYTES

        t_exp, accepted, auth_max, height = trials(p, samples)

        # Tweakable hashes continue from a state that holds PK.seed
        # (thash_sm3_simple.c:13-28, hash_sm3.c:21-38): address (32) || input.
        f1, h2n, tl, prf = blocks(32 + n), blocks(32 + 2 * n), blocks(32 + length * n), blocks(32 + n)

        # H_msg and H2 per trial (hash_sm3.c:106-235).
        inner = blocks(3 * n + MLEN + 4)                       # SM3(R || PK || M || ctr)
        digest = (a * k + 7) // 8 + n + (hp * (d - 1) + 7) // 8 + (hp + 7) // 8
        out_blocks = (digest + 31) // 32                       # 32 bytes per pseudoXOF call
        expand = out_blocks * blocks(2 * n + 32 + 4)           # R || PK.seed || seed || ct
        h2 = 16 * blocks(2 * n + 4)                            # 512-byte pool
        per_trial = inner + expand + h2

        leaves = 1 << height                                   # octopus.c:121-172
        tfors = leaves * (prf + f1) + (leaves - 1) * h2n
        layer = (1 << hp) * (length * prf + steps * f1 + tl) + ((1 << hp) - 1) * h2n
        rest_sign = tfors + d * layer                          # gwotsx1.c:40-80, sign.c:212-226
        sign = rest_sign + t_exp * per_trial
        verify = (k * f1 + (k + auth_max - 1) * h2n
                  + d * ((steps // 2) * f1 + tl + hp * h2n) + per_trial)

        seed_bytes = seed_bits // 8
        grow = out_blocks * (blocks(2 * n + seed_bytes + 4) - blocks(2 * n + 32 + 4))

        print("Phoenix-SM3-%s" % name)
        print("  signature bytes from the reference header: %d (Table 4: %d)" % (size, stated))
        print("  H_msg trials per signature: about %.0f (%d of %d sampled index sets "
              "have at most %d authentication nodes)" % (t_exp, accepted, samples, auth_max))
        print("  compressions per trial: seed %d + expansion %d + H2 %d = %d"
              % (inner, expand, h2, per_trial))
        print("  signing: about %.2fM compressions, %.1f%% in the trials, %.2f%% in the seed hash"
              % (sign / 1e6, 100 * t_exp * per_trial / sign, 100 * t_exp * inner / sign))
        print("  verification: about %d compressions, %.2f%% in the seed hash"
              % (verify, 100 * inner / verify))
        print("  control, Table 5 cycles per counted compression: signing %.0f, verification %.0f"
              % (sign_cycles / sign, verify_cycles / verify))
        print("  %d-bit seed, counted by calls and bytes (same message bytes, wider expansion "
              "input): +%d per trial; signing +%.2f%%, verification +%.2f%%"
              % (seed_bits, grow, 100 * t_exp * grow / sign, 100 * grow / verify))
        for passes in sorted({2, seed_bits // 256}):
            delta = grow + inner * (passes - 1)
            at_sign = 6.4 * sign / (t_exp * (passes - 1.1) - 0.1)       # bytes
            at_verify = 6.4 * verify / (passes - 1.1)
            print("  %d-bit seed at %d SM3 passes over the message: +%d per trial; signing +%.2f%%, "
                  "verification +%.2f%%; on longer messages the added hashing reaches 10%% at "
                  "about %.1f kB (signing) and %.0f kB (verification)"
                  % (seed_bits, passes, delta, 100 * t_exp * delta / sign, 100 * delta / verify,
                     at_sign / 1000, at_verify / 1000))


if __name__ == "__main__":
    main()
