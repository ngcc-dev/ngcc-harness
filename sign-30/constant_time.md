# Constant-time review — 30 TRINE

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/TRINE-128-ShortSig/iccs/SIG_AlgorithmInstance.c`. The six instances differ only in `params.h`, and each optimized tree is file-for-file identical to its reference tree (`diff -rq`). This is a source-level triage, not a constant-time certification.

Secret: witness/secret key and per-proof random tapes until revealed. Public: verification key, message, Fiat–Shamir challenge and opened transcript.

- Branch/loop trace: `Implementations/Reference_Implementation/TRINE-128-ShortSig/iccs/SIG_AlgorithmInstance.c:98` — The API wrapper checks a signing return code. Inside `crypto_sign`, each of the `r` round commitments is derived by `trine_derive_round_commitment_vartime` (`trine.c:373`, defined at line 138), which resamples the round vector `a` in `corank1_cal_vartime` until a rank test passes (`corank1.c:37-52`) and then runs `canonical_form_vartime` on it (`canonical.c:340`); for each non-base challenge the secret matrix is re-expanded and inverted by `trine_expand_secret_matrix_pair_vartime` (`trine.c:414`), which samples until invertible (`util.c:117-136`) and calls `pmod_mat_inv_vartime` (`trine_expand.c:207`). The elimination core behind the rank, invertibility and inverse routines searches for the first nonzero pivot and skips zero factors (`matrixelim.c:49-56,76-77`). `GF_inv` (`field.c:3`) exponentiates by the fixed public exponent `TRINE_q - 2`.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
