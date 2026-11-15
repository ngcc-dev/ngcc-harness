# Constant-time review — 12 Galas

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/Galas-256F/ngcc/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: witness/secret key and per-proof random tapes until revealed. Public: verification key, message, Fiat–Shamir challenge and opened transcript.

- Branch/loop trace: `Implementations/Reference_Implementation/Galas-256F/ngcc/SIG_AlgorithmInstance.c:326` — `galas_constraints_eval` tests the private witness, but the return is an error path; challenge-grinding branches later are transcript derived.
- Secret-dependent field arithmetic: every `sig_sign` evaluates `galas_owf_eval` on the secret key (`ngcc/SIG_AlgorithmInstance.c:282`) and calls `build_galas_witness` (`:318`); both run `galas_apply_lin_map` and the former also `gf_inv` (`galas/owf.c:56,61,98–100,114`) on key-derived field elements. The reference `gf_mul` skips each zero bit of its second operand (`galas/gf2n.c:110`), `gf_sqr` branches on each operand bit (`:130`), `gf_reduce_wide` branches on each high product bit (`:89`) and `gf_inv` returns early on zero (`:149`). `galas/gf2n.c` is identical in all eight reference trees; the optimized trees were not checked. This is a source-level observation; it has not been measured.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
