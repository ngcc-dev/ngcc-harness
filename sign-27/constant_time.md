# Constant-time review — 27 SQIsignTriangle

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/SQIsignTriangle_lvl1/sqisigntriangle/src/signature/ref/lvlx/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: ideal/isogeny trapdoor, signing randomness and intermediate quaternion/curve values. Public: verification key, challenge and final signature.

- Branch/loop trace: `Implementations/Reference_Implementation/SQIsignTriangle_lvl1/sqisigntriangle/src/signature/ref/lvlx/sign.c:72` — The signing loop retries after secret ideal/commitment computations; full timing dependence and observability remain to be measured.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
