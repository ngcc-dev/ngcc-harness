# Constant-time review — 18 Origami

Scope: reference implementation, representative instance only: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Origami-256/origami_ref.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: trapdoor/master seed, hidden central map and signing vinegar/solver state. Public: verification key, message and final signature.

- Branch/loop trace: `Implementations and Test_Vectors/Implementations/Reference_Implementation/Origami-256/origami_ref.c:636` — `solve_rect_random` chooses pivots by branching on hidden-zone matrix entries; row-skip at line 659 and retry at line 798 add further secret-dependent paths.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: Source-confirmed secret-dependent path; the implementation even has optional zone-attempt statistics. A full key-recovery or forgery side-channel is not demonstrated.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Table/index follow-up: `origami_gf.h:25,32-37` implements GF multiplication and inversion with secret-indexed tables; signing uses `gf_mult` on secret central-map coefficients and solver values (for example `origami_ref.c:400,599-601,653-661`). Separately, `origami_ref.c:421-426` derives `w_vars` from private `rho`, and `:798-800` writes `secret_y[w_vars[i]]`. These accesses are not functions of the public message or final signature alone. Cache/address observation and an EUF-CMA forgery are not demonstrated; the distinct source-level finding is `sign-18-4`.
