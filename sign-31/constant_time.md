# Constant-time review — 31 TSUOV

Scope: reference implementation, representative instance only: `Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation/TSUOV_128/tsuov_core.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: trapdoor/master seed, hidden central map and signing vinegar/solver state. Public: verification key, message and final signature.

- Branch/loop trace: `Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation/TSUOV_128/tsuov_core.c:695` — TSUOV's echelon solver advances `j` while a hidden-key-derived matrix entry is zero; the rank/retry path reaches signing.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: Source-confirmed secret-dependent pivot and retry. No external timing trace or key extraction has yet been demonstrated.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
