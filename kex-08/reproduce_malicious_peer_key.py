#!/usr/bin/env python3
"""Show that a crafted NIIKE-lv128 peer key forces the honest party's shared secret."""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from niike_witness_common import conjugate, derive, keygen, load   # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--victims", type=int, default=3)
    args = parser.parse_args()
    lib = load()
    # Attacker: predict the forced value once, using only its own throwaway key pair.
    pk_x, sk_x = keygen(lib, os.urandom(64))
    rc, predicted = derive(lib, sk_x, conjugate(pk_x))
    assert rc == 0
    honest_a, honest_b = keygen(lib, os.urandom(64)), keygen(lib, os.urandom(64))
    assert derive(lib, honest_a[1], honest_b[0]) == derive(lib, honest_b[1], honest_a[0])
    forced = zero = 0
    for _ in range(args.victims):
        pk_a, sk_a = keygen(lib, os.urandom(64))                 # fresh honest party
        rc, ss = derive(lib, sk_a, conjugate(pk_a))
        forced += rc == 0 and ss == predicted
        rc, ss = derive(lib, sk_a, bytes(len(pk_a)))
        zero += rc == 0 and ss == bytes(len(ss))
    print("CONTROL: two honest parties agree on a shared secret")
    print(f"CONFIRMED: {forced}/{args.victims} fresh honest parties given conj(own pk) derive the "
          f"attacker-predicted secret {predicted.hex()[:32]}...")
    print(f"CONFIRMED: {zero}/{args.victims} fresh honest parties given an all-zero peer key "
          "derive an all-zero secret with rc 0")
    assert forced == zero == args.victims


if __name__ == "__main__":
    main()
