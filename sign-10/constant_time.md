# Constant-time review — 10 Facto-DSA

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Facto-DSA-512/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: trapdoor/master seed, hidden central map and signing vinegar/solver state. Public: verification key, message and final signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Facto-DSA-512/SIG_AlgorithmInstance.c:1447` — recursive quadratic inversion branches on whether a secret-derived discriminant is a square and may explore a second root at line 1457. By contrast, `f_pow` at line 142 uses fixed public exponents at its call sites.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
