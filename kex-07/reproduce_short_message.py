#!/usr/bin/env python3
"""Check that NEV-AKE initiator derivation ignores the responder-message size."""

from pathlib import Path

root = Path(__file__).resolve().parent / "Implementations and Test_Vectors/Implementations/Reference_Implementation"
trees = sorted(root.glob("NEV-AKE-*/kat_test/KEX_AlgorithmInstance.c"))
assert len(trees) == 9, len(trees)
for wrapper in trees:
    source = wrapper.read_text()
    body = source.split("int kex_derive_ss_a(", 1)[1].split("\nint ", 1)[0]
    assert "(void)mb_len_bytes;" in body, wrapper
    assert "ake_der_init(ssa, idi, idj, ska, pkb, sta, mb);" in body, wrapper
    core = (wrapper.parent.parent / "ake.c").read_text()
    derivation = core.split("void ake_der_init(", 1)[1]
    assert "const uint8_t *ct_tilde = M_prime + PKE_OW_CT_BYTES;" in derivation, wrapper
    assert "ake_pke_decrypt_verify(mi_prime, ci," in derivation, wrapper
    assert "ow_kem_dec(kt, (uint8_t *)ct_tilde," in derivation, wrapper
print("CONFIRMED kex-07-2: nine wrappers pass unchecked short responder messages to fixed-size decryptions")
