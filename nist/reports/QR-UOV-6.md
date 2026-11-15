# QR-UOV-6 — Informational: The rank threshold Delta of the Appendix B parameter sets is not stated in the specification

Severity: Informational
Status: Confirmed
Layer: Design
Affected: The nine Appendix B parameter sets (q = 31 and q = 7); the six main q = 127 sets are consistent
Discovery: Moderate
Exploitation: Specification omission; with the only stated value the Theorem 2 premise would fail within 2^64 signatures, but the code uses larger values and no consequence was demonstrated
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Omission

§4.5 (p.23) says "we set Delta = 2 for all parameter sets", and Table 2 gives
Delta = 2 for the six main q = 127 sets. Appendix B (pp.68-69) states no Delta
for its nine q = 31 and q = 7 sets, and no value other than 2 (and 0 in a
proof variant) appears anywhere in the specification. The submitted header
uses Delta = 3 for q = 31 and Delta = 4 for q = 7
(`qruov_param.h`, identical in the three trees), and signing rejects a
system of rank below `m − Delta`
(`qruov_common.c` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/qruov_common.c`)).
§4.5 introduces "the proposed QR-UOV parameter sets", so "all parameter sets"
may refer to the main sets only.

## Evidence

The reproducer pins the specification and the submitted files, checks the
quoted sentences, reads m for all fifteen sets from the header, and computes
the exact rank-failure probability `epsilon_rank` of a uniform matrix.

- With Delta = 2: `epsilon_rank = 2^-44.54` (q = 31) and `2^-25.01` (q = 7),
  so the Theorem 2 premise `qsign epsilon_rank < 1/2` fails at `2^43.5` and
  `2^24.0` signatures.
- With the header values: `2^-79.22` and `2^-69.93`; the premise holds
  through `2^64`.
- The header values are exactly the smallest Delta meeting the change-log
  rule (p.4) at the specification's own `2^61` limit.
- Control: for the six q = 127 sets the specification and header agree
  (`epsilon_rank = 2^-62.89`; the `2^64` failure there is
  [QR-UOV-2](/nist/reports/QR-UOV-2.html)).

## Classification and limits

Informational / Confirmed / Design: a parameter the security argument depends
on is not stated for the Appendix B sets. Delta affects only the signer's
retries, so signatures verify whatever value an implementation picks, and the
submitted code uses values for which the premise holds. `epsilon_rank` is the
specification's uniform-matrix estimate, not a measured rank law. Stating
Delta for every set would close the item.

## Reproduction

```sh
python3 QR-UOV/reproduce_qruov_6.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-6
```
