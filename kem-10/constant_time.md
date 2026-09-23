# Constant-time review: C-Multi-UR-AG

The PKE secret reconstructed from `sk`, decoded message `m`, fallback `sigma`, and shared secret are secret. Ciphertext matrices, salt, and embedded public key are public. This review follows the 128-bit reference path; the decoder structure is present in the 256/512-bit sources. It is a source-level audit, not a remote attack measurement.

`src/kem.c:209-213` decapsulates with `sk`; `src/cmultiurag.c:248-257` computes a word using the secret PKE matrix and calls augmented-Gabidulin decoding. `src/augmented_gabidulin.c:185-201` chooses pivot `next` from discrepancies and uses it as the address in `u0[next]` and `u1[next]` loads/stores. `src/qpoly.c:509,580` contains degree-dependent decoder loops. These are secret-derived control-flow/memory-access candidates. The final re-encryption comparison and masked fallback selection are distinct from the decoder leakage; malformed-ciphertext crashes are separately reported in `report.md`.

No key-recovery path or remote channel is demonstrated. The source-level witness is the chain `kem.c:209-213` → `cmultiurag.c:248-257` → `augmented_gabidulin.c:185-201`.
