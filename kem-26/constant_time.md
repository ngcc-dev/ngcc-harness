# Constant-time review — kem-26 NSS-HQC

Scope: representative reference instance `Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-256/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-128/nss_hqc_core.c:565` — `pke_decrypt` re-expands the persistent seed at lines 814–820. The fixed-weight sampler makes a secret-derived random index `j`, accesses `perm[j]` at lines 565–568, and can repeat its XOF draw at lines 553–563. Its address and work trace are secret-dependent. FO selection at lines 1049–1057 is masked, though the API also returns validity.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-indexed access and draw loop. A measured cache/timing witness and key-recovery analysis remain open; this is not a constant-time certification.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
