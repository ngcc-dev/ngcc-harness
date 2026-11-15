# Constant-time review — 02 BiT

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/BiT-128/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Reference_Implementation/BiT-128/sign.c:154` — `check_reject_sample_z0z1` controls retry on a masked response derived from secret polynomials.
- Branch/loop trace: `sample.c:141-171` and `197-227` — `reject_sample_coeffs` and `reject_sample_coeffs_sparse` branch per coefficient on `|z_j|` and on a comparison involving `|v_j|` (a coefficient of `s_0·c`, `e·c` or `c`), and squeeze the XOF only for coefficients outside the fast-accept range; `sample.c:246,271` stop at the first rejecting polynomial. `poly.c:26-29` (`rej_S1`, key generation) branches on and indexes `map_S1` with the 2-bit values that become secret coefficients.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
