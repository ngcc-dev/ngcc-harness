# SNOVA-4 — Informational: withdrawn truncated-polynomial-ring complexity claim

Severity: Informational
Status: Confirmed
Layer: Design
Affected: Withdrawn SNOVA portion of the 2026/298 analysis; no current-version vulnerability asserted
Discovery: Non-trivial
Exploitation: None; the authors withdrew the SNOVA complexity evaluation
Remediation: None
Credit: Original analysis and withdrawal by Hiroki Furue and Yasuhiko Ikematsu; Round-3 scope reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

This entry prevents a superseded public announcement from being counted as an
additional confirmed break.

Furue and Ikematsu's February 2026 [pqc-forum announcement](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/vwv1BGxLwUI) describes key recovery through XL over p^l-truncated polynomial rings, including reconciliation and intersection attacks. The initial post says that the reconciliation analysis reduces estimates for Round-2 SNOVA sets with matrix dimension l=2. That SNOVA matrix dimension and the truncation exponent are different parameters despite their similar notation.

On June 18, 2026, the authors explicitly withdrew the SNOVA complexity analysis because an additional kernel invalidated the evaluation. The same thread links the revised [ePrint 2026/298](https://eprint.iacr.org/2026/298), whose history records revisions through June 8, 2026. Its UOV results are separate from the withdrawn SNOVA assessment; withdrawal of the latter does not retract the whole paper.

The [Round-3 SNOVA specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), §5.3.6, discusses p^l-truncated attacks and the extra-kernel issue. Its proposed handling and security margins are submitter analysis; they are not substituted here for an independent current-version attack.

This report has no exploit, no reproduced forgery or key recovery, and no current loss-of-security rating. It must not be aggregated with [SNOVA-2](/nist/reports/SNOVA-2.html) as two confirmed successful wedge-like attacks. The correct reproducible evidence is the dated claim followed by its author's dated correction, retained through [security/harvests.jsonl](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/harvests.jsonl), rather than a program that prints the original withdrawn complexity.

## Harness reproduction

This historical or withdrawn-claim report has no retained automated reproducer. `python3 nist/reproduce.py SNOVA-4` reports that limit explicitly.
