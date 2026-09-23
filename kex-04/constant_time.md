# DKEX constant-time review

Scope: DKEX-128 reference, with 256/512 sharing the same component layout. Long-term signature/KEM keys, signing nonce derivation, decapsulated material and session secret are secret. Protocol messages and input lengths are public.

- `DKEX-128/adkex_sig_mldsa.c:65-82` deterministically signs from `sk` and the transcript through `dilithium/sign.c:108-124`. The signing attempt computes secret-derived `z`, `w0`, and `h`; `dilithium/sign.c:156-183` rejects an attempt through four conditional jumps. This is variable signing control flow. The reference ML-DSA rejection loop is intended to suppress secret-dependent output bias; this review did **not** establish that observing attempt counts recovers secret information beyond the published signature, so it is recorded here rather than asserted as a new cryptanalytic report.
- `DKEX-128/dilithium/poly.c:319-325,394-410` has variable sampling branches on XOF output. Some seeds are derived from signing secrets; the loop count is not a proof by itself of exploitable leakage. `DKEX-128/dilithium/poly.c:274-285` norm checks receive secret-derived response vectors and can exit early.
- Length checks at `DKEX-128/adkex_sig_mldsa.c:87` and `dilithium/sign.c:218` depend on public/fixed lengths. Constant-divisor arithmetic in packing/rounding was not classified as a variable-time division without a compiled-code witness.

No additional report was promoted from this pass. The cited signing paths require statistical leakage analysis or a stronger timing witness; this is not a whole-program constant-time guarantee.
