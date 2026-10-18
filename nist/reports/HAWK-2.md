# HAWK-2 — Low: rejected signing attempts expose secret-derived control flow

**Severity:** Low
**Status:** Confirmed
**Layer:** Side-channel
**Discovery:** Moderate
**Affected:** withdrawn HAWK v1.1 portable reference signer, HAWK-256, HAWK-512 and HAWK-1024
**Remediation:** Moderate
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Exploitation:** pinned-source ordered-statement witness; no measured channel, basis recovery, or forgery
**Date:** 2026-10-18

## Behavior

The signer regenerates the secret basis and enters an unbounded attempt loop at
`hawk_sign.c:904`.  It derives the secret coset parity from the basis and public
hash at lines 955-977, then passes that parity to the constant-time Gaussian
sampler at lines 987-1012.  Although the sampler itself scans its tables with
fixed bounds, the caller restarts when the sampled norm is too large at lines
1019-1021.

The signer next computes `s1` using the secret basis.  It exits the coefficient
loop early and restarts if a bound is exceeded (`1111-1131`).  Finally,
Golomb–Rice encoding performs a data-dependent unary loop (`679-754`) and the
signer restarts when the encoded candidate does not fit (`1142-1158`).  Only the
successful salt and `s1` are returned.  Therefore the count and work profile of
rejected attempts are not determined by the final public signature.

The signer also regenerates `(f,g)` from the compact key after the `s1`-bound
and encoding-overflow retries (`hawk_sign.c:1128,1156`), increasing the work
distinguishing those retry paths.

The specification claims in §1.1.1 (p.4) that functions depending on secret
data are isochronous. Section 3.5.1 (p.28) notes the sampler's constant-time
table design, and §4.1.3 (p.39) specifically forbids an early exit from the
secret table scan. The submitted sampler follows that rule. The surrounding
rejection trace nevertheless processes secret-derived intermediates and is
observable in principle to a local timer, branch tracer, or physical observer.

## Impact and severity

The finding confirms secret-derived signing control flow at source level.  It
does not establish a statistically stable same-public-input channel, correlation
with the long-term basis, or an extraction algorithm.  It is unrelated to and
strictly weaker than the public-key-only recovery in [HAWK-1](/nist/reports/HAWK-1.html).

The specification reports restart probabilities of approximately `5.3e-6` for
HAWK-512 and `2.5e-6` for HAWK-1024 (§3.5.2, pp.29-30), dominated by encoding
failures. Its §6.3.3 proof also treats the restart predicate on a candidate
signature as publicly checkable and simulates the restarting signing oracle
from a non-restarting one. That proof argument does not establish that a local
timing trace is key-independent, but it materially limits what can be inferred
from the mere existence of retries.

The direct consequence boundary is Low under the classification methodology in
`security/methodology.md` (`security/methodology.md`): this is source-level
secret dependence without a measured channel or recovery map. Strict removal
would require fixed-work batching and mask-selection among complete signing
attempts, a material implementation change, but remediation cost alone does not
raise the severity. Unlike QR-UOV's finer pivot trace, the observed predicate
here is treated by the HAWK proof as publicly checkable and simulatable. No
primary NIST security property is shown to fail.

## Reproduction

```sh
python3 HAWK/reproduce_hawk_2.py
python3 HAWK/survey_runtime.py
```

The first command pins the three identical reference signer copies and the
specification by SHA-256, then checks that the relevant source statements occur
in the expected order. PASS denotes that source certificate only. It does not
trace execution or claim a timing measurement, key recovery, or forgery.

The functional runtime screen confirms normal signatures verify, deterministic
KAT-seed replay works, different seeds produce different keys, consecutive
signatures differ, malformed lengths are rejected, and tested one-bit mutations
do not produce an accepted signed message.  Those functional results are
compatible with the execution-trace finding.

## Remediation and follow-up

A hardened implementation must use fixed-work attempt batching and
constant-time selection of a successful complete encoding, with an explicitly
bounded residual failure probability.  Merely making `encode_sig` branchless
does not hide the number of rejected candidates.  Blinding or a platform
isolation boundary may reduce observability but does not make the source trace
constant-time.

A follow-up should instrument attempt causes and timing across many independently
generated keys, control for public message and final signature features, and
test whether the trace distinguishes or recovers any property of the basis.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py HAWK-2
```
