# Constant-time review — kem-35 Scloud+

Scope: representative reference instance, forwarding into `Implementations and Test_Vectors/Implementations/_shared/api_pkc/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations and Test_Vectors/Implementations/_shared/scloudplus_core/common/kem.c:108` — After private decryption at line 103, fixed-length verification produces `fail_mask`; `scloudplus_cmov` at lines 110–112 selects the rejection input. This FO selection is masked, not a secret-value branch in the inspected core. Barnes–Wall decoding remains a deeper audit target.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.
