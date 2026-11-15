# MQOM-5 — Low: Theorem 5.1's concrete EUF-KO bound reaches one 5 to 7.5 bits below 2^lambda oracle queries

Severity: Low
Status: Proof gap
Layer: Design
Affected: Theorem 5.1 for all 18 parameter sets; at Category 1 the boundary lies below the 2^143 gate comparison
Discovery: Moderate
Exploitation: Concrete proof-bound boundary in public random-oracle and ideal-cipher queries; no forgery or key recovery
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Bound

Theorem 5.1 (physical p.74) bounds the EUF-KO advantage by a sum that
includes `(4N+1) Q_RO Q_IC / 2^(2 lambda)`, derived in Eq. (15)-(16)
(physical p.76) and Eq. (19) (physical p.77). Its hypotheses are
`Q_RO <= 2^lambda` and `Q_IC <= 2^(lambda-1)`. With `Q_RO = Q_IC = Q` the
whole bound reaches one at:

| N | Q where the bound reaches one | Category 1 queries | Category 1 gates (2^15 per query) |
|---:|---:|---:|---:|
| 8192 | 2^(lambda−7.51) | 2^120.49 | 2^135.49 |
| 4096 | 2^(lambda−7.01) | 2^120.99 | 2^135.99 |
| 2048 | 2^(lambda−6.52) | 2^121.48 | 2^136.48 |
| 256 | 2^(lambda−5.06) | 2^122.94 | 2^137.94 |

The `(4N+1)` term contributes 93% to 99% of the bound at those points. All
Category 1 boundaries lie below the 2^143 gate comparison. Categories 3 and 5
reach one at about 2^200–2^202 and 2^265–2^267 gates against 2^207 and 2^272.

## Classification and limits

Low / Proof gap: a candidate-specific concrete boundary of the submitted
theorem inside the category target, with no attack. The queries are public
random-oracle and ideal-cipher queries of a key-only adversary, not signing
queries. The comparison assumes `Q_RO = Q_IC`, omits `epsilon_mq`, and prices
a query like one AES call (2^15 gates at Categories 1 and 3, 2^16 at
Category 5; the specification gives "2^15 or 2^16" without assigning levels).
Without this term the bound still reaches one at `2^(lambda−1.71)` queries,
about 2^141.3 gates at Category 1, and the hypothesis on `Q_IC` alone caps the
theorem below 2^143; the term adds 3.3 to 5.8 bits of loss. The theorem is
EUF-KO only; [MQOM-3](/nist/reports/MQOM-3.html) covers the separate PGOW-MQ gap.

## Reproduction

```sh
python3 MQOM/reproduce_mqom_5.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MQOM-5
```
