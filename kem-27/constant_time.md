# Constant-time review — kem-27 NTRE

Scope: representative reference instance `Implementations/Reference_Implementation/NTRE-256/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/NTRE-256/KEM_AlgorithmInstance.c:189` — `cpapke_dec` produces secret-dependent message material. A comparison later forms `fail` and the API returns it at line 211, so the validity bit is already explicit.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Size-build follow-up: `center_mod_q` (`src/poly.c:7`, inlined into `poly_msg_mod2_to_bytes` at `:79`) is reached by secret-key decryption, while `freeze_u12` (`src/poly.c:17`, inlined into `poly_tobytes` at `:25`) can process FO re-encryption values. GCC `-Os` emits hardware division at these sites; the reviewed `-O2` build does not. The dataflow is relevant to decapsulation, but no timing distinguisher or shared-secret recovery is established.

Message-dependent sampler: `poly_cbd1_prime` (`src/poly.c:61-77`) selects each coefficient with `beta0 ? … : 0`, where `beta0` is a message bit. `cpapke_enc` calls it (`KEM_AlgorithmInstance.c:69`) from `kem_enc` on the fresh message (line 151) and from `kem_dec` on the decrypted message during re-encryption (line 203). GCC 15.2.0 at the reference Makefile's `-O2 -std=c99` compiles the select to `test $0x1,%dl; je`, a conditional branch on the message bit. This is filed as kem-27-2.
