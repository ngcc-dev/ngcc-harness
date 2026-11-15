# Constant-time review — 34 YuanYang.DSA

Scope: reference implementation, representative instance only: `Implementations/Reference_Implementation/yuanyang-512/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Reference_Implementation/yuanyang-512/sign.c:511` — Sampler acceptance and retry depend on private lattice-point state; this expected rejection path needs a distinguishability test before claiming key leakage. In `sampler.c` of the same tree, `sampler_zero` (`:392-451`, called at `sign.c:164,168`) returns at `:422` when the drawn sign bit is 0 and otherwise reads `yuanyang_large_zero_berexp[z0]` (`:440-441`), a table indexed by the base-sampler output `z0`; `berexp_compare` (`:235-259`) draws a data-dependent number of random bytes; `sampler` (`:305-390`) retries until acceptance. `gaussian0_sampler` (`:177-233`) scans its whole table.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Key-generation division follow-up: `keygen/keygen.c:42` inverts secret `f_q` via `poly_inv_xn1_q`, reaching extended-Euclid `mod_inv` at `poly_inv_ntt.c:111-139`. GCC `-O2` emits two `idiv` instructions in `mod_inv` (other divides in the object may have public operands). This is a single key-generation trace; without a measured channel and an extraction argument it does not establish a forgery or justify a new Critical report.
