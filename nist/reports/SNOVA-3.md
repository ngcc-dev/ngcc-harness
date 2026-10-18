# SNOVA-3 — Informational: historical weak public keys and reduced-rank whipping forgery

Severity: Informational
Status: Confirmed
Layer: Design
Affected: Earlier Round-1 design; mitigated in Round 2 and, per the submitters, by fixed ABQ in On-Ramp Round 3
Discovery: Non-trivial
Exploitation: Published weak-key universal forgery on historical configurations; no current On-Ramp Round 3 exploit reproduced locally
Remediation: None
Credit: Original cryptanalysis by Ward Beullens; On-Ramp Round 3 applicability reassessment by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The published result is High against the original affected configurations.
This report rates the record Informational because no attack is
established against the pinned On-Ramp Round 3 version. The paper includes a
practically breakable fraction of historical keys, but does not establish that
every original key can be broken practically.

Beullens identifies SNOVA's structured whipping transformation and uses rank reduction to make the remaining signature-forgery system easier. For the historical SNOVA-37-17-2 set, the paper estimates that roughly 1 in 500 public keys admits a universal forgery attack around 2^97 bit operations, while approximately 1 in 143,000 is breakable within minutes. Other parameter estimates improve by factors between 2^8 and 2^39. This is a forgery result, distinct from the later wedge-product oil-space recovery. [Improved Cryptanalysis of SNOVA, ePrint 2024/1297](https://eprint.iacr.org/2024/1297), received August 19, 2024; latest listed revision February 24, 2025.

The [SNOVA team's August 22, 2024 response](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/w_VTjKtIeAk) acknowledges problematic weak keys for l=2 and l=3 and temporarily discourages those sets. Its September 30 follow-up describes adjustments intended to retain the key/signature sizes. This response is mitigation evidence for a historical version, not proof that a particular new source build has been exhaustively analyzed.

The [On-Ramp Round 3 specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), physical pp.18–19, explicitly states that the Round-1 weak-key issue was resolved in the Round-2 update. On-Ramp Round 3 additionally fixes the public ABQ generation seed; Algorithm 7 uses `SNOVA_ABQ` for recommended sets and describes checking the generated Q matrices for nonzero values. The current header (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_S/snova.h#L37`) defaults `FIXED_ABQ` to 1 and `ROUND2_T12` to 0; the reference initialization (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_S/snova_ref.c#L231`) selects fixed ABQ generation through line 244. Current recommended parameter names and tuples differ from the original SNOVA-37-17-2 example.

These source observations explain why the historical random-ABQ weak-key frequency cannot simply be assigned to the pinned v3.0 recommended keys. They do not prove that every fixed whipping map is secure: the current specification's §5.2.2 still analyzes MinRank-assisted forgery and states heuristic rank-distribution assumptions.

No old-package weak-key search or universal-forgery experiment has been rerun here, and there is no demonstrated On-Ramp Round 3 exploit in this report. A proper reproduction requires the old generation rules and an independently verified new-message forgery, with ordinary non-weak keys as controls. Current KAT success is unrelated evidence. Retained primary-source provenance is in [security/harvests.jsonl](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/harvests.jsonl).

## Harness reproduction

This historical or withdrawn-claim report has no retained automated reproducer. `python3 nist/reproduce.py SNOVA-3` reports that limit explicitly.
