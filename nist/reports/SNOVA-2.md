# SNOVA-2 — Informational: historical block-structured wedge-product key recovery

Severity: Informational
Status: Confirmed
Layer: Design
Affected: Legacy Round-2 parameters; current On-Ramp Round 3 applicability is not established
Discovery: Non-trivial
Exploitation: Published candidate-specific cost estimates; no local full attack or current-key recovery
Remediation: None
Credit: Original cryptanalysis by Maxime Bros, Thai Hung Le, Jacob Lichtinger, Brice Minaud, Ray Perlner, Daniel Smith-Tone, and Cristian Valenzuela; On-Ramp Round 3 applicability reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The published estimates are High against the affected Round-2 parameter
claims. This report rates the record Informational because applicability
to the pinned On-Ramp Round 3 version is not established. Confirmed status rests on
the primary paper and the SNOVA team's acknowledgement, not a local cost
reproduction.

Bros, Le, Lichtinger, Minaud, Perlner, Smith-Tone and Valenzuela adapt the wedge-product method to SNOVA's block-ring structure. Their treatment removes extra/spurious solutions that obstruct a direct application of the generic UOV attack, and adds unbalanced projection of oil coordinates. The paper reports six of eleven Round-2 sets below their intended security levels, with two further sets receiving improved estimates. Its example `(v,o,q,l)=(29,6,16,5)` falls from the previously reported 310-bit estimate to about 181 bits. These are attack-complexity estimates, not a practical full-size key-recovery demonstration in this review. [Paper 2026/237](https://eprint.iacr.org/2026/237), latest listed revision September 16, 2026.

The [February 24 announcement and February 25 team response](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/z3v13roERpU) agree that affected parameters require adjustment. The response links the SNOVA team's own independently conducted analysis, [2026/260](https://eprint.iacr.org/2026/260), and discusses increasing vinegar dimensions. The block-structure adaptation and the team analysis concern the same attack family and are recorded together here.

A separate warning is essential when reading the history. Ran's original May 2025 generic wedge announcement included SNOVA estimates, but the [June 19 follow-up](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/NNv4C5PRIjs) withdrew those SNOVA complexities because the structured system need not have the assumed rank. The later SNOVA-specific work addresses that obstacle. The withdrawn generic estimate is not an independent confirmed SNOVA break.

The [pinned v3.0 specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), §1.9.3 and §5.3.5, changes the parameter framework: r is independent of l and m1 is independent of o. Its Table 5 recommends different seven-component tuples `(v,o,q,l,r,m1,m2)`; Table 6 also discusses alternatives. The old four-component example does not identify any current recommended instance. The specification explicitly analyzes wedge attacks, including projection, and cites this work. Those changes establish version and parameter differences; they do not independently validate every new security estimate.

There is no local full wedge-attack reproduction or demonstrated current-key recovery behind this report. The scheme KATs test functional compatibility and do not test wedge-attack resistance. Future reproduction must pin the attacked Round-2 package and field representation, include the block-ring corrections, state the cost/memory model, and compare the resulting oil subspace or a new-message forgery against a control. Running a generic UOV estimator with On-Ramp Round 3 labels would not reproduce the published attack.

Primary-source retrievals and failures are retained in [security/harvests.jsonl](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/harvests.jsonl); run `python3 security/harvest.py --list` to locate the registered wedge sources. [SNOVA-1](/nist/reports/SNOVA-1.html) is a distinct current-version sampling issue and must not be conflated with this historical key-only structural attack.

## Harness reproduction

This historical or withdrawn-claim report has no retained automated reproducer. `python3 nist/reproduce.py SNOVA-2` reports that limit explicitly.
