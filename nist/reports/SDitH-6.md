# SDitH-6 — Informational: Cipher-grinding pseudocode contradicts its own remark and the code

Severity: Informational
Status: Confirmed
Layer: Design
Affected: Specification of the six cipher-grinding (cipherpow) parameter sets; the hash-grinding sets and the submitted code are consistent
Discovery: Moderate
Exploitation: Specification inconsistency only; both readings need at least 2^lambda cipher calls, so no reduction in forgery work and no interoperability consequence was shown
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Inconsistency

In the cipher variant of ExpandEvaluationChallenge (physical p.21, printed
p.19), line 1 is `xof <- XOF.Init(h_piop, ctr)` and line 2 reads `p0, p1, k0,
k1` from it, so the cipher keys depend on the counter. The remark on physical
p.22 (printed p.20) says that lines 1-4 and the key schedule do not depend on
the counter. The code follows the remark: the cipher initialisation absorbs a
prefix byte and `h_piop` only and runs the key schedule once
(`sdith_prng.c:323-347` (`SDitH/submission/submission_package/Reference_Implementation/cat1_short_cipherpow/src/sdith_prng.c#L323`)), and each counter then
costs two cipher calls and tests `w_pow − 1` bits.

The parameters satisfy `tau kappa + w_pow − log2 d = lambda` exactly in all
twelve sets (leaving zero grinding margin), and only
`w_pow − 1` bits are tested, so a forger needs `2^(lambda−1)` counter values.
Under either reading each counter value costs two cipher calls, so a forger
needs `2^lambda` cipher calls; the literal line 1 adds one XOF call and two key
schedules per counter value. The inconsistency therefore does not reduce the
forgery work. It does leave the unit in which a future proof would count the
grinding undetermined.

Further text defects: line 2 calls `GetBytes(4 lambda)`, but only `GetByte()`
is defined, and the code draws `4 lambda` bits at Categories 1 and 5 but 128
bytes at Category 3, where the pseudocode implies 96; `Extract_(w_pow−1)` is
used with one index although only a two-index form is defined.

## Evidence

The reproducer checks each quoted line on its page of the pinned
specification, the code facts in all 24 parameter-set directories, and the
accounting from the submitted parameter table (cross-checked against
specification Tables 4 and 5). Control: for the six hash-grinding sets the
pseudocode and code agree, at `2^lambda` XOF calls.

## Classification and limits

Informational / Confirmed / Design: a specification inconsistency with no
demonstrated cost or interoperability consequence. An earlier version of this
report compared `2^(lambda−1)` XOF calls with `2^lambda` cipher calls and
called the difference one bit; in like units both readings need at least
`2^lambda` cipher calls. SDitH has no EUF-CMA proof yet (physical p.46). Whether signatures from a signer written
from the literal line 1 would verify against the code was not tested. Gate
costs were not compared. [SDitH-5](/nist/reports/SDitH-5.html) records five other
specification/code differences.

## Reproduction

```sh
python3 SDitH/reproduce_sdith_6.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-6
```
