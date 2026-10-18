# UOV-3 — Low: uov-Is collision-forgery estimate relies on a memory penalty

Severity: Low
Status: Confirmed
Layer: Design
Affected: uov-Is in classic, PKC and PKC+SKC formats
Discovery: Moderate
Exploitation: Confirmed cost/model calculation for a generic public-key-only forgery; no attack execution
Remediation: Moderate
Credit: sqrt(q) rescaling result by Ward Beullens and Markku-Juhani O. Saarinen; local cost-model verification by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Result

Section 4.1 of the specification describes a meet-in-the-middle forgery that
matches public-map evaluations `P(s)` against message-and-salt hashes. Its
attacker-favorable RAM estimate is
`2^10.7 * sqrt(q^m * m * log2(q))`. For uov-Is, `q=16` and `m=64`, so the
formula evaluates to `2^142.7` binary gates, slightly below NIST's Category-1
reference of about `2^143`.

The specification acknowledges this value, rounds Table 5 to 143, and argues
that the large list incurs enough memory-access cost to restore the claimed
level. It also acknowledges but does not include a known `sqrt(q)` rescaling
gain. Because `P(c*s)=c^2*P(s)` and squaring is bijective in characteristic
two, that gain is two bits for GF(16), producing the optimistic RAM estimate
`2^140.7`. Accounting for normalization work reduces the gain somewhat; the
review estimated roughly `2^141.2` unless that work is amortized.

## Evidence and limits

The retained check evaluates the specification formula and the exact
`sqrt(q)` adjustment:

```sh
python3 UOV/reproduce_uov_3.py
```

This is a cost/model certificate. It does not allocate the lists or execute a
forgery. Estimates put the list storage around `2^123` entries, or roughly
`2^131` bits depending on representation. A memoryless collision search loses
part of the Gray-code evaluation advantage and exceeds the claimed level. The
security conclusion therefore depends on the memory-cost model NIST applies,
which is why this narrow confirmed margin issue is rated Low.

A larger `m` would remove the dependence on a disputed memory penalty. A full
evaluation should state hash, evaluation, normalization, storage and memory
access costs under one consistent model.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-3
```
