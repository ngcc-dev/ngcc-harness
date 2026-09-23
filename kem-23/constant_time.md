# Constant-time review — kem-23 Mito

Scope: representative reference instance `Implementations/Reference_Implementation/Mito-1-256/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/Mito-1-128/vector.c:52` — `mito_kem.c:127-134` passes the persistent PKE seed into `mito_pke_decrypt`; `mito_pke.c:128-134` and `parsing.c:15-19` re-expand it on each decapsulation. The sampler's reject and duplicate branches at `vector.c:52-73` therefore have secret-dependent iteration count.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-seed-dependent work, analogous to the TriQ/QUBE findings. A measured timing witness and key-recovery analysis remain open; no constant-time guarantee is made for the decoder.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
