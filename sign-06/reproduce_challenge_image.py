#!/usr/bin/env python3
"""Certify the COMPASS-SIG challenge-sampler width defect and bound its image.

Checks that every per-set poly.c copy carries the byte-wide position read and
the 64-bit sign word, then counts the challenges the delivered sampler can
still produce at the 384- and 512-bit sets (sign-06-5).
"""

from math import comb, log2
from pathlib import Path

ROOT = Path(__file__).resolve().parent / "Implementations"
COPIES = [
    ROOT / tier / f"COMPASS-SIG-{level}" / "poly.c"
    for tier in ("Reference_Implementation", "Optimized_Implementation")
    for level in (128, 256, 384, 512)
]

texts = [path.read_bytes() for path in COPIES]
assert all(text == texts[0] for text in texts), "per-set poly.c copies differ"
source = texts[0].decode()
body = source[source.index("void poly_challenge("):]
body = body[:body.index("\n}")]
for fragment in ("uint64_t signs;", "b = buf[pos++];", "} while(b > i);",
                 "c->coeffs[b] = 1 - 2*(signs & 1);", "signs >>= 1;"):
    assert fragment in body, fragment
print("delivered poly_challenge: byte-wide position, 64-bit sign word "
      f"(identical in {len(COPIES)} reference and optimized copies)")

for level, n, tau, ctilde_bits in ((384, 512, 78, 384), (512, 512, 120, 512)):
    reachable = 256 + tau            # positions 0..255 plus the loop range
    signs = sum(comb(tau, j) for j in range(65))   # at most 64 negatives
    image = comb(reachable, tau) * signs
    specified = comb(n, tau) * 2**tau
    cap = min(log2(specified), ctilde_bits)
    print(f"COMPASS-SIG-{level}: image <= 2^{log2(image):.1f} "
          f"(positions C({reachable},{tau}), signs 2^{log2(signs):.1f}); "
          f"Figure 1 gives 2^{log2(specified):.1f}, capped at 2^{cap:.0f} by the "
          f"{ctilde_bits // 8}-byte challenge hash; shortfall {level - log2(image):.1f} bits")
    if level == 384:
        assert 335.5 < log2(image) < 335.7
    else:
        assert 454.8 < log2(image) < 455.0

for level, n, tau in ((128, 256, 30), (256, 256, 60)):
    assert n - tau <= 256 and tau <= 64
    print(f"COMPASS-SIG-{level}: n={n}, tau={tau}: byte-wide positions cover "
          "every index and 64 sign bits suffice; unaffected")

print("CONFIRMED sign-06-5: the delivered sampler reaches at most 2^335.6 and "
      "2^454.9 challenges at the 384- and 512-bit sets")
print("LIMITATION sign-06-5: the challenge-guessing forgery is a counting bound "
      "and was not executed")
