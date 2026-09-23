# Constant-time review — kem-25 NEV

Scope: representative reference instance `Implementations/Reference_Implementation/NEV-R1/kat_test/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/NEV-R1/cca.c:47` — `ow_pke_dec` yields a private message; `verify` sets `fail`, and lines 48–51 branch on it and return it. This validity bit is already an explicit API result, so a timing signal here is not an additional hidden oracle.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
