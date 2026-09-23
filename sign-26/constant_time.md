# Constant-time review — 26 SQIsign2d-push1/2

Scope: reference implementation, representative instance only: `Implementations/sqisign2d_lvl1/src/sqisigndim2/ref/sqisigndim2x/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: ideal/isogeny trapdoor, signing randomness and intermediate quaternion/curve values. Public: verification key, challenge and final signature.

- Branch/loop trace: `Implementations/sqisign2d_lvl1/src/sqisigndim2/ref/sqisigndim2x/sign.c:690` — The sampling search `while (!found && count < 50)` processes a secret response candidate; no concrete timing/key-recovery witness has been established.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
