# Constant-time review — kem-30 PolarLAC

Scope: representative reference instance `Implementations/Reference_Implementation/x86/POLARLAC-128/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/x86/POLARLAC-128/KEM_AlgorithmInstance.c:217` — The decrypted message is re-encrypted and compared; `ct_cmov` at line 220 selects the shared secret without a validity branch in this wrapper.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

