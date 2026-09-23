# Constant-time review — 01 Aigis-Sig+

Scope: reference implementation, representative instance only: `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/sign.c`. Other instances and optimized implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/sign.c:122` — `polyvecl_chknorm(&z, …)` branches on a response containing the secret-key polynomial; further norm tests at lines 127–138 also abort.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Hint follow-up: signing calls `polyveck_make_hint` at `sign.c:137`; `rounding.c:86-103` contains value tests in `make_hint`/`use_hint` on signing intermediates. Hints in an accepted signature are public, so the required question is whether control flow reveals *more* than the returned signature, especially on rejected attempts. No differential channel or forgery is shown; this remains a lead.
