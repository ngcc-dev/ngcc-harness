# Constant-time review — 01 Aigis-Sig+

Scope: reference implementation, representative instance only: `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/sign.c`. The `Aigis-Sig+-II` and `Aigis-Sig+-III` reference directories are byte-identical to it except for the `Makefile` (the set is selected by `-DPARAMS`). The optimized and additional implementations have not been proven source-equivalent. This is a source-level triage, not a constant-time certification.

Secret: signing-key polynomials/basis, per-signature masks and sampler state. Public: verification key, algorithm parameters, message and serialized signature.

- Branch/loop trace: `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/sign.c:122` — `polyvecl_chknorm(&z, …)` branches on a response containing the secret-key polynomial; further norm tests at lines 127–138 also abort.
- Branch/table-offset trace: `pspm.c:31-33` (likewise `67-69`, `134-136`, `170-172`) — `emulate_cs1`, `emulate_ct0` and `emulate_cs2` branch on whether each challenge coefficient is non-zero and use its sign to select the offset into the secret-derived table; the challenge of a rejected attempt is not part of the signature.
- Table lookups and division/remainder: this source triage does not certify their absence or constant-time compilation. Only the explicitly traced operands above are classified. Public lengths, fixed parameters and verifier checks are not findings.

Assessment: No new vulnerability report is promoted from this representative source triage. A compiler/architecture-specific timing and cache test is still needed before claiming constant-time behavior or exploitable leakage.

Recheck the cited source line and its caller before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.

Hint follow-up: signing calls `polyveck_make_hint` at `sign.c:137`; `rounding.c:86-90` (`make_hint`) contains value tests on signing intermediates, and `polyveck_make_hint` returns at the first 64-coefficient block whose hint weight exceeds 15 (`polyvec.c:376-377`). `use_hint` (`rounding.c:92-112`) is called only from verification (`sign.c:189`). Hints in an accepted signature are public, so the required question is whether control flow reveals *more* than the returned signature, especially on rejected attempts. No differential channel or forgery is shown; this remains a lead.
