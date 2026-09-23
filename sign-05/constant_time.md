# Constant-time review — 05 Chinith

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/sm4th_d3_128s_tight/sig_impl.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: witness/secret key and per-proof random tapes until revealed. Public: verification key, message, Fiat–Shamir challenge and opened transcript.

- Branch/loop trace: `Implementations/Reference_Implementation/sm4th_d3_128s_tight/sig_impl.c:476` — Challenge-3 grinding and opening use outcome-dependent branches; the predicate is transcript/challenge derived, so this line alone is not evidence of secret-key leakage.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
