# MAYO-1 — Low: documented signing retry contradicts the broad constant-control-flow claim

**Severity:** Low
**Status:** Confirmed
**Layer:** Side-channel
**Discovery:** Moderate
**Affected:** submitted On-Ramp Round 3 C implementations (all five trees: Reference, Optimized, AVX2, GFNI, and NEON), MAYO-1, MAYO-2, MAYO-3 and MAYO-5
**Remediation:** Local
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Exploitation:** pinned-source data-flow witness plus AVX2 timing measurement of the retry count (64 keys showed no key dependence); no key recovery or forgery
**Date:** 2026-10-18

## Affected implementation and claim

The finding applies to all five submitted C trees (`Reference_Implementation`,
`Optimized_Implementation`, and `Additional_Implementations/{AVX2,GFNI,NEON}`),
which share byte-identical `src/` and `include/` directories, for MAYO-1,
MAYO-2, MAYO-3 and MAYO-5 (confirmed by taint runs on reference, optimized, and
AVX2 builds and source reading of GFNI and NEON; see
constant_time.md (`MAYO/constant_time.md`)). The retained evaluation builds the common code once
per parameter set. The On-Ramp Round 3 specification, §3, says the C implementations
“avoid secret-dependent data indexing and secret-dependent control flow.”  It
also says, more narrowly, that revealing whether the solver must restart is
acceptable.  This report records the concrete exception to the broad claim; it
does not turn that design rationale into a key-recovery claim.

The design discussion on physical p.4 explains why MAYO retains the salt and
retries with fresh preimage-sampling randomness, despite the resulting proof
loss.  In §5.3, physical pp.31–32, the specification bounds the per-signature
restart probability by
`B = (q^(k-(n-o)) + q^(m-ko))/(q-1)`.  For the submitted parameters this is
1/15 (6.67%) for MAYO1, 1/240 (0.417%) for MAYO2, and 1/3840 (0.0260%, about
2^-11.9) for both MAYO3 and MAYO5.  In the follow-up timing measurement
recorded in constant_time.md (`MAYO/constant_time.md`), AVX2 MAYO1 signing time was
quantized by the retry count (median 268k, 476k, and 683k cycles for 0, 1, and
2 retries across 6,000 signatures on one key), and 64 keys × 20,000 signatures
gave an empirical rejection rate of 0.066652 with no detected key dependence
(chi-square 51.7 on 63 degrees of freedom; MAYO2 gave 0.004230 over 32 keys,
chi-square 27.7 on 31).

## Data flow and observable behavior

The compact secret seed is assigned at `src/mayo.c:443`.  It is included in the
SHAKE input at lines 473-477 and again in the attempt expansion at lines
493-497.  The resulting vinegar values and the secret oil-space matrix `sk->O`
are passed into `m_calculate_M_and_VPV` at lines 499-506, producing the hidden
linear system `A`.

`sample_solution` computes the last-row rank predicate from that matrix at
`src/arithmetic.c:75-79`.  Under `ENABLE_CT_TESTING`, line 83 explicitly marks
the byte defined to Valgrind immediately before the branch at lines 86-88.  The
caller branches on the return value and breaks out of the attempt loop at
`src/mayo.c:520-523`; failure performs another full attempt.  Thus the signing
control-flow trace exposes the number of rank-deficient attempts even though
the hidden system and its derivation contain secret data.

The submitted echelon-form implementation otherwise uses fixed bounds, masked
pivot selection, and public loop indices (`src/generic/echelon_form.h:45-136`).
No secret-indexed table was found in the portable GF(16) path.

## Impact and classification

This is a confirmed source-level branch whose retry count is observable by
plain timing on AVX2 (constant_time.md (`MAYO/constant_time.md`)).  The number of
attempts is not serialized in the signature.  The specification deliberately
discusses the retry probability, accounts for its information-theoretic
leakage in the proof, and states that revealing whether the solver restarts is
acceptable.  The submitted Valgrind instrumentation likewise declassifies the
rank result immediately before the branch.  The 64-key MAYO1 and 32-key MAYO2
timing/count runs detected no correlation with the long-term oil space, and
this survey did not map retry observations to key recovery or a forgery.

The defect established here is therefore the conflict between that documented
exception and the blanket statement that all C implementations avoid
secret-dependent control flow.  Correcting the broad statement is local, and
this finding stays Low because the measured timing signal reflects only the
retry count the specification treats as public, with no detected key
dependence.  NIST's required EUF-CMA property is not shown to fail.

The submitters' narrower rationale is that, with a fixed public salt and fresh
vinegars for each attempt, the retry count need not be hidden.  That may be a
valid distributional argument, but it is different from the specification's
blanket implementation claim.  A proof that the trace is independent of the
long-term key, or a leakage experiment and recovery analysis, would change the
security interpretation.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_1.py
```

The deterministic witness pins the specification and three relevant source
files by SHA-256, checks that the same three source files are byte-identical
across all five submitted C trees, verifies the ordered secret-to-branch
source trace, and checks the rank-predicate branch model.  It reports PASS
only for that root-cause witness; the AVX2 retry-timing measurement is
recorded separately in constant_time.md (`MAYO/constant_time.md`) and is not rerun
by this script.

The functional control is:

```sh
make -C MAYO libs
python3 MAYO/survey_runtime.py
```

All four libraries passed seeded replay, seed sensitivity, signature diversity,
message recovery, malformed-length rejection and sampled mutation rejection.
Those results are compatible with this side-channel finding.

## Remediation and follow-up

A local documentation repair can qualify the blanket implementation claim and
identify the deliberately declassified retry result.  An implementation that
instead promises a fixed control-flow trace can run all 256 attempts and
mask-select the first successful solution, or use another fixed-work batching
strategy; that optional strengthening is a material signing-cost change.

Future work could extend timing measurements to the non-AVX2 backends and test
whether much larger sample sizes or microarchitectural channels distinguish
properties of `O`.  Until then no key-dependent leakage or recovery has been
demonstrated.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-1
```
