# Constant-time review: BRA

The PKE secret `y` (recovered from `sk`), decoded message `m`, fallback `sigma`, and shared secret are secret. The ciphertext and embedded public key are public. This review follows BRA-128; the decoder is duplicated in BRA-256/512. The finding is a source-level leakage trace, not a demonstrated key-recovery attack.

`src/kem.c:214` decrypts the ciphertext with `sk`, and `src/bra.c:277-288` forms `v-u*y` before augmented-Gabidulin decoding. In `src/augmented_gabidulin.c:184-205`, the pivot `next` is computed from decoded-word discrepancies and then used directly in `u0[next]` and `u1[next]` loads and stores. The memory address can therefore depend on the recipient secret and chosen ciphertext. `src/qpoly.c:509,580` also has degree-dependent division loops on decoder intermediates. By contrast, the final ciphertext comparison and fallback selection at `src/kem.c:228-254` use a fixed-length OR accumulator and masks.

This review does not claim that the pivot trace suffices for key recovery or that a network timing channel is exploitable. The source-level witness is the chain `kem.c:214` → `bra.c:277-288` → `augmented_gabidulin.c:184-205`; see `report.md` for the separate finding.
