# Constant-time review — kem-26 NSS-HQC

Scope: representative reference instance `Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-128/nss_hqc_core.c`; `nss_hqc_core.c` is identical in all four reference levels, and each `Optimized_Implementation/HQC-*` directory is identical to its reference directory. Compiled machine code is not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/HQC-128/nss_hqc_core.c:565` — `pke_decrypt` re-expands the persistent seed at lines 814–820. The fixed-weight sampler makes a secret-derived random index `j`, accesses `perm[j]` at lines 565–568, and can repeat its XOF draw at lines 553–563. Its address and work trace are secret-dependent. FO selection at lines 1049–1057 is masked, though the API also returns validity.
- Branches, indexed accesses and division/remainder: in the same file, `ring_mul_by_sparse` (lines 585–604) branches on every bit of both operands and writes at `i + j`; it is called with `y` in `pke_decrypt` (line 857) and key generation (line 948), and with `r2` in `pke_encrypt` (lines 725, 752), which decapsulation runs again at line 1040. The sampler computes `u % range` with a secret-derived dividend and a public divisor (line 565) and sets bits at `perm[i]` (lines 571–573). Decapsulation branches on the `pke_decrypt` result at lines 1024–1026 before the masked select. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-indexed access and draw loop. A measured cache/timing witness and key-recovery analysis remain open; this is not a constant-time certification.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
