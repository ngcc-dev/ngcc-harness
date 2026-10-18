# SQIsign-5 — Low: ROM bound loses security at 2^64 signing queries

Severity: Low
Status: Proof gap
Layer: Design
Affected: Theorem 8.1.7 security claim for p324_3, p500_27, and p664_17
Discovery: Moderate
Exploitation: Concrete proof-bound gap; no forgery or key-recovery attack
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

Theorem 8.1.7 in the pinned [specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf) (physical
p. 106) gives SQIsign's EUF-CMA reduction in the
random-oracle model.  One of its nonnegative additive terms is

```text
(q + 1)(s + 1) / 2^(lambda + 1),
```

where `q` bounds random-oracle queries and `s` bounds signing-oracle queries.
NIST permits up to `s = 2^64` chosen-message signing queries when estimating
security.  With that value, this term reaches one by
`q = 2^(lambda-63)`: approximately `2^65`, `2^129`, and `2^193` oracle queries
for the three parameter sets.  These query exponents are far below the
corresponding 143-, 207-, and 272-gate category thresholds.

The term alone is about one half for the Level-I set when both `q` and `s` are
`2^64`; it exceeds one at roughly `2^65` random-oracle queries.  For Levels III
and V, its respective thresholds are roughly `2^129` and `2^193`.  Once the
term reaches one, the theorem's right-hand side supplies no nontrivial upper
bound on forgery probability. The separate final min-entropy term is

```text
(q + s + 1)s / (sqrt(p) 2^lambda).
```

At `q = 2^lambda` and `s = 2^64`, its base-2 logarithm is approximately
`-98.8`, `-188.4`, and `-270.0` for the three parameter sets. It therefore
remains small under the Round-3 primes and does not repair this
statistical-distance loss.

The theorem is stated only in the classical ROM and is accompanied by a proof
sketch based on an earlier Fiat–Shamir-with-hints theorem.  Remark 8.1.8 obtains
a QROM statement only in the Algebraic Isogeny Model.  Section 1.1's statement
that the reduction is tight is expressly qualified on accepting standard
heuristics; the query-dependent term above still limits the concrete guarantee
available from the displayed theorem.

This is Low / Proof gap.  It does **not** give an attack: no signature is
forged, no key is recovered, and the calculation does not estimate SQIsign's
actual forgery advantage.  It shows only that Theorem 8.1.7 does not certify
the claimed concrete security through NIST's signing-query baseline.  A tighter
reduction or an explicit, accepted per-key usage limit is needed to close it.

Recompute the terms with:

```sh
python3 SQIsign/reproduce_sqisign_5.py
```

The deterministic script checks the pinned specification and NIST baseline,
computes the exact first integer `q` that makes the term at least one, and
checks the final min-entropy term for every Round-3 prime.  It is a proof-bound
calculation, not an attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-5
```
