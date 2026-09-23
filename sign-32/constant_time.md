# Constant-time review — 32 UVW

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/UVW-512/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: structured-code trapdoor, private permutations and decoder randomness. Public: verification key, message syndrome and final signature.

- Branch/loop trace: `Implementations/Reference_Implementation/UVW-512/SIG_AlgorithmInstance.c:154` — Gaussian elimination selects the first nonzero pivot in a matrix derived from private decoding state; check call-site provenance before promoting this as a key leak.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
