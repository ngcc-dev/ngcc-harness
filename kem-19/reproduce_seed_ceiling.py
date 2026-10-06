#!/usr/bin/env python3
"""Certify Lore-L3/L4's 32-byte output and PKE keygen seed ceilings."""

from pathlib import Path

root = Path(__file__).resolve().parent / "Implementations and Test_Vectors/Implementations"
trees = sorted(p.parent for p in root.rglob("params.h")
               if p.parent.name in {"Lore-L3", "Lore-L4"})
assert len(trees) == 12, len(trees)

for tree in trees:
    params = (tree / "params.h").read_text()
    kem = (tree / "kem.c").read_text()
    indcpa = (tree / "indcpa.c").read_text()
    assert "#define LORE_SYMBYTES 32" in params, tree
    assert "#define LORE_SSBYTES LORE_SYMBYTES" in params, tree
    assert "indcpa_keypair_derand(pk, sk, coins);" in kem, tree
    assert "coins[LORE_SYMBYTES + i]" in kem, tree
    assert "coins, LORE_SYMBYTES" in indcpa, tree
    assert "gen_matrix_ntt(a_ntt, seedA, 0);" in indcpa, tree
    assert "poly_getnoise(&s_crt, s_t_sparse" in indcpa, tree
    print(f"instance={tree.name} backend={tree.parent.name} ss_bits=256 keygen_seed_bits=256")

print("CONFIRMED kem-19-2: twelve higher-level trees emit 256-bit keys and derive public keys from 256-bit PKE seeds")
