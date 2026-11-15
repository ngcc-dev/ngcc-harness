# Constant-time review — 15 MORNING-ATLAS

Scope: reference implementation, representative instance only: `Implementation/Reference_Implementation/lwrdsa192/SIG_lwrdsa192.c`. `SIG_lwrdsa128.c`, `SIG_lwrdsa192.c` and `SIG_lwrdsa256.c` are byte-identical within the reference tree and within the optimized tree; the two trees differ from each other, and the `lwrdsa512` file and the per-set `poly.c` files differ. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementation/Reference_Implementation/lwrdsa192/SIG_lwrdsa192.c:365` — The signer retries when `polyvecl_chknorm(&z, …)` rejects a secret-key-dependent response. Lines 399–402 leave the `w1` comparison loop with `goto rej` at the first mismatching coefficient.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
