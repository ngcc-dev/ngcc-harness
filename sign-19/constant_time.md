# Constant-time review — 19 Phoenix

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-256f/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: SK seed/PRF seed and unrevealed tree-node values. Public: verification key, message, indices and paths once included in the signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Phoenix-SM3-256f/sign.c:189` — Signature-length/counter checks concern the message-derived TFORS path; this site is not evidence of secret-key leakage.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
