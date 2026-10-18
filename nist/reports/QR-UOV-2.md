# QR-UOV-2 — Low: EUF-CMA reduction does not cover NIST's 2^64-query estimate

**Severity:** Low
**Status:** Proof gap
**Layer:** Design
**Affected:** Round-3 main q=127, delta=2 parameter sets in categories 1, 3, and 5
**Discovery:** Moderate
**Exploitation:** proof/model gap; no forgery or key-recovery attack
**Remediation:** Moderate
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Bound and NIST baseline

QR-UOV's Theorem 2 contains the reduction factor
`(1 - qsign * epsilon_rank)^-1` and assumes
`qsign * epsilon_rank < 1/2` (specification §6.2, physical pp.36–38). Here
`epsilon_rank` is a worst-case-over-keys rank-rejection probability. The
specification does not prove that worst-case bound. It instead models the
secret-structured signing matrix as a uniform random matrix and obtains

```text
epsilon_rank approximately 1.173e-19 < 2^-62.88.
```

Even under that additional heuristic, the specification states that the
condition is supported only when signature generations per key are limited to
`qsign <= 2^61`. It expressly says the calculation is not a worst-case bound.

NIST's call §4.B.2, physical p.14, says that for estimating security strength
the attacker may be assumed to obtain signatures on no more than `2^64`
chosen messages. Applying QR-UOV's own uniform-matrix estimate gives

```text
2^61 * epsilon_rank = 0.270431... < 1/2
2^64 * epsilon_rank = 2.163448... > 1
```

Thus the theorem's required premise already fails before the full NIST query
ceiling. At `2^64`, even the displayed denominator is no longer a meaningful
positive reduction factor.

## Independent QROM loss

Theorem 2 also contains the additive term

```text
(3/2) * qsign' * sqrt((qsign' + 3*qsign + qqro + 5) / 2^lambda).
```

Here `qsign'` counts the random-oracle evaluations made during signing, so it
is at least `qsign`. Even in the favorable case `qsign'=qsign` and `qqro=0`,
this term alone is approximately `2^33.58`, `2^1.58`, and `2^-30.42` at
`qsign=2^64` for `lambda=128`, `192`, and `256`, respectively. It reaches one
at about `2^41.61` signing queries for category 1 and `2^62.94` for category 3.
The theorem's bound is therefore vacuous before the NIST ceiling for those two
categories independently of the rank-probability issue.

## Classification and limits

This is a candidate-specific proof and usage-bound shortfall, rated Low /
Proof gap under the project methodology. It is **not an attack**: the
calculation neither proves that the structured matrices attain the heuristic
failure probability nor turns rank failures into a forgery. It does not show
that any QR-UOV parameter has fewer than its claimed security bits in practice.
Conversely, the absence of a worst-case bound means the theorem is not fully
established even within the proposed `2^61` operational limit.

Closing the report requires a suitable per-key usage rule accepted as part of
the security target, parameters that keep both losses meaningful through
`2^64`, or a proof that avoids or rigorously bounds the rank-rejection and
QROM losses.

## Reproduction

```sh
python3 QR-UOV/reproduce_qruov_2.py
```

The deterministic script evaluates the specification's finite-field
random-matrix formula for every main-set value of `m`, checks the pinned
specification, compares the products at `2^61` and `2^64`, and evaluates the
first additive QROM term for all three security parameters. This is a
cost/model check of the stated theorem, not a security reduction or attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-2
```
