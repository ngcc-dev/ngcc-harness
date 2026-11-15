# Constant-time review — 29 Tins

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/Tins512/ff_arith.c` (called from `SIG_TINS512.c:212`). Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing witness `alpha,beta` expanded from the secret seed, its derived field elements and unrevealed MPC tapes. Public: verification key, message, challenge and opened transcript.

- Branch/loop trace: `Implementations/Reference_Implementation/Tins512/ff_arith.c:552` — `fe_inv` loops until a secret-derived polynomial remainder reaches degree zero and indexes `z_inv` by a secret-derived field coefficient at line 583.
- Witness-bit branches: `fe_f2_inner` (`ff_arith.c:653`) branches on every bit of the witness vectors `alpha` and `beta`; it is called in the signing-time secret expansion (`SIG_TINS512.c:192,202,206,210`), in key generation (`SIG_TINS512.c:88,98,102,106`) and in `ComputePoly` (`bavc_commit.c:452,456,460,464`). `ff12b_cond_add` (`ff_arith.c:292`) branches on the per-leaf share bits `alpha_rnd`/`beta_rnd` in `CommitPoly` (`bavc_commit.c:395–396`).
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: Source-confirmed secret-dependent loop and lookup. Existing sign-29-1 already exposes the complete witness without side-channel access; no separate report is justified absent a distinct demonstrated impact.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
