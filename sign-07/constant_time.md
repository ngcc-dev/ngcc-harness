# Constant-time review — 07 CS

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/CS-128/cs.c`. The `CS-256` and `CS-512` reference trees and the three optimized trees have byte-identical `.c` and `.h` sources apart from `params.h`. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Reference_Implementation/CS-128/cs.c:121` — `fail` controls signing retries; trace the mask and norm computations before treating elapsed time as a key leak.
- Branch/table trace: `SampleSign` (`sampling.c:859-973`, called at `cs.c:120`) — `AddorSubPoly` (`ntt.c:205`) and `sampling.c:966` branch on the sampler's per-step sign bit; `sampling.c:889-896`, `908-915` and `927-934` index `fre_0[]`, `fre_1[]` and `fre_2[]` with absolute values of `y ± c·s` coefficients; `bitlength`/`Approx` (`sampling.c:815-857`) return early and branch on products of those entries; the early returns at `sampling.c:885,905,924,955-957,963-964` depend on the same values, and the `-1` at line 956 makes `CS_Sign` return 1 (`cs.c:121-122`).
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
