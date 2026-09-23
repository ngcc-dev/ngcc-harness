# Constant-time review — kem-36 TRIKE

Scope: representative reference instance `Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-9/src/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-9/src/KEM_AlgorithmInstance.c:347` — The private `h0/h1/h2` indices are copied from the secret key at lines 331–333 and passed into `decode`; the later ciphertext/error comparison is combined with a full-length mask at lines 355–359. The compare is not a new branch oracle, but the decoder itself requires deeper address/control-flow review. Existing `kem-36-1` is a separate functional failure.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
