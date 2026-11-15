# Constant-time review — 32 UVW

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/UVW-512/SIG_AlgorithmInstance.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: structured-code trapdoor, private permutations and decoder randomness. Public: verification key, message syndrome and final signature.

- Branch/loop trace: `Implementations/Reference_Implementation/UVW-512/SIG_AlgorithmInstance.c:154` — `row_reduce_to_identity` selects the first nonzero pivot in the secret `H_1`; it is reached only from key generation (line 242 in `try_build_keypair`, called by `sig_keygen` at line 345) and from `rebuild_full_keypair` (line 437), which has no caller. On the signing path the first-nonzero pivot search is `gauss.c:17-22` (`gauss_jordan_elimination`), reached through `inverse_matrix` from `dec_z` and `dec_xy` (`uvw.c:188,275`) on randomly chosen column subsets of the secret `H_z` and `H_x + H_y`. `dec_z` and the first loop of `dec_xy` retry while that submatrix is singular (`uvw.c:152-193`, `:251-289`), and the second loop of `dec_xy` repeats until the weight equals `w` (`uvw.c:291-354`); all three are `while (true)` without an iteration bound.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
