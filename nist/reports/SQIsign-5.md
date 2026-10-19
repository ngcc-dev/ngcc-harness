# SQIsign-5 — Low: pinned ROM bound loses every category guarantee before its target

Severity: Low
Status: Proof gap
Layer: Design
Affected: Theorem 8.1.7 guarantee for p324_3, p500_27, and p664_17
Discovery: Moderate
Exploitation: Concrete proof-bound gap for all three levels; no forgery or key-recovery attack
Remediation: Moderate
Credit: Later Round-2 proof analysis by Maher Mamah and David Jao; pinned-theorem and On-Ramp parameter evaluation by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

Theorem 8.1.7 in the pinned [specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf) (physical
p. 106) gives SQIsign's EUF-CMA reduction in the random-oracle model. One of
its nonnegative additive terms is

```text
(q + 1)(s + 1) / 2^(lambda + 1),
```

where `q` bounds random-oracle queries and `s` bounds signing-oracle queries.
At NIST's `s=2^64` evaluation baseline, the term reaches one around
`q=2^(lambda-63)`: approximately `2^65`, `2^129`, and `2^193` queries for
p324_3, p500_27, and p664_17. These thresholds precede the respective 143-,
207-, and 272-gate category targets.

The separate final min-entropy term printed in the theorem is

```text
(q + s + 1)s / (sqrt(p) 2^lambda).
```

At `q=2^lambda` and `s=2^64`, its base-2 logarithm is approximately -98.8,
-188.4, and -270.0. It remains small under the On-Ramp primes and does not
repair the earlier statistical-distance loss.

The theorem is stated only in the classical ROM. Remark 8.1.8 obtains a QROM
statement only in the Algebraic Isogeny Model. Section 8.1 also says that the
underlying complete proof concerns a previous SQIsign version with even-degree
responses. The v3 specification adapts that framework to odd-degree responses
but retains the displayed loss.

## Why the later Round-2 bound does not narrow this finding

Mamah and Jao's [security-proof note](https://eprint.iacr.org/2026/1775)
improves the Round-2 min-entropy analysis from `O(1/sqrt(p))` to `O(1/p)`.
Their Theorem 3 uses the commitment-degree condition `Nmix > p^2`. Under that
condition its two statistical terms are

```text
(q+1) 2 sqrt(s+1) / sqrt(p) + 16 (q+s+1)s / p.
```

SQIsign v3 does not meet the premise. Table 9 (physical p. 92) fixes

```text
Dmix = next_prime(p * 2^(2*lambda)).
```

Ignoring the negligible next-prime increment, this is below `p^2` by 69.58,
120.75, and 156.09 bits for the three On-Ramp sets. The v3 specification
accordingly cites the Mamah--Jao result only while saying tighter reductions
“may be possible” and leaves that work for the future immediately before
Theorem 8.1.7.

Directly substituting the v3 primes into the later formula gives hypothetical
thresholds near `2^129.79`, `2^219.38`, and `2^301.04` random-oracle queries,
but those numbers do not establish a v3 theorem. The previous report used them
to say that Levels III and V were covered. That narrowing was unsupported and
has been removed. The applicable pinned guarantee remains loose for all three
sets.

This remains **Low / Proof gap**. Neither calculation gives an attack: no
signature is forged, no key is recovered, and the proof term does not estimate
SQIsign's actual forgery advantage. A v3 proof satisfying the later theorem's
premises, a new tight analysis for the v3 commitment distribution, or an
accepted per-key query limit would close the gap.

Recompute the terms and the `Dmix < p^2` premise failure with:

```sh
python3 SQIsign/reproduce_sqisign_5.py
```

The script checks the pinned specification and NIST baseline, computes the
printed terms for every current prime, and reports the later formula only as
an inapplicable sensitivity calculation.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-5
```
