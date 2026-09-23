# Constant-time review — kem-21 MAMBA-Viper

Scope: representative reference instance `Implementations/Reference_Implementation/MAMBA-Viper-128/kem.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/MAMBA-Viper-128/kem.c:55` — Decapsulation passes the secret key and public ciphertext to `viper_pke_dec`, then uses a mask-based `cmov` at line 62 for implicit rejection. No secret branch is established by the cited wrapper.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

