# Constant-time review — 08 DARTS

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/DARTS512/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Reference_Implementation/DARTS512/sign.c:239` — The `reject1 || …` branch controls a rejection sampler on secret-key-dependent response values.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Size-build follow-up: `poly_pack_highbits` (`poly.c:522-525`) and `poly_compress` (`:41`) process `w = A·y`, where signing mask `y` is secret. GCC `-Os` emits division in these routines; the reviewed `-O2` build does not. The eventual signature may disclose some function of `w`, so a side-channel claim needs leakage beyond that public transcript and a route to EUF-CMA forgery; neither is established here.
