# Constant-time review — kem-35 Scloud+

Scope: reference and optimized implementations, supplemented by the measured trace in [kem-35-1](report.md#kem-35-1-re-encryption-rejection-sampling-leaks-the-decrypted-message-class).

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations and Test_Vectors/Implementations/_shared/scloudplus_core/common/kem.c:108` — After private decryption at line 103, fixed-length verification produces `fail_mask`; `scloudplus_cmov` at lines 110–112 selects the rejection input. This FO selection is masked, not a secret-value branch in the inspected core. Barnes–Wall decoding (`common/encode.c:239-309`, `bddbwn`, shared by both trees) is branch-free at source level: recursion depth and loop bounds depend only on the public `n`, and the nearer candidate is chosen by `ct_mask_u64_lt` (line 301) and a masked select.
- Deeper trace: re-encryption derives its sampler seed from the decrypted message. The BD6/BD12 rejection samplers in `common/sample.c` and `avx2/sample_avx2.c` consume a data-dependent number of batches and XOF squeezes and compact accepted bits through a secret-derived index. This affects the 256-, 384-, and 512-bit sets; the non-rejection 128- and 192-bit samplers are controls.
- Branches, indexed accesses and division/remainder: public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of unrelated leaks has not been proved.

Assessment: the decrypted-message timing channel is filed as kem-35-1. A practical follow-up recovers 12,743 of 13,024 secret coefficients from physical timings, but supplies neither a method for correcting the remaining 281 coefficients nor a working recovered decapsulation key. Unrelated compiler output remains open audit work.

Reproduction is the source/dataflow inspection above plus the deterministic squeeze-count and timing witness in `reproduce_reencryption_timing.sh`.
