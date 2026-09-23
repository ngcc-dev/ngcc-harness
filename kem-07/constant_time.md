# Constant-time review: BRQC

The PKE secret `y`, decoded message `m`, fallback `sigma`, and shared secret are secret. The ciphertext and embedded public key are public. This review follows BRQC-128; the same decoder pattern occurs in the other reference parameter sets. It is a source-level audit, not a measured remote attack.

`src/kem.c:214` decrypts under `sk`; `src/brqc.c:277-288` computes the word `v-u*y` and calls Gabidulin decoding. `src/gabidulin.c:185-201` derives pivot `next` from discrepancies in that word and uses it to load and store `u0[next]` and `u1[next]`. These memory addresses depend on secret-key processing. `src/qpoly.c:509,580` has degree-dependent decoder loops. The final re-encryption comparison and fallback selection in `src/kem.c:228-254` are masked and are not the source of this finding. Public parameter and length checks are likewise outside the secret-leakage claim.

No full-key extraction or remote timing measurement is shown. The source-level witness is the chain `kem.c:214` → `brqc.c:277-288` → `gabidulin.c:185-201`; see `report.md` for the bounded finding.
