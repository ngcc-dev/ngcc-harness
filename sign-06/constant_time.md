# Constant-time review — 06 COMPASS-SIG

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/sign.c`. The per-set directories `COMPASS-SIG-<N>/` that the build uses hold a different `sign.c`; that file, `poly.c`, `polyvec.c`, `rounding.c`, `packing.c` and `symmetric-shake.c` are each byte-identical across the four reference and four optimized per-set trees. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Reference_Implementation/sign.c:220` — The `polyvecl_chknorm(&z, …)` retry branch tests a secret-key-dependent response. In the per-set `COMPASS-SIG-128/sign.c` the same branch is at line 176, and lines 211–219 leave the `w1` comparison loop at the first mismatching coefficient.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
