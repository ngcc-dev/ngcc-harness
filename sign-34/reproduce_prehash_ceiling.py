#!/usr/bin/env python3
"""Check YuanYang.DSA's salt-after-256-bit-prehash message binding."""

from math import log, log2
from pathlib import Path

root = Path(__file__).resolve().parent / "Implementations"
trees = sorted(p.parent for p in root.rglob("common.c")
               if p.parent.name in {"yuanyang-512", "yuanyang-1024", "yuanyang-2048"})
assert len(trees) == 6, len(trees)
affected = []
collision = 128 + log2(2 * log(2)) / 2  # 50% birthday success
for tree in trees:
    common = (tree / "common.c").read_text()
    signer = (tree / "sign.c").read_text()
    verifier = (tree / "vrfy.c").read_text()
    assert "memcpy(buf, m, (size_t)m_len_bytes)" in common, tree
    assert "memcpy(buf + m_len_bytes, h, YUANYANG_D * sizeof h[0])" in common, tree
    assert "sm3hash(256, buf, 8u * total_len, seed)" in common, tree
    prehash = signer.find("yuanyang_hash_message_with_public_key(seed, m, m_len_bytes")
    salt = signer.find("prng_get_bytes(&rng, sn, YUANYANG_SALT_BYTES)")
    challenge = signer.find("yuanyang_hash_to_challenge(challenge, sn, seed)")
    assert 0 <= prehash < salt < challenge, tree
    assert verifier.find("yuanyang_hash_message_with_public_key(seed, m, m_len_bytes") < \
           verifier.find("yuanyang_hash_to_challenge(c, sn, seed)"), tree
    level = int(tree.name.rsplit("-", 1)[1])
    target = {512: 128, 1024: 256, 2048: 512}[level]
    if collision < target:
        affected.append((level, tree))
    print(f"instance={tree.name} target={target} collision_50pct_bits={collision:.3f}")

assert len(affected) == 4
print("CONFIRMED sign-34-5: unsalted 256-bit prehash falls below the 256- and 512-bit targets")
