# Constant-time review — 11 FlexTree

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/Flextree-160s/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: SK seed/PRF seed and unrevealed tree-node values. Public: verification key, message, indices and paths once included in the signature.

- Branch/loop trace: `Implementations/Reference_Implementation/Flextree-160s/sign.c:133` — The `hash_message` error check alone is not a secret-value branch; tree operations require a separate address-index trace.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
