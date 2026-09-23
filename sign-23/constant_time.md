# Constant-time review — 23 Shuttle

Scope: reference implementation, representative instance only: `Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations/Reference_Implementation/SHUTTLE-256/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementation codes and test vectors/Shuttle 算法实现源代码及测试向量/Implementations/Reference_Implementation/SHUTTLE-256/sign.c:302` — The `if (!ok)` branch controls iterative sampling in a scheme intended to avoid rejection; the exact secret/public dependence is not yet established.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Size-build follow-up: `ntt_ref_fqmul` (`ntt_ref.c:50`) computes `a*b % NTT_Q` on a signing path that includes secret mask `y′`; GCC `-Os` emits hardware division, unlike the reviewed `-O2` build. A timing channel and EUF-CMA forgery path remain unshown, so this is a lead rather than a confirmed break.
