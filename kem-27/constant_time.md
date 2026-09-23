# Constant-time review — kem-27 NTRE

Scope: representative reference instance `Implementations/Reference_Implementation/NTRE-256/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/NTRE-256/KEM_AlgorithmInstance.c:189` — `cpapke_dec` produces secret-dependent message material. A comparison later forms `fail` and the API returns it at line 211, so the validity bit is already explicit.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Size-build follow-up: `center_mod_q` (`poly.c:79`) is reached by secret-key decryption, while `freeze_u12` (`poly.c:25`) can process FO re-encryption values. GCC `-Os` emits hardware division at these sites; the reviewed `-O2` build does not. The dataflow is relevant to decapsulation, but no timing distinguisher or shared-secret recovery is established.
