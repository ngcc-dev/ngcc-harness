#!/usr/bin/env python3
"""Check the SQIsign2D-push Level-3 failed-split verifier path."""

from pathlib import Path

root = Path(__file__).resolve().parent
src = root / "Implementations/sqisign2d_lvl3/src"
theta = (src / "hd/ref/hdx/theta_isogenies.c").read_text()
verify = (src / "sqisigndim2/ref/sqisigndim2x/sign.c").read_text()
for level in (1, 2, 4):
    other = root / f"Implementations/sqisign2d_lvl{level}/src"
    assert (other / "hd/ref/hdx/theta_isogenies.c").read_text() == theta
    assert (other / "sqisigndim2/ref/sqisigndim2x/sign.c").read_text() == verify

split = theta.split("splitting_comput(theta_splitting_t *out", 1)[1]
split = split.split("theta_product_structure_to_elliptic_product", 1)[0]
assert "if (good)" in split
assert "apply_isomorphism(&out->B.null_point" in split
assert "return good;" in split

chain = theta.split("theta_chain_comput_balanced(theta_chain_t *out", 1)[1]
call = "int is_split = splitting_comput(&out->last_step, &out->steps[n - 2].codomain);"
assert call in chain
tail = chain.split(call, 1)[1][:300]
assert "//assert(is_split);" in tail
assert "theta_product_structure_to_elliptic_product(&out->codomain, &out->last_step.B)" in tail

assert "theta_chain_comput_balanced(" in verify
assert "hash_to_challenge(" in verify
print("CONFIRMED sign-26-3: failed final split reaches codomain construction in all four byte-identical reference paths")
